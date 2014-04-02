/*
 * -----------------------------------------------------------------------------
 * -----                          MIFARE_KEY.C                             -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is a library of functions for Mifare DESFire Crypto Key Operations
 *
 * Table of Contents:
 *   MifareDesKeyNew             - Create Mifare DESFire DES Key
 *   MifareDesKeyNewWithVersion  - Create Mifare Desfire DES key with version
 *   Mifare3DesKeyNew            - Create Mifare Desfire Triple DES key
 *   Mifare3DesKeyNewWithVersion - Create Mifare Desfire Triple DES key with vsn
 *   Mifare3k3DesKeyNew          - Create Mifare Desfire 3-key Triple DES key
 *   Mifare3k3DesKeyNewWithVersion - Create Mifare Desfire 3-key Triple DES key
 *                                   with version
 *   MifareAesKeyNew             - Create Mifare Desfire AES key
 *   MifareAesKeyNewWithVersion  - Create Mifare Desfire AES key with version
 *   MifareKeyGetVersion         - Get Mifare Desfire key version
 *   MifareKeySetVersion         - Set Mifare Desfire key version
 *   MifareSessionKeyNew         - Create Mifare Desfire Session Key
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Documentation:
 *   http://read.pudn.com/downloads134/ebook/572228/M309_Mifare&Security_V1.pdf
 *
 * Revision History:
 *   Jan. 18, 2013      Nnoduka Eruchalu     Initial Revision
 *   May  03, 2013      Nnoduka Eruchalu     Removed dynamic memory allocation
 *                                           So changed functions to accept
 *                                           pointers to pre-allocated data
 */

#include <string.h>   /* for mem* operations */
#include <stdlib.h>
#include "../general.h"
#include "mifare_key.h"
#include "des.h"

static void UpdateKeySchedules(mifare_desfire_key *key);


/*
 * UpdateKeySchedules
 * Description: Update Mifare DESFire key schedules.
 *
 * Operation: Update key schedules 1 and 2, and if doing 3-key triple DES 
 *            encryption, update key schedule 3.
 *  
 *
 * Revision History:
 *   Jan. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
static void UpdateKeySchedules(mifare_desfire_key *key)
{
  DES_set_key((DES_cblock *)key->data, &(key->ks1));
  DES_set_key((DES_cblock *)(key->data + 8), &(key->ks2));
  if (T_3K3DES == key->type) {
    DES_set_key ((DES_cblock *)(key->data + 16), &(key->ks3));
  }
}


/*
 * MifareDesKeyNew
 * Description: Create Mifare DESFire DES Key schedules with key data. Version 
 *              will be preset to 0x00 00
 *
 * Operation:   Set version bits of passed in data to 0, then create key with 
 *              version
 *
 * Revision History:
 *   Jan. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareDesKeyNew(mifare_desfire_key *key, uint8_t value[/*8*/])
{
  uint8_t data[8];
  int n;
  memcpy (data, value, 8);
  for (n=0; n < 8; n++)
    data[n] &= 0xfe;
  MifareDesKeyNewWithVersion(key, data);
  return;
}


/*
 * MifareDesKeyNewWithVersion
 * Description: Create Mifare DESFire DES Key schedules with key data containing
 *              version
 *
 * Operation:   Set type to DES, copy data to both halves of the key (effective 
 *              single DES) update key schedules
 *
 * Revision History:
 *   Jan. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareDesKeyNewWithVersion(mifare_desfire_key *key, uint8_t value[/*8*/])
{
  key->type = T_DES;
  memcpy(key->data, value, 8);
  memcpy(key->data+8, value, 8);
  UpdateKeySchedules (key);
  
  return;
}


/*
 * Mifare3DesKeyNew
 * Description: Create Mifare Desfire Triple DES key schedule with key data. 
 *              Version will be hardcoded to 0x00FF;
 *
 * Operation:   Set version of passed in data to 0x00FF, then create key with 
 *              version
 *
 * Revision History:
 *   Jan. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
void Mifare3DesKeyNew(mifare_desfire_key *key, uint8_t value[/*16*/])
{
  uint8_t data[16];
  int n;
  memcpy (data, value, 16);
  for (n=0; n < 8; n++)
    data[n] &= 0xfe;
  for (n=8; n < 16; n++)
    data[n] |= 0x01;
  Mifare3DesKeyNewWithVersion(key, data);
  return;
}


/*
 * Mifare3DesKeyNewWithVersion
 * Description: Create Mifare Desfire Triple DES key schedule with key data 
 *              containing version.
 *
 * Operation:   Set type to triple DES, copy data, and update key schedules
 *
 * Revision History:
 *   Jan. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
void Mifare3DesKeyNewWithVersion(mifare_desfire_key *key, uint8_t value[/*16*/])
{
  key->type = T_3DES;
  memcpy(key->data, value, 16);
  UpdateKeySchedules(key);
  return;
}


/*
 * Mifare3k3DesKeyNew
 * Description: Create Mifare Desfire 3-key Triple DES key schedule with key 
 *              data. Version will be hardcoded to 0x00 00 00
 *
 * Operation:   Set version of passed in data to 0x00 -- -- (first byte set to 
 *              0x00) 
 *              Then create key with version
 *
 * Revision History:
 *   Jan. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
void Mifare3k3DesKeyNew(mifare_desfire_key *key, uint8_t value[/*24*/])
{
  uint8_t data[24];
  int n;
  memcpy(data, value, 24);
  for (n=0; n < 8; n++)
    data[n] &= 0xfe;
  Mifare3k3DesKeyNewWithVersion(key, data);
  return;
}


/*
 * Mifare3k3DesKeyNewWithVersion
 * Description: Create Mifare Desfire 3-key Triple DES key schedule with key 
 *              data containing version
 *
 * Operation:   Create a DESFire key
 *              Set type to 3-key triple DES, copy data, and update key 
 *              schedules
 *
 * Revision History:
 *   Jan. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
void Mifare3k3DesKeyNewWithVersion(mifare_desfire_key *key, 
                                   uint8_t value[/*24*/])
{
  key->type = T_3K3DES;
  memcpy(key->data, value, 24);
  UpdateKeySchedules(key);
  
  return;
}


/*
 * MifareAesKeyNew
 * Description: Create Mifare Desfire AES key. Version will be preset to 0.
 *
 * Operation:   Create key with version set to 0
 *
 * Revision History:
 *   Jan. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareAesKeyNew(mifare_desfire_key *key, uint8_t value[/*16*/])
{
  MifareAesKeyNewWithVersion(key, value, 0);
  return;
}


/*
 * MifareAesKeyNewWithVersion
 * Description: Create Mifare Desfire AES key with version
 *
 * Operation:   Create a DESFire key
 *              Set type to AES, copy data, and version
 *
 * Revision History:
 *   Jan. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareAesKeyNewWithVersion(mifare_desfire_key *key, uint8_t value[/*16*/],
                                uint8_t version)
{
  memcpy(key->data, value, 16);
  key->type = T_AES;
  key->aes_version = version;
  return;
}


/*
 * MifareKeyGetVersion
 * Description: Get Mifare Desfire key version
 *
 * Operation:
 * 
 *   First 8 bytes of 128 bit 3DES key:
 *     byte 1   byte 2   byte 3   byte 4   byte 5   byte 6   byte 7   byte 8
 *   +--------+--------+--------+--------+--------+--------+--------+--------+
 *   |xxxxxxxP|xxxxxxxP|xxxxxxxP|xxxxxxxP|xxxxxxxP|xxxxxxxP|xxxxxxxP|xxxxxxxP|
 *   +--------+--------+--------+--------+--------+--------+--------+--------+
 *  
 *  Version:
 *    +----+----+----+----+----+----+----+----+
 *    | P1 | P2 | P3 | P4 | P5 | P6 | P7 | P8 |
 *    +----+----+----+----+----+----+----+----+
 *    Both keys must have same version number
 *
 * Revision History:
 *   Jan. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
uint8_t MifareKeyGetVersion(mifare_desfire_key *key)
{
  uint8_t version = 0;
  int n;
  for (n = 0; n < 8; n++) {
    version |= ((key->data[n] & 1) << (7 - n));
  }
  
  return version;
}


/*
 * MifareKeySetVersion
 * Description: Set Mifare Desfire key version
 *
 * Operation:   Grab each version bit and set appropriately.
 *              For 3DES, write inverse of version bit for 2nd key to avoid 
 *              turning a 3DES key into a DES key
 *
 * Revision History:
 *   Jan. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareKeySetVersion(mifare_desfire_key *key, uint8_t version)
{
  int n;
  for (n = 0; n < 8; n++) {
    uint8_t version_bit = ((version & (1 << (7-n))) >> (7-n));
    key->data[n] &= 0xfe;
    key->data[n] |= version_bit;
    if (key->type == T_DES) {
      key->data[n+8] = key->data[n];
    } else {
      /* Write ~version to avoid turning a 3DES key into a DES key */
      key->data[n+8] &= 0xfe;
      key->data[n+8] |= ~version_bit;
    }
  }
}


/*
 * MifareSessionKeyNew
 * Description: Create Mifare Desfire Session Key
 *
 * Operation: 
 *   session key =RndA(1st half) +RndB(1st half) +RndA(2nd Half) +RndB(2nd half)
 *   create session key based on key type.
 *
 * Revision History:
 *   Jan. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareSessionKeyNew(mifare_desfire_key *key,
                         uint8_t rnda[/*8*/], uint8_t rndb[/*8*/],
                         mifare_desfire_key *authentication_key)
{
  uint8_t buffer[24];
  
  switch (authentication_key->type) {
  case T_DES:
    memcpy(buffer, rnda, 4);                   /* RndA(1st half) */
    memcpy(buffer+4, rndb, 4);                 /* RndB(1st half) */
    MifareDesKeyNewWithVersion(key, buffer);
    break;
  case T_3DES:
    memcpy(buffer, rnda, 4);                   /* RndA(1st half) */
    memcpy(buffer+4, rndb, 4);                 /* RndB(1st half) */
    memcpy(buffer+8, rnda+4, 4);               /* RndA(2nd half) */
    memcpy(buffer+12, rndb+4, 4);              /* RndB(2nd half) */
    Mifare3DesKeyNewWithVersion(key, buffer); 
    break;
  case T_3K3DES:
    memcpy(buffer, rnda, 4);                   /* RndA(1st third) */
    memcpy(buffer+4, rndb, 4);                 /* RndB(1st third) */
    memcpy(buffer+8, rnda+6, 4);               /* RndA(2nd third) */
    memcpy(buffer+12, rndb+6, 4);              /* RndB(2nd third) */
    memcpy(buffer+16, rnda+12, 4);             /* RndA(3rd third) */
    memcpy(buffer+20, rndb+12, 4);             /* RndB(3rd third) */
    Mifare3k3DesKeyNew(key, buffer);
    break;
  case T_AES:
    memcpy(buffer, rnda, 4);                  
    memcpy(buffer+4, rndb, 4);
    memcpy(buffer+8, rnda+12, 4);
    memcpy(buffer+12, rndb+12, 4);
    MifareAesKeyNew(key, buffer);
    break;
  }
  
  return;
}
