#ifndef __EEPROM_TEST_H__
#define __EEPROM_TEST_H__

#include "eeprom_sim.h"

/**
 * @brief 執行 EEPROM 模擬的壓力測試
 * @param ctx 指向已初始化的 EEPROM context
 * @return 0 代表成功, 非 0 代表失敗
 */
int eeprom_stress_test(EEPROM_Ctx_T *ctx);

#endif // __EEPROM_TEST_H__
