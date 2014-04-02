/*
 * -----------------------------------------------------------------------------
 * -----                           INTERFACE.H                             -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the header file for the library of functions and state machine 
 *   tables for a user interface, defined in interface.c.
 *
 * Assumptions:
 *   None
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Apr. 19, 2013      Nnoduka Eruchalu     Initial Revision
 */

#ifndef INTERFACE_H
#define INTERFACE_H


/* state types */
typedef enum {
  STATE_WELCOME,          /* system idle -- welcome  */
  STATE_PIN,              /* enter pin number        */
  STATE_HOME,             /* home page -- system nav */
  
  STATE_ACCOUNT,          /* account page            */
  STATE_ACCOUNTRECHARGE,  /* account->recharge       */
  
  STATE_PARKING,          /* parking summary page    */
  STATE_PARKINGSPACE,     /* parking: enter space    */
  STATE_PARKINGTIME,      /* parking: enter time     */
  
  STATE_MOBILE,           /* pick mobile top-up type */
  STATE_MOBILEMTN,        /* MTN VTU                 */
  STATE_MOBILEGLO,        /* Glo QuickCharge         */
  STATE_MOBILEAIRTEL,     /* Airtel Top-Up           */
  STATE_MOBILEETISALAT,   /* Etisalat E-Top up       */
  
  STATE_UTILITY,          /* select utility to pay   */
  STATE_UTILITYPOWER,     /* pay power: enter amount */
  STATE_UTILITYWATER,     /* pay water: enter amount */
    
  NUM_STATES              /* number of states        */
} state;

/* event representing:
 * - all possible input keycodes from keypad, 
 * - events from Smartcards (EasyCard, EasyTopup)
 * 
 */
typedef enum {
  /* keypad keycodes */
  KEYCODE_0,        /* <0> */
  KEYCODE_1,        /* <1> */
  KEYCODE_2,        /* <2> */
  KEYCODE_3,        /* <3> */
  KEYCODE_4,        /* <4> */
  KEYCODE_5,        /* <5> */
  KEYCODE_6,        /* <6> */
  KEYCODE_7,        /* <7> */
  KEYCODE_8,        /* <8> */
  KEYCODE_9,        /* <9> */
  KEYCODE_A,        /* <A> */
  KEYCODE_B,        /* <B> */
  KEYCODE_C,        /* <C> */
  KEYCODE_D,        /* <D> */
  KEYCODE_S,        /* <*> */
  KEYCODE_P,        /* <#> */
  KEYCODE_ILLEGAL,  /* other keypad keys      */
  
  /* smartcard events */
  CARDCODE_TAP,     /* EasyCard tapped/synced */
  CARDCODE_TOPUP,   /* EasyTopup tapped       */
  CARDCODE_ILLEGAL, /* other cards tapped     */   
  
  NUM_EVENTS        /* number of events       */
} eventcode;

/* state transition struct */
typedef struct {
  state nextstate;                                   /* state for transition  */
  state (* action)(state nextstate, eventcode event);/* action for transition */
    } state_transition;


/* FUNCTION PROTOTYPES */
/* loop driving the finite state machine */
extern void StateDriver(void);


#endif                                                         /* INTERFACE_H */
