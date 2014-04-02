/*
 * -----------------------------------------------------------------------------
 * -----                             DATA.H                                -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the header file for data.c, the library of functions for
 *   interfacing the PIC18F67K22 with the Http Server.
 *
 * Assumptions:
 *   None
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   May  14, 2013      Nnoduka Eruchalu     Initial Revision
 */

#ifndef DATA_H
#define DATA_H

/* library include files */
#include <stdint.h>

/* local include files*/
#include "mifare.h"


/* DATA CONSTANTS */
/* none */


/* FUNCTION PROTOTYPES */
/* initializes the module and http_response structs */
extern void DataInit(void);

/* determine smartcard type server side */
extern uint8_t DataCardValidate(mifare_tag *tag);

/* validate pin on server side */
extern uint8_t DataPinValidate(uint8_t *uid, uint16_t pin);

/* get account balance (in kobos) */
extern uint32_t DataAcctBalance(uint8_t *uid);

/* recharge account with EasyTopup card */
extern uint8_t DataAcctRecharge(uint8_t *uid, uint8_t *topup_id, 
                                uint32_t *recharge_value);
/* get parking details */
extern uint8_t DataParkDetails(uint8_t *uid, uint32_t *space, int32_t *time);

/* pay for time at parking space */
extern void DataParkPay(uint8_t *uid, uint32_t space, int32_t *time);


/* alert routines */
void DataAlertPark(uint32_t space, int32_t time);


#endif                                                              /* DATA_H */
