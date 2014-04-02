/*
 * -----------------------------------------------------------------------------
 * -----                             DATA.C                                -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *  This is the library of functions for interfacing the PIC18F67K22 with the
 *  Http Server.
 *  Makes calls like:
 *    SimHttpGet("/test/", "p1=trust&p2=me", &http_response);
 *    SimHttpGet("/test/json/", NULL, &http_response);
 *    SimHttpPost("/test/", "p5=stay&p6=positive", &http_response);
 *
 * Table of Contents:
 * (private)
 *   UidToString      - convert int array UID to a char array
 *
 * (public)
 *   DataInit         - initializes the data module and it's variables
 *   DataCardValidate - determine smartcard type server side
 *   DataPinValidate  - validate pin on server side
 *   DataAcctBalance  - get account balance (in kobos)
 *   DataAcctRecharge - recharge account with EasyTopup card
 *   DataParkDetails  - get parking space & time if they exist 
 *   DataParkPay      - pay for/extend a parking space
 *   DataAlertPark    - send notification Email for successful parking payment
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
 *   May  14, 2013      Nnoduka Eruchalu     Initial Revision
 *   Mar 30, 2014      Nnoduka Eruchalu     Cleaned up comments
 */
#include "general.h"
#include <stdint.h>
#include "data.h"
#include "sim5218.h"
#include "mifare.h"
#include "eventproc.h"

/* shared variables have to be local to this file */
static http_data http_response; /* Http Response struct */
static sim_data module;         /* SIM5218 module       */

static const char *card_validate_url = "/card/validate/";
static const char *pin_validate_url = "/pin/validate/";
static const char *acct_balance_url = "/account/balance/";
static const char *acct_recharge_url = "/account/recharge/";
static const char *park_details_url = "/park/details/";
static const char *park_pay_url = "/park/pay/";

static const char *alert_park_url = "/alert/park/";


/* local functions that need not be public */
static void UidToString(uint8_t *uid, char *buffer);


/*
 * UidToString
 * Description: Save Mifare tag's UID in a string buffer array
 *
 * Arguments:   uid:    UID to convert to string
 *              buffer: array to save string in [must be at least 14 bytes long]
 * Return:      None
 *
 * Operation:   Loop through UID bytes and save as hex string.
 *  
 * Revision History:
 *   May 15, 2013      Nnoduka Eruchalu     Initial Revision
 *   Mar 30, 2014      Nnoduka Eruchalu     Cleaned up comments
 */
static void UidToString(uint8_t *uid, char *buffer)
{
  char nibble, num;
  size_t i;
  for(i=0; i < 7;i++) {
    num = uid[i];
    
    /* extract high nibble and write out numeric or alpha */
    nibble = (num & 0xF0) >> 4;
    if (nibble < 10) buffer[2*i] = '0'+nibble;
    else             buffer[2*i] = nibble-10+'A';
    
    /* extract low nibble and write out numeric or alpha */
    nibble = (num & 0x0F);
    if (nibble < 10) buffer[2*i+1] = '0'+nibble;
    else             buffer[2*i+1] = nibble-10+'A';
  }
}


/*
 * DataInit
 * Description: This procedure initializes the shared variables, and clears the 
 *              data transfer timer.
 *
 * Arguments:   None
 * Return:      None
 *
 * Operation:   Initialize the module representation.
 *
 * Assumptions: Called after setting up Serial channel 2 and interrupts are 
 *              enabled
 *  
 * Revision History:
 *   May 14, 2013      Nnoduka Eruchalu     Initial Revision
 *   Mar 30, 2014      Nnoduka Eruchalu     Cleaned up comments
 */
void DataInit(void)
{
  SimStartTimer(0);            /* reset SIMCOM module Timer */
  SimDataInit(&module);        /* initialize module object */
}


/*
 * DataCardValidate
 * Description: Determine smartcard type server side
 * 
 * Arguments:   tag: pointer to repr of MIFARE card to validate
 * Return:      smartcard type (SMARTCARD CODE) with following values:
 *              - CARD_TAP:     EasyCard
 *              - CARD_TOPUP:   EasyTopup
 *              - CARD_INVALID: Invalid Card
 *
 * Operation:   Convert UID to a string.
 *              Do a HTTP POST with the string UID as a parameter. The smartcard
 *              type is returned from server
 *              The smartcard code is returned in the HTTP response's number
 *  
 * Revision History:
 *   May 15, 2013      Nnoduka Eruchalu     Initial Revision
 *   Mar 30, 2014      Nnoduka Eruchalu     Cleaned up comments
 */
uint8_t DataCardValidate(mifare_tag *tag)
{
  /* each uid hex num is 2 bytes [so 14], "uid=" is 4 bytes, & NULL-terminator
   *
   * "uid=" [4]
   * each uid hex num is 2 bytes [14],
   * NULL-terminator [1]
   */
  char param_str[4+14+1];          /* allocate space for param */
  strcpy(param_str, "uid=");       /* load in UID key    */
  UidToString(tag->uid, &param_str[4]); /* load in UID string */
  param_str[sizeof(param_str)-1] = '\0'; /* add NULL-terminator */
    
  SimHttpPost(card_validate_url, param_str, &http_response);
  
  return ((uint8_t) http_response.number);
}


/*
 * DataPinValidate
 * Description: Validate entered pin-code
 * 
 * Arguments:   uid: UID of EasyCard
 * Return:      TRUE:  for valid pincode
 *              FALSE: for invalid pincode
 *
 * Operation:   Convert UID to a string.
 *              Do a HTTP POST with the string UID and pin as a parameters.
 *              Note that a pin number of 0123 (base 10) is transmitted as 
 *              pin=123
 *              The validity of the PIN will be in the HTTP response's boolean
 *
 * Assumption:  Assumes complete pin number [all digits present]
 *  
 * Revision History:
 *   May 15, 2013      Nnoduka Eruchalu     Initial Revision
 */
uint8_t DataPinValidate(uint8_t *uid, uint16_t pin)
{
  /*
   * "uid=" [4]
   * each uid hex num is 2 bytes [14],
   * "&pin=" [5]
   * pin number [NUM_PIN_DIGITS]
   * NULL-terminator [1]
   */
  char param_str[4+14+5+NUM_PIN_DIGITS+1];   /* allocate space for params */
  strcpy(param_str, "uid=");          /* load in UID key    */
  UidToString(uid, &param_str[4]);    /* load in UID string */
  strcpy(&param_str[18], "&pin="); 
  sprintf(&param_str[23], "%u", pin); /* automatically adds NULL-terminator */
  param_str[sizeof(param_str)-1] = '\0'; /* add NULL-terminator: just because */
  
  SimHttpPost(pin_validate_url, param_str, &http_response);
  return http_response.boolean;
}


/*
 * DataAcctBalance
 * Description: Get account balance (in Kobos)
 * 
 * Arguments:   uid: UID of EasyCard
 * Return:      account balance (in kobos)
 *
 * Operation:   Convert UID to a string.
 *              Do a HTTP GET with the string UID as a parameter.
 *              The balance will be in the HTTP response's number.
 *  
 * Revision History:
 *   May 16, 2013      Nnoduka Eruchalu     Initial Revision
 */
uint32_t DataAcctBalance(uint8_t *uid)
{
  /*
   * "uid=" [4]
   * each uid hex num is 2 bytes [14],
   * NULL-terminator [1]
   */
  char param_str[4+14+1];             /* allocate space for params */
  strcpy(param_str, "uid=");          /* load in UID key    */
  UidToString(uid, &param_str[4]);    /* load in UID string */
  param_str[sizeof(param_str)-1] = '\0'; /* add NULL-terminator */
  
  SimHttpGet(acct_balance_url, param_str, &http_response);
  return http_response.number;
}


/*
 * DataAcctRecharge
 * Description: Recharge account with EasyTopup card
 * 
 * Arguments:   uid: UID of EasyCard
 *              topup_id: UID of EasyTopup card
 *              recharge_value: (ptr) to value of EasyTopup (in kobo) [modifed]
 * Return:      recharge success
 *
 * Operation:   Do a HTTP POST with the UID and topup_id as parameters
 *              The value of the EasyTopup card will will be in the HTTP 
 *              response's number.
 *              This will be saved in recharge_value
 *              The success of the recharge operation is recorded in the HTTP 
 *              response's boolean. An unsuccessful recharge operation means the
 *              EasyTopup card has been used.
 *  
 * Revision History:
 *   May 16, 2013      Nnoduka Eruchalu     Initial Revision
 */
uint8_t DataAcctRecharge(uint8_t *uid, uint8_t *topup_id, 
                         uint32_t *recharge_value)
{
  /*
   * "uid=" [4]
   * each uid hex num is 2 bytes [14],
   * "&tid=" [5]
   * each topup id hex num is 1 bytes [14]
   * NULL-terminator [1]
   */
  char param_str[4+14+5+14+1];           /* allocate space for params */
  strcpy(param_str, "uid=");             /* load in UID key        */
  UidToString(uid, &param_str[4]);       /* load in UID string     */
  strcpy(&param_str[18], "&tid=");       /* load in TopupID key    */
  UidToString(topup_id, &param_str[23]); /* load in TopupID string */
  param_str[sizeof(param_str)-1] = '\0'; /* add NULL-terminator: just because */
  
  SimHttpPost(acct_recharge_url, param_str, &http_response);
  
  if (http_response.boolean)          /* if recharge was successful, update */
    *recharge_value = http_response.number;  /* get value of EasyTopup card */
  return http_response.boolean;              /* return success of recharge  */
}


/*
 * DataParkDetails
 * Description: Get parking details
 * 
 * Arguments:   uid: UID array for EasyCard
 *              space: (ptr) for storing space number [modified]
 *              time:  (ptr) for storing time (in seconds) [modified]
 *
 * Return:      TRUE: if user has currently running parking space
 *              FALSE: user doesn't currently have parking time left at a space
 *
 * Operation:   Do a HTTP GET with the UID as a parameter
 *              The space number will be in the HTTP response's number
 *              The time left will be in the HTTP response's number2
 *              HTTP response's boolean will be the return value.
 *  
 * Revision History:
 *   May 16, 2013      Nnoduka Eruchalu     Initial Revision
 */
uint8_t DataParkDetails(uint8_t *uid, uint32_t *space, int32_t *time)
{
  /*
   * "uid=" [4]
   * each uid hex num is 2 bytes [14],
   * NULL-terminator [1]
   */
  char param_str[4+14+1];             /* allocate space for params */
  strcpy(param_str, "uid=");          /* load in UID key    */
  UidToString(uid, &param_str[4]);    /* load in UID string */
  param_str[sizeof(param_str)-1] = '\0'; /* add NULL-terminator */
  
  SimHttpGet(park_details_url, param_str, &http_response);
  
  if(http_response.boolean) {         /* if user has time left at a space */
    *space = http_response.number;    /* save those details */
    *time = http_response.number2;
  }
  
  return http_response.boolean;
}


/*
 * DataParkPay
 * Description: Pay for/extend a parking space. Update the parking time if it is
 *              extended.
 *              If this is a time extension, time will be updated accordingly.
 * 
 * Arguments:   uid: UID array for EasyCard
 *              space: space number 
 *              time:  (ptr) for time (in seconds) [modified]
 *  
 * Return:      None
 *
 * Operation:   Do a HTTP POST with the UID, space and time as parameters
 *              The possibly extended time left will be in HTTP response's 
 *              number.
 *              If there is a time extension, it is recorded in HTTP response's 
 *              boolean.
 *  
 * Revision History:
 *   May 16, 2013      Nnoduka Eruchalu     Initial Revision
 */
void DataParkPay(uint8_t *uid, uint32_t space, int32_t *time)
{
  /*
   * "uid=" [4]
   * each uid hex num is 2 bytes [14],
   * "&space=" [7]
   * number of parking digits max out at NUM_PARK_SPACE_DIGITS
   * "&time=" [6]
   * number of time digits max out at NUM_PARK_TIME_DIGITS+2, with the +2 for
   *  converting from minutes to seconds
   * NULL-terminator [1]
   */
  
  /* allocate space for params */
  char param_str[4+ 14 +7 +NUM_PARK_SPACE_DIGITS +6 +NUM_PARK_TIME_DIGITS+2 +1];
  strcpy(param_str, "uid=");             /* load in UID key, values */
  UidToString(uid, &param_str[4]);       /* load in UID string     */
  
  /*
   * Load in key,value pairs utilizing the fact that strcpy and sprintf append
   * NULLs and so strlen will be used to get the next index to start writing
   * from. Start writing over last NULL character.
   */
  strcpy(&param_str[18], "&space=");     /* load in space key and value */
  sprintf(&param_str[25], "%u", space);  
  strcpy(&param_str[strlen(param_str)], "&time="); /* load time key and value */
  sprintf(&param_str[strlen(param_str)], "%u", *time);
  
  SimHttpPost(park_pay_url, param_str, &http_response);
  
  if (http_response.boolean)             /* if time was extended, update it */
    *time = http_response.number;        
  
  return;
}



/* ALERT ROUTINES */
/*
 * DataAlertPark
 * Description: Alert the user of successful parking payment for a given space 
 *              at a given time.
 * 
 * Arguments:   space: space number 
 *              time:  time (in seconds)
 * Return:      None
 *
 * Operation:   Do a HTTP POST with the space and time as parameters
 *  
 * Revision History:
 *   May 16, 2013      Nnoduka Eruchalu     Initial Revision
 *   Mar 30, 2014      Nnoduka Eruchalu     Cleaned up comments
 */
void DataAlertPark(uint32_t space, int32_t time)
{
  /*
   * "&s=" [3]
   * number of parking digits max out at NUM_PARK_SPACE_DIGITS
   * "&t=" [3]
   * number of time digits max out at NUM_PARK_TIME_DIGITS+2, with the +2 for
   *  converting from minutes to seconds
   * NULL-terminator [1]
   */
  
  /* allocate space for params */
  char param_str[3 +NUM_PARK_SPACE_DIGITS +3 +NUM_PARK_TIME_DIGITS+2 +1];
    
  /*
   * Load in key,value pairs utilizing the fact that strcpy and sprintf append
   * NULLs and so strlen will be used to get the next index to start writing
   * from. Start writing over last NULL character.
   */
  strcpy(param_str, "&s=");     /* load in space key and value */
  sprintf(&param_str[strlen(param_str)], "%u", space);  
  strcpy(&param_str[strlen(param_str)], "&t="); /* load time key and value */
  sprintf(&param_str[strlen(param_str)], "%u", time);
  
  SimHttpPost(alert_park_url, param_str, &http_response);
      
  return;
}
