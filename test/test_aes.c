#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../mifare/aes.h"
#include "test_general.h"


#define BUFSIZE 16



void test256bitkey(void) {
  static const unsigned char key[] = { 
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f 
  }; 
  
  AES_KEY ectx; 
  AES_KEY dectx; 
  
  unsigned char in[BUFSIZE], out[BUFSIZE], back[BUFSIZE];
  unsigned char expected_back[BUFSIZE];
  unsigned char expected_out[BUFSIZE] = {
    0xfe, 0x40, 0xc2, 0xb0, 0xaa, 0x6c, 0xec, 0xea,
    0x4b, 0xf5, 0x90, 0x0d, 0x1a, 0xbb, 0x29, 0x82};

  memset(in, 0, sizeof(in));
  memset(out, 0, sizeof(out));
  memset(back, 0, sizeof(back));
  memset(expected_back, 0, sizeof(expected_back));
  
  
  memcpy(in, "hello world", 12); /* set random plaintext input */
  memcpy(expected_back, in, BUFSIZE); /* expect input back  */
  
  AES_set_encrypt_key(key, 256, &ectx); /* encrypt text */ 
  AES_encrypt(in, out, &ectx); 
  
  AES_set_decrypt_key(key, 256, &dectx); /* decrypt it */
  AES_decrypt(out, back, &dectx); 
    
  assert_equal_memory(out, BUFSIZE, expected_out, BUFSIZE,
                      "AES: Wrong 256 bit encrypted data");
  assert_equal_memory(back, BUFSIZE, expected_back, BUFSIZE,
                      "AES: Wrong 256 bit decrypted data");
}

/*
 * An example of using the AES block cipher,
 * with key (in hex) 01000000000000000000000000000000
 * and input (in hex) 01000000000000000000000000000000.
 * The result should be the output (in hex) FAEB01888D2E92AEE70ECC1C638BF6D6
 * (AES_encrypt corresponds to computing AES in the forward direction.)
 * We then compute the inverse, and check that we recovered the original input.
 * (AES_decrypt corresponds to computing the inverse of AES.)
 * (The terminology "AES_encrypt" and "AES_decrypt" is unfortunate since
 * AES is a block cipher, not an encryption scheme.
 */
void test128bitkey(void) {
  AES_KEY AESkey;
  unsigned char MBlock[BUFSIZE];
  unsigned char MBlock2[BUFSIZE];
  unsigned char CBlock[BUFSIZE];
  unsigned char Key[16];
  int i;
  unsigned char expected_CBlock[BUFSIZE] =
    {0xfa, 0xeb, 0x01, 0x88, 0x8d, 0x2e, 0x92, 0xae,
     0xe7, 0x0e, 0xcc, 0x1c, 0x63, 0x8b, 0xf6, 0xd6};
  unsigned char expected_MBlock2[BUFSIZE];
  
  /* 
   * Key contains the actual 128-bit AES key. AESkey is a data structure 
   * holding a transformed version of the key, for efficiency. 
   */
  Key[0]=1;
  for (i=1; i<=15; i++) {
    Key[i] = 0;
  } 
  
  MBlock[0] = expected_MBlock2[0] = 1;   /* set message block and expected */
  for (i=1; i<16; i++)                   /* message block equal to each other */
    MBlock[i] =  expected_MBlock2[i] = 0;
  
  AES_set_encrypt_key((const unsigned char *) Key, 128, &AESkey);
  AES_encrypt((const unsigned char *) MBlock, CBlock,(const AES_KEY *) &AESkey);
  
  /* 
   * We need to set AESkey appropriately before inverting AES. 
   * Note that the underlying key Key is the same; just the data structure
   * AESkey is changing (for reasons of efficiency).
   */
  AES_set_decrypt_key((const unsigned char *) Key, 128, &AESkey);
  AES_decrypt((const unsigned char *) CBlock, MBlock2,(const AES_KEY *)&AESkey);
  
  assert_equal_memory(CBlock, BUFSIZE, expected_CBlock, BUFSIZE,
                      "AES: Wrong 128 bit encrypted data");
  assert_equal_memory(&MBlock, BUFSIZE, &expected_MBlock2, BUFSIZE,
                      "AES: Wrong 128 bit decrypted data");
  
}


void test_aes(void)
{
  test256bitkey();
  test128bitkey();
}


