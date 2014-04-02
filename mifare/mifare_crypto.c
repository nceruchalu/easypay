/*
 * -----------------------------------------------------------------------------
 * -----                         MIFARE_CRYPTO.C                           -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is a library of functions for Mifare DESFire Crypto Operations
 *
 * Table of Contents:
 *   Rol                 - ROL operation (byte level)
 *   Lsl                 - LSL operation (bit level)
 *   CmacGenerateSubkeys - get 2 subkeys from key
 *   Cmac                - MAC generation process of CMAC
 *
 *   Crc32               - get a CRC32 checksum
 *   Crc32Append         - get a CRC32 checksum and append to data
 *   MifareCrc32         - get a Desfire CRC32 checksum
 *   MifareCrc32Append   - get a Desfire CRC32 checksum and append to data
 *   MifareCrc16         - get a Desfire ISO1444-3 Type A CRC16 checksum
 *   MifareCrc16Append   - get a Desfire CRC16 checksum and append to data
 *
 *   KeyBlockSize        - get key block size
 *   PaddedDataLength    - size required to store nbytes of data in a buffer of
 *                         size n*block_size
 *   MacedDataLength     - buffer size required to MAC nbytes of data
 *   EncipheredDataLength        - buffer size required to encipher nbytes of
 *                                 data and a CRC
 *   MifareCryptoPreprocessData  - Data Encipher before transmission
 *   MifareCryptoPostprocessData - Data Decipher/Verification after reception.
 *   MifareCipherSingleBlock
 *   MifareCipherBlocksChained   - performs all CBC ciphering/deciphering
 *
 * Documentation Sources:
 *   - libfreefare
 *
 *   - NIST Special Publication 800-38B
 *     Recommendation for Block Cipher Modes of Operation: The CMAC Mode for
 *     Authentication
 *     May 2005
 *
 *   - http://www.ross.net/crc/download/crc_v3.txt
 *     A Painless Guide to CRC Error Detection Algorithms
 *
 * Assumptions:
 *   - Data from PICC has status byte at the end,
 *   - MSByte is at lowest index (0)
 *   - bit^n = bit repeated n times, ex: 0^3 = 000
 *
 * ToDo:
 *   Check all uses of CIPH()
 *   STATUS byte is expected to come last (See Postprocess)
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Jan. 02, 2013      Nnoduka Eruchalu     Initial Revision
 *   May  03, 2013      Nnoduka Eruchalu     Removed dynamic memory allocation
 *                                           So changed functions to accept
 *                                           pointers to pre-allocated data
 */

#include <string.h>   /* for mem* operations */
#include <stdlib.h>
#include "mifare.h"
#include "mifare_crypto.h"
#include "des.h"      /* for des_ecb_encrypt() */
#include "aes.h"      /* for AES operations    */


/* --------------------------------------
 * Functions Local To This File
 * --------------------------------------
 */
/* XOR operation (byte level) */
static void Xor(uint8_t *ivect, uint8_t *data, size_t len);
/* size of MACing produced with the key */
static size_t KeyMacingLength(mifare_desfire_key *key);


/*
 * Xor
 * Description: Bytewise Xor: data = data XOR ivect
 *
 * Arguments:   ivect = pointer to initialization vector
 *              data  = pointer to data block [modified]
 *              len   = length of data & ivect
 * Return:      None
 *
 * Operation: 
 *  Loop through and do bytewise xors
 *
 * Revision History:
 *  Jan. 03, 2012      Nnoduka Eruchalu     Initial Revision
 *  May  03, 2013      Nnoduka Eruchalu     Updated Comments
 */
static void Xor(uint8_t *ivect, uint8_t *data, size_t len)
{
  size_t i; /* index into arrays */
  for (i=0; i< len; i++) {
    data[i] ^= ivect[i];
  }
}


/*
 * Rol
 * Description: Rotate Byte left. (Move MSByte over to LSByte position, and 
 *              shift up other bytes)
 *              Ex: Rol {0,1,2,3,4} = {1,2,3,4,0}
 *
 * Arguments:   data = pointer to data block [modified]
 *              len  = length of data block
 * Return:      None
 *
 * Operation:   Copy first byte; shift all other bytes down one index, and place
 *              first byte in now empty last slot.
 *
 * Assumptions: MSByte is at index 0
 *
 * Revision History:
 *   Jan. 03, 2012      Nnoduka Eruchalu     Initial Revision
 *   May  03, 2013      Nnoduka Eruchalu     Updated Comments
 */
void Rol(uint8_t *data, size_t len)
{
  size_t i;       /* index into data */
  uint8_t first = data[0];
  for (i = 0; i < len-1; i++) {
    data[i] = data[i+1];
  }
  data[len-1] = first;
}


/*
 * Lsl
 * Description: Logical Bit Shift Left. Shift MSB out, and place a 0 at LSB 
 *              position
 *              Ex: LSL {5,1,2,3,4} = {1,2,3,4,0}
 *
 * Arguments:   data = pointer to data block [modified]
 *              len  = length of data block
 *
 * Operation:   For each byte position, shift out highest bit with 
 *              (data[n] << 1) and add in the highest bit from next lower byte 
 *              with (data[n+1] >> 7)
 *              The addition is done with a bitwise OR (|)
 *              For the Least significant byte however, there is no next lower 
 *              byte so this is handled last and simply set to data[len-1] <<= 1
 *
 * Assumptions: MSByte is at index 0
 *
 * Revision History:
 *   Jan. 03, 2012      Nnoduka Eruchalu     Initial Revision
 *   May  03, 2013      Nnoduka Eruchalu     Updated Comments
 */
void Lsl(uint8_t *data, size_t len)
{
  size_t n;      /* index into data */
  for (n = 0; n < len - 1; n++) {
    data[n] = (data[n] << 1) | (data[n+1] >> 7);
  }
  data[len - 1] <<= 1;
}


/*
 * CmacGenerateSubkeys
 * Description: Derive Subkeys K1 and K2
 *
 * Arguments:   key = block cipher key
 * Return:      None
 *
 * Operation:   Get key block size; it is length of each subkey, and is used in 
 *              determining R, also of length "key block size"
 *              R is a bit string that is used in subkey generation process.
 *              If block size is 8 bytes, R=0x1B; if 16 bytes, R = 0x87,
 *
 *              Steps:
 *              1. Let l = Ciph(key, 0^b)
 *              2. If MSBit(L) == 0, then K1 = L << 1;
 *                 Else K1 = (L << 1) XOR R
 *              3. If MSBit(K1) = 0, then K2 = (K1 << 1);
 *                 Else K2 = (K1 << 1) XOR R
 *
 * Assumptions: MSB is at index 0
 *
 * Revision History:
 *   Jan. 03, 2012      Nnoduka Eruchalu     Initial Revision
 */
void CmacGenerateSubkeys(mifare_desfire_key *key)
{
  int kbs = KeyBlockSize (key);                /* get key block size; */
  uint8_t R = (kbs == 8) ? 0x1B : 0x87;        /* use this to determine R and */
                                               /* length of subkeys */
  uint8_t l[MAX_CRYPTO_BLOCK_SIZE];            /* allocate memory that is at */
  uint8_t ivect[MAX_CRYPTO_BLOCK_SIZE];        /* least of size KeyBlockSize */
  
  memset (l, 0, kbs);                          /* start block cipher     */
  memset (ivect, 0, kbs);                      /* on a zero'd init block */
  
  /* store block cipher result in l */
  MifareCipherBlocksChained(NULL, key, ivect, l, kbs, MCD_RECEIVE,MCO_ENCIPHER);
    
  /* Used to compute CMAC on complete blocks */
  memcpy (key->cmac_sk1, l, kbs);              /* K1 = L */
  Lsl(key->cmac_sk1, kbs);                     /* K1 = L << 1, if MSBit(L)==0 */
  if (l[0] & 0x80)                             /* if however MSBit(L) != 0 */
    key->cmac_sk1[kbs-1] ^= R;                 /* then K1 = (L << 1) xor R */
  
  /* Used to compute CMAC on the last block if non-complete */
  memcpy (key->cmac_sk2, key->cmac_sk1, kbs);  /* K2 = K1 */
  Lsl(key->cmac_sk2, kbs);                     /* K2=(K1<<1), if MSBit(K2)==0 */
  if (key->cmac_sk1[0] & 0x80)                 /* if however MSBit(K1) != 0 */
    key->cmac_sk2[kbs-1] ^= R;                 /* then K2 = (K1 << 1) xor R */
}


/*
 * Cmac
 * Description: MAC generation process of CMAC
 *
 * Arguments:   key   = pointer to block cipher key
 *              ivect = pointer to zero'd init vector block  
 *              data  = pointer message to generate MAC on
 *              len   = number of message bytes
 *              cmac  = pointer to buffer for saving CMAC [modified]
 * Return:      None
 *
 * Operation:
 *   1. Apply the subkey generation process to Key, K, to produce K1 and K2.
 *   2. If message (M) bit length (Mlen) = 0, let n=1, else, n=ceiling(Mlen/b)
 *      Note that Mlen = 8*len and b = 8*kbs
 *   3. Let M1, M2, ..., M{n-1}, Mn* denote the unique sequence of bit strings
 *      such that M = M1 (concat) M2 (concat) ... (concat) M{n-1} (concat) Mn*,
 *      where M1, M2, ... M{n-1} are complete blocks.
 *   4. If Mn* is a complete block, let Mn = K1 xor Mn*;
 *      else let Mn = K2 xor (Mn* concat 10^j), where j = n*(kbs) -Mlen -1
 *   5. Let C0 = 0^b
 *   6. For i=1 to n, let Ci = CIPH(K, (C{i-1} xor Mi))
 *   7. Let T= Most Significant Tlen bits of (Cn), where Tlen is MAC length
 *      parameter (equal to kbs*8)
 *   8. Return T in cmac buffer.
 *
 * Assumptions: MSB is at index 0
 *              data len > key block size
 *              ivect is all 0s
 *
 * Revision History:
 *   Jan. 03, 2012      Nnoduka Eruchalu     Initial Revision
 */
void Cmac(mifare_desfire_key *key, uint8_t *ivect, uint8_t *data, size_t len,
          uint8_t *cmac)
{
  int kbs = KeyBlockSize(key);         /* get key block size and set message, */
                                       /*M, to buffer of size n blocks (n*kbs)*/
  /* allocate buffer at least as big as PaddedDataLength(len, kbs) */
  uint8_t buffer[MAX_PADDED_DATA_LENGTH];
    
  memcpy(buffer, data, len);           /* load msg buffer with data */
  
                                       /* if Mn* isn't a complete block */
  if ((!len) || (len % kbs)) {         /* (has padded bytes) */ 
    buffer[len++] = 0x80;              /* set Mn* = Mn* (concat) 10^j */
    while (len % kbs) {                /* where j = (n*kbs) - Mlen - 1 bits */
      buffer[len++] = 0x00;            /* note n*kbs - Mlen = puts you at the */
    }                                  /* beginning of the first bit of the */
                                       /* byte following the last byte of Mn* */
    Xor(key->cmac_sk2, buffer + len - kbs, kbs); /* Mn = K2 xor (updated Mn*) */
  } else {                             /*Mn* a complete block(no padded bytes)*/
    Xor(key->cmac_sk1, buffer + len - kbs, kbs); /* Mn = K1 xor Mn* */
  }
  
  /* initialization vector is C0 and should be all 0s */
  /* get Ci = CIPH(K, (C{i-1} xor Mi)) for i=1 to n*/
  MifareCipherBlocksChained (NULL, key, ivect, buffer, len, MCD_SEND,
                             MCO_ENCIPHER);
  memcpy (cmac, ivect, kbs);           /* return top kbs bytes of Cn */
}


/*
 * Crc32
 * Description: Generate a CRC checksum of width 32 for given data array of 
 *              length len.
 * 
 * Arguments:   data = pointer to data block
 *              len  = length of data block
 *              crc = ptr to CRC checksum array, in big endian format [modified]
 * Return:      None
 *
 * Operation:
 *  Choose, the generator polynomial: [32-bit width so Ethernet]
 *  x32 + x26 + x23 + x22 + x16 + x12 + x11 + x10 + x8 + x7 + x5 + x4 + x2 + x+1
 *
 *                                                        TRUNCATED HEX
 *                                                        -------------
 *  BINARY  : 1 0000 0100 1100 0001 0001 1101 1011 0111 = 04C11DB7
 *  REVERSED: 1110 1101 1011 1000 1000 0011 0010 0000 1 = EDB88320 
 *
 * This uses the UNREVERSED POLY
 *  
 *  The CRC checksum is the remainder of CRC division throught a register as
 *  drawn below.
 *
 *  For the pruposes of example, consier a poly for CRC4 and the poly = 10111.
 *  Then, to perform the division, we need to use a 4bit register:
 *
 *              3  2   1    0   Bits
 *            +---+---+---+---+
 *  Pop!  <-- |   |   |   |   |  <-- augmented message (Message (concat) 0 bits)
 *            +---+---+---+---+
 *          1   0   1   1   1    = The Poly
 *
 *  To perform the division perform the following:
 *  1. Load the register with 0 bits.
 *  2. Augment the message by appending W zero bits to the end of it.
 *  3. While (more message bits)
 *       Begin
 *         Shift the register left by one bit, reading the next bit of the
 *         augmented message into register bit position 0.
 *         If (a 1 bit popped out of the register during this shift)
 *            Register = Register XOR Poly
 *         End
 *   4. The register now contains the remainder.
 *
 *
 * Documentation:
 *   http://www.ross.net/crc/download/crc_v3.txt
 *
 *   Test this function's outputs with:
 *     http://depa.usst.edu.cn/chenjq/www2/SDesign/JavaScript/CRCcalculation.htm
 *     http://ghsi.de/CRC/index.php?Polynom=100000100110000010001110110110111&Message=123456
 *
 * Revision History:
 *   Jan. 04, 2012      Nnoduka Eruchalu     Initial Revision
 */
void Crc32(uint8_t *data, size_t len, uint8_t *crc)
{
  uint32_t reg = 0;      /* load register with 0 bits */
  int current_bit;       /* need current bit and byte values to keep track of */
  uint8_t current_byte;  /* shifts into register */
  int byte;              /* index into data message bytes */
  uint8_t popped_bit;    /* bit popped out of register after shift */
  
  len += CRC32_NUMBYTES; /* augment the message by updating length */
                         /* these extra bits will be zero's */
    
  while(len--) {         /* shift in bits of the data message keeping track */
    current_byte = *data++; /* keeping track of current byte and bit values */
    for(current_bit=8; current_bit > 0; current_bit--) { /* if bit to popped */
      popped_bit = reg >> (CRC32_NUMBITS -1); /* from reg is a 1 then */
      reg = (reg << 1) | ((len < CRC32_NUMBYTES) ? 0 : (current_byte >> 7));
      current_byte <<= 1;   /* xor in the generator polynomial to register */
      if (popped_bit) reg ^= CRC32_POLY;
    }
  }
  
  for(byte=0; byte < CRC32_NUMBYTES; byte++) { /* save the reg in crc array */
    crc[byte] = reg >> (8*(CRC32_NUMBYTES - 1 - byte)) & 0xFF;
  }
}


/*
 * Crc32Append
 * Description: Get Crc32 checksum and append to data.
 *
 * Arguments:   data = pointer to data array
 *              len  = length of data array
 * Return:      None
 *
 * Operation:   Call Crc32, and the pointer for the location of the crc value is
 *              the address of byte right after LSB of the data.
 *
 * Assumptions: Data is augmented i.e. has space for one extra byte, after its 
 *              LSB. This is where the CRC will be saved
 *
 * Revision History:
 *   Jan. 04, 2012      Nnoduka Eruchalu     Initial Revision
 */
void Crc32Append(uint8_t *data, size_t len)
{
  Crc32(data, len, data+len);
}


/*
 * MifareCrc32
 * Description: Generate a MIFARE DESFire CRC checksum of width 32 for given 
 *              data array of length len. 
 *
 * Arguments:   data = pointer to data block
 *              len  = length of data block
 *              crc  = ptr to generated CRC checksum array, as it's to be
 *                     transmitted (little endian) [modified] 
 * Return: None
 *
 * Operation:
 *   Choose, the generator polynomial: [32-bit width so Ethernet]
 *   x32 + x26 + x23 + x22 + x16 + x12 + x11 + x10 + x8 + x7 + x5 + x4 + x2 +x+1
 * 
 *                                                         TRUNCATED HEX
 *                                                         -------------
 *   BINARY  : 1 0000 0100 1100 0001 0001 1101 1011 0111 = 04C11DB7
 *   REVERSED: 1110 1101 1011 1000 1000 0011 0010 0000 1 = EDB88320 
 *
 *   This uses the REVERSED POLY
 *
 * Documentation:
 *   libfreefare source code
 *
 * Test Values:
 *   data[26] ={0x3D, 0x00, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x01, 0x02,
 *              0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12,
 *              0x13, 0x14, 0x15, 0x16, 0x17, 0x18};
 *   CRC = 0x59F71A9C
 *   and transmit as {9C, 1A, F7, 59}
 *
 * Revision History:
 *   Jan. 05, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareCrc32 (uint8_t *data, size_t len, uint8_t *crc)
{
  uint32_t reg = 0xFFFFFFFF; /* load register with 1 bits */
  int current_bit;           /* need current bit and byte values to keep */
  uint8_t current_byte;      /* track of shifts into register */
  size_t byte;               /* index into CRC array bytes */
  uint8_t popped_bit;        /* bit popped out of register after shift */
  
  while(len--) {             /* shift in bytes of data message keeping track */
    current_byte = *data++;  /* of current byte and bit values */
    reg ^= current_byte;
    for (current_bit = 7; current_bit >= 0; current_bit--) {
      popped_bit = (reg) & 0x00000001;
      reg >>= 1;
      if (popped_bit) reg ^=  CRC32_POLY_REV;
    }
  }
  
  for(byte=0; byte < CRC32_NUMBYTES; byte++) { /* save the reg in crc array */
    crc[byte] = (reg >> (8*byte)) & 0xFF;      /* in little endian format */
  }
}


/*
 * MifareCrc32Append
 * Description: Get a DESFire Crc32 checksum and append to data.
 *
 * Arguments:   data = pointer to data array
 *              len  = length of data array
 * Return:      None
 *
 * Operation:   Call MifareCrc32, and the pointer for the location of the crc 
 *              value is the address of byte right after LSB of the data.
 *
 * Assumptions: Data is augmented i.e. has space for one extra byte, after its 
 *              LSB. This is where the CRC will be saved
 *
 * Revision History:
 *   Jan. 05, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareCrc32Append(uint8_t *data, size_t len)
{
  MifareCrc32(data, len, data+len);
}


/*
 * MifareCrc16
 * Description: Generate a MIFARE DESFire CRC checksum of width 16 for given 
 *              data array of length len.
 *              This is done as specified for ISO 14443-3 Type A.
 * 
 * Arguments:   data = pointer to data block
 *              len  = length of data block
 *              crc  = ptr to generated CRC checksum array, as it's to be
 *                     transmitted (little endian) [modified]
 * Return:      None
 *
 * Operation:
 *  Use the specified generator polynomial: [16-bit width]
 *  x16 + x12 + x5 + 1
 *
 *  The process of encoding and decoding may be conveniently carried out by
 *  a 16-stage cyclic shift register with appropriate feedback gates. 
 *  The flip-flops of the register shall be numbered from FF0 to FF15. FF0 shall
 *  be the leftmost flip-flop where data is shifted in. FF15 shall be the
 *  rightmost flip-flop where data is shifted out.
 *
 *  Below is the initial content of the register.
 *        0  1   2   3   4  5   6   7   8  9  10  11  12 13  14  15
 *      +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *      | 0 | 1 | 1 | 0 | 0 | 0 | 1 | 1 | 0 | 1 | 1 | 0 | 0 | 0 | 1 | 1 |
 *      +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *  message -->                                                    --> Pop!
 *
 *
 * Note that the LSByte of the CRC is transmitted first, so the positions of MSB
 * and LSB are swapped before returning.
 *
 * Documentation:
 *   http://www.waazaa.org/download/fcd-14443-3.pdf
 *
 * Test Values:
 *   data = {0x00, 0x00}, CRC = 0x1EA0, and transmit as {0xA0, 0x1E}
 *   data = {0x12, 0x34}, CRC = 0xCF26, and transmit as {0x26, 0xCF}
 *
 * Revision History:
 *   Jan. 05, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareCrc16 (uint8_t *data, size_t len, uint8_t *crc)
{
  uint8_t  bt;                               /* get current data byte */
  uint16_t reg = 0x6363;                     /* ITU-V.41 */
  
  do {
    bt = *data++;                            /* loop through data bytes */
    bt = (bt ^ (uint8_t) (reg & 0x00FF));    
    bt = (bt ^ (bt << 4));
    reg = (reg >> 8) ^ ((uint16_t) bt << 8) ^ ((uint16_t) bt << 3) ^ 
      ((uint16_t) bt >> 4);
  } while (--len);                           /* stay within data array limits */
  
  *crc++ = (uint8_t) (reg & 0xFF);           /* transmit LSB first */
  *crc = (uint8_t) ((reg >> 8) & 0xFF);      /* transmit MSB second */
}


/*
 * MifareCrc16Append
 * Description: Get a DESFire Crc16 checksum and append to data.
 *
 * Arguments:   data = pointer to data array
 *              len  = length of data array
 * Return:      None
 *
 * Operation:   Call MifareCrc16, and the pointer for the location of the crc 
 *              value is the address of byte right after LSB of the data.
 *
 * Assumptions:  Data is augmented i.e. has space for one extra byte, after its
 *               LSB. This is where the CRC will be saved
 *
 * Revision History:
 *   Jan. 05, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareCrc16Append(uint8_t *data, size_t len)
{
  MifareCrc16(data, len, data+len);
}


/*
 * KeyBlockSize
 * Description: Get MIFARE DESFire key block size
 *
 * Arguments:   key       = block cipher key
 * Return:      block_size = key block size
 * 
 * Operation:   If crypto operation is DES, 3DES, 3K3DES the block size is 8,
 *              else if crypto operation is AES the block size is 16.
 *
 * Revision History:
 *   Jan. 05, 2012      Nnoduka Eruchalu     Initial Revision
 */
size_t KeyBlockSize(mifare_desfire_key *key)
{
  size_t block_size;

  switch (key->type) {
  case T_DES:          /* if crypto operation is DES, 3DES, 3K3DES  */
  case T_3DES:         /* the block size is 8 */
  case T_3K3DES:
    block_size = 8;
    break;
  case T_AES:          /* if crypto operation is AES, the block size is 16 */
    block_size = 16;
    break;
  }
  return block_size;
}


/*
 * KeyMacingLength
 * Description: Size of MACing produced with the key
 *
 * Arguments:    key        = block cipher key
 * Return:       mac_length = number of MAC/CMAC bytes
 *
 * Operation:    If crypto operation is DES or 3DES, Data Authenticity is done 
 *               Eusing MAC
 *               Else if crypto operation is 3K3DES or AES, Data Authenticity is
 *               done using CMAC
 *
 * Revision History:
 *   Jan. 05, 2012      Nnoduka Eruchalu     Initial Revision
 */
static size_t KeyMacingLength(mifare_desfire_key *key)
{
  size_t mac_length;  
  
  switch (key->type) { /* if crypto operation is DES or 3DES, Data */
  case T_DES:          /* Authenticity is done using MAC */
  case T_3DES:
    mac_length = MAC_LENGTH;
    break;
  case T_3K3DES:       /* if crypto operation is 3K3DES or AES, Data */
  case T_AES:          /* Authenticity is done using CMAC */
    mac_length = CMAC_LENGTH;
    break;
  }
  return mac_length;
}


/*
 * PaddedDataLength
 * Description: Size required to store nbytes of data in a buffer of size 
 *              n*block_size
 *              If nbytes == 0, n=1 else n=ceiling(nbytes/block_size);
 *
 * Arguments:   nbytes     = number of bytes to be padded
 *              block_size = block size to pad with
 * Return:      number of bytes after padding to block_size
 *
 * Operation:   If nbytes==0 or nbytes is not a multiple of block_size, return
 *                n*block_size where n = max(1, ceiling(nbytes/block_size))
 *              else, return nbytes.
 *
 * Revision History:
 *   Jan. 05, 2012      Nnoduka Eruchalu     Initial Revision
 */
size_t PaddedDataLength(size_t nbytes, size_t block_size)
{
  if ((!nbytes) || (nbytes % block_size)) /* if numbytes is 0 or nbytes is not*/
    return ((nbytes / block_size) + 1) * block_size;/*a multiple of block_size*/
  else      /* return n*block_size where n=max(1, ceiling(nbytes/block_size)) */
    return nbytes;                          /* else no need to paa data bytes */
}


/*
 * MacedDataLength
 * Description: Buffer size required to MAC nbytes of data
 *
 * Arguments:   key    = Block cipher key
 *              nbytes = number of data bytes to be MACed
 * Return:      number of bytes after adding MAC bytes
 *
 * Operation:   Buffer size is nbytes + KeyMacingLength(key)
 *
 * Revision History:
 *   Jan. 05, 2012      Nnoduka Eruchalu     Initial Revision
 */
size_t MacedDataLength(mifare_desfire_key* key, size_t nbytes)
{
  return (nbytes + KeyMacingLength(key));
}


/*
 * EncipheredDataLength
 * Description: Buffer size required to encipher nbytes of data and CRC
 *
 * Arguments:   tag    = Mifare DESFire PICC
 *              nbytes = number of data bytes to be Enciphered
 *              communication_settings = comm. settings between PCD and PICC
 * Return:      number of bytes after enciphering data
 *
 * Operation:   Actual number of enciphered data bytes is: 
 *                nbytes + crc length (2 or 4)
 *              This has to be padded to multiples of (key block size)
 *              Crc Length of 2 is for Legacy Authentication, and 4 is for 
 *                New Authentication (DESFire EV1 only)
 *
 * Revision History:
 *   Jan. 05, 2012      Nnoduka Eruchalu     Initial Revision
 */
size_t EncipheredDataLength(mifare_tag* tag, size_t nbytes,
                            int communication_settings)
{
  size_t crc_length = 0;     /* crc length: initialize it for NO_CRC */
  size_t block_size;         /* key block size */
  
  if (!(communication_settings & NO_CRC)) {
    switch (tag->authentication_scheme) {  /* DESFire CRC is 2 bytes for */
    case AS_LEGACY:                        /* legacy authentication */
      crc_length = 2;
      break;
    case AS_NEW:                           /* DESFire CRC is 4 bytes for new */
      crc_length = 4;                      /* authentication schemes */
      break;
    }
  }
  
  block_size = KeyBlockSize(&tag->session_key); /* get key block size */
  /* enciphered data consisting of nbytes and crc is padded to multiples of */
  /* key block size */
  return PaddedDataLength (nbytes + crc_length, block_size);
}


/*
 * MifareCryptoPreprocessData
 * Description: Data Encipher before transmission.
 *  
 * Arguments:   tag    = PICC
 *              data   = pointer to data to be transmitted 
 *              nbytes =ptr to number of data bytes to be transmitted [modified]
 *              offset = offset of command plus headers
 *  
 *              |<---------------- data ---------------->|
 *              |<--- offset -->|                        |
 *              +---------------+------------------------+
 *              | CMD + HEADERS | DATA TO BE TRANSMITTED |
 *              +---------------+------------------------+
 *
 * Return:      - pointer to data or crypto buffer, so this doesn't allocate any
 *                new memory;
 *                if pointer is NULL, there was an error and data couldnt be 
 *                preprocessed.
 *              - nbytes is [modified] length of processed data.
 *  
 * Operation:
 *  If plain data transfer then function should be returning a pointer to data, 
 *  so start by initializing result to point to the data array.
 *  allocate array to contain Legacy authentication scheme MAC bytes,
 *  and start by assuming MAC will be appended.
 *  
 *  If the passed in tag doesn't have a session key, then there will be no
 *  encrypted data, so simply return the pointer to data array
 *
 *  Now we are ready for the actual preprocessing, and this depends on the
 *  data transmission mode (contained in communication_settings).
 *
 *  1. Plain Data Transfer
 *   If legacy authentication scheme, there is no preprocessing to be done.
 *
 *   When using new authentication methods, PLAIN data transmission from the
 *   PICC to the PCD are CMACed, so we have to maintain the cryptographic
 *   initialization vector up-to-date to check data integrity later.
 *   The only difference with CMACed data transmission is that the CMAC is not
 *   appended to the data sent by the PCD to the PICC.
 *
 *  2. Plain data transfer with MAC
 *   If legacy authentication scheme, only generate MAC if settings say so.
 *   If a MAC is to be generated, first determine the encrypted data length.
 *   Encrypted data length accounts for the offset. This is done by first
 *   subtracting offset from nbytes before computing the padded data length. 
 *   Offset is then added back to padded data length.
 *   Before we do any encrypting, we need to be sure the crypto buffer is big
 *   enough, and if we cant make this so we abort.
 *   Next, fill the crypto buffer with data and add 0 padding.
 *   Perform CBC encipher for sending, to get and save the MAC bytes.
 *   Finally append these MAC bytes, and update nbytes to account for this.
 * 
 *   If new authentication scheme, MAC will be generated by CMAC, and this will
 *   simply be called CMAC from here on out.
 *   Only generate CMAC if communication settings say so.
 *   If CMAC is to be generated, generate it by calling Cmac().
 *   This CMAC should not be be appended if communication mode is PLAIN.
 *   If however communication mode isn't PLAIN, but MACED, then we will
 *   be returning an updated crypto buffer, so first ensure it's big enough to
 *   hold MACed data. If it isn't abort. If however crypto buffer is big enough,
 *   copy the data into it, then copy the CMAC into it.
 *   Update nbytes to account for the added CMAC bytes.
 *  
 *  3. DES/3DES encrypted data transfer
 *   Message Format:
 *
 *   |<-------------- data -------------->|
 *   |<--- offset -->|                    |
 *   +---------------+--------------------+-----+---------+
 *   | CMD + HEADERS | DATA TO BE SECURED | CRC | PADDING |
 *   +---------------+--------------------+-----+---------+ ----------------
 *   |               |<~~~~v~~~~~~~~~~~~~>|  ^  |         |   (DES / 3DES)
 *   |               |     `---- Crc16() ----'  |         |
 *   |               |                    |  ^  |         | ----- *or* -----
 *   |<~~~~~~~~~~~~~~~~~~~~v~~~~~~~~~~~~~>|  ^  |         |  (3K3DES / AES)
 *                   |     `---- Crc32() ----'  |         |
 *                   |                                    | ---- *then* ----
 *                   |<---------------------------------->|
 *                           Encipher() / Decipher()
 *   
 *   Only Encipher Data if settings say so.
 *   If Data is to be encrypted, first get encrypted data length, again
 *   accounting for the offset.
 *   This encrypted data will be saved in the crypto buffer so it has to be big
 *   enough for this.
 *   If crypto buffer is indeed big enough for this, load it with data, then
 *   the CRC, and finally load in 0s for the padding. 
 *   The CRC is CRC16 if using legacy auth., and CRC32 if using new auth.
 *   nbytes has to be updated to be the encrypted data length.
 *
 * Revision History:
 *   Jan. 05, 2012      Nnoduka Eruchalu     Initial Revision
 */
uint8_t *MifareCryptoPreprocessData(mifare_tag *tag, uint8_t *data, 
                                    size_t *nbytes, off_t offset, 
                                    int communication_settings)
{
  uint8_t *res = data;         /* with no processing, result is original data */
  uint8_t mac[MAC_LENGTH];     /* setup MAC array */
  size_t edl, mdl;             /* encrypted and MACed data length */
  uint8_t append_mac = TRUE;   /* start by assuming we need to append MAC */
  mifare_desfire_key *key = &tag->session_key; /* get pointer to session key */
  
  if(!key)                     /* if no session key, there can't be crypto */
    return data;
  
  switch(communication_settings & MDCM_MASK) { /* toggle based on comm. mode */
  case MDCM_PLAIN:             /* plain data transfer */
    if (AS_LEGACY == tag->authentication_scheme)
      break;                   /* do nothing if legacy authentication scheme */
    
    append_mac = FALSE;        /* when using new auth. scheme need to CMAC */
                               /* without appending to data; so pass through */
    
  case MDCM_MACED:             /* plain data transfer with MAC */
    switch (tag->authentication_scheme) {
    case AS_LEGACY:            /* if legacy authentication scheme and not to */
      if (!(communication_settings & MAC_COMMAND)) /* MAC command, then done */
        break;
      
      /* if however it is required to MAC data, then process data */
      edl = PaddedDataLength(*nbytes - offset, KeyBlockSize(&tag->session_key))
        +offset;                      /* grab encrypted data length */
      /* crypto buffer should be (or is reallocated) to at least as big as edl*/
      res = tag->crypto_buffer;
      
      memcpy(res, data, *nbytes);               /* Fill in crypto buffer with */
      memset(res + *nbytes, 0, edl - *nbytes);  /* data and add 0 padding     */
      
      /* CBC encipher for sending, to get MAC */
      MifareCipherBlocksChained(tag, NULL, NULL, res + offset, edl - offset, 
                                MCD_SEND, MCO_ENCIPHER);
      memcpy(mac, res + edl - 8, MAC_LENGTH);    /* save MAC value */
      memcpy(res, data, *nbytes); /* copy provided data again (was */
                                  /* overwritten by MifareCipherBlocksChained)*/
      
      /* finally append MAC, by first grabbing MACed data length  */
      mdl = MacedDataLength(&tag->session_key, *nbytes - offset) + offset;
      /* crypto buffer should be (or is reallocated) to at least as big as mdl*/
      /* res = tag->crypto_buffer; */
      
      memcpy(res + *nbytes, mac, MAC_LENGTH);            /* append MAC then */
      *nbytes += MAC_LENGTH;   /* update number of bytes to account for MAC */
      
      break;                   /* done handling legacy auth. scheme */
            
    case AS_NEW:               /* if new auth. sceme and not to MAC command,*/
      if (!(communication_settings & CMAC_COMMAND)) /* then done */
        break;
      
      /* if however it's required to MAC command, then use CMAC generation */
      Cmac(key, tag->ivect, res, *nbytes, tag->cmac);
      
      if (append_mac) { /* don't append MAC if passed through from MDCM_PLAIN */
        mdl = MacedDataLength(key, *nbytes);
        /* crypto buffer is (or is reallocated to) at least as big as mdl */
        res = tag->crypto_buffer;
                
        memcpy(res, data, *nbytes); /* fill in crypto buffer with data and */
        memcpy(res + *nbytes, tag->cmac, CMAC_LENGTH); /* save CMAC */
        *nbytes += CMAC_LENGTH;      /* update # of bytes to account for CMAC */
      }
      break;                   /* done handling new auth. scheme */
    }                          /* end switch(tag->authentication_scheme) */
    break;                     /* done handling MACed data transfer */
  
  case MDCM_ENCIPHERED:        /* DES/3DES ecnrypted data transfer */
    if (!(communication_settings & ENC_COMMAND)) /* if not to encipher data */
      break;                                     /* then done */
    /* if however it is required to encipher data, then process data */
    edl = EncipheredDataLength(tag, *nbytes - offset, communication_settings) 
      + offset;
    /* crypto buffer is (or is reallocated to) at least as big as edl */
    res = tag->crypto_buffer;
        
    
    memcpy(res, data, *nbytes);/* fill in crypto buffer with data and */
    if (!(communication_settings & NO_CRC)) { /* if CRC is needed, protocol */
      switch (tag->authentication_scheme) {   /* depends on auth. scheme */
      case AS_LEGACY:          /* legacy authentication uses Crc16 */
        MifareCrc16Append(res + offset, *nbytes - offset);
        *nbytes += 2;          /* account for 2 extra CRC Bytes */
        break;
      case AS_NEW:             /* new authentication uses Crc32 */
        MifareCrc32Append(res, *nbytes);
        *nbytes += 4;          /* account for 4 extra CRC bytes */
        break;
      }                        /* end switch(tag->authentication_scheme) */
    }                          /* end CRC required check */
    
    memset(res + *nbytes, 0, edl - *nbytes); /* pad crypto buffer with 0s */
    *nbytes = edl;             /* record actual encrypted data length */
    
    MifareCipherBlocksChained (tag, NULL, NULL, res + offset, *nbytes - offset, 
                               MCD_SEND, (AS_NEW == tag->authentication_scheme) 
                               ? MCO_ENCIPHER : MCO_DECIPHER);
    break;                     /* done handling encrypted data transfer */
  
  default:                     /* unknown communication settings */
    res = NULL;                /* do nothing, just setup a blank result ptr */
    *nbytes = -1;
    break;
  }                            /* end switch(communication_settings&MDCM_MASK)*/
  
  return res;                  /* return pointer to preprocessed data */
}


/*
 * MifareCryptoPostprocessData
 * Description: Data Decipher/Verification after reception.
 *
 * Arguments:   tag    = PICC
 *              data   = pointer to received data 
 *              nbytes = pointer to number of received data bytes [modified]
 *              communication settings
 *
 * Return:      - pointer to decrypted and verified data. NULL if data couldn't 
 *                be verified.
 *                The data is now just payload followed by status byte
 *                Again this is a pointer to data or crypto buffer, so doesn't 
 *                allocate any new memory.
 *              - nbytes [modified] to account for this (so excludes MAC, CRC 
 *                and padding)
 *  
 * Operation:
 *  if no processing is to be done, the returned data will be the passed in
 *  data, so initialize result to that.
 *  for comparisons we create a copy of the passed in data (that will undergo
 *  encryption). This should start out as an empty array.
 *
 *  If no session key, then no processing can be done; assume plain
 *  communication with legacy authentication, so just return data.
 *  If only 1 byte is Rx'd, then its just a status byte, so return it.
 *  
 *  Now move on to data verification which depends on the data transmission mode
 *  contained in the communication settings:
 *
 *  1. Plain Data Transfer
 *  If legacy authentication, there is nothing to verify here. so done
 
 *  If new authentication, then we expect there to be a CMAC appended to the
 *  plain data. Do a passthrough in the switch-case statement and let the
 *  MDCM_MACED case handle this.
 *
 *  2. Plain data transfer with MAC
 *  If legacy authentication only verify the MAC if communication settings 
 *  require this. 
 *  Update nbytes to not count # of MAC bytes.
 *  Get length of data to be enciphered, which is all data bytes but status.
 *  Allocate memory for this.
 *  Copy over all data bytes (excluding status byte), with appropriate padding.
 *  Go on to Generate the MAC and append this to the duplicate data buffer.
 *  Compare this generated data and MAC buffer to the original data buffer. If
 *  they aren't equivalent, then there is a problem.
 *
 *  If new authentication mode, and CMAC is appended to Tx'd commands, then
 *  there is nothing else to be done here.
 *  If however we are using new auth, and the communication settings require
 *  Rx'd CMACs are verified, then do just that.
 *  The first step is checking that the total number of bytes (including status
 *  byte) isn't less than 9. If it is, then there will be no space for a CMAC,
 *  which is fixed at 8 bytes.
 *  Save the first CMAC byte, then move the status byte to its position, right
 *  after the payload.
 *  Generate CMAC for new payload (including status).
 *  If communication settings require CMAC verification, restore the original
 *  first cmac byte to its old slot right after the payload, (where status byte
 *  is now).
 *  If generated cmac is not equal to the passed in CMAC, then CMAC isn't
 *  verified and there is a crypto error.
 *
 *  3. DES/3DES encrypted data transfer
 *  Use the following data format for reference
 *
 *  AS_LEGACY:
 *  ,-----------------+-------------------------------+--==----+
 *  \     BLOCK n-1   |              BLOCK n          | STATUS |
 *  /  PAYLOAD | CRC0 | CRC1 | 0x80? | 0x000000000000 |  0x00  |
 *  `-----------------+-------------------------------+--------+
 * 
 *          <------------ DATA ------------>
 *  FRAME = PAYLOAD + CRC(PAYLOAD) + PADDING
 * 
 *  AS_NEW:
 *  ,-----------------------+------------------------------------------+------+
 *  \         BLOCK n-1     |                 BLOCK n                  |STATUS|
 *  / PAYLOAD|CRC0|CRC1|CRC2| CRC3|0x80?|0x0000000000000000000000000000| 0x00 |
 *  `-----------------------+------------------------------------------+------+
 *  <-------------------------------- DATA --------------------------->|
 * 
 *          <----------------- DATA ---------------->
 *  FRAME = PAYLOAD + CRC(PAYLOAD + STATUS) + PADDING + STATUS
 *                                     `------------------'
 *  First decipher the passed in data.
 *  Then look for the CRC and ensure it is either at the end of a block or is
 *  followed by NULL padding.
 *  To do this we have to start from minimum possible CRC first byte index (will
 *  call this crc position from here on out). 
 *
 *  For legacy authentication, you can see from above diagram that:
 *  crc position = # of bytes - block size (8) - 1 is the lowest crc position
 *  if this calculation yields a negative number, reset it to 0.
 *  
 *  For new authentication scheme, allocate space in the tag's crypto buffer
 *  of the passed in number of data bytes + 1. This is because we will be
 *  inserting a copy of the status byte after the payload. Copy in the data
 *  bytes into this new result buffer (yes there will be an empty slot at the
 *  end).
 *  Now from the above diagram, we can see the minimum crc position is:
 *  crc position = # of bytes - block size (16) - 3 is the lowest crc position.
 *  again, if this calculation yields a negative number, reset it to 0.
 *  Now inset the status byte (OPERATION_OK), into the spot right after the
 *  payload in this crypto buffer, so this initial crc position is now bumped up
 *  Don't forget to update the total # of bytes to account for this status byte.
 *
 *  now that lowest possible crc position has been determined, loop through the
 *  crypto buffer which now contains the decrypted data buffer till either
 *  the CRC bytes are verified or we run out of crypto buffer slots.
 *   for each crc position, calculate the crc (16 bytes for legacy, 32 bytes for
 *   new authentication). This crc is calculated on payload+crc, so it will be 0
 *   if our calculated crc matches the passed in crc.
 *   if calculated crc is 0, then the Rx'd data can be further verified for
 *   padding, by looping through the remaining data bytes except the last one.
 *   The idea is, if the loop never runs, it means we are already at the last
 *   data byte, so no padding needed. If however it does run then we expect
 *   padding of the form 0x80...00
 *   
 *   If the padding is fine, then this Rx'd data is verified and we have to
 *   status is before the CRC before returning (for both auth. schemes).
 *   However if Rx'd data isn't verified, do nothing but continue searching
 *   if using legacy scheme. If however we are using a new authentication scheme
 *   move the status byte up one byte. This move should happen via a swap.
 *   The idea is what we need to reorganize the payload.
 * 
 *  If after all this, we still haven't verified the CRC, then there is a crypto
 *  error.
 *
 *
 * Assumptions:
 *  Status byte comes last
 *
 * Revision History:
 *  Jan. 06, 2012      Nnoduka Eruchalu     Initial Revision
 */
uint8_t *MifareCryptoPostprocessData (mifare_tag *tag, uint8_t *data, 
                                      ssize_t *nbytes, 
                                      int communication_settings)
{
  uint8_t *res = data;         /* with no processing, result is original data */
  size_t edl;                  /* encrypted data length */           
  uint8_t edata[MAX_ENCIPHERED_DATA_LENGTH];  /* encrypted data array */
  uint8_t first_cmac_byte;
  uint8_t num_cmac_bytes = 0;  /* assume no cmac bytes */
  
  uint8_t verified;          /* verified? boolean */
  int crc_pos;               /* index of first crc byte */
  int end_crc_pos;           /* index of byte right after crc */
  uint8_t x;                 /* temp byte for use in swap */
  int n;                     /* index for looping through crc bytes */
  
  mifare_desfire_key *key = &tag->session_key; 
  if (!key) return data;       /* if no session key, then no processing */  
  if(1 == *nbytes) return res; /* if we just have a status code, no processing*/
  
  switch(communication_settings & MDCM_MASK) {
  case MDCM_PLAIN:             /* plain data transfer */
    if(AS_LEGACY == tag->authentication_scheme)
      break;                   /* legacy auth scheme means no processing */
                               /* but new auth scheme means we expect a CMAC */
                               /* so pass through */
  case MDCM_MACED:             /* plain data transfer with MAC */
    switch(tag->authentication_scheme) {
    case AS_LEGACY:            /* if legacy authentication scheme and */
      if (communication_settings & MAC_VERIFY) { /* required to verify MAC */
        *nbytes -= KeyMacingLength(key); /* update nbytes to exclude MAC bytes*/
        /* nbytes still has status byte so use -1 to get just payload */
        edl = EncipheredDataLength(tag, *nbytes - 1, communication_settings);
        /* edata must be allocated to at least edl bytes */
        memcpy (edata, data, *nbytes - 1); /* and load with passed in data */
                                           /* then pad with 0s */
        memset ((uint8_t *)edata + *nbytes - 1, 0, edl - (*nbytes - 1));
        /* generate MAC */
        MifareCipherBlocksChained(tag, NULL, NULL, edata, edl, MCD_SEND,
                                  MCO_ENCIPHER);
        /* compare generated MAC with received MAC */
        if (0 != memcmp ((uint8_t *)data + *nbytes - 1,
                         (uint8_t *)edata + edl - 8, 
                         MAC_LENGTH)) {       /* if MAC isn't verified, there */
          tag->last_pcd_error = CRYPTO_ERROR; /* is a crypto error */
          *nbytes = -1;
          res = NULL;
        } else {                             /* MAC verified so update nbytes*/
          *nbytes -= MAC_LENGTH;             /* to not count MAC bytes, and  */
          ((uint8_t *)res)[*nbytes - 1] = 0x00; /* place status after payload */
        }
      }
      break;
      
    case AS_NEW:               /* if new auth. scheme and not generating MAC */
      if (!(communication_settings & CMAC_COMMAND)) /* then done */
        break;                 /* if new auth scheme and required to verify */
      if (communication_settings & CMAC_VERIFY) {   /*CMAC,get first CMAC byte*/
        if (*nbytes < (1+CMAC_LENGTH)) /* but first check there is room for */
          break;                       /* CMAC. If there isn't, abort */
        
        /* temporarily place status byte right after data bytes */
        first_cmac_byte = ((uint8_t *)data)[*nbytes - 1 - CMAC_LENGTH];
        ((uint8_t *)data)[*nbytes -1-CMAC_LENGTH] =((uint8_t *)data)[*nbytes-1];
        num_cmac_bytes = CMAC_LENGTH;  /* have confirmed number of CMAC bytes */
      }
      /* recompute Cmac on data+status */
      Cmac(key, tag->ivect, ((uint8_t *)data), (*nbytes - num_cmac_bytes),
           tag->cmac);         /* generate CMAC */
      if (communication_settings & CMAC_VERIFY) { /*if required to verify CMAC*/
        ((uint8_t *)data)[*nbytes - 1-CMAC_LENGTH] = first_cmac_byte;
        if (0 != memcmp(tag->cmac,                /* CMP generated with rx'd */
                        (uint8_t *)data + *nbytes -1- CMAC_LENGTH,
                        CMAC_LENGTH)) {      /* If CMAC isn't verified, there */
          tag->last_pcd_error = CRYPTO_ERROR;/* is a crypto eror*/
          *nbytes = -1;
          res = NULL;
        } else {                             /* CMAC verified so update nbytes*/
          *nbytes -= CMAC_LENGTH;            /* to not count CMAC bytes, and  */
          ((uint8_t *)res)[*nbytes - 1] = 0x00; /* place status after payload */
        }
      }
      break;
    }                          /* end switch(tag->authentication_scheme) */
    
    break;                     /* done handling MACed data transfer */
    
  case MDCM_ENCIPHERED:        /* DES/3DES ecnrypted data transfer */
    (*nbytes)--;               /* update nbytes to exclude status byte */
    verified = FALSE;          /* start by assuming not verified */
            
    MifareCipherBlocksChained(tag, NULL, NULL, res, *nbytes, MCD_RECEIVE,
                              MCO_DECIPHER); /* first decipher Rx'd data */
    /*
     * Then Look for the CRC and ensure it is followed by NULL padding.  We
     * can't start by the end because the CRC is supposed to be 0 when
     * verified, and accumulating 0's in it should not change it.
     *
     * get the lowest possible byte indices for the crc's
     */
    switch (tag->authentication_scheme) {
    case AS_LEGACY:
      crc_pos = *nbytes - 8 - 1;    /* The CRC can be over two blocks */
      if (crc_pos < 0) crc_pos = 0; /* but we could also be working with a */
      break;                        /* single block */
    case AS_NEW:
      /* CRC is computed on payload and status, so
       * Move status between payload and CRC; will need extra byte for that */
      /* crypto buffer is (or is reallocated to) at least as big as *nbytes +1*/
      res = tag->crypto_buffer;
      memcpy (res, data, *nbytes); /* copy data and crc again */
      
      crc_pos = (*nbytes) - 16 - 3; /* the CRC can be over two blocks */
      if (crc_pos < 0) crc_pos = 0; /* Single block */
 
      /* shift crc bytes up a byte. creating a blank slot between payload and 
       * crc. Note that number of crc bytes = nbytes - index of first crc byte 
       */
      memmove((uint8_t *)res + crc_pos + 1, (uint8_t *)res + crc_pos, 
              *nbytes - crc_pos);
      ((uint8_t *)res)[crc_pos] = 0x00; /* move status byte, OPERATION_OK, */
      crc_pos++;               /* between payload and crc. Don't forget to */
      *nbytes += 1;            /* update crc's first byte index, and the # of */
      break;                   /* data bytes to account for inserted status */
    }                          /* end switch(tag->authentication_scheme) */
    
                               /* starting from lowest possible crc indices */
    do {                       /* move up slowly till actual crc bytes are hit*/
      uint16_t crc16;          /* 16 byte CRC */
      uint32_t crc;            /* 32 byte CRC or padded 16 byte CRC */
      
      switch (tag->authentication_scheme) {
      case AS_LEGACY:          /* calculate CRC depending on auth. scheme */
        end_crc_pos = crc_pos + 2; /* get number of bytes to calculate CRC on */
        MifareCrc16(res, end_crc_pos, (uint8_t *)&crc16); /* calc. new  crc */
        crc = crc16;           /* the trick here is it doesn't matter if */
        break;                 /* using little endian or big endian. the crc */
      case AS_NEW:             /* should be all 0s */
        end_crc_pos = crc_pos + 4; 
        MifareCrc32 (res, end_crc_pos, (uint8_t *)&crc);
        break;
      }                        /* end switch(tag->authentication_scheme) */
      
      if (!crc) {              /* if calculated crc is 0, then the Rx'd data */
        verified = TRUE;       /* can be further verified for padding */
        
        /* note the trick here, with checking for n < *nbytes - 1. the idea is
         * if padding wasn't needed then this loop will never run because
         * end_crc_pos == *nbytes
         */
        for (n = end_crc_pos; n < *nbytes - 1; n++) { /* which should be */
          uint8_t byte = ((uint8_t *)res)[n];         /* 0x80 00...00    */
          if (!( (0x00 == byte) || 
                 ((0x80 == byte) && (n == end_crc_pos)) )) {
            verified = FALSE;
          }
        }
      }
      
      if (verified) {          /* if Rx'd data has been verified update nbytes*/
        *nbytes = crc_pos;     /* to account for just payload and status */
        switch (tag->authentication_scheme) {
        case AS_LEGACY:        /* for legacy authentication place status byte */
          ((uint8_t *)data)[(*nbytes)++] = 0x00;
          break;               /* right after payload */
        case AS_NEW:           /* for new authentication, status byte was */
          break;               /* already before the CRC */
        }                      /* end switch(tag->authentication_scheme) */
        
      } else {                 /* if Rx'd data isn't verified */
        switch (tag->authentication_scheme) {
        case AS_LEGACY:        /* and auth scheme is legacy, continue search */
          break;               /* for first crc byte if possible */
        case AS_NEW:           /* if however auth scheme is new, then move */
          x = ((uint8_t *)res)[crc_pos - 1]; /* STATUS byte up with a swap */
          ((uint8_t *)res)[crc_pos - 1] = ((uint8_t *)res)[crc_pos];
          ((uint8_t *)res)[crc_pos] = x;    /* search for first crc byte */
          break;
        }                      /* end switch(tag->authentication_scheme) */
        crc_pos++;             /* maybe next byte is first crc byte? */
      }
    } while (!verified && (end_crc_pos < *nbytes));
    
    /* FIXME In some configurations, the file is transmitted PLAIN */
    if (!verified) { /* if CRC not verified in deciphered stream */
      tag->last_pcd_error = CRYPTO_ERROR; /* there is crypto error */
      *nbytes = -1;
      res = NULL;
    }

    break;                     /* done handling ecnrypted data transfer */
    
  default:                     /* unknown communication settings */
    res = NULL;                /* do nothing, just setup a blank result ptr */
    *nbytes = -1;
    break;
  }                            /* end switch(communication_settings&MDCM_MASK)*/
  
  return res;                  /* return final data */
}


/*
 * MifareCipherSingleBlock
 * Description: This function performs Block Ciphering/Deciphering.
 *
 * Operation:
 *   First check the direction of the cipher, if it is a Send operation, Xor the
 *   ivect into data. If however it is a Receive operation, copy the data into
 *   the ovect. Now the data is ready for processing.
 *  
 *   For single DES:
 *   If Encipher Operation: Do a simple DES encrypt
 *   If Decipher Operation: Do a simple DES decrypt
 *
 *  For triple DES:
 *   If Encipher Operation: DES encrypt data using ks1, DES decrypt using ks2,
 *                          DES encrypt again using ks1
 *   If Decipher Operation: DES decrypt data using ks1, DES encrypt using ks2,
 *                          DES decrypt again using ks1
 *
 *   For 3-key triple DES:
 *   If Encipher Operation: DES encrypt data using ks1, DES decrypt using ks2,
 *                          DES encrypt again using ks3
 *   If Decipher Operation: DES decrypt data using ks3, DES encrypt using ks2,
 *                          DES decrypt again using ks1
 *
 *   For AES:
 *   If Encipher Operatin: set 128 bit AES encrypt key then AES encrypt
 *   If Decipher Operatin: set 128 bit AES decrypt key then AES decrypt
 * 
 *   Again check the direction of the cipher, if it is a Send operation, copy
 *   data into the ivect. If however it is a Receive operation, xor ivect into
 *   the data, then copy ovect into ivect.
 *
 * Revision History:
 *   Jan. 16, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareCipherSingleBlock(mifare_desfire_key *key, uint8_t *data,
                             uint8_t *ivect, mifare_crypto_direction direction,
                             mifare_crypto_operation operation,
                             size_t block_size)
{
  AES_KEY k;
  uint8_t ovect[MAX_CRYPTO_BLOCK_SIZE];
  uint8_t edata[MAX_CRYPTO_BLOCK_SIZE];

  if (direction == MCD_SEND) {       /* for direction is send */
    Xor(ivect, data, block_size);    /* xor ivect into data */
  } else {                           /* for direction is receive */
    memcpy(ovect, data, block_size); /* copy data into ovect */
  }
  
  switch (key->type) {
  case T_DES:                        /* single DES */
    switch (operation) {
    case MCO_ENCIPHER:
      DES_ecb_encrypt((DES_cblock *) data, (DES_cblock *) edata, &(key->ks1),
                      DES_ENCRYPT);
      break;
    case MCO_DECIPHER:
      DES_ecb_encrypt((DES_cblock *) data, (DES_cblock *) edata, &(key->ks1),
                      DES_DECRYPT);
      break;
    }
    break;
    
  case T_3DES:                       /* triple DES */
    switch (operation) {
    case MCO_ENCIPHER:
      DES_ecb_encrypt((DES_cblock *) data,  (DES_cblock *) edata, &(key->ks1),
                      DES_ENCRYPT);
      DES_ecb_encrypt((DES_cblock *) edata, (DES_cblock *) data,  &(key->ks2),
                      DES_DECRYPT);
      DES_ecb_encrypt((DES_cblock *) data,  (DES_cblock *) edata, &(key->ks1),
                      DES_ENCRYPT);
      break;
    case MCO_DECIPHER:
      DES_ecb_encrypt((DES_cblock *) data,  (DES_cblock *) edata, &(key->ks1),
                      DES_DECRYPT);
      DES_ecb_encrypt((DES_cblock *) edata, (DES_cblock *) data,  &(key->ks2), 
                      DES_ENCRYPT);
      DES_ecb_encrypt((DES_cblock *) data,  (DES_cblock *) edata, &(key->ks1), 
                      DES_DECRYPT);
      break;
    }
    break;
    
  case T_3K3DES:                     /* 3-key triple DES */
    switch (operation) {
    case MCO_ENCIPHER:
      DES_ecb_encrypt((DES_cblock *) data,  (DES_cblock *) edata, &(key->ks1),
                      DES_ENCRYPT);
      DES_ecb_encrypt((DES_cblock *) edata, (DES_cblock *) data,  &(key->ks2),
                      DES_DECRYPT);
      DES_ecb_encrypt((DES_cblock *) data,  (DES_cblock *) edata, &(key->ks3),
                      DES_ENCRYPT);
      break;
    case MCO_DECIPHER:
      DES_ecb_encrypt((DES_cblock *) data,  (DES_cblock *) edata, &(key->ks3),
                      DES_DECRYPT);
      DES_ecb_encrypt((DES_cblock *) edata, (DES_cblock *) data,  &(key->ks2),
                      DES_ENCRYPT);
      DES_ecb_encrypt((DES_cblock *) data,  (DES_cblock *) edata, &(key->ks1),
                      DES_DECRYPT);
      break;
    }
    break;
    
  case T_AES:                        /* AES */
    switch (operation) {
    case MCO_ENCIPHER:
      AES_set_encrypt_key(key->data, 8*16, &k);
      AES_encrypt(data, edata, &k);
      break;
    case MCO_DECIPHER:
      AES_set_decrypt_key(key->data, 8*16, &k);
      AES_decrypt(data, edata, &k);
      break;
    }
    break;
  }
  
  memcpy (data, edata, block_size);
  
  if (direction == MCD_SEND) {        /* if direction is send, */
    memcpy (ivect, data, block_size); /* copy data into ivect */
  } else {                            /* if direction is receive, */
    Xor(ivect, data, block_size);     /* xor ivect into data */
    memcpy (ivect, ovect, block_size);/* copy ovect into ivect */
  }
}


/*
 * MifareCipherBlocksChained
 * Description: This function performs all CBC Ciphering/Deciphering.
 *              The tag argument may be NULL, in which case both key and ivect 
 *              shall be set.
 *              When using the tag session_key and ivect for processing data, 
 *              these arguments should be set to NULL.
 *
 *              Because the tag may contain additional data, one may need to 
 *              call this function with tag, key and ivect defined
 *
 * Return:      None
 *
 * Operation:   If a tag isn't passed in then key and ivect must be set.
 *              If tag is defined, and key isn't defined, then set it to tag's 
 *              session key.
 *              If tag is defined, and ivect isn't defined, then set it to tag' 
 *              ivect.
 *              If LEGACY authentication scheme, set ivect to all 0s
 *              If at this point key or ivect isn't defined, then this is a 
 *              problem, so abort.
 *              Perform single block ciphers on contiguous data blocks.
 *
 * Revision History:
 *   Jan. 16, 2012      Nnoduka Eruchalu     Initial Revision
 */
void MifareCipherBlocksChained(mifare_tag *tag, mifare_desfire_key *key,
                               uint8_t *ivect, uint8_t *data, size_t data_size,
                               mifare_crypto_direction direction,
                               mifare_crypto_operation operation)
{
  size_t block_size;
  size_t offset = 0;
  
  if(tag) {                          /* if tag is defined and key isn't set */
    if(!key)                         /* key to tag's session key */
      key = &tag->session_key;
    if(!ivect)                       /* if tag is defined and ivect isn't */
      ivect = tag->ivect;            /* set ivect to tag's ivect */
    
    switch (tag->authentication_scheme) {
    case AS_LEGACY:                  /* if legacy auth scheme, ivect = all 0s */
      memset (ivect, 0, MAX_CRYPTO_BLOCK_SIZE);
      break;
    case AS_NEW:
      break;
    }
  }
  
  if (!key || !ivect)                /* if no key or ivect defined, there is */
    return;                          /* a problem so abort */
  
  block_size = KeyBlockSize(key);    /* get key block size for block ciphers */
  
  while (offset < data_size) {       /* Cipher contiguous data blocks */
    MifareCipherSingleBlock(key, data + offset, ivect, direction, operation,
                            block_size);
    offset += block_size;
  }
}
