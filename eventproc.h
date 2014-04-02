/*
 * -----------------------------------------------------------------------------
 * -----                           EVENTPROC.H                             -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the header file for the library of functions for the event 
 *   processing functions defined in eventproc.c
 *
 * Assumptions:
 *   None
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Apr. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */


#ifndef EVENTPROC_H
#define EVENTPROC_H

/* library include files */
/* none */

/* local include files */
#include "interface.h"
#include "interrupts.h"


/* CONSTANTS */
#define NUM_PIN_DIGITS        4    /* number of pincode digits              */
#define NUM_PARK_SPACE_DIGITS 4    /* number of parking space digits        */
#define NUM_PARK_TIME_DIGITS  4    /* time format is hh:mm                  */
#define MSG_FLASH_TIME        1    /* time to flash a msg (in seconds)      */
#define PIN_FLASH_TIME        0.5  /* time to flash a pin digit (in seconds)*/

/* TIMING PARAMETERS 
   -- assuming Timer2 
   Counting up to 1s requires counting 1/TMR2_PERIOD == TMR2_FREQ
*/
#define ELAPSED_SCALE         TMR2_FREQ
#define TIME_SCALE            ELAPSED_SCALE
#define DISPLAYTIME_SECS      6         /* DisplayTime format is hh:mm:ss */
#define DISPLAYTIME_MINS      4         /* DisplayTime format is hh:mm    */

/* FUNCTION PROTOTYPES */
/* Interrupt Service Routine Functions */
/* time counter */
void EventTimer(void);


/* UpdateTable Routines */
/* do nothing */
extern state NoUpdate(state curr_state);

/* flash newest digit then hide it */
extern state UpdatePin(state curr_state);

/* write in numerical account balance on Account Page */
extern state UpdateAccount(state curr_state);

/* write in parking space and time left */
extern state UpdatePark(state curr_state);

/* write in current space or entered space */
extern state UpdateParkSpace(state curr_state);

/* write in entered time sequence */
extern state UpdateParkTime(state curr_state);


/* State Machine Actions */
/* do nothing */
extern state NoAction(state nextstate, eventcode event); 

/* reset system variables excluding state */
extern state ResetAction(state nextstate, eventcode event);

/* get user data from tapped EasyCard */
extern state GetUserData(state nextstate, eventcode event);

/* Add Pin Digit */
extern state AddPinDigit(state nextstate, eventcode event);

/* Process Pin Number */
extern state ProcessPin(state nextstate, eventcode event);

/* Exit to welcome page or undo last digit entry */
extern state ExitOrUndoDigit(state nextstate, eventcode event);

/* Get Account Balance */
extern state GetAcctBalance(state nextstate, eventcode event);

/* Process EasyTopup */
extern state ProcessAcctRecharge(state nextstate, eventcode event);

/* Get Current Parking Status */
extern state GetParkStatus(state nextstate, eventcode event);

/* Add Parking Space Digit */
extern state AddParkSpaceDigit(state nextstate, eventcode event);

/* Process Parking Space Number */
extern state ProcessParkSpace(state nextstate, eventcode event);

/* Add Parking Time Digit */
extern state AddParkTimeDigit(state nextstate, eventcode event);

/* Process Parking Time */
extern state ProcessParkTime(state nextstate, eventcode event);

/* get user's utility IDs */
extern state GetUtilityData(state nextstate, eventcode event);


/* Get MTN VTU Specific Recharge Vouchers */
extern state MobileGetMtn100(state nextstate, eventcode event);
extern state MobileGetMtn200(state nextstate, eventcode event);
extern state MobileGetMtn400(state nextstate, eventcode event);
extern state MobileGetMtn750(state nextstate, eventcode event);
extern state MobileGetMtn1500(state nextstate, eventcode event);

/* Get Glo QuickCharge Specific Recharge Vouchers */
extern state MobileGetGlo100(state nextstate, eventcode event);
extern state MobileGetGlo200(state nextstate, eventcode event);
extern state MobileGetGlo500(state nextstate, eventcode event);
extern state MobileGetGlo1000(state nextstate, eventcode event);
extern state MobileGetGlo3000(state nextstate, eventcode event);

/* Get Airtel Top-up Specific Recharge Vouchers */
extern state MobileGetAirtel100(state nextstate, eventcode event);
extern state MobileGetAirtel200(state nextstate, eventcode event);
extern state MobileGetAirtel500(state nextstate, eventcode event);
extern state MobileGetAirtel1000(state nextstate, eventcode event);

/* Get Etisalat E-Top up Specific Recharge Vouchers */
extern state MobileGetEtisalat100(state nextstate, eventcode event);
extern state MobileGetEtisalat200(state nextstate, eventcode event);
extern state MobileGetEtisalat500(state nextstate, eventcode event);
extern state MobileGetEtisalat1000(state nextstate, eventcode event);



#endif                                                         /* EVENTPROC_H */
