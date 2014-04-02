/*
 * -----------------------------------------------------------------------------
 * -----                          MIFARE_AUTH.H                            -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *  This is the header file for mifare_auth.c, the library of functions for
 *  automatic authentication.
 *
 * Compiler:
 *  HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *  May  04, 2013      Nnoduka Eruchalu     Initial Revision
 */
#ifndef MIFARE_AUTH_H
#define MIFARE_AUTH_H

/* library includes */
#include <stdint.h>

/* local includes */
#include "mifare.h"  

extern uint8_t mifare_key_data_null[8];
extern uint8_t mifare_key_data_des[8];
extern uint8_t mifare_key_data_3des[16];
extern uint8_t mifare_key_data_aes[16];
extern uint8_t mifare_key_data_3k3des[24];
extern const uint8_t mifare_key_data_aes_version;

void MifareAutoAuthenticate(mifare_tag *tag, uint8_t key_no);


#endif                                                       /* MIFARE_AUTH_H */
