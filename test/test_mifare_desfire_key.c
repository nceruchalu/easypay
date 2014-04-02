#include <string.h>
#include "../mifare/mifare.h"
#include "../mifare/mifare_crypto.h"
#include "../mifare/mifare_key.h"
#include "test_general.h"

void test_mifare_desfire_key (void)
{
  mifare_desfire_key key;
  int version;
  
  uint8_t key1_des_data[8] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 };
  uint8_t key2_des_data[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  
  uint8_t key1_3des_data[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0XEE, 0xFF};
  uint8_t key2_3des_data[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0X01, 0x00,
                                0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
  uint8_t key3_3des_data[16] = {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0X00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x77};
    
  MifareDesKeyNew(&key, key1_des_data);
  version = MifareKeyGetVersion(&key);
  assert_equal_int(0x00, version, "Wrong MifareDESFireKey version 1");

  MifareDesKeyNewWithVersion(&key, key1_des_data);
  version = MifareKeyGetVersion(&key);
  assert_equal_int(0x55, version, "Wrong MifareDESFireKey version 2");
  
  MifareKeySetVersion(&key, 0xaa);
  version = MifareKeyGetVersion(&key);
  assert_equal_int(0xaa, version, "Wrong MifareDESFireKey version 3");
  
  MifareDesKeyNew(&key, key2_des_data);
  version = MifareKeyGetVersion(&key);
  assert_equal_int(0x00, version, "Wrong MifareDESFireKey version 4");
    
  MifareDesKeyNewWithVersion(&key, key2_des_data);
  version = MifareKeyGetVersion(&key);
  assert_equal_int(0x00, version, "Wrong MifareDESFireKey version 5");
    
  Mifare3DesKeyNew(&key, key1_3des_data);
  version = MifareKeyGetVersion(&key);
  assert_equal_int(0x00, version, "Wrong MifareDESFireKey version 6");
    
  Mifare3DesKeyNewWithVersion(&key, key1_3des_data);
  version = MifareKeyGetVersion(&key);
  assert_equal_int(0x55, version, "Wrong MifareDESFireKey version 7");
  
  MifareKeySetVersion(&key, 0xaa);
  version = MifareKeyGetVersion(&key);
  assert_equal_int(0xaa, version, "Wrong MifareDESFireKey version 8");
    
  Mifare3DesKeyNew(&key, key2_3des_data);
  version = MifareKeyGetVersion(&key);
  assert_equal_int(0x00, version, "Wrong MifareDESFireKey version 9");
  
  Mifare3DesKeyNewWithVersion(&key, key2_3des_data);
  version = MifareKeyGetVersion(&key);
  assert_equal_int(0x02, version, "Wrong MifareDESFireKey version 10");
    
  Mifare3DesKeyNew(&key, key3_3des_data);
  version = MifareKeyGetVersion(&key);
  assert_equal_int(0x00, version, "Wrong MifareDESFireKey version 11");
  
  Mifare3DesKeyNewWithVersion(&key, key3_3des_data);
  version = MifareKeyGetVersion(&key);
  assert_equal_int (0x10, version, "Wrong MifareDESFireKey version 12");
}
