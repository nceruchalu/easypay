/*
 * -----------------------------------------------------------------------------
 * -----                           INTERRUPTS.H                            -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the header file for interrupts.c, the setup file for interrupts.
 *
 * Assumptions:
 *   None
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Dec. 19, 2012      Nnoduka Eruchalu     Initial Revision
 *   Apr. 26, 2013      Nnoduka Eruchalu     Added Tmr2Init
 */

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

/* TIMING CONSTANTS */
#define TMR0_FREQ  1125    /* 0.8889 ms period == 1125Hz */
#define TMR2_FREQ  2000    /* 0.5 ms period    == 2000Hz */

/* FUNCTION PROTOTYPES */
/* initialize Timer 0 */
extern void Tmr0Init(void);

/* initialize Timer 2 */
extern void Tmr2Init(void);


#endif                                                        /* INTERRUPTS_H */
