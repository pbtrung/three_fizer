#include "include/controller.h"

//A sanity check for decrypting any files smaller then 3 blocks couldn't have been encrypted by this program
static inline bool isAtLeastThreeBlocks(arguments* args)
{
    //FUCK YOUR COUCH!
    return args != NULL && (args->file_size >= ((uint64_t)(args->state_size/8) * 3));
}

/* This takes the argument and hashed the user specified password into a key of block size
* or uses the user entered key directly if they have elected to use this option
* the key is then hashed again and this is ued as the hmac key.
* It assumes the user has correctly allocated block size buffers for the cipher and mac key and passes them into the function
*/
static bool handleKeys(const arguments* args, 
                       ThreefishKey_t* cipher_key, 
                       ThreefishKey_t* mac_key)
{
    pdebug("handleKeys()\n");
    bool success = true;
    if(cipher_key != NULL && mac_key != NULL)
    {
        uint64_t* _cipher_key = NULL;
        uint64_t* _mac_key = NULL;
        uint64_t block_byte_size = (uint64_t)args->state_size/8;

        if(args->hash == true)
        {
           //hash the user entered password so the key matches state size
          _cipher_key = (uint64_t*)sf_hash(args->password, args->pw_length, (uint64_t)args->state_size);
         }
         else
         {
            //use the user entered password as the key directly
           _cipher_key = noHashKey(args->password, args->pw_length, (uint64_t)args->state_size);
         }

         //hash the hmac key from the cipher_key
         _mac_key = (uint64_t*)sf_hash(_cipher_key, block_byte_size, (uint64_t)args->state_size);

         //initialize the key structures
         threefishSetKey(cipher_key, (ThreefishSize_t) args->state_size, _cipher_key, cipher_tweak);
                  
         threefishSetKey(mac_key, (ThreefishSize_t) args->state_size, _mac_key, mac_tweak);

         //free allocated resources
         if(_cipher_key != NULL) { free(_cipher_key); }
         if(_mac_key != NULL) { free(_mac_key); }
    }
    else { success = false; }

    return success;
}

/*TODO finish me
* this function will run hmac on every element in the hmac queue
* then queue the digest 
*/
static bool hmacData(ThreefishKey_t* hmac_key, queue* in, queue* out)
{
    pdebug("hmacData()\n");

    //iterate through the queue and
    while(in != NULL && front(in)->action != DONE)
    {
        //pop the current message in the queue out of the hmac stack
        chunk* cipher_text_to_write = front(in); 
        deque(in);
        //run the appropriate hmac function
        genMAC(hmac_key);
        if(enque(cipher_text_to_write, out) != true) 
        { 
            perror("Error queuing data for file write\n");
            return false; 
        }
        //que mac
    }

    //queue the HMAC here  

    return true;
} 

//Put a done flag into the Queue telling all threads using it that there operation on queued data is complete
static inline bool queueDone(queue* q)
{
    pdebug("queueDone()\n");
    bool success = false;
    chunk* done = createChunk();
    done->action = DONE;

    if(enque(done, q)) { success = true; }

    return success; 
} 

//queue IV and Header pair into cryptoQueue. Must be called before queueFile
static bool queueHeader(const arguments* args, queue* q)
{
    pdebug("queueHeader()\n");
    bool success = false;
    const uint64_t block_byte_size = ((uint64_t)args->state_size/8);
    uint64_t* iv = getRand((uint64_t) args->state_size);
    uint64_t* header_info = genHeader(iv, args->file_size, args->state_size);

    chunk* header = createChunk();
    header->action = ENCRYPT;
    header->data = calloc(1, 2*block_byte_size); 
    header->data_size = 2*block_byte_size;
    
    if(header->data != NULL) //check that allocate succeeded
    {
        //test that header was created in ram
        if(memcpy(header->data, iv, block_byte_size)) 
        { 
            success = true;
            enque(header, q); 
        }
        else 
        { 
            destroyChunk(header);
            perror("Error allocating memory for header cannot continue\n"); 
        }
    }

    if(iv != NULL) { free(iv); }
    if(header_info != NULL) { free(header_info); }
    return success;
} 

//Queues the file into the que passed in if threefizer is running in decrypt mode the header is queud as a seperate chunk
static bool queueFile(const arguments* args, queue* q) //right now this is blocking until the entire file is queued TODO put me on my own thread so MAC and ENCRYPTION can take place as the file is buffered not just after it
{
    pdebug("queueFile()\n");
    bool header = false;
    const uint64_t block_size = ((uint64_t)args->state_size/8); //get the threefish block size
    const uint64_t header_size = 2*(block_size);
    uint64_t file_size = getSize(args->argz); //get the file size in bytes
    FILE* read = openForBlockRead(args->argz); //open a handle to the file

    if(!args->encrypt) { header = true; } //if we are decrypting then we need to look for a header

    while(file_size > 0)
    {
         if(!queueIsFull(q))
         {
              chunk* newchunk = calloc(1, sizeof(chunk)); //create a new chunk        
              if(args->encrypt) { newchunk->action = ENCRYPT; } //set the action flag
              else { newchunk->action = DECRYPT; }            
              if(header) //decrypt mode then read the header into its own check to make checking it simpler
              {
                  if(!isAtLeastThreeBlocks(args)) //sanity check
                  {
                      fclose(read);
                      return false;
                  }
                  newchunk->data = readBlock(header_size, read);
                  newchunk->data_size = header_size;
                  file_size -= header_size;
                  header = false;
              }
              else //Normal Operation (not queueing just the header)
              {
                  //read the file into chunks
                  if(file_size < MAX_CHUNK_SIZE)
                  {
                      newchunk->data = pad(readBlock(file_size, read), file_size, args->state_size);
                      newchunk->data_size = getPadSize(file_size, args->state_size);
                      file_size -= file_size;
                  }
                  else
                  {
                      //read a chunk of the file
                      newchunk->data = readBlock(MAX_CHUNK_SIZE, read);
                      newchunk->data_size = MAX_CHUNK_SIZE;
                      file_size -= MAX_CHUNK_SIZE; //subtract the chunk size from the counter
                  }
              }
              if(newchunk->data == NULL) //file read failed for whatever reason
              { 
                  fclose(read);
                  return false; 
              } 
              enque(newchunk, q); //If the file read was succesfull que the chunk
         }
         //otherwise spin (this will create an infinite loop until threading is introduced)
    }

    fclose(read); //close the file handle
    return true;
}

static int decryptChunks(const ThreefishKey_t* tf_key, 
                         uint64_t* file_size, //a pointer to a 64bit integer that stores the file size as listed in the header  
                         queue* in, 
                         queue* out)
{
    pdebug("decryptChunks()\n");

    bool first_chunk = true;
    bool hmac_good = false;
    uint64_t* chain = NULL;

    while(front(in) != NULL && front(in)->action != DONE)
    {
       chunk* decrypt_chunk = front(in); //get the chunk on the top of the queue
       deque(in); //pop it off the queue   
     
       if(first_chunk)
       {
           decryptHeader(tf_key, decrypt_chunk->data);
           //check the header and only continue if its valid
           if(!checkHeader(decrypt_chunk->data, &file_size, tf_key->stateSize))
           {
               perror("Header check failed either the password is incorrect (most likely) or the file has been corrupted or the file was not encrypted with this program\n");
               return HEADER_CHECK_FAIL;
           }
           chain = getChain(decrypt_chunk->data, (uint64_t)tf_key->stateSize, 2);
           first_chunk = false;
           pdebug("read valid header\n");
       }
       else
       {
           uint64_t num_blocks = getNumBlocks(decrypt_chunk->data_size, (uint64_t)tf_key->stateSize);
           decryptInPlace(tf_key, chain, decrypt_chunk->data, num_blocks); //decrypt the chunk

           chain = getChain(decrypt_chunk->data, (uint64_t)tf_key->stateSize, num_blocks);
       } 

       if(enque(decrypt_chunk, out) != true) { return QUEUE_OPERATION_FAIL; }
    }
    return SUCCESS;
}

/************************************************************************************ 
* This encrypts all queued data passed to 'in' and puts it in the 'out' queue.
* this function assumes a properly formatted header starting with an IV
* is the first thing queued and an empty chunk with the action set to DONE is the
* last thing queued. 
*************************************************************************************/
static bool encryptChunks(const ThreefishKey_t* tf_key, queue* in, queue* out)
{
    /********************************************************
    * The encrypted file should be written like this        *
    * |HEADER|CIPHER_TEXT|MAC|                              *
    * note the MAC operation must include the entire header *
    * which in turn includes the IV                         *
    ********************************************************/
    pdebug("encryptChunks()\n");

    bool first_chunk = true;
    bool success = true;
    uint64_t* chain = NULL;

    while(front(in) != NULL && front(in)->action != DONE && success)
    {
        //get the top chunk in the queue
        chunk* encrypt_chunk = front(in); 
        deque(in); //pop it off the queue        

        if(first_chunk)
        {
            encryptHeader(tf_key, encrypt_chunk->data);
            chain = getChain(encrypt_chunk->data, (uint64_t)tf_key->stateSize, 2); //save the chain of cipher text for use in the next chunk           
            first_chunk = false;
        }
        else
        {
            uint64_t num_blocks = getNumBlocks(encrypt_chunk->data_size, (uint64_t)tf_key->stateSize);
            encryptInPlace(tf_key, chain, encrypt_chunk->data, num_blocks); //encrypt the chunk
            chain = getChain(encrypt_chunk->data, (uint64_t)tf_key->stateSize, num_blocks);
        }
        //que the encrypted chunk into the out buffer
        if(enque(encrypt_chunk, out) != true) { success = false; } 
    }
 
    pdebug("Before return\n");
    return success;
}

//Write everything queued to file
static bool asyncWrite(const arguments* args, queue* q, uint64_t max_file_size)
{
    pdebug("asyncWrite()\n");
    FILE* write = openForBlockWrite("test.fnsa");
    uint64_t bytes_written = 0;
    if(max_file_size == 0) { max_file_size = ULLONG_MAX; } //if max_file_size is 0 then set it to max effectively disabling max_file_size limits

    while(q != NULL && front(q)->action != DONE  && bytes_written < max_file_size)
    {
        chunk* chunk_to_write = front(q); //get the next chunk in the queue
        if(deque(q) == false) { return false; } //and pop it off and check for errors
        writeBlock((uint8_t*)chunk_to_write->data, chunk_to_write->data_size, write); //write it to file
        bytes_written += chunk_to_write->data_size;
        destroyChunk(chunk_to_write); //free the chunk 
    }

    terminateFile(write);
    return true;
}

int32_t runThreefizer(const arguments* args)
{
    queue* cryptoQueue = createQueue(QUE_SIZE);
    queue* macQueue = createQueue(QUE_SIZE);
    queue* writeQueue = createQueue(QUE_SIZE);
    static int32_t status = SUCCESS;
    static ThreefishKey_t tf_key; 
    static ThreefishKey_t mac_key; 
    uint64_t file_size = 0;

    pdebug("Threefizer controller\n");
    pdebug("Arguments { ");
    pdebug("free: %d, encrypt: %d, hash: %d, argz: [%s], argz_len: %zu, State Size: %u, password: [%s], pw_length %lu, file_length %lu }\n", args->free, args->encrypt, args->hash, args->argz, args->argz_len, args->state_size, args->password, args->pw_length, args->file_size);

    handleKeys(args, &tf_key, &mac_key); //generate and initialize the keys

    if(args->encrypt == true)
    { //most of this nesting will be done away with when threading is added to each queue
        //generate header and queue the file
        if(queueHeader(args, cryptoQueue) && queueFile(args, cryptoQueue))
        {
            //queue the done flag and start encryption
            if(queueDone(cryptoQueue)) 
            { 
                encryptChunks(&tf_key, cryptoQueue, macQueue);
                if(queueDone(macQueue))
                { //TODO finish me. This is temporary just to get encryption working
                    hmacData(&mac_key, macQueue, writeQueue);
                    queueDone(writeQueue);
                    //Do the hmac stuff and then move to the write queue
                    asyncWrite(args, writeQueue, 0); //which simply writes everything in it to file 
                } 
            }
            else { status = QUEUE_OPERATION_FAIL; }
        }
        else { status = FILE_IO_FAIL; }
    }
    else //decrypt
    { //the amount of nesting here is sketchy but I don't know how better to do it
        if(queueFile(args, cryptoQueue)) //in decrypt mode the first chunk queued will be the header
        {
            chunk* header = front(cryptoQueue);
            deque(cryptoQueue);
            destroyChunk(header);

            //check the header if it is valid continue with decrypt
            if(checkHeader(header->data, file_size , (uint32_t)args->state_size))
            {
                pdebug("Valid header encountered\n");
                status = decryptChunks(&tf_key, &file_size, cryptoQueue, macQueue);
            }
            else { status = HEADER_CHECK_FAIL; }
        }
        else { status = FILE_IO_FAIL; }
    }
    //spin up the crypto thread and sick it on the cryptoQueue
    //run hmac on each encrypted chunk when done queue it to the writeQueue
    //spin up the write thread and sick it on the writeQueue
    //free all allocated resources
    destroyQueue(cryptoQueue);
    destroyQueue(macQueue);
    destroyQueue(writeQueue);

    pdebug("Threefizer operation complete\n");

    return status;
}
