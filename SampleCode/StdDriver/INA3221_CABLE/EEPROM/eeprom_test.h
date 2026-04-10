#ifndef __EEPROM_TEST_H__
#define __EEPROM_TEST_H__

#include "eeprom_sim.h"

/**
 * @brief Perform stress test on the EEPROM simulation
 * @param ctx Pointer to an initialized EEPROM context
 * @return 0 on success, non-zero on failure
 */
int eeprom_stress_test(EEPROM_Ctx_T *ctx);

#endif // __EEPROM_TEST_H__
