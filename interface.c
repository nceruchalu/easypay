/*
 * -----------------------------------------------------------------------------
 * -----                           INTERFACE.C                             -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *  This is a library of functions and state machine tables for a user interface
 *
 * Table of Contents 
 *  (Functions):
 *   KeyLookup           - translate a key value from keypad to a keycode
 *   CardLookup          - translate smartcard comm. status to an eventcode
 *   StateDriver         - loop driving the finite state machine
 *
 * (Tables):
 *   WelcomeTable        - Welcome Screen Display Strings
 *   PinTable            - Enter Pin Number Screen
 *   HomeTable           - Homepage Screen
 *   AccountTable        - Account Summary Screen
 *   AccountRechargeTable- Account Recharge Screen
 *   ParkingTable        - Current Parking Summary Screen
 *   ParkingSpaceTable   - Enter Parking Space Screen
 *   ParkingTimeTable    - Enter Parking Time Screen
 *   MobileTable         - Select Mobile Recharge Vendor
 *   MobileMtnTable      - Select MTN VTU Recharge Value
 *   MobileGloTable      - Select Glo QuickCharge Value
 *   MobileAirtelTable   - Select Airtel Top-up Value
 *   MobileEtisalatTable - Select Etisalat E-Top up Value
 *   UtilityTable        - Pay Utility Bill
 *   UtilityPowerTable   - Pay Power Bill
 *   UtilityWaterTable   - Pay Water Bill
 *
 *   DisplayTables       - Table of Screens [Above Tables]
 *   UpdateTable         - Table of Update functions for each state
 *   StateTable          - FSM Transition Table
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
 *   Apr. 19, 2013      Nnoduka Eruchalu     Initial Revision
 */

#include <stdint.h>     /* for uint*_t */
#include <stdlib.h>     /* for size_t */
#include <htc.h>
#include "general.h"
#include "keypad.h"
#include "smartcard.h"
#include "interface.h"
#include "eventproc.h"
#include "lcd.h"

/* variables local to this file */
/* none */

/* functions local to this file */
static eventcode KeyLookup(void);
static eventcode CardLookup(void);



/* GENERAL NAVIGATION
 ----------------------- */
/*
 * WelcomeTable
 * Description: This table holds the message on the LCD on the Welcome Page.
 *              Note that each row has 1 extra slot for NULL-terminator.
 *
 * Revision History:
 *   Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
static const char WelcomeTable[LCD_HEIGHT][LCD_WIDTH+1] = {
  "-Welcome to EasyPay-", /* row 0 */
  "                    ", /* row 1 */
  " Tap Card to Start  ", /* row 2 */
  "                    "  /* row 3 */
};

/*
 * PinTable
 * Description: This table holds the message on the LCD on the "Enter Pin"
 *              Page.
 *              Note that each row has 1 extra slot for NULL-terminator.
 *
 * Revision History:
 *   Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
static const char PinTable[LCD_HEIGHT][LCD_WIDTH+1] = {
  "-    Enter Pin     -", /* row 0 */
  "                    ", /* row 1 */
  "*Done*           ~ C", /* row 2 */
  "*Exit*           ~ D"  /* row 3 */
};

/*
 * HomeTable
 * Description: This table holds the message on the LCD on the Home Page
 *              Note that each row has 1 extra slot for NULL-terminator.
 *
 * Revision History:
 *   Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
static const char HomeTable[LCD_HEIGHT][LCD_WIDTH+1] = {
  "-     EasyPay      -", /* row 0 */
  "4~Account  B~Parking", /* row 1 */
  "7~Mobile   C~Utility", /* row 2 */
  "           D~*Quit* "  /* row 3 */
};


/* ACCOUNT 
 --------------------*/
/*
 * AccountTable
 * Description: This table holds the message on the LCD on the Account Page
 *              Note that each row has 1 extra slot for NULL-terminator.
 *
 * Revision History:
 *   Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
static const char AccountTable[LCD_HEIGHT][LCD_WIDTH+1] = {
  "-   EasyAccount    -", /* row 0 */
  "Balance:            ", /* row 1 */
  "Recharge         ~ C", /* row 2 */
  "*Exit*           ~ D"  /* row 3 */
};

/*
 * AccountRechargeTable
 * Description: This table holds the message on the LCD on the Account->Recharge
 *              Page.
 *              Note that each row has 1 extra slot for NULL-terminator.
 *
 * Revision History:
 *   Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
static const char AccountRechargeTable[LCD_HEIGHT][LCD_WIDTH+1] = {
  "-  Top-Up EasyCard -", /* row 0 */
  " Tap Recharge Card  ", /* row 1 */
  "                    ", /* row 2 */
  "*Exit*           ~ D"  /* row 3 */
};


/* PARKING
 --------------------*/
/*
 * ParkingTable
 * Description: This table holds the message on the LCD on the Parking Page.
 *              Note that each row has 1 extra slot for NULL-terminator.
 *
 * Revision History:
 *   Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 *   Apr. 24, 2013      Nnoduka Eruchalu     Separated this from ParkingSpace
 */
static const char ParkingTable[LCD_HEIGHT][LCD_WIDTH+1] = {
  "-Space #:          -", /* row 0 */
  " Time Left:         ", /* row 1 */
  "New / Extend     ~ C", /* row 2 */
  "*Exit*           ~ D"  /* row 3 */
};

/*
 * ParkingSpaceTable
 * Description: This table holds the message on the LCD when entering a parking
 *              space number
 *              Note that each row has 1 extra slot for NULL-terminator.
 *
 * Revision History:
 *  Apr. 24, 2013      Nnoduka Eruchalu     Initial Revision
 */
static const char ParkingSpaceTable[LCD_HEIGHT][LCD_WIDTH+1] = {
  "Park: Enter Space   ", /* row 0 */
  "                    ", /* row 1 */
  "*Done*           ~ C", /* row 2 */
  "*Exit*           ~ D"  /* row 3 */
};

/*
 * ParkingTimeTable
 * Description: This table holds the message on the LCD when entering time on
 *              the Parking Page.
 *              Note that each row has 1 extra slot for NULL-terminator.
 *
 * Revision History:
 *   Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
static const char ParkingTimeTable[LCD_HEIGHT][LCD_WIDTH+1] = {
  "Park Time: hh:mm    ", /* row 0 */
  "                    ", /* row 1 */
  "*Done*           ~ C", /* row 2 */
  "*Exit*           ~ D"  /* row 3 */
};


/* MOBILE (CELLPHONE) RECHARGE
 ------------------------------*/
/*
 * MobileTable
 * Description: This table holds the message on the LCD when selecting a mobile
 *              provider to buy recharge units.
 *              Note that each row has 1 extra slot for NULL-terminator.
 *
 * Revision History:
 *   Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
static const char MobileTable[LCD_HEIGHT][LCD_WIDTH+1] = {
  "- Mobile Recharge  -", /* row 0 */
  "4~MTN     B~  Airtel", /* row 1 */
  "7~GLO     C~Etisalat", /* row 2 */
  "          D~  *Exit*"  /* row 3 */
};

/*
 * MobileMtnTable
 * Description: This table holds the message on the LCD when viewing available
 *              MTN recharge cards
 *              See here: http://www.mtnonline.com/products-services/airtime/vtu
 *              Note that each row has 1 extra slot for NULL-terminator.
 *
 * Revision History:
 *   Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
static const char MobileMtnTable[LCD_HEIGHT][LCD_WIDTH+1] = {
  "-     MTN VTU      -", /* row 0 */
  "4~N100      B~  N750", /* row 1 */
  "7~N200      C~ N1500", /* row 2 */
  "*~N400      D~*Exit*"  /* row 3 */
};

/*
 * MobileGloTable
 * Description: This table holds the message on the LCD when viewing available
 *              Glo recharge cards.
 *              See here: https://www.quickteller.com/gloquickcharge
 *              Note that each row has 1 extra slot for NULL-terminator.
 *
 * Revision History:
 *   Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
static const char MobileGloTable[LCD_HEIGHT][LCD_WIDTH+1] = {
  "-  Glo QuickCharge -", /* row 0 */
  "4~N100      B~ N1000", /* row 1 */
  "7~N200      C~ N3000", /* row 2 */
  "*~N500      D~*Exit*"  /* row 3 */
};

/*
 * MobileAirtelTable
 * Description: This table holds the message on the LCD when viewing available
 *              Airtel recharge cards.
 *              See here: https://www.quickteller.com/airtel
 *              Note that each row has 1 extra slot for NULL-terminator.
 *
 * Revision History:
 *   Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
static const char MobileAirtelTable[LCD_HEIGHT][LCD_WIDTH+1] = {
  "-   Airtel Top-up  -", /* row 0 */
  "4~N100      B~  N500", /* row 1 */
  "7~N200      C~ N1000", /* row 2 */
  "            D~*Exit*"  /* row 3 */
};

/*
 * MobileEtisalatTable
 * Description: This table holds the message on the LCD when viewing available
 *              Etisalat recharge cards
 *              See here: http://www.etisalat.com.ng/etopup/etopup.php
 *              Note that each row has 1 extra slot for NULL-terminator.
 *
 * Revision History:
 *   Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
static const char MobileEtisalatTable[LCD_HEIGHT][LCD_WIDTH+1] = {
  "- Etisalat E-Top up-", /* row 0 */
  "4~N100      B~  N500", /* row 1 */
  "7~N200      C~ N1000", /* row 2 */
  "            D~*Exit*"  /* row 3 */
};


/* PAY UTILIY BILLS
 ------------------------------*/
/*
 * UtilityTable
 * Description: This table holds the message on the LCD when selecting a utility
 *              bill to pay.
 *              Note that each row has 1 extra slot for NULL-terminator.
 *
 * Revision History:
 *  Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
static const char UtilityTable[LCD_HEIGHT][LCD_WIDTH+1] = {
  "-  Utility Bills   -", /* row 0 */
  "Power            ~ B", /* row 1 */
  "Water            ~ C", /* row 2 */
  "*Exit*           ~ D"  /* row 3 */
};

/*
 * UtilityPowerTable
 * Description: This table holds the message on the LCD when paying a power bill
 *              Note that each row has 1 extra slot for NULL-terminator.
 *
 * Revision History:
 *   Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
static const char UtilityPowerTable[LCD_HEIGHT][LCD_WIDTH+1] = {
  "-Power: Enter Amount", /* row 0 */
  "                    ", /* row 1 */
  "*Done*           ~ C", /* row 2 */
  "*Exit*           ~ D"  /* row 3 */
};

/*
 * UtilityWaterTable
 * Description: This table holds the message on the LCD when paying a water bill
 *              Note that each row has 1 extra slot for NULL-terminator.
 *
 * Revision History:
 *   Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
static const char UtilityWaterTable[LCD_HEIGHT][LCD_WIDTH+1] = {
  "-Water: Enter Amount", /* row 0 */
  "                    ", /* row 1 */
  "*Done*           ~ C", /* row 2 */
  "*Exit*           ~ D"  /* row 3 */
};


/*
 * DisplayTables
 * Description: This table holds a pointer to the display table for each system
 *              state.
 *
 * Usage:       To get the string for 3rd row of display on welcome page: 
 *                  DisplayTable[STATE_WELCOME][2]
 *              
 *              To get the entire table of display strings on welcome page
 *                  DisplayTable[STATE_WELCOME]
 *              To pass this as an argument to a function, its prototype should
 *              be of this form:
 *                  void func(const char (*table)[LCD_WIDTH+1]);
 *
 * Revision History:
 *   Apr. 21, 2013      Nnoduka Eruchalu     Initial Revision
 */
static const char (* DisplayTables[NUM_STATES])[LCD_WIDTH+1] =
{
  WelcomeTable,          /* STATE_WELCOME         */
  PinTable,              /* STATE_PIN             */
  HomeTable,             /* STATE_HOME            */
  
  AccountTable,          /* STATE_ACCOUNT         */
  AccountRechargeTable,  /* STATE_ACCOUNTRECHARGE */
  
  ParkingTable,          /* STATE_PARKING         */
  ParkingSpaceTable,     /* STATE_PARKINGSPACE    */
  ParkingTimeTable,      /* STATE_PARKINGTIME     */
  
  MobileTable,           /* STATE_MOBILE          */
  MobileMtnTable,        /* STATE_MOBILEMTN       */
  MobileGloTable,        /* STATE_MOBILEGLO       */
  MobileAirtelTable,     /* STATE_MOBILEAIRTEL    */
  MobileEtisalatTable,   /* STATE_MOBILEETISALAT  */
  
  UtilityTable,          /* STATE_UTILITY         */
  UtilityPowerTable,     /* STATE_UTILITYPOWER    */
  UtilityWaterTable      /* STATE_UTILITYWATER    */
};


/*
 * UpdateTable
 * Description:  This is the table of update functions. One for each system
 *               state.
 *
 * Revision History:
 *   Apr. 22, 2013      Nnoduka Eruchalu     Initial Revision
 */
static state (* const UpdateTable[NUM_STATES])(state curr_state) = {
  NoUpdate,              /* STATE_WELCOME         */
  UpdatePin,             /* STATE_PIN             */
  NoUpdate,              /* STATE_HOME            */
  
  UpdateAccount,         /* STATE_ACCOUNT         */
  NoUpdate,              /* STATE_ACCOUNTRECHARGE */
  
  UpdatePark,            /* STATE_PARKING         */
  UpdateParkSpace,       /* STATE_PARKINGSPACE    */
  UpdateParkTime,        /* STATE_PARKINGTIME     */
  
  NoUpdate,              /* STATE_MOBILE          */
  NoUpdate,              /* STATE_MOBILEMTN       */
  NoUpdate,              /* STATE_MOBILEGLO       */
  NoUpdate,              /* STATE_MOBILEAIRTEL    */
  NoUpdate,              /* STATE_MOBILEETISALAT  */
  
  NoUpdate,              /* STATE_UTILITY         */
  NoUpdate,              /* STATE_UTILITYPOWER    */
  NoUpdate               /* STATE_UTILITYWATER    */
};


/*
 * StateTable
 * Description:  This is the State Transition Table for the Finite State Machine
 *               Each entry consists of the next state and action for that
 *               transition. The rows are associated with the current state and
 *               the columns with the input key.         
 *
 * Revision History:
 *   Apr. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
static const state_transition StateTable[NUM_STATES][NUM_EVENTS] = {
  /* Current State = STATE_WELCOME */
  { {STATE_WELCOME, NoAction},             /* <0> */
    {STATE_WELCOME, NoAction},             /* <1> */
    {STATE_WELCOME, NoAction},             /* <2> */
    {STATE_WELCOME, NoAction},             /* <3> */
    {STATE_WELCOME, NoAction},             /* <4> */
    {STATE_WELCOME, NoAction},             /* <5> */
    {STATE_WELCOME, NoAction},             /* <6> */
    {STATE_WELCOME, NoAction},             /* <7> */
    {STATE_WELCOME, NoAction},             /* <8> */
    {STATE_WELCOME, NoAction},             /* <9> */
    {STATE_WELCOME, NoAction},             /* <A> */
    {STATE_WELCOME, NoAction},             /* <B> */
    {STATE_WELCOME, NoAction},             /* <C> */
    {STATE_WELCOME, NoAction},             /* <D> */
    {STATE_WELCOME, NoAction},             /* <*> */
    {STATE_WELCOME, NoAction},             /* <#> */
    {STATE_WELCOME, NoAction},             /* other keypad keys  */
    {STATE_PIN, ResetAction},              /* card tapped/synced */
    {STATE_WELCOME, NoAction},             /* topup card tapped  */
    {STATE_WELCOME, NoAction}              /* other card tapped  */
  },
  
  /* Current State = STATE_PIN */
  { {STATE_PIN, AddPinDigit},             /* <0> */
    {STATE_PIN, AddPinDigit},             /* <1> */
    {STATE_PIN, AddPinDigit},             /* <2> */
    {STATE_PIN, AddPinDigit},             /* <3> */
    {STATE_PIN, AddPinDigit},             /* <4> */
    {STATE_PIN, AddPinDigit},             /* <5> */
    {STATE_PIN, AddPinDigit},             /* <6> */
    {STATE_PIN, AddPinDigit},             /* <7> */
    {STATE_PIN, AddPinDigit},             /* <8> */
    {STATE_PIN, AddPinDigit},             /* <9> */
    {STATE_PIN, NoAction},                /* <A> */
    {STATE_PIN, NoAction},                /* <B> */
    {STATE_PIN, ProcessPin},              /* <C> */
    {STATE_PIN, ExitOrUndoDigit},         /* <D> */
    {STATE_PIN, NoAction},                /* <*> */
    {STATE_PIN, NoAction},                /* <#> */
    {STATE_PIN, NoAction},                /* other keypad keys  */
    {STATE_PIN, NoAction},                /* card tapped/synced */
    {STATE_PIN, NoAction},                /* topup card tapped  */
    {STATE_PIN, NoAction}                 /* other card tapped  */
  },
  
  /* Current State = STATE_HOME */
  { {STATE_HOME, NoAction},             /* <0> */
    {STATE_HOME, NoAction},             /* <1> */
    {STATE_HOME, NoAction},             /* <2> */
    {STATE_HOME, NoAction},             /* <3> */
    {STATE_ACCOUNT, GetAcctBalance},    /* <4> */
    {STATE_HOME, NoAction},             /* <5> */
    {STATE_HOME, NoAction},             /* <6> */
    {STATE_MOBILE, NoAction},           /* <7> */
    {STATE_HOME, NoAction},             /* <8> */
    {STATE_HOME, NoAction},             /* <9> */
    {STATE_HOME, NoAction},             /* <A> */
    {STATE_PARKING, GetParkStatus},     /* <B> */
    {STATE_UTILITY, GetUtilityData},    /* <C> */
    {STATE_WELCOME, NoAction},          /* <D> */
    {STATE_HOME, NoAction},             /* <*> */
    {STATE_HOME, NoAction},             /* <#> */
    {STATE_HOME, NoAction},             /* other keypad keys  */
    {STATE_HOME, NoAction},             /* card tapped/synced */
    {STATE_HOME, NoAction},             /* topup card tapped  */
    {STATE_HOME, NoAction}              /* other card tapped  */
  },

  /* Current State = STATE_ACCOUNT */
  { {STATE_ACCOUNT, NoAction},             /* <0> */
    {STATE_ACCOUNT, NoAction},             /* <1> */
    {STATE_ACCOUNT, NoAction},             /* <2> */
    {STATE_ACCOUNT, NoAction},             /* <3> */
    {STATE_ACCOUNT, NoAction},             /* <4> */
    {STATE_ACCOUNT, NoAction},             /* <5> */
    {STATE_ACCOUNT, NoAction},             /* <6> */
    {STATE_ACCOUNT, NoAction},             /* <7> */
    {STATE_ACCOUNT, NoAction},             /* <8> */
    {STATE_ACCOUNT, NoAction},             /* <9> */
    {STATE_ACCOUNT, NoAction},             /* <A> */
    {STATE_ACCOUNT, NoAction},             /* <B> */
    {STATE_ACCOUNTRECHARGE, NoAction},     /* <C> */
    {STATE_HOME, NoAction},                /* <D> */
    {STATE_ACCOUNT, NoAction},             /* <*> */
    {STATE_ACCOUNT, NoAction},             /* <#> */
    {STATE_ACCOUNT, NoAction},             /* other keypad keys  */
    {STATE_ACCOUNT, NoAction},             /* card tapped/synced */
    {STATE_ACCOUNT, NoAction},             /* topup card tapped  */
    {STATE_ACCOUNT, NoAction}              /* other card tapped  */
  },
  
  /* Current State = STATE_ACCOUNTRECHARGE */
  { {STATE_ACCOUNTRECHARGE, NoAction},             /* <0> */
    {STATE_ACCOUNTRECHARGE, NoAction},             /* <1> */
    {STATE_ACCOUNTRECHARGE, NoAction},             /* <2> */
    {STATE_ACCOUNTRECHARGE, NoAction},             /* <3> */
    {STATE_ACCOUNTRECHARGE, NoAction},             /* <4> */
    {STATE_ACCOUNTRECHARGE, NoAction},             /* <5> */
    {STATE_ACCOUNTRECHARGE, NoAction},             /* <6> */
    {STATE_ACCOUNTRECHARGE, NoAction},             /* <7> */
    {STATE_ACCOUNTRECHARGE, NoAction},             /* <8> */
    {STATE_ACCOUNTRECHARGE, NoAction},             /* <9> */
    {STATE_ACCOUNTRECHARGE, NoAction},             /* <A> */
    {STATE_ACCOUNTRECHARGE, NoAction},             /* <B> */
    {STATE_ACCOUNTRECHARGE, NoAction},             /* <C> */
    {STATE_HOME, NoAction},                        /* <D> */
    {STATE_ACCOUNTRECHARGE, NoAction},             /* <*> */
    {STATE_ACCOUNTRECHARGE, NoAction},             /* <#> */
    {STATE_ACCOUNTRECHARGE, NoAction},             /* other keypad keys  */
    {STATE_ACCOUNTRECHARGE, NoAction},             /* card tapped/synced */
    {STATE_ACCOUNT, ProcessAcctRecharge},          /* topup card tapped  */
    {STATE_ACCOUNTRECHARGE, NoAction}              /* other card tapped  */
  },
  
  /* Current State = STATE_PARKING */
  { {STATE_PARKING, NoAction},             /* <0> */
    {STATE_PARKING, NoAction},             /* <1> */
    {STATE_PARKING, NoAction},             /* <2> */
    {STATE_PARKING, NoAction},             /* <3> */
    {STATE_PARKING, NoAction},             /* <4> */
    {STATE_PARKING, NoAction},             /* <5> */
    {STATE_PARKING, NoAction},             /* <6> */
    {STATE_PARKING, NoAction},             /* <7> */
    {STATE_PARKING, NoAction},             /* <8> */
    {STATE_PARKING, NoAction},             /* <9> */
    {STATE_PARKING, NoAction},             /* <A> */
    {STATE_PARKING, NoAction},             /* <B> */
    {STATE_PARKINGSPACE, NoAction},        /* <C> */
    {STATE_HOME, NoAction},                /* <D> */
    {STATE_PARKING, NoAction},             /* <*> */
    {STATE_PARKING, NoAction},             /* <#> */
    {STATE_PARKING, NoAction},             /* other keypad keys  */
    {STATE_PARKING, NoAction},             /* card tapped/synced */
    {STATE_PARKING, NoAction},             /* topup card tapped  */
    {STATE_PARKING, NoAction}              /* other card tapped  */
  },
  
  /* Current State = STATE_PARKINGSPACE */
  { {STATE_PARKINGSPACE, AddParkSpaceDigit},            /* <0> */
    {STATE_PARKINGSPACE, AddParkSpaceDigit},            /* <1> */
    {STATE_PARKINGSPACE, AddParkSpaceDigit},            /* <2> */
    {STATE_PARKINGSPACE, AddParkSpaceDigit},            /* <3> */
    {STATE_PARKINGSPACE, AddParkSpaceDigit},            /* <4> */
    {STATE_PARKINGSPACE, AddParkSpaceDigit},            /* <5> */
    {STATE_PARKINGSPACE, AddParkSpaceDigit},            /* <6> */
    {STATE_PARKINGSPACE, AddParkSpaceDigit},            /* <7> */
    {STATE_PARKINGSPACE, AddParkSpaceDigit},            /* <8> */
    {STATE_PARKINGSPACE, AddParkSpaceDigit},            /* <9> */
    {STATE_PARKINGSPACE, NoAction},                     /* <A> */
    {STATE_PARKINGSPACE, NoAction},                     /* <B> */
    {STATE_PARKINGSPACE, ProcessParkSpace},             /* <C> */
    {STATE_PARKINGSPACE, ExitOrUndoDigit},              /* <D> */
    {STATE_PARKINGSPACE, NoAction},                     /* <*> */
    {STATE_PARKINGSPACE, NoAction},                     /* <#> */
    {STATE_PARKINGSPACE, NoAction},                     /* other keypad keys  */
    {STATE_PARKINGSPACE, NoAction},                     /* card tapped/synced */
    {STATE_PARKINGSPACE, NoAction},                     /* topup card tapped  */
    {STATE_PARKINGSPACE, NoAction}                      /* other card tapped  */
  },

  /* Current State = STATE_PARKINGTIME */
  { {STATE_PARKINGTIME, AddParkTimeDigit},             /* <0> */
    {STATE_PARKINGTIME, AddParkTimeDigit},             /* <1> */
    {STATE_PARKINGTIME, AddParkTimeDigit},             /* <2> */
    {STATE_PARKINGTIME, AddParkTimeDigit},             /* <3> */
    {STATE_PARKINGTIME, AddParkTimeDigit},             /* <4> */
    {STATE_PARKINGTIME, AddParkTimeDigit},             /* <5> */
    {STATE_PARKINGTIME, AddParkTimeDigit},             /* <6> */
    {STATE_PARKINGTIME, AddParkTimeDigit},             /* <7> */
    {STATE_PARKINGTIME, AddParkTimeDigit},             /* <8> */
    {STATE_PARKINGTIME, AddParkTimeDigit},             /* <9> */
    {STATE_PARKINGTIME, NoAction},                     /* <A> */
    {STATE_PARKINGTIME, NoAction},                     /* <B> */
    {STATE_PARKINGTIME, ProcessParkTime},              /* <C> */
    {STATE_PARKINGTIME, ExitOrUndoDigit},              /* <D> */
    {STATE_PARKINGTIME, NoAction},                     /* <*> */
    {STATE_PARKINGTIME, NoAction},                     /* <#> */
    {STATE_PARKINGTIME, NoAction},                     /* other keypad keys  */
    {STATE_PARKINGTIME, NoAction},                     /* card tapped/synced */
    {STATE_PARKINGTIME, NoAction},                     /* topup card tapped  */
    {STATE_PARKINGTIME, NoAction}                      /* other card tapped  */
  },
  
  /* Current State = STATE_MOBILE */
  { {STATE_MOBILE, NoAction},             /* <0> */
    {STATE_MOBILE, NoAction},             /* <1> */
    {STATE_MOBILE, NoAction},             /* <2> */
    {STATE_MOBILE, NoAction},             /* <3> */
    {STATE_MOBILEMTN, NoAction},          /* <4> */
    {STATE_MOBILE, NoAction},             /* <5> */
    {STATE_MOBILE, NoAction},             /* <6> */
    {STATE_MOBILEGLO, NoAction},          /* <7> */
    {STATE_MOBILE, NoAction},             /* <8> */
    {STATE_MOBILE, NoAction},             /* <9> */
    {STATE_MOBILE, NoAction},             /* <A> */
    {STATE_MOBILEAIRTEL, NoAction},       /* <B> */
    {STATE_MOBILEETISALAT, NoAction},     /* <C> */
    {STATE_HOME, NoAction},               /* <D> */
    {STATE_MOBILE, NoAction},             /* <*> */
    {STATE_MOBILE, NoAction},             /* <#> */
    {STATE_MOBILE, NoAction},             /* other keypad keys  */
    {STATE_MOBILE, NoAction},             /* card tapped/synced */
    {STATE_MOBILE, NoAction},             /* topup card tapped  */
    {STATE_MOBILE, NoAction}              /* other card tapped  */
  },
  
  /* Current State = STATE_MOBILEMTN */
  { {STATE_MOBILEMTN, NoAction},             /* <0> */
    {STATE_MOBILEMTN, NoAction},             /* <1> */
    {STATE_MOBILEMTN, NoAction},             /* <2> */
    {STATE_MOBILEMTN, NoAction},             /* <3> */
    {STATE_MOBILE, MobileGetMtn100},         /* <4> */
    {STATE_MOBILEMTN, NoAction},             /* <5> */
    {STATE_MOBILEMTN, NoAction},             /* <6> */
    {STATE_MOBILE, MobileGetMtn200},         /* <7> */
    {STATE_MOBILEMTN, NoAction},             /* <8> */
    {STATE_MOBILEMTN, NoAction},             /* <9> */
    {STATE_MOBILEMTN, NoAction},             /* <A> */
    {STATE_MOBILE, MobileGetMtn750},         /* <B> */
    {STATE_MOBILE, MobileGetMtn1500},        /* <C> */
    {STATE_HOME, NoAction},                  /* <D> */
    {STATE_MOBILE, MobileGetMtn400},         /* <*> */
    {STATE_MOBILEMTN, NoAction},             /* <#> */
    {STATE_MOBILEMTN, NoAction},             /* other keypad keys  */
    {STATE_MOBILEMTN, NoAction},             /* card tapped/synced */
    {STATE_MOBILEMTN, NoAction},             /* topup card tapped  */
    {STATE_MOBILEMTN, NoAction}              /* other card tapped  */
  },
  
  /* Current State = STATE_MOBILEGLO */
  { {STATE_MOBILEGLO, NoAction},             /* <0> */
    {STATE_MOBILEGLO, NoAction},             /* <1> */
    {STATE_MOBILEGLO, NoAction},             /* <2> */
    {STATE_MOBILEGLO, NoAction},             /* <3> */
    {STATE_MOBILE, MobileGetGlo100},         /* <4> */
    {STATE_MOBILEGLO, NoAction},             /* <5> */
    {STATE_MOBILEGLO, NoAction},             /* <6> */
    {STATE_MOBILE, MobileGetGlo200},         /* <7> */
    {STATE_MOBILEGLO, NoAction},             /* <8> */
    {STATE_MOBILEGLO, NoAction},             /* <9> */
    {STATE_MOBILEGLO, NoAction},             /* <A> */
    {STATE_MOBILE, MobileGetGlo1000},        /* <B> */
    {STATE_MOBILE, MobileGetGlo3000},        /* <C> */
    {STATE_HOME, NoAction},                  /* <D> */
    {STATE_MOBILE, MobileGetGlo500},         /* <*> */
    {STATE_MOBILEGLO, NoAction},             /* <#> */
    {STATE_MOBILEGLO, NoAction},             /* other keypad keys  */
    {STATE_MOBILEGLO, NoAction},             /* card tapped/synced */
    {STATE_MOBILEGLO, NoAction},             /* topup card tapped  */
    {STATE_MOBILEGLO, NoAction}              /* other card tapped  */
  },
  
  /* Current State = STATE_MOBILEAIRTEL */
  { {STATE_MOBILEAIRTEL, NoAction},             /* <0> */
    {STATE_MOBILEAIRTEL, NoAction},             /* <1> */
    {STATE_MOBILEAIRTEL, NoAction},             /* <2> */
    {STATE_MOBILEAIRTEL, NoAction},             /* <3> */
    {STATE_MOBILE, MobileGetAirtel100},         /* <4> */
    {STATE_MOBILEAIRTEL, NoAction},             /* <5> */
    {STATE_MOBILEAIRTEL, NoAction},             /* <6> */
    {STATE_MOBILE, MobileGetAirtel200},         /* <7> */
    {STATE_MOBILEAIRTEL, NoAction},             /* <8> */
    {STATE_MOBILEAIRTEL, NoAction},             /* <9> */
    {STATE_MOBILEAIRTEL, NoAction},             /* <A> */
    {STATE_MOBILE, MobileGetAirtel500},         /* <B> */
    {STATE_MOBILE, MobileGetAirtel1000},        /* <C> */
    {STATE_HOME, NoAction},                     /* <D> */
    {STATE_MOBILEAIRTEL, NoAction},             /* <*> */
    {STATE_MOBILEAIRTEL, NoAction},             /* <#> */
    {STATE_MOBILEAIRTEL, NoAction},             /* other keypad keys  */
    {STATE_MOBILEAIRTEL, NoAction},             /* card tapped/synced */
    {STATE_MOBILEAIRTEL, NoAction},             /* topup card tapped  */
    {STATE_MOBILEAIRTEL, NoAction}              /* other card tapped  */
  },
  
  /* Current State = STATE_MOBILEETISALAT */
  { {STATE_MOBILEETISALAT, NoAction},             /* <0> */
    {STATE_MOBILEETISALAT, NoAction},             /* <1> */
    {STATE_MOBILEETISALAT, NoAction},             /* <2> */
    {STATE_MOBILEETISALAT, NoAction},             /* <3> */
    {STATE_MOBILE, MobileGetEtisalat100},         /* <4> */
    {STATE_MOBILEETISALAT, NoAction},             /* <5> */
    {STATE_MOBILEETISALAT, NoAction},             /* <6> */
    {STATE_MOBILE, MobileGetEtisalat200},         /* <7> */
    {STATE_MOBILEETISALAT, NoAction},             /* <8> */
    {STATE_MOBILEETISALAT, NoAction},             /* <9> */
    {STATE_MOBILEETISALAT, NoAction},             /* <A> */
    {STATE_MOBILE, MobileGetEtisalat500},         /* <B> */
    {STATE_MOBILE, MobileGetEtisalat1000},        /* <C> */
    {STATE_HOME, NoAction},                       /* <D> */
    {STATE_MOBILEETISALAT, NoAction},             /* <*> */
    {STATE_MOBILEETISALAT, NoAction},             /* <#> */
    {STATE_MOBILEETISALAT, NoAction},             /* other keypad keys  */
    {STATE_MOBILEETISALAT, NoAction},             /* card tapped/synced */
    {STATE_MOBILEETISALAT, NoAction},             /* topup card tapped  */
    {STATE_MOBILEETISALAT, NoAction}              /* other card tapped  */
  },
  
  /* Current State = STATE_UTILITY */
  { {STATE_UTILITY, NoAction},             /* <0> */
    {STATE_UTILITY, NoAction},             /* <1> */
    {STATE_UTILITY, NoAction},             /* <2> */
    {STATE_UTILITY, NoAction},             /* <3> */
    {STATE_UTILITY, NoAction},             /* <4> */
    {STATE_UTILITY, NoAction},             /* <5> */
    {STATE_UTILITY, NoAction},             /* <6> */
    {STATE_UTILITY, NoAction},             /* <7> */
    {STATE_UTILITY, NoAction},             /* <8> */
    {STATE_UTILITY, NoAction},             /* <9> */
    {STATE_UTILITY, NoAction},             /* <A> */
    {STATE_UTILITYPOWER, NoAction},        /* <B> */
    {STATE_UTILITYWATER, NoAction},        /* <C> */
    {STATE_HOME, NoAction},                /* <D> */
    {STATE_UTILITY, NoAction},             /* <*> */
    {STATE_UTILITY, NoAction},             /* <#> */
    {STATE_UTILITY, NoAction},             /* other keypad keys  */
    {STATE_UTILITY, NoAction},             /* card tapped/synced */
    {STATE_UTILITY, NoAction},             /* topup card tapped  */
    {STATE_UTILITY, NoAction}              /* other card tapped  */
  },
  
  /* Current State = STATE_UTILITYPOWER */
  { {STATE_UTILITYPOWER, NoAction},             /* <0> */
    {STATE_UTILITYPOWER, NoAction},             /* <1> */
    {STATE_UTILITYPOWER, NoAction},             /* <2> */
    {STATE_UTILITYPOWER, NoAction},             /* <3> */
    {STATE_UTILITYPOWER, NoAction},             /* <4> */
    {STATE_UTILITYPOWER, NoAction},             /* <5> */
    {STATE_UTILITYPOWER, NoAction},             /* <6> */
    {STATE_UTILITYPOWER, NoAction},             /* <7> */
    {STATE_UTILITYPOWER, NoAction},             /* <8> */
    {STATE_UTILITYPOWER, NoAction},             /* <9> */
    {STATE_UTILITYPOWER, NoAction},             /* <A> */
    {STATE_UTILITYPOWER, NoAction},             /* <B> */
    {STATE_UTILITYPOWER, NoAction},             /* <C> */
    {STATE_HOME, NoAction},                     /* <D> */
    {STATE_UTILITYPOWER, NoAction},             /* <*> */
    {STATE_UTILITYPOWER, NoAction},             /* <#> */
    {STATE_UTILITYPOWER, NoAction},             /* other keypad keys  */
    {STATE_UTILITYPOWER, NoAction},             /* card tapped/synced */
    {STATE_UTILITYPOWER, NoAction},             /* topup card tapped  */
    {STATE_UTILITYPOWER, NoAction}              /* other card tapped  */
  },
  
  /* Current State = STATE_UTILITYWATER */
  { {STATE_UTILITYWATER, NoAction},             /* <0> */
    {STATE_UTILITYWATER, NoAction},             /* <1> */
    {STATE_UTILITYWATER, NoAction},             /* <2> */
    {STATE_UTILITYWATER, NoAction},             /* <3> */
    {STATE_UTILITYWATER, NoAction},             /* <4> */
    {STATE_UTILITYWATER, NoAction},             /* <5> */
    {STATE_UTILITYWATER, NoAction},             /* <6> */
    {STATE_UTILITYWATER, NoAction},             /* <7> */
    {STATE_UTILITYWATER, NoAction},             /* <8> */
    {STATE_UTILITYWATER, NoAction},             /* <9> */
    {STATE_UTILITYWATER, NoAction},             /* <A> */
    {STATE_UTILITYWATER, NoAction},             /* <B> */
    {STATE_UTILITYWATER, NoAction},             /* <C> */
    {STATE_HOME, NoAction},                     /* <D> */
    {STATE_UTILITYWATER, NoAction},             /* <*> */
    {STATE_UTILITYWATER, NoAction},             /* <#> */
    {STATE_UTILITYWATER, NoAction},             /* other keypad keys  */
    {STATE_UTILITYWATER, NoAction},             /* card tapped/synced */
    {STATE_UTILITYWATER, NoAction},             /* topup card tapped  */
    {STATE_UTILITYWATER, NoAction}              /* other card tapped  */
  }
};
  


/*
 * KeyLookup
 * Description:      This function gets a key from the keypad and translates the
 *                   raw keycode to an enumerated keycode (eventcode) for the 
 *                   main loop.
 *
 * Arguments:        None
 * Return:           (enum eventcode) - type of the key input on keypad
 *
 * Input:            Keys from the keypad
 * Output:           None
 *
 * Operation:        Get key value from keypad. Loop through key values array,
 *                   and get the corresponding index. Use that index and get the
 *                   corresponding (enum eventcode) from the keycodes array.
 *                   Note that for this to work the array of keycodes MUST match
 *                   the array of key values.
 *
 * Error Handling:   Invalid keypad keys are returned as KEYCODE_ILLEGAL
 *
 * Algorithms:       The function uses an array to lookup the key types
 * Data Strutures:   Array of key types versus key codes
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 19, 2013      Nnoduka Eruchalu     Initial Revision
 */
static eventcode KeyLookup(void)
{
  /* variables */
  static const eventcode keycodes[] = /* array of (enum eventcode)s. */
    {                                    /* order must match keyvalues array */
      KEYCODE_1,        /* <1> */        /* exactly. Also needs to have an */
      KEYCODE_2,        /* <2> */        /* entry for illegal codes */
      KEYCODE_3,        /* <3> */
      KEYCODE_A,        /* <A> */
      KEYCODE_4,        /* <4> */
      KEYCODE_5,        /* <5> */
      KEYCODE_6,        /* <6> */
      KEYCODE_B,        /* <B> */
      KEYCODE_7,        /* <7> */
      KEYCODE_8,        /* <8> */
      KEYCODE_9,        /* <9> */
      KEYCODE_C,        /* <C> */
      KEYCODE_S,        /* <*> */
      KEYCODE_0,        /* <0> */
      KEYCODE_P,        /* <#> */
      KEYCODE_D,        /* <D> */
      KEYCODE_ILLEGAL   /* other keys */
    };
  
  static const uint8_t keyvalues[] =  /* array of key values */
    {                                 /* order must match keycodes array */
      KEY_1,        /* <1> */         /* exactly */
      KEY_2,        /* <2> */   
      KEY_3,        /* <3> */
      KEY_A,        /* <A> */
      KEY_4,        /* <4> */
      KEY_5,        /* <5> */
      KEY_6,        /* <6> */
      KEY_B,        /* <B> */
      KEY_7,        /* <7> */
      KEY_8,        /* <8> */
      KEY_9,        /* <9> */
      KEY_C,        /* <C> */
      KEY_S,        /* <*> */
      KEY_0,        /* <0> */
      KEY_P,        /* <#> */
      KEY_D         /* <D> */  
    };
  
  uint8_t keyval;     /* an input key value */
  size_t i;           /* general loop index */
  
  keyval = GetKey();  /* get a key */
  
  /* lookup index in key values array */
  for(i=0; 
      ((i < (sizeof(keyvalues)/sizeof(uint8_t))) && (keyval != keyvalues[i]));
      i++);
  
  return keycodes[i]; /* return the appropriate (enum eventcode) */
}


/*
 * CardLookup
 * Description:      This function gets a code from the smartcard and translates
 *                   it to an enumerated eventcode for the main loop.
 *
 * Arguments:        None
 * Return:           (enum eventcode) - type of smartcard last tapped
 *
 * Input:            Communications from SmartCards
 * Output:           Communications to SmartCards
 *
 * Operation:        Get card value from smartcard reader. Loop through card
 *                   values array, and get the corresponding index. Use that
 *                   index and get the corresponding (enum eventcode) from the 
 *                   cardcodes array.
 *                   Note that for this to work the array of cardcodes MUST
 *                   match the array of card values.
 *
 * Error Handling:   Invalid but recognized cards are returned as
 *                   CARDCODE_ILLEGAL     
 *
 * Error Handling:   None
 *
 * Algorithms:       None.
 * Data Strutures:   None.
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 21, 2013      Nnoduka Eruchalu     Initial Revision
 *   May  14, 2013      Nnoduka Eruchalu     Fleshed out from stub function
 */
static eventcode CardLookup(void)
{
  /* variables */
  static const eventcode cardcodes[] =   /* array of (enum eventcode)s. */
    {                                    /* order must match cardvalues array */
      CARDCODE_TAP,     /* EasyCard  */  /* exactly. Also needs to have an */
      CARDCODE_TOPUP,   /* EasyTopup */  /* entry for illegal codes */
      CARDCODE_ILLEGAL  /* other cards */
    };
  
  static const uint8_t cardvalues[] =  /* array of card values */
    {                                  /* order must match cardcodes array */
      CARD_TAP,     /* EasyCard  */    /* exactly */
      CARD_TOPUP,   /* EasyTopup */
    };
  
  uint8_t cardval;      /* a tapped card value */
  size_t i;             /* general loop index */
  
  cardval = GetCard();  /* get a card */
  
  /* lookup index in card values array */
  for(i=0; 
      ((i < (sizeof(cardvalues)/sizeof(uint8_t))) && (cardval !=cardvalues[i]));
      i++);
  
  return cardcodes[i]; /* return the appropriate (enum eventcode) */
}
 
 
/*
 * StateDriver
 * Description:      This procedure is to be called in the main loop.
 *                   It loops getting keys from the keypad, processing those
 *                   keys as is appropriate. It also handles updating the
 *                   display.
 *                   
 * Arguments:        None
 * Return:           None
 *
 * Input:            Keys from the keypad
 * Output:           Status info and UI -- display
 *
 * Operation:        To prevent system hanging, first check for a keypress
 *                   before trying to get a keycode. Use this keycode to index
 *                   the StateTable.
 *
 * Error Handling:   Invalid input is ignored
 *
 * Algorithms:       The function is table-driven. The processing routines for
 *                   each input are given in tables which are selected based on
 *                   the context (state) in which the program is operating.
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *   Apr. 19, 2013      Nnoduka Eruchalu     Initial Revision
 *   May  14, 2013      Nnoduka Eruchalu     Added cardtap detection
 */
void StateDriver(void)
{
  /* variables */
  eventcode event;             /* a system input/event       */
  state prev_state;            /* previous system state      */
  state curr_state;            /* current sysem state        */
  state nextstate;             /* proposed next state in FSM */
  
  curr_state = STATE_WELCOME;  /* start system on welcome page */
  prev_state = STATE_WELCOME; 
  LcdWriteFill(DisplayTables[curr_state]); /* start by showing something */

  /* infinite loop processing input */
  while (TRUE) {
    /* handle updates */
    curr_state = UpdateTable[curr_state](curr_state);
    
    if (IsAKey() || IsACard()) { /* check for keypad input/ card tap */
      if(IsAKey()) event = KeyLookup();  /* keypad input is higher priority */
      else         event = CardLookup(); /* than card tap                   */
      
      nextstate = StateTable[curr_state][event].nextstate;
      /* execute action and update state */
      curr_state = StateTable[curr_state][event].action(nextstate, event);
    }
    
    /* finally, if the state has changed - update display to reflect it */
    if (curr_state != prev_state) {
      LcdWriteFill(DisplayTables[curr_state]);
    }
    
    /* always remember the current status for the next loop iteration */
    prev_state = curr_state;
  }
}
