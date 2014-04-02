#include <string.h>
#include "../mifare/mifare.h"
#include "../mifare/mifare_aid.h"
#include "test_general.h"


void test_mifare_aid(void)
{
  mifare_desfire_aid aid;
  uint8_t expected_data[3] = {0x56, 0x34, 0x12}; /* little endian */
  uint32_t rx_aid, expected_aid = 0x00123456;
    
  MifareAidNew(&aid, expected_aid); 
  assert_equal_memory(expected_data,3, aid.data, sizeof(aid.data),
                "AID: wrong data created");
    
  rx_aid = MifareAidGetAid(&aid);
  assert_equal_int(expected_aid, rx_aid, "AID: wrong data retrieved");
}
