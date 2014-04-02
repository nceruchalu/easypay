/*
 * -----------------------------------------------------------------------------
 * -----                          MIFARE_AUTH.C                            -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *  This is the library of functions for automatic authentication.
 *
 * Compiler:
 *  HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *  May  04, 2013      Nnoduka Eruchalu     Initial Revision
 */

#include <stdint.h>
#include "mifare.h"
#include "mifare_auth.h"  

uint8_t mifare_key_data_null[8]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00};
uint8_t mifare_key_data_des[8]  = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};

uint8_t mifare_key_data_3des[16] = {'C', 'a', 'r', 'd', ' ', 'M', 'a', 's', 
                                    't', 'e', 'r', ' ', 'K', 'e', 'y', '!' };
uint8_t  mifare_key_data_aes[16]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00,0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00,0x00};
uint8_t mifare_key_data_3k3des[24]={0x00,0x01,0x00, 0x01, 0x00, 0x01,0x00,0x01, 
                                    0x00,0x00,0x00, 0x00, 0x00, 0x00,0x00,0x00,
                                    0x00,0x00,0x00, 0x00, 0x00, 0x00,0x00,0x00};

const uint8_t mifare_key_data_aes_version = 0x42;

/*
 * MifareAutoAuthenticate
 * Description:
 *  Return manufacturing related data of the PICC
 *  
 * Arguments: tag    - A DESFire tag
 *            key_no - Key to authenticate with
 * Return:    None
 *
 * Operation:
 *  Get the current key version of the specified key_no within the current appl.
 *  Use that to create a new key, and authenticate with that.
 *  
 *
 * Revision History:
 *  May  04, 2013      Nnoduka Eruchalu     Initial Revision
 */
void MifareAutoAuthenticate(mifare_tag *tag, uint8_t key_no) {
  
  uint8_t key_version;
  mifare_desfire_key key;
  
  /* Determine which key is currently the master one */
  MifareGetKeyVersion(tag, key_no, &key_version);
    
  switch (key_version) {
  case 0x00:
    MifareDesKeyNewWithVersion(&key, key_data_null);
    break;
  case 0x42:
    MifareAesKeyNewWithVersion(&key, mifare_key_data_aes, 
                               mifare_key_data_aes_version);
    break;
  case 0xAA:
    MifareDesKeyNewWithVersion(&key, mifare_key_data_des);
    break;
  case 0xC7:
    Mifare3DesKeyNewWithVersion(&key, mifare_key_data_3des);
    break;
  case 0x55:
    Mifare3k3DesKeyNewWithVersion(&key, mifare_key_data_3k3des);
    break;
  default:
    /* unknown master key */
    break;
  }

  /* Authenticate with this key */
  switch (key_version) {
  case 0x00:
  case 0xAA:
  case 0xC7:
    MifareAuthenticate(tag, key_no, &key);
    break;
  case 0x55:
     MifareAuthenticateIso(tag, key_no, &key);
     break;
  case 0x42:
    MifareAuthenticateAes(tag, key_no, &key);
    break;
  default:
    /* unknown master key*/
    break;
  }
}

