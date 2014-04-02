/*
 * -----------------------------------------------------------------------------
 * -----                             SERIAL.H                              -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the header file for serial.c, the library of functions for using
 *   the PIC18F67K22 EUSART module.
 *
 * Assumptions:
 *   None.
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Dec. 19, 2012      Nnoduka Eruchalu     Initial Revision
 *   May. 01, 2013      Nnoduka Eruchalu     Added routines for serial channel 2
 */

#ifndef SERIAL_H
#define SERIAL_H

/* GENERAL CONSTANTS */
#define SERIAL_ERRORS_MASK  0xFF    /* will be used to detect set error bits */
#define SERIAL_NO_ERROR     0       /* no errors in serial channel */
#define SERIAL_RX_BUF_OE    0x40    /* mask for Rx buffer overflow error bit: */
                                    /* bit must be a default of 0 in RCSTAx */
#define SERIAL_ERR_MASK     0x06 + SERIAL_RX_BUF_OE    /* serialErrors bits */
                                    


/* FUNCTION PROTOTYPES */
/* initialize the EUSART module */
extern void SerialInit(void);
extern void SerialInit2(void);

/* check if there is a serial channel byte available */
extern unsigned char SerialInRdy(void);
extern unsigned char SerialInRdy2(void);

/* get the next byte from the serial channel. */
extern unsigned char SerialGetChar(void);
extern unsigned char SerialGetChar2(void);

/* check if serial channel is ready to transmit a byte */
extern unsigned char SerialOutRdy(void);
extern unsigned char SerialOutRdy2(void);

/* output a byte to the serial channel */
extern void SerialPutChar(unsigned char b);
extern void SerialPutChar2(unsigned char b);

/* return the serial channel error status */
extern unsigned char SerialStatus(void);
extern unsigned char SerialStatus2(void);

/* handles the serial channel RX interrupts */
extern void SerialRxISR(void);
extern void SerialRxISR2(void);

/* handles the serial channel TX interrupts */
extern void SerialTxISR(void);
extern void SerialTxISR2(void);


#endif                                                            /* SERIAL_H */
