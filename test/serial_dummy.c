/*
 * -----------------------------------------------------------------------------
 * -----                          SERIAL_DUMMY.C                           -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *  A version of serial.c that doesn't depend on hardware. Use this for unix
 *  based tests
 *
 * Revision History:
 *  Jan. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */

#include "../general.h"
#include "../queue.h"
#include "../serial.h"

/* shared variables have to be local to this file */
static queue serialRxQueue; /* queue holding serially RX'd data */
static queue serialTxQueue; /* queue holding data to be serially TX'd*/
static unsigned char serialErrors;   /* byte holding serial channel errors */


void SerialInit(void)
{
  serialErrors = SERIAL_NO_ERROR; /* reset the error status */
  QueueInit(&serialTxQueue); /* initialize the Tx Queue */
  QueueInit(&serialRxQueue); /* initialize the Rx Queue */
}


unsigned char SerialInRdy(void)
{
  return (!QueueEmpty(&serialRxQueue)); /* there's a byte in non-empty queue */
}


unsigned char SerialGetChar(void)
{
  return Dequeue(&serialRxQueue); /* get a byte by dequeuing the Rx Queue */
}


unsigned char SerialOutRdy(void)
{
  return (!QueueFull(&serialTxQueue)); /* there's a byte in non-empty queue */
}


void SerialPutChar(unsigned char b)
{
  Enqueue(&serialTxQueue, b); /* "output" a byte by enqueuing the Tx Queue */
}


/*
 * SerialStatus
 * Description:
 *  The function returns the error status for the serial channel. This error
 *  status is stored in the serialErrors variable.
 *  This function resets the serial error status, after reading it.
 *  The exact definition of the error information in the serialErrors byte is:
 *  Bit 0 - serialRxQueue Buffer Overflow Error
 *  Bit 1 - Overrun Error (cleared by clearing bit, CREN)
 *  Bit 2 - Framing Error (cleared by reading RCREGx and Rx'ing next valid byte)
 *  Bit 3 - ignore
 *  Bit 4 - ignore
 *  Bit 5 - ignore
 *  Bit 6 - ignore
 *  Bit 7 - ignore
 *  --> The above error bits are set if the associated errors occur.
 *
 * Operation:
 *  save a temporary copy of serialErrors
 *  reset serialErrors to NO_ERROR
 *  return saved temp. copy.
 *
 * Limitations
 *  - Unfortunately this is critical code. Between the saving of the temp copy,
 *    and resetting of serialErrors, an error could occur and will be cleared.
 *  - When the error status is reported, we can only tell a certain error
 *    occurred, and cannot tell the number of times it occurred.
 *
 * Revision History:
 *  Dec. 21, 2012      Nnoduka Eruchalu     Initial Revision
 */
unsigned char SerialStatus(void)
{
  unsigned char temp = serialErrors; /* save a temp copy of the serial errors */
  serialErrors = SERIAL_NO_ERROR;    /* reset channel to a state of no error */
  temp &= SERIAL_ERR_MASK;
  return temp;
  
}


/* how to make it receive values ?? */
void SerialRxISR(void)
{
  unsigned char rxval = 0;           /* value to receive */
    
  serialErrors = 0;
  
  if (QueueFull(&serialRxQueue)) {   /* if RX Queue is full, */
    serialErrors |= SERIAL_RX_BUF_OE;/* log an Rx Buffer Overflow error */
  
  } else {                           /* if RX Queue isn't full, it is */
    Enqueue(&serialRxQueue, rxval);  /* enqueued with the received byte */
  }
}

/* how to make it transmit values?? */
void SerialTxISR(void)
{
  unsigned char txval;              /* value to transmit */
  
  if(QueueEmpty(&serialTxQueue)) {  /* if TX Queue is empty, start kickstart */
      
  } else {                          /* if TX Queue isn't empty, it can be */
    txval = Dequeue(&serialTxQueue);/* dequeued, and the obtained byte should */
  }
}
