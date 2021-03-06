#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include "skeinApi.h"    //SkeinSize_t type
#include <stdbool.h>     //bool type
#include <stdint.h>      //uint_t types

typedef struct //arguments passed to argp containing argz (all non optional args)
{
     uint8_t* key_file;
     uint8_t* password;
     uint8_t* salt;
     uint8_t* rename_file;
     uint8_t* target_file;
     bool encrypt;
     bool free;
     bool hash;
     bool hash_from_file;
     bool rename;
     SkeinSize_t state_size;
     uint64_t file_size;
     uint64_t pw_length;
     uint64_t st_length;
} arguments;

void initArguments(arguments* arg);

#endif
