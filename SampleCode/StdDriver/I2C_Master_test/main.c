/**************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * @brief
 *           Show a Master how to access Slave.
 *           This sample code needs to work with I2C_Slave.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2022 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <string.h>
#include <stdio.h>
#include "NuMicro.h"


void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable Internal RC 48 MHz clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    /* Waiting for Internal RC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);
    /* Switch HCLK clock source to Internal RC and HCLK source divide 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));
    /* Enable UART0 module clock */
    CLK_EnableModuleClock(UART0_MODULE);
    /* Enable I2C0 module clock */
    CLK_EnableModuleClock(I2C0_MODULE);
    /* Enable GPIO clock */
    CLK_EnableModuleClock(GPB_MODULE);
    /* Select UART clock source from HIRC */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));
    /* Update System Core Clock */
    SystemCoreClockUpdate();
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    Uart0DefaultMPF();
    /* Set I2C0 multi-function pins */
    SYS->GPB_MFPL = (SYS->GPB_MFPL & ~(SYS_GPB_MFPL_PB4MFP_Msk | SYS_GPB_MFPL_PB5MFP_Msk)) |
                    (SYS_GPB_MFPL_PB4MFP_I2C0_SDA | SYS_GPB_MFPL_PB5MFP_I2C0_SCL);
    /* I2C pins enable schmitt trigger */
    PB->SMTEN |= GPIO_SMTEN_SMTEN4_Msk | GPIO_SMTEN_SMTEN5_Msk;
}

void UART0_Init()
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Reset IP */
    SYS_ResetModule(UART0_RST);
    /* Configure UART0 and set UART0 Baudrate */
    UART_Open(UART0, 115200);
}

void I2C0_Init(void)
{
    /* Open I2C module and set bus clock */
    I2C_Open(I2C0, 100000);
    /* Get I2C0 Bus Clock */
    printf("I2C clock %d Hz\n", I2C_GetBusClockFreq(I2C0));   
}

void I2C0_Close(void)
{
    /* Disable I2C0 interrupt and clear corresponding NVIC bit */
    I2C_DisableInt(I2C0);
    NVIC_DisableIRQ(I2C0_IRQn);
    /* Disable I2C0 and close I2C0 clock */
    I2C_Close(I2C0);
    CLK_DisableModuleClock(I2C0_MODULE);
}

void print_hex_array(const char *title, uint8_t *array, int len)
{
    int i;
    printf("%s", title);
    for (i = 0; i < len; i++)
    {
        printf("0x%02X ", array[i]);
    }
    printf("\n");
}

/*---------------------------------------------------------------------------------------------------------*/
/*  I2C Master Test Cases (Based on MCU FW v1.2 Spec)                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#define I2C_SLAVE_ADDR 0x4D

void Run_MCU_Test_Cases(void)
{
    uint32_t i;
    uint8_t data[32];

    printf("\n--- TC-01: Device Connection and Firmware Version Read ---\n");
    uint8_t fw_ver = I2C_ReadByteOneReg(I2C0, I2C_SLAVE_ADDR, 0xF0);
    printf("Firmware version read: 0x%02X\n", fw_ver);
    if (fw_ver != 0xFF && fw_ver != 0x00)
    {
        printf("TC-01 PASS\n");
    }
    else
    {
        printf("TC-01 FAIL\n");
    }

    printf("\n--- TC-02: Real-time Monitoring Data Read ---\n");
    for (i = 0; i < 4; i++)
    {
        data[i] = I2C_ReadByteOneReg(I2C0, I2C_SLAVE_ADDR, 0x30 + i);
    }
    print_hex_array("Raw data read from 0x30: ", data, 4);
    uint16_t current = (data[1] << 8) | data[0];
    uint16_t voltage = (data[3] << 8) | data[2];
    printf("Calculated result: Ch1 Current = %d mA, Ch1 Voltage = %d mV\n", current * 10, voltage * 10);
    if (current != 0xFFFF && voltage != 0xFFFF)
    {
        printf("TC-02 PASS\n");
    }
    else
    {
        printf("TC-02 FAIL\n");
    }

    printf("\n--- TC-03: Special Command Write - Update Imbalance Threshold (UPIT) ---\n");
    uint8_t cmd_upit[] = {0x55, 0x50, 0x49, 0x54, 0xDC, 0x05, 0x00, 0x00};
    I2C_WriteMultiBytes(I2C0, I2C_SLAVE_ADDR, cmd_upit, sizeof(cmd_upit));
    printf("Sending UPIT command (set to 1500mA).\n");
    CLK_SysTickDelay(10000); // Wait for Flash write completion (10ms)
    for (i = 0; i < 4; i++)
    {
        data[i] = I2C_ReadByteOneReg(I2C0, I2C_SLAVE_ADDR, 0x04 + i);
    }
    print_hex_array("Data read from 0x04: ", data, 4);
    if (data[0] == 0xDC && data[1] == 0x05 && data[2] == 0x00 && data[3] == 0x00)
    {
        printf("TC-03 PASS\n");
    }
    else
    {
        printf("TC-03 FAIL\n");
    }

    printf("\n--- TC-04: Special Command Write - Update Countdown Duration (UPCD) ---\n");
    uint8_t cmd_upcd[] = {0x55, 0x50, 0x43, 0x44, 0x10, 0x27, 0x00, 0x00};
    I2C_WriteMultiBytes(I2C0, I2C_SLAVE_ADDR, cmd_upcd, sizeof(cmd_upcd));
    printf("Sending UPCD command (set to 10000ms).\n");
    CLK_SysTickDelay(10000); // 10ms
    for (i = 0; i < 4; i++)
    {
        data[i] = I2C_ReadByteOneReg(I2C0, I2C_SLAVE_ADDR, 0x08 + i);
    }
    print_hex_array("Data read from 0x08: ", data, 4);
    if (data[0] == 0x10 && data[1] == 0x27 && data[2] == 0x00 && data[3] == 0x00)
    {
        printf("TC-04 PASS\n");
    }
    else
    {
        printf("TC-04 FAIL\n");
    }

    printf("\n--- TC-05: Special Command Write - Update SW Debounce Count (SWDC) ---\n");
    uint8_t cmd_swdc[] = {0x55, 0x50, 0x44, 0x43, 0x05};
    I2C_WriteMultiBytes(I2C0, I2C_SLAVE_ADDR, cmd_swdc, sizeof(cmd_swdc));
    printf("Sending SWDC command (set to 5).\n");
    CLK_SysTickDelay(10000); // 10ms
    data[0] = I2C_ReadByteOneReg(I2C0, I2C_SLAVE_ADDR, 0x0C);
    printf("Data read from 0x0C: 0x%02X\n", data[0]);
    if (data[0] == 0x05)
    {
        printf("TC-05 PASS\n");
    }
    else
    {
        printf("TC-05 FAIL\n");
    }

    printf("\n--- TC-06: Special Command Write - Update Calibration Data (UPCA) ---\n");
    uint8_t cmd_upca[] = {0x55, 0x50, 0x43, 0x41, 0x01, 0xE8, 0x03, 0xFB, 0xFF};
    I2C_WriteMultiBytes(I2C0, I2C_SLAVE_ADDR, cmd_upca, sizeof(cmd_upca));
    printf("Sending UPCA command (Ch 2, Gain=1000, Offset=-5).\n");
    CLK_SysTickDelay(10000); // 10ms
    for (i = 0; i < 4; i++)
    {
        data[i] = I2C_ReadByteOneReg(I2C0, I2C_SLAVE_ADDR, 0x14 + i);
    }
    print_hex_array("Data read from 0x14: ", data, 4);
    if (data[0] == 0xE8 && data[1] == 0x03 && data[2] == 0xFB && data[3] == 0xFF)
    {
        printf("TC-06 PASS\n");
    }
    else
    {
        printf("TC-06 FAIL\n");
    }

    printf("\n--- TC-07: Asset Information Write and Read ---\n");
    uint8_t cmd_upmf[20] = {0x55, 0x50, 0x4D, 0x46, '2', '0', '2', '6', '0', '2', '1', '3', 0, 0, 0, 0, 0, 0, 0, 0};
    I2C_WriteMultiBytes(I2C0, I2C_SLAVE_ADDR, cmd_upmf, sizeof(cmd_upmf));
    printf("Sending UPMF command (write '20260213').\n");
    CLK_SysTickDelay(10000); // 10ms
    for (i = 0; i < 16; i++)
    {
        data[i] = I2C_ReadByteOneReg(I2C0, I2C_SLAVE_ADDR, 0xC0 + i);
    }
    print_hex_array("Data read from 0xC0: ", data, 16);
    if (memcmp(data, &cmd_upmf[4], 8) == 0 && data[8] == 0) // Simple verification
    {
        printf("TC-07 PASS\n");
    }
    else
    {
        printf("TC-07 FAIL\n");
    }

    printf("\n--- TC-08: Jump to LDROM (ISP Mode) Command Test ---\n");
    uint8_t cmd_jmpld[] = {0x4E, 0x05, 0x4A, 0x4D, 0x50, 0x4C, 0x44};
    I2C_WriteMultiBytes(I2C0, I2C_SLAVE_ADDR, cmd_jmpld, sizeof(cmd_jmpld));
    printf("Sending JMPLD command.\n");
    printf("Waiting 500ms for MCU to reboot...\n");
    CLK_SysTickDelay(500000); // 500ms
    printf("Attempting to read firmware version again (expected to fail with NACK or Timeout).\n");
    fw_ver = I2C_ReadByteOneReg(I2C0, I2C_SLAVE_ADDR, 0xF0);
    printf("Read operation returned: 0x%02X (this value is invalid).\n", fw_ver);
    printf("TC-08 Please verify manually: Check if the device has exited the main program and entered LDROM.\n");

    printf("\n--- TC-09: Event Log Read (Template Only) ---\n");
    printf("This test case requires specific preconditions and read logic. Please implement it according to the specification.\n");
}

int32_t main(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();
    /* Init System, IP clock and multi-function I/O */
    SYS_Init();
    /* Init UART0 for printf */
    UART0_Init();
    /* Lock protected registers */
    SYS_LockReg();
    /*
        This sample code sets I2C bus clock to 100kHz. Then, Master accesses Slave with Byte Write
        and Byte Read operations, and check if the read data is equal to the programmed data.
    */
    printf("+-------------------------------------------------------+\n");
    printf("|       I2C Driver Sample Code(Master) for access Slave |\n");
    printf("+-------------------------------------------------------+\n");
    printf("Configure I2C0 as a master.\n");
    printf("The I/O connection for I2C0:\n");
    printf("I2C0_SDA(PB.4), I2C0_SCL(PB.5)\n");
    /* Init I2C0 */
    I2C0_Init();

    printf("\nPlease make sure the I2C Slave device (address 0x%X) is running!\n", I2C_SLAVE_ADDR);
    printf("Press any key to start running test cases...\n");
    getchar();

    Run_MCU_Test_Cases();

    I2C0_Close();

    printf("\nTest finished.\n");

    while (1);
}
