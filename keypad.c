/*
 * -----------------------------------------------------------------------------
 * -----                             KEYPAD.C                              -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is a library of functions for interfacing the PIC18F67K22 with a 4x4
 *   matrix keypad
 *
 * Table of Contents:
 *   KeypadInit      - initializes the keypad and ScanAndDebounce variables
 *   IsAKey          - checks if a fully debounced key is available
 *   GetKey          - gets a key from the keypad
 *   ScanAndDebounce - scan the keypad for keypresses and debounce the presses
 *
 * Assumptions:
 *   Hardware Hookup defined in include file.
 *
 * Limitations:
 *   - Simultaneous multiple key presses are invalid, but the program still
 *     handles them and returns key codes for them.
 *   - Because of hardware limitations there will be ghosting and masking errors
 *   - If a key has been debounced and not processed, and another key is
 *     debounced, the first keypress will be lost.
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Dec. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
#include "general.h"
#include "keypad.h"

/* shared variables have to be local to this file */
static unsigned char keyDebounced;
static volatile unsigned char keyValue;

/*
 * KeypadInit
 * Description: This procedure initializes the shared variables.
 *
 * Arguments:   None
 * Return:      None
 *
 * Input:       None
 * Output:      None
 *
 * Operation:   Put the keypad in a state of no debounced keys, by setting 
 *              keyDebounced to FALSE.
 *              Also the keyValue is set to an invalid key code.
 *              Setup keypad I/O.
 *              Start the ScanAndDebounce Timer
 *
 * Revision History:
 *   Dec. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
void KeypadInit(void)
{
  keyDebounced = FALSE;          /* no debounced key press */
  keyValue = NO_KEY_PRESSED;     /* start with an invalid key press */
  KEY_TRIS = KEY_TRIS_VAL;       /* setup Rows as outputs; Cols as Inputs */
  NO_ROW();                      /* row outputs start off inactive */
}


/*
 * IsAKey
 * Description: This procedure checks if a fully debounced key is available and 
 *              returns with a TRUE or FALSE
 *
 * Arguments:   None 
 * Return:      TRUE : A fully debounced key is available
 *              FALSE: A fully debounced key is not available.
 *
 * Input:       None
 * Output:      None
 
 * Operation:   This function returns the value in keyDebounced
 *
 * Revision History:
 *  Dec. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
unsigned char IsAKey(void)
{
  return keyDebounced;
}


/*
 * GetKey
 * Description: This returns the code for fully debounced key presses on the 
 *              keypad.
 *              It is a blocking function.
 *              Note that this procedure returns with the key code after the key
 *              has been pressed and debounced but not necessarily released.
 *
 * Arguments:   None
 * Return:
 *   Fully debounced key code.
 *   Below is a depiction of the keypad showing its keycode map.
 * 
 *            column0       column1       column2       column3
 *   row0     KEY_1         KEY_2         KEY_3         KEY_A
 *   row1     KEY_4         KEY_5         KEY_6         KEY_B
 *   row2     KKE_7         KEY_8         KEY_9         KEY_C
 *   row3     KEY_S         KEY_0         KEY_P         KEY_D
 *
 *
 * Input:       None
 * Output:      None
 *
 * Operation:   A while loop runs until IsAKey indicates that there is now a 
 *              fully debounced key. When there is a key, the keyDebounced 
 *              variable is reset and the key code is returned.
 *
 *
 * Revision History:
 *  Dec. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
unsigned char GetKey(void)
{
  while(!IsAKey())             /* block until there is a key */
    continue;
  
  keyDebounced = FALSE;        /* reset keypad to a state of no key */
  return keyValue;             /* return key code */
}


/*
 * ScanAndDebounce
 * Description: This scans the keypad and if a key is pressed, the key is 
 *              debounced for KEY_DEBOUNCE_TIME ms.
 *              It also handles the keypad auto repeat with a repeat rate of
 *              KEY_REPEAT_TIME ms.
 *              This procedure is called by a timer ISR.
 *
 * Arguments:   None 
 * Return:      None
 *
 * Input:       Keypad (4x4 matrix keypad)
 * Output:      None
 *
 * Operation:   The function goes through each row of the keypad checking for 
 *               pressed keys.
 *              If no key has been pressed the debounce counter is reset to
 *               KEY_DEBOUNCE_TIME, the lastPortVal is set to curPortVal and the
 *               scanner goes on to the next keypad row. However if a key has 
 *               been pressed, the scanner stays on that row and does some 
 *               further checks.
 *              If the same key is being pressed, then the debouncer starts/
 *               continues debouncing the key.
 *               When a key is fully debounced, keyDebounced is set to TRUE and 
 *               the debounce counter is reloaded with KEY_REPEAT_TIME (to 
 *               enable auto repeat).
 *              The debounced key code is stored in keyValue.
 *              If a different key is being pressed when this function is called
 *               by the ISR, the debounce counter is reloaded with 
 *               (KEY_DEBOUNCE_TIME - 1). The -1 makes up for 1 lost count 
 *               during this check.
 *
 * Limitations:  - Simultaneous multiple key presses are invalid, but the 
 *                 procedure still handles them and returns key codes. 
 *               - Ghosting/Aliasing
 *               - If a key has been debounced and not processed, and another
 *                 key is debounced, the first keypress will be lost.
 *
 * Revision History:
 *   Dec. 18, 2012      Nnoduka Eruchalu     Initial Revision
 */
void ScanAndDebounce(void)
{
  /* remember these are static variables: so only initialized once */
  static unsigned char keyRow = 0; /* 0-indexed row number on the keypad */
  static unsigned int debounceCntr = KEY_DEBOUNCE_TIME;
  static volatile unsigned char lastPortVal;
  static volatile unsigned char curPortVal;
  
  SET_ROW(keyRow);                    /* set the row to read from and */
  curPortVal = KEY_PORT;              /* read in the keypad values*/
  
  /* check if a key is being pressed */
  if ((curPortVal & KEY_COLS_MASK) == NO_KEY_PRESSED) { /* no key pressed */
    lastPortVal = curPortVal;         /* save current port value */
    debounceCntr = KEY_DEBOUNCE_TIME; /* reset debounce counter */
    keyRow++; keyRow %= KEY_NUM_ROWS; /* move on to next row, don't overshoot */
    
  } else {                            /* key pressed; do more checks */
    if (lastPortVal == curPortVal) {  /* check 1: is this the same key press? */
      if (--debounceCntr <= 0) {      /* done debouncing */
        keyDebounced = TRUE;          /* a fully debounced key now available */
        debounceCntr=KEY_REPEAT_TIME; /* enable key auto repeat */
        keyValue = curPortVal;
      }
      
    } else {                          /* check 2: or a different key press? */
      lastPortVal = curPortVal;       /* save this port value, and reset cntr */
      debounceCntr = KEY_DEBOUNCE_TIME - 1; /* accounting for a missed cnt */
    }                                 /* end check for same key press */
    
  }                                   /* end check for key press */
}
