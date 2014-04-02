/*
 * -----------------------------------------------------------------------------
 * -----                             MAIN.C                                -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the main file for EasyPay.
 * 
 * Assumptions:
 *   None
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Dec. 16, 2012      Nnoduka Eruchalu     Initial Revision
 *   May  14, 2013      Nnoduka Eruchalu     Updated for demo
 */

#include "general.h"
#include <htc.h>
#include "configs.h"
#include "interrupts.h"
#include "serial.h"
#include "keypad.h"
#include "smartcard.h"
#include "lcd.h"
#include "data.h"
#include "delay.h"
#include "interface.h"
#include "sim5218.h"
#include "eventproc.h"


/* POWER PIN DEFINITIONS */
#define DATAPOWER_MCU_TRIS TRISG0
#define DATAPOWER_MCU      LATG0

#define LCDPOWER_MCU_TRIS TRISB4
#define LCDPOWER_MCU      LATB4

/* Microcontroller Configurations */
#pragma config CONFIG1L = EASYPAY_CONFIG1L;
#pragma config CONFIG1H = EASYPAY_CONFIG1H;

#pragma config CONFIG2L = EASYPAY_CONFIG2L;
#pragma config CONFIG2H = EASYPAY_CONFIG2H;

#pragma config CONFIG3L = EASYPAY_CONFIG3L;
#pragma config CONFIG3H = EASYPAY_CONFIG3H;

#pragma config CONFIG4L = EASYPAY_CONFIG4L;

#pragma config CONFIG5L = EASYPAY_CONFIG5L;
#pragma config CONFIG5H = EASYPAY_CONFIG5H;

#pragma config CONFIG6L = EASYPAY_CONFIG6L;
#pragma config CONFIG6H = EASYPAY_CONFIG6H;

#pragma config CONFIG7L = EASYPAY_CONFIG7L;
#pragma config CONFIG7H = EASYPAY_CONFIG7H;



/*
 * main
 * Description:      This procedure is the main program loop for the EasyPay
 *                   project.
 *
 * Arguments:        None.
 * Return:           Never returns.
 *
 * Input:            Keypad
 *                   SmartCard
 * Output:           LCD
 *                   Data module
 *                   Power Switches (LCD and Data module)
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
void main(void)
{
  /* POWER ON THE MODULES 
   * ----------------------------
   */
  /* Lcd Module power on circuitry is:
   * power line goes straight from MCU to VCC of LCD
   */
  LCDPOWER_MCU_TRIS = 0;  /* power line is an output */
  LCDPOWER_MCU = 1;
  
  /* 3G module (SIM5218A) power on sequence is:
   * - DATAPOWER high
   * - DATAPOWER low for at least 64ms
   * - DATAPOWER high
   * but that comes out of an inverting switch so DATAPOWER_MCU (from MCU)
   * does exact opposite
   */
  DATAPOWER_MCU_TRIS = 0; /* power control is an output */
  DATAPOWER_MCU = 0;
  DATAPOWER_MCU = 1; __delay_s(2);
  DATAPOWER_MCU = 0;
  __delay_s(SIM_STARTUP_TIME);    /* wait for SIM5218A to be ready for input */
  
    
  /* INITIALIZE THE MODULES 
   * ---------------------------
   */
  /* timer modules */
  Tmr0Init();              /* Setup Timer Event for ScanAndDebounce */
  Tmr2Init();              /* Setup Timer Event for EventProcessors */
  
  /* communication modules */
  SerialInit();            /* setup serial channel 1 */
  SerialInit2();           /* setup serial channel 2 */
  
  /* I/O modules */
  KeypadInit();            /* setup Keypad and its interrupts */
  CardInit();              /* setup smartcard */
  LcdInit();               /* setup lcd */
  
  /* interrupts */
  GIE = 1;    /* Enable Global and Peripheral interrupts.*/
  PEIE = 1;
  
  /* initialization routines that need interrupts */
  DataInit();  /* must be called after SerialInit2() and enabling interrupts */
  
  /* FSM loop */
  StateDriver();   /* this should never return */
    
  
  /* Loop Forever: System shouldn't get here */
  while(TRUE) continue;
}


void interrupt isr(void)
{ 
  if(RC1IE && RC1IF) {   /* EUSART1 RX interrupt has occured */
    SerialRxISR();       /* Call its event handler */
    RC1IF = 0;           /* clear the flag so next overflow can be detected */
  }
  
  if(TX1IE && TX1IF) {   /* EUSART1 TX interrupt has occured */
    SerialTxISR();       /* Call its event handler */
                         /* TX1IF cannot be cleared in software */
  }
  
  if(RC2IE && RC2IF) {   /* EUSART2 RX interrupt has occured */
    SerialRxISR2();       /* Call its event handler */
    RC2IF = 0;           /* clear the flag so next overflow can be detected */
  }

  if(TX2IE && TX2IF) {   /* EUSART2 TX interrupt has occured */
    SerialTxISR2();       /* Call its event handler */
                         /* TX1IF cannot be cleared in software */
  }
  
  if(TMR0IE && TMR0IF) { /* interrupt from Timer0 has occured */
    ScanAndDebounce();   /* Call keypad event handler */
    MifareTimerISR();    /* Call Mifare time based event handler */
    SimTimerISR();       /* Call Sim5218's time based event handler */
    TMR0IF = 0;          /* clear the flag so next overflow can be detected */
  }

  /* make this light weight (i.e. only call 1 func) because it's a clock timer*/
  if(TMR2IE && TMR2IF) { /* interrupt from Timer2 has occured */
    EventTimer();        /* call timer for interface events */
    TMR2IF = 0;          /* clear the flag so next overflow can be detected */
  }
}
