/*
 * -----------------------------------------------------------------------------
 * -----                            SIM5218.H                              -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the header file for sim5218.c, the library of functions for
 *   interfacing with the SIM5218A
 *
 * Assumptions:
 *   None.
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   May 07, 2013      Nnoduka Eruchalu     Initial Revision
 *   May 16, 2013      Nnoduka Eruchalu     Added number2 to http_response
 */

#ifndef SIM5218_H
#define SIM5218_H

/* library include files */
#include <stdint.h>

/* --------------------------------------
 * SIM5218 Timer Counters
 * --------------------------------------
 */
#define SIM_TIMERCOUNT         500   /* ms to wait before comm. timeout   */
#define SIM_STARTUP_TIME       10    /* startup time of module in seconds */
#define SIM_RESET_TIME         20    /* module reset time in seconds      */
#define SIM_HTTP_RESPONSE_TIME 10000 /* ms to wait for HTTP response      */


/* --------------------------------------
 * SIM5218 HTTP Trial Counters
 * --------------------------------------
 */
#define SIM_HTTP_NETREG_TRIALS 10  /* max trials for network reg during http */
#define SIM_HTTP_RESET_TRIALS   2  /* max number of reset cycles during http */


/* --------------------------------------
 * SIM5218 HTTP OPERATIONS
 * --------------------------------------
 */
#define SIM_HTTP_GET   0    /* perform a GET  */
#define SIM_HTTP_POST  1    /* perform a POST */


/* --------------------------------------
 * SIM5218 Data Objects
 * --------------------------------------
 */
typedef struct {        /* SIM5218A data */
  uint8_t imei[15];     /*IMEI is 14 digits + check digit, and device specific*/
  uint8_t imsi[15];     /*IMSI identifies the SIM-card */
} sim_data;

typedef struct {        /* HTTP response data */
  uint32_t number;      /* numeric   */
  uint32_t number2;     /* numeric 2 */
  uint8_t message[40];  /* message string */
  uint8_t boolean;      /* binary with possible values: TRUE/FALSE */
} http_data; 


/* --------------------------------------
 * SIM5218 FUNCTION PROTOTYPES
 * --------------------------------------
 */
/* --------------------------------------
 * Timer Routines
 * --------------------------------------
 */
/* start a countdown timer with a Timer */
extern void SimStartTimer(unsigned int ms);

/* interrupt service routine for a Timer */
extern void SimTimerISR(void);


/* --------------------------------------
 * Memory Management Functions
 * --------------------------------------
 */
/* Initialize the sim5218 data representation */
extern void SimDataInit(sim_data *module);


/* --------------------------------------
 * Communication Functions
 * --------------------------------------
 */
/* output a command string to the serial channel */
extern void SimPutStr(const char *cmdstr);

/* output a command string to the serial channel with a new line */
extern void SimPutStrLn(const char *cmdstr);

/* get a buffer of bytes from the serial channel */
extern void SimGetBuf(void);

/* for debugging */
extern void SimPrintBuf(void);


/* --------------------------------------
 * AT Commands
 * --------------------------------------
 */
/* Reset Module */
extern int SimReset(void);

/* Get Network registration status */
extern int SimNetworkReg(uint8_t *status);

/* Set IP APN */
extern int SimSetApn(const char *apn);

/* Launch a HTTP Operation */
extern void SimHttpLaunch(void);

/* Launch a HTTP GET Operation */
extern void SimHttpLaunchGet(const char *url, const char *param_str);

/* Launch a HTTP POST Operation */
extern void SimHttpLaunchPost(const char *url, const char *param_str);

/* Perform a HTTP Operation */
extern int SimHttp(uint8_t method, const char *url, const char *param_str, 
                   http_data *http_response);

/* Peform a HTTP GET Operation */
extern int SimHttpGet(const char *url, const char *param_str, 
                      http_data *http_response);

/* Perform a HTTP POST Operation */
extern int SimHttpPost(const char *url, const char *param_str, 
                       http_data *http_response);

/* Parse HTTP Response */
extern int SimHttpParseResponse(http_data *http_response);


#endif                                                           /* SIM5218_H */
