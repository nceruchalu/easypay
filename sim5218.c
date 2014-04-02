/*
 * -----------------------------------------------------------------------------
 * -----                            SIM5218.C                              -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is a library of functions for interfacing the PIC18F67K22 with the
 *   SIM5218A, a data module
 *
 * Table of Contents:
 *   (local)
 *   CheckForOk              - Check for OK at the end of a message buffer
 *   ParseHttpBodyJson       - Parse json data in HTTP response body
 *
 *   (timer)
 *   SimStartTimer           - start a countdown timer with a Timer
 *   SimTimerISR             - Timer interrupt service routine.
 *
 *   (communication)
 *   SimPutStr               - output a command string to the serial channel
 *   SimPutStrLn             - output a string to serial channel and a newline
 *   SimGetBuf               - get a buffer of bytes from the serial channel
 *   SimPrintBuf             - (debug) useful for debugging
 *
 *   (memory management) 
 *   SimDataInit             - initialize the sim5218 data representation
 *  
 *   (AT Commands)
 *   SimReset                - reset the module
 *   SimNetworkReg           - check for network registration
 *   SimSetApn               - set IP APN
 *   SimHttpLaunch           - launch a Http Operation
 *   SimHttpLaunchGet        - launch a Http Get Operation 
 *   SimHttpLaunchPost       - launch a Http Post Operation
 *   SimHttp                 - perform a HTTP GET/POST Operation
 *   SimHttpGet              - perform a HTTP GET Operation
 *   SimHttpPost             - perform a HTTP POST Operation
 *   SimHttpParseResponse    - Parse HTTP Response
 * 
 * Limitations:
 *   None
 *
 * Documentation Sources:
 *   - SIM5218 datasheet
 *   - SIM5218_Serial_ATC_V1.36 - AT Command Set
 *   - http://www.cooking-hacks.com/index.php/documentation/tutorials/arduino-3g-gprs-gsm-gps
 *
 * Assumptions:
 *   None
 * 
 * Testing:
 *   To test SIM5218A call SimPutStrLn() with following, and see expected 
 *   outputs
 *     "AT"                                          --> OK
 *     "AT+CREG?"                                    --> +CREG: 0,1
 *     "AT+COPS?"                                    --> 0,0,"AT&T",2
 *     "AT+CGSOCKCONT=1,\"IP\",\"broadband\""        --> OK
 *     "AT+NETOPEN=\"TCP\",60000"                    --> Network Open
 *     "AT+CHTTPACT=\"easypay.strivinglink.com\",80" --> REQUEST
 *
 * Todo:
 *   Error Checking
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   May 07, 2013      Nnoduka Eruchalu     Initial Revision
 *   May 16, 2013      Nnoduka Eruchalu     Added number2 to http_response
 */

#include "general.h"
#include <stdint.h>     /* for uint*_t */
#include <string.h>
#include "sim5218.h"
#include "serial.h"
#include "delay.h"
#include "lcd.h"


/* shared variables have to be local to this file */
static unsigned int timer;                    /* serial comm. ms time counter */
static unsigned char timerOvertime;           /* serial comm. timeout flag */
static unsigned char rxBuf[800];              /* serial channel Rx buffer */
static unsigned int rxCount;

/* connection strings */
static const char *protocol = "http";
static const char *server = "easypay.strivinglink.com";
static const char *port = "80";
static const char *apn_att_ipad = "broadband"; /* AT&T ipad2 3G APN */

/* HTTP response JSON key strings */
static const char json_key_number[]  = "num1";
static const char json_key_number2[]  = "num2";
static const char json_key_message[] = "msg";
static const char json_key_bool[] = "bool"; 



/* local functions */
static int CheckForOk(void);
static void ParseHttpBodyJson(uint16_t start_index, uint16_t end_index,
                              http_data *http_response);


/*
 * CheckForOk
 * Description: Check for OK at the end of a response
 *
 * Operation:   Assuming response ends as:  <CR><LF>OK<CR><LF>
 *              So ensure rxCount >= 6 and rxBuf[rxCount-4] == 'O' and 
 *              rxBuf[rxCount-3]=='K'
 *
 * Arguments:   None
 * Return:      SUCCESS/FAIL
 *
 * Assumptions: response ends with  <CR><LF>OK<CR><LF>
 *
 * Revision History:
 *  May 11, 2013      Nnoduka Eruchalu     Initial Revision
 */
static int CheckForOk(void)
{
  if ((rxCount >= 6) && (rxBuf[rxCount-4]=='O') && (rxBuf[rxCount-3]=='K')) {
    return SUCCESS;
  } else {
    return FAIL;
  }
}


/*
 * ParseHttpBodyJson
 * Description: Parse Http response JSON content where response body contains
 *              a 1-level deep JSON block of form:
 *              {"bool": true, "num": 425, "msg": "hello world"}
 *             Save content in http_response
 *
 * Operation:   check for the number, message, and boolean keys. If those
 *              are detected extract their corresponding values.
 * 
 * Arguments:   start_index   - index of '{' in rxBuf
 *              end_index     - index of '}' in rxBuf
 * Return:      None
 * 
 * Shared:      http_response    [modified]
 *              json_key_number  [read only]
 *              json_key_number2 [read_only]
 *              json_key_message [read only]
 *              json_key_bool    [read only]
 *
 * Assumptions: HTTP response content-type: application/json
 *              message in json doesn't contain [escaped] quotes
 *
 * Revision History:
 *  May 13, 2013      Nnoduka Eruchalu     Initial Revision
 */
static void ParseHttpBodyJson(uint16_t start_index, uint16_t end_index, 
                              http_data *http_response) {
  size_t i, j; /* indices into rxBuf, and http_response.messages respectively */
    
  i = start_index;
  while(i<end_index) {  /* iterate through JSON block */
    /* if looking for key and see an open-quote, then determine exact value */
    if (rxBuf[i] == '"') { 
      
      /* is this a number?, perform check after open-quote */
      if(0 == memcmp(&rxBuf[i+1], json_key_number, sizeof(json_key_number)-1)) {
        /* contine after key and end quote */
        i += sizeof(json_key_number) + 1;
        
        /* skip space and colon characters */
        while((rxBuf[i] == ':') || (rxBuf[i] == ' ')) i++;
        
        /* now on a number character, extract all chars by checking for a comma
         * and closing curly brace that demarcates end of it all.
         * Convert chars to digits by subtracting '0'
         */
        http_response->number = 0;
        while((rxBuf[i] != ',') && (rxBuf[i] != '}')) {
          http_response->number *= 10;                /* get digit and move */
          http_response->number += rxBuf[i++] - '0';  /* on to next digit   */
        }
      } /* end is a number */
      
      
      /* is this a number2?, perform check after open-quote */
      if(0 ==memcmp(&rxBuf[i+1], json_key_number2, sizeof(json_key_number2)-1)){
        /* contine after key and end quote */
        i += sizeof(json_key_number2) + 1;
        
        /* skip space and colon characters */
        while((rxBuf[i] == ':') || (rxBuf[i] == ' ')) i++;
        
        /* now on a number2 character, extract all chars by checking for a comma
         * and closing curly brace that demarcates end of it all.
         * Convert chars to digits by subtracting '0'
         */
        http_response->number2 = 0;
        while((rxBuf[i] != ',') && (rxBuf[i] != '}')) {
          http_response->number2 *= 10;                /* get digit and move */
          http_response->number2 += rxBuf[i++] - '0';  /* on to next digit   */
        }
      } /* end is a number2 */
      
      
      /* is this a message?, perform check after open-quote */
      if(0 ==memcmp(&rxBuf[i+1], json_key_message, sizeof(json_key_message)-1)){
        /* contine after key and end quote */
        i += sizeof(json_key_message) + 1;
        
        /* skip space, colon and open-quote characters */
        while((rxBuf[i]==':') || (rxBuf[i]==' ') || (rxBuf[i]=='"')) i++;
        
        /* now on a string character, extract all chars by checking for a 
         * close-quote, and stay within boundsof message buffer
         */
        j = 0;
        while((rxBuf[i] != '"') && (j<(sizeof(http_response->message)-1))) {
          http_response->message[j++] = rxBuf[i++]; /* get chars sequentially */
        }
        http_response->message[j] = '\0';
      } /* end is a message */
      
      
      /* is this a boolean?, perform check after open-quote */
      if(0 ==memcmp(&rxBuf[i+1], json_key_bool, sizeof(json_key_bool)-1)){
        /* contine after key and end quote */
        i += sizeof(json_key_bool) + 1;
        
        /* skip space and colon characters */
        while((rxBuf[i]==':') || (rxBuf[i]==' ')) i++;
        
        /* now on first character of boolean, which is enough to extract value
         * as t is for true, and f is for false
         */
        if(rxBuf[i] == 't') http_response->boolean = TRUE;
        else                http_response->boolean = FALSE;
        
        /* keep going till done with boolean string */
        while((rxBuf[i] != ',') && (rxBuf[i] != '}')) i++;
      } /* end is a boolean */
      
      
    }     /* end is a key's open-quote */
    i++;  /* move on to next character byte */

  }       /* done iterating through JSON block */ 
}


/*
 * SimStartTimer
 * Description: Start a countdown timer with a Timer. Countdown for passed in 
 *              milliseconds.
 *
 * Arguments:   Countdown time in milliseconds
 * Return:      None
 *
 * Operation:   Start timer countdown to passed in argument, and clear out 
 *              overtime flag, as timer hasn't timed out yet.
 *
 * Limitations: Critical code on timerOvertime
 *
 * Revision History:
 *   May 07, 2013      Nnoduka Eruchalu     Initial Revision
 */
void SimStartTimer(unsigned int ms)
{
  timer = ms;             /* Start timer */
  timerOvertime = FALSE;  /* timer hasn't timed out yet */
}


/*
 * SimTimerISR
 * Description: Interrupt Service Routine for a Timer.
 *
 * Arguments:   None
 * Return:      None
 *
 * Operation:   On each ISR routine call, decrement the timer. When it hits 0, 
 *              the timer has hit timed out/hit overtime, so set the overtime 
 *              flag.
 * 
 * Limitations: Critical code on timerOvertime
 *
 * Revision History:
 *   May 07, 2013      Nnoduka Eruchalu     Initial Revision
 */
void SimTimerISR(void)
{
  if(timer > 0) {                          /* if timer hasn't timed out */
    timer--;                               /* decrement it; set the overtime */
    if(timer == 0) timerOvertime = TRUE;   /* flag when it does time out */
  }
}


/*
 * SimPutStr (to the SIM5218)
 * Description: This function outputs the passed string to the serial channel, 
 *              char by char. The bytewise outputting is done using 
 *              SerialPutChar2.
 *
 * Arguments:   cmdstr: command string
 * Return:      None
 *
 * Communication Format (MCU to SIM5218):
 *   - A prefix of "AT" or "at" (not case sensitive) must be included at the
 *     beginning of each command except A/ and +++
 *   - The character <CR> is used to finish a command line
 *   It is recommended that a command line only includes a command
 *
 * Operation:   This function loops through the string and outputs the bytes 
 *              using SerialPutChar2.
 *
 * Revision History:
 *   May 07, 2013      Nnoduka Eruchalu     Initial Revision
 */
void SimPutStr(const char *cmdstr)
{
  while(*cmdstr != '\0') {         /* loop through string chars */
    SerialPutChar2(*cmdstr++);     /* output chars              */
  }
}


/*
 * SimPutStrLn (to the SIM5218)
 * Description: This function outputs the passed string to the serial channel, 
 *              and end with a newline (<CR> and <LF>)
 *
 * Arguments:   cmdstr: command string
 * Return:      None
 *
 * Communication Format (MCU to SIM5218):
 *   - A prefix of "AT" or "at" (not case sensitive) must be included at the
 *     beginning of each command except A/ and +++
 *   - The character <CR> is used to finish a command line
 *   It is recommended that a command line only includes a command
 *
 * Operation:   Call SimPutStr to output string. After that output <CR> ('\r') 
 *              and <LF> ('\n')
 *
 * Revision History:
 *   May 11, 2013      Nnoduka Eruchalu     Initial Revision
 */
void SimPutStrLn(const char *cmdstr)
{
  SimPutStr(cmdstr);            /* first write command string */
  SerialPutChar2('\r');         /* then output <CR> */
  SerialPutChar2('\n');         /* and <LF> */
}


/*
 * SimGetBuf (from the SIM5218)
 * Description: This function gets data from the SIM5218 and saves in rxBuf.
 *
 * Communication Format (SL032 to MCU):
 *   - Valid responses: <CR><LF>response<CR><LF>
 *     <CR> = '\r' and <LF> = '\n'
 *   - Error responses: "ERROR" or "+CME ERROR" or "+CMS ERROR"
 *
 * Arguments:   None
 * Return:      None
 *
 * Operation:   First assume success on this Rx Sequence.
 *
 *              Keep track of the number of received bytes using the 0-indexed 
 *              rxCount.
 *              This will double as index into the rx buffer, and so always 1 
 *              less than actual number of rx'd bytes.
 *
 *              Next is to get into a loop and stay in that loop while we have a
 *              timeout.
 *              In each iteration, set timer and wait for a timeout or a serial 
 *              byte.
 *              If there are byte(s), grab. Else was a timeout so break from
 *              loop.
 *
 *              Note that checking for overtime is about more than just checking
 *              the flag.
 *              There is critical code surrounding this flag, so also check the 
 *              timer counter is 0.
 *
 *              When done receiving, zero out the unused rx buffer slots and 
 *              return.
 *
 * Error Checks: [TODO] UART Rx is successful (return of SUCCESS/FAIL depends 
 *               on this)
 *
 * Revision History:
 *   May 07, 2013      Nnoduka Eruchalu     Initial Revision
 */
void SimGetBuf(void)
{
  int rxCountOld; /* use to track changed rxCount */
  rxCount = 0;    /* 0-index'd number of bytes received */
  
  /* no way of detecting length so just keep looping till timeout */
  while(TRUE) {
    SimStartTimer(SIM_TIMERCOUNT); /* set timer; wait for timeout or byte */
    while(!SerialInRdy2() && !(timerOvertime && timer == 0));
    
    rxCountOld = rxCount;                       /* record old timer value   */
    while(SerialInRdy2()) {                     /* if no timeout grab bytes */
      rxBuf[rxCount++] = SerialGetChar2();      /* from serial channel      */
    }
    if (rxCount == rxCountOld){                 /* if there were no new bytes */
      break;                                    /* then we timed out; break   */
    }
  } 
}


/*
 * SimPrintBuf
 * Description: This function prints data received after SimGetBuf to Serial 
 *              Channel 1 and LCD. Great routine for debugging.
 *
 * Arguments:   None
 * Return:      None
 *
 * Operation:   Utilize the fact that SIM5218A wraps responses in <CR><LF> to 
 *              not worry about formatting. Print to Serial Channel and it 
 *              appears great in hyperterminal. However it appears wonky on LCD.
 *
 * Revision History:
 *   May 07, 2013      Nnoduka Eruchalu     Initial Revision
 */
void SimPrintBuf(void)
{
  int i = 0;
  LcdCursor(1,0);LcdWriteInt(rxCount); LcdWrite(':');
  while(i < rxCount) {
    SerialPutChar(rxBuf[i]);
    LcdWrite(rxBuf[i++]);
  }
}


/*
 * SimDataInit
 * Description: Initialize the SIM5218A data object with it's IMEI and IMSI
 *
 * Arguments:   module: pointer to sim data struct
 * Return:      None
 *
 * Operation:   Clear Timer
 *              Then Get the IMEI, and IMSI
 *              IMEI comes as:
 *                AT+CGSN<CR><LF>355841030885378<CR><LF><CR><LF>OK<CR><LF>
 *              IMSI comes as:
 *                AT+CIMI<CR><LF>310410603690803<CR><LF><CR><LF>OK<CR><LF>
 *
 *              Note that first imei/imsi digit is (15+8 = 23) digits from end
 *              So rxCount is expected to be at least 25
 *                -> [accounting for initial <CR><LF> before digits]
 *
 * Shared Variables: SIM5218A data representation
 *
 * Assumptions: Called after setting up Serial channel 2 and interrupts
 *
 * Error Check: [TODO]
 *
 * Revision History:
 *   May 07, 2013      Nnoduka Eruchalu     Initial Revision
 *   May 11, 2013      Nnoduka Eruchalu     Fleshed out the details
 *   May 13, 2013      Nnoduka Eruchalu     Make module an argument
 */
void SimDataInit(sim_data *module)
{
  size_t i;
  SimStartTimer(0);                             /* clear timer */

  SimPutStrLn("AT"); SimGetBuf();        /* read startup messages */

  SimPutStrLn("AT+CGSN"); SimGetBuf();          /* get the IMEI   */
  for(i=0;((i<15) && (rxCount >= 24));i++) {    /* and save it as */
    module->imei[i] = rxBuf[rxCount-23+i] - '0'; /* numeric digits */
  }
  
  SimPutStrLn("AT+CIMI"); SimGetBuf();          /* get the IMSI   */
  for(i=0;((i<15) && (rxCount >= 24));i++) {    /* and save it as */
    module->imsi[i] = rxBuf[rxCount-23+i] - '0'; /* numeric digits */
  }
  
  return;
}


/*
 * SimReset
 * Description: Reset the module
 *
 * Operation:   Send the reset AT-command, and wait appropriate # of seconds
 *
 * Arguments:   None
 * Return:      SUCCESS/FAIL
 *
 * Assumptions: None
 *
 * Revision History:
 *   May 11, 2013      Nnoduka Eruchalu     Initial Revision
 */
int SimReset(void) 
{
  int status;
  SimPutStrLn("AT+CRESET");      /* send reset AT-command   */
  SimGetBuf();                   /* get results immediately */
  status = CheckForOk();
  
  if (status == SUCCESS) {       /* if reset was successful    */
    __delay_s(SIM_RESET_TIME);   /* give SIM time to restart */
  }
  
  return status;
}


/*
 * SimNetworkReg
 * Description: Check for Network registration status.
 *
 * Operation:   Send the CREG AT-command and check return string is correct.
 *              The expected end of the return string is:
 *                <CR><LF>+CREG: 0,1<CR><LF><CR><LF>OK<CR><LF>
 *                So rxCount is >= 20
 *                and status is at index 9 from the end.
 *
 * Arguments:   pointer to status [modified]
 * Return:      SUCCESS/FAIL
 *              On SUCCESS, status updated with following values:
 *                0 - not registered; NOT searching new operator to register to
 *                1 - registered, home network
 *                2 - not registered; searching new operator to register to
 *                3 - registration denied
 *                4 - unknown
 *                5 - registered, roaming
 *
 * Assumptions: None
 *
 * Error Checking: Check for "OK" at the end
 *
 * Revision History:
 *   May 11, 2013      Nnoduka Eruchalu     Initial Revision
 */
int SimNetworkReg(uint8_t *status)
{
  uint8_t res;
  SimPutStrLn("AT+CREG?");      /* send Network Registration check AT-command */
  SimGetBuf();                  /* get results immediately */
  res = CheckForOk();           /* success of command depends on "OK" at end */
  
  if((rxCount >= 20) && (res==SUCCESS)) { /*if successful so far update */
    *status = rxBuf[rxCount-9] - '0';     /* status as a numeric */
  }
  
  return res;
}


/*
 * SimSetApn
 * Description: Set IP APN
 *
 * Operation:   Send the CGSOCKCONT AT-command and check "OK" is returned at the
 *              end. AT-command parameters are:
 *                Packet Data Protocol type = "IP"
 *                Access Point Name = passed in apn.
 *              Sample command string is: "AT+CGSOCKCONT=1,\"IP\",\"broadband\""
 *
 * Arguments:   apn - APN value string
 * Return:      SUCCESS/FAIL
 *
 * Assumptions: None
 *
 * Error Checking: Check for "OK" at the end
 *
 * Revision History:
 *   May 11, 2013      Nnoduka Eruchalu     Initial Revision
 */
int SimSetApn(const char *apn)
{
  SimPutStr("AT+CGSOCKCONT=1,\"IP\",\"");  /* set APN */
  SimPutStr(apn);
  SimPutStrLn("\"");
  SimGetBuf();                             /* get response from module */
  
  return CheckForOk();
}


/*
 * SimHttpLaunch
 * Description: Launch a HTTP Operation like GET or POST by first establishing
 *              connection to server.
 *
 * Operation:   Send the CHTTPACT AT-command with server and port as parameters.
 *              Be sure to send a <Ctrl+Z> (0x1A) after it
 *              Sample command string is: 
 *                 "AT+CHTTPACT=\"easypay.strivinglink.com\",80"
 *
 * Arguments:   None
 * Return:      None
 *
 * Assumptions: None
 *
 * Error Checking: None;
 *
 * Revision History:
 *   May 11, 2013      Nnoduka Eruchalu     Initial Revision
 */
void SimHttpLaunch(void) {  
  SimPutStr("AT+CHTTPACT=\"");    /* start connection to server */
  SimPutStr(server);
  SimPutStr("\",");
  SimPutStrLn(port);
  SerialPutChar2(0x1A);           /* send a <Ctrl+Z> */
}


/*
 * SimHttpLaunchGet
 * Description: Launch a HTTP GET Operation
 *
 * Operation:   Send the GET, Host and Content-Length commands. Sample command
 *              strings:
 *                 "GET http://easypay.strivinglink.com/test/json/ HTTP/1.1"
 *                 "Host: easypay.strivinglink.com"
 *                 "Content-Length: 0"
 *                 "" [empty line of course still has <CR> and <LF>]
 *              Be sure to send <Ctrl+Z> (0x1A) after these
 *
 * Arguments:   url - GET URL assuming servername is already known. So to access
 *                    http://servname.com/location/ set url="/location/"   
 *              param_str - [optional] complete parameter string. Example:
 *                          myparam1=test1&myparam2=test2 
 * Return:      None
 *
 * Assumptions: None
 *
 * Error Checking: None;
 *
 * Revision History:
 *   May 11, 2013      Nnoduka Eruchalu     Initial Revision
 */
void SimHttpLaunchGet(const char *url, const char *param_str)
{
  SimPutStr("GET ");                /* Get command specify protocol, and url */
  SimPutStr(protocol);              /* complete with servername */
  SimPutStr("://");
  SimPutStr(server);
  SimPutStr(url);
  if(param_str) { /* if parameter string is present then submit it */
    SerialPutChar2('?'); SimPutStr(param_str);
  }
  SimPutStrLn(" HTTP/1.1");
  
  SimPutStr("Host: ");               /* specify host */
  SimPutStrLn(server);
  
  SimPutStrLn("Content-Length: 0");  /* send content-length */
  
  SerialPutChar2(0x0D);              /* <CR>     */
  SerialPutChar2(0x0A);              /* <LF>     */
  SerialPutChar2(0x1A);              /* <Ctrl+Z> */
}


/*
 * SimHttpLaunchPost
 * Description: Launch a HTTP POST Operation
 *
 * Operation:   Send the POST, Host and Content-Length commands. Sample command
 *              strings:
 *                 "POST http://easypay.strivinglink.com/test/post/ HTTP/1.1"
 *                 "Host: easypay.strivinglink.com"
 *                 "Content-Type: application/x-www-form-urlencoded"
 *                 "Content-Length: 29"
 *                 "" [empty line of course still has <CR> and <LF>]
 *                 "myparam1=test1&myparam2=test2"
 *                 <Ctrl+Z>
 *
 * Arguments:   url - GET URL assuming servername is already known. So to access
 *                    http://servname.com/location/ set url="/location/" 
 *              param_str - [required] complete parameter string. Example:
 *                          myparam1=test1&myparam2=test2
 * Return:      None
 *
 * Assumptions: None
 *
 * Error Checking: None;
 *
 * Revision History:
 *   May 13, 2013      Nnoduka Eruchalu     Initial Revision
 */
void SimHttpLaunchPost(const char *url, const char *param_str)
{
  char contentlength[6];            /* buffer to save param string length */
  
  if(!param_str) return;            /* parameter string required for post */

  SimPutStr("POST ");               /* Get command specify protocol, and url */
  SimPutStr(protocol);              /* complete with servername */
  SimPutStr("://");
  SimPutStr(server);
  SimPutStr(url);
  SimPutStrLn(" HTTP/1.1");
  
  SimPutStr("Host: ");               /* specify host */
  SimPutStrLn(server);
  
  /* send content-type */  
  SimPutStrLn("Content-Type: application/x-www-form-urlencoded"); 

  sprintf(contentlength, "%u", strlen(param_str)); /* determine and       */
  SimPutStr("Content-Length: ");                   /* send content-length */
  SimPutStrLn(contentlength);  
  
  SerialPutChar2(0x0D);              /* <CR>     */
  SerialPutChar2(0x0A);              /* <LF>     */
  
  SimPutStrLn(param_str);            /* send parameter string */
  SerialPutChar2(0x1A);              /* <Ctrl+Z> */
}


/*
 * SimHttp
 * Description: Perform HTTP GET Operation with the following steps:
 *               - Network registration
 *               - Set APN
 *               - Launch HTTP Operation (start connection)
 *               - Launch HTTP GET/POST Operation
 *               - Parse HTTP Response
 *
 * Operation:   First ensure Network Registration is done
 *              Keep trying to get network registration up to max number of
 *              tries. If trial count maxes out, reset the module and try again.
 *              continue this reset cycle up to max allowed, and if still no
 *              network registration, return FAIL;
 *
 *              After network registration, launch http operation. If return
 *              is what's expected then launch http get operation, and wait for
 *              return value. If it's valid proceed to launch http get, else
 *              end this function and return FAIL.
 *
 * Arguments:   method - SIM_HTTP_GET/SIM_HTTP_POST
 *              url - GET/POST URL assuming servername is already known. So to
 *                    access http://servname.com/location/ set url="/location/"
 *              param_str - complete parameter string
 *                          Example: myparam1=test1&myparam2=test2
 *              http_response - pointer to structure to save http response data
 * Return:      SUCCESS/FAIL
 *
 * Assumptions: None
 *
 * Error Checking: Check for appropriate response 
 *
 * Revision History:
 *  May 12, 2013      Nnoduka Eruchalu     Initial Revision
 *  May 13, 2013      Nnoduka Eruchalu     Changed from SimHttpGet -> SimHttp
 */
int SimHttp(uint8_t method, const char *url, const char *param_str, 
            http_data *http_response)
{
  uint8_t netreg_trials = 0;    /* # of network reg attempts since last reset */
  uint8_t reset_trials = 0;     /* number of system resets */
  uint8_t netreg_status = 0;    /* start by assuming not registered */
  int netreg_success;           /* variables for operation success/fail */
  int apn_success;
  uint8_t num_crlf = 0;         /* # of <CR><LF> recorded          */
  
  /* POST requires param_str */
  if((method == SIM_HTTP_POST) && (!param_str)) return FAIL;

  /* keep attempting connections, until # of system resets max's out */
  while (reset_trials < SIM_HTTP_RESET_TRIALS) {
    
    /* in a recet cycle, keep trying network reg up till max # of trials */
    while(netreg_trials < SIM_HTTP_NETREG_TRIALS) {
      netreg_success = SimNetworkReg(&netreg_status);/* attempt network reg */
      netreg_trials++;                               /* and record attempt  */
      
      if((netreg_success == SUCCESS) && (netreg_status == 1))
        break;                                       /* if successful, break */
    }
    
    /* get here from either a successful network registration attempt or 
     * it's simply time for a system reset.
     */
    if((netreg_success == SUCCESS) && (netreg_status == 1))
      break;
   
    SimReset();                                      /* if here, then reset */
    reset_trials++;                                  /* and record reset    */
  }
  
  /* if still an unsuccessful network registration, return fail. */
  if(!((netreg_success == SUCCESS) && (netreg_status == 1)))
    return FAIL;
  
  /* if a successful network registration, set APN */
  apn_success = SimSetApn(apn_att_ipad);
  /* fail operation if APN setting is uncessful */
  if(apn_success == FAIL) return FAIL;
 
  /* if apn set successfully launch http operation */
  SimHttpLaunch();
  
  /* and don't go anywhere till a response is received
   * A valid response is:   +CHTTPACT: REQUEST, and
   * invalid responses are: +CHTTPACT: 22[0-7]
   *                        network error
   *                        and many others
   * A response has been received when <CR><LF> characters have appeared twice
   */
  rxCount = 0;
  do {
    rxBuf[rxCount++] = SerialGetChar2();   /* get char and INC rxBuf index */
    if((rxCount>=2) && (rxBuf[rxCount-2]=='\r') && (rxBuf[rxCount-1]=='\n'))
      num_crlf++;                          /* record <CR><LF> cluster count */
    
  }while(num_crlf < 2);
  
  /* if http launch was unsuccessful then return a failed operation */
  if(!((rxBuf[rxCount-4] == 'S') && (rxBuf[rxCount-3] == 'T')))
    return FAIL;
  
  /* a successful http launch, so go finally execute http get/post */
  if(method == SIM_HTTP_GET) SimHttpLaunchGet(url, param_str);
  else                       SimHttpLaunchPost(url, param_str);
  
  return SimHttpParseResponse(http_response);
}


/*
 * SimHttpGet
 * Description: Perform HTTP GET Operation.
 *
 * Operation:   Call SimHttp with appropriate parameters
 *
 * Arguments:   url - GET URL assuming servername is already known. So to access
 *                    http://servname.com/location/ set url="/location/"
 *              param_str - complete parameter string
 *                          Example: myparam1=test1&myparam2=test2
 *              http_response - pointer to structure to save http response data
 *
 * Return:      SUCCESS/FAIL
 *
 * Assumptions: None
 *
 * Error Checking: None
 *
 * Revision History:
 *  May 13, 2013      Nnoduka Eruchalu     Initial Revision
 */
int SimHttpGet(const char *url, const char *param_str, 
               http_data *http_response) {
  return SimHttp(SIM_HTTP_GET, url, param_str, http_response);
}


/*
 * SimHttpPost
 * Description: Perform HTTP POST Operation.
 *
 * Operation:   Call SimHttp with appropriate parameters
 *
 * Arguments:   url - GET URL assuming servername is already known. So to access
 *                    http://servname.com/location/ set url="/location/"
 *              param_str - complete parameter string
 *                          Example: myparam1=test1&myparam2=test2
 *              http_response - pointer to structure to save http response data
 *
 * Return:      SUCCESS/FAIL
 *
 * Assumptions: None
 *
 * Error Checking: None
 *
 * Revision History:
 *  May 13, 2013      Nnoduka Eruchalu     Initial Revision
 */
int SimHttpPost(const char *url, const char *param_str, 
               http_data *http_response) {
  return SimHttp(SIM_HTTP_POST, url, param_str, http_response);
}



/*
 * SimHttpParseResponse
 * Description: Parse HTTP Response (with json body)
 *
 * Operation:   Format of Http response is:
 *              -> With Content-length (so transfer-encoding: identity)
--------------------- screenshot 1 from hyperterminal --------------------------
AT+CHTTPACT="easypay.strivinglink.com",80

+CHTTPACT: REQUEST
GET http://easypay.strivinglink.com/test/json/ HTTP/1.1
Host: easypay.strivinglink.com
Content-Length: 0


OK

+CHTTPACT: DATA,192
http/1.1 200 ok
server: nginx
date: sat, 11 may 2013 20:02:36 gmt
content-type: application/json
vary: accept-encoding
content-length: 46

{"txt": "json hello world", "from": "nnoduka"}
+CHTTPACT: 0

--------------------------------------------------------------------------------
 *              
 *              -> Without content-length (so transfer-encoding: chunked)
--------------------- screenshot 2 from hyperterminal --------------------------
AT+CHTTPACT="easypay.strivinglink.com",80

+CHTTPACT: REQUEST
GET http://easypay.strivinglink.com/test/json/ HTTP/1.1
Host: easypay.strivinglink.com
Content-Length: 0


OK

+CHTTPACT: DATA,206
http/1.1 200 ok
server: nginx
date: sun, 12 may 2013 06:22:10 gmt
content-type: application/json
vary: accept-encoding
transfer-encoding: chunked

2E
{"txt": "json hello world", "from": "nnoduka"}

+CHTTPACT: DATA,5
0


+CHTTPACT: 0

--------------------------------------------------------------------------------
 *
 *              Notice that after the SIM5218 module echoes back the Commands
 *              we sent it acknowledges with <CR><LF>OK<CR><LF>
 *              Yes there are two spaces between Content-Length and OK, but
 *              that's because when sending the command, there was a <CR><LF> at
 *              the end before <Ctrl+Z>
 *              The HTTP response header block "+CHTTP... " also starts with
 *              <CR><LF> and each line ends with <CR><LF>
 *              So rewriting the response with hidden characters:
 *                <CR><LF>OK<CR><LF>
 *                <CR><LF>+CHTTPACT: DATA...<CR><LF>
 *                http/1.1 200 ok<CR><LF>
 *                server: nginx<CR><LF
 *                other header lines<CR><LF>
 *                <CR><LF>2E<CR><LF>
 *                {"txt": "json hello world", "from": "nnoduka"}<CR><LF>
 *                ...
 *              
 *              So to determine if there is cause to proceed further while
 *              reading bytes from serial channel, first check for "OK", then
 *              "+CHTTPACT: DATA" and "http/1.1 200 ok". If all these check
 *              out, grab the json data which is demarcated by curly braces.
 *
 *
 * Arguments:   http_response - pointer to structure to save http response data
 * Return:      SUCCESS/FAIL
 *
 * Assumptions: - Http Response body contains single-level json data.
 *              - Called right after launching http get/post.
 *
 * Error Checking: None
 * Todo:           Extend to more than just json 
 *
 *
 * Revision History:
 *   May 12, 2013      Nnoduka Eruchalu     Initial Revision
 */
int SimHttpParseResponse(http_data *http_response)
{
  uint8_t have_body = FALSE;   /* received body? assume no */
  uint16_t start_body = 0;     /* index to where '{' is in rxBuf */
  uint16_t end_body = 0;       /* index to where '}' is in rxBuf */
  size_t i;                    /* index into body section of rxBuf */
  
  /* wait for timeout or entire content of body */
  SimStartTimer(SIM_HTTP_RESPONSE_TIME);
  do {
    rxBuf[rxCount] = SerialGetChar2();             /* get char from channel  */
    if(rxBuf[rxCount]=='{') start_body = rxCount;  /* get index to '{'       */
    if(rxBuf[rxCount]=='}') {                      /* get index to '}' which */
      end_body = rxCount; have_body = TRUE;        /* which marks body rx'd  */
    }
    rxCount++;                                     /* move to next buffer slot*/
  }while((have_body == FALSE) && !(timerOvertime && timer==0));
  
  /* if still don't have body, return FAIL */
  if(have_body == FALSE) {     
    return FAIL;
  } 
  
  /* if here, then have body and start and end tags, so extract content */
  ParseHttpBodyJson(start_body, end_body, http_response);
  
  return SUCCESS; /* frankly, if function got here, all is well */
}
