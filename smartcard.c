/*
 * -----------------------------------------------------------------------------
 * -----                           SMARTCARD.C                             -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the library of functions for interfacing the PIC18F67K22 with 
 *   Mifare SmartCards.
 *
 * Table of Contents:
 *  (private)
 *   CardValidate    - validate a tapped card and return it's card code.
 *
 *  (public)
 *   CardInit        - initializes the card and the CardScan variables
 *   IsACard         - checks if a smartcard has been tapped
 *   GetCard         - get smartcard details
 *   GetCardTag      - get a pointer to smartcard representation
 *
 * Assumptions:
 *   Hardware Hookup defined in include file.
 *
 * Limitations:
 *   - Simultaneous multiple cards are invalid. This is collision and won't be
 *     registered as a card tap.
 *   - If a card tap has been registered and not processed, and another card is
 *     tapped, the first card's info will be lost.
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   May  05, 2013      Nnoduka Eruchalu     Initial Revision
 *   May  06, 2013      Nnoduka Eruchalu     Updated to use mifare_tag_simple
 *   May  07, 2013      Nnoduka Eruchalu     Back to mifare_tag and no longer
 *                                           interrupt driven.
 *   May  14, 2013      Nnoduka Eruchalu     changed CARD_USER->CARD_TAP
 *   May  15, 2013      Nnoduka Eruchalu     Add call to DataCardValidate
 *   May  25, 2013      Nnoduka Eruchalu     Remove call to DataCardValidate
 */
#include "general.h"
#include <stdint.h>
#include "smartcard.h"
#include "mifare.h"
#include "string.h" /* for memcmp */

/* shared variables have to be local to this file */
static uint8_t cardDetected; /* (bool) has a card tap been registered? */
static uint8_t cardValue;    /* cardcode of last tapped card */
static mifare_tag tag;       /* MIFARE tag holding user info */


/* local functions that need not be public */
static uint8_t CardValidate(mifare_tag *tag);


/*
 * CardValidate
 * Description: Validate a tapped card and return it's card code
 *
 * Arguments:   tag: MIFARE tag
 * Return:      Smarcard Code - CARD_TAP, CARD_TOPUP, or CARD_INVALID
 *
 * Operation:   Determine card type using following checks:
 *              - CARD_TAP (EasyCard):
 *                DESFire Card: SL032 card type is MIFARE_CARD_DES
 *                Local CardType: Saved in an application's file on card says 
 *                                "easycard"
 *                Server CardType: Server side check of UID returns "easycard"
 *  
 *              - CARD_TOPUP (EasyTopup):
 *                Classic Card: SL032 card type is MIFARE_CARD_1K_[4|7]
 *                Local CardType: saved in a sector on card says "easytopup"
 *                Server CardType: Server side check of UID returns "easytopup"
 *
 *  Todo:         Actually do a check based on card values
 *  
 * Revision History:
 *   May 05, 2013      Nnoduka Eruchalu     Initial Revision
 *   May 15, 2013      Nnoduka Eruchalu     Add server side check with
 *                                          DataCardValidate()
 *   May 25, 2013      Nnoduka Eruchalu     Remove call to DataCardValidate()
 */
static uint8_t CardValidate(mifare_tag *tag)
{
  uint8_t card_type = CARD_INVALID;    /* start by assuming invalid card */

  uint8_t tapcard_uid[7] =   {0x04, 0x53, 0x16, 0x7A, 0xEC, 0x22, 0x80};
  uint8_t topupcard_uid[7] = {0x04, 0x3B, 0x11, 0x7A, 0xEC, 0x22, 0x80};
  
  if(memcmp(tapcard_uid, tag->uid, 7) == 0)
    card_type = CARD_TAP;
  else if (memcmp(topupcard_uid, tag->uid, 7) == 0)
    card_type = CARD_TOPUP;
  else
    card_type = CARD_INVALID;

  return card_type;
}


/*
 * CardInit
 * Description: This procedure initializes the shared variables.
 *
 * Arguments:   None
 * Return:      None
 *
 * Operation:   Put the card reader in a state of no scanning and no detected 
 *              tags, by setting canScan and cardDetected to FALSE.
 *              Also the cardValue's type is set to an invalid card code.
 *              Initialize the Mifare ISR Timer to a clear state and initialize 
 *              the tag representation.
 *  
 * Revision History:
 *   May 05, 2013      Nnoduka Eruchalu     Initial Revision
 */
void CardInit(void)
{
  cardDetected = FALSE;        /* no detected card tap */
  cardValue= CARD_INVALID;     /* start with an invalid card type */
  MifareStartTimer(0);         /* reset Mifare Timer */
  MifareTagInit(&tag);         /* initialize tg object */
}


/*
 * IsACard
 * Description: This procedure checks if a card has been tapped and detected.
 *
 * Arguments:   None
 * Return:      Boolean:
 *              - TRUE : A fully debounced key is available
 *              - FALSE: A fully debounced key is not available.
 *
 * Operation:   This function returns the value in cardDetected
 *
 * Revision History:
 *   May 05, 2013      Nnoduka Eruchalu     Initial Revision
 */
uint8_t IsACard(void)
{
  if (cardDetected == FALSE) {

    if (MifareDetect(&tag) == SUCCESS) {
      cardDetected = TRUE;
    }
  }
  return cardDetected;
}


/*
 * GetCard
 * Description: This returns the code for a detected card tap. The code tells 
 *              the type of card, EasyCard, EasyTopup, Invalid.
 *              It is a blocking function.
 *              Note that this procedure returns with the card code after the
 *              card has been tapped but not necessarily taken out of the 
 *              reader's range.
 * 
 * Arguments:   None
 * Return:      Verified card code: CARD_TAP, CARD_TOPUP, CARD_INVALID
 *
 * Operation:   A while loop runs until IsACard indicates that there is now a 
 *              detected card. When there is a card, the the card is verified 
 *              and a cardCode is obtained.
 *              Now that the last detected card has been verified, cardDetected 
 *              is reset to False, and the cardScanner is turned off.
 *
 * Revision History:
 *   May 05, 2013      Nnoduka Eruchalu     Initial Revision
 */
uint8_t GetCard(void)
{
  while(!IsACard())             /* block until there is a card tap */
    continue;
  
  cardDetected = FALSE;        /* reset reader to a state of no detected card */
  
  cardValue = CardValidate(&tag);
  return cardValue;            /* return card code */
}


/*
 * GetCardTag
 * Description: Get the pointer to the PICC representation
 *
 * Arguments:   None
 * Return:      Address of tag
 *
 * Operation:   Return the address of tag.
 *
 * Revision History:
 *   May 05, 2013      Nnoduka Eruchalu     Initial Revision
 */
mifare_tag *GetCardTag(void)
{
  return &tag;
}
