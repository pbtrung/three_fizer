#ifndef DECRYPT_H
#define DECRYPT_H

#include <stdbool.h>        //bool types
#include <stdint.h>         //uint types
#include "cbcDecrypt.h"     //cbcXXXXDecrypt() functions
#include "threefishApi.h"   //ThreefishKey_t type
#include "tfHeader.h"       //stripIV()

bool decryptHeader(ThreefishKey_t* key,
                   uint64_t* header);

#endif