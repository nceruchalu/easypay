/*
 * -----------------------------------------------------------------------------
 * -----                          MIFARE_AID.C                             -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is a library of functions for Mifare DESFire AID Operations
 *
 * Table of Contents:
 *   MifareAidNew        - Create Mifare DESFire AID in little endian format
 *   MifareAidGetAid     - Get Mifare DESFire AID
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Jan. 18, 2013      Nnoduka Eruchalu     Initial Revision
 *   May  03, 2013      Nnoduka Eruchalu     Removed dynamic memory allocation
 *                                           So changed functions to accept
 *                                           pointers to pre-allocated data
 */

#include <string.h>   /* for mem* operations */
#include <stdlib.h>
#include "mifare_aid.h"



/*
 * MifareAidNew
 * Description: Create Mifare DESFire AID in little endian format
 *
 * Arguments:   res:    pointer to structure to save aid into [modified]
 *              aid:    3 byte data value to save
 * Return:      None
 *
 * Operation:   Use mifare_desfire_aid data structure to save in little endian 
 *              array
 *
 * Error Checking: Return NULL if there is an error (aid is too big, not enough 
 *                 memory)
 *
 * Revision History:
 *   Jan. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareAidNew(mifare_desfire_aid *res, uint32_t aid)
{
  int i;
  
  if (aid > 0x00FFFFFF) 
    return;                            /* aid can only be 24 bits */

  for(i=0; i < 3; i++) {               /* save AID in little endian */
    res->data[i] = (aid >> (8*i)) & 0xFF;
  }
  return;
}


/*
 * MifareAidGetAid
 * Description: Get Mifare DESFire AID
 *
 * Operation:   Reconstruct from Little Endian Array to 24 bit data.
 *
 * Revision History:
 *   Jan. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
uint32_t MifareAidGetAid(mifare_desfire_aid *aid)
{
  return ((uint32_t) aid->data[0] | ((uint32_t) aid->data[1] << 8) | 
          ((uint32_t) aid->data[2] << 16));
}
