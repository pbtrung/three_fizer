#include "include/readThread.h"

//Queues the file into out if threefizer is running in decrypt mode the header is queud as a seperate chunk and the MAC is queued as a seperate chunk 
void* queueFile(void* parameters) //right now this is blocking until the entire file is queued TODO put me on my own thread so MAC and ENCRYPTION can take place as the file is buffered not just after it
{
    pdebug("queueFile()\n");
    bool header = false;
    chunk* header_chunk = NULL;
    chunk* data_chunk = NULL;
    chunk* mac_chunk = NULL;
    const readParams* params = parameters;
    const uint64_t block_byte_size = ((uint64_t)params->args->state_size/8); //get the threefish block size
    const uint64_t header_byte_size = 2*(block_byte_size);
    uint64_t orig_file_size = getSize(params->args->argz); //get the file size in bytes
    //ensure file size is big enough to continue
    if((params->args->encrypt && orig_file_size == 0) || 
       (!(params->args->encrypt) && !isGreaterThanThreeBlocks(params->args)))
    {
        if(!params->args->encrypt) { pdebug("File size of %lu is too small could not have been encrypted by this program\n", orig_file_size); }
        else { pdebug("File is empty cannot encrypt\n"); }

        *(params->error) = FILE_TOO_SMALL;

        return NULL;
    }

    FILE* read = openForBlockRead(params->args->argz); //open a handle to the file

    if(!params->args->encrypt) { header = true; } //if we are decrypting then we need to look for a header

    while(*(params->running) && *(params->error) == 0 && orig_file_size > 0)
    {
        //only read if we aren't waiting for something else to get queued
        if(header_chunk == NULL && data_chunk == NULL && mac_chunk == NULL)
        {
            //All files encrypted with this program are prepended by a header.
            //We know how big the header will be based on the state size of the
            //block cipher so we put the header in its own chunk to test it.   
            if(header)
            {
                header_chunk = createChunk();
                if(header_chunk == NULL)
                {
                    perror("Error allocating memory for file read\n");
                    fclose(read);
                    destroyChunk(header_chunk);
                    *(params->error) = MEMORY_ALLOCATION_FAIL;
                    break;
                }
                header_chunk->action = CHECK_HEADER;
                header_chunk->data = (uint64_t*)readBlock(header_byte_size, read);
                header_chunk->data_size = header_byte_size;
                pdebug("### Reading header of size %lu ###\n", header_chunk->data_size);
                orig_file_size -= header_byte_size;
                header = false;
            }
            //Files encrypted with this program have a MAC of 1 block appended
            //to the end of the file. In decrypt mode we assume this MAC is 
            //present and break it into its own chunk so it can be compared
            // to the MAC generated by the cipher text.
            if(!params->args->encrypt && orig_file_size <= MAX_CHUNK_SIZE) //end of the file in decrypt mode
            {
                data_chunk = createChunk();
                mac_chunk = createChunk();

                if(data_chunk == NULL || mac_chunk == NULL)
                {
                    perror("Error allocating memory for file read\n");
                    fclose(read);
                    destroyChunk(data_chunk);
                    destroyChunk(mac_chunk);
                    *(params->error) = MEMORY_ALLOCATION_FAIL;
                    break;
                }
                //Read the remaining file into a chunk
                data_chunk->action = GEN_MAC;
                data_chunk->data = (uint64_t*)readBlock(orig_file_size-block_byte_size, read);
                data_chunk->data_size = (orig_file_size-block_byte_size);
                pdebug("### Reading data of size %lu ###\n", data_chunk->data_size);

                //update the file size counter
                orig_file_size -= (orig_file_size-block_byte_size);
                //Read the appended MAC into its own chunk
                mac_chunk->action = MAC;
                mac_chunk->data = (uint64_t*)readBlock(orig_file_size, read);
                mac_chunk->data_size = orig_file_size;
                pdebug("### Reading mac of size %lu ###\n", mac_chunk->data_size);
                orig_file_size = 0; //the MAC is always the last thing in the file
            }
            else if(orig_file_size <= MAX_CHUNK_SIZE) //end of the file in encrypt mode
            {
                data_chunk = createChunk();
                if(data_chunk == NULL)
                {  
                    perror("Error allocating memory for file read\n");
                    fclose(read);
                    destroyChunk(data_chunk);
                    *(params->error) = MEMORY_ALLOCATION_FAIL;
                    break;
                }
                data_chunk->action = ENCRYPT;
                data_chunk->data = pad(readBlock(orig_file_size, read),
                                                 orig_file_size,
                                                 params->args->state_size);
                data_chunk->data_size = getPadSize(orig_file_size, params->args->state_size);
                pdebug("### Reading data of size %lu ###\n", data_chunk->data_size);
                orig_file_size -= orig_file_size; 
            }
            else //read a full data chunk of the file
            {
                data_chunk = createChunk();
                if(data_chunk == NULL)
                {
                    perror("Error allocating memory for file read\n");
                    fclose(read);
                    destroyChunk(header_chunk);
                    *(params->error) = MEMORY_ALLOCATION_FAIL;
                    break;
                }
                data_chunk->action = params->args->encrypt ? ENCRYPT : GEN_MAC;
                data_chunk->data = (uint64_t*)readBlock(MAX_CHUNK_SIZE, read);
                data_chunk->data_size = MAX_CHUNK_SIZE;
                pdebug("### Reading data of size %lu ###\n", data_chunk->data_size);
                orig_file_size -= MAX_CHUNK_SIZE; //subtract the chunk size from the counter
            } //end queue operation
        } //end nullcheck

        //Queue any read chunks in their prefered order
        if(header_chunk != NULL) //attempt to queue the header
        {
            pthread_mutex_lock(params->mutex);
            if(!queueIsFull(params->out))
            {
                if(enque(header_chunk, params->out))
                {
                    pdebug("### Queueing header of size %lu ###\n", header_chunk->data_size);
                    header_chunk = NULL;
                }
            }
            pthread_mutex_unlock(params->mutex);
        }
        if(data_chunk != NULL) //attempt to queue any data (this will be done the most)
        {
            pthread_mutex_lock(params->mutex);
            if(!queueIsFull(params->out))
            {
                if(enque(data_chunk, params->out))
                {
                    pdebug("### Queueing data of size %lu ###\n", data_chunk->data_size);
                    data_chunk = NULL;
                }
            }
            pthread_mutex_unlock(params->mutex);
        }
        if(mac_chunk != NULL) //attempt to queue the crypto text MAC
        {
            pthread_mutex_lock(params->mutex);
            if(!queueIsFull(params->out))
            {
                if(enque(mac_chunk, params->out))
                {
                    pdebug("### Queueing mac of size %lu ###\n", mac_chunk->data_size);
                    mac_chunk = NULL;
                }
            }
            pthread_mutex_unlock(params->mutex);
        }
        //otherwise keep looping until the queue is not full
    } //end of while loop

    //queue Done flag
    while(queueIsFull(params->out));
    pthread_mutex_lock(params->mutex);
    if(!queueDone(params->out))
    {
        pdebug("Error queueing done\n");
        *(params->error) = QUEUE_OPERATION_FAIL;
        return NULL;
    }
    pthread_mutex_unlock(params->mutex);
    pdebug("### Done queued ### \n");


    fclose(read); //close the file handle
    pdebug("readThread() success\n");

    return NULL;
}

inline void setUpReadParams(readParams* read_params, 
                     const arguments* args, 
                     bool* running,
                     pthread_mutex_t* mutex, 
                     queue* out,
                     uint32_t* error)
{
    read_params->args = args;
    read_params->running = running;
    read_params->mutex = mutex;
    read_params->out = out;
    read_params->error = error;
}
