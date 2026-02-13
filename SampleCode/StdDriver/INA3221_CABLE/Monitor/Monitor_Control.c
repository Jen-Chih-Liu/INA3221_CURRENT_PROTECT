/******************************************************************************//**
* @file     Monitor_Control.c
* @version  V1.00
* @brief    Monitor_Control sample file
*
* @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
******************************************************************************/

/*!<Includes */
#include "device.h"
#include "NuMicro.h"
#include "I2C_Control.h"
#include "Monitor_Control.h"

extern uint8_t volatile eeprom_ram[256];
#define I2C_REG_MONITOR_DATA_OFFSET 0x30

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
/* Monitor Setting 0 */
const uint8_t CH_Enalbe_Monitor_0[MONITOR_MAX_CHANNEL] = {USE_MONITOR_0_CH1, USE_MONITOR_0_CH2, USE_MONITOR_0_CH3};
volatile uint8_t g_u8TargetCH_0 = 0;
volatile uint8_t g_u8GetEndFlag_0 = 1;
volatile uint8_t g_u8GetErrorFlag_0 = 0;
volatile uint8_t g_u8Status_0 = MONITOR_IDLE;
volatile uint16_t g_u16TempData_0[REGISTER_DATA_COUNT];
Monitor_Data_T au8MonitorData_0[MONITOR_MAX_CHANNEL];
const uint16_t Shunt_Ratio_Molecular_0[MONITOR_MAX_CHANNEL] = {SHUNT_RATIO_MOLECULAR_0_CH0, SHUNT_RATIO_MOLECULAR_0_CH1, SHUNT_RATIO_MOLECULAR_0_CH2};
const uint16_t Shunt_Ratio_Denomination_0[MONITOR_MAX_CHANNEL] = {SHUNT_RATIO_DENOMINATOR_0_CH0, SHUNT_RATIO_DENOMINATOR_0_CH1, SHUNT_RATIO_DENOMINATOR_0_CH2};
/* Shunt Ratio for Monitor 1 */
const uint16_t Shunt_Ratio_Molecular_1[MONITOR_MAX_CHANNEL] = {SHUNT_RATIO_MOLECULAR_0_CH0, SHUNT_RATIO_MOLECULAR_0_CH1, SHUNT_RATIO_MOLECULAR_0_CH2};
const uint16_t Shunt_Ratio_Denomination_1[MONITOR_MAX_CHANNEL] = {SHUNT_RATIO_DENOMINATOR_0_CH0, SHUNT_RATIO_DENOMINATOR_0_CH1, SHUNT_RATIO_DENOMINATOR_0_CH2};
/* Monitor Setting 1 */
const uint8_t CH_Enalbe_Monitor_1[MONITOR_MAX_CHANNEL] = {USE_MONITOR_1_CH1, USE_MONITOR_1_CH2, USE_MONITOR_1_CH3};
volatile uint8_t g_u8TargetCH_1 = 0;
volatile uint8_t g_u8GetEndFlag_1 = 1;
volatile uint8_t g_u8GetErrorFlag_1 = 0;
volatile uint16_t g_u16TempData_1[REGISTER_DATA_COUNT];
volatile uint8_t g_u8Status_1 = MONITOR_IDLE;
Monitor_Data_T au8MonitorData_1[MONITOR_MAX_CHANNEL];

uint32_t TimeCounterMonitorUpdate;
volatile uint8_t u8MonitorFlag;

#if (INA3221_MOCK_TEST == 1)
/* Mock data generation for testing without hardware */
void Mock_Update_Monitor_Data_0(void)
{
    static uint16_t mock_raw_data = 1000; // A changing value to simulate real data
    uint16_t data;
    uint8_t ch;

    for (ch = 0; ch < MONITOR_MAX_CHANNEL; ch++)
    {
        if (CH_Enalbe_Monitor_0[ch])
        {
            /* Mock Current Calculation */
            data = ((mock_raw_data * 40 * Shunt_Ratio_Molecular_0[ch]) / (Shunt_Ratio_Denomination_0[ch] * 1000) + 5) / 10;
            uint16_t u16CurrentOffset = I2C_REG_MONITOR_DATA_OFFSET + (ch * 4);
            eeprom_ram[u16CurrentOffset + 0] = (uint8_t)(data);
            eeprom_ram[u16CurrentOffset + 1] = (uint8_t)(data >> 8);

            /* Mock Voltage Calculation */
            data = ((mock_raw_data / 2) * 8 + 5) / 10; // Use a different value for voltage
            uint16_t u16VoltageOffset = I2C_REG_MONITOR_DATA_OFFSET + (ch * 4) + 2;
            eeprom_ram[u16VoltageOffset + 0] = (uint8_t)(data);
            eeprom_ram[u16VoltageOffset + 1] = (uint8_t)(data >> 8);
        }
    }
    mock_raw_data += 50; // Increment data for next call
    if (mock_raw_data > 4000) mock_raw_data = 1000;
}

void Mock_Update_Monitor_Data_1(void)
{
    // Use a different static variable for the second monitor to have different data
    static uint16_t mock_raw_data_1 = 2000; // A changing value to simulate real data
    uint16_t data;
    uint8_t ch;

    for (ch = 0; ch < MONITOR_MAX_CHANNEL; ch++)
    {
        if (CH_Enalbe_Monitor_1[ch])
        {
            /* Mock Current Calculation using Monitor 1's shunt ratios */
            data = ((mock_raw_data_1 * 40 * Shunt_Ratio_Molecular_1[ch]) / (Shunt_Ratio_Denomination_1[ch] * 1000) + 5) / 10;
            /* The offset for the second monitor's channels (3, 4, 5) */
            uint16_t u16CurrentOffset = I2C_REG_MONITOR_DATA_OFFSET + ((ch + MONITOR_MAX_CHANNEL) * 4);
            eeprom_ram[u16CurrentOffset + 0] = (uint8_t)(data);
            eeprom_ram[u16CurrentOffset + 1] = (uint8_t)(data >> 8);

            /* Mock Voltage Calculation */
            data = ((mock_raw_data_1 / 2) * 8 + 5) / 10; // Use a different value for voltage
            uint16_t u16VoltageOffset = I2C_REG_MONITOR_DATA_OFFSET + ((ch + MONITOR_MAX_CHANNEL) * 4) + 2;
            eeprom_ram[u16VoltageOffset + 0] = (uint8_t)(data);
            eeprom_ram[u16VoltageOffset + 1] = (uint8_t)(data >> 8);
        }
    }
    mock_raw_data_1 += 75; // Increment data for next call with a different step
    if (mock_raw_data_1 > 5000) mock_raw_data_1 = 2000;
}
#endif
/*---------------------------------------------------------------------------------------------------------*/
/*  I2C0 IRQ Handler                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void I2C0_Error_Hanlder(I2C_T *i2c)
{
    uint8_t i;

    /* Disable Timeout Detect */
    I2C_DisableTimeout(i2c);

    /* Clear data */
    for (i = 0; i < MONITOR_MAX_CHANNEL; i++)
    {
        au8MonitorData_0[i].Current.Sign = 0;
        au8MonitorData_0[i].Current.Value = 0;
        au8MonitorData_0[i].Voltage.Sign = 0;
        au8MonitorData_0[i].Voltage.Value = 0;
    }

    g_u8GetEndFlag_0 = 1;
    g_u8GetErrorFlag_0 = 1;
}

void I2C0_IRQHandler(void)
{
    uint32_t u32Status;

    u32Status = I2C_GET_STATUS(I2C0);

    if (I2C_GET_TIMEOUT_FLAG(I2C0))
    {
        /* Clear I2C0 Timeout Flag */
        I2C_ClearTimeoutFlag(I2C0);

        if (g_u8GetEndFlag_0 != 1)
        {
            I2C0_Error_Hanlder(I2C0);

            I2C_Close(I2C0);
            I2C0_Init();
        }
    }
    else
    {
        if (s_I2C0HandlerFn != NULL)
        {
            s_I2C0HandlerFn(I2C0, u32Status);
        }
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/*  I2C TRx Callback Function                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void I2C0_MasterTRx(I2C_T *i2c, uint32_t u32Status)
{
    uint8_t sign;
    uint16_t data;

    /* Write Index and Data */
    if (u32Status == 0x08)                      /* START has been transmitted */
    {
        I2C_SET_DATA(i2c, ADDRESS_MONIOR_0_8BIT_W);     /* Write SLA+W to Register I2CDAT */
        g_u8Status_0++;
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI);
    }
    else if (u32Status == 0x18)                 /* SLA+W has been transmitted and ACK has been received */
    {
        if (g_u8Status_0 == MONITOR_WRITE_I_REGISTER)
        {
            I2C_SET_DATA(i2c, REGISTER_START_ADDRESS + g_u8TargetCH_0 * 2);
        }
        else if (g_u8Status_0 == MONITOR_WRITE_V_REGISTER)
        {
            I2C_SET_DATA(i2c, REGISTER_START_ADDRESS + g_u8TargetCH_0 * 2 + 1);
        }

        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI);
    }
    else if (u32Status == 0x20)                 /* SLA+W has been transmitted and NACK has been received */
    {
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_STO_SI);

        I2C0_Error_Hanlder(i2c);
    }
    else if (u32Status == 0x28)                 /* DATA has been transmitted and ACK has been received */
    {
        /* Change status */
        g_u8Status_0++;

        /* Repeat start */
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_STA_SI);
    }
    else if (u32Status == 0x30)                 /* DATA has been transmitted and NACK has been received */
    {
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_STO_SI);

        I2C0_Error_Hanlder(i2c);
    }
    else if (u32Status == 0x10)                 /* Repeat START has been transmitted and prepare SLA+R */
    {
        if ((g_u8Status_0 == MONITOR_START_I_WRITE) ||
                (g_u8Status_0 == MONITOR_START_V_WRITE)
           )
        {
            I2C_SET_DATA(i2c, ADDRESS_MONIOR_0_8BIT_W);     /* Write SLA+W to Register I2CDAT */
        }
        else if ((g_u8Status_0 == MONITOR_START_I_READ) ||
                 (g_u8Status_0 == MONITOR_START_V_READ)
                )
        {
            I2C_SET_DATA(i2c, ADDRESS_MONIOR_0_8BIT_R);     /* Write SLA+R to Register I2CDAT */
        }

        g_u8Status_0++;
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI);
    }
    else if (u32Status == 0x40)                /* SLA+R has been transmitted and ACK has been received */
    {
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if (u32Status == 0x48)                 /* SLA+R has been transmitted and NACK has been received */
    {
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_STO_SI);

        I2C0_Error_Hanlder(i2c);
    }
    else if (u32Status == 0x50)                 /* DATA has been received and ACK has been returned */
    {
        g_u16TempData_0[0] = (unsigned char) I2C_GET_DATA(i2c);

        g_u8Status_0++;
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI);
    }
    else if (u32Status == 0x58)                 /* DATA has been received and NACK has been returned */
    {
        g_u16TempData_0[1] = (unsigned char) I2C_GET_DATA(i2c);

        if (g_u8Status_0 == MONITOR_READ_I_SECOND_DATA)
        {
            sign = g_u16TempData_0[0] & BIT7;
            data = (((g_u16TempData_0[0] << 8) | g_u16TempData_0[1]) & 0x7FF8) >> 3;

//            au8MonitorData_0[g_u8TargetCH_0].Current.Sign = (sign)?1:0;
//            au8MonitorData_0[g_u8TargetCH_0].Current.Value = (data * 40 * SHUNT_RATIO_MOLECULAR_0) / (SHUNT_RATIO_DENOMINATOR_0 * 1000);

            /* Update report data to eeprom_ram */
            data = ((data * 40 * Shunt_Ratio_Molecular_0[g_u8TargetCH_0]) / (Shunt_Ratio_Denomination_0[g_u8TargetCH_0] * 1000) + 5) / 10;
            uint16_t u16CurrentOffset = I2C_REG_MONITOR_DATA_OFFSET + (g_u8TargetCH_0 * 4);
            eeprom_ram[u16CurrentOffset + 0] = (uint8_t)(data);
            eeprom_ram[u16CurrentOffset + 1] = (uint8_t)(data >> 8);

            /* Change status */
            g_u8Status_0++;

            /* Repeat start */
            I2C_SET_CONTROL_REG(i2c, I2C_CTL_STA_SI);
        }
        else if (g_u8Status_0 == MONITOR_READ_V_SECOND_DATA)
        {
            sign = g_u16TempData_0[0] & BIT7;
            data = (((g_u16TempData_0[0] << 8) | g_u16TempData_0[1]) & 0x7FF8) >> 3;

            //            au8MonitorData_0[g_u8TargetCH_0].Voltage.Sign = (sign)?1:0;
            //            au8MonitorData_0[g_u8TargetCH_0].Voltage.Value = data * 8;

            /* Update report data to eeprom_ram */
            data = (data * 8 + 5) / 10;
            uint16_t u16VoltageOffset = I2C_REG_MONITOR_DATA_OFFSET + (g_u8TargetCH_0 * 4) + 2;
            eeprom_ram[u16VoltageOffset + 0] = (uint8_t)(data);
            eeprom_ram[u16VoltageOffset + 1] = (uint8_t)(data >> 8);

            /* Set next target channel */
            g_u8TargetCH_0++;

            while (g_u8TargetCH_0 < MONITOR_MAX_CHANNEL)
            {
                if (CH_Enalbe_Monitor_0[g_u8TargetCH_0] == 0)
                {
                    g_u8TargetCH_0++;
                }
                else
                    break;
            }

            if (g_u8TargetCH_0 == MONITOR_MAX_CHANNEL)
            {
                /* End operation */
                I2C_SET_CONTROL_REG(i2c, I2C_CTL_STO_SI);

                /* Disable Timeout Detect */
                I2C_DisableTimeout(i2c);

                g_u8GetEndFlag_0 = 1;
            }
            else
            {
                g_u8Status_0 = MONITOR_START_I_WRITE;

                /* Repeat start */
                I2C_SET_CONTROL_REG(i2c, I2C_CTL_STA_SI);
            }
        }
    }
    else                                        /* Unexpect status */
    {
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_STO_SI);

        I2C0_Error_Hanlder(i2c);
    }
}

void I2C0_Init(void)
{
#if (INA3221_MOCK_TEST == 0)
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable I2C0 module clock */
    CLK_EnableModuleClock(I2C0_MODULE);

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PC multi-function pins for I2C0 SDA and SCL */
    SYS->GPC_MFPL = (SYS->GPC_MFPL & ~(SYS_GPC_MFPL_PC0MFP_Msk | SYS_GPC_MFPL_PC1MFP_Msk)) | \
                    (SYS_GPC_MFPL_PC0MFP_I2C0_SDA | SYS_GPC_MFPL_PC1MFP_I2C0_SCL);
    PC->SMTEN |= GPIO_SMTEN_SMTEN0_Msk | GPIO_SMTEN_SMTEN1_Msk;
    /* Open I2C module and set bus clock */
    I2C_Open(I2C0, SPEED_MONIOR_0_BUS);

    //initial ina3221 sample 16
    uint8_t data[2];
    data[0] = (ina3221_config>>8)&0xff;//0X7A;
    data[1] = (ina3221_config)&0xff;//0X47;

    //wait i2c stop finish.
    I2C_WriteMultiBytes(I2C0, ADDRESS_MONIOR_0_7BIT, data, 2);
    unsigned int u32TimeOutCount = SystemCoreClock;

    while ((I2C0)->CTL0 & I2C_CTL0_STO_Msk)
    {
        u32TimeOutCount--;

        if (u32TimeOutCount == 0)
        {
            break;
        }
    }



    /* Enable I2C interrupt */
    I2C_EnableInt(I2C0);
    NVIC_EnableIRQ(I2C0_IRQn);
    NVIC_SetPriority(I2C0_IRQn, INT_PRIORITY_NORMAL);

    /* Add hold time */
    //    I2C0->TMCTL = 0x02 << I2C_TMCTL_HTCTL_Pos;

    /* Lock protected registers */
    SYS_LockReg();

    /* I2C function to Master receive/transmit data */
    s_I2C0HandlerFn = I2C0_MasterTRx;
#endif
}

/*---------------------------------------------------------------------------------------------------------*/
/*  USCI_I2C IRQ Handler                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
void UI2C1_Error_Hanlder(UI2C_T *ui2c)
{
    uint8_t i;

    /* Disable Timeout Detect */
    UI2C_DisableTimeout(ui2c);
    UI2C_DISABLE_PROT_INT(ui2c, UI2C_PROTIEN_TOIEN_Msk);

    /* Clear data */
    for (i = 0; i < MONITOR_MAX_CHANNEL; i++)
    {
        au8MonitorData_1[i].Current.Sign = 0;
        au8MonitorData_1[i].Current.Value = 0;
        au8MonitorData_1[i].Voltage.Sign = 0;
        au8MonitorData_1[i].Voltage.Value = 0;
    }

    g_u8GetEndFlag_1 = 1;
    g_u8GetErrorFlag_1 = 1;
}

void USCI1_IRQHandler(void)
{
    uint32_t u32Status;

    u32Status = (UI2C1->PROTSTS);

    if (UI2C_GET_TIMEOUT_FLAG(UI2C1))
    {
        /* Clear UI2C1 Timeout Flag */
        UI2C_ClearTimeoutFlag(UI2C1);

        if (g_u8GetEndFlag_1 != 1)
        {
            UI2C1_Error_Hanlder(UI2C1);

            UI2C_Close(UI2C1);
            UI2C1_Init();
        }
    }
    else
    {
        if (s_UI2C1HandlerFn != NULL)
        {
            s_UI2C1HandlerFn(UI2C1, u32Status);
        }
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/*  USCI_I2C TRx Callback Function                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void UI2C1_MasterTRx(UI2C_T *ui2c, uint32_t u32Status)
{
    uint8_t sign;
    uint16_t data;

    if ((u32Status & UI2C_PROTSTS_STARIF_Msk) == UI2C_PROTSTS_STARIF_Msk)
    {
        UI2C_CLR_PROT_INT_FLAG(ui2c, UI2C_PROTSTS_STARIF_Msk);                       /* Clear START INT Flag */

        if (g_u8Status_1 == MONITOR_START_I_WRITE)
        {
            UI2C_SET_DATA(ui2c, ADDRESS_MONIOR_1_8BIT_W);                            /* Write SLA+W to Register TXDAT */
        }
        else if (g_u8Status_1 == MONITOR_START_I_READ)
        {
            UI2C_SET_DATA(ui2c, ADDRESS_MONIOR_1_8BIT_R);                            /* Write SLA+R to Register TXDAT */
        }
        else if (g_u8Status_1 == MONITOR_START_V_WRITE)
        {
            UI2C_SET_DATA(ui2c, ADDRESS_MONIOR_1_8BIT_W);                            /* Write SLA+W to Register TXDAT */
        }
        else if (g_u8Status_1 == MONITOR_START_V_READ)
        {
            UI2C_SET_DATA(ui2c, ADDRESS_MONIOR_1_8BIT_R);                            /* Write SLA+R to Register TXDAT */
        }

        UI2C_SET_CONTROL_REG(ui2c, UI2C_CTL_PTRG);
    }
    else if ((u32Status & UI2C_PROTSTS_ACKIF_Msk) == UI2C_PROTSTS_ACKIF_Msk)
    {
        UI2C_CLR_PROT_INT_FLAG(ui2c, UI2C_PROTSTS_ACKIF_Msk);                        /* Clear ACK INT Flag */

        if (g_u8Status_1 == MONITOR_START_I_WRITE)                                   /* SLA+W has been transmitted and write ADDRESS to Register TXDAT */
        {
            UI2C_SET_DATA(ui2c, REGISTER_START_ADDRESS + g_u8TargetCH_1 * 2);

            g_u8Status_1++;

            UI2C_SET_CONTROL_REG(ui2c, UI2C_CTL_PTRG);
        }
        else if (g_u8Status_1 == MONITOR_WRITE_I_REGISTER)
        {
            g_u8Status_1++;

            UI2C_SET_CONTROL_REG(ui2c, (UI2C_CTL_PTRG | UI2C_CTL_STA));              /* Send repeat START signal */
        }
        else if (g_u8Status_1 == MONITOR_START_I_READ)                               /* SLA+R has been transmitted and ACK has been received */
        {
            g_u8Status_1++;

            UI2C_SET_CONTROL_REG(ui2c, UI2C_CTL_PTRG | UI2C_CTL_AA);
        }
        else if (g_u8Status_1 == MONITOR_READ_I_FIRST_DATA)                          /* DATA has been received and ACK has been returned */
        {
            g_u16TempData_1[0] = (unsigned char) UI2C_GET_DATA(ui2c);

            g_u8Status_1++;

            UI2C_SET_CONTROL_REG(ui2c, UI2C_CTL_PTRG);
        }
        else if (g_u8Status_1 == MONITOR_START_V_WRITE)                              /* SLA+W has been transmitted and write ADDRESS to Register TXDAT */
        {
            UI2C_SET_DATA(ui2c, REGISTER_START_ADDRESS + g_u8TargetCH_1 * 2 + 1);

            g_u8Status_1++;

            UI2C_SET_CONTROL_REG(ui2c, UI2C_CTL_PTRG);
        }
        else if (g_u8Status_1 == MONITOR_WRITE_V_REGISTER)
        {
            g_u8Status_1++;

            UI2C_SET_CONTROL_REG(ui2c, (UI2C_CTL_PTRG | UI2C_CTL_STA));              /* Send repeat START signal */
        }
        else if (g_u8Status_1 == MONITOR_START_V_READ)                               /* SLA+R has been transmitted and ACK has been received */
        {
            g_u8Status_1++;

            UI2C_SET_CONTROL_REG(ui2c, UI2C_CTL_PTRG | UI2C_CTL_AA);
        }
        else if (g_u8Status_1 == MONITOR_READ_V_FIRST_DATA)                          /* DATA has been received and ACK has been returned */
        {
            g_u16TempData_1[0] = (unsigned char) UI2C_GET_DATA(ui2c);

            g_u8Status_1++;

            UI2C_SET_CONTROL_REG(ui2c, UI2C_CTL_PTRG);
        }
    }
    else if ((u32Status & UI2C_PROTSTS_NACKIF_Msk) == UI2C_PROTSTS_NACKIF_Msk)
    {
        UI2C_CLR_PROT_INT_FLAG(ui2c, UI2C_PROTSTS_NACKIF_Msk);                      /* Clear NACK INT Flag */

        if (g_u8Status_1 == MONITOR_READ_I_SECOND_DATA)                              /* DATA has been received and NACK has been returned */
        {
            g_u16TempData_1[1] = (unsigned char) UI2C_GET_DATA(ui2c);

            sign = g_u16TempData_1[0] & BIT7;
            data = (((g_u16TempData_1[0] << 8) | g_u16TempData_1[1]) & 0x7FF8) >> 3;

            //au8MonitorData_1[g_u8TargetCH_1].Current.Sign = (sign) ? 1 : 0;
            //au8MonitorData_1[g_u8TargetCH_1].Current.Value = (data * 40 * Shunt_Ratio_Molecular_1[g_u8TargetCH_1]) / (Shunt_Ratio_Denomination_1[g_u8TargetCH_1] * 1000);

            /* Update report data to eeprom_ram for channels 4-6 */
            data = ((data * 40 * Shunt_Ratio_Molecular_1[g_u8TargetCH_1]) / (Shunt_Ratio_Denomination_1[g_u8TargetCH_1] * 1000) + 5) / 10;
            uint16_t u16CurrentOffset = I2C_REG_MONITOR_DATA_OFFSET + ((g_u8TargetCH_1 + 3) * 4);
            eeprom_ram[u16CurrentOffset + 0] = (uint8_t)(data);
            eeprom_ram[u16CurrentOffset + 1] = (uint8_t)(data >> 8);
            /* Change status */
            g_u8Status_1++;

            /* Repeat start */
            UI2C_SET_CONTROL_REG(ui2c, (UI2C_CTL_PTRG | UI2C_CTL_STA));              /* Send repeat START signal */
        }
        else if (g_u8Status_1 == MONITOR_READ_V_SECOND_DATA)                         /* DATA has been received and NACK has been returned */
        {
            g_u16TempData_1[1] = (unsigned char) UI2C_GET_DATA(ui2c);

            sign = g_u16TempData_1[0] & BIT7;
            data = (((g_u16TempData_1[0] << 8) | g_u16TempData_1[1]) & 0x7FF8) >> 3;

            //au8MonitorData_1[g_u8TargetCH_1].Voltage.Sign = (sign) ? 1 : 0;
            //au8MonitorData_1[g_u8TargetCH_1].Voltage.Value = data * 8;            

            /* Update report data to eeprom_ram for channels 4-6 */
            data = (data * 8 + 5) / 10;
            uint16_t u16VoltageOffset = I2C_REG_MONITOR_DATA_OFFSET + ((g_u8TargetCH_1 + 3) * 4) + 2;
            eeprom_ram[u16VoltageOffset + 0] = (uint8_t)(data);
            eeprom_ram[u16VoltageOffset + 1] = (uint8_t)(data >> 8);
            /* Set next target channel */
            g_u8TargetCH_1++;

            while (g_u8TargetCH_1 < MONITOR_MAX_CHANNEL)
            {
                if (CH_Enalbe_Monitor_1[g_u8TargetCH_1] == 0)
                {
                    g_u8TargetCH_1++;
                }
                else
                    break;
            }

            if (g_u8TargetCH_1 == MONITOR_MAX_CHANNEL)
            {
                /* End operation */
                UI2C_SET_CONTROL_REG(ui2c, (UI2C_CTL_PTRG | UI2C_CTL_STO));         /* Send STOP signal */
            }
            else
            {
                g_u8Status_1 = MONITOR_START_I_WRITE;

                /* Repeat start */
                UI2C_SET_CONTROL_REG(ui2c, (UI2C_CTL_PTRG | UI2C_CTL_STA));          /* Send repeat START signal */
            }
        }
        else
        {
            UI2C_SET_CONTROL_REG(ui2c, (UI2C_CTL_PTRG | UI2C_CTL_STO));

            UI2C1_Error_Hanlder(ui2c);
        }
    }
    else if ((u32Status & UI2C_PROTSTS_STORIF_Msk) == UI2C_PROTSTS_STORIF_Msk)
    {
        UI2C_CLR_PROT_INT_FLAG(ui2c, UI2C_PROTSTS_STORIF_Msk);                       /* Clear STOP INT Flag */

        UI2C_SET_CONTROL_REG(ui2c, UI2C_CTL_PTRG);

        /* Disable Timeout Detect */
        UI2C_DisableTimeout(ui2c);
        UI2C_DISABLE_PROT_INT(ui2c, UI2C_PROTIEN_TOIEN_Msk);

        g_u8GetEndFlag_1 = 1;
    }
}

void UI2C1_Init(void)
{
#if (INA3221_MOCK_TEST == 0)
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable USCI1 module clock */
    CLK_EnableModuleClock(USCI1_MODULE);

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PB multi-function pins for UI2C1 SDA and SCL */
    SYS->GPB_MFPL = (SYS->GPB_MFPL & ~(SYS_GPB_MFPL_PB2MFP_Msk | SYS_GPB_MFPL_PB1MFP_Msk)) | \
                    (SYS_GPB_MFPL_PB2MFP_USCI1_DAT0 | SYS_GPB_MFPL_PB1MFP_USCI1_CLK);
    PB->SMTEN |= GPIO_SMTEN_SMTEN2_Msk | GPIO_SMTEN_SMTEN1_Msk;
    /* Open UI2C1 module and set bus clock */
    UI2C_Open(UI2C1, SPEED_MONIOR_0_BUS);


    //initial ina3221 sample 16
    uint8_t data[2];
    data[0] = (ina3221_config>>8)&0xff;//0X7A;
    data[1] = (ina3221_config)&0xff;//0X47;

    //wait i2c stop finish.
    UI2C_WriteMultiBytes(UI2C1, ADDRESS_MONIOR_0_7BIT, data, 2);

    /* Enable UI2C1 interrupt */
    UI2C_ENABLE_PROT_INT(UI2C1, (UI2C_PROTIEN_ACKIEN_Msk | UI2C_PROTIEN_NACKIEN_Msk | UI2C_PROTIEN_STORIEN_Msk | UI2C_PROTIEN_STARIEN_Msk));
    NVIC_EnableIRQ(USCI1_IRQn);
    NVIC_SetPriority(USCI1_IRQn, INT_PRIORITY_NORMAL);

    /* Lock protected registers */
    SYS_LockReg();

    /* UI2C function to Master receive/transmit data */
    s_UI2C1HandlerFn = UI2C1_MasterTRx;
#endif
}

void Read_Monitor_Data_0(void)
{
#if (INA3221_MOCK_TEST == 1)
    /* In mock mode, directly update eeprom_ram with simulated data */
    Mock_Update_Monitor_Data_0();
#else
    /* In real mode, perform I2C communication */
    if (g_u8GetEndFlag_0 == 1) // Only start a new transaction if the previous one is finished
    {
        /* Initial Status */
        g_u8TargetCH_0 = 0;
        g_u8GetEndFlag_0 = 0;
        g_u16TempData_0[0] = 0;
        g_u16TempData_0[1] = 0;

        /* Set target channel */
        while (g_u8TargetCH_0 < MONITOR_MAX_CHANNEL)
        {
            if (CH_Enalbe_Monitor_0[g_u8TargetCH_0] == 0)
            {
                g_u8TargetCH_0++;
            }
            else
                break;
        }

        if (g_u8TargetCH_0 == MONITOR_MAX_CHANNEL)
        {
            g_u8GetEndFlag_0 = 1;
        }
        else
        {
            /* Enable Timeout Detect */
            I2C_EnableTimeout(I2C0, 1);

            /* Set status */
            g_u8Status_0 = MONITOR_START_I_WRITE;

            /* I2C0 as master sends START signal */
            I2C_SET_CONTROL_REG(I2C0, I2C_CTL_STA);
        }
    }
#endif
}

void Read_Monitor_Data_1(void)
{
#if (INA3221_MOCK_TEST == 1)
    /* In mock mode, directly update eeprom_ram with simulated data */
    Mock_Update_Monitor_Data_1(); // Call the mock function for the second monitor
#else
    /* In real mode, perform I2C communication */
    if (g_u8GetEndFlag_1 == 1) // Only start a new transaction if the previous one is finished
    {
        /* Initial Status */
        g_u8TargetCH_1 = 0;
        g_u8GetEndFlag_1 = 0;
        g_u16TempData_1[0] = 0;
        g_u16TempData_1[1] = 0;

        /* Set target channel */
        while (g_u8TargetCH_1 < MONITOR_MAX_CHANNEL)
        {
            if (CH_Enalbe_Monitor_1[g_u8TargetCH_1] == 0)
            {
                g_u8TargetCH_1++;
            }
            else
                break;
        }

        if (g_u8TargetCH_1 == MONITOR_MAX_CHANNEL)
        {
            g_u8GetEndFlag_1 = 1;
        }
        else
        {
            /* Enable Timeout Detect */
            UI2C_EnableTimeout(UI2C1, 0x3FF);
            UI2C_CLR_PROT_INT_FLAG(UI2C1, UI2C_PROTSTS_TOIF_Msk);
            UI2C_ENABLE_PROT_INT(UI2C1, UI2C_PROTIEN_TOIEN_Msk);

            /* Set status */
            g_u8Status_1 = MONITOR_START_I_WRITE;

            /* USCI_I2C as master sends START signal */
            UI2C_SET_CONTROL_REG(UI2C1, UI2C_CTL_STA);
        }
    }
#endif
}
