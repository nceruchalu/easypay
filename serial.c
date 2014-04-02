/*
 * -----------------------------------------------------------------------------
 * -----                             SERIAL.C                              -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is a library of functions for using the PIC18F67K22 EUSART module.
 *   Each function is duplicated to account for both serial channels 1 and 2.
 *
 * Table of Contents:
 *   SerialInit    - initialize the EUSART module
 *   SerialInit2
 *   SerialInRdy   - check if there is a serial channel byte available
 *   SerialInRdy2
 *   SerialGetChar - get the next byte from the serial channel.
 *   SerialGetChar2
 *   SerialOutRdy  - check if serial channel is ready to transmit a byte
 *   SerialOutRdy2
 *   SerialPutChar - output a byte to the serial channel
 *   SerialPutChar2
 *   SerialStatus  - return the serial channel error status
 *   SerialStatus2
 *   SerialRxISR   - handles the serial channel RX interrupts
 *   SerialRxISR2
 *   SerialTxISR   - handles the serial channel TX interrupts
 *   SerialTxISR2
 *
 * Limitations:
 *   - sizes of the serialTxQ and serialRxQ are fixed
 *   - when error status is reported by SerialStatus, we can only tell an error
 *     occurred, but cannot tell the occurrence count.
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Todo:
 *   Come up with a cleaner way of accounting for both serial channels 1 and 2.
 *   Copy-pasting code and changing 1s to 2s is pretty lame.
 *
 * Revision History:
 *   Dec. 19, 2012      Nnoduka Eruchalu     Initial Revision
 *   Dec. 21, 2012      Nnoduka Eruchalu     Start Implementation
 *   May. 01, 2013      Nnoduka Eruchalu     Added routines for serial channel 2
 */

#include <htc.h>
#include "general.h"
#include "queue.h"
#include "serial.h"

/* shared variables have to be local to this file */
static queue serialRxQueue; /* queue holding serially RX'd data */
static queue serialTxQueue; /* queue holding data to be serially TX'd*/
static unsigned char serialErrors;   /* byte holding serial channel errors */

static queue serialRxQueue2; /* queue holding serially RX'd data */
static queue serialTxQueue2; /* queue holding data to be serially TX'd*/
static unsigned char serialErrors2;   /* byte holding serial channel errors */

/*
 * SerialInit
 * Description: Initializes serial port, serial routine variables, data 
 *              structures.
 *              Serial interrupts are enabled.
 *
 * Arguments:   None
 * Return:      None
 *
 * Operation:   Reset error status
 *              Initialize Rx and Tx Queues.
 *              Configure I/O pins, TXSTA1, RCSTA1, BAUDCON1, Baudrate Generator
 *              Enable Interrupts
 *
 * Revision History:
 *   Dec. 21, 2012      Nnoduka Eruchalu     Initial Revision
 */
void SerialInit(void)
{
  serialErrors = SERIAL_NO_ERROR; /* reset the error status */
  QueueInit(&serialTxQueue); /* initialize the Tx Queue */
  QueueInit(&serialRxQueue); /* initialize the Rx Queue */

  
  /* configure I/O pins as follows: */
  TRISC7 = 1; /* RX pin is an input */
  TRISC6 = 0; /* TX pin is an output */
  
  /* configure TXSTA1 as follows: */
  BRGH1 = 1;  /* high speed baud rate */
  SYNC1 = 0;  /* Asynchronous mode */
  TX91 = 0;   /* Select 8-bit transmission */
  
  /* configure RCSTA1 as follows: */
  CREN1 = 1;  /* Enable receiver */
  RX91 = 0;   /* select 8-bit reception */
  SPEN1 = 1;  /* serial port is enabled */
  
  /* configure BAUDCON1 as follows: */
  ABDEN1 = 0; /* Disable Baud Rate Measurement */
  WUE1   = 0; /* RX1 pin not monitored for Auto-Wake-Up */
  BRG161 = 0; /* 8-bit Baud Rate Generator */
  
  /* Calculating baud rate:
     Desired Baud Rate = Fosc/[16*(N+1)]
     N = Fosc/[16*{Desired Baud Rate}] - 1
     N = 18.432M/[16*115.2K] - 1
     N = 9
     N = 0x09
     
     Error = 0%
   */
  SPBRGH1 = 0; /* high byte of N */
  SPBRG1 = 9;  /* low byte of N */
  
  /* configure interrupts */
  TX1IE = 0;  /* interrupt driven transmitter with kickstarting */  
  TXEN1 = 1;  /* Transmit is enabled; this sets TX1IF */
  RC1IF = 0;
  RC1IE = 1;  /* interrupt driven receiver */
}
void SerialInit2(void)
{
  serialErrors2 = SERIAL_NO_ERROR; /* reset the error status */
  QueueInit(&serialTxQueue2); /* initialize the Tx Queue */
  QueueInit(&serialRxQueue2); /* initialize the Rx Queue */

  
  /* configure I/O pins as follows: */
  ANSEL18 = 0;/* RG2 is digital     */
  TRISG2 = 1; /* RX pin is an input */
  TRISG1 = 0; /* TX pin is an output */
  
  /* configure TXSTA2 as follows: */
  BRGH2 = 1;  /* high speed baud rate */
  SYNC2 = 0;  /* Asynchronous mode */
  TX92 = 0;   /* Select 8-bit transmission */
  
  /* configure RCSTA2 as follows: */
  CREN2 = 1;  /* Enable receiver */
  RX92 = 0;   /* select 8-bit reception */
  SPEN2 = 1;  /* serial port is enabled */
  
  /* configure BAUDCON1 as follows: */
  ABDEN2 = 0; /* Disable Baud Rate Measurement */
  WUE2   = 0; /* RX2 pin not monitored for Auto-Wake-Up */
  BRG162 = 0; /* 8-bit Baud Rate Generator */
  
  /* Calculating baud rate:
     Desired Baud Rate = Fosc/[16*(N+1)]
     N = Fosc/[16*{Desired Baud Rate}] - 1
     N = 18.432M/[16*115.2K] - 1
     N = 9
     N = 0x09
     
     Error = 0%
   */
  SPBRGH2 = 0; /* high byte of N */
  SPBRG2 = 9;  /* low byte of N */
  
  /* configure interrupts */
  TX2IE = 0;  /* interrupt driven transmitter with kickstarting */  
  TXEN2 = 1;  /* Transmit is enabled; this sets TX2IF */
  RC2IF = 0;
  RC2IE = 1;  /* interrupt driven receiver */
}


/*
 * SerialInRdy
 * Description: This function returns with TRUE if there is a byte available 
 *              from the serial channel; otherwise it returns FALSE.
 *              In other words, if SerialInRdy returns with TRUE, that means 
 *              SerialGetChar would return immediately if called because there
 *              is a byte available.
 *
 * Argumets:    None
 * Return:      boolean (TRUE/FALSE)
 *
 * Operation: Checks if there is a byte available from the serial channel by 
 *            looking at where Rx'd bytes are stored, serialRxQueue.
 *            Procedure checks if serialRxQueue is not empty.
 *
 * Revision History:
 *   Dec. 21, 2012      Nnoduka Eruchalu     Initial Revision
 */
unsigned char SerialInRdy(void)
{
  return (!QueueEmpty(&serialRxQueue)); /* there's a byte in non-empty queue */
}
unsigned char SerialInRdy2(void)
{
  return (!QueueEmpty(&serialRxQueue2)); /* there's a byte in non-empty queue */
}


/*
 * SerialGetChar
 * Description: This function returns with the next byte from the serial 
 *              channel. The routine does not return until it has a received 
 *              byte. It is a blocking function.
 *
 * Argumets:    None
 * Return:      data byte from serial channel
 *
 * Operation:   This is a blocking function. However instead of waiting on 
 *              SerialInRdy() to know if there is an available byte, it simply 
 *              calls the queue routine Dequeue on serialRxQueue.
 *              This works because Dequeue blocks until serialRxQueue is not 
 *              empty.
 *
 * Revision History:
 *   Dec. 21, 2012      Nnoduka Eruchalu     Initial Revision
 */
unsigned char SerialGetChar(void)
{
  return Dequeue(&serialRxQueue); /* get a byte by dequeuing the Rx Queue */
}
unsigned char SerialGetChar2(void)
{
  return Dequeue(&serialRxQueue2); /* get a byte by dequeuing the Rx Queue */
}


/*
 * SerialOutRdy
 * Description: This function returns with TRUE if the serial channel is ready 
 *              for another byte to transmit; otherwise it returns FALSE.
 *              In other words, if SerialOutRdy returns with TRUE, that means 
 *              SerialPutChar would return immediately if called because there 
 *              is the channel is ready to transmit another byte.
 *
 * Argumets:    None
 * Return:      None
 *
 * Operation:   Checks if the channel is ready to accept another byte (for 
 *              transmission) by looking at where the bytes to be transmitted 
 *              are stored, serialTxQueue.
 *              Procedure checks if serialTxQueue is not full.
 *
 * Revision History:
 *   Dec. 21, 2012      Nnoduka Eruchalu     Initial Revision
 */
unsigned char SerialOutRdy(void)
{
  return (!QueueFull(&serialTxQueue)); /* there's a byte in non-empty queue */
}
unsigned char SerialOutRdy2(void)
{
  return (!QueueFull(&serialTxQueue2)); /* there's a byte in non-empty queue */
}


/*
 * SerialPutChar
 * Description: This function outputs the passed byte to the serial channel. 
 *              The routine does not return until the byte has been "output" to 
 *              the channel; "output" means placing the byte in the channel's Tx
 *              queue.
 *              The function also enables the TX Interrupt. This is so as to 
 *              (possibly) end the kickstartng process of disabling the 
 *              TX Interrupt in the ISR if the TX Int occured and the 
 *              serialTxQueue was empty.
 *
 * Arguments:   Data byte to "output"
 * Return:      None    
 *
 * Operation:   This is a blocking function. However instead of waiting on 
 *              SerialOutRdy() to know if there is room on the output queue, it 
 *              simply calls the queue routine Enqueue on serialTxQueue.
 *              This works because Enqueue blocks until serialTxQueue is not 
 *              full.
 *              To enable the TX interrupt, the function enables TXIE.
 *
 * Revision History:
 *   Dec. 21, 2012      Nnoduka Eruchalu     Initial Revision
 */
void SerialPutChar(unsigned char b)
{
  Enqueue(&serialTxQueue, b); /* "output" a byte by enqueuing the Tx Queue */
  TX1IE = 1;                  /* end kickstarting process by enabling TX Ints */
}
void SerialPutChar2(unsigned char b)
{
  Enqueue(&serialTxQueue2, b); /* "output" a byte by enqueuing the Tx Queue */
  TX2IE = 1;                  /* end kickstarting process by enabling TX Ints */
}


/*
 * SerialStatus
 * Description: The function returns the error status for the serial channel. 
 *              This error status is stored in the serialErrors variable.
 *              This function resets the serial error status, after reading it.
 *
 * Arguments:   None
 * Return:      Serial Error status.
 *              The definition of the error information in this status byte is:
 *                Bit 0 - serialRxQueue Buffer Overflow Error
 *                Bit 1 - Overrun Error (cleared by clearing bit, CREN)
 *                Bit 2 - Framing Error (cleared by reading RCREGx and Rx'ing 
 *                                       next valid byte)
 *                Bit 3 - ignore
 *                Bit 4 - ignore
 *                Bit 5 - ignore
 *                Bit 6 - ignore
 *                Bit 7 - ignore
 *              --> The above error bits are set if the associated errors occur.
 *
 * Operation:   Save a temporary copy of serialErrors
 *              Reset serialErrors to NO_ERROR
 *              Return saved temp. copy.
 *
 * Limitations: - Unfortunately this is critical code. Between the saving of the
 *                temp copy, and resetting of serialErrors, an error could occur
 *                and will be cleared.
 *              - When the error status is reported, we can only tell a certain 
 *                error occurred, and cannot tell the number of times it 
 *                occurred.
 *
 * Revision History:
 *   Dec. 21, 2012      Nnoduka Eruchalu     Initial Revision
 */
unsigned char SerialStatus(void)
{
  unsigned char temp = serialErrors; /* save a temp copy of the serial errors */
  serialErrors = SERIAL_NO_ERROR;    /* reset channel to a state of no error */
  temp &= SERIAL_ERR_MASK;          /* finally return serial error status */
  return temp;
}
unsigned char SerialStatus2(void)
{
  unsigned char temp = serialErrors2;/* save a temp copy of the serial errors */
  serialErrors2 = SERIAL_NO_ERROR;   /* reset channel to a state of no error */
  temp &= SERIAL_ERR_MASK;          /* finally return serial error status */
  return temp;
}


/*
 * SerialRxISR
 * Description: This is the Interrupt Service Routine that handles received 
 *              data. It also updates serial errors byte.
 *
 * Arguments:   None
 * Return:      None
 *
 * Operation:   On a Receive Interrupt the data is read in from the Receive 
 *              Buffer Register and only enqueued in the serialRxQueue if the
 *              queue is not full. If however it is full the procedure logs an 
 *              Rx Buffer Overflow Error. It also reads in serial errors.
 *
 * Revision History:
 *   Dec. 21, 2012      Nnoduka Eruchalu     Initial Revision
 */
void SerialRxISR(void)
{
  unsigned char rxval;               /* value to receive */
    
  serialErrors = RCSTA1;             /* read in and save error bits only */
    
  rxval = RCREG1;                    /* read in the Receive Buffer Register */
  
  if (OERR1) {                       /* if an overrun error occurred, reset */
    CREN1 = 0; CREN1 = 1;            /* receiver; framing error cleared by */
  }                                  /* reading RCREGx */
  
  if (QueueFull(&serialRxQueue)) {   /* if RX Queue is full, */
    serialErrors |= SERIAL_RX_BUF_OE;/* log an Rx Buffer Overflow error */
  
  } else {                           /* if RX Queue isn't full, it is */
    Enqueue(&serialRxQueue, rxval);  /* enqueued with the received byte */
  }
}
void SerialRxISR2(void)
{
  unsigned char rxval;               /* value to receive */
    
  serialErrors2 = RCSTA2;            /* read in and save error bits only */
    
  rxval = RCREG2;                    /* read in the Receive Buffer Register */
  
  if (OERR2) {                       /* if an overrun error occurred, reset */
    CREN2 = 0; CREN2 = 1;            /* receiver; framing error cleared by */
  }                                  /* reading RCREGx */
  
  if (QueueFull(&serialRxQueue2)) {   /* if RX Queue is full, */
    serialErrors2 |= SERIAL_RX_BUF_OE;/* log an Rx Buffer Overflow error */
  
  } else {                           /* if RX Queue isn't full, it is */
    Enqueue(&serialRxQueue2, rxval); /* enqueued with the received byte */
  }
}


/*
 * SerialTxISR
 * Description: This is the Interrupt Service Routine that handles serial 
 *              transmitter empty.
 *              It also makes sure to disable the TX Interrupt, thus beginning 
 *              the kickstarting process.
 *
 * Arguments:   None
 * Return:      None
 *
 * Operation:   On a Transmit interrupt, the procedure only dequeues the 
 *              serialTxQueue if it is not empty. If however serialTxQueue is 
 *              empty the procedure begins the kickstarting process. This means 
 *              the procedure will disable the TX Int.
 *              The TX Int will be re-enabled in SerialPutChar, thus ending the 
 *              kickstart process started here. This way multiple and 
 *              unnecessary calls to this ISR are avoided.
 *
 *              Note that TXIF is not cleared immediately upon loading TXREG, 
 *              but becomes valid in the second instruction cycle following a 
 *              load of TXREG. Polling TXIF immediately following a load of 
 *              TXREG will return invalid results. So two NOPs are inserted to 
 *              give TXIF time to become valid.
 *
 * Revision History:
 *   Dec. 21, 2012      Nnoduka Eruchalu     Initial Revision
 */
void SerialTxISR(void)
{
  unsigned char txval;              /* value to transmit */
  
  if(QueueEmpty(&serialTxQueue)) {  /* if TX Queue is empty, start kickstart */
    TX1IE = 0;                      /* process by disabling TX Ints */
  
  } else {                          /* if TX Queue isn't empty, it can be */
    txval = Dequeue(&serialTxQueue);/* dequeued, and the obtained byte should */
    TXREG1 = txval;                 /* be written to the serial chip's */
                                    /* Transmit Buffer register */
    NOP(); NOP();                   /* allow TXIF time to become valid */    
  }
}
void SerialTxISR2(void)
{
  unsigned char txval;              /* value to transmit */
  
  if(QueueEmpty(&serialTxQueue2)) { /* if TX Queue is empty, start kickstart */
    TX2IE = 0;                      /* process by disabling TX Ints */
  
  } else {                          /* if TX Queue isn't empty, it can be */
    txval = Dequeue(&serialTxQueue2);/* dequeued, and the obtained byte should*/
    TXREG2 = txval;                 /* be written to the serial chip's */
                                    /* Transmit Buffer register */
    NOP(); NOP();                   /* allow TXIF time to become valid */    
  }
}
