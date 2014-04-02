/*
 * -----------------------------------------------------------------------------
 * -----                            CONFIGS.H                              -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *  This is the header file for the PIC18F67K22 configuration bits
 * 
 * Assumptions:
 *  None
 *
 * Compiler:
 *  HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *  Apr. 28, 2013      Nnoduka Eruchalu     Initial Revision
 */

#ifndef EASYPAY_CONFIGS_H
#define EASYPAY_CONFIGS_H

/* library include files */
/* none */

/* chip settings
 *  for more info on these, look at section 28.1 (Configuration Bits) of the
 *  PIC18F67K22 datasheet.
 *  As of today, this datasheet can be found here:
 *      http://ww1.microchip.com/downloads/en/DeviceDoc/39960d.pdf
 */
#define EASYPAY_CONFIG1L 0x11     /* 0------- Unimplemented                   */
                                  /* -0------ HI-TECH doesn't support XINST   */
                                  /* --0----- Unimplemented                   */
                                  /* ---10--- RC0 and RC1 are I/O             */
                                  /* -----0-- LF-INTOSC in Low-Power in Sleep */
                                  /* ------0- Unimplemented                   */
                                  /* -------1 VREG sleep enabled              */
                                  /* 00010001 = 0x11                          */

#define EASYPAY_CONFIG1H 0x05     /* 0------- 2-speed start-up disabled       */
                                  /* -0------ fail-safe clock monitor disabled*/
                                  /* --0----- Unimplemented                   */
                                  /* ---0---- Oscillator is used directly     */
                                  /* ----0101 EC oscillator High power        */
                                  /* 00000101 = 0x05                          */

#define EASYPAY_CONFIG2L 0x79     /* 0------- Unimplemented                   */
                                  /* -11----- ZPBORVMV instead of BORMV       */
                                  /* ---11--- Brown-Out Reset Voltage = 1.8V  */
                                  /* -----00- Brown-Out Rst is disabled       */
                                  /* -------1 Power-up timer is disabled      */
                                  /* 01111001 = 0x79                          */

#define EASYPAY_CONFIG2H 0x3C     /* 0------- Unimplemented                   */
                                  /* -01111-- WDT postscale is 1:32768        */
                                  /* ------00 WatchDog Timer is disabled      */
                                  /* 00111100 = 0x3C                          */
      
#define EASYPAY_CONFIG3L 0x01     /* 0000000- Unimplemented                   */
                                  /* -------1 RTCC reference clock is SOSC    */
                                  /* 00000001 = 0x01                          */

#define EASYPAY_CONFIG3H 0x89     /* 1------- MCLR\ is enabled (RG5 disabled) */
                                  /* -000---- Unimplemented                   */
                                  /* ----1--- Enable 7-bit Addr. Masking mode */
                                  /* -----00- Unimplemented                   */
                                  /* -------1 ECCP2 is multiplexed with RC1   */
                                  /* 10001001 = 0x89                          */
      
#define EASYPAY_CONFIG4L 0x91     /* 1------- Disable background debugger     */
                                  /* -00----- Unimplemented                   */
                                  /* ---1---- 2KW Boot Block Size             */
                                  /* ----000- Unimplemented                   */
                                  /* -------1 Stack full/underflow will Reset */
                                  /* 10010001 = 0x91                          */


#define EASYPAY_CONFIG5L 0xFF     /* 1------- Block 7 not code-protected      */
                                  /* -1------ Block 6 not code-protected      */
                                  /* --1----- Block 5 not code-protected      */
                                  /* ---1---- Block 4 not code-protected      */
                                  /* ----1--- Block 3 not code-protected      */
                                  /* -----1-- Block 2 not code-protected      */
                                  /* ------1- Block 1 not code-protected      */
                                  /* -------1 Block 0 not code-protected      */
                                  /* 11111111 = 0xFF                          */

#define EASYPAY_CONFIG5H 0xC0     /* 1------- Data EEPROM not code-protected  */
                                  /* -1------ Boot block not code-protected   */
                                  /* --000000 Unimplemented                   */
                                  /* 11000000 = 0xC0                          */

#define EASYPAY_CONFIG6L 0xFF     /* 1------- Block 7 not write-protected     */
                                  /* -1------ Block 6 not write-protected     */
                                  /* --1----- Block 5 not write-protected     */
                                  /* ---1---- Block 4 not write-protected     */
                                  /* ----1--- Block 3 not write-protected     */
                                  /* -----1-- Block 2 not write-protected     */
                                  /* ------1- Block 1 not write-protected     */
                                  /* -------1 Block 0 not write-protected     */
                                  /* 11111111 = 0xFF                          */

#define EASYPAY_CONFIG6H 0xE0     /* 1------- Data EEPROM not write-protected */
                                  /* -1------ Boot block not write-protected  */
                                  /* --1----- Config Regs not write-protected */
                                  /* ---00000 Unimplemented                   */
                                  /* 11100000 = 0xE0                          */

#define EASYPAY_CONFIG7L 0xFF     /* 1------- Block 7 not table-read protected*/
                                  /* -1------ Block 6 not table-read protected*/
                                  /* --1----- Block 5 not table-read protected*/
                                  /* ---1---- Block 4 not table-read protected*/
                                  /* ----1--- Block 3 not table-read protected*/
                                  /* -----1-- Block 2 not table-read protected*/
                                  /* ------1- Block 1 not table-read protected*/
                                  /* -------1 Block 0 not table-read protected*/
                                  /* 11111111 = 0xFF                          */

#define EASYPAY_CONFIG7H 0x40     /* 0------- Unimplemented                   */
                                  /* -1------ Boot blck not tbl-read protected*/
                                  /* --000000 Unimplemented                   */
                                  /* 01000000 = 0x40                          */


#endif                                                   /* EASYPAY_CONFIGS_H */


