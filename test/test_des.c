#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../mifare/des.h"
#include "test_general.h"


#define BUFSIZE 64


void test_des(void)
{
  unsigned char in[BUFSIZE], out[BUFSIZE], back[BUFSIZE];
  unsigned char expected_back[BUFSIZE];
  unsigned char expected_out[BUFSIZE] =
    {0xc8, 0x66, 0xf7, 0x14, 0xfb, 0xfa, 0x95, 0x78, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00};

    
  DES_cblock key = {1,5, 7};
  DES_key_schedule keysched;
  DES_set_key(&key, &keysched);
  
    
  memset(in, 0, sizeof(in));
  memset(out, 0, sizeof(out));
  memset(back, 0, sizeof(back));
  memset(expected_back, 0, sizeof(expected_back));
  
  /* 8 bytes of plaintext */
  memcpy(in, "HillTown", 9);
  memcpy(expected_back, in, BUFSIZE); /* expect input back  */
  
  /* encrypt */
  DES_ecb_encrypt((DES_cblock *)in,(DES_cblock *)out, &keysched, DES_ENCRYPT);
  
  /* decrypt */
  DES_ecb_encrypt((DES_cblock *)out,(DES_cblock *)back, &keysched, DES_DECRYPT);
    
  assert_equal_memory(out, BUFSIZE, expected_out, BUFSIZE,
                      "DES: Wrong encrypted data");
  assert_equal_memory(back, BUFSIZE, expected_back, BUFSIZE,
                      "DES: Wrong decrypted data");
}

