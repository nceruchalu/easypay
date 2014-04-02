/*
 * -----------------------------------------------------------------------------
 * -----                           INTERRUPTS.C                            -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This file contains setup for interrupts
 *
 * Table of Contents:
 *   Tmr0Init       - Initialize Timer0
 *   Tmr2Init       - Initialize Timer2
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

#include <htc.h>
#include "interrupts.h"


/*
 * Tmr0Init
 * Description: Initialize Tmr0 to generate 0.889ms periods
 *
 * Arguments:   None
 * Return:      None
 *
 * Operation:   Setup T0CON.
 *              Setup INTCON: T0IE, T0IF
 *
 * Revision History:
 *   Dec. 19, 2012      Nnoduka Eruchalu     Initial Revision
 */
void Tmr0Init(void)
{ 
  /* configure Timer0 as follows:
     
     Period = 1 / [(Processor Frequency)/4] = 1 / (18.432MHz/4) = 217nS
     PreScalar = 16
     n = 8 bit timer
     Time between Interrupts  = Period * PreScaler * 2^n = 0.889 ms
   */
  T0PS0  = 1;            /* prescale is divide by 16 */
  T0PS1  = 1;
  T0PS2  = 0;            
    
  PSA    = 0;            /* Timer0 clock source is from prescaler */
  T0CS   = 0;            /* use internal clock (Fosc/4) */
  T08BIT = 1;            /* 8 bit timer/counter */
  
  
  TMR0 = 0;               /* Clear the TMR0 register */
  
  TMR0IF   = 0; 
  TMR0IE   = 1;          /* enable TIMER0 overflow interrupts */
  
  TMR0ON = 1;            /* now start the timer */
}



/*
 * Tmr2Init
 * Description: Initialize Tmr2 to generate 0.5ms periods
 *
 * Arguments:   None
 * Return:      None
 *
 * Operation:   Setup T2CON.
 *              Setup INTCON: T2IE, T2IF
 *
 * Revision History:
 *   Apr. 26, 2013      Nnoduka Eruchalu     Initial Revision
 */
void Tmr2Init(void)
{
  /* configure Timer2 as follows:
     
     Period = 1 / [(Processor Frequency)/4] = 1 / (18.432MHz/4) = 217nS
     PreScalar = 16
     PR2 = 144
     Time between Interrupts  = Period * PreScaler * PR2 = 0.5ms
   */
    
  T2CKPS0 = 1;          /* prescale is divide by 16 */
  T2CKPS1 = 1;
  
  T2OUTPS0 = 0;         /* postscale is 1:1 */
  T2OUTPS1 = 0;
  T2OUTPS2 = 0;
  T2OUTPS3 = 0;
  
  PR2 = 144;
  
  TMR2 = 0;              /* Clear the TMR2 register */
  
  TMR2IF   = 0; 
  TMR2IE   = 1;          /* enable TIMER2 overflow interrupts */
  
  TMR2ON = 1;            /* now start the timer */
}


