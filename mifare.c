/*
 * -----------------------------------------------------------------------------
 * -----                             MIFARE.C                              -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is a library of functions for interfacing the PIC18F67K22 with the
 *   SL032, a MIFARE DESFire reader/writer module.
 *
 * Table of Contents:
 *   MifareStartTimer        - start a countdown timer with a Timer
 *   MifareTimerISR          - Timer interrupt service routine.
 *   MifarePutBuf            - output a buffer of bytes to the serial channel
 *   MifareGetBuf            - get a buffer of bytes from the serial channel
 *   MifareTagInit           - initialize a MIFARE DESFire tag 
 *   MifareDetect            - detect a card within range
 *   MifareConnect           - establish connection to the provided tag.
 *   MifareDisconnect        - terminate connection with the provided tag
 *  
 * Limitations:
 *   None
 *
 * Documentation Sources:
 *   - libfreefare
 *
 *   - Draft ISO/IEC FCD 14443-4
 *     Identification cards
 *     + Contactless integrated circuit(s) cards
 *      + Proximity cards
 *        + Part 4: Transmission protocol
 *     Final Committee Draft - 2000-03-10
 *   
 *   - SL032 user manual
 *   - DESFire datasheet
 * -http://ridrix.wordpress.com/2009/09/19/mifare-desfire-communication-example/
 *   
 *
 * Assumptions:
 *   - Data Received from PICC ends with status byte
 *     + The SL032 actually returns the data with status byte ahead of data, so
 *       it should be copied to be after the data.
 *   - When CryptoPreprocessing data, can use an offset of 0 if will be using
 *     MDCM_PLAIN communication mode.
 *   - the SL032's select command buffer fits into Serial's Tx Queue.
 * 
 * Todo:
 *   Replace this weak code with the code in `mifare/`
 * 
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Dec. 28, 2012      Nnoduka Eruchalu     Initial Revision
 *   May  03, 2013      Nnoduka Eruchalu     Removed dynamic memory allocation
 *                                           So changed functions to accept
 *                                           pointers to pre-allocated data
 *   May  04, 2013      Nnoduka Eruchalu     Added AuthenticateIso and 
 *                                           AuthenticateAes
 *   May  06, 2013      Nnoduka Eruchalu     Added mifare_tag_simple struct and
 *                                           used for MifareDetect functions.
 *   May  07, 2013      Nnoduka Eruchalu     Simplified this for demo project
 */

#include "general.h"
#include <string.h>   /* for mem* operations */
#include <stdlib.h>
#include "mifare.h"
#include "serial.h"


/* shared variables have to be local to this file */
static unsigned int timer;                    /* serial comm. ms time counter */
static unsigned char timerOvertime;           /* serial comm. timeout flag */
static unsigned char uartStatus;              /* SL032 uart channel status */
static unsigned char rxBuf[MAX_FRAME_SIZE+5]; /* serial channel Rx buffer */
                                              /* +5 for SL032 comm. bytes */

static uint8_t SelectCard[3] = {0xBA, 0x02, SL_SELECT_CARD};/* SL032 commands */
static uint8_t RATSDesfire[3]= {0xBA, 0x02, SL_RATS};


/* SL032 specific defines */
#define SL032_RXCMD  rxBuf[2]    /* SL032 Rx Command is 3rd Uart Rx byte */
#define SL032_RXSTA  rxBuf[3]    /* SL032 Rx Status is 4th Uart Rx byte */

/* SL032 defines during T=CL exchange */
#define MF_RXSTA     rxBuf[4]    /* DESFire Rx Status: 1st Data byte */
#define MF_RXDATA    (&rxBuf[5]) /* DESFire Rx Data array (excluding status) */
#define MF_RXLEN     rxBuf[1]-3  /* Len of DESFire Rx data (including status) */
                                 /* subtract 0x21, Status, Checksum from SL032*/


/*
 * MifareStartTimer
 * Description: Start a countdown timer with a Timer. Countdown for passed in 
 *              milliseconds.
 *
 * Arguments:   ms: countdown time in milliseconds
 * Return:      None
 *
 * Operation:   Start timer countdown to passed in argument, and clear out 
 *              overtime flag, as timer hasn't timed out yet.
 *
 * Limitations: Critical code on timerOvertime
 *
 * Revision History:
 *   Dec. 30, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareStartTimer(unsigned int ms)
{
  timer = ms;             /* Start timer */
  timerOvertime = FALSE;  /* timer hasn't timed out yet */
}


/*
 * MifareTimerISR
 * Description: Interrupt Service Routine for a Timer.
 *
 * Arguments:   None
 * Return:      None
 *
 * Operation:   On each ISR routine call, decrement the timer. When it hits 0, 
 *              the timer has hit timed out/hit overtime, so set the overtime 
 *              flag.
 * 
 * Limitations: Critical code on timerOvertime
 *
 * Revision History:
 *   Dec. 30, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareTimerISR(void)
{
  if(timer > 0) {                          /* if timer hasn't timed out */
    timer--;                               /* decrement it; set the overtime */
    if(timer == 0) timerOvertime = TRUE;   /* flag when it does time out */
  }
}


/*
 * MifarePutBuf (through the SL032)
 * Description:
 *   This function outputs the passed buffer to the serial channel, byte by 
 *   byte.
 *   The bytewise outputting is done using SerialPutChar.
 *
 * Arguments:
 *   - command array, each slot representing a byte as described in 
 *     Communication Format below. This array however shouldnt contain Checksum 
 *     byte. This will be computed on the fly.
 *   - command array size
 * Return:
 *   None
 *
 * Communication Format (MCU to SL032):
 *   |----------|-----|---------|------|----------|
 *   | Preamble | Len | Command | Data | Checksum |
 *   |----------|-----|---------|------|----------|
 *   Preamble : 1 byte equal to 0xBA
 *   Len      : 1 byte indicating the number of bytes from Command to Checksum 
 *   Command  : 1 byte SL032 Command code
 *   Data     : Variable length; depends on the command type
 *   Checksum : 1 byte XOR of all the bytes from Preamble to Data
 *
 * Operation:
 *   This function loops through the buffer and outputs the bytes using
 *   SerialPutChar. After outputting these bytes, it outputs the checksum which
 *   is  cumulative XOR of all the buffer bytes.
 *   End this by updating Uart Status to indicate a successful Tx.
 *
 * Limitations:
 *   size argument >= 3
 *
 * Revision History:
 *   Dec. 30, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifarePutBuf(unsigned char *buffer, unsigned char size)
{
  unsigned char i = 0;          /* buffer index */
  unsigned char data;           /* data byte */
  unsigned char checksum = 0;   /* SL032 checksum is XOR of all bytes */
  
  uartStatus = MF_UARTSTATUS_TX; /* about to start Tx */
  
  while(i < size) {             /* loop through buffer slots */
    data = buffer[i];           /* extract data byte */
    checksum ^= data;           /* checksum is cummulative */
    i++;                        /* keep going through the buffer */
    SerialPutChar(data);        /* output buffer contents */
  }
  SerialPutChar(checksum);      /* finally, output checksum byte */
  
  uartStatus = MF_UARTSTATUS_TXSUCC; /* a successful Tx */
}


/*
 * MifareGetBuf (from the SL032)
 * Description:
 *   This function gets data from the SL032 and saves in MifareRxBuf.
 *
 * Communication Format (SL032 to MCU):
 *   |----------|-----|---------|--------|------|----------|
 *   | Preamble | Len | Command | Status | Data | Checksum |
 *   |----------|-----|---------|--------|------|----------|
 *   Preamble : 1 byte equal to 0xBD
 *   Len      : 1 byte indicating the number of bytes from Command to Checksum 
 *   Command  : 1 byte SL032 Command code
 *   Status   : 1 byte SL032 Status code
 *   Data     : Variable length; depends on the command type
 *   Checksum : 1 byte XOR of all the bytes from Preamble to Data
 *
 * Arguments:
 *   None
 * Return:
 *   None
 *
 * Operation:
 *   First assume success on this Rx Sequence.
 * 
 *   Keep track of the number of received bytes using the 0-indexed rxCount.
 *   This will double as index into the rx buffer, and so always 1 less than
 *   actual number of rx'd bytes.
 *
 *   Next is to get into a loop and stay in that loop while we have received 
 *   less than 2 bytes, or more than 2 bytes while still less than the total 
 *   number of bytes (Len + 2), where "Len" is as described above.
 *   In each iteration, set timer and wait for a timeout or a serial byte. If
 *   there is a timeout record an rx error and break from the loop.
 *   If there isnt a timeout, grab a byte and compute cummulative checksum.
 *
 *   When evaluating the checksum, instead of XORing only from Preamble
 *   to Data, its a lot easier to XOR all bytes, and if the values are right, 
 *   the result should be 0. This is because XORing a value with itselt yields 0
 *
 *   Note that checking for overtime is about more than just checking the flag.
 *   There is critical code surrounding this flag, so also check the timer
 *   counter is 0.
 *
 *   When done receiving, zero out the unused rx buffer slots.
 *   checking and returns with a SUCCESS if no errors, and a FAIL otherwise.
 *
 * Error Checking:
 *   + UART Rx is successful
 *   + SL032 Rx'd Command is the SL032 Tx'd Command
 *   + SL032 Rx'd Status is "Operation Success"
 *   + Desfire Rx'd Status is "Operation OK"
 *
 * Revision History:
 *   Dec. 30, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareGetBuf(void)
{
  unsigned int rxCount = 0;   /* 0-index'd number of bytes received */
  unsigned char checkSum = 0; /* cummulative checksum */
  
  uartStatus = MF_UARTSTATUS_RXSUCC;  /* assume success for rx status */
  
  /* rxCount is either < 2 and not a problem, or >= 2 and still < the length 
   * received in the second byte (+2 to account for preamble and len bytes). 
   * Note that rxCount is a 0-based index into rxBuf
   */
  while((rxCount < 2) || ((rxCount >= 2) && (rxCount < rxBuf[1]+2))) {
    
    MifareStartTimer(MIFARE_TIMERCOUNT); /*set timer; wait for timeout or byte*/
    while(!SerialInRdy() && !(timerOvertime && timer == 0)); /* if a timeout  */
    if(!SerialInRdy()) uartStatus = MF_UARTSTATUS_RXERR;     /* record error  */
        
    if(uartStatus != MF_UARTSTATUS_RXERR) {     /* if no errors grab bytes */
      rxBuf[rxCount] = SerialGetChar();         /* from serial channel */
      checkSum ^= rxBuf[rxCount];               /* perform cummulative XOR */  
      rxCount++;
    } else {                                    /*if there is an error, break */
      break;
    }
  } 
  
  /* if there is a checkSum error, there is an Rx Error */
  if(checkSum != 0) uartStatus = MF_UARTSTATUS_RXERR;
  
  /* clear the rest of the Rx Buffer */
  while(rxCount < MAX_FRAME_SIZE) {
    rxBuf[rxCount] = 0;                         /* zero out buffer slot and */
    rxCount++;                                  /* move on to the next slot */
  }
}



/*
 * MifareTagInit
 * Description: Initialize a MIFARE DESFire tag
 *
 * Operation:   Reset last picc/pcd errors to a no error state (OPERATION_OK)
 *
 * Arguments:   PICC
 * Return:      None
 *
 * Revision History:
 *   Dec 28, 2012      Nnoduka Eruchalu     Initial Revision
 *   May 04, 2013      Nnoduka Eruchalu     Changed: MifareTagNew->MifareTagInit
 */
void MifareTagInit(mifare_tag *tag)
{
  tag->active = FALSE;
  return;
}

/*
 * MifareDetect
 * Description: Detect a card in the read-range of SL032
 *
 * Arguments:   tag: PICC
 * Return:      SUCCESS - if successfully detected a DESFire card,
 *              FAIL    - couldn't detect to a DESFire card.
 *
 * Operation:   Select a PICC if available. Multiple cards (collision) aren't 
 *              detected as a card tap.
 *
 * Error Checking: When selecting card, perform the following error checks:
 *                 - uart rx is successful
 *                 - rx'd sl032 command is "select card"
 *                 - rx'd sl032 status is "operation success"
 *
 * Revision History:
 *   May  05, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareDetect(mifare_tag *tag)
{
  unsigned char selected = FALSE;          /* start by assuming no selection */
  uint8_t detect = FAIL;                        /* and failed detection  */
  uint8_t i;                                    /* index into uid arrays */
  
  MifarePutBuf(SelectCard, sizeof(SelectCard)); /* send "select card" command */
  MifareGetBuf();                               /* hopefully get feedback */
  
  if((uartStatus == MF_UARTSTATUS_RXSUCC) && (SL032_RXCMD == SL_SELECT_CARD) &&
     (SL032_RXSTA ==SL_OPERATION_SUCC)) {
    selected = TRUE;                           /* select if appropriate */
  }
  
  if(selected) {                               /* connect after selecting */
    tag->type = rxBuf[rxBuf[1]];               /* save card type */
    for(i=0; i<MIFARE_UID_BYTES; i++) {        /* save uid       */
       tag->uid[i] = rxBuf[4+i];
    }
    detect = SUCCESS;                          /* a successful detection */
  }
  
  return detect;                               /* return detection status */
}


/*
 * MifareConnect
 * Description: Establish a connection to the provided PICC, by selecting the 
 *              card and sending a Request for Answer To Select.
 *
 * Argument:    tag: PICC 
 * Return:      SUCCESS: if successfully connected to a DESFire card,
 *              FAIL: couldn't connect to a DESFire card.
 * 
 * Operation:   Check the tag is inactive; Can't connect if already active.
 *              Select a DESFire PICC if available, and connect to it if it 
 *                possible.
 *              If no connection is made, then return FAIL
 *              If however a connection is made setup tag data structure and 
 *                return SUCCESS.
 *              Tag setup is done by activating tag, freeing and clearing the
 *                session key, clearing all errors, authenticated key no, and
 *                selected App ID.
 *
 * Error Check: When selecting card, perform the following error checks:
 *              - uart rx is successful
 *              - rx'd sl032 command is "select card"
 *              - rx'd sl032 status is "operation success"
 *              - card type is a DESFire card
 *              When connecting to a card, perform the following error checks
 *              - uart rx is successful
 *              - rx'd sl032 command is "Request for Answer To Select"
 *              - rx'd sl032 status is "operation success"
 *
 * Revision History:
 *   Dec. 31, 2012      Nnoduka Eruchalu     Initial Revision
 */
int MifareConnect(mifare_tag *tag)
{
  unsigned char selected = FALSE;          /* start by assuming no selection */
  unsigned char connected = FALSE;         /* start by assuming no connection */
  uint8_t uid[MIFARE_UID_BYTES];           /* temp uid */
  uint8_t i;                               /* index into uid arrays */
  
    
  MifarePutBuf(SelectCard, sizeof(SelectCard)); /* send "select card" command */
  MifareGetBuf();                               /* hopefully get feedback */
  if((uartStatus == MF_UARTSTATUS_RXSUCC) && (SL032_RXCMD == SL_SELECT_CARD) &&
     (SL032_RXSTA ==SL_OPERATION_SUCC) && (rxBuf[rxBuf[1]] ==MIFARE_CARD_DES)) {
    selected = TRUE;                           /* select if appropriate */
  }
  
  if(selected) {                                   /* connect after selecting */
    for(i=0; i<MIFARE_UID_BYTES; i++) {            /* save uid */
       uid[i] = rxBuf[4+i];
    }
    MifarePutBuf(RATSDesfire, sizeof(RATSDesfire));/* send "RATS" command */
    MifareGetBuf();                                /* hopefully get feedback */
    if((uartStatus == MF_UARTSTATUS_RXSUCC) && (SL032_RXCMD == SL_RATS) &&
       (SL032_RXSTA == SL_OPERATION_SUCC)) {
      connected = TRUE;                            /* connect if appropriate */
    }
  }
  
  if(connected) {            /* only setup tag if successfully connected */
    MifareTagInit(tag);
    tag->active = TRUE;
    for(i=0; i<MIFARE_UID_BYTES; i++) {  /* save uid */
      tag->uid[i] = uid[i];
    }
  } else {
    return FAIL;
  }
  return SUCCESS; /* if you made it this far, it is all good */
}


/*
 * MifareDisconnect
 * Description: Terminate connection with the provided tag
 *
 * Arguments:    tag: PICC
 * Return:       SUCCESS: if successfully terminated connection
 *               FAIL:    couldn't terminate connection from the tag
 *
 * Operation:   Check the tag is active; Can't disconnect if inactive.
 *              Free and clear session key pointer
 *              Deactivate tag.
 *
 * Revision History:
 *   Dec. 31, 2012      Nnoduka Eruchalu     Initial Revision
 */
int MifareDisconnect(mifare_tag *tag) {
    
  tag->active = FALSE;         /* deactivate tag */
  
  return SUCCESS; /* if you made it this far, it is all good */
}
