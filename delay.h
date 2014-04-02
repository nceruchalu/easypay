/*
 * -----------------------------------------------------------------------------
 * -----                             DELAY.H                               -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the header file of time delay routines for EasyPay. 
 *   Purposely named delay.h so as to not conflict with system library, delays.h
 * 
 * Assumptions:
 *   None
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Apr. 25, 2012      Nnoduka Eruchalu     Initial Revision
 */

#ifndef DELAY_H
#define DELAY_H
/* library include files */
#include <htc.h>   /* for __delay_ms */


/*
 * __delay_s(x)
 * max number of __delay_ms is 40ms
 * so each second requires 25 loops of __delay_ms(40)
 */
#define __delay_s(x)                             \
  do {                                           \
    unsigned long n;                             \
    for(n=0; n < (unsigned long) 25*x; n++) {__delay_ms(40);}    \
  } while(0)


#endif                                                             /* DELAY_H */


