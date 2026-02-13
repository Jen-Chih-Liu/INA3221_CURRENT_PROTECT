/******************************************************************************
 * @file     Flash.h
 * @brief    Flash header file
 *
 * @note
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __Flash_H__
#define __Flash_H__

#include <stdint.h>
#include "EEPROM_FRU_Table.h"

/*------Define Data Flash for Serial Number and Build Data----*/
#define FLASH_ADDR_FRU_BOARD_PART_NUMBER           (0xEA00)
#define FLASH_ADDR_FRU_OEM_INFORMATION             (0xEC00)
#define FLASH_ADDR_FRU_SERIAL_NUMBER               (0xEE00)
#define FLASH_ADDR_FRU_MARKETING_NAME              (0xF000)
#define FLASH_ADDR_FRU_BUILD_DATE                  (0xF200)
#define FLASH_ADDR_FRU_HW_VERSION                  (0xF400)
#define FLASH_ADDR_FRU_FW_VERSION                  (0xF600)
#define FLASH_ADDR_FRU_PCI_CONFIG_VENDOR_ID        (0xF800)
#define FLASH_ADDR_FRU_PCI_CONFIG_DEVICE_ID        (0xFA00)
#define FLASH_ADDR_FRU_PCI_CONFIG_SUB_VENDOR_ID    (0xFC00)
#define FLASH_ADDR_FRU_PCI_CONFIG_SUB_DEVICE_ID    (0xFE00)

extern volatile uint8_t EEPROM_Table[EEPROM_SIZE];
typedef struct FRU_Data_Tag
{
    uint8_t  Offset;
    uint8_t  Size;
    uint32_t Flash_Addr;
} FRU_Data_T;
extern const FRU_Data_T FRU_Data_Attr[EEPROM_FRU_TABLE_TOTAL_COUNT];

void Init_EEPROM_Content(void);
void Read_Produce_Info(uint8_t *des_data, uint32_t src_data_addr, uint8_t size);
void Update_FRU(uint32_t addr, uint8_t *data, uint8_t size);

#endif  /* __Flash_H__ */

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
