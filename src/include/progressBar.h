#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include <pthread.h>    //pthead_mutex_t
#include <stdint.h>     //uintxx_t types
#include <stdio.h>      //printf()
#include <sys/ioctl.h>  //ioctl()
#include <unistd.h>     //usleep()
#include "arguments.h"  //arguments structure

#define STD_TERM_WIDTH 80
#define SUBTRACT_SIZE 19

typedef struct
{
    bool encrypt;
    pthread_mutex_t* progress_mutex;
    uint64_t operations;
    uint64_t progress;
} progress_t;

/*Print an ascii progress bar of the form "Progress[############/             ]50%"
 * Loops until operations = progress
 */
void printProgressBar(bool* running, int* error, progress_t* p);

void setUpProgress(const arguments* args, progress_t* p, pthread_mutex_t* mutex);

#endif