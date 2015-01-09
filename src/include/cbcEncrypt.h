#ifndef CBCENCRYPT_H
#define CBCENCRYPT_H

#include "cbc.h" //Cipher size macros
#include <stdint.h> //uintxx_t types

/*
 * Used internally to cbc encrypt 256bit blocks
 */
void cbc256Encrypt(ThreefishKey_t* key,
		   const uint64_t* iv,
		   uint64_t* plain_text,
		   const uint64_t num_blocks);

/*
 * Used internally to cbc encrypt 512bit blocks
 */
void cbc512Encrypt(ThreefishKey_t* key,
		const uint64_t* iv,
		uint64_t* plain_text,
		const uint64_t num_blocks);

/*
 * Used internally to cbc encrypt 1024bit blocks
 */
void cbc1024Encrypt(ThreefishKey_t* key,
		    const uint64_t* iv,
		    uint64_t* plain_text,
		    const uint64_t num_blocks);

#endif