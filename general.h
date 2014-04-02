/*
 * -----------------------------------------------------------------------------
 * -----                            GENERAL.H                              -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the main header file for EasyPay. Contains system information
 * 
 * Assumptions:
 *   None
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Dec. 16, 2012      Nnoduka Eruchalu     Initial Revision
 */

#ifndef GENERAL_H
#define GENERAL_H

/* library include files */
#include <string.h>             /* so ssize_t is defined during testing */
#include <stdio.h>              /* so off_t is defined during testing */


/* --------------------------------------
 * Constants
 * --------------------------------------
 */
#define _XTAL_FREQ 18432000UL   /* the PIC clock frequency is 18.432MHz */
                                /* important for delay macros */

#define FALSE      0            /* because C doesn't have bool */
#define TRUE       1            /* any non-zero value works here */

#define FAIL      -1            
#define SUCCESS    0            


/* --------------------------------------
 * Type defines not defined in include files
 * --------------------------------------
 */
#ifdef __PICC18__               /* only define for HI-TECH PICC-18 compiler */
typedef int        ssize_t;
typedef long       off_t;
#endif


/* --------------------------------------
 * Convenience Macros
 * --------------------------------------
 */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#endif                                                           /* GENERAL_H */


