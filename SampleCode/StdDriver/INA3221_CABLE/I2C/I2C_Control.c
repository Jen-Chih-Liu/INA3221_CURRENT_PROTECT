/******************************************************************************//**
* @file     I2C_Control.c
* @version  V1.00
* @brief    I2C_Control sample file
*
* @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
******************************************************************************/

/*!<Includes */
#include "device.h"
#include "NuMicro.h"
#include "I2C_Control.h"
#include "Monitor_Control.h"
#include "string.h"
#include "i2c_eeprom_sim.h"
#define fw_version 0x07
/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
volatile uint16_t u16SlvDataLen;
uint8_t volatile au8SlvRxData[LEN_MAX_I2C_DATA];
volatile uint8_t u8RxPtr = 0;
volatile uint8_t u8ReportEEPROMFlag = 0;
uint8_t au8UpdateData[LEN_MAX_I2C_DATA];
volatile uint8_t u8UpdateFRUDataFlag = 0;
volatile uint32_t u32UpdateTargetAddress = 0;
volatile uint8_t u8UpdateTargetOffset = 0;
volatile uint8_t u8UpdateTargetSize = 0;
volatile uint8_t u8UpdateISPFlag = 0;
volatile uint8_t u8UPITFlag = 0;
volatile uint8_t u8UPCDFlag = 0;
volatile uint8_t u8UPDCFlag = 0;
volatile uint8_t u8UPCAFlag = 0;
volatile uint8_t u8UPMFFlag = 0;
volatile uint8_t u8UPLTFlag = 0;
volatile uint8_t u8UPSNFlag = 0;
uint8_t volatile u8RxLen = 0;
uint8_t volatile eeprom_ram[256]={0x0};
uint8_t volatile u8EVEN_INDEX_FLAG = 0;
/* Report Power Information */
extern AppConfig_T g_AppConfig;
extern CalibData_T g_CalibData;
extern uint8_t g_au8SerialNumber[EEPROM_SERIAL_NUMBER_SIZE];
extern uint8_t g_au8LotID[EEPROM_LOT_ID_SIZE];
extern uint8_t g_au8MFGDate[EEPROM_MFG_DATE_SIZE];


const uint8_t CMD_ISP[CMD_LEN_ISP] //jmp to ldrom
    = {0x4E, 0X05, 0x4A, 0x4D, 0x50, 0x4C, 0x44
      };

const uint8_t CMD_UPIT[CMD_LEN_UPDATE_Imbalance_STR] //jmp to ldrom
    = {'U', 'P', 'I', 'T'};

const uint8_t CMD_UPCD[4] = {'U', 'P', 'C', 'D'};
#define CMD_LEN_UPDATE_Countdown_STR 4
#define CMD_LEN_UPDATE_Countdown_Threshold 8

const uint8_t CMD_SWDC[4] = {'S', 'W', 'D', 'C'};
#define CMD_LEN_UPDATE_Debounce_STR 4
#define CMD_LEN_UPDATE_Debounce_Threshold 5

const uint8_t CMD_UPCA[4] = {'U', 'P', 'C', 'A'};
#define CMD_LEN_UPDATE_CALIB_STR 4
#define CMD_LEN_UPDATE_CALIB_DATA 9

const uint8_t CMD_UPMF[4] = {'U', 'P', 'M', 'F'};
const uint8_t CMD_UPLT[4] = {'U', 'P', 'L', 'T'};
const uint8_t CMD_UPSN[4] = {'U', 'P', 'S', 'N'};
#define CMD_LEN_ASSET_STR 4
#define CMD_LEN_ASSET_DATA 20




uint8_t Data_Get_Power_Info[DATA_LEN_GET_POWER_INFO]
=  /* Fixed Header */
{
    0x0F, 0x07, 0x1F, 0x00,
    /* Volatge and Current data, */
    0x00, 0x00,    // Voltage 1, unit in 10 mV
    0x00, 0x00,    // Current 1, unit in 10 mA
    0x00, 0x00,    // Voltage 2, unit in 10 mV
    0x00, 0x00,    // Current 2, unit in 10 mA
    0x00, 0x00,    // Voltage 3, unit in 10 mV
    0x00, 0x00,    // Current 4, unit in 10 mA
};

/*---------------------------------------------------------------------------------------------------------*/
/*  I2C1 IRQ Handler                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void I2C1_IRQHandler(void)
{
    uint32_t u32Status;

    u32Status = I2C_GET_STATUS(I2C1);

    if (I2C_GET_TIMEOUT_FLAG(I2C1))
    {
        /* Clear I2C1 Timeout Flag */
        I2C_ClearTimeoutFlag(I2C1);

        I2C_Close(I2C1);
        I2C1_Init();
    }
    else
    {
        if (s_I2C1HandlerFn != NULL)
            s_I2C1HandlerFn(I2C1, u32Status);
    }
}
#define event_index 0x0f
/*---------------------------------------------------------------------------------------------------------*/
/*  I2C TRx Callback Function                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void I2C_SlaveTRx(I2C_T *i2c, uint32_t u32Status)
{
    uint8_t i;
    uint8_t u8Data;

    if (u32Status == 0x60)                     /* Own SLA+W has been receive; ACK has been return */
    {
        u16SlvDataLen = 0;
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if (u32Status == 0x80)                 /* Previously address with own SLA address
                                                  Data has been received; ACK has been returned*/
    {
        u8Data = (unsigned char) I2C_GET_DATA(i2c);

        if (u16SlvDataLen < LEN_MAX_I2C_DATA)
        {
            au8SlvRxData[u16SlvDataLen++] = u8Data;
        }

        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if (u32Status == 0xA8)                /* Own SLA+R has been receive; ACK has been return */
    {
        /* Request FRU table */
        if (u8ReportEEPROMFlag == 1)
        {
          
                I2C_SET_DATA(i2c, eeprom_ram[u8RxPtr]); 
                u8RxPtr=u8RxPtr+1;
        }
        else
        {
            I2C_SET_DATA(i2c, 0x00);
        }

        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if (u32Status == 0xB8)                /* Data byte in I2CDAT has been transmitted ACK has been received */
    {
        /* Request FRU table */
        if (u8ReportEEPROMFlag == 1)
        {                            
                I2C_SET_DATA(i2c, eeprom_ram[u8RxPtr]); //fw version
                u8RxPtr=u8RxPtr+1;              
        }
        else
        {
            I2C_SET_DATA(i2c, 0x00);
        }

        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if (u32Status == 0xC0)                 /* Data byte or last data in I2CDAT has been transmitted
                                                  Not ACK has been received */
    {
        /* Clear flag */
        u8ReportEEPROMFlag =  0;

        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if (u32Status == 0x88)                 /* Previously addressed with own SLA address; NOT ACK has
                                                  been returned */
    {
        u16SlvDataLen = 0;
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if (u32Status == 0xA0)                 /* A STOP or repeated START has been received while still
                                                  addressed as Slave/Receiver*/
    {

        /* Request FRU table */
        if (u16SlvDataLen == 1) 
        {
            /* Set pointer */
            u8RxPtr = au8SlvRxData[0];
            /* Set flag */
            u8ReportEEPROMFlag = 1;
      
        }
        else if ((u16SlvDataLen == 2)&&(au8SlvRxData[0]==event_index))
        {
            eeprom_ram[event_index]=au8SlvRxData[1];  
            u8EVEN_INDEX_FLAG=1;
         
        }

        //ISP COMMAND
        else if ((u16SlvDataLen == CMD_LEN_ISP) && (memcmp(au8SlvRxData, CMD_ISP, CMD_LEN_ISP) == 0))
        {
            /* Set flag */
            u8UpdateISPFlag = 1;

            /* Clear flag */
            u8ReportEEPROMFlag = 0;
        }


        
        
   // Command to Update Imbalance Threshold
        else if ((u16SlvDataLen == CMD_LEN_UPDATE_Imbalance_Threshold) && (memcmp(au8SlvRxData, CMD_UPIT, CMD_LEN_UPDATE_Imbalance_STR) == 0))
        {
            /* Set flag */
            u8UPITFlag = 1;
            memcpy(&g_AppConfig.u32IMBALANCE_THRESHOLD, &au8SlvRxData[4], sizeof(uint32_t));
            memcpy((void *)&eeprom_ram[EE_OFFSET_IMBALANCE_THRESHOLD], &g_AppConfig.u32IMBALANCE_THRESHOLD, sizeof(uint32_t));
            
            /* Clear flag */
            u8ReportEEPROMFlag = 0;
        }


        // Command to Update Countdown Duration
        else if ((u16SlvDataLen == CMD_LEN_UPDATE_Countdown_Threshold) && (memcmp(au8SlvRxData, CMD_UPCD, CMD_LEN_UPDATE_Countdown_STR) == 0))
        {
            /* Set flag to notify main loop to write to EEPROM */
            u8UPCDFlag = 1;

            memcpy(&g_AppConfig.u32Countdown, &au8SlvRxData[4], sizeof(uint32_t));
            /* Update the value in the I2C RAM map for immediate host readback */
            memcpy((void *)&eeprom_ram[EE_OFFSET_COUNTDOWN], &g_AppConfig.u32Countdown, sizeof(uint32_t));

            u8ReportEEPROMFlag = 0;
        }

        // Command to Update SW Debounce Count
        else if ((u16SlvDataLen == CMD_LEN_UPDATE_Debounce_Threshold) && (memcmp(au8SlvRxData, CMD_SWDC, CMD_LEN_UPDATE_Debounce_STR) == 0))
        {
            /* Set flag to notify main loop to write to EEPROM */
            u8UPDCFlag = 1;

            /* Update the value in the global config struct and I2C RAM map */
            g_AppConfig.u8swdebounce = au8SlvRxData[4];
            eeprom_ram[EE_OFFSET_SW_DEBOUNCE] = g_AppConfig.u8swdebounce;

            u8ReportEEPROMFlag = 0;
        }

        // Command to Update Calibration Data
        else if ((u16SlvDataLen == CMD_LEN_UPDATE_CALIB_DATA) && (memcmp(au8SlvRxData, CMD_UPCA, CMD_LEN_UPDATE_CALIB_STR) == 0))
        {
            uint8_t ch_idx = au8SlvRxData[4];
            if (ch_idx < 6) // Validate channel index (0-5)
            {
                /* Set flag to notify main loop to write to EEPROM */
                u8UPCAFlag = 1;

                /* Update the value in the global calibration data struct */
                g_CalibData.channels[ch_idx].u16Gain = (au8SlvRxData[6] << 8) | au8SlvRxData[5];
                g_CalibData.channels[ch_idx].u16Offset = (au8SlvRxData[8] << 8) | au8SlvRxData[7];

                /* Update the value in the I2C RAM map for immediate host readback */
                uint16_t ram_offset = EE_OFFSET_CALIB_DATA + (ch_idx * 4);
                memcpy((void *)&eeprom_ram[ram_offset], &g_CalibData.channels[ch_idx], 4);
            }
            u8ReportEEPROMFlag = 0;
        }

        // Command to Update Manufacturing Date
        else if ((u16SlvDataLen == CMD_LEN_ASSET_DATA) && (memcmp(au8SlvRxData, CMD_UPMF, CMD_LEN_ASSET_STR) == 0))
        {
            u8UPMFFlag = 1;
            memcpy(g_au8MFGDate, &au8SlvRxData[4], EEPROM_MFG_DATE_SIZE);
            memcpy((void *)&eeprom_ram[I2C_REG_OFFSET_MFG_DATE], g_au8MFGDate, EEPROM_MFG_DATE_SIZE);
            u8ReportEEPROMFlag = 0;
        }

        // Command to Update Lot ID
        else if ((u16SlvDataLen == CMD_LEN_ASSET_DATA) && (memcmp(au8SlvRxData, CMD_UPLT, CMD_LEN_ASSET_STR) == 0))
        {
            u8UPLTFlag = 1;
            memcpy(g_au8LotID, &au8SlvRxData[4], EEPROM_LOT_ID_SIZE);
            memcpy((void *)&eeprom_ram[I2C_REG_OFFSET_LOT_ID], g_au8LotID, EEPROM_LOT_ID_SIZE);
            u8ReportEEPROMFlag = 0;
        }

        // Command to Update Serial Number
        else if ((u16SlvDataLen == CMD_LEN_ASSET_DATA) && (memcmp(au8SlvRxData, CMD_UPSN, CMD_LEN_ASSET_STR) == 0))
        {
            u8UPSNFlag = 1;
            memcpy(g_au8SerialNumber, &au8SlvRxData[4], EEPROM_SERIAL_NUMBER_SIZE);
            memcpy((void *)&eeprom_ram[I2C_REG_OFFSET_SERIAL_NUMBER], g_au8SerialNumber, EEPROM_SERIAL_NUMBER_SIZE);
            u8ReportEEPROMFlag = 0;
        }

        else
        {
            /* Clear flag */
            u8ReportEEPROMFlag = 0;
        }

        u16SlvDataLen = 0;
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else
    {
        if (u32Status == 0x68)             /* Slave receive arbitration lost, clear SI */
        {
            I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
        }
        else if (u32Status == 0xB0)        /* Address transmit arbitration lost, clear SI  */
        {
            I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
        }
        else                               /* Slave bus error, stop I2C and clear SI */
        {
            I2C_SET_CONTROL_REG(i2c, I2C_CTL_STO_SI);
            I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI);
        }
    }
}

void I2C1_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable I2C1 module clock */
    CLK_EnableModuleClock(I2C1_MODULE);

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PA multi-function pins for I2C1 SDA and SCL */
    SYS->GPA_MFPL = (SYS->GPA_MFPL & ~(SYS_GPA_MFPL_PA2MFP_Msk | SYS_GPA_MFPL_PA3MFP_Msk)) | \
                    (SYS_GPA_MFPL_PA2MFP_I2C1_SDA | SYS_GPA_MFPL_PA3MFP_I2C1_SCL);
    PA->SMTEN |= GPIO_SMTEN_SMTEN2_Msk | GPIO_SMTEN_SMTEN3_Msk;
    /* Open I2C module and set bus clock */
    I2C_Open(I2C1, SPEED_I2C_BUS);

    /* Set I2C Slave Addresses */
    I2C_SetSlaveAddr(I2C1, 0, ADDRESS_I2C_SLAVE_7BIT, I2C_GCMODE_DISABLE);

    /* Enable I2C interrupt */
    I2C_EnableInt(I2C1);
    NVIC_EnableIRQ(I2C1_IRQn);
    NVIC_SetPriority(I2C1_IRQn, INT_PRIORITY_HIGH);

    /* Add hold time */
    //    I2C1->TMCTL = 0x02 << I2C_TMCTL_HTCTL_Pos;

    /* Lock protected registers */
    SYS_LockReg();

    /* I2C function to Slave receive/transmit data */
    s_I2C1HandlerFn = I2C_SlaveTRx;

    /* I2C enter no address SLV mode */
    I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI_AA);
}
