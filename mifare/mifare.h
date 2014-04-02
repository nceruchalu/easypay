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
 */

#ifndef MIFARE_H
#define MIFARE_H

/* library include files */
#include <stdint.h>     /* for uint*_t */
#include <stdlib.h>     /* for size_t */

/* local include files */
#include "des.h"
#include "../general.h"    /* for SUCCESS and FAIL */


/* -----------------------------------------------------
 * Buffer management macros: These will make life easier
 * -----------------------------------------------------
 */
/* 
 * buffer is represented by a struct containing a buffer and its number of  
 * filled slots
 */
#define BUFFER(size)  struct {size_t n; uint8_t array[size];}

/* 
 * initialize a buffer named buffer_name of size bytes
 *   set n, number of filled slots, to 0
 */
#define BUFFER_INIT(buffer_name, size) \
  BUFFER(size) buffer_name = {0, {0}}

/*
 * get buffer size
 */
#define BUFFER_SIZE(buffer_name) (buffer_name.n)

/*
 * get buffer array
 */
#define BUFFER_ARRAY(buffer_name) (buffer_name.array)

/*
 * clear buffer
 */
#define BUFFER_CLEAR(buffer_name) (buffer_name.n = 0)

/*
 * append one byte of data to the buffer buffer_name
 */
#define BUFFER_APPEND(buffer_name, data) \
  (buffer_name.array[buffer_name.n++] = data)

/*
 * append size bytes of data to the buffer buffer_name
 */
#define BUFFER_APPEND_BYTES(buffer_name, data, size)               \
  do {                                                             \
    unsigned int n = 0;                                            \
    unsigned int buf_size = sizeof(buffer_name.array);             \
    while((n < size) && (buffer_name.n < buf_size)){               \
      buffer_name.array[buffer_name.n++] = ((uint8_t *)data)[n++]; \
    }                                                              \
  } while(0)


/*
 * append size bytes of data in (Little Endian) to the buffer buffer_name 
 * data is assumed to be a fixed width integer type with max width of 4 bytes
 */
#define BUFFER_APPEND_LE(buffer_name, data, size)                  \
  do {                                                             \
    unsigned int n = 0;                                            \
    unsigned int buf_size = sizeof(buffer_name.array);             \
    uint32_t temp = (uint32_t) data;                               \
    while((n < size) && (buffer_name.n < buf_size)){               \
      buffer_name.array[buffer_name.n++]=(uint8_t)(temp & 0x00FF); \
      n++;                                                         \
      temp >>= 8;                                                  \
    }                                                              \
  } while(0)
 



/* --------------------------------------
 * MIFARE Communication Constants
 * --------------------------------------
 */
#define MIFARE_TIMERCOUNT  200     /* ms to wait before communication timeout */


/* --------------------------------------
 * DESFire Commands Constants
 * --------------------------------------
 */
#define MF_MAX_WRITE_LENGTH 60


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
#define MIFARE_MAX_APPLICATION_COUNT 28 /* max applications on one PICC */
#define MIFARE_MAX_FILE_COUNT        16 /* max # of files in each application */
#define MIFARE_UID_BYTES             7  /* number of UID bytes */
#define MIFARE_AID_SIZE              3  /* number of AID bytes */


/* --------------------------------------
 * DESFire Authentication Commands
 * --------------------------------------
 */
#define MF_AUTHENTICATE_LEGACY 0x0A
#define MF_AUTHENTICATE_ISO    0x1A
#define MF_AUTHENTICATE_AES    0xAA


/* --------------------------------------
 * DESFire File Types
 * --------------------------------------
 */
enum mifare_desfire_file_types {
  MDFT_STANDARD_DATA_FILE             = 0x00, 
  MDFT_BACKUP_DATA_FILE               = 0x01,
  MDFT_VALUE_FILE_WITH_BACKUP         = 0x02,   
  MDFT_LINEAR_RECORD_FILE_WITH_BACKUP = 0x03, 
  MDFT_CYCLIC_RECORD_FILE_WITH_BACKUP = 0x04    
};

/* --------------------------------------
 * DESFire Communication Modes
 * --------------------------------------
 */
#define MDCM_PLAIN         0x0000    /* Plain Communication */
#define MDCM_MACED         0x0001    /* Plain Comm secured by DES/3DES MACing */
#define MDCM_ENCIPHERED    0x0003    /* Fully DES/3DES enciphered comm. */

/* --------------------------------------
 * DESFire Communication Constants (OR'd in with Comm. Modes)
 * --------------------------------------
 */
#define MDCM_MASK      0x000F /* extract communication mode */

#define CMAC_NONE      0
#define CMAC_COMMAND   0x0010 /* data send to the PICC is used to update CMAC */
#define CMAC_VERIFY    0x0020 /* data from the PICC is used to update CMAC */

#define MAC_COMMAND    0x0100 /* MAC the command (when MDCM_MACED) */
#define MAC_VERIFY     0x0200 /* cmd returns a MAC to verify (when MDCM_MACED)*/

#define ENC_COMMAND    0x1000
#define NO_CRC         0x2000

#define CMAC_MASK      0x00F0
#define MAC_MASK       0x0F00

#define MAC_LENGTH     4     /* MAC is 4 bytes */
#define CMAC_LENGTH    8     /* CMAC is 8 bytes */


/* --------------------------------------
 * DESFire Communication settings
 * --------------------------------------
 */
#define MAX_CRYPTO_BLOCK_SIZE  16
#define MAX_FRAME_SIZE         60
#define FRAME_PAYLOAD_SIZE     (MAX_FRAME_SIZE - 5)
#define NOT_YET_AUTHENTICATED  255        /* an impossible auth. key no */

/*
 * crypto buffer size set by:
 * - PaddedDataLength     <=  nbytes + (MAX_CRYPTO_BLOCK_SIZE-1)
 * - MacedDataLength      <=  nbytes + CMAC_LENGTH
 * - EncipheredDataLength <=  nbytes + 4 (Crc32) + (MAX_CRYPTO_BLOCK_SIZE-1)
 * So EncipheredDataLength needs most space
 */
#define MAX_PADDED_DATA_LENGTH      MAX_FRAME_SIZE + MAX_CRYPTO_BLOCK_SIZE - 1
#define MAX_MACED_DATA_LENGTH       MAX_FRAME_SIZE + CMAC_LENGTH
#define MAX_ENCIPHERED_DATA_LENGTH  MAX_FRAME_SIZE +4 +MAX_CRYPTO_BLOCK_SIZE -1
#define MAX_CRYPTO_BUFFER_SIZE      MAX_ENCIPHERED_DATA_LENGTH


/* --------------------------------------
 * DESFire EV1 Application Crypto Operations
 * --------------------------------------
 */
#define APPLICATION_CRYPTO_DES    0x00
#define APPLICATION_CRYPTO_3K3DES 0x40
#define APPLICATION_CRYPTO_AES    0x80


/* --------------------------------------
 * DESFire Access Rights
 * --------------------------------------
 */

/* create access right word  
 *  15         12 11           8 7                 4 3                    0
 * |-------------|--------------|-------------------|----------------------|
 * | Read Access | Write Access | Read&Write Access | Change Access Rights |
 * |_____________|______________|___________________|______________________|
 */
#define MDAR(read,write,read_write,change_access_rights) \
  ((read << 12) | (write << 8) | (read_write << 4 ) | (change_access_rights))

/* extract access right nibbles */
#define MDAR_READ(ar)           (((ar) >> 12) & 0x0F)
#define MDAR_WRITE(ar)          (((ar) >>  8) & 0x0F)
#define MDAR_READ_WRITE(ar)     (((ar) >>  4) & 0x0F)
#define MDAR_CHANGE_AR(ar)      ((ar)         & 0x0F)

/* access right keys -- access right nibble values */
#define MDAR_KEY0              0x00
#define MDAR_KEY1              0x01
#define MDAR_KEY2              0x02
#define MDAR_KEY3              0x03
#define MDAR_KEY4              0x04
#define MDAR_KEY5              0x05
#define MDAR_KEY6              0x06
#define MDAR_KEY7              0x07
#define MDAR_KEY8              0x08
#define MDAR_KEY9              0x09
#define MDAR_KEY10             0x0A
#define MDAR_KEY11             0x0B
#define MDAR_KEY12             0x0C
#define MDAR_KEY13             0x0D
#define MDAR_FREE              0x0E
#define MDAR_DENY              0x0F


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
 * DESFire macros
 * --------------------------------------
 */
#define ASSERT_ACTIVE(tag)       if(!tag->active) return FAIL
#define ASSERT_INACTIVE(tag)     if(tag->active) return FAIL
#define ASSERT_AUTHENTICATED(tag) \
    if(tag->authenticated_key_no == NOT_YET_AUTHENTICATED) return FAIL

/* cs != a DESFire communication mode, there is a CommunicationSettings error */
#define ASSERT_CS(cs) \
  if((cs!=MDCM_PLAIN) && (cs!=MDCM_MACED) && (cs!=MDCM_ENCIPHERED)) return FAIL


/* --------------------------------------
 * DESFire Data Objects
 * --------------------------------------
 */
typedef struct {
  uint8_t data[24];
  enum {
    T_DES,              /* 56-bit DES (single DES, DES) */
    T_3DES,             /* 112-bit 3DES (triple DES, 2K3DES) */
    T_3K3DES,           /* 168-bit 3DES (3 key triple DES, 3K3DES) */
    T_AES               /* AES-128 */
  } type;

  DES_key_schedule ks1; /* first component of a TDEA key  */
  DES_key_schedule ks2; /* second component of a TDEA key */
  DES_key_schedule ks3; /* third component of a TDEA key  */
  uint8_t cmac_sk1[24]; /* CMAC Subkey 1 */
  uint8_t cmac_sk2[24]; /* CMAC Subkey 2 */
  uint8_t aes_version;
} mifare_desfire_key;


typedef struct {
    uint8_t data[3];
} mifare_desfire_aid;


typedef struct {
  uint8_t active;
  uint8_t uid[7];
  uint8_t type;         /* One of SL032 Card Type Defines */
} mifare_tag_simple;


typedef struct {
  uint8_t active;
  uint8_t uid[7];
  uint8_t type;         /* One of SL032 Card Type Defines */
  
  /* desfire specific section */
  uint8_t last_picc_error;
  uint8_t last_pcd_error;
  mifare_desfire_key session_key;
  enum { AS_LEGACY, AS_NEW } authentication_scheme;
  uint8_t authenticated_key_no;
  uint8_t ivect[MAX_CRYPTO_BLOCK_SIZE];         /* init vector for CBC */
  uint8_t cmac[16];
  uint8_t crypto_buffer[MAX_CRYPTO_BUFFER_SIZE];
  uint32_t selected_application;
} mifare_tag;

typedef struct {             /* structure for the GetDfNames command */
  uint32_t aid;
  uint16_t fid;
  uint8_t df_name[16];
  size_t df_name_len;
} mifare_desfire_df;


typedef struct {
  struct {
    uint8_t vendor_id;
    uint8_t type;
    uint8_t subtype;
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t storage_size;
    uint8_t protocol;
  } hardware;
  
  struct {
    uint8_t vendor_id;
    uint8_t type;
    uint8_t subtype;
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t storage_size;
    uint8_t protocol;
  } software;
  
  uint8_t uid[7];
  uint8_t batch_number[5];
  uint8_t production_week;
  uint8_t production_year;
} mifare_desfire_version_info;


typedef struct {
  uint8_t file_type;
  uint8_t communication_settings;
  uint8_t access_rights[2];
  
  union {
    struct {
      uint8_t file_size[3];
    } standard_file;
    struct {
      int8_t lower_limit[4];
      int8_t upper_limit[4];
      int8_t limited_credit_value[4];
      uint8_t limited_credit_enabled;
    } value_file;
    struct {
      uint8_t record_size[3];
      uint8_t max_number_of_records[3];
      uint8_t current_number_of_records[3];
    } record_file;
  } settings;
  
} mifare_desfire_raw_file_settings;


typedef struct {
  uint8_t file_type;
  uint8_t communication_settings;
  uint16_t access_rights;
  
  union {
    struct {
      uint32_t file_size;
    } standard_file;
    struct {
      int32_t lower_limit;
      int32_t upper_limit;
      int32_t limited_credit_value;
      uint8_t limited_credit_enabled;
    } value_file;
    struct {
      uint32_t record_size;
      uint32_t max_number_of_records;
      uint32_t current_number_of_records;
    } record_file;                        /* linear and cyclic record files */
  } settings;
} mifare_desfire_file_settings;


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

/* get a buffer of bytes to the serial channel */
extern void MifareGetBuf(void);

/* communicate via T=CL; send command, get response */
extern int MifareCommTCL(unsigned char *buffer, unsigned char size);


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
extern int MifareDetect(mifare_tag_simple *tag);


/* establish connection to the provided tag */
extern int MifareConnect(mifare_tag *tag);

/* terminate connection with the provided tag */
extern int MifareDisconnect(mifare_tag *tag);


/* --------------------------------------
 * Commands - as described in datasheet
 * --------------------------------------
 */ 

/* Security Related Commands */
extern int MifareAuthenticate(mifare_tag *tag, uint8_t key_no, 
                              mifare_desfire_key *key);
extern int MifareAuthenticateIso(mifare_tag *tag, uint8_t key_no, 
                                 mifare_desfire_key *key); /* EV1 only */
extern int MifareAuthenticateAes(mifare_tag *tag, uint8_t key_no, 
                                 mifare_desfire_key *key); /* EV1 only */
extern int MifareChangeKeySettings(mifare_tag *tag, uint8_t settings);

extern int MifareSetConfiguration(mifare_tag *tag, uint8_t disable_format, 
                                  uint8_t enable_random_uid); /* EV1 only */
/* following two are EV1 only but not in datasheets I have */
extern int MifareSetDefaultKey(mifare_tag *tag, mifare_desfire_key *key);
extern int MifareSetAts(mifare_tag *tag, uint8_t *ats);

extern int MifareGetKeySettings(mifare_tag *tag, uint8_t *settings,
                                uint8_t *max_keys);
extern int MifareChangeKey(mifare_tag *tag, uint8_t key_no, 
                           mifare_desfire_key *new_key,
                           mifare_desfire_key *old_key);
extern int MifareGetKeyVersion(mifare_tag *tag,uint8_t key_no,
                               uint8_t *version);


/* PICC Level Commands */
extern int MifareCreateApplication(mifare_tag *tag, uint32_t aid,
                                   uint8_t settings, uint8_t no_keys);
extern int MifareCreateApplicationIso(mifare_tag *tag, uint32_t aid,
                                      uint8_t settings, uint8_t no_keys,
                                      uint8_t want_iso_file_identifiers,
                                      uint8_t iso_file_id,
                                      uint8_t *iso_file_name,
                                      size_t iso_file_name_len);
extern int MifareCreateApplication3k3Des(mifare_tag *tag, uint32_t aid,
                                         uint8_t settings, uint8_t no_keys);
extern int MifareCreateApplication3k3DesIso(mifare_tag *tag, uint32_t aid,
                                            uint8_t settings, uint8_t no_keys,
                                            uint8_t want_iso_file_identifiers,
                                            uint16_t iso_file_id,
                                            uint8_t *iso_file_name,
                                            size_t iso_file_name_len);
extern int MifareCreateApplicationAes(mifare_tag *tag, uint32_t aid,
                                      uint8_t settings, uint8_t no_keys);
extern int MifareCreateApplicationAesIso(mifare_tag *tag, uint32_t aid,
                                         uint8_t settings, uint8_t no_keys,
                                         uint8_t want_iso_file_identifiers,
                                         uint16_t iso_file_id,
                                         uint8_t *iso_file_name, 
                                         size_t iso_file_name_len);
extern int MifareDeleteApplication(mifare_tag *tag, uint32_t aid);
extern int MifareGetApplicationIds(mifare_tag *tag, uint32_t aids[],
                                   uint8_t aid_max_count, size_t *count);
extern int MifareGetFreeMem(mifare_tag *tag, uint32_t *size); /* EV1 only */
extern int MifareGetDfNames(mifare_tag *tag, mifare_desfire_df dfs[],
                            uint8_t df_max_count, size_t *count); /* EV1 only */
extern int MifareSelectApplication(mifare_tag *tag, uint32_t aid);
extern int MifareFormatPicc(mifare_tag *tag);
extern int MifareGetVersion(mifare_tag *tag, 
                            mifare_desfire_version_info *version_info);
extern int MifareGetCardUid(mifare_tag *tag, uint8_t uid[/*7*/]);  /* EV1 only*/


/* Application Level Commands */
extern int MifareGetFileIds(mifare_tag *tag, uint8_t files[],
                            uint8_t fid_max_count, size_t *count);
extern int MifareGetIsoFileIds(mifare_tag *tag, uint16_t files[],
                               uint8_t fid_max_count, size_t *count);/*EV1only*/
extern int MifareGetFileSettings(mifare_tag *tag, uint8_t file_no,
                                 mifare_desfire_file_settings *settings);
extern int MifareChangeFileSettings(mifare_tag *tag, uint8_t file_no,
                                    uint8_t communication_settings,
                                    uint16_t access_rights);
extern int MifareCreateStdDataFile(mifare_tag *tag, uint8_t file_no, 
                                   uint8_t communication_settings, 
                                   uint16_t access_rights, uint32_t file_size);
extern int MifareCreateStdDataFileIso(mifare_tag *tag, uint8_t file_no,
                                      uint8_t communication_settings,
                                      uint16_t access_rights, 
                                      uint32_t file_size, uint16_t iso_file_id);
extern int MifareCreateBackupDataFile(mifare_tag *tag, uint8_t file_no,
                                      uint8_t communication_settings, 
                                      uint16_t access_rights, 
                                      uint32_t file_size);
extern int MifareCreateBackupDataFileIso(mifare_tag *tag, uint8_t file_no, 
                                         uint8_t communication_settings,
                                         uint16_t access_rights, 
                                         uint32_t file_size, 
                                         uint16_t iso_file_id);
extern int MifareCreateValueFile(mifare_tag *tag, uint8_t file_no,
                                 uint8_t communication_settings,
                                 uint16_t access_rights, int32_t lower_limit, 
                                 int32_t upper_limit, int32_t value, 
                                 uint8_t limited_credit_enable);
extern int MifareCreateLinearRecordFile(mifare_tag *tag, uint8_t file_no, 
                                        uint8_t communication_settings, 
                                        uint16_t access_rights, 
                                        uint32_t record_size, 
                                        uint32_t max_number_of_records);
extern int MifareCreateLinearRecordFileIso(mifare_tag *tag, uint8_t file_no, 
                                           uint8_t communication_settings, 
                                           uint16_t access_rights, 
                                           uint32_t record_size, 
                                           uint32_t max_number_of_records, 
                                           uint16_t iso_file_id);
extern int MifareCreateCyclicRecordFile(mifare_tag *tag, uint8_t file_no, 
                                        uint8_t communication_settings,
                                        uint16_t access_rights,
                                        uint32_t record_size,
                                        uint32_t max_number_of_records);
extern int MifareCreateCyclicRecordFileIso(mifare_tag *tag, uint8_t file_no, 
                                           uint8_t communication_settings, 
                                           uint16_t access_rights, 
                                           uint32_t record_size, 
                                           uint32_t max_number_of_records, 
                                           uint16_t iso_file_id);
extern int MifareDeleteFile(mifare_tag *tag, uint8_t file_no);


/* Data Manipulation Commands */
extern int MifareReadData(mifare_tag *tag, uint8_t file_no, uint32_t offset, 
                          uint32_t length, uint8_t data[], uint32_t max_count, 
                          ssize_t *data_size);
extern int MifareReadDataEx(mifare_tag *tag, uint8_t file_no, uint32_t offset, 
                            uint32_t length, uint8_t data[], uint32_t max_count,
                            ssize_t *data_size,
                            uint8_t communication_mode);
extern int MifareWriteData(mifare_tag *tag,  uint8_t file_no, uint32_t offset, 
                           uint32_t length, uint8_t *data, size_t *data_sent);
extern int MifareWriteDataEx(mifare_tag *tag,  uint8_t file_no, uint32_t offset,
                             uint32_t length, uint8_t *data, size_t *data_sent, 
                             uint8_t communication_mode);
extern int MifareGetValue(mifare_tag *tag, uint8_t file_no, int32_t *value);
extern int MifareGetValueEx(mifare_tag *tag, uint8_t file_no, int32_t *value,
                            uint8_t communication_mode);
extern int MifareCredit(mifare_tag *tag, uint8_t file_no, int32_t amount);
extern int MifareCreditEx(mifare_tag *tag, uint8_t file_no, int32_t amount,
                          uint8_t communication_mode);
extern int MifareDebit(mifare_tag *tag, uint8_t file_no, int32_t amount);
extern int MifareDebitEx(mifare_tag *tag, uint8_t file_no, int32_t amount,
                         uint8_t communication_mode);
extern int MifareLimitedCredit(mifare_tag *tag, uint8_t file_no,
                               int32_t amount);
extern int MifareLimitedCreditEx(mifare_tag *tag, uint8_t file_no, 
                                 int32_t amount, uint8_t communication_mode);
extern int MifareWriteRecord(mifare_tag *tag,  uint8_t file_no, uint32_t offset,
                           uint32_t length, uint8_t *data, size_t *data_sent);
extern int MifareWriteRecordEx(mifare_tag *tag, uint8_t file_no, 
                               uint32_t offset, uint32_t length, uint8_t *data, 
                               size_t *data_sent, uint8_t communication_mode);
extern int MifareReadRecords(mifare_tag *tag, uint8_t file_no, uint32_t offset, 
                             uint32_t length, uint8_t data[], 
                             uint32_t max_count, ssize_t *data_size);
extern int MifareReadRecordsEx(mifare_tag *tag, uint8_t file_no,
                               uint32_t offset, uint32_t length, uint8_t data[],
                               uint32_t max_count, ssize_t *data_size, 
                               uint8_t communication_mode);
extern int MifareClearRecordFile(mifare_tag *tag, uint8_t file_no);
extern int MifareCommitTransaction(mifare_tag *tag);
extern int MifareAbortTransaction(mifare_tag *tag);

#endif                                                            /* MIFARE_H */
