#include "../mifare/mifare.h"
#include "../mifare/mifare_crypto.h"
#include "../mifare/mifare_key.h"
#include "test_general.h"


void test_mifare_rol(void)
{
  uint8_t data[8] = "01234567";
  uint8_t data2[16] = "0123456789abcdef";
  
  Rol(data, 8);
  assert_equal_memory("12345670", 8, data, 8, "Rol: Wrong data 1");
    
  Rol(data2, 16);
  assert_equal_memory(data2, 16, "123456789abcdef0", 16, "Rol: Wrong data 2");
}


void test_mifare_desfire_des_receive(void)
{
  uint8_t null_ivect[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  
  uint8_t data[8]  = { 0xd6, 0x59, 0xe1, 0x70, 0x43, 0xa8, 0x40, 0x68 };
  uint8_t key_data[8]   = { 1, 1, 1, 1, 1, 1, 1, 1 };
  mifare_desfire_key key;
    
  uint8_t expected_data[8]  = { 0x73, 0x0d, 0xdf, 0xad, 0xa4, 0xd2, 0x07, 0x89};
  uint8_t expected_key[8]   = { 1, 1, 1, 1, 1, 1, 1, 1 };
  
  MifareDesKeyNewWithVersion(&key, key_data);
  
  MifareCipherBlocksChained(NULL, &key, null_ivect, data, 8, MCD_RECEIVE,
                            MCO_DECIPHER);
  
  assert_equal_memory(&expected_data, 8, &data, 8, "DES receive 1: Wrong data");
  assert_equal_memory(&expected_key, 8, key.data, 8, "DES receive: Wrong key");
}


void test_mifare_desfire_des_send(void)
{
  uint8_t null_ivect[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  
  uint8_t data[8]  = { 0x73, 0x0d, 0xdf, 0xad, 0xa4, 0xd2, 0x07, 0x89 };
  uint8_t key_data[8]   = { 1, 1, 1, 1, 1, 1, 1, 1 };
  mifare_desfire_key key;
  
  uint8_t expected_data[8] = { 0xd6, 0x59, 0xe1, 0x70, 0x43, 0xa8, 0x40, 0x68 };
  uint8_t expected_key[8]   = { 1, 1, 1, 1, 1, 1, 1, 1 };
  
  MifareDesKeyNewWithVersion(&key, key_data);
  
  MifareCipherBlocksChained(NULL, &key, null_ivect, data, 8, MCD_SEND,
                            MCO_DECIPHER);
  
  assert_equal_memory(&expected_data, 8, &data, 8, "DES send: Wrong data");
  assert_equal_memory(&expected_key, 8, key.data, 8, "DES send: Wrong key");
}


void test_mifare_desfire_padded_data_length (void)
{
    size_t res;

    res = PaddedDataLength(0,  8);
    assert_equal_int(res, 8,  "PaddedDataLength: Invalid size 1");
    res = PaddedDataLength(1,  8);
    assert_equal_int(res, 8,  "PaddedDataLength: Invalid size 2");
    res = PaddedDataLength(8,  8);
    assert_equal_int(res, 8,  "PaddedDataLength: Invalid size 3");
    res = PaddedDataLength(9,  8);
    assert_equal_int(res, 16, "PaddedDataLength: Invalid size 4");
    res = PaddedDataLength(0,  16);
    assert_equal_int(res, 16, "PaddedDataLength: Invalid size 5");
    res = PaddedDataLength(33, 16);
    assert_equal_int(res, 48, "PaddedDataLength: Invalid size 6");
}



void test_mifare_desfire_des(void)
{
  test_mifare_rol();
  test_mifare_desfire_des_receive();
  test_mifare_desfire_des_send();
  test_mifare_desfire_padded_data_length();
}
