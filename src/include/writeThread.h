#ifndef WRITETHREAD_H
#define WRITETHREAD_H

#include <pthread.h> //pthead_mutex_t type
#include "chunkQueue.h" //queue data type
#include "debug.h" //pdebug()
#include "error.h" //error codes
#include "fileIO.h" //writeChunk()

typedef struct
{
    const arguments* args;
    bool* running;
    bool* valid;
    pthread_mutex_t* mutex;
    queue* in;
    const uint8_t* temp_file_name;
    uint32_t* error;
    uint64_t* file_size;
} writeParams;

void* asyncWrite(void* parameters);

void setUpWriteParams(writeParams* params,
                      const arguments* args,
                      bool* running,
                      bool* valid,
                      pthread_mutex_t* mutex,
                      queue* in,
                      const uint8_t* temp_file_name,
                      uint32_t* error,
                      uint64_t* file_size);

#endif