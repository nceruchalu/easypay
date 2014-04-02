/*
 * -----------------------------------------------------------------------------
 * -----                          MIFARE_AID.H                             -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the header file for mifare_aid.c, the library of functions for
 *   Mifare DESFire AID Operations.
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

#ifndef MIFARE_AID_H
#define MIFARE_AID_H

/* library include files */
#include <stdint.h>    /* for uint*_t */

/* local include files */
#include "mifare.h"


/* --------------------------------------
 * FUNCTION PROTOTYPES
 * --------------------------------------
 */
/* Create Mifare Desfire Aid */
extern void MifareAidNew(mifare_desfire_aid *res, uint32_t aid);

/* get Mifare Desfire Aid */
extern uint32_t MifareAidGetAid(mifare_desfire_aid *aid);


#endif                                                        /* MIFARE_AID_H */
