/******************************************************************************//**
 * @file     Flash.c
 * @version  V1.00
 * @brief    Flash sample file
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

/*!<Includes */
#include <stdio.h>
#include "device.h"
#include "NuMicro.h"
#include "Flash.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
volatile uint8_t EEPROM_Table[EEPROM_SIZE];
const FRU_Data_T FRU_Data_Attr[EEPROM_FRU_TABLE_TOTAL_COUNT] =
{
{EEPROM_OFFSET_FRU_OEM_INFORMATION,          EEPROM_SIZE_FRU_OEM_INFORMATION,          FLASH_ADDR_FRU_OEM_INFORMATION},
{EEPROM_OFFSET_FRU_BOARD_PART_NUMBER,        EEPROM_SIZE_FRU_BOARD_PART_NUMBER,        FLASH_ADDR_FRU_BOARD_PART_NUMBER},
{EEPROM_OFFSET_FRU_SERIAL_NUMBER,            EEPROM_SIZE_FRU_SERIAL_NUMBER,            FLASH_ADDR_FRU_SERIAL_NUMBER},
{EEPROM_OFFSET_FRU_MARKETING_NAME,           EEPROM_SIZE_FRU_MARKETING_NAME,           FLASH_ADDR_FRU_MARKETING_NAME},
{EEPROM_OFFSET_FRU_BUILD_DATE,               EEPROM_SIZE_FRU_BUILD_DATE,               FLASH_ADDR_FRU_BUILD_DATE},
{EEPROM_OFFSET_FRU_HW_VERSION,               EEPROM_SIZE_FRU_HW_VERSION,               FLASH_ADDR_FRU_HW_VERSION},
{EEPROM_OFFSET_FRU_FW_VERSION,               EEPROM_SIZE_FRU_FW_VERSION,               FLASH_ADDR_FRU_FW_VERSION},
{EEPROM_OFFSET_FRU_PCI_CONFIG_VENDOR_ID,     EEPROM_SIZE_FRU_PCI_CONFIG_VENDOR_ID,     FLASH_ADDR_FRU_PCI_CONFIG_VENDOR_ID},
{EEPROM_OFFSET_FRU_PCI_CONFIG_DEVICE_ID,     EEPROM_SIZE_FRU_PCI_CONFIG_DEVICE_ID,     FLASH_ADDR_FRU_PCI_CONFIG_DEVICE_ID},
{EEPROM_OFFSET_FRU_PCI_CONFIG_SUB_VENDOR_ID, EEPROM_SIZE_FRU_PCI_CONFIG_SUB_VENDOR_ID, FLASH_ADDR_FRU_PCI_CONFIG_SUB_VENDOR_ID},
{EEPROM_OFFSET_FRU_PCI_CONFIG_SUB_DEVICE_ID, EEPROM_SIZE_FRU_PCI_CONFIG_SUB_DEVICE_ID, FLASH_ADDR_FRU_PCI_CONFIG_SUB_DEVICE_ID},
};

void Init_EEPROM_Content(void)
{
    uint8_t i;

    /* Get each FRU table attribute to EEPROM */
    for(i = 0; i < EEPROM_FRU_TABLE_TOTAL_COUNT; i++)
    {
        Read_Produce_Info((uint8_t *)(EEPROM_Table + FRU_Data_Attr[i].Offset),
                          FRU_Data_Attr[i].Flash_Addr,
                          FRU_Data_Attr[i].Size
                         );
    }
}

void Read_Produce_Info(uint8_t *des_data, uint32_t src_data_addr, uint8_t size)
{
    uint8_t i, j;
    uint32_t temp;

    /* Unlock write-protected registers */
    SYS_UnlockReg();

    /* Enable FMC ISP function */
    FMC_Open();

    for(i = 0; i < size;)
    {
        /* Read data from Flash */
        temp = FMC_Read(src_data_addr + i);
        for(j = 0; j < 4; j++)
        {
            des_data[i++] = (uint8_t)((temp & (0xFF << (j*8))) >> (j*8));

            /* Finish read */
            if(i == size)
            {
                /* Lock protected registers */
                SYS_LockReg();

                return;
            }
        }
    }

    /* Lock protected registers */
    SYS_LockReg();
}

void Update_FRU(uint32_t addr, uint8_t *data, uint8_t size)
{
    uint32_t i, j;
    uint32_t temp;

    /* Unlock write-protected registers */
    SYS_UnlockReg();

    /* Enable FMC ISP function */
    FMC_Open();

    /* Enable update APROM */
    FMC_ENABLE_AP_UPDATE();

    /* Erase target page */
    FMC_Erase(addr);

    /* Write new data */
    for(i = 0; i < size;)
    {
        temp = 0;

        for(j = 0; j < 4; j++)
        {
            temp |= (data[i++] << (j*8));

            if(i == size)
            {
                break;
            }
        }

        /* Write to the Flash */
        FMC_Write(addr, temp);
        /* Add target address */
        addr += 4;
    }

    /* Disable update APROM */
    FMC_DISABLE_AP_UPDATE();

    /* Lock protected registers */
    SYS_LockReg();
}
