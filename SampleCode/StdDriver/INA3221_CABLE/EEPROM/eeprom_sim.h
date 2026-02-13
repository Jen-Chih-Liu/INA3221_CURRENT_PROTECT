#ifndef __EEPROM_EMULATE_H__
#define __EEPROM_EMULATE_H__

#include "NuMicro.h"

/* 定義 EEPROM 控制結構體 */
typedef struct {
    uint32_t Flash_BaseAddr;     // Flash 起始位址
    uint32_t Amount_Pages;       // 使用的頁數 (例如 10)
    uint32_t Amount_of_Data;     // 有效資料長度 (Max 255)
    uint32_t Current_Valid_Page; // 目前有效頁面索引
    uint32_t Current_Cursor;     // 目前寫入游標位置
    uint8_t  *Written_Data;      // SRAM 緩衝區指標
} EEPROM_Ctx_T;

/* 錯誤代碼定義 */
#define Err_OverAmountData      0x01
#define Err_OverPageAmount      0x02
#define Err_ErrorIndex          0x03
#define Err_WriteBlockStatus    0x04

/* 功能函式宣告 */
void FMC_Enable(void); // 統一改名為標準的 FMC_Enable
uint32_t Init_EEPROM(EEPROM_Ctx_T *ctx, uint32_t base_addr, uint32_t data_size, uint32_t use_pages);
uint32_t Read_Data(EEPROM_Ctx_T *ctx, uint8_t index, uint8_t *data);
uint32_t Write_Data(EEPROM_Ctx_T *ctx, uint8_t index, uint8_t data);
uint16_t Get_Cycle_Counter(EEPROM_Ctx_T *ctx);
uint32_t Read_EEPROM(EEPROM_Ctx_T *ctx, uint32_t offset, uint8_t *data, uint32_t len);
uint32_t Write_EEPROM(EEPROM_Ctx_T *ctx, uint32_t offset, uint8_t *data, uint32_t len);

#endif