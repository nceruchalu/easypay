/*
 * -----------------------------------------------------------------------------
 * -----                              AES.H                                -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the header file for aes.c, the library of functions for basic AES
 *   crypto operations
 * 
 * Copyright:
 *   This file includes and is based off of cryptographic software written by
 *   Eric Young (eay@cryptsoft.com)
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Jan. 06, 2013      Nnoduka Eruchalu     Initial Revision
 */

#ifndef AES_H
#define AES_H


/* -----------------------------------------------------
 * Constants
 * -----------------------------------------------------
 */

/* Because array size can't be a const in C, the following two are macros.
   Both sizes are in bytes. */
#define AES_MAXNR       14
#define AES_BLOCK_SIZE  16

/* -----------------------------------------------------
 * Data Types
 * -----------------------------------------------------
 */
typedef unsigned long u32;
typedef unsigned short u16;
typedef unsigned char u8;

struct aes_key_st {
  unsigned long rd_key[4 *(AES_MAXNR + 1)];
  int rounds;
};
typedef struct aes_key_st AES_KEY;

/* -----------------------------------------------------
 * Macros
 * -----------------------------------------------------
 */

# define GETU32(pt) (((u32)(pt)[0] << 24) ^ ((u32)(pt)[1] << 16) ^ ((u32)(pt)[2] <<  8) ^ ((u32)(pt)[3]))
# define PUTU32(ct, st) { (ct)[0] = (u8)((st) >> 24); (ct)[1] = (u8)((st) >> 16); (ct)[2] = (u8)((st) >>  8); (ct)[3] = (u8)(st); }



/* -----------------------------------------------------
 * Function Prototypes
 * -----------------------------------------------------
 */

extern int AES_set_encrypt_key(const unsigned char *userKey, const int bits,
                               AES_KEY *key);
extern int AES_set_decrypt_key(const unsigned char *userKey, const int bits,
                               AES_KEY *key);

extern int private_AES_set_encrypt_key(const unsigned char *userKey,
                                       const int bits, AES_KEY *key);
extern int private_AES_set_decrypt_key(const unsigned char *userKey,
                                       const int bits, AES_KEY *key);

extern void AES_encrypt(const unsigned char *in, unsigned char *out,
                           const AES_KEY *key);
extern void AES_decrypt(const unsigned char *in, unsigned char *out,
                           const AES_KEY *key);


#endif                                                               /* AES_H */
