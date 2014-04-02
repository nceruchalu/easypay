/*
 * -----------------------------------------------------------------------------
 * -----                           SMARTCARD.H                             -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the header file for smartcard.c, the library of functions for
 *   interfacing the PIC18F67K22 with Mifare SmartCards.
 *
 * Assumptions:
 *   None
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   May  05, 2013      Nnoduka Eruchalu     Initial Revision
 *   May  06, 2013      Nnoduka Eruchalu     Updated to use mifare_tag_simple
 *   May  14, 2013      Nnoduka Eruchalu     removed unused scanning functions
 *                                          changed CARD_USER->CARD_TAP
 */

#ifndef SMARTCARD_H
#define SMARTCARD_H

/* library include files */
#include <stdint.h>

/* local include files*/
#include "mifare.h"


/* SMARTCARD CONSTANTS */
/* none */


/* SMARTCARD CODES */
#define CARD_TAP      0  /* user's EasyCard     */
#define CARD_TOPUP    1  /* EasyTopup Card      */
#define CARD_INVALID  2  /* invalid Card tapped */


/* FUNCTION PROTOTYPES */
/* initializes the card and the CardScan variables */
extern void CardInit(void);

/* checks if a smartcard has been tapped */
extern uint8_t IsACard(void);

/* Gets a smartcard code from the tapped card  */
extern uint8_t GetCard(void);

/* Get a pointer to PICC representation */
extern mifare_tag *GetCardTag(void);


#endif                                                         /* SMARTCARD_H */
