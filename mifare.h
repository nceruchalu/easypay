/*
 * -----------------------------------------------------------------------------
 * -----                             MIFARE.H                              -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the header file for mifare.c, the library of functions for
 *   interfacing the PIC18F67K22 with the SL032, a MIFARE DESFire reader/writer
 *   module.
 *
 * Assumptions:
 *   None.
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Dec. 27, 2012      Nnoduka Eruchalu     Initial Revision
 *   May  03, 2013      Nnoduka Eruchalu     Removed dynamic memory allocation
 *                                           So changed functions to accept
 *                                           pointers to pre-allocated data
 *   May  04, 2013      Nnoduka Eruchalu     Added AuthenticateIso and 
 *                                           AuthenticateAes
 *   May  05, 2013      Nnoduka Eruchalu     Added MifareDetect
 *   May  06, 2013      Nnoduka Eruchalu     Added mifare_tag_simple struct and
 *                                           used for MifareDetect functions.
 *   May  07, 2013      Nnoduka Eruchalu     Simplified this for demo project
 */

#ifndef MIFARE_H
#define MIFARE_H

/* library include files */
#include <stdint.h>     /* for uint*_t */
#include <stdlib.h>     /* for size_t */


/* --------------------------------------
 * MIFARE Communication Constants
 * --------------------------------------
 */
#define MIFARE_TIMERCOUNT  200     /* ms to wait before communication timeout */


/* --------------------------------------
 * UART Status Words Define
 * --------------------------------------
 */
#define MF_UARTSTATUS_FREE   0
#define MF_UARTSTATUS_TX     1
#define MF_UARTSTATUS_TXSUCC 2
#define MF_UARTSTATUS_RXSUCC 3
#define MF_UARTSTATUS_RXERR  4


/* --------------------------------------
 * SL032 Card Type Define
 * --------------------------------------
 */
#define MIFARE_CARD_1K_4    0x01     /* Mifare 1k, 4 byte UID */
#define MIFARE_CARD_PRO     0x02     /* Mifare Pro */
#define MIFARE_CARD_UL      0x03     /* Mifare UltraLight*/
#define MIFARE_CARD_4K_4    0x04     /* Mifare 4k, 4 byte UID */
#define MIFARE_CARD_PRO_X   0x05     /* Mifare ProX */
#define MIFARE_CARD_DES     0x06     /* Mifare DesFire */
#define MIFARE_CARD_1K_7    0x07     /* Mifare 1k, 7 byte UID */
#define MIFARE_CARD_4K_7    0x08     /* Mifare 4k, 7 byte UID */
#define MIFARE_CARD_OTHER   0x0A     /* Other */


/* --------------------------------------
 * SL032 Commands
 * --------------------------------------
 */
#define SL_SELECT_CARD      0x01   /* select mifare card */
#define SL_RATS             0x20   /* Request for Answer to Select ISO14443-4 */
#define SL_TCL              0x21   /* Exchange Transparent Data with T=CL */


/* --------------------------------------
 * SL032 Status and Error Codes 
 * --------------------------------------
 */
#define SL_OPERATION_SUCC   0x00     /* operation success */
#define SL_NO_TAG           0x01     /* no tag */
#define SL_LOGIN_SUCC       0x02     /* login success */
#define SL_LOGIN_FAIL       0x03     /* login fail */
#define SL_READ_FAIL        0x04     /* read fail */
#define SL_WRITE_FAIL       0x05     /* write fail */
#define SL_UNABLE_READ      0x06     /* unable to read after write */
#define SL_ADDRESS_OF       0x08     /* address overflow */
#define SL_ATS_FAIL         0x10     /* ATS failed */
#define SL_TCL_FAIL         0x11     /* T=CL communication failed */
#define SL_COLLISION        0x0A     /* collision occur */
#define SL_NOT_AUTH         0x0D     /* not authenticate */
#define SL_NOT_VALUE        0x0E     /* not a value block */
#define SL_CHECKSUM_ERR     0xF0     /* checksum error */
#define SL_COMMAND_ERR      0xF1     /* command code error */


/* --------------------------------------
 * DESFire Logical Structure
 * --------------------------------------
 */
#define MIFARE_UID_BYTES             7  /* number of UID bytes */


/* --------------------------------------
 * DESFire Communication settings
 * --------------------------------------
 */
#define MAX_FRAME_SIZE         60


/* --------------------------------------
 * DESFire Status and Error Codes 
 * --------------------------------------
 */
#define MF_OPERATION_OK           0x00  /* successful operation */
#define MF_NO_CHANGES             0x0C  /* no changes done to backup files */
#define MF_OUT_OF_EEPROM_ERROR    0x0E  /*insufficient NV-Mem. to complete cmd*/
#define MF_ILLEGAL_COMMAND_CODE   0x1C  /* command code not supported */
#define MF_INTEGRITY_ERROR        0x1E  /* CRC or MAC does not match data */
#define MF_NO_SUCH_KEY            0x40  /* invalid key number specified */
#define MF_LENGTH_ERROR           0x7E  /* length of command string invalid */
#define MF_PERMISSION_ERROR       0x9D  /* curr conf/status doesnt allow cmd */
#define MF_PARAMETER_ERROR        0x9E  /* value of the parameter(s) invalid */
#define MF_APPLICATION_NOT_FOUND  0xA0  /* requested AID not present on PICC */
#define MF_APPL_INTEGRITY_ERROR   0xA1  /* unrecoverable err within app */
#define MF_AUTHENTICATION_ERROR   0xAE  /*cur auth status doesnt allow req cmd*/
#define MF_ADDITIONAL_FRAME       0xAF  /* additional data frame to be sent */
#define MF_BOUNDARY_ERROR         0xBE  /* attempt to read/write beyond limits*/
#define MF_PICC_INTEGRITY_ERROR   0xC1  /* unrecoverable error within PICC */
#define MF_COMMAND_ABORTED        0xCA  /*previous command not fully completed*/
#define MF_PICC_DISABLED_ERROR    0xCD  /*PICC disabled by unrecoverable error*/
#define MF_COUNT_ERROR            0xCE  /* cant create more apps, already @ 28*/
#define MF_DUPLICATE_ERROR        0xDE  /* cant create dup. file/app */
#define MF_EEPROM_ERROR           0xEE  /* couldnt complete NV-write operation*/
#define MF_FILE_NOT_FOUND         0xF0  /* specified file number doesnt exist */
#define MF_FILE_INTEGRITY_ERROR   0xF1  /* unrecoverable error within file */

/* Error Code Managed by this Library */
#define CRYPTO_ERROR              0x01  /* crypto verification error */


/* --------------------------------------
 * DESFire Data Objects
 * --------------------------------------
 */

typedef struct {
  uint8_t active;
  uint8_t uid[7];
  uint8_t type;         /* One of SL032 Card Type Defines */
} mifare_tag;



/* --------------------------------------
 * FUNCTION PROTOTYPES
 * --------------------------------------
 */
/* --------------------------------------
 * Mifare Timer routines
 * --------------------------------------
 */
/* start a countdown timer with a Timer */
extern void MifareStartTimer(unsigned int ms);

/* interrupt service routine for a Timer */
extern void MifareTimerISR(void);


/* --------------------------------------
 * SL032 specific functions
 * --------------------------------------
 */
/* output a buffer of bytes to the serial channel */
extern void MifarePutBuf(unsigned char *buffer, unsigned char size);

/* get a buffer of bytes from the serial channel */
extern void MifareGetBuf(void);


/* --------------------------------------
 * Memory Management functions
 * --------------------------------------
 */
/* initialize a MIFARE DESFire tag */
extern void MifareTagInit(mifare_tag *tag);


/* --------------------------------------
 * Card Communication Prep Functions
 * --------------------------------------
 */ 
/* Detect a card in the read-range of SL032 */
extern int MifareDetect(mifare_tag *tag);


/* establish connection to the provided tag */
extern int MifareConnect(mifare_tag *tag);

/* terminate connection with the provided tag */
extern int MifareDisconnect(mifare_tag *tag);


#endif                                                            /* MIFARE_H */
