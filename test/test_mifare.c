/*
 * -----------------------------------------------------------------------------
 * -----                          TEST_MIFARE.C                            -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is a test suite for routines defined in mifare.c
 *   This requires hardware.
 *
 * Compiler:
 *  HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *   
 * Revision History:
 *  May  04, 2013      Nnoduka Eruchalu     Initial Revision
 */
#include <stdint.h>
#include "../mifare/mifare.h"
#include "serial.h"
#include "mifare_auth.h"
#include "test_mifare.h"

static mifare_tag tag; /* make global so compiler can allocate space */

/* for newline append "\r\n" */
static void PrintMessage(const char *str) {
  while(*str != '\0') {
    SerialPutChar2(*str++);
  }
}

static void PrintBuffer(uint8_t *buffer, size_t len) {
  size_t i;
  for(i=0;i<len;i++) {
    SerialPutChar2(buffer[i]);
  }
}

static void PrintBufferHex(uint8_t *buffer, size_t len) {
  size_t i;
  char nibble, num;
  
  for(i=0;i<len;i++) {
    num = buffer[i];
    
    /* extract high nibble and write out numeric or alpha */
    nibble = (num & 0xF0) >> 4;
    if (nibble < 10) SerialPutChar2('0'+nibble);
    else             SerialPutChar2(nibble-10+'A');
  
    /* extract low nibble and write out numeric or alpha */
    nibble = (num & 0x0F);
    if (nibble < 10) SerialPutChar2('0'+nibble);
    else             SerialPutChar2(nibble-10+'A');
    
    SerialPutChar2(' '); /* need spacing between hex characters */
  }
  
}

static void PrintNewline(void) {
  PrintMessage("\r\n");
}

/*
 * main
 * Description:      This procedure is the main test for Mifare.c
 *
 * Arguments:        None.
 * Return:           None
 *
 * Input:            SmartCard
 * Output:           UART Channel 2
 *
 * Error Handling:   None
 *
 * Algorithms:       None
 * Data Strutures:   None
 *
 * Shared Variables: None
 *
 * Revision History:
 *  May  04, 2013      Nnoduka Eruchalu     Initial Revision
 */
void TestMifare(void) {
  mifare_desfire_version_info version_info;;

  MifareStartTimer(0);
  
  PrintMessage("Serial Channel Initialized -- starting test\r\n");
  
  /* initialize tag representation and connect to tag */
  MifareTagInit(&tag);
  MifareConnect(&tag);
  
  /* proof of successful connection */
  PrintMessage("\r\n");
  PrintMessage("Connected PICC's UID: ");
  PrintBufferHex(tag.uid, 7);
  PrintMessage("\r\n");

  /* select the master application */
  MifareSelectApplication(&tag, 0);
  PrintNewline();
  PrintMessage("Select master application\r\n");
  
  /* get version information */
  MifareGetVersion(&tag, &version_info);
  PrintNewline();
  PrintMessage("Version info  PICC:\r\n");
  PrintMessage("--> Hardware: "); PrintBufferHex(&version_info.hardware, 7);
  PrintNewLine();
  PrintMessage("--> Software: "); PrintBufferHex(&version_info.software, 7);
  PrintNewLine();
  PrintMessage("--> UID: "); PrintBufferHex(version_info.uid, 7);
  PrintNewLine();
  PrintMessage("-->batch_number: ");PrintBufferHex(version_info.batch_number,7);
  PrintNewLine();
  PrintMessage("wk yr: "); PrintBufferHex(&version_info.production_week, 2);
  PrintNewLine();
  
  /* authenticate using PICC's master key */
  MifareAutoAuthenticate(&tag, 0);
  PrintNewline();
  PrintMessage("Auto Authenticated");
  
}
