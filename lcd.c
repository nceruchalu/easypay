/*
 * -----------------------------------------------------------------------------
 * -----                              LCD.C                                -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is a library of functions for interfacing the PIC18F67K22 with a 20x4
 *   LCD (ST7066U driver) in its 8-bit databus mode
 *
 * Table of Contents:
 *   (private)
 *   GenSpecChars  - load special characters into LCD's CGRAM
 *
 *   (public)
 *   LcdCommand    - write a command byte to the LCD.
 *   LcdWrite      - write a data byte to the LCD.
 *   LcdWaitBF     - wait for LCD BF (Busy Flag) to be clear
 *   LcdInit       - initialize the LCD
 *   LcdClear      - clear the LCD and home the cursor
 *   LcdWriteStr   - write a string of chars to the LCD
 *   LcdWriteInt   - write an integer to the LCD
 *   LcdWriteHex   - write a hex byte to the LCD
 *   LcdWriteFill  - write strings to fill all rows of display
 *   LcdCursor     - move the cursor to a specified location on the LCD
 *
 * Assumptions:
 *   Hardware Hookup defined in include file.
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Dec. 16, 2012      Nnoduka Eruchalu     Initial Revision
 *   Apr. 21, 2013      Nnoduka Eruchalu     Added LcdWriteFill
 *   May  02, 2013      Nnoduka Eruchalu     Added LcdWriteHex
 *   May  15, 2013      Nnoduka Eruchalu     LcdWriteInt argument changed:
 *                                           unsigned int32_t -> uint32_t
 */

#include <htc.h>
#include <stdio.h>
#include "lcd.h"

/* tables local to this file 
 ---------------------------- */
/*
 * NairaTable
 * Description: This is the table containing the bit patten for the rows (group 
 *              of bits representing pixels) in the CGRAM that will be used in 
 *              generating the image of the Naira Symbol (=N=).
 *              This table MUST be exactly LCD_CHAR_HEIGHT rows long.
 *
 * Revision History:
 *  Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
static char NairaTable[LCD_CHAR_HEIGHT] = {
  0b00010001,  /* row 0 */
  0b00010001,  /* row 1 */
  0b00011111,  /* row 2 */
  0b00010101,  /* row 3 */
  0b00011111,  /* row 4 */
  0b00010001,  /* row 5 */
  0b00010001,  /* row 6 */
  0b00000000   /* row 7 */
};


/* functions local to this file 
 ------------------------------- */
static void GenSpecChars(void);


/*
 * LcdCommand
 * Description: This procedure simply writes a command byte to the LCD.
 *              It does not return until the LCD's busy flag is cleared.
 *
 * Argument:    c: command byte
 * Return:      None
 *
 * Input:       None
 * Output:      LCD
 * 
 * Operation:   Send the byte, then wait for the BF to be cleared.
 *
 * Revision History:
 *   Dec. 16, 2012      Nnoduka Eruchalu     Initial Revision
 */
void LcdCommand(unsigned char c)
{
  LCD_DATA_LAT = c;           /* put data on output port */
  CLEAR_RS();                 /* RS = 0: Command */
  CLEAR_RW();                 /* R/W = 0: Write */
  LCD_STROBE();
  LcdWaitBF();               /* dont exit until the LCD is no longer busy */
}


/*
 * LcdWrite
 * Description: This procedure simply writes a data byte to the LCD.
 *              It does not return until the LCD's busy flag is cleared.
 *
 * Argument:    c: data byte
 * Return:      None
 *
 * Input:       None
 * Output:      LCD
 * 
 * Operation:   Send the byte, then wait for the BF to be cleared.
 *
 * Revision History:
 *   Dec. 16, 2012      Nnoduka Eruchalu     Initial Revision
 */
void LcdWrite(unsigned char c)
{
  LCD_DATA_LAT = c;           /* put data on output port */
  SET_RS();                   /* RS = 1: Data */
  CLEAR_RW();                 /* R/W = 0: Write */
  LCD_STROBE();
  LcdWaitBF();               /* dont exit until the LCD is no longer busy */
}

/*
 * LcdWaitBF
 * Description: This procedure simply loops until the LCD is not busy.
 *              This clears the RS bit.
 *
 * Argument:    None
 * Return:      None
 *
 * Input:       LCD's Busy Flag
 * Output:      None
 *
 * Operation:   Keep on looping and reading the LCD busy flag. Exit when it 
 *              indicates the LCD is not busy.
 *
 * Revision History:
 *   Dec. 16, 2012      Nnoduka Eruchalu     Initial Revision
 */
void LcdWaitBF(void)
{
  unsigned char busy, status=0x00;
  LCD_DATA_TRIS = 0xFF;        /* when reading a port change it to an input */
  
  CLEAR_RS();                  /* prepare to read BF and Address Counter */
  SET_RW();                    /* and put the LCD in read mode */
  
  do {
    SET_E();                   /* during reads the E has to be active */
    __delay_us(0.5);           /* wait tPW for data to become available */
    
    status = LCD_DATA_PORT;    /* read in value on data lines */
    busy = status & 0x80;      /* busy flag is highest status bit */
    
    __delay_us(0.5);
    
    CLEAR_E();                 /* pull E low for at least tC-tPW */ 
    __delay_us(1);            
  } while(busy);
  
  /* put the LCD in write mode */
  CLEAR_RW();                  /* in write mode when R/W\ is cleared */
  LCD_DATA_TRIS = 0x00;        /* and the I/O pins are set as outputs */
}

/*
 * LcdInit
 * Description: This initializes the LCD. This must be called before the LCD can
 *              be used.
 *              Thus this function has to be called before calling any other 
 *              LCD-interface functions.
 *              The LCD is set to the following specifications:
 *                8-bit mode, 4-line display, 5x8 font
 *                cursor INCs, display doesn't shift
 *                cursor visible, cursor blinking
 *
 * Argument:    None
 * Return:      None
 *
 * Input:       None
 * Output:      LCD
 *
 * Operation:   
 *   Really just follows standard initialization sequence. See sketch below
 *
 *   POWER ON
 *       |
 *       |  Wait time >40ms
 *      \|/
 *   FUNCTION SET (RS = 0, RW=0, DB = 0b0011NFXX) [BF cannot be checked b4 this]
 *       |
 *       |  Wait time >37us
 *      \|/
 *   FUNCTION SET (RS = 0, RW=0, DB = 0b0011NFXX) [BF cannot be checked b4 this]
 *       |
 *       |  Wait time >37us
 *      \|/
 *   DISPLAY ON/OFF control (RS = 0, RW=0, DB = 0b00001DCB)
 *       |
 *       |  Wait time >37us
 *      \|/
 *   DISPLAY Clear (RS=0, RW=0, DB=0b00000001)
 *       |
 *       |  Wait time >1.52ms
 *      \|/
 *   Entry Mode Set (RS=0, RW=0, DB=0b0000001{I/D}S)
 *       |
 *       |  Wait time >37us
 *      \|/
 *   Initialization End
 *
 * Revision History:
 *   Dec. 16, 2012      Nnoduka Eruchalu     Initial Revision
 */
void LcdInit(void)
{
  LCD_DATA_TRIS = 0x00;        /* setup LCD IO ports as outputs */
  LCD_E_TRIS = 0;
  LCD_RW_TRIS = 0;
  LCD_RS_TRIS = 0;
  
  LCD_DATA_LAT = 0;            /* clear IO lines*/
  CLEAR_RS(); CLEAR_RW(); CLEAR_E();
  
  __delay_ms(40);              
  
  LCD_DATA_LAT = LCD_FUNC_SET; /* FUNCTION SET, done manually to prevent a */
  LCD_STROBE();                /* BF check before next command */
  __delay_us(40);
  
  LcdCommand(LCD_FUNC_SET);    /* FUNCTION SET, again done manually */
  LCD_STROBE();
  __delay_us(40);
  
  LcdCommand(LCD_ON);          /* DISPLAY ON/OFF control: Turn display on */
  __delay_us(40);
  
  LcdCommand(LCD_CLEAR);        /* DISPLAY Clear */
  __delay_ms(2);
  
  LcdCommand(LCD_ENTRY_MD);     /* ENTRY mode set */
  
  GenSpecChars();               /* Now create some special characters */
}


/*
 * LcdClear
 * Description: This function clears the LCD and return the cursor to the 
 *              starting position
 *              This function doesnt return until the LCD's BF is cleared.
 *
 * Arguments:   None
 * Return:      None
 *
 * Input:       None
 * Output:      LCD
 *
 * Operation:   Write the clear command to the instruction register of the LCD.
 *
 * Revision History:
 *   Dec. 16, 2012      Nnoduka Eruchalu     Initial Revision
 */
void LcdClear(void)
{
  LcdCommand(0x01);
}


/*
 * LcdWriteStr
 * Description: This function writes a string of characters to the LCD
 *              This function doesnt return until the LCD's BF is cleared.
 *
 * Arguments:   str: string to write to LCD
 * Return:      None
 *
 * Input:       None
 * Output:      LCD
 *
 * Operation:   Choose the data register of the LCD then write each char of the 
 *              string to the LCD.
 *
 * Revision History:
 *   Dec. 16, 2012      Nnoduka Eruchalu     Initial Revision
 */
void LcdWriteStr(const char *str)
{
  while (*str != '\0') {
    LcdWrite(*str++);
  }
}


/*
 * LcdWriteInt
 * Description: This function writes an integer to the lcd
 *              This function doesnt return until the LCD's BF is cleared.
 *
 * Arguments:   num: integer to write.
 * Return:      None
 *
 * Input:       None
 * Output:      LCD
 * 
 * Operation:   Convert the integer to a string then write it to the LCD.
 *
 * Issues:      Buffer Overflow/Truncation Issues
 *
 * Revision History:
 *   Dec. 16, 2012      Nnoduka Eruchalu     Initial Revision
 */
void LcdWriteInt(uint32_t num)
{
  char buffer[LCD_WIDTH];   /* buffer to hold int string */
  size_t i;
  sprintf(buffer, "%lu", num);
  
  for(i=0; ((buffer[i]!='\0') && (i<LCD_WIDTH)); i++) {
    LcdWrite(buffer[i]);
  }  
}


/*
 * LcdWriteHex
 * Description: This function writes a hex byte to the LCD.
 *              This function doesnt return until the LCD's BF is cleared.
 *
 * Arguments:   num = hex number in range [0, 255]
 * Return:      None
 *
 * Input:       None
 * Output:      LCD
 *
 * Operation:   Extract high nibble then extract low nibble.
 *              For each nibble:
 *                if it is in range [0,9] print it as an ASCII numeric, 
 *                  '0' + nibble
 *                if it is in range [10,15] print it as an ASCII alpha,
 *                   nibble - 10 + 'A'
 *
 * Limitations: hex input must  1 byte long so in range [0, 255]
 *
 * Revision History:
 *  May  02, 2012      Nnoduka Eruchalu     Initial Revision
 */
void LcdWriteHex(unsigned int8_t num)
{
  char nibble;
  /* extract high nibble and write out numeric or alpha */
  nibble = (num & 0xF0) >> 4;
  if (nibble < 10) LcdWrite('0'+nibble);
  else             LcdWrite(nibble-10+'A');
  
  /* extract low nibble and write out numeric or alpha */
  nibble = (num & 0x0F);
  if (nibble < 10) LcdWrite('0'+nibble);
  else             LcdWrite(nibble-10+'A');
  
}


/*
 * LcdWriteFill
 * Description: This function writes strings to all rows of display.
 * 
 * Arguments:   displaytable - a table of strings (one string per row).
 *                each string is of length (LCD_WIDTH+1). The +1 accounts for
 *                each string's NULL-terminator.
 * Return:      None
 *
 * Input:       None
 * Output:      LCD
 *
 * Operation:   Clear LCD. Return Cursor Home. Write strings in table, one row 
 *              at a time.
 *
 * Revision History:
 *   Apr. 21, 2013      Nnoduka Eruchalu     Initial Revision
 *   May  14, 2013      Nnoduka Eruchalu     Setting cursor per row
 */
void LcdWriteFill(const char (*displaytable)[LCD_WIDTH+1]) {
  size_t i;                       /* row counter             */
  LcdCommand(LCD_CLEAR);          /* clear LCD               */
  LcdCommand(LCD_RET_HOME);       /* return cursor home      */
  for (i=0; i<LCD_HEIGHT; i++) {  /* write strings in table, */
    LcdCursor(i,0);               /* one row at a time       */
    LcdWriteStr(displaytable[i]); 
  }
}


/* LcdCursor
 * Description: This function moves the cursor to a specific position
 *              Below is a DDRAM address map by Row:
 *                Row 1:  0x00  -->  0x13
 *                Row 2:  0x40  -->  0x53
 *                Row 3:  0x14  -->  0x27
 *                Row 4:  0x54  -->  0x67
 *
 * Arguments:   row: 0-indexed row number
 *              col: 0-indexed column number
 * Return:      None
 *
 * Input:       None
 * Output:      LCD
 * 
 * Operation:   Choose the base register of the LCD then update the cursor 
 *              location
 * 
 * Error Checking: If row >= LCD_HEIGHT or col >= LCD_WIDTH, it is reset to 0.
 *
 * Revision History:
 *   Dec. 16, 2012      Nnoduka Eruchalu     Initial Revision
 *   Apr. 20, 2013      Nnoduka Eruchalu     Updated to actually compute loc 
 *                                           from row and col arguments.
 */
void LcdCursor(uint8_t row, uint8_t col)
{
  uint8_t loc;
  
  if (row >LCD_HEIGHT) row = 0;      /* error checking of indices */
  if (col >= LCD_WIDTH) col = 0;
  
  switch(row) {  /* get LCD's relative DDRAM location */
  case 0:
    loc = 0x00;
    break;
  case 1:
    loc = 0x40;
    break;
  case 2:
    loc = 0x14;
    break;
  case 3:
    loc = 0x54;
    break;
  default:
    break;
  }
  
  loc += col;  /* offset relative DDRAM location by column */
  
  LcdCommand(DDRAM_BASE+loc); /* have to write to absolute DDRAM location */
}


/* GenSpecChars
 * Description: This procedure loads the CGRAM with custom characters.
 *              This procedure should be called after initializing the LCD.
 *
 * Arguments:   None
 * Return:      None
 *
 * Input:       None
 * Output:      LCD CGRAM
 * 
 * Operation:   For each custom character, call this ShapeX, the procedure sets 
 *                the CGRAM address to point to the corresponding base address 
 *                as defined in lcd.h.
 *              Then use the values in ShapeXTable to write the pixel data.
 *              After generating all the custom characters of interest, the 
 *                procedure returns the Cursor to a DDRAM location.
 *
 * Error Checking: None
 *
 * Revision History:
 *   Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
static void GenSpecChars(void)
{
  size_t i; /* looping index */
  /* Generate Naira Character */
  LcdCommand(NAIRA_BASE);              /* point to Naira Char's CGRAM address */
  for(i=0; i < LCD_CHAR_HEIGHT; i++) { /* Loop through and write the bits in */
    LcdWrite(NairaTable[i]);           /* the pixel rows */
  }
  
  /* Done generating special Characters */
  LcdCommand(DDRAM_BASE);              /* return the Cursor to a DDRAM loc */
  
}
