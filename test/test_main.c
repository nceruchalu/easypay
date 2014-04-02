/* compile with:
 * make
*/
#include "test_general.h"
#include "test_main.h"


int main(void)
{
  test_aes();
  test_des();
  test_queue();
  test_mifare_desfire_aes();
  test_mifare_desfire_des();
  test_mifare_desfire_key();
  test_mifare_aid();
  test_mifare_crypto();
 
  test_print_stats();
  return 0;
}
