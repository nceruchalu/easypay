/*
 * -----------------------------------------------------------------------------
 * -----                          MIFARE_CRYPTO.H                          -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the header file for mifare_crypto.c, the library of functions for
 *   Mifare DESFire Crypto Operations.
 *
 * Assumptions:
 *   None.
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Jan. 02, 2013      Nnoduka Eruchalu     Initial Revision
 *   May  03, 2013      Nnoduka Eruchalu     Removed dynamic memory allocation
 *                                           So changed functions to accept
 *                                           pointers to pre-allocated data
 */

#ifndef MIFARE_CRYPTO_H
#define MIFARE_CRYPTO_H

/* library include files */
#include <stdint.h>
#include <stdlib.h>  /* for size_t */

/* local include files */
#include "../general.h" /* for off_t  */


/* --------------------------------------
 * MIFARE Crypto Constants
 * --------------------------------------
 */
#define CRC32_POLY     0x04C11DB7
#define CRC32_POLY_REV 0xEDB88320   /* polynomial reversed */
#define CRC32_NUMBYTES 4
#define CRC32_NUMBITS  8*CRC32_NUMBYTES


/* --------------------------------------
 * MIFARE Crypto Data Types
 * --------------------------------------
 */
typedef enum {
  MCD_SEND,
  MCD_RECEIVE
} mifare_crypto_direction;

typedef enum {
  MCO_ENCIPHER,
  MCO_DECIPHER
} mifare_crypto_operation;


/* --------------------------------------
 * FUNCTION PROTOTYPES
 * --------------------------------------
 */
/* ROL operation */
extern void Rol(uint8_t *data, size_t len);

/* LSL operation */
extern void Lsl(uint8_t *data, size_t len);

/* Cmac Generate subkeys */
extern void CmacGenerateSubkeys(mifare_desfire_key *key);

/* MAC Generation process of CMAC */
extern void Cmac(mifare_desfire_key *key, uint8_t *ivect, uint8_t *data,
                 size_t len, uint8_t *cmac);

/* get a CRC32 checksum for data of len bytes */
extern void Crc32(uint8_t *data, size_t len, uint8_t *crc);

/* get a CRC32 checksum for data of len bytes, and append to data */
extern void Crc32Append(uint8_t *data, size_t len);

/* get a Desfire CRC32 checksum for data of len bytes */
extern void MifareCrc32(uint8_t *data, size_t len, uint8_t *crc);

/* get a Desfire CRC32 checksum for data of len bytes, and append to data */
extern void MifareCrc32Append(uint8_t *data, size_t len);

/* get a Desfire ISO1444-3 Type A CRC16 checksum for data of len bytes */
extern void MifareCrc16(uint8_t *data, size_t len, uint8_t *crc);

/* get a Desfire CRC16 checksum for data of len bytes, and append to data */
extern void MifareCrc16Append(uint8_t *data, size_t len);

/* key block size */
extern size_t KeyBlockSize(mifare_desfire_key *key);

/* size required to store nbytes of data in a buffer of size n*block_size */
extern size_t PaddedDataLength(size_t nbytes, size_t block_size);

/* buffer size required to MAC nbytes of data */
extern size_t MacedDataLength(mifare_desfire_key* key, size_t nbytes);

/* buffer size required to encipher nbytes of data and CRC. */
extern size_t EncipheredDataLength(mifare_tag* tag, size_t nbytes,
                                   int communication_settings);

/* Data Encipher before transmission */
extern uint8_t *MifareCryptoPreprocessData(mifare_tag *tag, uint8_t *data,
                                           size_t *nbytes, off_t offset,
                                           int communication_settings);

/* Data Decipher/Verification after reception */
extern uint8_t *MifareCryptoPostprocessData(mifare_tag *tag, uint8_t *data,
                                            ssize_t *nbytes,
                                            int communication_settings);

/* Mifare Cipher Single Block */
extern void MifareCipherSingleBlock(mifare_desfire_key *key, uint8_t *data,
                                    uint8_t *ivect,
                                    mifare_crypto_direction direction,
                                    mifare_crypto_operation operation,
                                    size_t block_size);

/* This function performs all CBC ciphering/deciphering. */
extern void MifareCipherBlocksChained(mifare_tag *tag, mifare_desfire_key *key,
                                      uint8_t *ivect, uint8_t *data,
                                      size_t data_size,
                                      mifare_crypto_direction direction,
                                      mifare_crypto_operation operation);


#endif                                                     /* MIFARE_CRYPTO_H */
