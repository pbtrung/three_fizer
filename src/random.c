#include "include/random.h"

uint8_t* getRand(const uint64_t size)
{
    //pdebug("(getRand)\n");
    int randFile_fd = 0;
    uint8_t* randomData = NULL;
    if(exists((uint8_t*)HWRNG)) //try hardware random number generator first
    {
        randFile_fd = openForRead((uint8_t*)HWRNG);
        if(randFile_fd < 0)
        {
            fprintf(stderr, "Unable to open %s cannot continue\n", HWRNG);
            return NULL; 
        }
    }
    else if(exists((uint8_t*)PSRNG)) //use /dev/urandom since /dev/random blocks
    {
        randFile_fd = openForRead((uint8_t*)PSRNG);
        if(randFile_fd < 0)
        {
            fprintf(stderr, "Unable to open %s cannot continue\n", PSRNG);
            return NULL;
        }
    }

    randomData = readBytes(size, randFile_fd);
    close(randFile_fd);

    return randomData;
}
