/*
 * -----------------------------------------------------------------------------
 * -----                           EVENTPROC.C                             -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This file contains the event processing functions
 *
 * Table of Contents:
 *  (static functions)
 *   UpdateDisplay       - write string to specific row and column
 *   DisplayMoney        - write a number as a monetary value to a row,col
 *   DisplayTime         - write a time in hh:mm:ss to a row,col
 *   ClearNumber         - clear variables associated with recording numbers. 
 *   ClearBalance        - clear variables associated with account balance
 *   ClearParking        - clear variables associated with parking status
 *   AddDigit            - add a digit to the local number sequence.
 *   MobileGet           - Get Mobile Top-Up of a spefici amount
 *   UpdateExitOrUndo    - switch between *Exit* and *Undo* on number entry page
 *   ElapsedTime         - get number of elapsed milliseconds since last call
 *   ConvertTimeToMin    - convert integer time of hh:mm format to minutes
 *
 *  (interrupt service routine functions)
 *   EventTimer           - elapsed time counter
 *
 *  (update functions)
 *   NoUpdate            - do nothing
 *   UpdatePin           - flash newest pin digit before hiding it.
 *   UpdateAccount       - write in account balance on Account Page
 *   UpdatePark          - write in parking space and time left
 *   UpdateParkSpace     - write in current space or entered space
 *   UpdateParkTime      - write in entered parking time
 *  
 *  (event processing functions)
 *   NoAction            - do nothing
 *   ResetAction         - reset system variables excluding state
 *   GetUserData         - get user info from tapped EasyCard
 *   AddPinDigit         - add a digit to the pin number sequence
 *   ProcessPin          - process pin number
 *   ExitOrUndoDigit     - Exit to welcome page or undo last digit entry
 *   GetAcctBalance      - get user's account balance
 *   ProcessAcctRecharge - process EasyTopup
 *   GetParkStatus       - get current parking status
 *   AddParkSpaceDigit   - add parking space digit
 *   ProcessParkSpace    - process parking space number 
 *   AddParkTimeDigit    - add parking time digit
 *   ProcessParkTime     - process parking time number 
 *   GetUtilityData      - get user's utility meter #s
 *   MobileGetMtn100     - Get MTN VTU Recharge of 100, 200, 400, 750, 1500
 *   MobileGetMtn200
 *   MobileGetMtn400
 *   MobileGetMtn750
 *   MobileGetMtn1500
 *   MobileGetGlo100     - Get Glo QuickCharge of 100, 200, 500, 1000, 3000
 *   MobileGetGlo200
 *   MobileGetGlo500
 *   MobileGetGlo1000
 *   MobileGetGlo3000
 *   MobileGetAirtel100  - Get Airtel Top-up of 100, 200, 500, 1000
 *   MobileGetAirtel200
 *   MobileGetAirtel500
 *   MobileGetAirtel1000
 *   MobileGetEtisalat100 -Get Etisalat E-Top up of 100, 200, 500, 1000
 *   MobileGetEtisalat200
 *   MobileGetEtisalat500
 *   MobileGetEtisalat1000
 *
 *
 * Assumptions:
 *   None
 *
 * Limitations:
 *   None
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 *   May  15, 2013      Nnoduka Eruchalu     Changed name ConvertTime to
 *                                          ConvertTimeToMin
 */
#include <stdint.h>     /* for uint*_t */
#include <stdlib.h>     /* for size_t  */
#include <htc.h>
#include "general.h"
#include "delay.h"      /* for delay_s */
#include "lcd.h"
#include "interface.h"
#include "eventproc.h"
#include "data.h"
#include "smartcard.h"


/* shared variables have to be local to this file */
static uint32_t number;            /* entered number sequences are saved here */
static uint8_t  num_digits;        /* number of entered digits */
static uint8_t  updated_number;    /* (bool) if the number is updated */

static uint32_t balance;           /* account balance (in kobo) */
static uint8_t  updated_balance;   /* (bool) balance has been updated */

static uint32_t parking_space;     /* parking space number */
static int32_t parking_time;       /* parking time left  */
                                   /* (uints of 1/TIME_SCALE seconds) */
static uint8_t  updated_space;     /* (bool) parking space has been updated */

static uint32_t elapsed_ms;        /* elapsed milliseconds */

static uint8_t uid_easycard[7];   /* UID of EasyCard  */ 
static uint8_t uid_easytopup[7];  /* UID of EasyTopup */ 


/* static functions local to this file */
static void UpdateDisplay(uint8_t row, uint8_t col, const char *str);
static void DisplayMoney(uint8_t row, uint8_t col, uint32_t n);
static void DisplayTime(uint8_t row, uint8_t col, uint32_t time, uint8_t unit);
static void ClearNumber(void);
static void ClearBalance(void);
static void ClearParking(void);
static void UpdateExitOrUndo(void);
static void AddDigit(eventcode digit, uint8_t num_digits_limit);
static void MobileGet(uint32_t amount);
static uint32_t ElapsedTime(void);
static uint32_t ConvertTimeToMin(uint32_t time, uint8_t num_time_digits);


/*
 * UpdateDisplay
 * Description:      Update the display with the passed in String starting
 *                   from the specified row and column
 *
 * Arguments:        row: 0-indexed LCD display row
 *                   col: 0-indexed LCD display column
 *                   str: string
 * Return:           None
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        Set LCD Cursor, and Write the string.
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
static void UpdateDisplay(uint8_t row, uint8_t col, const char *str)
{
  LcdCursor(row, col); /* set LCD Cursor   */
  LcdWriteStr(str);    /* write the string */
}


/*
 * DisplayMoney
 * Description:      At the specified row and column, write a monetary value
 *                   that is supplied in kobos.
 *                   Example: n = 123456 becomes N1,234.56
 *                            n = 0      becomes N0.00
 *
 * Arguments:        row: 0-indexed LCD row
 *                   col: 0-indexed LCD column
 *                   n:   monetary amount in kobos
 * Return:           None
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        Write exactly 2 digits after a decimal point, and at least
 *                   1 digit before the decimal point. To ensure you get a
 *                   digit before decimal point, use a do-while loop so that
 *                   even if the number starts out as 0, it is written at least
 *                   once.
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 25, 2013      Nnoduka Eruchalu     Initial Revision
 */
static void DisplayMoney(uint8_t row, uint8_t col, uint32_t n)
{
  char retbuf[30];                    /* allocate space for writing string    */
  char *p = &retbuf[sizeof(retbuf)-1];/* and prepare to start from end        */
  int i = 0;                          /* counter of chars written             */
  
  
  *p = '\0';                          /* strings end with a null terminator   */
  
  do {                                /* pre-decimal point loop               */
    *--p = '0' + (n % 10);            /* save next char in ASCII by adding '0'*/
    n /= 10;                          /* and drop the saved char from number  */
    i++;                              /* 1 more digit written                 */
  } while(i<2);                       /* write at least 2 digits b4 decimal pt*/
  *--p = '.';                         /* write decimal point                  */
  i = 0;                              /* and reset counter for next loop      */
  
  do {                                /* comma insertion loop                 */
    if ((i%3 == 0) && (i != 0)) {     /* if on a 3rd digit                    */
      *--p = ',';                     /* then next character is a comma       */
    }
    *--p = '0' + (n % 10);            /* save next char in ASCII by adding '0'*/
    n /= 10;                          /* and drop the saved char from number  */
    i++;                              /* 1 more digit written                 */
  } while (n != 0);                   /* keep going till number is exhausted  */
  
  *--p = NAIRA_CHAR;                  /* throw in currency logo               */
  
  UpdateDisplay(row, col, p);         /* FINALLY, update display              */
}


/*
 * DisplayTime
 * Description:      At the specified row and column, write a time with format:
 *                   hh:mm:ss
 *                   Example: time = 721*60 becomes 12:01:00 or 12:01
 *                            n = 0         becomes 00:00:00 or 00:00
 *
 * Arguments:        row: 0-indexed LCD row
 *                   col: 0-indexed LCD column
 *                   time: in seconds or minutes
 *                   unit: DISPLAYTIME_SECS or DIPLAYTIME_MINS
 * Return:           None
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        time-separator is ':'
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 26, 2013      Nnoduka Eruchalu     Initial Revision
 */
static void DisplayTime(uint8_t row, uint8_t col, uint32_t time, uint8_t unit)
{
  char retbuf[9];           /* hh:mm:ss is 8 chars and +1 for null-terminator */
  char *p = &retbuf[sizeof(retbuf)-1];    /* prepare to start from end        */
  uint8_t tmp;                            /* saves hour, minutes, seconds     */
  int i=0;
  
  *p = '\0';                              /* strings end with NULL            */
  
  do {
    if ((i%2 == 0) && (i != 0)) {        /* write a colon after every 2 chars */
      *--p = ':';
    }
    
    tmp = time%60;                       /* get a block: hh,mm, or ss         */
    *--p = '0'+(tmp%10); i++;            /* write lower digit                 */
    *--p = '0'+(tmp/10); i++;            /* write upper digit                 */
    time /= 60;                          /* drop the entire block             */

  } while (i < unit);
    
  UpdateDisplay(row, col, p);            /* FINALLY, update display           */
}


/*
 * ClearNumber
 * Description:      Clear the variables associated with entered numbers.
 *
 * Arguments:        None
 * Return:           None
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        reset number to 0, num_digits to 0, update_number to FALSE
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
static void ClearNumber(void)
{
  number = 0;
  num_digits = 0;
  updated_number = FALSE;
}


/*
 * ClearBalance
 * Description:      Clear the variables associated with account balance.
 *
 * Arguments:        None
 * Return:           None
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        reset balance to 0, and flag indicating it has updated,
 *                   updated_balance
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
static void ClearBalance(void)
{
  balance = 5000000; /* TODO: actually reset to 0 */
  updated_balance = FALSE;
}


/*
 * ClearParking
 * Description:      Clear the variables associated with parking status.
 *
 * Arguments:        None
 * Return:           None
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        reset parking_space to 0, parking_time to 0, and flags
 *                   indicating space have been updated
 *                   Call ElapsedTime() to reset elapsed_time
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
static void ClearParking(void)
{
  parking_space = 0;
  parking_time = 0;
  updated_space = FALSE;
  ElapsedTime();
}


/*
 * UpdateExitOrUndo
 * Description:      Appropriately select "exit" or "undo" on number entry pages
 *                   and show the right text.
 *
 * Arguments:        None
 * Return:           None
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        Move cursor appopriately. If there is no digit on the page
 *                   then can exit, else will be in undo mode.
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
static void UpdateExitOrUndo(void)
{
  LcdCursor(3,0);          /* move cursor to row 3, column 0        */
  if (num_digits > 0)      /* if there are digits then in undo mode */
    LcdWriteStr("*Undo*");
  else                     /* else in exit mode                     */
    LcdWriteStr("*Exit*");
}


/*
 * EventTimer
 * Description:      This function keeps count of the number of milliseconds
 *                   passing by. This procedure should be called every 1ms.
 *
 * Arguments:        None
 * Return:           None
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        increment elapsed_ms
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: elapsed_ms - read and modified
 *
 * Assumptions:      Assumes elapsed_ms will be reset to 0 before it overflows
 *
 * Revision History:
 *   Apr. 26, 2013      Nnoduka Eruchalu     Initial Revision
 */
void EventTimer(void)
{
  elapsed_ms++;
}


/*
 * NoUpdate
 * Description:      This function handles when a state has no update routine
 *                   It just returns the current state as the next state.
 *
 * Arguments:        curr_state - the current system state
 * Return:           nextstate  - the next system state
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        Return the argument.
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 22, 2013      Nnoduka Eruchalu     Initial Revision
 */
state NoUpdate(state curr_state)
{
  return curr_state;
}


/*
 * UpdatePin
 * Description:      Flash the newest pin digit before changing it to a '*'
 *
 * Arguments:        curr_state - the current system state
 * Return:           nextstate  - the next system state
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        if there is a new digit, show the preceding digits as '*'
 *                   Show this digit as it is for some time, then change it to
 *                   '*'. Follow this up with updating the exit/undo
 *                   functionality and resetting the updated_number flag, since
 *                   the update has been handled.
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 26, 2013      Nnoduka Eruchalu     Initial Revision
 */
state UpdatePin(state curr_state)
{
  int i;                /* counter for loop through number digits   */
  
  if (updated_number) { /* only do update if pin number has changed */
    UpdateExitOrUndo();
    
    LcdCursor(1,8);     /* center the pin number on row 1           */
    for(i=0; i<num_digits-1; i++ ) LcdWrite('*'); /* hide earlier digits */
    if (num_digits > 0) LcdWriteInt(number%10);   /* show newest digit   */
    __delay_s(PIN_FLASH_TIME);                    /* really just a flash */
    
    LcdCursor(1,8);    /* prepare to overwrite what was just shown */
    for(i=0; i<num_digits; i++ )  LcdWrite('*'); /* now hide all digits */

    while(i++<NUM_PIN_DIGITS) LcdWrite(' '); /* blank out earlier writes */
    
    LcdCursor(1,8+num_digits);

    updated_number = FALSE;
  }
  
  return curr_state;   /* current state doesn't change */
}


/*
 * UpdateAccount
 * Description:      Show the updated account balance on the new page
 *
 * Arguments:        curr_state - the current system state
 * Return:           nextstate  - the next system state
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        if there is an updated balance, show it on the account 
 *                   page. 
 *                   Then reset the updated_balance flag to FALSE, since
 *                   the update has been handled.
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 26, 2013      Nnoduka Eruchalu     Initial Revision
 */
state UpdateAccount(state curr_state)
{
  if (updated_balance) {        /* only do work if there has been an update */
    DisplayMoney(1,9, balance); /* display the new balance as*/
    updated_balance = FALSE;
  }
  return curr_state;
}


/*
 * UpdatePark
 * Description:      write in parking space and time left
 *
 * Arguments:        curr_state - the current system state
 * Return:           nextstate  - the next system state
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        if there is an updated parking space, show it on the
 *                   parking page. 
 *                   Then reset the updated_space flag to FALSE, since
 *                   the update has been handled.
 *                   display new parking time if time has elapsed
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 26, 2013      Nnoduka Eruchalu     Initial Revision
 */
state UpdatePark(state curr_state)
{
  int32_t old_parking_time = parking_time;

  if (updated_space) {
    LcdCursor(0,12); LcdWriteInt(parking_space); /* write space number */
    updated_space = FALSE;
  }

  /* always update the displayed time */
  parking_time -= ElapsedTime();   /* get elapsed seconds   */
  if (parking_time < 0)                          /* but keep time         */
    parking_time = 0;                            /* non-negative          */
 
   /* if time has changed, update it in seconds */
  if (parking_time/TIME_SCALE != old_parking_time/TIME_SCALE)
    DisplayTime(1,12,parking_time/TIME_SCALE, DISPLAYTIME_SECS); /* update it (in secs) */
  
  return curr_state;
}


/*
 * UpdateParkSpace
 * Description:      write in current parking space or new space if one is
 *                   entered
 *
 * Arguments:        curr_state - the current system state
 * Return:           nextstate  - the next system state
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        Only run if there is a changed number digit.
 *                   if there isn't an entered digit, display current space.
 *                   else display currently entered sequence.
 *                   Update Exit/Undo button and clear flag indicating a changed
 *                   digit.
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 27, 2013      Nnoduka Eruchalu     Initial Revision
 */
state UpdateParkSpace(state curr_state)
{
  if (updated_number) { /* only do update if space number has changed */
    UpdateExitOrUndo();
    
    LcdCursor(1,8);     /* center the space number on row 1           */
    
    if(num_digits > 0)  /* write currently entered sequence if exists */
      LcdWriteInt(number);
    else                /* else write in current space               */
      LcdWriteInt(parking_space);
    
    updated_number = FALSE;
  }
  
  return curr_state;   /* current state doesn't change */ 
}


/*
 * UpdateParkTime
 * Description:      write in entered parking time
 *
 * Arguments:        curr_state - the current system state
 * Return:           nextstate  - the next system state
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        Only run if there is a changed number digit.
 *                   Display current entered sequence in hh:mm format
 *                   Update Exit/Undo button and clear flag indicating a changed
 *                   digit.
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 27, 2013      Nnoduka Eruchalu     Initial Revision
 */
state UpdateParkTime(state curr_state)
{
  if (updated_number) {  /* only do update if time number has changed */
    UpdateExitOrUndo();
    /* update it (in mins) */
    DisplayTime(1,11,ConvertTimeToMin(number, num_digits), DISPLAYTIME_MINS);
    /* place cursor after last written character and skip colon */
    LcdCursor(1,11+((num_digits <= 2) ? num_digits : (num_digits+1)));
    
    updated_number = FALSE;
  }
  
  return curr_state;   /* current state doesn't change */ 
}



/*
 * NoAction
 * Description:      This function handles an event when there is nothing to be
 *                   done. It just returns next the next state.
 *
 * Arguments:        nextstate- expected next state
 *                   event    - current event
 * Return:           nextstate- actual next state
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        Return the argument.
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
state NoAction(state nextstate, eventcode event)
{
  return nextstate;
}


/*
 * ResetAction
 * Description:      This function reset system variables excluding state.
 *
 * Arguments:        nextstate- expected next state
 *                   event    - current event
 * Return:           nextstate- actual next state
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        Clear variables and return the argument.
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
state ResetAction(state nextstate, eventcode event)
{
  ClearNumber();
  ClearBalance();
  ClearParking();
  return nextstate;
}


/*
 * GetUserData
 * Description:      Get user data from last tapped EasyCard
 *
 * Arguments:        nextstate- expected next state
 *                   event    - current event
 * Return:           nextstate- actual next state
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        Get user data from PICC
 *                   End with a call to ResetAction()
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   May  16, 2013      Nnoduka Eruchalu     Initial Revision
 */
state GetUserData(state nextstate, eventcode event)
{
  size_t i;
  mifare_tag *tag; /* EasyCard representation */
  tag = GetCardTag();  
  for(i=0; i<7; i++) { /* copy UID from tag */
    uid_easycard[i] = tag->uid[i];
  }
  return ResetAction(nextstate, event); /* perform action reset */
}


/*
 * AddDigit
 * Description:      Add a digit to the pin number sequence
 *
 * Arguments:        digit            - digit to be added to number sequence
 *                   num_digits_limit - maximum number of digits in sequence
 * Return:           None
 *
 * Input:            None.
 * Output:           None
 *
 * Operation:        If number of digits is less than num_digit_limit:
 *                     Multiply current number by 10 then add the keycode value
 *                     to it, and update number of digits entered. 
 *
 * Error Handling:   None. 
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: number - read and moified
 * Assumptions:      keycode values go 0 to 9 for corresponding keys <0> to <9>
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
static void AddDigit(eventcode digit, uint8_t num_digits_limit)
{
  if (num_digits < num_digits_limit) {  /* if number not yet complete  */
    number = number*10 + digit;         /* add in new digit and update */
    num_digits += 1;                    /* number of digits thus far   */
    updated_number = TRUE;              /* captured a new digit        */
  }  
}


/*
 * MobileGet
 * Description:      Function for getting mobile topup of a specific amount
 *                   
 * Arguments:        amount: value of mobile recharge to process (in Naira)
 * Return:           None.
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        Flash success message before returning home
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 24, 2013      Nnoduka Eruchalu     Initial Revision
 *   Apr. 25, 2013      Nnoduka Eruchalu     Added DisplayMoney
 */
static void MobileGet(uint32_t amount)
{
   /* update LCD */
  UpdateDisplay(1, 0, "                    "); /* row 1, col 0   */
  UpdateDisplay(2, 0, " Successful Top-Up  "); /* row 2, col 0   */
  UpdateDisplay(3, 0, "+                   "); /* row 3, col 0   */
  DisplayMoney(3, 1, amount*100);              /* show amount of topup   */
  balance -= amount*100;                       /* update balance. */
  __delay_s(MSG_FLASH_TIME);                   /* a msg flash takes time */
}



/*
 * ElapsedTime
 * Description:      Get the number of ms that have elapsed since the last
 *                   time this function was called. 
 *                   
 * Arguments:        None
 * Return:           elapsed milliseconds
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        Ideally: set retval to 0, and exchange its value with
 *                   elapsed_ms. This prevents critical code, gets the elapsed
 *                   time and resets the counter.
 *                   However the asm instruction, EXCH, used 16-bit registers
 *                   So will simply turn off interrupts around critical code.
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 * Todo:             Fix Critical code: Ideally this code should be:
 *                   retval = 0;
 *                   XCHG retval, elapsed_ms
 *                   return retval
 *
 * Revision History:
 *   Apr. 26, 2013      Nnoduka Eruchalu     Initial Revision
 */
static uint32_t ElapsedTime(void)
{
  uint32_t retval;
  
  retval = elapsed_ms;     /* get elapsed time and clear counter   */
  elapsed_ms = 0;
  return retval;
}


/*
 * ConvertTimeToMin
 * Description:      Convert time argument of format hh:mm to minutes
 *                   Example time = 1120 represents 11:20
 *                           this is converted to  11*60 + 20 = 680 seconds 
 *                   
 * Arguments:        time            - which represents hh:mm using 4 digits
 *                   time_num_digits - number of digits entered thus far from
 *                                     left to right
 * Return:           actual number of minutes in the time argument
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        Ideally: set retval to 0, and exchange its value with
 *                   elapsed_ms. This prevents critical code, gets the elapsed
 *                   time and resets the counter.
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 26, 2013      Nnoduka Eruchalu     Initial Revision
 *   May  15, 2013      Nnoduka Eruchalu     Changed name ConvertTime to
 *                                           ConvertTimeToMin
 */
static uint32_t ConvertTimeToMin(uint32_t time, uint8_t num_time_digits)
{
  uint8_t hours, minutes;
  
  switch(num_time_digits) { /* break it into hours and minutes */
  case 1:                 /* entered format is: h*:** */
    hours = time*10;
    minutes = 0;
    break;
    
  case 2:                /* entered format is hh:** */
    hours = time;      
    minutes = 0;
    break;
    
  case 3:                /* entered format is hh:m* */
    hours = time/10;
    minutes = (time%10) *  10;
    break;
    
  case 4:                /* entered format is hh:mm */
    hours = time/100;
    minutes = time%100;
    break;
    
  default:               /* no time digits is 00:00 */
    hours = 0;
    minutes = 0;
    break;
  }
  return (hours*60 + minutes); /* return converted time in minutes */ 
}



/*
 * AddPinDigit
 * Description:      Add a digit to the pin number sequence
 *
 * Arguments:        nextstate- expected next state
 *                   event    - entered digit
 * Return:           nextstate- actual next state
 *
 * Input:            None.
 * Output:           None
 *
 * Operation:        Add pin digit and stay within max number of pin digits. 
 *
 * Error Handling:   None. 
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Assumptions:      None
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 *   Apr. 24, 2013      Nnoduka Eruchalu     Modified to use AddDigit()
 */
state AddPinDigit(state nextstate, eventcode event)
{
  /* add pin digit up to number of allowed digits */
  AddDigit(event, NUM_PIN_DIGITS); 
  
  return nextstate;
}


/*
 * ProcessPin
 * Description:      Process the entered Pin Number.
 *                   
 * Arguments:        nextstate- expected next state
 *                   event    - current event
 * Return:           nextstate- next state assuming unverified pin.
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        If the pin number is complete, verify the pin number. A
 *                   verified pin number means the FSM can move on to "Home"
 *                   
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
state ProcessPin(state nextstate, eventcode event)
{
  state result = nextstate;
  if (num_digits == NUM_PIN_DIGITS) { /* if pin number is complete */
    
    /* let user know the pin is being verified */
    UpdateDisplay(1, 0, "                    "); /* row 1, col 0   */
    UpdateDisplay(2, 0, "   Verifying Pin    "); /* row 2, col 0   */
    UpdateDisplay(3, 0, "                    "); /* row 3, col 0   */
    
    /*TODO: Use DataPinValidate(uid_easycard, number) */
    if (TRUE) { /* and if is verified, the   */
      __delay_s(0.5);   /* delay to mimic verfication. TODO: remove */
      result = STATE_HOME;         /* nextstate is the homepage */
      /* flash success message */
      UpdateDisplay(2, 0, "   Sucessful Pin!   "); /* row 2, col 0 */
          
    } else {                          /* if pin number is invalid  */
       result = STATE_WELCOME;        /* go back to welcome page   */
      /* flash error message */
      UpdateDisplay(2, 0, "    Invalid  Pin!   "); /* row 2, col 0 */
    }
    
    __delay_s(MSG_FLASH_TIME);        /* a msg flash takes time    */
    ClearNumber();                    /* clear pin since it was complete */
  } 
  
  return result;
}


/*
 * ExitOrUndoDigit
 * Description:      Exit to welcome page or undo last digit entry
 *                   
 * Arguments:        nextstate- expected next state
 *                   event    - current event
 * Return:           nextstate- next state assuming the action will be "Undo".
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        If no entered digits, this function is an exit function.
 *                   If there are digits entered, this function will undo it.
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
state ExitOrUndoDigit(state nextstate, eventcode event)
{
  state result = nextstate;
  if (num_digits == 0) {        /* if no digits                */
    result = STATE_WELCOME;  /* exit to welcome page        */
    
  } else {                      /* if however there are digits */
    number /= 10;               /* remove last appended digit  */
    num_digits--;               /* and account for this in num_digits */
    updated_number = TRUE;
  }
  
  return result;
}


/*
 * GetAcctBalance
 * Description:      Get current user's account balance
 *                   
 * Arguments:        nextstate- expected next state
 *                   event    - current event
 * Return:           nextstate- actual next state 
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        Clear balance-releated variables, then get actual account
 *                   balance.
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
state GetAcctBalance(state nextstate, eventcode event)
{
  /* TODO: ClearBalance();  DataAcctBalance(uid_easycard); */
  updated_balance = TRUE;
  
  return nextstate;
}


/*
 * ProcessAcctRecharge
 * Description:      Process loading of top-up units unto the EasyCard.
 *                   
 * Arguments:        nextstate- expected next state
 *                   event    - current event
 * Return:           nextstate- actual next state. Goes to same place regardless
 *                              of success/fail.
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        get topup card details
 *                   validate topup card, and flash success/fail message.
 *                   Refresh the account balance data, by calling GetAcctBalance
 *                   
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
state ProcessAcctRecharge(state nextstate, eventcode event)
{
  uint32_t recharge_value = 0; /* in kobo */
  /* TODO: copy topup card uid */
  /*
  size_t i;
  mifare_tag *tag;     
  
  tag = GetCardTag();
  for(i=0; i<7; i++) {
    uid_easytopup[i] = tag->uid[i];
    }*/
  
  /* let user know the EasyTopUp card value is being validated */
  UpdateDisplay(1, 0, "                    "); /* row 1, col 0   */
  UpdateDisplay(2, 0, "Checking Top-Up Card"); /* row 2, col 0   */
  UpdateDisplay(3, 0, "                    "); /* row 3, col 0   */
  
  /* TODO: use DataAcctRecharge(uid_easycard, uid_easytopup, &recharge_value) */
  if (TRUE) {
    __delay_s(0.5);  /* mimic "check". TODO: remove this crap */
    recharge_value = 10000;
    /* if EasyTopUp card is valid, flash success message */
    UpdateDisplay(2, 0, " Successful Top-Up! "); /* row 2, col 0 */
    UpdateDisplay(3, 0, " +                  "); /* row 3, col 0   */
    DisplayMoney(3, 2, recharge_value);         /* show amount of topup   */
    
  } else {
    /* if card has already been used, flash error message */
    UpdateDisplay(2, 0, "   Failed Top-Up!   "); /* row 2, col 0 */
  }
  
  __delay_s(MSG_FLASH_TIME);        /* a msg flash takes time    */
  
  balance += recharge_value;              /* update balance      */
  return GetAcctBalance(nextstate, event);/* refresh balance data*/
}


/*
 * GetParkStatus
 * Description:      Get user's parking payment status
 *
 * Arguments:        nextstate- expected next state
 *                   event    - current system event
 * Return:           nextstate- actual next state
 *
 * Input:            None.
 * Output:           None
 *
 * Operation:        Clear parking-related variables, then get actual parking
 *                   status
 *
 * Error Handling:   None. 
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Assumptions:      None
 *
 * Revision History:
 *   Apr. 24, 2013      Nnoduka Eruchalu     Initial Revision
 */
state GetParkStatus(state nextstate, eventcode event)
{
  ClearNumber();        /* clear space/time entry variables */
  ClearParking();       /* clear parking related variables */
  
  /* get parking space and time */
  /* TODO: use DataParkDetails(uid_easycard, &parking_space, &parking_time); */
  parking_space = 12; parking_time = 79*60;
  
  parking_time *= TIME_SCALE; /* scale time appropriately */
  updated_space = TRUE; 
  
  return nextstate;
}


/*
 * AddParkSpaceDigit
 * Description:      Add a digit to the parking space number sequence
 *
 * Arguments:        nextstate- expected next state
 *                   event    - entered digit
 * Return:           nextstate- actual next state
 *
 * Input:            None.
 * Output:           None
 *
 * Operation:        Add parking space digit. Stay within max number of digits
 *
 * Error Handling:   None. 
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Assumptions:      None
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 *   Apr. 24, 2013      Nnoduka Eruchalu     Modified to use AddDigit()
 */
state AddParkSpaceDigit(state nextstate, eventcode event)
{
  /* add parking space digit up to number of allowed digits */
  AddDigit(event, NUM_PARK_SPACE_DIGITS); 
    
  return nextstate;
}


/*
 * ProcessParkSpace
 * Description:      Process the entered Parking Space Number.
 *                   
 * Arguments:        nextstate- expected next state
 *                   event    - current event
 * Return:           nextstate- next state assuming invalid parking space number
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        If there is a valid parking space or entered sequence
 *                   then can move on to enter time
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
state ProcessParkSpace(state nextstate, eventcode event)
{
  state result = nextstate;
  /* only process if a valid parking space     */
  /* TODO: ValidParkSpace(parking_space) instead of parking_space > 0 */
  if ((parking_space>0) || num_digits >= 1) {
    result = STATE_PARKINGTIME; /* move on to entering time     */
    
    if (num_digits >= 1)           /* update space number if necessary */
      parking_space = number;
    
    ClearNumber();                 /* and clear number memory holder  */
  }
  
  return result;
}


/* AddParkTimeDigit
 * Description:      Add a digit to the parking time number sequence with format
 *                   hh:mm
 *
 * Arguments:        nextstate- expected next state
 *                   event    - entered digit
 * Return:           nextstate- actual next state
 *
 * Input:            None.
 * Output:           None
 *
 * Operation:        Add parking time digit with format hh:mm
 *
 * Error Handling:   None. 
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Assumptions:      None
 *
 * Revision History:
 *   Apr. 24, 2013      Nnoduka Eruchalu     Initial Revision
 */
state AddParkTimeDigit(state nextstate, eventcode event)
{
  uint8_t add_digit = FALSE;             /* first assume invalid digit */
  
  switch(num_digits) {                   /* each digit has special bounds */
  case 0:                                /* on first digit (h*:**) */
    if (event < 2) add_digit = TRUE;     /* 0 & 1 are only valid digits   */
    break;
    
  case 1:                                /*on 2nd digit (*h:**) if 1st digit*/
    /* is 1 the valid digits are 0,1,2. But if 0, then all digits are valid */
    if ((number==0) || ((number == 1) && (event < 3))) add_digit = TRUE;
    break;
    
  case 2:                                /* on 3rd digit (**:m*) */
    if (event < 6) add_digit = TRUE;     /* valid digits have to be < 6 */
    break;
    
  case 3:                                /* on 4rd digit (**:*m) */
    add_digit = TRUE;                    /* all digits are valid */
    break;
    
  default:                               /* too many digits! do nothing */
    break;
  }
  
  if(add_digit) {                        /* if new digit is valid       */
    number = number*10 + event;          /* add it in and update        */
    num_digits += 1;                     /* number of digits thus far   */
    updated_number = TRUE;               /* captured a new digit */
  }
  
  return nextstate;
}


/*
 * ProcessParkTime
 * Description:      Process the entered Parking Time. Convert it from hh:mm
 *                   to seconds
 *                   
 * Arguments:        nextstate- expected next state
 *                   event    - current event
 * Return:           nextstate- next state assuming invalid parking time number
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        If there is at least 1 non-zero entered parking time digit
 *                   then can process parking space and time.
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
state ProcessParkTime(state nextstate, eventcode event)
{
  state result = nextstate;
  uint32_t parking_muls; /* how many multiples of 30 minutes is parking_time? */
  uint32_t parking_time_min;
  if ((num_digits >= 1) && (number != 0)) { /* if parking time is valid */
    /* save parking time in seconds */
    parking_time = ConvertTimeToMin(number, num_digits)*60;
    
    /* TODO: use DataParkPay(uid_easycard, parking_space, &parking_time);
     * pay for space and server will figure out if this is a new payment or 
     * an extension of time. If it is a time extension, the time will
     * be updated with an extended value.
     */
    
    /* update balance at N10.00 for 30 minutes by using modulo */
    parking_time_min = parking_time/60;
    if (parking_time_min % 30)                      /* if not exact multiple of 30*/
      parking_muls = (parking_time_min/30) + 1;     /* then use ceiling function */
    else if (parking_time_min > 0)
      parking_muls = parking_time_min/30;
    balance -= parking_muls*1000;
    DataAlertPark(parking_space, parking_time_min);
    
    parking_time *= TIME_SCALE; /* scale time appropriately */
    
      
    /* go back to parking page to see new parking summary */
    result = STATE_PARKING;
    updated_space = TRUE;
    ElapsedTime();  /* reset elapsed time */
    
    ClearNumber();                 /* and clear its memory holder  */
    
}
  return result;
}



/*
 * GetUtilityData
 * Description:      Get Utility Meter numbers
 *                   
 * Arguments:        nextstate- expected next state
 *                   event    - current event
 * Return:           nextstate- actual next state
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        TODO: Pull these numbers of MIFARE tag
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   May 25, 2013      Nnoduka Eruchalu     Initial Revision
 */
state GetUtilityData(state nextstate, eventcode event)
{
  ClearNumber();
  return nextstate;
}


/*
 * MobileGetMtn****
 * Description:      Get MTN VTU Recharge of 100, 200, 400, 750, 1500
 *                   
 * Arguments:        nextstate- expected next state
 *                   event    - current event
 * Return:           nextstate- expected next state
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        Call MobileGet() with the exact recharge amount, and
 *                   return expected next state
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 25, 2013      Nnoduka Eruchalu     Initial Revision
 */
state MobileGetMtn100(state nextstate, eventcode event)
{
  MobileGet(100);
  return nextstate;
}
state MobileGetMtn200(state nextstate, eventcode event)
{
  MobileGet(200);
  return nextstate;
}
state MobileGetMtn400(state nextstate, eventcode event)
{
  MobileGet(400);
  return nextstate;
}
state MobileGetMtn750(state nextstate, eventcode event)
{
  MobileGet(750);
  return nextstate;
}
state MobileGetMtn1500(state nextstate, eventcode event)
{
  MobileGet(1500);
  return nextstate;
}


/*
 * MobileGetGlo****
 * Description:      Get Glo QuickCharge of 100, 200, 500, 1000, 3000
 *                   
 * Arguments:        nextstate- expected next state
 *                   event    - current event
 * Return:           nextstate- expected next state
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        Call MobileGet() with the exact recharge amount, and
 *                   return expected next state
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 25, 2013      Nnoduka Eruchalu     Initial Revision
 */
state MobileGetGlo100(state nextstate, eventcode event)
{
  MobileGet(100);
  return nextstate;
}
state MobileGetGlo200(state nextstate, eventcode event)
{
  MobileGet(200);
  return nextstate;
}
state MobileGetGlo500(state nextstate, eventcode event)
{
  MobileGet(500);
  return nextstate;
}
state MobileGetGlo1000(state nextstate, eventcode event)
{
  MobileGet(1000);
  return nextstate;
}
state MobileGetGlo3000(state nextstate, eventcode event)
{
  MobileGet(3000);
  return nextstate;
}


/*
 * MobileGetAirtel****
 * Description:      Get Airtel Top-up of 100, 200, 500, 1000
 *                   
 * Arguments:        nextstate- expected next state
 *                   event    - current event
 * Return:           nextstate- expected next state
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        Call MobileGet() with the exact recharge amount, and
 *                   return expected next state
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 25, 2013      Nnoduka Eruchalu     Initial Revision
 */
state MobileGetAirtel100(state nextstate, eventcode event)
{
  MobileGet(100);
  return nextstate;
}
state MobileGetAirtel200(state nextstate, eventcode event)
{
  MobileGet(200);
  return nextstate;
}
state MobileGetAirtel500(state nextstate, eventcode event)
{
  MobileGet(500);
  return nextstate;
}
state MobileGetAirtel1000(state nextstate, eventcode event)
{
  MobileGet(1000);
  return nextstate;
}


/*
 * MobileGetEtisalat****
 * Description:      Get Etisalat Top-up of 100, 200, 500, 1000
 *                   
 * Arguments:        nextstate- expected next state
 *                   event    - current event
 * Return:           nextstate- expected next state
 *
 * Input:            None
 * Output:           None
 *
 * Operation:        Call MobileGet() with the exact recharge amount, and
 *                   return expected next state
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 25, 2013      Nnoduka Eruchalu     Initial Revision
 */
state MobileGetEtisalat100(state nextstate, eventcode event)
{
  MobileGet(100);
  return nextstate;
}
state MobileGetEtisalat200(state nextstate, eventcode event)
{
  MobileGet(200);
  return nextstate;
}
state MobileGetEtisalat500(state nextstate, eventcode event)
{
  MobileGet(500);
  return nextstate;
}
state MobileGetEtisalat1000(state nextstate, eventcode event)
{
  MobileGet(1000);
  return nextstate;
}
