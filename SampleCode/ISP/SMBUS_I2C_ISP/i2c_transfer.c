/***************************************************************************//**
 * @file     i2c_transfer.c
 * @brief    ISP support function source file
 * @version  0x32
 * @date     14, June, 2017
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2017-2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "targetdev.h"
#include "config.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
//uint8_t i2c_rcvbuf[64];
//volatile uint8_t bI2cDataReady;

volatile uint8_t g_u8SlvDataLen;

__STATIC_INLINE void I2C_SlaveTRx(I2C_T *i2c, uint32_t u32Status);

extern uint32_t Pclk0;

#define LEN_MAX_I2C_DATA            34
uint8_t au8UpdateData[LEN_MAX_I2C_DATA];
uint8_t au8SlvRxData[LEN_MAX_I2C_DATA];
volatile uint8_t i2c_ack_data = 0;
volatile uint8_t u8eraseflashflag = 0;
volatile uint8_t u8JMPAPflag = 0;
volatile uint8_t u8PROGAPflag = 0;
volatile uint16_t u16SlvDataLen;

#define CMD_LEN_ERASE     7
const uint8_t CMD_ERASE[CMD_LEN_ERASE]
    = {0xb0, 0X05, 0x45, 0x52, 0x41, 0x53, 0x45
      };

#define CMD_LEN_JMPAP     7
const uint8_t CMD_JMPAP[CMD_LEN_JMPAP]
    = {0x4F, 0X05, 0x4A, 0x4D, 0x50, 0x41, 0x50
      };


#define CMD_LEN_PROG_AP  34
const uint8_t CMD_PROG_AP[2]
    = {0xb1, 0X20};

extern __ALIGNED(4) uint8_t Write_buff[32];
extern __ALIGNED(4) uint8_t Read_buff[32];
void I2C_Init(void)
{
    /* Reset I2C0 */
    SYS->IPRST1 |=  SYS_IPRST1_I2C1RST_Msk;
    SYS->IPRST1 &= ~SYS_IPRST1_I2C1RST_Msk;
    /* Open I2C0 and set clock to 100k */
    I2C1->CLKDIV = (uint32_t)(((Pclk0 * 10U) / (100000 * 4U) + 5U) / 10U - 1U); /* Compute proper divider for I2C clock */;
    I2C1->CTL0 |= I2C_CTL0_I2CEN_Msk;
    /* Set I2C0 ADDR0 Slave Addresses */
    I2C1->ADDR0  = (I2C_ADDR << 1U) | I2C_GCMODE_DISABLE;
    I2C1->CTL0 |= I2C_CTL0_INTEN_Msk;
    /* I2C enter no address SLV mode */
    I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI_AA);
    NVIC_EnableIRQ(I2C1_IRQn);
}



/*---------------------------------------------------------------------------------------------------------*/
/*  I2C0 IRQ Handler                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void I2C1_IRQHandler(void)
{
    uint32_t u32Status;
    u32Status = I2C_GET_STATUS(I2C1);

    if (I2C_GET_TIMEOUT_FLAG(I2C1))
    {
        /* Clear I2C0 Timeout Flag */
        I2C1->TOCTL |= I2C_TOCTL_TOIF_Msk;
    }
    else
    {
        I2C_SlaveTRx(I2C1, u32Status);
    }
}
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

        I2C_SET_DATA(i2c, i2c_ack_data);
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if (u32Status == 0xB8)                /* Data byte in I2CDAT has been transmitted ACK has been received */
    {

        I2C_SET_DATA(i2c, 0xa1); //read not over 2 byte
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if (u32Status == 0xC0)                 /* Data byte or last data in I2CDAT has been transmitted
                                                  Not ACK has been received */
    {
        /* Clear flag */
        I2C_SET_DATA(i2c, 0xa2);     //read not over 2 byte
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
        if ((u16SlvDataLen == 1) && 
					(au8SlvRxData[0] == 0xa1)) //read verison
        {
            i2c_ack_data = ldrom_fw_Verision; //isp fw version.
        }
        else if ((u16SlvDataLen == CMD_LEN_ERASE) &&
                 (au8SlvRxData[0] == CMD_ERASE[0])
                )
        {
            /* Check if get correct command */
            for (i = 0; i < CMD_LEN_ERASE; i++)
            {
                if (au8SlvRxData[i] != CMD_ERASE[i])
                {
                    break;
                }
            }

            /* Get correct command */
            if (i == CMD_LEN_ERASE)
            {
                /* Set flag */
                u8eraseflashflag = 1;
            }
        }
        //JUMP APROM
        else if ((u16SlvDataLen == CMD_LEN_JMPAP) &&
                 (au8SlvRxData[0] == CMD_JMPAP[0])
                )
        {
            /* Check if get correct command */
            for (i = 0; i < CMD_LEN_JMPAP; i++)
            {
                if (au8SlvRxData[i] != CMD_JMPAP[i])
                {
                    break;
                }
            }

            /* Get correct command */
            if (i == CMD_LEN_JMPAP)
            {
                /* Set flag */
                u8JMPAPflag = 1;
            }
        }


        else if ((u16SlvDataLen == CMD_LEN_PROG_AP) &&
                 (au8SlvRxData[0] == CMD_PROG_AP[0]) &&
                 (au8SlvRxData[1] == CMD_PROG_AP[1])
                )
        {
            for (i = 0; i < 32; i++)
            {
                Write_buff[i] = au8SlvRxData[i + 2];
            }

            /* Set flag */
            u8PROGAPflag = 1;

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

#if 0

/*---------------------------------------------------------------------------------------------------------*/
/*  I2C TRx Callback Function                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void I2C_SlaveTRx(I2C_T *i2c, uint32_t u32Status)
{
    uint8_t u8data;

    if (u32Status == 0x60)                      /* Own SLA+W has been receive; ACK has been return */
    {
        bI2cDataReady = 0;
        g_u8SlvDataLen = 0;
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if (u32Status == 0x80)                 /* Previously address with own SLA address
                                                   Data has been received; ACK has been returned*/
    {
        i2c_rcvbuf[g_u8SlvDataLen] = I2C_GET_DATA(i2c);
        g_u8SlvDataLen++;
        g_u8SlvDataLen &= 0x3F;
        bI2cDataReady = (g_u8SlvDataLen == 0);

        if (g_u8SlvDataLen == 0x3F)
        {
            I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI);
        }
        else
        {
            I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
        }
    }
    else if (u32Status == 0xA8)                 /* Own SLA+R has been receive; ACK has been return */
    {
        g_u8SlvDataLen = 0;
        u8data = response_buff[g_u8SlvDataLen];
        I2C_SET_DATA(i2c, u8data);
        g_u8SlvDataLen++;
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if (u32Status == 0xB8)
    {
        u8data = response_buff[g_u8SlvDataLen];
        I2C_SET_DATA(i2c, u8data);
        g_u8SlvDataLen++;
        g_u8SlvDataLen &= 0x3F;

        if (g_u8SlvDataLen == 0x00)
        {
            I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI);
        }
        else
        {
            I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
        }
    }
    else if (u32Status == 0xC8)
    {
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if (u32Status == 0xC0)                 /* Data byte or last data in I2CDAT has been transmitted
                                                   Not ACK has been received */
    {
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if (u32Status == 0x88)                 /* Previously addressed with own SLA address; NOT ACK has
                                                   been returned */
    {
        i2c_rcvbuf[g_u8SlvDataLen] = I2C_GET_DATA(i2c);
        g_u8SlvDataLen++;
        bI2cDataReady = (g_u8SlvDataLen == 64);
        g_u8SlvDataLen = 0;
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if (u32Status == 0xA0)                 /* A STOP or repeated START has been received while still
                                                   addressed as Slave/Receiver*/
    {
        g_u8SlvDataLen = 0;
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else
    {
        /* TO DO */
        // printf("Status 0x%x is NOT processed\n", u32Status);
    }
}
#endif

