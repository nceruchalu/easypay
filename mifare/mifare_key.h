/*
 * -----------------------------------------------------------------------------
 * -----                          MIFARE_KEY.H                             -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the header file for mifare_key.c, the library of functions for
 *   Mifare DESFire Crypto Key Operations.
 *
 * Assumptions:
 *   None.
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Jan. 18, 2013      Nnoduka Eruchalu     Initial Revision
 *   May  03, 2013      Nnoduka Eruchalu     Removed dynamic memory allocation
 *                                          So changed functions to accept
 *                                          pointers to pre-allocated data
 */

#ifndef MIFARE_KEY_H
#define MIFARE_KEY_H

/* library include files */
#include <stdint.h>

/* local include files */
#include "mifare.h"


/* --------------------------------------
 * FUNCTION PROTOTYPES
 * --------------------------------------
 */
/* Create Mifare Desfire DES key */
extern void MifareDesKeyNew(mifare_desfire_key *key, uint8_t value[/*8*/]);

/* Create Mifare Desfire DES key with version */
extern void MifareDesKeyNewWithVersion(mifare_desfire_key *key, 
                                       uint8_t value[/*8*/]);

/* Create Mifare Desfire Triple DES key */
extern void Mifare3DesKeyNew(mifare_desfire_key *key, uint8_t value[/*16*/]);

/* Create Mifare Desfire Triple DES key with version */
extern void Mifare3DesKeyNewWithVersion(mifare_desfire_key *key, 
                                        uint8_t value[/*16*/]);

/* Create Mifare Desfire 3-key Triple DES key */
extern void Mifare3k3DesKeyNew(mifare_desfire_key *key, uint8_t value[/*24*/]);

/* Create Mifare Desfire 3-key Triple DES key with version */
extern void Mifare3k3DesKeyNewWithVersion(mifare_desfire_key *key, uint8_t value[/*24*/]);

/* Create Mifare Desfire AES key */
extern void MifareAesKeyNew(mifare_desfire_key *key, uint8_t value[/*16*/]);

/* Create Mifare Desfire AES key with version */
extern void MifareAesKeyNewWithVersion(mifare_desfire_key *key, 
                                       uint8_t value[/*16*/], uint8_t version);

/* Get Mifare Desfire key version */
extern uint8_t MifareKeyGetVersion(mifare_desfire_key *key);

/* Set Mifare Desfire key version */
extern void MifareKeySetVersion(mifare_desfire_key *key, uint8_t version);

/* Create Mifare Desfire Session Key */
extern void MifareSessionKeyNew(mifare_desfire_key *key,
                                uint8_t rnda[/*8*/], uint8_t rndb[/*8*/],
                                mifare_desfire_key *authentication_key);


#endif                                                        /* MIFARE_KEY_H */
