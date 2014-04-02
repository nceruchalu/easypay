/*
 * -----------------------------------------------------------------------------
 * -----                             KEYPAD.H                              -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *  This is the header file for keypad.c, the library of functions for
 *  interfacing the PIC18F67K22 with a 4x4 matrix keypad.
 *
 * Assumptions:
 *  Keypad Columns are active low inputs into the MCU.
 *    Since they are active low they have to be pulled up by default (to an
 *    inactive state)
 *  Keypad Row values are active low outputs from the MCU. So they are inputs to
 *    the keypad.
 *
 * Compiler:
 *  HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *  Dec. 17, 2012      Nnoduka Eruchalu     Initial Revision
 */

#ifndef KEYPAD_H
#define KEYPAD_H

/* library include files */
#include <htc.h>

/* local include files*/
#include "interrupts.h"

/* HARDWARE CONNECTIONS */
#define KEY_ROW0       LATE0   /* use LAT for writing to I/O pins */
#define KEY_ROW1       LATE1
#define KEY_ROW2       LATE2
#define KEY_ROW3       LATE3
#define KEY_COL0       RE4     /* USE PORT for reading from I/O pins */
#define KEY_COL1       RE5
#define KEY_COL2       RE6
#define KEY_COL3       RE7

#define KEY_PORT       PORTE
#define KEY_ROWS       LATE
#define KEY_TRIS       TRISE

#define KEY_ROWS_OFFSET    0    /* offset of row bits on PORT E:   0  or    4 */
#define KEY_COLS_MASK      0xF0 /* mask for extracting Col bits: 0xF0 or 0x0F */
#define KEY_NUM_ROWS       4

#define KEY_TRIS_VAL       0xF0 /* Row=0 --> output; Col=1 --> input */


/* KEYPAD TIMING PARAMETERS 
   -- assuming Timer0
   Counting up to 1s requires counting 1/TMR0_PERIOD == TMR0_FREQ
   so 200ms = 0.2s = 0.2 * TMR0_FREQ = TMR0_FREQ/5
*/
#define KEY_DEBOUNCE_TIME  TMR0_FREQ/5  /* debounce time of 200 ms */
#define KEY_REPEAT_TIME    TMR0_FREQ    /* repeat rate of 1Hz */  


/* KEYPAD CODES */
/* The codes are designed such that the row number is in the lower bits of
   of the keypad port. The column number is contained in the higher bits.
   Below is a depiction of the keypad showing its key code values with the key
   value in brackets
   
             column0        column1        column2        column3
   row0      1110 1110(1)   1101 1110(2)   1011 1110(3)   0111 1110(A)
   row1      1110 1101(4)   1101 1101(5)   1011 1101(6)   0111 1101(B)
   row2      1110 1011(7)   1101 1011(8)   1011 1011(9)   0111 1011(C)
   row3      1110 0111(*)   1101 0111(0)   1011 0111(#)   0111 0111(D)
 */
#define KEY_1 0b11101110
#define KEY_2 0b11011110
#define KEY_3 0b10111110
#define KEY_A 0b01111110
#define KEY_4 0b11101101
#define KEY_5 0b11011101
#define KEY_6 0b10111101
#define KEY_B 0b01111101
#define KEY_7 0b11101011
#define KEY_8 0b11011011
#define KEY_9 0b10111011
#define KEY_C 0b01111011
#define KEY_S 0b11100111
#define KEY_0 0b11010111
#define KEY_P 0b10110111
#define KEY_D 0b01110111

/* any other key code is considered invalid, thus multiple key presses are
   ignored
*/
#define NO_KEY_PRESSED KEY_COLS_MASK


/* MACROS */
/* set all row bits to 1 */
#define NO_ROW() (KEY_ROWS |= ~(KEY_COLS_MASK))  

/* select a row by setting its bit to 0, and all others to 1.
   - no error checking. i has to be from 0 to 3 */
#define SET_ROW(i)  NO_ROW(); (KEY_ROWS &= (~(1<<i+KEY_ROWS_OFFSET))) 


/* FUNCTION PROTOTYPES */
/* initializes the keypad and ScanAndDebounce variables */
extern void KeypadInit(void);

/* checks if a fully debounced key is available */
extern unsigned char IsAKey(void);

/* Gets a key from the keypad */
extern unsigned char GetKey(void);

/* scans the keypad for keypresses and debounces the presses */
extern void ScanAndDebounce(void);


#endif                                                            /* KEYPAD_H */
