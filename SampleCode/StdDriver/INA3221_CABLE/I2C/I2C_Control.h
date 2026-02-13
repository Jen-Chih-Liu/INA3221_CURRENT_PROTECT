/******************************************************************************
 * @file     I2C_Control.h
 * @brief    I2C_Control Hardware file
 *
 * @note
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __I2C_CONTROL_H
#define __I2C_CONTROL_H

#include <stdint.h>
#include <NuMicro.h>
#include "device.h"

#define ADDRESS_I2C_SLAVE_7BIT      0x4D    // The device address is 0x9A
#define ADDRESS_I2C_SLAVE_8BIT_W    ((ADDRESS_I2C_SLAVE_7BIT << 1)    )
#define ADDRESS_I2C_SLAVE_8BIT_R    ((ADDRESS_I2C_SLAVE_7BIT << 1) + 1)
#define SPEED_I2C_BUS               100000
#define LEN_MAX_I2C_DATA            256

/*-------------------------------------------------------------*/
//extern volatile uint8_t u8UpdateFRUDataFlag;
extern volatile uint32_t u32UpdateTargetAddress;
extern volatile uint8_t u8UpdateTargetOffset;
extern volatile uint8_t u8UpdateTargetSize;
extern volatile uint8_t u8UpdateISPFlag;
extern uint8_t au8UpdateData[LEN_MAX_I2C_DATA];

/* Global Variables for I2C Communication */
#define CMD_LEN_ISP     7
#define CMD_LEN_UPDATE_Imbalance_Threshold  8
#define CMD_LEN_UPDATE_Imbalance_STR 4
#define DATA_LEN_GET_POWER_INFO    16
#define DATA_OFFSET_GET_POWER_INFO 4
extern uint8_t Data_Get_Power_Info[DATA_LEN_GET_POWER_INFO];
#define CMD_OFFSET_GET_POWER_INFO  0xDE

/* Update FRU */
//#define CMD_OFFSET_UPDATE_START    0xF0
/*-------------------------------------------------------------*/

/*-------------------------------------------------------------*/
typedef void (*I2C_FUNC)(I2C_T *i2c, uint32_t u32Status);
static I2C_FUNC s_I2C1HandlerFn = NULL;

void I2C1_Init(void);
/*-------------------------------------------------------------*/

#endif  /* __I2C_CONTROL_H */
