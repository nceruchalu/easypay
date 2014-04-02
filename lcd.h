/*
 * -----------------------------------------------------------------------------
 * -----                              LCD.H                                -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the header file for lcd.c, the library of functions for interfacing
 *   the PIC18F67K22 with a 20x4 LCD (ST7066U driver) in its 8-bit databus mode.
 *
 * Assumptions:
 *   All Hardware connections done here
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Dec. 16, 2012      Nnoduka Eruchalu     Initial Revision
 *   Apr. 21, 2013      Nnoduka Eruchalu     Added LcdWriteFill
 *   May  02, 2013      Nnoduka Eruchalu     Added LcdWriteHex
 *   May  15, 2013      Nnoduka Eruchalu     LcdWriteInt argument changed:
 *                                          unsigned int32_t -> uint32_t
 */

#ifndef LCD_H
#define LCD_H

/* library include files */
#include <htc.h>
#include <stdint.h>     /* for uint*_t */
#include <stdlib.h>     /* for size_t */

/* local include files */
#include "general.h"

/* HARDWARE CONNECTIONS */
#define LCD_DATA_LAT   LATD    /* use LAT for writing to I/O pins */
#define LCD_E_LAT      LATB0
#define LCD_RW_LAT     LATB1
#define LCD_RS_LAT     LATB2

#define LCD_DATA_TRIS  TRISD   /* set a bit to 1 to make it an input */
#define LCD_E_TRIS     TRISB0  /* set a bit to 0 to make it an output */
#define LCD_RW_TRIS    TRISB1
#define LCD_RS_TRIS    TRISB2

#define LCD_DATA_PORT  PORTD   /* use PORT for reading from I/O pins */


/* LCD CONTROL BIT MANIPULATIONS */
#define SET_E()        (LCD_E_LAT = 1)
#define SET_RW()       (LCD_RW_LAT = 1)
#define SET_RS()       (LCD_RS_LAT = 1)

#define CLEAR_E()      (LCD_E_LAT = 0)
#define CLEAR_RW()     (LCD_RW_LAT = 0)
#define CLEAR_RS()     (LCD_RS_LAT = 0)


/* MACROS */
#define LCD_STROBE()   SET_E(); __delay_us(1); CLEAR_E(); __delay_us(1)


/* CONSTANTS */
#define LCD_WIDTH       20     /* max number of characters on one row */
#define LCD_HEIGHT      4      /* number of rows */
#define LCD_CHAR_HEIGHT 8      /* number of rows in a custom character */
#define DDRAM_BASE      0x80   /* base address of DDRAM */


/* Specific LCD Command Definitions */
#define LCD_FUNC_SET  0x38     /* Function Set Command    */
                               /* 0011---- Required       */
                               /* ----1--- 2-line display */
                               /* -----0-- 5x8 character  */
                               /* ------00 Dont Care      */
                               /* 00111000 = 0x38         */

#define LCD_ENTRY_MD 0x06      /* Entry Mode Command           */
                               /* 000001-- Required            */
                               /* ------1- Cursor Increment    */
                               /* -------0 Don't shift display */
                               /* 00000110 = 0x06              */

#define LCD_ON        0x0F     /* Display On Command      */
                               /* 00001--- Required       */
                               /* -----1-- Display On     */
                               /* ------1- Cursor On      */
                               /* -------1 Cursor Blink   */
                               /* 00001111 = 0x0F         */

#define LCD_OFF       0x08     /* Display Off Command     */

#define LCD_CLEAR     0x01     /* Display Clear Command   */

#define LCD_RET_HOME  0x02     /* Return Cursor Home      */


/* ASCII codes for special characters in CGRAM */
#define NAIRA_CHAR  '\x01'


/* Character codes for special characters in CGRAM */
/* Do not use the first available index which is at 0H. This is because it is */
/* equivalent to ASCII_NULL which is not displayed and is a string-terminator */
#define NAIRA_CHAR_CODE  0x01


/* CGRAM Base Address of special characters in LCD */
#define CGRAM_BASE  0x40                            /* base address of CGRAM */
#define NAIRA_BASE  CGRAM_BASE + (NAIRA_CHAR_CODE << 3)/* base address of =N= */


/* FUNCTION PROTOTYPES */
/* write a command byte to the LCD */
extern void LcdCommand(unsigned char c);

/* write a data byte to the LCD */
extern void LcdWrite(unsigned char c);

/* wait on the LCD busy flag */
extern void LcdWaitBF(void);

/* initialize the LCD to some documented specs */
extern void LcdInit(void);

/* clear the LCd and return the cursor to the starting position */
extern void LcdClear(void);

/* write a string to the LCD */
extern void LcdWriteStr(const char *str);

/* write an integer to the LCD */
extern void LcdWriteInt(uint32_t num);

/* write a hex byte to the LCD */
extern void LcdWriteHex(unsigned uint8_t num);

/* write characters to fill all rows and columns of display */
extern void LcdWriteFill(const char (*displaytable)[LCD_WIDTH+1]);

/* move the cursor to a specific location */
extern void LcdCursor(uint8_t row, uint8_t col);


#endif                                                               /* LCD_H */
