/*
 * -----------------------------------------------------------------------------
 * -----                             MIFARE.C                              -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is a library of functions for interfacing the PIC18F67K22 with the
 *   SL032, a MIFARE DESFire reader/writer module.
 *
 * Table of Contents:
 *   le16toh                 - save 16-bit little-endian data block to host
 *   le24toh                 - save 24-bit little-endian data block to host
 *   le32toh                 - save 32-bit little-endian data block to host
 *   MifareStartTimer        - start a countdown timer with a Timer
 *   MifareTimerISR          - Timer interrupt service routine.
 *   MifarePutBuf            - output a buffer of bytes to the serial channel
 *   MifareGetBuf            - get a buffer of bytes to the serial channel
 *   MifareCommTCL           - communicate via T=CL; send command, get response
 *   MifareTagInit           - initialize a MIFARE DESFire tag 
 *   MifareDetect            - detect a card within range
 *   MifareConnect           - establish connection to the provided tag.
 *   MifareDisconnect        - terminate connection with the provided tag
 *
 *   MifareAuthenticate      - perform legacy authentication
 *   MifareAuthenticateIso
 *   MifareAuthenticateAes
 *   MifareChangeKeySettings - change master key config settings
 *   MifareSetConfiguration  - configures and pre-personalizes PICC with a key
 *   MifareSetDefaultKey     -
 *   MifareSetAts            -
 *   MifareGetKeySettings    - get master key config settings and max # of keys
 *   MifareChangeKey         - change any key stored on the PICC
 *   MifareGetKeyVersion     - retrieve version information for a given key
 *
 *   MifareCreateApplication - create new application on the PICC
 *   MifareCreateApplicationIso
 *   MifareCreateApplication3k3Des
 *   MifareCreateApplication3k3DesIso
 *   MifareCreateApplicationAes
 *   MifareCreateApplicationAesIso
 *   MifareDeleteApplication - delete application on the PICC
 *   MifareGetApplicationIds - gets the AIDS of all active applications on PICC
 *   MifareGetFreeMem        - returns the free memory available on the card
 *   MifareGetDfNames        - returns the DF names
 *   MifareSelectApplication - select a specific application for further access
 *   MifareFormatPicc        - release the PICC user memory
 *   MifareGetVersion        - return manufacturing related data of the PICC
 *   MifareGetCardUid        - returns the UID
 *
 *   MifareGetFileIds        - return File IDentifieres of active files in app.
 *   MifareGetIsoFileIds     - return Iso FIDs of active files in current app.
 *   MifareGetFileSettings   - get info on the properties of a specific file.
 *   MifareChangeFileSettings- change the access params of an existing file
 *   CreateFileData          - helper func. for creating std/backup data files
 *   MifareCreateStdDataFile - create standard data file
 *   MifareCreateStdDataFileIso
 *   MifareCreateBackupDataFile - create backup data file
 *   MifareCreateBackupDataFileIso
 *   MifareCreateValueFile   - create value file
 *   CreateFileRecord        - helper for creating linear/cyclic record files
 *   MifareCreateLinearRecordFile - create linear record file
 *   MifareCreateLinearRecordFileIso
 *   MifareCreateCyclicRecordFile - create cyclic record file
 *   MifareCreateCyclicRecordFileIso
 *   MifareDeleteFile        - deactivate a file within directory of current app
 *
 *   GetReadCommunicationSettings- get communication mode of a file in an app 
 *   GetWriteCommunicationSettings- get communication mode of a file in an app 
 *   ReadData                - helper function to read data and record files
 *   MifareReadData          - read data (communication mode to be derived)
 *   MifareReadDataEx        - read data files with explicit communication mode
 *   WriteData               - helper function to write data and record files
 *   MifareWriteData         - write data (communication mode to be derived)
 *   MifareWriteDataEx       - write data files with explicit communication mode
 *   MifareGetValue          -read value file (with implicit communication mode)
 *   MifareGetValueEx        - read value file with explicit communication mode
 *   MifareCredit            -increase value in value file (extracted comm_mode)
 *   MifareCreditEx          - increase value in value file (explicit comm_mode)
 *   MifareDebit             -decrease value in value file (extracted comm_mode)
 *   MifareDebitEx           - decrease value in value file (explicit comm_mode)
 *   MifareLimitedCredit     -limited increase in value file, implicit comm_mode
 *   MifareLimitedCreditEx   -limited increase in value file, explicit comm_mode
 *   MifareWriteRecord       - write data to a record in a record file
 *   MifareWriteRecordEx     - write data to a record (explicit comm_mode)
 *   MifareReadRecords       - read a set of complete records from record file
 *   MifareReadRecordsEx     -read records from record file (explicit comm_mode)
 *   MifareClearRecordFile   - reset a record file to the empty state
 *   MifareCommitTransaction -validate all previous write access on files in app
 *   MifareAbortTransaction  - invalidate all previous write access on files
 *  
 * Limitations:
 *   Using uint32_t for AID instead of uint24_t, because uint24_t is 
 *   non-standard
 *
 * Documentation Sources:
 *   - libfreefare
 *
 *   - Draft ISO/IEC FCD 14443-4
 *     Identification cards
 *     + Contactless integrated circuit(s) cards
 *      + Proximity cards
 *        + Part 4: Transmission protocol
 *     Final Committee Draft - 2000-03-10
 *
 * -http://ridrix.wordpress.com/2009/09/19/mifare-desfire-communication-example/
 * - SL032 user manual
 * - DESFire datasheet
 *
 * Assumptions:
 *   - Data Received from PICC ends with status byte
 *     + The SL032 actually returns the data with status byte ahead of data, so
 *       it should be copied to be after the data.
 *   - When CryptoPreprocessing data, can use an offset of 0 if will be using
 *     MDCM_PLAIN communication mode.
 *   - the SL032's select command buffer fits into Serial's Tx Queue.
 * 
 * Todo:
 *   Use MifareCommTCL returned error status
 * 
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Dec. 28, 2012      Nnoduka Eruchalu     Initial Revision
 *   May  03, 2013      Nnoduka Eruchalu     Removed dynamic memory allocation
 *                                           So changed functions to accept
 *                                           pointers to pre-allocated data
 *   May  04, 2013      Nnoduka Eruchalu     Added AuthenticateIso and 
 *                                           AuthenticateAes
 *  May  06, 2013      Nnoduka Eruchalu     Added mifare_tag_simple struct and
 *                                          used for MifareDetect functions.
 */

#include <string.h>   /* for mem* operations */
#include <stdlib.h>
#include "mifare.h"
#include "mifare_crypto.h"
#include "mifare_key.h"
#include "../serial.h"
#include "rand.h"


/* functions local to this file */
static uint16_t le16toh(uint8_t data[/*2*/]);
static uint32_t le24toh(uint8_t data[/*3*/]);
static uint32_t le32toh(uint8_t data[/*4*/]);

static int Authenticate(mifare_tag *tag, uint8_t cmd, uint8_t key_no,
                        mifare_desfire_key *key);

static int CreateApplication(mifare_tag *tag, uint32_t aid,
                             uint8_t settings1, uint8_t settings2,
                             uint8_t want_iso_application,
                             uint8_t want_iso_file_identifiers,
                             uint16_t iso_file_id, uint8_t *iso_file_name,
                             size_t iso_file_name_len);

static int CreateFileData(mifare_tag *tag, uint8_t command, uint8_t file_no,
                          uint8_t has_iso_file_id, uint16_t iso_file_id, 
                          uint8_t communication_settings, 
                          uint16_t access_rights, uint32_t file_size);
static int CreateFileRecord(mifare_tag *tag, uint8_t command, uint8_t file_no,
                            uint8_t has_iso_file_id, uint16_t iso_file_id,
                            uint8_t communication_settings, 
                            uint16_t access_rights, uint32_t record_size, 
                            uint32_t max_number_of_records);

static uint8_t GetReadCommunicationSettings(mifare_tag *tag, uint8_t file_no);
static uint8_t GetWriteCommunicationSettings(mifare_tag *tag, uint8_t file_no);

static int ReadData(mifare_tag *tag,  uint8_t command, uint8_t file_no, 
                    uint32_t offset, uint32_t length, uint8_t data[],
                    uint32_t max_count, ssize_t *data_size,
                    uint8_t communication_mode);
static int WriteData(mifare_tag *tag,  uint8_t command, uint8_t file_no, 
                     uint32_t offset, uint32_t length, uint8_t *data, 
                     size_t *data_sent, uint8_t communication_mode);


/* shared variables have to be local to this file */
static unsigned int timer;                    /* serial comm. ms time counter */
static unsigned char timerOvertime;           /* serial comm. timeout flag */
static unsigned char uartStatus;              /* SL032 uart channel status */
static unsigned char rxBuf[MAX_FRAME_SIZE+5]; /* serial channel Rx buffer */
                                              /* +5 for SL032 comm. bytes */

static uint8_t SelectCard[3] = {0xBA, 0x02, SL_SELECT_CARD};/* SL032 commands */
static uint8_t RATSDesfire[3]= {0xBA, 0x02, SL_RATS};


/* SL032 specific defines */
#define SL032_RXCMD  rxBuf[2]    /* SL032 Rx Command is 3rd Uart Rx byte */
#define SL032_RXSTA  rxBuf[3]    /* SL032 Rx Status is 4th Uart Rx byte */

/* SL032 defines during T=CL exchange */
#define MF_RXSTA     rxBuf[4]    /* DESFire Rx Status: 1st Data byte */
#define MF_RXDATA    (&rxBuf[5]) /* DESFire Rx Data array (excluding status) */
#define MF_RXLEN     rxBuf[1]-3  /* Len of DESFire Rx data (including status) */
                                 /* subtract 0x21, Status, Checksum from SL032*/


/* save 16-bit little endian data to host memory 
 * argument is pointer to memory block
 */
static uint16_t le16toh(uint8_t data[/*2*/])
{
  return ((uint16_t) data[0] | ((uint16_t) data[1] << 8));
}

/* save 24-bit little endian data to host memory
 * argument is pointer to memory block
 */
static uint32_t le24toh(uint8_t data[/*3*/])
{
  return ((uint32_t) data[0] | ((uint32_t) data[1] << 8) | 
          ((uint32_t) data[2] << 16));
}

/* save 32-bit little endian data to host memory
 * argument is pointer to memory block
 */
static uint32_t le32toh(uint8_t data[/*4*/])
{
  return ((uint32_t) data[0] | ((uint32_t) data[1] << 8) | 
          ((uint32_t) data[2] << 16)  | ((uint32_t) data[3] << 24));
}


/*
 * MifareStartTimer
 * Description:
 *  Start a countdown timer with a Timer. Countdown for passed in milliseconds.
 *
 * Operation:
 *  Start timer countdown to passed in argument, and clear out overtime flag,
 *  as timer hasn't timed out yet.
 *
 * Limitations:
 *  Critical code on timerOvertime
 *
 * Revision History:
 *  Dec. 30, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareStartTimer(unsigned int ms)
{
  timer = ms;             /* Start timer */
  timerOvertime = FALSE;  /* timer hasn't timed out yet */
}


/*
 * MifareTimerISR
 * Description:
 *  Interrupt Service Routine for a Timer.
 *
 * Operation:
 *  on each ISR routine call, decrement the timer. When it hits 0, the timer
 *  has hit timed out/hit overtime, so set the overtime flag.
 * 
 * Limitations:
 *  Critical code on timerOvertime
 *
 * Revision History:
 *  Dec. 30, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareTimerISR(void)
{
  if(timer > 0) {                          /* if timer hasn't timed out */
    timer--;                               /* decrement it; set the overtime */
    if(timer == 0) timerOvertime = TRUE;   /* flag when it does time out */
  }
}


/*
 * MifarePutBuf (through the SL032)
 * Description:
 *  This function outputs the passed buffer to the serial channel, byte by byte.
 *  The bytewise outputting is done using SerialPutChar.
 *
 * Arguments:
 *  - command array, each slot representing a byte as described in Communication
 *  Format below. This array however shouldnt contain Checksum byte. This will
 *  be computed on the fly.
 *  - command array size
 *
 * Communication Format (MCU to SL032):
 *  |----------|-----|---------|------|----------|
 *  | Preamble | Len | Command | Data | Checksum |
 *  |----------|-----|---------|------|----------|
 *  Preamble : 1 byte equal to 0xBA
 *  Len      : 1 byte indicating the number of bytes from Command to Checksum   
 *  Command  : 1 byte SL032 Command code
 *  Data     : Variable length; depends on the command type
 *  Checksum : 1 byte XOR of all the bytes from Preamble to Data
 *
 * Operation:
 *  This function loops through the buffer and outputs the bytes using
 *  SerialPutChar. After outputting these bytes, it outputs the checksum which
 *  is  cumulative XOR of all the buffer bytes.
 *  End this by updating Uart Status to indicate a successful Tx.
 *
 * Limitations:
 *   size argument >= 3
 *
 * Revision History:
 *  Dec. 30, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifarePutBuf(unsigned char *buffer, unsigned char size)
{
  unsigned char i = 0;          /* buffer index */
  unsigned char data;           /* data byte */
  unsigned char checksum = 0;   /* SL032 checksum is XOR of all bytes */
  
  uartStatus = MF_UARTSTATUS_TX; /* about to start Tx */
  
  while(i < size) {             /* loop through buffer slots */
    data = buffer[i];           /* extract data byte */
    checksum ^= data;           /* checksum is cummulative */
    i++;                        /* keep going through the buffer */
    SerialPutChar(data);        /* output buffer contents */
  }
  SerialPutChar(checksum);      /* finally, output checksum byte */
  
  uartStatus = MF_UARTSTATUS_TXSUCC; /* a successful Tx */
}


/*
 * MifareGetBuf (from the SL032)
 * Description:
 *  This function gets data from the SL032 and saves in MifareRxBuf.
 *
 * Communication Format (SL032 to MCU):
 *  |----------|-----|---------|--------|------|----------|
 *  | Preamble | Len | Command | Status | Data | Checksum |
 *  |----------|-----|---------|--------|------|----------|
 *  Preamble : 1 byte equal to 0xBD
 *  Len      : 1 byte indicating the number of bytes from Command to Checksum   
 *  Command  : 1 byte SL032 Command code
 *  Status   : 1 byte SL032 Status code
 *  Data     : Variable length; depends on the command type
 *  Checksum : 1 byte XOR of all the bytes from Preamble to Data
 *
 * Operation:
 *  First assume success on this Rx Sequence.
 *
 *  Keep track of the number of received bytes using the 0-indexed rxCount.
 *  This will double as index into the rx buffer, and so always 1 less than
 *  actual number of rx'd bytes.
 *
 *  Next is to get into a loop and stay in that loop while we have received less
 *  than 2 bytes, or more than 2 bytes while still less than the total number of
 *  bytes (Len + 2), where "Len" is as described above.
 *  In each iteration, set timer and wait for a timeout or a serial byte. If
 *  there is a timeout record an rx error and break from the loop.
 *  If there isnt a timeout, grab a byte and compute cummulative checksum.
 *
 *  When evaluating the checksum, instead of XORing only from Preamble
 *  to Data, its a lot easier to XOR all bytes, and if the values are right, the
 *  result should be 0. This is because XORing a value with itselt yields 0.
 *
 *  Note that checking for overtime is about more than just checking the flag.
 *  There is critical code surrounding this flag, so also check the timer
 *  counter is 0.
 *
 *  When done receiving, zero out the unused rx buffer slots.
 *  checking and returns with a SUCCESS if no errors, and a FAIL otherwise.
 *
 * Error Checking:
 *   + UART Rx is successful
 *   + SL032 Rx'd Command is the SL032 Tx'd Command
 *   + SL032 Rx'd Status is "Operation Success"
 *   + Desfire Rx'd Status is "Operation OK"
 *
 * Revision History:
 *  Dec. 30, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareGetBuf(void)
{
  unsigned int rxCount = 0;   /* 0-index'd number of bytes received */
  unsigned char checkSum = 0; /* cummulative checksum */
  
  uartStatus = MF_UARTSTATUS_RXSUCC;  /* assume success for rx status */
  
  /* rxCount is either < 2 and not a problem, or >= 2 and still < the length 
   * received in the second byte (+2 to account for preamble and len bytes). 
   * Note that rxCount is a 0-based index into rxBuf
   */
  while((rxCount < 2) || ((rxCount >= 2) && (rxCount < rxBuf[1]+2))) {
    
    MifareStartTimer(MIFARE_TIMERCOUNT); /*set timer; wait for timeout or byte*/
    while(!SerialInRdy() && !(timerOvertime && timer == 0)); /* if a timeout  */
    if(!SerialInRdy()) uartStatus = MF_UARTSTATUS_RXERR;     /* record error  */
        
    if(uartStatus != MF_UARTSTATUS_RXERR) {     /* if no errors grab bytes */
      rxBuf[rxCount] = SerialGetChar();         /* from serial channel */
      checkSum ^= rxBuf[rxCount];               /* perform cummulative XOR */  
      rxCount++;
    } else {                                    /*if there is an error, break */
      break;
    }
  } 
  
  /* if there is a checkSum error, there is an Rx Error */
  if(checkSum != 0) uartStatus = MF_UARTSTATUS_RXERR;
  
  /* clear the rest of the Rx Buffer */
  while(rxCount < MAX_FRAME_SIZE) {
    rxBuf[rxCount] = 0;                         /* zero out buffer slot and */
    rxCount++;                                  /* move on to the next slot */
  }
}


/*
 * MifareCommTCL (with the SL032)
 * Description:
 *  This function exchanges transparent data via T=CL; sends command, and
 *  gets a response.
 *
 * Arguments:
 *  - data array, each slot representing a 'Data' byte as described in 
 *  Communication Format below.
 *  - data array size
 *
 * Shared Variables:
 *  rxBuf: update to have the PICC status byte appear after other data.
 *         note this works, by replacing SL032 checksum with PICC status
 *
 * Communication Format (2):
 *  |------|-----|------|------|----------|
 *  | 0xBA | Len | 0x21 | Data | Checksum |
 *  |------|-----|------|------|----------|
 *  Len      : 1 byte indicating the number of bytes from Command to Checksum
 *             Will be computed by function
 *  Data     : Function argument. Variable length; depends on the command type
 *  Checksum : 1 byte XOR of all the bytes from Preamble to Data.
 *             Will be computed by function
 *
 * Return:
 *  SUCCESS: if communication was successful
 *  FAIL:    if communication failed.
 *
 * Operation:
 *  This function creates a new command buffer of length (size+3), to prepend
 *  0xBA, Len, and 0x21.
 *  In this case, Len = size + 2 (to account for 0x21 and Checksum)
 *  This then calls MifarePutBuf with this new command buffer.
 *  Then follows up with MifareGetBuf and some error checking.
 *
 *  At the end of this the MF_RXDATA is of the format
 *    +-----------------------+---------+
 *    |    PICC Payload       | Status  |
 *    +-----------------------+---------+
 *    PICC Status comes after the payload and is at MF_RXDATA[MF_RXLEN-1]
 *
 * Error Checking:
 *  - uart rx is successful
 *  - rx'd sl032 command is "Exchange Transparent Data (T=CL)"
 *  - rx'd sl032 status is "operation success"
 *  - rx'd DESFire status is "Operation OK" or "Additional Frame"
 *
 * Revision History:
 *  Jan. 2, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareCommTCL(unsigned char *buffer, unsigned char size)
{
  uint8_t success = FAIL;                   /* communication success status */
  unsigned int i;                           /* index into command buffer */
  /* new command buffer needs to be at least size+3 */
  unsigned char comm[MAX_FRAME_SIZE+3];
  /* setup with SL032 commands */
  comm[0] = 0xBA;                           /* SL032 Preamble */
  comm[1] = size+2;                         /* Len: include 0x21 & checksum */
  comm[2] = SL_TCL;                         /* SL032 T=CL command code */
  
  for (i = 0; i < size; i++) {              /* copy data into command buffer*/
    comm[i+3] = buffer[i];                  /* remembering command buffer */
                                            /* has 3 pre-appended bytes */
    
    MifarePutBuf(comm, size+3);             /* send T = CL command */
    MifareGetBuf();                         /* hopefully get feedback */
    
    /* error checking */
    if((uartStatus == MF_UARTSTATUS_RXSUCC) && (SL032_RXCMD == SL_TCL) &&
       (SL032_RXSTA == SL_OPERATION_SUCC)  && 
       ((MF_RXSTA == MF_OPERATION_OK) || (MF_RXSTA == MF_ADDITIONAL_FRAME))) {
      success = SUCCESS;                    /* no communication error */
    }
  }
  
  MF_RXDATA[MF_RXLEN-1] = MF_RXSTA;  /* place DESFire Rx Status after Rx data */
                                     /* using spot that was for SL032 checksum*/
    
  return success;                           /* and return comm. status */
}


/*
 * MifareTagInit
 * Description:
 *  Initialize a MIFARE DESFire tag
 *
 * Operation:
 *  reset last picc/pcd errors to a no error state (OPERATION_OK)
 *
 * Arguments: PICC
 * Return:    None
 *
 * Revision History:
 *  Dec. 28, 2012      Nnoduka Eruchalu     Initial Revision
 *  May  04, 2013      Nnoduka Eruchalu     Changed: MifareTagNew->MifareTagInit
 */
void MifareTagInit(mifare_tag *tag)
{
  tag->active = FALSE;
  tag->last_picc_error = MF_OPERATION_OK;  /* reset picc/pcd errors to no */
  tag->last_pcd_error  = MF_OPERATION_OK;  /* error states: OPERATION_OK */
  tag->authenticated_key_no = NOT_YET_AUTHENTICATED;
  tag->selected_application = 0;
  return;
}

/*
 * MifareDetect
 * Description:
 *  Detect a card in the read-range of SL03
 *
 * Arguments: PICC
 * Return:    SUCCESS - if successfully connected to a DESFire card,
 *            FAIL    - couldn't connect to a DESFire card.
 *
 * Operation:
 *  Select a PICC if available. Multiple cards (collision) aren't detected as
 *  a card tap.
 *
 * Error Checking:
 *  When selecting card, perform the following error checks:
 *    - uart rx is successful
 *    - rx'd sl032 command is "select card"
 *    - rx'd sl032 status is "operation success"
 *
 * Revision History:
 *  May  05, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareDetect(mifare_tag_simple *tag)
{
  unsigned char selected = FALSE;          /* start by assuming no selection */
  uint8_t detect = FAIL;                        /* and failed detection  */
  uint8_t i;                                    /* index into uid arrays */
  
  MifarePutBuf(SelectCard, sizeof(SelectCard)); /* send "select card" command */
  MifareGetBuf();                               /* hopefully get feedback */
  
  if((uartStatus == MF_UARTSTATUS_RXSUCC) && (SL032_RXCMD == SL_SELECT_CARD) &&
     (SL032_RXSTA ==SL_OPERATION_SUCC)) {
    selected = TRUE;                           /* select if appropriate */
  }
  
  if(selected) {                               /* connect after selecting */
    tag->type = rxBuf[rxBuf[1]];               /* save card type */
    for(i=0; i<MIFARE_UID_BYTES; i++) {        /* save uid       */
       tag->uid[i] = rxBuf[4+i];
    }
    detect = SUCCESS;                          /* a successful detection */
  }
  
  return detect;                               /* return detection status */
}


/*
 * MifareConnect
 * Description:
 *  Establish a connection to the provided PICC, by selecting the card and
 *  sending a Request for Answer To Select.
 *
 * Operation:
 *  Check the tag is inactive; Can't connect if already active.
 *  Select a DESFire PICC if available, and connect to it if it possible.
 *  If no connection is made, then return FAIL
 *  If however a connection is made setup tag data structure and return SUCCESS.
 *    tag setup is done by activating tag, freeing and clearing the session key,
 *    clearing all errors, authenticated key no, and selected App ID.
 *
 * Error Checking:
 *  When selecting card, perform the following error checks:
 *    - uart rx is successful
 *    - rx'd sl032 command is "select card"
 *    - rx'd sl032 status is "operation success"
 *    - card type is a DESFire card
 *  When connecting to a card, perform the following error checks 
 *    - uart rx is successful
 *    - rx'd sl032 command is "Request for Answer To Select"
 *    - rx'd sl032 status is "operation success"
 *
 * Return:  
 *  SUCCESS: if successfully connected to a DESFire card,
 *  FAIL:    couldn't connect to a DESFire card.
 *
 * Revision History:
 *  Dec. 31, 2012      Nnoduka Eruchalu     Initial Revision
 */
int MifareConnect(mifare_tag *tag)
{
  unsigned char selected = FALSE;          /* start by assuming no selection */
  unsigned char connected = FALSE;         /* start by assuming no connection */
  uint8_t uid[MIFARE_UID_BYTES];           /* temp uid */
  uint8_t i;                               /* index into uid arrays */
  
  ASSERT_INACTIVE(tag);
    
  MifarePutBuf(SelectCard, sizeof(SelectCard)); /* send "select card" command */
  MifareGetBuf();                               /* hopefully get feedback */
  if((uartStatus == MF_UARTSTATUS_RXSUCC) && (SL032_RXCMD == SL_SELECT_CARD) &&
     (SL032_RXSTA ==SL_OPERATION_SUCC) && (rxBuf[rxBuf[1]] ==MIFARE_CARD_DES)) {
    selected = TRUE;                           /* select if appropriate */
  }
  
  if(selected) {                                   /* connect after selecting */
    for(i=0; i<MIFARE_UID_BYTES; i++) {            /* save uid */
       uid[i] = rxBuf[4+i];
    }
    MifarePutBuf(RATSDesfire, sizeof(RATSDesfire));/* send "RATS" command */
    MifareGetBuf();                                /* hopefully get feedback */
    if((uartStatus == MF_UARTSTATUS_RXSUCC) && (SL032_RXCMD == SL_RATS) &&
       (SL032_RXSTA == SL_OPERATION_SUCC)) {
      connected = TRUE;                            /* connect if appropriate */
    }
  }
  
  if(connected) {            /* only setup tag if successfully connected */
    MifareTagInit(tag);
    tag->active = TRUE;
    for(i=0; i<MIFARE_UID_BYTES; i++) {  /* save uid */
      tag->uid[i] = uid[i];
    }
  } else {
    return FAIL;
  }
  return SUCCESS; /* if you made it this far, it is all good */
}


/*
 * MifareDisconnect
 * Description:
 *  Terminate connection with the provided tag
 *
 * Operation:
 *  Check the tag is active; Can't disconnect if inactive.
 *  free and clear session key pointer
 *  deactivate tag.
 *
 * Return:  
 *  SUCCESS: if successfully terminated connection
 *  FAIL:    couldn't terminate connection from the tag
 *
 * Revision History:
 *  Dec. 31, 2012      Nnoduka Eruchalu     Initial Revision
 */
int MifareDisconnect(mifare_tag *tag) {
  ASSERT_ACTIVE(tag);
  
  tag->active = FALSE;         /* deactivate tag */
  
  return SUCCESS; /* if you made it this far, it is all good */
}


/*
 * Authenticate
 * Description:
 *  This is the base function for authentication
 *
 * Operation:
 *  check tag is active; can only authenticate active tag
 *  initialize init-vector by zeroing all bytes
 *  reset authenticated key, to not yet authenticated
 *  free and clear session key, because we are about to get a new one.
 *  set appropriate authentication scheme [legacy(desfire) vs new (desfire ev1)]
 *  Then follow detailed steps for each
 *  
 *  Legacy:
 *  1. PCD sends command {Authenticate, key number}
 *  2. PICC generates RndB and DES/3DES enciphers with the selected key and
 *     returns eK(RndB)
 *  3. PCD DES/3DES deciphers the received eK(RndB) to get RndB.
 *     RndB is rotated left by 8 bits (first byte is moved to end) to get RndB'.
 *     PCD then generates 8 byte random number RndA.
 *     A token dK(RndA+RndB') is generated and sent to PICC
 *     dK is DES/3DES deciphering (The decryption of the two blocks is chained
 *     using the CBC send mode).
 *  4. PICC DES/3DES enciphers token to extract RndA and rotates it left a byte
 *     to get RndA'.
 *     This is enciphered again to send back eK(RndA') to PCD.
 *  5. PCD DES/3DES deciphers received eK(RndA'), and thus gains RndA' for
 *     comparison with the PCD-internally rotated RndA'.
 *  6. If comparison is successful save auth key no, create session key, reset
 *     ivect.
 * 
 *  New:
 *  
 *
 * Return:  
 *  SUCCESS: PCD successfully authenticated with PICC
 *  FAIL:    failed authentication
 *
 * Revision History:
 *  Jan. 19, 2013      Nnoduka Eruchalu     Initial Revision
 */
static int Authenticate(mifare_tag *tag, uint8_t cmd, uint8_t key_no,
                            mifare_desfire_key *key)
{
  size_t key_length;
  uint8_t PICC_E_RndB[16];                   /* eK(RndB) from PICC */
  uint8_t PICC_RndB[16];                     /* RndB from PICC */

  uint8_t PCD_RndA[16];                      /* RndA from PCD */
  uint8_t PCD_r_RndB[16];                    /* RndB': rotated RndB */
  uint8_t token[32];                         /* for dK(RndA+RndB') */
  
  uint8_t PICC_E_r_RndA[16];                 /* eK(RndA') from PICC */
  uint8_t PICC_r_RndA[16];                   /* RndA' from PICC */
  uint8_t PCD_r_RndA[16];                    /* RndA' from PCD */
    
  BUFFER_INIT(cmd1, 2);                      /* create auth command buffer */
  BUFFER_INIT(cmd2, 33);                     /* cmd + 32 possible token bytes */
  
  ASSERT_ACTIVE(tag);                        /* can only auth active tag */
  memset(tag->ivect, 0, MAX_CRYPTO_BLOCK_SIZE); /* initialize init-vector to 0*/
  
  tag->authenticated_key_no = NOT_YET_AUTHENTICATED; /* reset auth key. */
    
  /* save the passed in authentication scheme */
  tag->authentication_scheme=(MF_AUTHENTICATE_LEGACY==cmd) ? AS_LEGACY : AS_NEW;
  
  BUFFER_APPEND(cmd1, cmd);                  /* auth command format is: */
  BUFFER_APPEND(cmd1, key_no);               /* { cmd, key_no }         */
  
  MifareCommTCL(BUFFER_ARRAY(cmd1), BUFFER_SIZE(cmd1));/* transceive auth cmd */
  key_length = MF_RXLEN-1;                             /* get key length and */
  memcpy(PICC_E_RndB, MF_RXDATA, key_length);          /* copy Enciphered RndB*/
  memcpy(PICC_RndB, PICC_E_RndB, key_length);          /* prep to decipher it */
  
  MifareCipherBlocksChained(tag, key, tag->ivect, PICC_RndB, key_length,
                            MCD_RECEIVE, MCO_DECIPHER);/* decipher to get RndB*/
  
  RAND_bytes(PCD_RndA, 16);                  /* Generate cryptographic RndA*/
  memcpy(PCD_r_RndB, PICC_RndB, key_length); /* RndB' = Rol(RndB) */
  Rol(PCD_r_RndB, key_length);
  
  memcpy(token, PCD_RndA, key_length);       /* set token = dK(RndA + RndB') */
  memcpy(token+key_length, PCD_r_RndB, key_length);
  MifareCipherBlocksChained(tag, key, tag->ivect, token, 2*key_length, MCD_SEND,
                            (MF_AUTHENTICATE_LEGACY == 
                             cmd) ? MCO_DECIPHER : MCO_ENCIPHER);
  
  BUFFER_APPEND(cmd2, 0xAF);                      /* auth command 2 format is:*/
  BUFFER_APPEND_BYTES(cmd2, token, 2*key_length); /* {0x0AF, token bytes}  */
  MifareCommTCL(BUFFER_ARRAY(cmd2), BUFFER_SIZE(cmd2)); /*transceive auth cmd2*/
  
  memcpy(PICC_E_r_RndA, MF_RXDATA, key_length);   /* read in eK(RndA') */
  memcpy(PICC_r_RndA, PICC_E_r_RndA, key_length); /* and decrypt to get RndA' */
  MifareCipherBlocksChained(tag, key, tag->ivect, PICC_r_RndA, key_length,
                            MCD_RECEIVE, MCO_DECIPHER);
  
  memcpy(PCD_r_RndA, PCD_RndA, key_length);  /* get RndA' from PCD's RndA to */
  Rol(PCD_r_RndA, key_length);               /* use for comparison with PICC's*/
  
  if (0 != memcmp(PCD_r_RndA, PICC_r_RndA, key_length)) {
    return FAIL;
  }
  
  tag->authenticated_key_no = key_no;
  MifareSessionKeyNew(&tag->session_key, PCD_RndA, PICC_RndB, key);
  memset(tag->ivect, 0, MAX_CRYPTO_BLOCK_SIZE);
  
  switch(tag->authentication_scheme) {
  case AS_LEGACY:
    break;
  case AS_NEW:
    CmacGenerateSubkeys(&tag->session_key);
    break;
  }
  
  return SUCCESS;
}


/*
 * MifareAuthenticate
 * Description:
 *  Perform DESFire legacy and new authentications
 *
 * Operation:
 *  Call Authenticae with appropriate authentication commands
 *
 * Return:  
 *  SUCCESS: PCD successfully authenticated with PICC
 *  FAIL:    failed authentication
 *
 * Revision History:
 *  Jan. 20, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareAuthenticate(mifare_tag *tag, uint8_t key_no, 
                       mifare_desfire_key *key)
{
  int status = FAIL;
  switch(key->type) {
  case T_DES:
  case T_3DES:
    status = Authenticate(tag, MF_AUTHENTICATE_LEGACY, key_no, key);
    break;
  case T_3K3DES:
    status = Authenticate(tag, MF_AUTHENTICATE_ISO, key_no, key);
    break;
  case T_AES:
    status = Authenticate(tag, MF_AUTHENTICATE_AES, key_no, key);
    break;
  }
  
  return status;
}
int MifareAuthenticateIso(mifare_tag *tag, uint8_t key_no, 
                          mifare_desfire_key *key)
{
  return Authenticate(tag, MF_AUTHENTICATE_ISO, key_no, key);
}
int MifareAuthenticateAes(mifare_tag *tag, uint8_t key_no, 
                          mifare_desfire_key *key)
{
  return Authenticate(tag, MF_AUTHENTICATE_AES, key_no, key);
}


/*
 * MifareChangeKeySettings
 * Description:
 *  Change the master key configuration settings depending on the currently
 *  selected AID
 *
 * Arguments:
 *  tag: DESFire tag
 *  settings: PICC Master Key Settings (for AID = 0x00 00 00)
 *            Appl Master Key Settings (for AID != 0x00 00 00)
 *
 * PICC Master Key Settings:
 *  Bit7 - Bit4: RFU, has to be set to 0
 *  Bit3:        configuration changeable
 *  Bit2:        PICC master key not required for create/delete
 *  Bit1:        free directory list access without PICC master key
 *  Bit0:        allow changing the PICC master key
 *
 * Application Master Key Settings:
 *  Bit7 - Bit4: ChangeKey Access Rights Bits3 - Bit0
 *  Bit3:        configuration changeable
 *  Bit2:        free create/delete without master key
 *  Bit1:        free directory list without master key
 *  Bit0:        allow change master key
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |    1         8                  |       |                                 |
 * | +----+------------------------+ |       |                                 |
 * | |0x54| deciphered key settings| ---->   |                                 |
 * | +----+------------------------+ |       |    1                            |
 * |                                 |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1                            |
 * |                                 |       | +----+                          |
 * |                                 |   <---- |0x00|status                    |
 * |                                 |       | +----+                          |
 * |                                 |       |                                 |
 * +---------------------------------+       +---------------------------------+
 * 
 *  Have to preprcess with MDCM_ENCIPHERED and ENC_COMMAND
 *  
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Jan. 20, 2013      Nnoduka Eruchalu     Initial Revision
 *  Jan. 21, 2013      Nnoduka Eruchalu     Updated Comments
 */
int MifareChangeKeySettings(mifare_tag *tag, uint8_t settings)
{
  uint8_t *p;                        /* processed and preprocessed data ptrs */
  ssize_t nbytes;
  
  /* todo: remove this CMAC_LENGTH, Preprocess doesnt change array */
  BUFFER_INIT(cmd, 9 + CMAC_LENGTH); /*cmd is {0x54, 8 bytes for key settings}*/
  BUFFER_APPEND(cmd, 0x54);         
  BUFFER_APPEND(cmd, settings);
  
  ASSERT_ACTIVE(tag);
  ASSERT_AUTHENTICATED(tag);
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd),  &BUFFER_SIZE(cmd), 1,
                                 MDCM_ENCIPHERED | ENC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, MDCM_PLAIN | 
                                  CMAC_COMMAND | CMAC_VERIFY | MAC_COMMAND | 
                                  MAC_VERIFY);
  if(!p)
    return FAIL;
  
  return SUCCESS;
}


/*
 * MifareGetKeySettings
 * Description:
 *  Get configuration information on the PICC and application master key config
 *  settings. In addition it returns the maximum number of keys which can
 *  be stored within the selected application
 *
 * Arguments
 *  tag: DESFire tag
 *  settings: updated with value returned from PICC
 *  max_keys: updated with value returned from PICC
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |                             1   |       |                                 |
 * |                          +----+ |       |                                 |
 * |                          |0x45| ---->   |                                 |
 * |                          +----+ |       |                                 |
 * |                           cmd   |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1        1           1       |
 * |                                 |       | +----+------------+----------+  |
 * |                                 |   <---- |0x00|key settings|max # keys|  |
 * |                                 |       | +----+------------+----------+  |
 * |                                 |       |  status                         |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Jan. 21, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareGetKeySettings(mifare_tag *tag, uint8_t *settings, uint8_t *max_keys)
{
  uint8_t *p;                        /* processed and preprocessed data ptrs */
  ssize_t nbytes;
  
  BUFFER_INIT(cmd, 1);
  ASSERT_ACTIVE(tag);
  
  BUFFER_APPEND(cmd, 0x45); /* cmd {0x45} */
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 1,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  if(!p)
    return FAIL;
  
  if(settings)
    *settings = p[0];
  if(max_keys)
    *max_keys = p[1] & 0x0F; /* fits in one nibble */
  
  return SUCCESS;
}


/*
 * MifareSetConfiguration
 * Description:
 *  Configures the card and pre-personalizes the card with a key, defines if
 *  the UID or the random ID is sent back during communication setup and
 *  configures the ATS string.
 *
 * Arguments
 *  tag: DESFire tag
 *  disable_format: flag
 *  enable_random_uid: flag
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |                 1    1    1     |       |                                 |
 * |               +----+----+-----+ |       |                                 |
 * |               |0x5C|0x00|flags| ---->   |                                 |
 * |               +----+----+-----+ |       |    1                            |
 * |                                 |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1                            |
 * |                                 |       | +----+                          |
 * |                                 |   <---- |0x00|status                    |
 * |                                 |       | +----+                          |
 * |                                 |       |                                 |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Notes:
 *  Read DESFIRE EV1 manual, chapter 9.4.9 for details.
 *  DO NOT USE THIS COMMAND unless you're really sure you know
 *  what you're doing!!!
 *
 * Revision History:
 *  Mar. 21, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareSetConfiguration(mifare_tag *tag, uint8_t disable_format, 
                           uint8_t enable_random_uid)
{ 
  uint8_t *p;                        /* processed and preprocessed data ptrs */
  ssize_t nbytes;
  
  BUFFER_INIT(cmd, 10);
  ASSERT_ACTIVE(tag);
  
  BUFFER_APPEND (cmd, 0x5C);         /* create the info block containing the */
  BUFFER_APPEND (cmd, 0x00);         /* command code and key number argument */
  BUFFER_APPEND (cmd, 
                 (enable_random_uid ? 0x02 : 0x00) | 
                 (disable_format ? 0x01 : 0x00));
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 2,
                                 MDCM_ENCIPHERED | ENC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  
  if(!p)
    return FAIL;
  
  return SUCCESS;
}


/*
 * MifareSetDefaultKey
 * Description:
 *  SetConfiguration Command with different parameters
 *
 * Arguments
 *  tag: DESFire tag
 *  key: DESFire key
 *
 * Operation:
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 21, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareSetDefaultKey(mifare_tag *tag, mifare_desfire_key *key)
{
  uint8_t *p;                        /* processed and preprocessed data ptrs */
  ssize_t nbytes;
  size_t key_data_length;
  
  BUFFER_INIT(cmd, 34);
  ASSERT_ACTIVE(tag);
  
  BUFFER_APPEND (cmd, 0x5C);         /* create the info block containing the */
  BUFFER_APPEND (cmd, 0x01);         /* command code and key number argument */
  
  switch(key->type) {
  case T_DES:
  case T_3DES:
  case T_AES:
    key_data_length = 16;
    break;
  case T_3K3DES:
    key_data_length = 24;
    break;
  }
  BUFFER_APPEND_BYTES (cmd, key->data, key_data_length);
  while (BUFFER_SIZE(cmd) < 26)
    BUFFER_APPEND(cmd, 0x00);
  BUFFER_APPEND (cmd, MifareKeyGetVersion(key));
    
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 2,
                                 MDCM_ENCIPHERED | ENC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  
  if(!p)
    return FAIL;
  
  return SUCCESS;
}


/*
 * MifareSetAts
 * Description:
 *  SetConfiguration Command with different parameters
 *
 * Arguments
 *  tag: DESFire tag
 *  ats: ats parameter array [first byte contains length]
 *
 * Operation:
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 21, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareSetAts(mifare_tag *tag, uint8_t *ats)
{
  uint8_t *p;                        /* processed and preprocessed data ptrs */
  ssize_t nbytes;
  
  BUFFER_INIT(cmd, 34);
  ASSERT_ACTIVE(tag);
  
  BUFFER_APPEND (cmd, 0x5C);         /* create the info block containing the */
  BUFFER_APPEND (cmd, 0x02);         /* command code and key number argument */
  BUFFER_APPEND_BYTES (cmd, ats, *ats); 
  
  switch(tag->authentication_scheme) {
  case AS_LEGACY:
    MifareCrc16Append(BUFFER_ARRAY(cmd) + 2, BUFFER_SIZE(cmd) - 2);
    BUFFER_SIZE(cmd) += 2;
    break;
  case AS_NEW:
    MifareCrc32Append(BUFFER_ARRAY(cmd), BUFFER_SIZE(cmd));
    BUFFER_SIZE(cmd) += 4;
    break;
  }
  BUFFER_APPEND(cmd, 0x80);
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 2,
                                 MDCM_ENCIPHERED | NO_CRC | ENC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  
  if(!p)
    return FAIL;
  
  return SUCCESS; 
}


/*
 * MifareChangeKey
 * Description:
 *  change any key stored on the PICC.
 *  If AID=0x00 the change applies to the PICC master key, therefore only
 *  key_no=0x00 is valid. In all other cases (AID != 0x00) the change applies
 *  to the specified key_no within the current selected application.
 *
 * Arguments
 *  tag: DESFire tag
 *  key_no: key to change (0 to # of application keys -1)
 *  new_key: ptr to new key
 *  old_key: ptr to old key
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |    1    1          24           |       |                                 |
 * | +----+-------------------------+|       |                                 |
 * | |0xC4|key #|deciphered key data|---->   |                                 |
 * | +----+-------------------------+|       |    1                            |
 * |  cmd                            |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1                            |
 * |                                 |       | +----+                          |
 * |                                 |   <---- |0x00|status                    |
 * |                                 |       | +----+                          |
 * |                                 |       |                                 |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Jan. 21, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareChangeKey(mifare_tag *tag, uint8_t key_no, 
                    mifare_desfire_key *new_key, mifare_desfire_key *old_key)
{
  BUFFER_INIT(cmd,42); /* 1 cmd byte, 1 key_no byte, 24 byte key for AES, */
                       /* 4 byte CRC appended twice, 8 byte padding = 42  */
  int new_key_length;
  int n;
  uint8_t *p;
  ssize_t nbytes;
  
  
  ASSERT_ACTIVE(tag);
  ASSERT_AUTHENTICATED(tag);
    
  key_no &= 0x0F; /* clear unused bits */
  /* 
   * because new crypto methods can be setup only at application creation,
   * changing the card master key to one of them requires a key_no tweak.
   */
  if (0x000000 == tag->selected_application) {
    switch (new_key->type) {
    case T_DES:
    case T_3DES:
      break;
    case T_3K3DES:
      key_no |= 0x40;
      break;
    case T_AES:
      key_no |= 0x80;
      break;
    }
  }
  
  BUFFER_APPEND(cmd, 0xC4);
  BUFFER_APPEND(cmd, key_no);
  
  switch (new_key->type) {
  case T_DES:
  case T_3DES:
  case T_AES:
    new_key_length = 16;
    break;
  case T_3K3DES:
    new_key_length = 24;
    break;
  }
   
  memcpy(BUFFER_ARRAY(cmd) + BUFFER_SIZE(cmd), new_key->data, new_key_length);
   /* if key_no used for auth is diffrent from the key_no to be changed */
  if ((tag->authenticated_key_no & 0x0f) != (key_no & 0x0f)) {
    if (old_key) {
      for (n = 0; n < new_key_length; n++) {
        BUFFER_ARRAY(cmd)[BUFFER_SIZE(cmd) + n] ^= old_key->data[n];
      }
    }
  }
  
  BUFFER_SIZE(cmd) += new_key_length;
  
  if(new_key->type == T_AES)
    BUFFER_ARRAY(cmd)[BUFFER_SIZE(cmd)++] = new_key->aes_version;
  
  if ((tag->authenticated_key_no & 0x0f) != (key_no & 0x0f)) { /* not auth key*/
    switch (tag->authentication_scheme) {
    case AS_LEGACY:
      MifareCrc16Append(BUFFER_ARRAY(cmd) + 2, BUFFER_SIZE(cmd) - 2);
      BUFFER_SIZE(cmd) += 2;
      MifareCrc16(new_key->data, new_key_length,
                  BUFFER_ARRAY(cmd) + BUFFER_SIZE(cmd));
      BUFFER_SIZE(cmd) += 2;
      break;
    case AS_NEW:
      MifareCrc32Append (BUFFER_ARRAY(cmd), BUFFER_SIZE(cmd));
      BUFFER_SIZE(cmd) += 4;
      MifareCrc32 (new_key->data, new_key_length,
                   BUFFER_ARRAY(cmd) + BUFFER_SIZE(cmd));
      BUFFER_SIZE(cmd) += 4;
      break;
    }
  } else {                                 /* change key is authenticated key */
    switch (tag->authentication_scheme) {
    case AS_LEGACY:
      MifareCrc16Append(BUFFER_ARRAY(cmd) + 2, BUFFER_SIZE(cmd) - 2);
      BUFFER_SIZE(cmd) += 2;
      break;
    case AS_NEW:
      MifareCrc32Append(BUFFER_ARRAY(cmd), BUFFER_SIZE(cmd));
      BUFFER_SIZE(cmd) += 4;
      break;
    }
  }
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 2,
                                 MDCM_ENCIPHERED | ENC_COMMAND | NO_CRC);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  
  if(!p)
    return FAIL;

  /* 
   * if we changed the current authenticated key, we are not authenticated
   * anymore.
   */
    
  return SUCCESS;
}


/*
 * MifareGetKeyVersion
 * Description:
 *  Retrieve version information for a given key
 *  If AID=0x00 is selected, the command returns the version of the PICC master
 *  key and therefor only key_no=0x00 is vald. If AID != 0x00 the version of
 *  the specified key_no within the selected application is returned.
 *
 * Arguments
 *  tag: DESFire tag
 *  key_no: key to get version info (0 to # of application keys -1)
 *  version: ptr to version info array
  *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |                       1    1    |       |                                 |
 * |                    +----+-----+ |       |                                 |
 * |                    |0x64|key #| ---->   |                                 |
 * |                    +----+-----+ |       |    1                            |
 * |                     cmd         |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1       1                    |
 * |                                 |       | +----+-----------+              |
 * |                                 |   <---- |0x00|key version|              |
 * |                                 |       | +----+-----------+              |
 * |                                 |       | status                          |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Jan. 21, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareGetKeyVersion(mifare_tag *tag, uint8_t key_no, uint8_t *version)
{
  BUFFER_INIT(cmd,2);
  uint8_t *p;
  ssize_t nbytes;
  
  ASSERT_ACTIVE(tag);
  BUFFER_APPEND(cmd, 0x64);
  BUFFER_APPEND(cmd, key_no);
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY 
                                  | MAC_VERIFY);
  
  if (!p)
    return FAIL;
  *version = p[0];
  return SUCCESS;
}


/*
 * CreateApplication
 * Description:
 *  Create new applications on the PICC.
 *  
 * Arguments
 *  tag: DESFire tag
 *  aid: AID
 *  settings1: settings of the application master key
 *  settings2: # of keys that can be stored within the application crypto
 *             purposes, plus flags to specify crypto method and to enable
 *             giving ISO names to the EF
 *  want_iso_application: flag
 *  want_iso_file_identifiers: flag
 *  iso_file_id:       ID of the ISO DF
 *  iso_file_name[]:   name of the ISO DF
 *  iso_file_name_len: lenght of iso_file_name
 *
 *  settings: application master key settings
 *  no_keys: # of keys that can be stored within the application
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |    1        3      1            |       |                                 |
 * | +----+-----------+-----+------+ |       |                                 |
 * | |0xCA|    AID    |sett.|# keys| ---->   |                                 |
 * | +----+-----------+-----+------+ |       |    1                            |
 * |  cmd  LSB     MSB               |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1                            |
 * |                                 |       | +----+                          |
 * |                                 |   <---- |0x00|status                    |
 * |                                 |       | +----+                          |
 * |                                 |       |                                 |
 * +---------------------------------+       +---------------------------------+
 *
 * Assumptions:
 *  Assumes AID is already little endian
 *  
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Jan. 22, 2013      Nnoduka Eruchalu     Initial Revision
 */
static int CreateApplication(mifare_tag *tag, uint32_t aid,
                             uint8_t settings1, uint8_t settings2,
                             uint8_t want_iso_application,
                             uint8_t want_iso_file_identifiers,
                             uint16_t iso_file_id, uint8_t *iso_file_name,
                             size_t iso_file_name_len)
{
  BUFFER_INIT(cmd,22);
  uint8_t *p;
  ssize_t nbytes;
  
  ASSERT_ACTIVE(tag);
  
  if(want_iso_file_identifiers)
    settings2 |= 0x20;
  
  BUFFER_APPEND(cmd, 0xCA);
  BUFFER_APPEND_LE(cmd, aid, MIFARE_AID_SIZE);
  BUFFER_APPEND(cmd, settings1);
  BUFFER_APPEND(cmd, settings2);
  
  if(want_iso_application)
    BUFFER_APPEND_LE(cmd, iso_file_id, sizeof(iso_file_id));
  
  if(iso_file_name_len)
    BUFFER_APPEND_BYTES(cmd, iso_file_name, iso_file_name_len);
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY 
                                  | MAC_VERIFY);
  if(!p)
    return FAIL;
  
  return SUCCESS;
}


/*
 * MifareCreateApplication
 * Description:
 *  Creae new applications on the PICC.
 *  
 * Arguments
 *  tag: DESFire tag
 *  aid: AID
 *  settings: application master key settings
 *  no_keys: # of keys that can be stored within the application
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |    1        3      1            |       |                                 |
 * | +----+-----------+-----+------+ |       |                                 |
 * | |0xCA|    AID    |sett.|# keys| ---->   |                                 |
 * | +----+-----------+-----+------+ |       |    1                            |
 * |  cmd  LSB     MSB               |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1                            |
 * |                                 |       | +----+                          |
 * |                                 |   <---- |0x00|status                    |
 * |                                 |       | +----+                          |
 * |                                 |       |                                 |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Jan. 22, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareCreateApplication(mifare_tag *tag, uint32_t aid, uint8_t settings,
                            uint8_t no_keys)
{
  return CreateApplication(tag, aid, settings, no_keys, FALSE, FALSE, 0,NULL,0);
}


int MifareCreateApplicationIso(mifare_tag *tag, uint32_t aid, uint8_t settings, 
                               uint8_t no_keys,
                               uint8_t want_iso_file_identifiers,
                               uint8_t iso_file_id, uint8_t *iso_file_name,
                               size_t iso_file_name_len)
{
  return CreateApplication(tag, aid, settings, no_keys, TRUE,
                           want_iso_file_identifiers, iso_file_id, 
                           iso_file_name, iso_file_name_len);
}

int MifareCreateApplication3k3Des(mifare_tag *tag, uint32_t aid,
                                  uint8_t settings, uint8_t no_keys)
{
  return CreateApplication(tag, aid, settings,
                           APPLICATION_CRYPTO_3K3DES | no_keys,
                           FALSE, FALSE, 0, NULL, 0);
}


int MifareCreateApplication3k3DesIso(mifare_tag *tag, uint32_t aid,
                                     uint8_t settings, uint8_t no_keys,
                                     uint8_t want_iso_file_identifiers,
                                     uint16_t iso_file_id,
                                     uint8_t *iso_file_name,
                                     size_t iso_file_name_len)
{
  return CreateApplication(tag, aid, settings, 
                           APPLICATION_CRYPTO_3K3DES | no_keys, 
                           TRUE, want_iso_file_identifiers, iso_file_id,
                           iso_file_name, iso_file_name_len);
}

int MifareCreateApplicationAes(mifare_tag *tag, uint32_t aid,
                               uint8_t settings, uint8_t no_keys)
{
  return CreateApplication(tag, aid, settings, APPLICATION_CRYPTO_AES | no_keys,
                           FALSE, FALSE, 0, NULL, 0);
}

int MifareCreateApplicationAesIso(mifare_tag *tag, uint32_t aid,
                                  uint8_t settings, uint8_t no_keys,
                                  uint8_t want_iso_file_identifiers,
                                  uint16_t iso_file_id,
                                  uint8_t *iso_file_name, 
                                  size_t iso_file_name_len)
{
  return CreateApplication(tag, aid, settings, APPLICATION_CRYPTO_AES | no_keys,
                           TRUE, want_iso_file_identifiers, iso_file_id,
                           iso_file_name, iso_file_name_len);
}


/*
 * MifareDeleteApplication
 * Description:
 *  Delete Application on the PICC
 *  
 * Arguments
 *  tag: DESFire tag
 *  aid: AID
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |                 1        3      |       |                                 |
 * |              +----+-----------+ |       |                                 |
 * |              |0xDA|    AID    | ---->   |                                 |
 * |              +----+-----------+ |       |    1                            |
 * |               cmd  LSB     MSB  |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1                            |
 * |                                 |       | +----+                          |
 * |                                 |   <---- |0x00|status                    |
 * |                                 |       | +----+                          |
 * |                                 |       |                                 |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Jan. 22, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareDeleteApplication(mifare_tag *tag, uint32_t aid)
{
  BUFFER_INIT(cmd, 4 + CMAC_LENGTH);
  uint8_t *p;
  ssize_t nbytes;
    
  ASSERT_ACTIVE(tag);
  
  BUFFER_APPEND(cmd, 0xDA);
  BUFFER_APPEND_LE(cmd, aid, MIFARE_AID_SIZE);
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  if (!p)
    return FAIL;
  
  /* if we have deleted the current application, we are not authenticated
   * anymore
   */
  if(tag->selected_application == aid) {
    tag->selected_application = 0x000000; 
  }
  
  return SUCCESS;
}


/*
 * MifareGetApplicationIds
 * Description:
 *  Get the AIDs of all active applications on a PICC
 *  
 * Arguments
 *  tag: DESFire tag
 *  aids: pointer array where AIDs are to be stored [already allocated]
 *  aid_max_count: max size of array of aids
 *                 Range [1, MIFARE_MAX_APPLICATION_COUNT]
 *  count: to be updated with number of AIDs retrieved
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |                            1    |       |                                 |
 * |                          +----+ |       |                                 |
 * |                          |0x6A| ---->   |                                 |
 * |                          +----+ |       |    1                            |
 * |                            cmd  |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |       0 up to 19 AIDs           |
 * |                                 |       |    1     3       3              |
 * |                                 |       | +----+-------+-------+          |
 * |                                 |   <---- |0xAF|  AID  |  AID  |          |
 * |                                 |       | +----+-------+-------+          |
 * |                                 |       | status                          |
 * |                                 |       |                                 |
 * |                                 |       |                                 |
 * |                            1    |       |                                 |
 * |                          +----+ |       |                                 |
 * |                          |0xAF| ---->   |                                 |
 * |                          +----+ |       |    1                            |
 * |                            cmd  |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |        1 up to 7 AIDs           |
 * |                                 |       |    1     3       3              |
 * |                                 |       | +----+-------+-------+          |
 * |                                 |   <---- |0x00|  AID  |  AID  |          |
 * |                                 |       | +----+-------+-------+          |
 * |                                 |       | status                          |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Jan. 22, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareGetApplicationIds(mifare_tag *tag, uint32_t aids[], 
                            uint8_t aid_max_count, size_t *count)
{
  BUFFER_INIT(cmd,1);
  uint8_t buffer[MIFARE_AID_SIZE*MIFARE_MAX_APPLICATION_COUNT + CMAC_LENGTH +1];
  uint8_t *p;
  ssize_t nbytes;
  int i;        /* buffer index */
  
  ASSERT_ACTIVE(tag);
  
  BUFFER_APPEND(cmd, 0x6A);
  *count = 0;
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                   MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
 
  nbytes = MF_RXLEN;
  
  memcpy(buffer, MF_RXDATA, MF_RXLEN);
  if (MF_RXSTA  == 0xAF) {
    off_t offset = MF_RXLEN -1;
    p[0] = 0xAF;
    MifareCommTCL(p, BUFFER_SIZE(cmd));
    memcpy((uint8_t *)buffer+offset, MF_RXDATA, MF_RXLEN);
    nbytes += offset;
  }
  
  p = MifareCryptoPostprocessData(tag, buffer, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY | 
                                  MAC_VERIFY);
  if(!p)
    return FAIL;
  
  *count = (nbytes-1)/MIFARE_AID_SIZE; /* # of AIDs = */
                                       /* [# of rx'd bytes (except status)]/3 */
  
  for(i =0; (i < *count) && (i < aid_max_count); i++) {
    /* extract AID, which comes in Little Endian */
    aids[i] = le24toh(&buffer[MIFARE_AID_SIZE*i]);
  }
  
  return SUCCESS;
}


/*
 * MifareGetFreeMem
 * Description:
 *  Returns the free memory (in bytes) available on the card.
 *
 * Arguments
 *  tag: DESFire tag
 *  size: pointer to uint32_t to store the size (in bytes) [will be updated]
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |                            1    |       |                                 |
 * |                          +----+ |       |                                 |
 * |                          |0x6E| ---->   |                                 |
 * |                          +----+ |       |    1                            |
 * |                            cmd  |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1       3                    |
 * |                                 |       | +----+-----------+              |
 * |                                 |   <---- |0x00|   size    |              |
 * |                                 |       | +----+-----------+              |
 * |                                 |       | status LSB    MSB               |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 21, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareGetFreeMem(mifare_tag *tag, uint32_t *size)
{
  BUFFER_INIT(cmd, 1);
  uint8_t *p;
  ssize_t nbytes;
    
  ASSERT_ACTIVE(tag);
  
  BUFFER_APPEND(cmd, 0x6E);
    
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  if (!p)
    return FAIL;
  
  /* save little-endian size */
  *size = le24toh(p);
  
  return SUCCESS;
}


/*
 * MifareGetDfNames
 * Description:
 *  Returns the Application IDentifiers (AIDs), ISO DF Ids (fid) and ISO DF
 *  Names of all active applications on a DESFire card having an
 *  ISO DF ID/ DF Name 
 *
 * Arguments
 *  tag: DESFire tag
 *  dfs: array (of mifare_desfire_df structs) [pre-allocated]
 *  df_max_count: maximum number of entries in dfs array
 *  count: pointer to be updated with number of retrieved apps [will be updated]
 *                   
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |                            1    |       |                                 |
 * |                          +----+ |       |                                 |
 * |                          |0x6D| ---->   |                                 |
 * |                          +----+ |       |    1                            |
 * |                            cmd  |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 21, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareGetDfNames(mifare_tag *tag, mifare_desfire_df dfs[],
                     uint8_t df_max_count, size_t *count)
{
  BUFFER_INIT(cmd, 1);
  uint8_t *p;
  uint32_t aid;
  uint16_t fid;
  *count = 0;  /* no apps retrieved yet */
    
  ASSERT_ACTIVE(tag);
  
  BUFFER_APPEND(cmd, 0x6D);
    
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  do {
    MifareCommTCL(p, BUFFER_SIZE(cmd));
    
    if (MF_RXLEN > 1) {
      if (*count < df_max_count) {
        /* extract application data */
        aid = le24toh(MF_RXDATA);      /* have to assign aid and fid to temp */
        fid = le16toh(&MF_RXDATA[3]);  /* variables to satisfy MPLAB... weak */
        dfs[*count].aid = aid;     /* le24toh(MF_RXDATA); */
        dfs[*count].fid = fid;     /* le16toh(&MF_RXDATA[3]); */
        
        /* -6 accounts for 1st 5 bytes for aid and fid, and final status byte */
        memcpy (dfs[*count].df_name, MF_RXDATA + 5, MF_RXLEN - 6);
        dfs[*count].df_name_len = MF_RXLEN - 6;
        *count += 1;
      }
    }
    
    p[0] = 0xAF;  /* get additional frames */
  } while ((MF_RXSTA == 0xAF) && (*count < df_max_count));
    
  if (!p)
    return FAIL;
  
  /* save little-endian size */
  *count = le24toh(p);
  
  return SUCCESS;
}


/*
 * MifareSelectApplication
 * Description:
 *  Select one specific application for further access
 *  
 * Arguments
 *  tag: DESFire tag
 *  aid: AID
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |                 1        3      |       |                                 |
 * |              +----+-----------+ |       |                                 |
 * |              |0x5A|    AID    | ---->   |                                 |
 * |              +----+-----------+ |       |    1                            |
 * |               cmd  LSB     MSB  |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1                            |
 * |                                 |       | +----+                          |
 * |                                 |   <---- |0x00|status                    |
 * |                                 |       | +----+                          |
 * |                                 |       |                                 |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Jan. 22, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareSelectApplication(mifare_tag *tag, uint32_t aid) {
  BUFFER_INIT(cmd, 4 + CMAC_LENGTH);
  uint8_t *p;
  ssize_t nbytes;
  
  ASSERT_ACTIVE(tag);
    
  BUFFER_APPEND(cmd, 0x5A);
  BUFFER_APPEND_LE(cmd, aid, MIFARE_AID_SIZE);
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND);
  if (!p)
    return FAIL;
  
  /* SelectApplication invalidates the current authentication status */
  tag->selected_application = aid;
  
  return SUCCESS;
}


/*
 * MifareFormatPicc
 * Description:
 *  Release the PICC user memory
 *  
 * Arguments
 *  tag: DESFire tag
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |                             1   |       |                                 |
 * |                          +----+ |       |                                 |
 * |                          |0xFC| ---->   |                                 |
 * |                          +----+ |       |    1                            |
 * |                            cmd  |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1                            |
 * |                                 |       | +----+                          |
 * |                                 |   <---- |0x00|status                    |
 * |                                 |       | +----+                          |
 * |                                 |       |                                 |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Jan. 22, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareFormatPicc(mifare_tag *tag)
{
  BUFFER_INIT(cmd, 1 + CMAC_LENGTH);
  uint8_t *p;
  ssize_t nbytes;
  
  ASSERT_ACTIVE(tag);
  ASSERT_AUTHENTICATED(tag);
    
  BUFFER_APPEND(cmd, 0xFC);
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  if(!p)
    return FAIL;
  
  /* SelectApplication invalidates the current authentication status */
  tag->selected_application = 0x000000;
  
  return SUCCESS;
}


/*
 * MifareGetVersion
 * Description:
 *  Return manufacturing related data of the PICC
 *  
 * Arguments
 *  tag: DESFire tag
 *
 * Operation:
 * +--------+       +----------------------------------------------------------+
 * |  PCD:  |       |            PICC:                                         |
 * |        |       |                                                          |
 * |    1   |       |                                                          |
 * | +----+ |       |                                                          |
 * | |0x60| ---->   |                                                          |
 * | +----+ |       |    1    1     1    1    1     1      1        1          |
 * |   cmd  |  1st  | +----+-----+----+----+-----+-----+-------+--------+      |
 * |        |   <---- |0xAF|vend.|type|sub |major|minor|storage|protocol|      |
 * |        |  frame| |    |ID   |    |type|ver  |ver  |size   |        |      |
 * |        |       | +----+-----+----+----+-----+-----+-------+--------+      |
 * |        |       | status                                                   |
 * |        |       |                                                          |
 * |    1   |       |                                                          |
 * | +----+ |       |                                                          |
 * | |0xAF| ---->   |                                                          |
 * | +----+ |       |    1    1     1    1    1     1      1        1          |
 * |        |  2nd  | +----+-----+----+----+-----+-----+-------+--------+      |
 * |        |   <---- |0xAF|vend.|type|sub |major|minor|storage|protocol|      |
 * |        |  frame| |    |ID   |    |type|ver  |ver  |size   |        |      |
 * |        |       | +----+-----+----+----+-----+-----+-------+--------+      |
 * |        |       | status                                                   |
 * |        |       |                                                          |
 * |    1   |       |                                                          |
 * | +----+ |       |                                                          |
 * | |0xAF| ---->   |                                                          |
 * | +----+ |       |    1       7            5          1          1          |
 * |        |  3rd  | +----+-----------+-------------+--------+-----------+    |
 * |        |   <---- |0x00|    UID    |  batch No.  | cw prod  year prod |    |
 * |        |  frame| +----+-----------+-------------+--------+-----------+    |
 * |        |       | status                                                   |
 * |        |       |                                                          |
 * +--------+       +----------------------------------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Jan. 22, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareGetVersion(mifare_tag *tag, mifare_desfire_version_info *version_info)
{
  BUFFER_INIT(cmd, 1);
  uint8_t *p;
  ssize_t nbytes;
  uint8_t buffer[28+CMAC_LENGTH+1]; /* + cmac + status byte on 3rd/last frame */
  
  ASSERT_ACTIVE(tag);
    
  BUFFER_APPEND(cmd, 0x60);
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  memcpy(&(version_info->hardware), MF_RXDATA, 7); /* dont have to worry about*/
  memcpy(buffer, MF_RXDATA, 7);                    /* endianness because */
                                                   /* struct's elements are */
  p[0] = 0xAF;                                     /* bytes or arrays of bytes*/
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  memcpy(&(version_info->software), MF_RXDATA, 7);
  memcpy(buffer+7, MF_RXDATA, 7);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  memcpy(&(version_info->uid), MF_RXDATA, 14);
  memcpy(buffer+14, MF_RXDATA, MF_RXLEN); /* include cmac and status byte */
  
  
  nbytes = 28+CMAC_LENGTH+1;              /* sizeof(buffer) */
  p = MifareCryptoPostprocessData(tag, buffer, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  if(!p)
    return FAIL;
  
  return SUCCESS;
}


/*
 * MifareGetCardUid
 * Description:
 *  Return the card's UID
 *  
 * Arguments
 *  tag: DESFire tag
 *  uid: array of bytes [pre-allocated to 7 bytes]
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |                             1   |       |                                 |
 * |                          +----+ |       |                                 |
 * |                          |0x51| ---->   |                                 |
 * |                          +----+ |       |    1                            |
 * |                            cmd  |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1       7                    |
 * |                                 |       | +----+------------+             |
 * |                                 |   <---- |0x00|     UID    |             |
 * |                                 |       | +----+------------+             |
 * |                                 |       | status                          |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Notes:
 *  This command must be preceded by an authentication (with any key)
 *
 * Revision History:
 *  Mar. 21, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareGetCardUid(mifare_tag *tag, uint8_t uid[/*7*/])
{
  BUFFER_INIT(cmd, 1);
  uint8_t *p;
  ssize_t nbytes;
  
  ASSERT_ACTIVE(tag);
  ASSERT_AUTHENTICATED(tag);
    
  BUFFER_APPEND(cmd, 0x51);
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 1,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, MDCM_ENCIPHERED);
  
  if(!p)
    return FAIL;
  
  memcpy(uid, MF_RXDATA, 7);
  
  return SUCCESS;
}


/*
 * MifareGetFileIds
 * Description:
 *  Return the File IDentifiers of all active files within the currently
 *  selected application.
 *  
 * Arguments
 *  tag: DESFire tag
 *  files: array of FIDs [memory pre-allocated]
 *  fid_max_count: max number of FIDs to get. Range is [1,MIFARE_MAX_FILE_COUNT]
 *  count: number of active files in currently selected application
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |                             1   |       |                                 |
 * |                          +----+ |       |                                 |
 * |                          |0x6F| ---->   |                                 |
 * |                          +----+ |       |    1                            |
 * |                            cmd  |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |         0 to 16                 |
 * |                                 |       |    1    1    1    1             |
 * |                                 |       | +----+----+----+----+           |
 * |                                 |   <---- |0x00|FID |FID |FID |           |
 * |                                 |       | +----+----+----+----+           |
 * |                                 |       | status                          |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 21, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareGetFileIds(mifare_tag *tag, uint8_t files[], uint8_t fid_max_count,
                     size_t *count)
{
  BUFFER_INIT(cmd, 1+CMAC_LENGTH);
  uint8_t *p;
  ssize_t nbytes;
  
  ASSERT_ACTIVE(tag);
  
  BUFFER_APPEND(cmd, 0x6F);
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  
  if(!p)
    return FAIL;
  
  *count = nbytes - 1; /* dont account for status */
  
  /* copy the smaller of available FID space and available FIDs */
  memcpy(files, MF_RXDATA, MIN(fid_max_count, *count));
    
  return SUCCESS;
}


int MifareGetIsoFileIds(mifare_tag *tag, uint16_t files[],
                        uint8_t fid_max_count, size_t *count)
{
  BUFFER_INIT(cmd, 1);
  uint8_t *p;
  ssize_t nbytes;
  uint8_t data[(27+5)*2 + 8];
  off_t offset = 0;
  size_t i; /* index into files array */
  
  ASSERT_ACTIVE(tag);
  BUFFER_APPEND(cmd, 0x61);
      
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  do {
    MifareCommTCL(p, BUFFER_SIZE(cmd));
    
    memcpy(data+offset, MF_RXDATA, MF_RXLEN - 1);
    offset += MF_RXLEN - 1;
    
    p[0] = 0xAF;  /* get additional frames */
  } while (MF_RXSTA == 0xAF);
  
  nbytes = offset;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, data, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND);
  
  if(!p)
    return FAIL;
  
  *count = nbytes/2;

  
  for(i=0; (i < *count) && (i < fid_max_count); i++) {
    files[i] = le16toh(&p[(2*i)]);
  }
  
  return SUCCESS;
}


/*
 * MifareGetFileSettings
 * Description:
 *  Get information on the properties of a specific file.
 *  The information provided by this command depends on the type of the file 
 *  which is queried.
 *  
 * Arguments
 *  tag: DESFire tag
 *  file_no: DESFire File IDentifier
 *  settings: pointer to mifare_desfire_file_settings struct
 *
 * Operation:
 * +-----------+       +-------------------------------------------------------+
 * |  PCD:     |       |            PICC:                                      |
 * |           |       |                                                       |
 * |    1      |       |                                                       |
 * | +----+---+|       |                                                       |
 * | |0xF5|FID|---->   |                                                       |
 * | +----+---+|           1                                                   |
 * |   cmd     |       | +----+                                                |
 * |           |   <---- | EC |status                                          |
 * |           |       | +----+                                                |
 * |           |   or  |                                                       |
 * |           |       |    1    1     1      2           3                    |
 * |           |       | +----+-----+----+--------+-------------+              |
 * |           |   <---- |0x00|file |comm| access |  file size  |              |
 * |           |       | |    |type |sett| rights |             |              |
 * |           |       | +----+-----+----+--------+-------------+              |
 * |           |       | status                                                |
 * |           |       |                                                       |
 * |           |       |                                                       |
 * |           |       |    1    1     1     2     4     4     4        1      |
 * |           |       | +----+-----+----+------+-----+-----+-------+-------+  |
 * |           |   <---- |0x00|file |comm|access|lower|upper|limited|limited|  |
 * |           |       | |    |type |sett|rights|limit|limit|cr. val|cr. en.|  |
 * |           |       | +----+-----+----+------+-----+-----+-------+-------+  |
 * |           |       | status           L    M L   M L   M LSB   M           |
 * |           |       |                                                       |
 * |           |       |                                                       |
 * |           |       |    1    1     1     2     3       3          3        |
 * |           |       | +----+-----+----+------+------+--------+----------+   |
 * |           |   <---- |0x00|file |comm|access|record|max # of|curr. # of|   |
 * |           |       | |    |type |sett|rights|size  |records |records   |   |
 * |           |       | +----+-----+----+------+------+--------+----------+   |
 * |           |       | status           L    M L  MSB LSB  MSB LSB    MSB    |
 * |           |       |                                                       |
 * +-----------+       +-------------------------------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 22, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareGetFileSettings(mifare_tag *tag, uint8_t file_no,
                          mifare_desfire_file_settings *settings)
{
  BUFFER_INIT(cmd, 1+CMAC_LENGTH);
  uint8_t *p;
  ssize_t nbytes;
  mifare_desfire_raw_file_settings raw_settings;
  
  ASSERT_ACTIVE(tag);
  
  BUFFER_APPEND(cmd, 0xF5);
  BUFFER_APPEND(cmd, file_no);
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  
  if(!p)
    return FAIL;
  
  memcpy(&raw_settings, p, nbytes-1);
  
  settings->file_type = raw_settings.file_type;
  settings->communication_settings = raw_settings.communication_settings;
  settings->access_rights = le16toh(raw_settings.access_rights);
  
  switch(settings->file_type) {
  case MDFT_STANDARD_DATA_FILE:
  case MDFT_BACKUP_DATA_FILE:
    settings->settings.standard_file.file_size = 
      le24toh(raw_settings.settings.standard_file.file_size);
    break;
    
  case MDFT_VALUE_FILE_WITH_BACKUP:
    settings->settings.value_file.lower_limit = 
      le32toh((uint8_t *) raw_settings.settings.value_file.lower_limit);
    settings->settings.value_file.upper_limit = 
      le32toh((uint8_t *) raw_settings.settings.value_file.upper_limit);
    settings->settings.value_file.limited_credit_value = 
      le32toh((uint8_t *)raw_settings.settings.value_file.limited_credit_value);
    settings->settings.value_file.limited_credit_enabled = 
      raw_settings.settings.value_file.limited_credit_enabled;
    break;
    
  case MDFT_LINEAR_RECORD_FILE_WITH_BACKUP:
  case MDFT_CYCLIC_RECORD_FILE_WITH_BACKUP:
    settings->settings.record_file.record_size = 
      le24toh(raw_settings.settings.record_file.record_size);
    settings->settings.record_file.max_number_of_records = 
      le24toh(raw_settings.settings.record_file.max_number_of_records);
    settings->settings.record_file.current_number_of_records = 
      le24toh(raw_settings.settings.record_file.current_number_of_records);
    break;
    
  default:
    return FAIL;
    /* break; */
  } /* end switch(settings->file_type) */
  
  
  return SUCCESS;
}


/*
 * MifareChangeFileSettings
 * Description:
 *  Change the access parameters of an existing file.
 *  
 * Arguments
 *  tag: DESFire tag
 *  file_no: DESFire File IDentifier
 *  communication_settings: new communication settings
 *  access_rights: new access rights
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |     1    1    1     2           |       |                                 |
 * |  +----+----+----+--------+      |       |                                 |
 * |  |0x5F|File|Comm| Access |      ---->   |                                 |
 * |  |    | No |Sett| Rights |      |       |                                 |
 * |  +----+----+----+--------+      |       |                                 |
 * |   cmd            LSB  MSB       |       |                                 |
 * |                                 | or    |                                 |
 * |     1    1        8             |       |                                 |
 * |  +----+----+--------------+     |       |                                 |
 * |  |0x5F|File|new deciphered|     ---->   |                                 |
 * |  |    | No |settings      |     |       |                                 |
 * |  +----+----+--------------+     |       |                                 |
 * |   cmd                           |       |                                 |
 * |                                 |       |                                 |
 * |                                 |       |    1                            |
 * |                                 |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1                            |
 * |                                 |       | +----+                          |
 * |                                 |   <---- |0x00|status                    |
 * |                                 |       | +----+                          |
 * |                                 |       |                                 |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 22, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareChangeFileSettings(mifare_tag *tag, uint8_t file_no,
                             uint8_t communication_settings,
                             uint16_t access_rights) 
{
  /* really should be either 5+CMAC_LENGTH or 10, so use max size */
  BUFFER_INIT(cmd, 10+CMAC_LENGTH);
  uint8_t *p;
  ssize_t nbytes;
  mifare_desfire_file_settings settings; 
  int res = MifareGetFileSettings(tag, file_no, &settings);
  
  if (res != SUCCESS)
    return res;
  
  ASSERT_ACTIVE(tag);
  
  BUFFER_APPEND(cmd, 0x5F);
  BUFFER_APPEND(cmd, file_no);
  BUFFER_APPEND(cmd, communication_settings);
  BUFFER_APPEND_LE(cmd, access_rights, 2);
  
  if (MDAR_CHANGE_AR(settings.access_rights) == MDAR_FREE) {
    /* if ChangeAccessRights Access Rights is set with value "free", no security
     * mechanism is necessary and therefore the data is sent as plain text
     */
    p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                   MDCM_PLAIN | CMAC_COMMAND);
  } else {
    p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 2,
                                   MDCM_ENCIPHERED | ENC_COMMAND);
  }
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
    
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  if (!p)
    return FAIL;
  
  return SUCCESS;
}



/*
 * CreateFileData
 * Description:
 *  Helper function to use in MifareCreateStdDataFile and 
 *  MifareCreateBackupDataFile
 *  
 * Arguments
 *  tag: DESFire tag
 *  command:         for create standard(0xCD) or backup(0xCB) data files
 *  file_no:         DESFire File IDentifier
 *  has_iso_file_id: flag for presence of iso_file_id
 *  iso_file_id:     IDentifier of the EF for ISO 7816-4 applications
 *  communication_settings: file's communication settings
 *  access_rights:   file's Access Rights
 *  file_size:       size of the file in bytes
 *
 * Operation:
 * +---------------------------------------+       +---------------------------+
 * |           PCD:                        |       |            PICC:          |
 * |    1     1    1      2        3       |       |                           |
 * | +-----+----+----+--------+--------+   |       |                           |
 * | |0xCD/|File|Comm| Access |  File  |   ---->   |                           |
 * | |0xCB |No  |Sett| Rights |  Size  |   |       |                           |
 * | +-----+----+----+--------+--------+   |       |    1                      |
 * |   cmd            LSB  MSB LSB  MSB    |       | +----+                    |
 * |                                       |   <---- | EC |status              |
 * |                                       |       | +----+                    |
 * |                                       |   or  |                           |
 * |                                       |       |    1                      |
 * |                                       |       | +----+                    |
 * |                                       |   <---- |0x00|status              |
 * |                                       |       | +----+                    |
 * |                                       |       |                           |
 * +---------------------------------------+       +---------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 22, 2013      Nnoduka Eruchalu     Initial Revision
 */
static int CreateFileData(mifare_tag *tag, uint8_t command, uint8_t file_no,
                          uint8_t has_iso_file_id, uint16_t iso_file_id, 
                          uint8_t communication_settings, 
                          uint16_t access_rights, uint32_t file_size)
{
  BUFFER_INIT(cmd, 10+CMAC_LENGTH);
  uint8_t *p;
  ssize_t nbytes;
  
  ASSERT_ACTIVE(tag);
  
  BUFFER_APPEND(cmd, command);
  BUFFER_APPEND(cmd, file_no);
  if (iso_file_id)
    BUFFER_APPEND_LE(cmd, iso_file_id, sizeof(iso_file_id));
  BUFFER_APPEND(cmd, communication_settings);
  BUFFER_APPEND_LE(cmd, access_rights, 2);
  BUFFER_APPEND_LE(cmd, file_size, 3);
    
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  
  if(!p)
    return FAIL;
  
  return SUCCESS;
}

int MifareCreateStdDataFile(mifare_tag *tag, uint8_t file_no, 
                            uint8_t communication_settings, 
                            uint16_t access_rights, uint32_t file_size)
{
  return CreateFileData(tag, 0xCD, file_no, FALSE, 0x0000, 
                        communication_settings, access_rights, file_size);
}
int MifareCreateStdDataFileIso(mifare_tag *tag, uint8_t file_no,
                               uint8_t communication_settings,
                               uint16_t access_rights, uint32_t file_size, 
                               uint16_t iso_file_id)
{
  return CreateFileData(tag, 0xCD, file_no, TRUE, iso_file_id, 
                        communication_settings, access_rights, file_size);
}

int MifareCreateBackupDataFile(mifare_tag *tag, uint8_t file_no,
                               uint8_t communication_settings, 
                               uint16_t access_rights, uint32_t file_size)
{
  return CreateFileData(tag, 0xCB, file_no, FALSE, 0x0000, 
                        communication_settings, access_rights, file_size);
}

int MifareCreateBackupDataFileIso(mifare_tag *tag, uint8_t file_no, 
                                  uint8_t communication_settings,
                                  uint16_t access_rights, uint32_t file_size, 
                                  uint16_t iso_file_id)
{
  return CreateFileData(tag, 0xCB, file_no, TRUE, iso_file_id, 
                        communication_settings, access_rights, file_size);
}


/*
 * MifareCreateValueFile
 * Description:
 *  create files for the storage and manipulation of 32bit signed integer values
 *  with an existing application on the PICC.
 *  
 * Arguments
 *  tag:                    DESFire tag
 *  file_no:                DESFire File IDentifier
 *  communication_settings: 
 *  access_rights:          file Access Rights
 *  lower_limit:            lower limit which cant be passed by a Debit on value
 *  upper_limit:            upper limit which cant be passed by a Credit on val.
 *                          should be >= lower_limit
 *  value:                  initial value of the value file
 *  limited_credit_enable:  flag to activate the LimitedCredit feature.
 *
 * Operation:
 * +-----------------------------------------------------+       +-------------+
 * |                         PCD:                        |       |    PICC:    |
 * |    1    1    1     2      4     4     4      1      |       |             |
 * | +----+----+----+------+-----+-----+-----+-------+   |       |             |
 * | |0xCC|File|Comm|Access|Lower|Upper|Value|Limited|   ---->   |             |
 * | |    |No  |Sett|Rights|Limit|Limit|     |Cr. En.|   |       |             |
 * | +----+----+----+------+-----+-----+-----+-------+   |       |    1        |
 * |  cmd            LSB  M L   M L   M L   M            |       | +----+      |
 * |                                                     |   <---- | EC |      |
 * |                                                     |       | +----+      |
 * |                                                     |   or  |             |
 * |                                                     |       |    1        |
 * |                                                     |       | +----+      |
 * |                                                     |   <---- |0x00|      |
 * |                                                     |       | +----+      |
 * |                                                     |       |             |
 * +-----------------------------------------------------+       +-------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 22, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareCreateValueFile(mifare_tag *tag, uint8_t file_no,
                                 uint8_t communication_settings,
                                 uint16_t access_rights, int32_t lower_limit, 
                                 int32_t upper_limit, int32_t value, 
                                 uint8_t limited_credit_enable)
{
  BUFFER_INIT(cmd, 18+CMAC_LENGTH);
  uint8_t *p;
  ssize_t nbytes;
  
  ASSERT_ACTIVE(tag);
  
  BUFFER_APPEND(cmd, 0xCC);
  BUFFER_APPEND(cmd, file_no);
  BUFFER_APPEND(cmd, communication_settings);
  BUFFER_APPEND_LE(cmd, access_rights, 2);
  BUFFER_APPEND_LE(cmd, lower_limit, 4);
  BUFFER_APPEND_LE(cmd, upper_limit, 4);
  BUFFER_APPEND_LE(cmd, value, 4);
  
  if (limited_credit_enable)
    BUFFER_APPEND(cmd, 0x01);
  else
    BUFFER_APPEND(cmd, 0x00);
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  
  if(!p)
    return FAIL;
  
  return SUCCESS;
}


/*
 * CreateFileRecord
 * Description:
 *  Helper function to use in MifareCreateLinearRecordFile and 
 *  MifareCreateCyclicRecordFile
 *  
 * Arguments
 *  tag: DESFire tag
 *  command:         for create linear(0xC1) or cyclic(0xC0) record files
 *  file_no:         DESFire File IDentifier
 *  has_iso_file_id: flag for presence of iso_file_id
 *  iso_file_id:     IDentifier of the EF for ISO 7816-4 applications
 *  communication_settings: file's communication settings
 *  access_rights:   file's Access Rights
 *  record_size:     size of one record in bytes
 *  max_number_of_records:  # of records
 *
 * Operation:
 * +-----------------------------------------------------+       +-------------+
 * |                         PCD:                        |       |    PICC:    |
 * |       1    1    1     2         3           3       |       |             |
 * |    +-----+----+----+------+----------+------------+ |       |             |
 * |    |0xC1/|File|Comm|Access|  Record  | Max Num Of | ---->   |             |
 * |    |0xCO |No  |Sett|Rights|  Size    | Records    | |       |             |
 * |    +-----+----+----+------+----------+------------+ |       |    1        |
 * |      cmd            LSB  M LSB    MSB LSB      MSB  |       | +----+      |
 * |                                                     |   <---- | EC |      |
 * |                                                     |       | +----+      |
 * |                                                     |   or  |             |
 * |                                                     |       |    1        |
 * |                                                     |       | +----+      |
 * |                                                     |   <---- |0x00|      |
 * |                                                     |       | +----+      |
 * |                                                     |       |             |
 * +-----------------------------------------------------+       +-------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
static int CreateFileRecord(mifare_tag *tag, uint8_t command, uint8_t file_no,
                            uint8_t has_iso_file_id, uint16_t iso_file_id,
                            uint8_t communication_settings, 
                            uint16_t access_rights, uint32_t record_size, 
                            uint32_t max_number_of_records)
{
  BUFFER_INIT(cmd, 11+CMAC_LENGTH);
  uint8_t *p;
  ssize_t nbytes;
  
  ASSERT_ACTIVE(tag);
  
  BUFFER_APPEND(cmd, command);
  BUFFER_APPEND(cmd, file_no);
  if (iso_file_id)
    BUFFER_APPEND_LE(cmd, iso_file_id, sizeof(iso_file_id));
  BUFFER_APPEND(cmd, communication_settings);
  BUFFER_APPEND_LE(cmd, access_rights, 2);
  BUFFER_APPEND_LE(cmd, record_size, 3);
  BUFFER_APPEND_LE(cmd, max_number_of_records, 3);
    
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  
  if(!p)
    return FAIL;
  
  return SUCCESS;
}


int MifareCreateLinearRecordFile(mifare_tag *tag, uint8_t file_no, 
                                 uint8_t communication_settings, 
                                 uint16_t access_rights,
                                 uint32_t record_size, 
                                 uint32_t max_number_of_records)
{
  return CreateFileRecord(tag, 0xC1,file_no, FALSE, 0x0000, 
                          communication_settings, access_rights, record_size,
                          max_number_of_records);
}

int MifareCreateLinearRecordFileIso(mifare_tag *tag, uint8_t file_no, 
                                    uint8_t communication_settings, 
                                    uint16_t access_rights, 
                                    uint32_t record_size, 
                                    uint32_t max_number_of_records, 
                                    uint16_t iso_file_id)
{
  return CreateFileRecord(tag, 0xC1,file_no, TRUE, iso_file_id,
                          communication_settings, access_rights, record_size,
                          max_number_of_records);
}

int MifareCreateCyclicRecordFile(mifare_tag *tag, uint8_t file_no, 
                                 uint8_t communication_settings,
                                 uint16_t access_rights,
                                 uint32_t record_size,
                                 uint32_t max_number_of_records)
{
   return CreateFileRecord(tag, 0xC0,file_no, FALSE, 0x0000, 
                          communication_settings, access_rights, record_size,
                          max_number_of_records);
}

int MifareCreateCyclicRecordFileIso(mifare_tag *tag, uint8_t file_no, 
                                    uint8_t communication_settings, 
                                    uint16_t access_rights, 
                                    uint32_t record_size, 
                                    uint32_t max_number_of_records, 
                                    uint16_t iso_file_id)
{
  return CreateFileRecord(tag, 0xC0,file_no, TRUE, iso_file_id,
                          communication_settings, access_rights, record_size,
                          max_number_of_records);
}


/*
 * MifareDeleteFile
 * Description:
 *  Deactivate a file within the file directory of the currently selected
 *  application.
 *
 * Arguments
 *  tag:     DESFire tag
 *  file_no: DESFire File IDentifier
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |                     1     1     |       |                                 |
 * |                  +----+-------+ |       |                                 |
 * |                  |0xDF|File No| ---->   |                                 |
 * |                  +----+-------+ |       |    1                            |
 * |                    cmd          |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1                            |
 * |                                 |       | +----+                          |
 * |                                 |   <---- |0x00|                          |
 * |                                 |       | +----+                          |
 * |                                 |       | status                          |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareDeleteFile(mifare_tag *tag, uint8_t file_no)
{
  BUFFER_INIT(cmd, 2+CMAC_LENGTH);
  uint8_t *p;
  ssize_t nbytes;
  
  ASSERT_ACTIVE(tag);
  
  BUFFER_APPEND(cmd, 0xDF);
  BUFFER_APPEND(cmd, file_no);
      
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  
  if(!p)
    return FAIL;
  
  return SUCCESS; 
}


/*
 * GetReadCommunicationSettings
 * Description:
 *  Get read communication mode for a file in currently selected application.
 *
 * Arguments
 *  tag:     DESFire tag
 *  file_no: DESFire File IDentifier  
 *
 * Operation:
 *  First get file settings using MifareGetFileSettings()
 *  If that is successful, depending on the AccessRights field (settings r and 
 *  r/w) we have to decide whether we are able to communicate in the mode
 *  indicated by the settings' communication_settings.  
 *  If tag->authenticated_key_no does neither match r nor r/w and one of these
 *  settings contains the value "free" (0x0E) communication has to be done in
 *  plain mode regardless of the mode indicated by communication_settings.
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
static uint8_t GetReadCommunicationSettings(mifare_tag *tag, uint8_t file_no)
{
  mifare_desfire_file_settings settings;
  uint8_t read_only_access, read_write_access;
  
  if (MifareGetFileSettings(tag, file_no, &settings) != SUCCESS) {
    return -1; /* return an invalid communication mode */
  }
  
  read_only_access = MDAR_READ(settings.access_rights);
  read_write_access = MDAR_READ_WRITE(settings.access_rights);
  
  if((read_only_access != tag->authenticated_key_no) &&
     (read_write_access != tag->authenticated_key_no) &&
     ((read_only_access == 0x0E) || (read_write_access == 0x0E))) {
    return MDCM_PLAIN;
    
  } else {
    return settings.communication_settings;
  }      
}


/*
 * GetWriteCommunicationSettings
 * Description:
 *  Get write communication mode for a file in currently selected application.
 *
 * Arguments
 *  tag:     DESFire tag
 *  file_no: DESFire File IDentifier  
 *
 * Operation:
 *  First get file settings using MifareGetFileSettings()
 *  If that is successful, depending on the AccessRights field (settings w and 
 *  r/w) we have to decide whether we are able to communicate in the mode
 *  indicated by the settings' communication_settings.  
 *  If tag->authenticated_key_no does neither match w nor r/w and one of these
 *  settings contains the value "free" (0x0E) communication has to be done in
 *  plain mode regardless of the mode indicated by communication_settings.
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
static uint8_t GetWriteCommunicationSettings(mifare_tag *tag, uint8_t file_no)
{
  mifare_desfire_file_settings settings;
  uint8_t write_only_access, read_write_access;
  
  if (MifareGetFileSettings(tag, file_no, &settings) != SUCCESS) {
    return -1; /* return an invalid communication mode */
  }
  
  write_only_access = MDAR_WRITE(settings.access_rights);
  read_write_access = MDAR_READ_WRITE(settings.access_rights);
  
  if((write_only_access != tag->authenticated_key_no) &&
     (read_write_access != tag->authenticated_key_no) &&
     ((write_only_access == 0x0E) || (read_write_access == 0x0E))) {
    return MDCM_PLAIN;
    
  } else {
    return settings.communication_settings;
  }      
}





/*
 * ReadData
 * Description:
 *  Helper function to read data from Data and Record Files
 *
 * Arguments
 *  tag:                DESFire tag
 *  command:            to read data (0xBD) or records (0xBB)
 *  file_no:            DESFire File IDentifier
 *  offset:             starting position for read within the file
 *  length:             number of bytes to be read
 *  data:               buffer to receive the data [pre-allocated] [modified]
 *  max_count           maximum data length to read. Set to 0 to read whole
 *                      file
 *  data_size:          actual number of bytes read [modified ]
 *  communication_mode: file's communication mode
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |    1     1        3       3     |       |                                 |
 * | +-----+------+-------+--------+ |       |                                 |
 * | |0xBD/| File |Offset | Length | ---->   |                                 |
 * | |0xBB | No   |       |        | |       |                                 |
 * | +-----+------+-------+--------+ |       |    1                            |
 * |  cmd          LSB  MSB LSB MSB  |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1         1 up to 59         |
 * |                                 |       | +----+---------------------+    |
 * |                                 |   <---- |0xAF|          DATA       |    |
 * |                                 |       | +----+---------------------+    |
 * |                                 |       | status                          |
 * |                          +----+ |       |                                 |
 * |                          |0xAF| ---->   |                                 |
 * |                          +----+ |       |    1                            |
 * |                           cmd   |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1         1 up to 59         |
 * |                                 |       | +----+---------------------+    |
 * |                                 |   <---- |0xAF|          DATA       |    |
 * |                                 |       | +----+---------------------+    |
 * |                                 |       | status                          |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
static int ReadData(mifare_tag *tag,  uint8_t command, uint8_t file_no, 
                    uint32_t offset, uint32_t length, uint8_t data[], 
                    uint32_t max_count, ssize_t *data_size, 
                    uint8_t communication_mode)
{
  BUFFER_INIT(cmd, 8);
  uint8_t *p;
  ssize_t nbytes;
  size_t frame_bytes; /* number of bytes in a frame (excluding status) */
  *data_size = 0;     /* no data yet */
    
  ASSERT_ACTIVE(tag);
  ASSERT_CS(communication_mode);
  
  BUFFER_APPEND(cmd, command);
  BUFFER_APPEND(cmd, file_no);
  BUFFER_APPEND_LE(cmd, offset, 3);
  BUFFER_APPEND_LE(cmd, length, 3);
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 8,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  do {
    MifareCommTCL(p, BUFFER_SIZE(cmd));
    
    frame_bytes = MF_RXLEN - 1;
    if ((*data_size+frame_bytes) <= max_count-1) { /* -1 for status byte */
      memcpy(data+*data_size, MF_RXDATA, frame_bytes);
      *data_size += frame_bytes;
    }
    
    p[0] = 0xAF; /* get additional frames */
  } while((MF_RXSTA == 0xAF) && (*data_size < max_count-1));
  
  /* append status byte */
  data[*data_size] = 0x00;
  *data_size += 1;
    
  nbytes = *data_size;               /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, data, &nbytes, 
                                  communication_mode | CMAC_COMMAND | 
                                  CMAC_VERIFY | MAC_VERIFY);
  
  if(!p)
    return FAIL;
    
  /* remove status byte from actual length */
  *data_size = (nbytes <= 0 ) ? nbytes : (nbytes - 1);
  
  return SUCCESS; 
}


/*
 * MifareReadData
 * Description:
 *  Read data from Standard Data Files or Backup Data Files, with communication
 *  mode to be determined.
 *
 * Arguments
 *  tag:                DESFire tag
 *  file_no:            DESFire File IDentifier
 *  offset:             starting position for read within the file
 *  length:             number of bytes to be read
 *  data:               buffer to receive the data [pre-allocated]
 *  max_count           maximum data length to read. Set to 0 to read whole
 *                      file
 *  data_size:          actual number of bytes read.
 *  
 * Operation:
 *  Call MifareReadDataEx using the Communication mode returned by
 *  GetReadCommunicationSettings()
 * 
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareReadData(mifare_tag *tag, uint8_t file_no, uint32_t offset, 
                   uint32_t length, uint8_t data[], uint32_t max_count, 
                   ssize_t *data_size)
{
  return MifareReadDataEx(tag, file_no, offset, length, data, max_count,
                          data_size, GetReadCommunicationSettings(tag,file_no));
}

/*
 * MifareReadDataEx
 * Description:
 *  Read data from Standard Data Files or Backup Data Files, with explicit
 *  communication mode.
 *
 * Arguments
 *  tag:                DESFire tag
 *  file_no:            DESFire File IDentifier
 *  offset:             starting position for read within the file
 *  length:             number of bytes to be read
 *  data:               buffer to receive the data [pre-allocated]
 *  max_count           maximum data length to read. Set to 0 to read whole
 *                      file
 *  data_size:          actual number of bytes read.
 *  communication_mode: file's communication mode
 *
 * Operation:
 *  Call ReadData with appropriate command for Data Files (0xBD).
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareReadDataEx(mifare_tag *tag, uint8_t file_no, uint32_t offset, 
                     uint32_t length, uint8_t data[], uint32_t max_count,
                     ssize_t *data_size, uint8_t communication_mode)
{
  return ReadData(tag, 0xBD, file_no, offset, length, data, max_count, 
                  data_size, communication_mode);
}


/*
 * WriteData
 * Description:
 *  Helper function to write data to Data and Record Files
 *
 * Arguments
 *  tag:                DESFire tag
 *  command:            to write data files (0x3D) or records (0x3B)
 *  file_no:            DESFire File IDentifier
 *  offset:             starting position for write within the file
 *  length:             number of bytes to be written.
 *                      range is [1, MF_MAX_WRITE_LENGTH]
 *  data:               buffer of data bytes to be written
 *  data_sent:          actual number of data bytes written
 *  communication_mode: file's communication mode
 *  
 *
 * Operation:
 * +-----------------------------------------------------+       +-------------+
 * |                         PCD:                        |       |    PICC:    |
 * |    1    1       3       3          0 up to 52       |       |             |
 * | +-----+----+-------+--------+---------------------+ |       |             |
 * | |0x3D/|File||Offset| Length |         DATA        | ---->   |             |
 * | |0x3B |No  |       |        |                     | |       |             |
 * | +-----+----+-------+--------+---------------------+ |       |    1        |
 * |  cmd        LSB   M L    MSB                        |       | +----+      |
 * |                                                     |   <---- |0xAF|      |
 * |                                                     |       | +----+      |
 * |                                                     |   or  |             |
 * |                                                     |       |    1        |
 * |                                                     |       | +----+      |
 * |                                                     |   <---- | EC |      |
 * |                                                     |       | +----+      |
 * |                                                     |       |             |
 * |    1                 0 up to 59                     |       |             |
 * | +-----+-------------------------------------------+ |       |             |
 * | |0xAF |                 DATA                      | ---->   |             |
 * | |     |                                           | |       |             |
 * | +-----+-------------------------------------------+ |       |    1        |
 * | status                                              |       | +----+      |
 * |                                                     |   <---- |0xAF|      |
 * |                                                     |       | +----+      |
 * |                                                     |   or  |             |
 * |                                                     |       |    1        |
 * |                                                     |       | +----+      |
 * |                                                     |   <---- | EC |      |
 * |                                                     |       | +----+      |
 * +-----------------------------------------------------+       +-------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
static int WriteData(mifare_tag *tag,  uint8_t command, uint8_t file_no, 
                     uint32_t offset, uint32_t length, uint8_t *data, 
                     size_t *data_sent, uint8_t communication_mode)
{
  uint8_t *p;
  ssize_t nbytes;
  size_t bytes_left /* in d */, bytes_sent, overhead_size, frame_bytes;
  BUFFER_INIT(d, FRAME_PAYLOAD_SIZE);
  /* cmd buffer must be big enough to contain 8+length+CMAC_LENGTH */
  BUFFER_INIT(cmd, 8 + MF_MAX_WRITE_LENGTH + CMAC_LENGTH);
  bytes_sent = 0;
  
  ASSERT_ACTIVE(tag);
  ASSERT_CS(communication_mode);
  
  BUFFER_APPEND(cmd, command);
  BUFFER_APPEND(cmd, file_no);
  BUFFER_APPEND_LE(cmd, offset, 3);
  BUFFER_APPEND_LE(cmd, length, 3);
  BUFFER_APPEND_BYTES(cmd, data, length);
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 8,
                                 communication_mode | MAC_COMMAND | 
                                 CMAC_COMMAND | ENC_COMMAND);
  overhead_size = BUFFER_SIZE(cmd) - length; /* (CRC | padding) + headers */
  
  bytes_left = FRAME_PAYLOAD_SIZE - 8; /* todo: why is this being done? */
  
  while(bytes_sent < BUFFER_SIZE(cmd)) {
    /* number of bytes to be copied into the frame is the smaller of the bytes
     * left in d, and the bytes left in the original command buffer */
    frame_bytes = MIN(bytes_left, BUFFER_SIZE(cmd) - bytes_sent);
    BUFFER_APPEND_BYTES(d, p+bytes_sent, frame_bytes);
    
    MifareCommTCL(BUFFER_ARRAY(d), BUFFER_SIZE(d));
    
    bytes_sent += frame_bytes;
    
    if (0x00 == MF_RXSTA) { /* break on operation ok */
      break;
    }
    
    /* PICC returned 0xAF and expects more data */
    BUFFER_CLEAR(d);
    BUFFER_APPEND(d, 0xAF);
    bytes_left = FRAME_PAYLOAD_SIZE - 1;
  }
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
      
  if(!p)
    return FAIL;
  
  if (0x00 == MF_RXSTA) {
    bytes_sent -= overhead_size; /* remove header_length */
  } else {
    /* 0xAF (additional frame) failure can happen here (wrong crypto method) */
    tag->last_picc_error = MF_RXSTA;
    bytes_sent = -1;
  }
  
  *data_sent = bytes_sent;
    
  return SUCCESS; 
}


/*
 * MifareWriteData
 * Description:
 *  Write data to Standard Data Files and Backup Data Files, with communication
 *  mode to be determined.
 *
 * Arguments
 *  tag:                DESFire tag
 *  file_no:            DESFire File IDentifier
 *  offset:             starting position for write within the file
 *  length:             number of bytes to be written.
 *  data:               buffer of data bytes to be written
 *  data_sent:          actual number of data bytes written
 *  
 * Operation:
 *  Call MifareWriteDataEx using the Communication mode returned by
 *  GetWriteCommunicationSettings()
 * 
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareWriteData(mifare_tag *tag,  uint8_t file_no, uint32_t offset, 
                           uint32_t length, uint8_t *data, size_t *data_sent)
{
  return MifareWriteDataEx(tag, file_no, offset, length, data, data_sent,
                           GetWriteCommunicationSettings(tag, file_no));
}


/*
 * MifareWriteDataEx
 * Description:
 *  Write data to Standard Data Files and Backup Data Files, with an explicit
 *  communication mode
 *
 * Arguments
 *  tag:                DESFire tag
 *  file_no:            DESFire File IDentifier
 *  offset:             starting position for write within the file
 *  length:             number of bytes to be written.
 *  data:               buffer of data bytes to be written
 *  data_sent:          actual number of data bytes written
 *  communication_mode: file's communication mode
 *  
 * Operation:
 *  Call WriteData using the command for data files (0x3D)
 * 
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareWriteDataEx(mifare_tag *tag,  uint8_t file_no, uint32_t offset,
                      uint32_t length, uint8_t *data, size_t *data_sent, 
                      uint8_t communication_mode)
{
  return WriteData(tag,  0x3D, file_no, offset, length, data, data_sent,
                   communication_mode);
}


/*
 * MifareGetValue
 * Description:
 *  Read the currently stored value from a Value File. The communication mode
 *  will be determined.
 *
 * Arguments
 *  tag:                DESFire tag
 *  file_no:            DESFire File IDentifier
 *  value:              where to store value [will be updated]
 *
 * Operation:
 *  Call MifareGetValueEx using the Communication mode returned by
 *  GetReadCommunicationSettings()
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareGetValue(mifare_tag *tag, uint8_t file_no, int32_t *value)
{
  return MifareGetValueEx(tag, file_no, value,
                          GetReadCommunicationSettings(tag, file_no));
}

/*
 * MifareGetValueEx
 * Description:
 *  Read the currently stored value from a Value File, with an explicit
 *  communication mode
 *
 * Arguments
 *  tag:                DESFire tag
 *  file_no:            DESFire File IDentifier
 *  value:              where to store value [will be updated]
 *  communication_mode: file communication mode
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |                     1    1      |       |                                 |
 * |                  +----+-------+ |       |                                 |
 * |                  |0x6C|File No| ---->   |                                 |
 * |                  +----+-------+ |       |    1                            |
 * |                   cmd           |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1   4(plain),8(enc),8(MACed) |
 * |                                 |       | +----+------------------------+ |
 * |                                 |   <---- |0x00|          DATA          | |
 * |                                 |       | +----+------------------------+ |
 * |                                 |       | status  LSB                MSB  |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareGetValueEx(mifare_tag *tag, uint8_t file_no, int32_t *value,
                            uint8_t communication_mode)
{
  BUFFER_INIT(cmd, 2+CMAC_LENGTH);
  uint8_t *p;
  ssize_t nbytes;
  
  ASSERT_ACTIVE(tag);
  ASSERT_CS(communication_mode);
  
  BUFFER_APPEND(cmd, 0x6C);
  BUFFER_APPEND(cmd, file_no);
      
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  communication_mode | MDCM_PLAIN | 
                                  CMAC_COMMAND | CMAC_VERIFY);
  
  if(!p)
    return FAIL;
  
  *value = le32toh(p);
  
  return SUCCESS; 
}


/*
 * MifareCredit
 * Description:
 *  Increase the value stored in a Value File, with a communication mode to be
 *  determined.
 *
 * Arguments
 *  tag:                DESFire tag
 *  file_no:            DESFire File IDentifier
 *  amount:             credit value
 *
 * Operation:
 *  Call MifareCreditEx using the Communication mode returned by
 *  GetWriteCommunicationSettings()
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareCredit(mifare_tag *tag, uint8_t file_no, int32_t amount) 
{
  return MifareCreditEx(tag, file_no, amount,
                        GetWriteCommunicationSettings(tag, file_no));
}


/*
 * MifareCreditEx
 * Description:
 *  Increase a value stored in a Value File, with an explicit communication mode
 *
 * Arguments:
 *  tag:                DESFire tag
 *  file_no:            DESFire File IDentifier
 *  amount:             credit value
 *  communication_mode: file's communication mode
 *
 * Operation:
 * +-----------------------------------------+       +-------------------------+
 * |                   PCD:                  |       |            PICC:        |
 * |    1     1    4(plain),8(enc),8(MACed)  |       |                         |
 * | +----+-------+------------------------+ |       |                         |
 * | |0x0C|File No|          DATA          | ---->   |                         |
 * | +----+-------+------------------------+ |       |    1                    |
 * |  cmd          LSB                  MSB  |       | +----+                  |
 * |                                         |   <---- | EC |status            |
 * |                                         |       | +----+                  |
 * |                                         |   or  |                         |
 * |                                         |       |    1                    |
 * |                                         |       | +----+                  |
 * |                                         |   <---- |0x00|                  |
 * |                                         |       | +----+                  |
 * |                                         |       | status                  |
 * +-----------------------------------------+       +-------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareCreditEx(mifare_tag *tag, uint8_t file_no, int32_t amount,
                          uint8_t communication_mode)
{
  BUFFER_INIT(cmd, 10+CMAC_LENGTH);
  uint8_t *p;
  ssize_t nbytes;
  
  ASSERT_ACTIVE(tag);
  ASSERT_CS(communication_mode);
  
  BUFFER_APPEND(cmd, 0x0C);
  BUFFER_APPEND(cmd, file_no);
  BUFFER_APPEND_LE(cmd, amount, 4);
    
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 2,
                                 communication_mode | MAC_COMMAND | 
                                 CMAC_COMMAND | ENC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  
  if(!p)
    return FAIL;
  
  return SUCCESS;
}


/*
 * MifareDebit
 * Description:
 *  Decrease the value stored in a Value File, with a communication mode to be
 *  determined.
 *
 * Arguments
 *  tag:                DESFire tag
 *  file_no:            DESFire File IDentifier
 *  amount:             debit value
 *
 * Operation:
 *  Call MifareDebitEx using the Communication mode returned by
 *  GetWriteCommunicationSettings()
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareDebit(mifare_tag *tag, uint8_t file_no, int32_t amount) 
{
  return MifareDebitEx(tag, file_no, amount,
                       GetWriteCommunicationSettings(tag, file_no));
}


/*
 * MifareDebitEx
 * Description:
 *  Decrease a value stored in a value file, with an explicit communication mode
 *
 * Arguments:
 *  tag:                DESFire tag
 *  file_no:            DESFire File IDentifier
 *  amount:             debit value
 *  communication_mode: file's communication mode
 *
 * Operation:
 * +-----------------------------------------+       +-------------------------+
 * |                   PCD:                  |       |            PICC:        |
 * |    1     1    4(plain),8(enc),8(MACed)  |       |                         |
 * | +----+-------+------------------------+ |       |                         |
 * | |0xDC|File No|          DATA          | ---->   |                         |
 * | +----+-------+------------------------+ |       |    1                    |
 * |  cmd          LSB                  MSB  |       | +----+                  |
 * |                                         |   <---- | EC |status            |
 * |                                         |       | +----+                  |
 * |                                         |   or  |                         |
 * |                                         |       |    1                    |
 * |                                         |       | +----+                  |
 * |                                         |   <---- |0x00|                  |
 * |                                         |       | +----+                  |
 * |                                         |       | status                  |
 * +-----------------------------------------+       +-------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareDebitEx(mifare_tag *tag, uint8_t file_no, int32_t amount,
                          uint8_t communication_mode)
{
  BUFFER_INIT(cmd, 10+CMAC_LENGTH);
  uint8_t *p;
  ssize_t nbytes;
  
  ASSERT_ACTIVE(tag);
  ASSERT_CS(communication_mode);
  
  BUFFER_APPEND(cmd, 0xDC);
  BUFFER_APPEND(cmd, file_no);
  BUFFER_APPEND_LE(cmd, amount, 4);
    
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 2,
                                 communication_mode | MAC_COMMAND | 
                                 CMAC_COMMAND | ENC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  
  if(!p)
    return FAIL;
  
  return SUCCESS;
}

/*
 * MifareLimitedCredit
 * Description:
 *  Limited increase of a value stored in a value file without having full
 *  Read&Write permissions to the file. This feature can be enabled or disabled
 *  during value file creation. This function is called with a communication
 *  mode to be determined.
 *
 * Arguments
 *  tag:                DESFire tag
 *  file_no:            DESFire File IDentifier
 *  amount:             credit value
 *
 * Operation:
 *  Call MifareLimitedCreditEx using the Communication mode returned by
 *  GetWriteCommunicationSettings()
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareLimitedCredit(mifare_tag *tag, uint8_t file_no, int32_t amount) 
{
  return MifareLimitedCreditEx(tag, file_no, amount,
                               GetWriteCommunicationSettings(tag, file_no));
}

/*
 * MifareLimitedCreditEx
 * Description:
 *  Limited increase of a value stored in a value file without having full
 *  Read&Write permissions to the file. This feature can be enabled or disabled
 *  during value file creation. This function is called with an explicit
 *  communication mode.
 *
 * Arguments:
 *  tag:                DESFire tag
 *  file_no:            DESFire File IDentifier
 *  amount:             credit value
 *  communication_mode: file's communication mode
 *
 * Operation:
 * +-----------------------------------------+       +-------------------------+
 * |                   PCD:                  |       |            PICC:        |
 * |    1     1    4(plain),8(enc),8(MACed)  |       |                         |
 * | +----+-------+------------------------+ |       |                         |
 * | |0x1C|File No|          DATA          | ---->   |                         |
 * | +----+-------+------------------------+ |       |    1                    |
 * |  cmd          LSB                  MSB  |       | +----+                  |
 * |                                         |   <---- | EC |status            |
 * |                                         |       | +----+                  |
 * |                                         |   or  |                         |
 * |                                         |       |    1                    |
 * |                                         |       | +----+                  |
 * |                                         |   <---- |0x00|                  |
 * |                                         |       | +----+                  |
 * |                                         |       | status                  |
 * +-----------------------------------------+       +-------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareLimitedCreditEx(mifare_tag *tag, uint8_t file_no, int32_t amount,
                          uint8_t communication_mode)
{
  BUFFER_INIT(cmd, 10+CMAC_LENGTH);
  uint8_t *p;
  ssize_t nbytes;
  
  ASSERT_ACTIVE(tag);
  ASSERT_CS(communication_mode);
  
  BUFFER_APPEND(cmd, 0x1C);
  BUFFER_APPEND(cmd, file_no);
  BUFFER_APPEND_LE(cmd, amount, 4);
    
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 2,
                                 communication_mode | MAC_COMMAND | 
                                 CMAC_COMMAND | ENC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  
  if(!p)
    return FAIL;
  
  return SUCCESS;
}


/*
 * MifareWriteRecord
 * Description:
 *  Write data to a record in a Cyclic or Linear Record File, with communication
 *  mode to be determined.
 *
 * Arguments
 *  tag:                DESFire tag
 *  file_no:            DESFire File IDentifier
 *  offset:             starting position for write within the file
 *  length:             number of bytes to be written.
 *  data:               buffer of data bytes to be written
 *  data_sent:          actual number of data bytes written
 *  
 * Operation:
 *  Call MifareWriteRecordEx using the Communication mode returned by
 *  GetWriteCommunicationSettings()
 * 
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareWriteRecord(mifare_tag *tag,  uint8_t file_no, uint32_t offset, 
                      uint32_t length, uint8_t *data, size_t *data_sent)
{
  return MifareWriteRecordEx(tag, file_no, offset, length, data, data_sent,
                             GetWriteCommunicationSettings(tag, file_no));
}


/*
 * MifareWriteRecordEx
 * Description:
 *  Write data to a record in a Cyclic or Linear Record File, with an explicit
 *  communication mode
 *
 * Arguments
 *  tag:                DESFire tag
 *  file_no:            DESFire File IDentifier
 *  offset:             starting position for write within the file
 *  length:             number of bytes to be written.
 *  data:               buffer of data bytes to be written
 *  data_sent:          actual number of data bytes written
 *  communication_mode: file's communication mode
 *  
 * Operation:
 *  Call WriteData using the command for record files (0x3B)
 * 
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareWriteRecordEx(mifare_tag *tag,  uint8_t file_no, uint32_t offset,
                      uint32_t length, uint8_t *data, size_t *data_sent, 
                      uint8_t communication_mode)
{
  return WriteData(tag,  0x3B, file_no, offset, length, data, data_sent,
                   communication_mode);
}


/*
 * MifareReadRecords
 * Description:
 *  Read out a set of complete records from a Cyclic or Linear Record File, with
 *  communication mode to be determined.
 *
 * Arguments
 *  tag:                DESFire tag
 *  file_no:            DESFire File IDentifier
 *  offset:             starting position for read within the file
 *  length:             number of bytes to be read
 *  data:               buffer to receive the data [pre-allocated]
 *  max_count           maximum data length to read.
 *  data_size:          actual number of bytes read.
 *  
 * Operation:
 *  Call MifareReadRecordsEx using the Communication mode returned by
 *  GetReadCommunicationSettings()
 * 
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareReadRecords(mifare_tag *tag, uint8_t file_no, uint32_t offset, 
                      uint32_t length, uint8_t data[], uint32_t max_count,
                      ssize_t *data_size)
{
  return MifareReadRecordsEx(tag, file_no, offset, length, data, max_count, 
                             data_size,
                             GetReadCommunicationSettings(tag,file_no));
}


/*
 * MifareReadRecordsEx
 * Description:
 *  Read out a set of complete records from a Cyclic or Linear Record File,
 *  with explicit communication mode.
 *
 * Arguments
 *  tag:                DESFire tag
 *  file_no:            DESFire File IDentifier
 *  offset:             starting position for read within the file
 *  length:             number of bytes to be read
 *  data:               buffer to receive the data [pre-allocated]
 *  max_count           maximum data length to read.
 *  data_size:          actual number of bytes read.
 *  communication_mode: file's communication mode
 *
 * Operation:
 *  Call ReadData with appropriate command for Record Files (0xBB).
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareReadRecordsEx(mifare_tag *tag, uint8_t file_no, uint32_t offset, 
                        uint32_t length, uint8_t data[], uint32_t max_count, 
                        ssize_t *data_size, uint8_t communication_mode)
{
  return ReadData(tag, 0xBB, file_no, offset, length, data, max_count, 
                  data_size, communication_mode);
}


/*
 * MifareClearRecordFile
 * Description:
 *  Reset a Cyclic or Linear Record File to the empty state.
 *
 * Arguments
 *  tag:                DESFire tag
 *  file_no:            DESFire File IDentifier
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |                     1    1      |       |                                 |
 * |                  +----+-------+ |       |                                 |
 * |                  |0xEB|File No| ---->   |                                 |
 * |                  +----+-------+ |       |    1                            |
 * |                   cmd           |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1                            |
 * |                                 |       | +----+                          |
 * |                                 |   <---- |0x00|                          |
 * |                                 |       | +----+                          |
 * |                                 |       | status                          |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareClearRecordFile(mifare_tag *tag, uint8_t file_no)
{
  BUFFER_INIT(cmd, 2+CMAC_LENGTH);
  uint8_t *p;
  ssize_t nbytes;
  
  ASSERT_ACTIVE(tag);
    
  BUFFER_APPEND(cmd, 0xEB);
  BUFFER_APPEND(cmd, file_no);
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  
  if(!p)
    return FAIL;
  
  return SUCCESS;
}


/*
 * MifareCommitTransaction
 * Description:
 *  Validate all previous write access to files with integrated backup 
 *  mechanisms within one application: backup data files, data files and
 *  record files.
 *  
 * Arguments
 *  tag: DESFire tag
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |                             1   |       |                                 |
 * |                          +----+ |       |                                 |
 * |                          |0xC7| ---->   |                                 |
 * |                          +----+ |       |    1                            |
 * |                            cmd  |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1                            |
 * |                                 |       | +----+                          |
 * |                                 |   <---- |0x00|status                    |
 * |                                 |       | +----+                          |
 * |                                 |       |                                 |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareCommitTransaction(mifare_tag *tag)
{
  BUFFER_INIT(cmd, 1+CMAC_LENGTH);
  uint8_t *p;
  ssize_t nbytes;
  
  ASSERT_ACTIVE(tag);
  
  BUFFER_APPEND(cmd, 0xC7);
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  
  if(!p)
    return FAIL;
  
  return SUCCESS;
}


/*
 * MifareAbortTransaction
 * Description:
 *  Invalidate all previous write access to files with integrated backup 
 *  mechanisms within one application: backup data files, data files and
 *  record files.
 *  
 * Arguments
 *  tag: DESFire tag
 *
 * Operation:
 * +---------------------------------+       +---------------------------------+
 * |           PCD:                  |       |            PICC:                |
 * |                             1   |       |                                 |
 * |                          +----+ |       |                                 |
 * |                          |0xA7| ---->   |                                 |
 * |                          +----+ |       |    1                            |
 * |                            cmd  |       | +----+                          |
 * |                                 |   <---- | EC |status                    |
 * |                                 |       | +----+                          |
 * |                                 |   or  |                                 |
 * |                                 |       |    1                            |
 * |                                 |       | +----+                          |
 * |                                 |   <---- |0x00|status                    |
 * |                                 |       | +----+                          |
 * |                                 |       |                                 |
 * +---------------------------------+       +---------------------------------+
 *
 * Return:  
 *  SUCCESS: command executed successfully
 *  FAIL:    failed command execution
 *
 * Revision History:
 *  Mar. 23, 2013      Nnoduka Eruchalu     Initial Revision
 */
int MifareAbortTransaction(mifare_tag *tag)
{
  BUFFER_INIT(cmd, 1+CMAC_LENGTH);
  uint8_t *p;
  ssize_t nbytes;
  
  ASSERT_ACTIVE(tag);
    
  BUFFER_APPEND(cmd, 0xA7);
  
  p = MifareCryptoPreprocessData(tag, BUFFER_ARRAY(cmd), &BUFFER_SIZE(cmd), 0,
                                 MDCM_PLAIN | CMAC_COMMAND);
  
  MifareCommTCL(p, BUFFER_SIZE(cmd));
  
  nbytes = MF_RXLEN;                /* number of Rx'd bytes */
  p = MifareCryptoPostprocessData(tag, MF_RXDATA, &nbytes, 
                                  MDCM_PLAIN | CMAC_COMMAND | CMAC_VERIFY);
  
  if(!p)
    return FAIL;
  
  return SUCCESS;
}
