/******************************************************************************
 * @file     Monitor_Control.h
 * @brief    Monitor_Control Hardware file
 *
 * @note
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __Monitor_CONTROL_H
#define __Monitor_CONTROL_H

/* Set to 1 to enable mock test mode without actual INA3221 hardware */
//#define INA3221_MOCK_TEST           1

#include <stdint.h>
#include <NuMicro.h>
#include "device.h"

/* Monitor INA3221 */
#define MONITOR_MAX_CHANNEL         3
#define REGISTER_START_ADDRESS      0x01
#define REGISTER_OFFSET_ADDRESS     0x02
#define REGISTER_DATA_COUNT         2

typedef struct
{
    uint16_t Value : 15;  // Bit 0 ~ 14
    uint16_t Sign  : 1;   // Bit 15
} BitField16_T;
typedef struct Monitor_Data_Tag
{
    BitField16_T Current;
    BitField16_T Voltage;
} Monitor_Data_T;

/* Monitor Setting 0 */
/* I2C Bus Setting */
#define ADDRESS_MONIOR_0_7BIT       0x40
#define ADDRESS_MONIOR_0_8BIT_W     ((ADDRESS_MONIOR_0_7BIT << 1)    )
#define ADDRESS_MONIOR_0_8BIT_R     ((ADDRESS_MONIOR_0_7BIT << 1) + 1)
#define SPEED_MONIOR_0_BUS          100000
extern Monitor_Data_T au8MonitorData_0[MONITOR_MAX_CHANNEL];
/* Shunt Resistor */
#define SHUNT_RATIO_MOLECULAR_0_CH0     200    // 0.005 ohm, 
#define SHUNT_RATIO_DENOMINATOR_0_CH0   1
#define SHUNT_RATIO_MOLECULAR_0_CH1     200   // 0.005 ohm, big current
#define SHUNT_RATIO_DENOMINATOR_0_CH1   1
#define SHUNT_RATIO_MOLECULAR_0_CH2     200     // 0.005 ohm
#define SHUNT_RATIO_DENOMINATOR_0_CH2   1
/* MCU report slave address */
#define ADDRESS_REPORT_0_7BIT       0x0A
#define ADDRESS_REPORT_0_8BIT_W     ((ADDRESS_REPORT_0_7BIT << 1)    )
#define ADDRESS_REPORT_0_8BIT_R     ((ADDRESS_REPORT_0_7BIT << 1) + 1)

/* Monitor Setting 1 */
/* I2C Bus Setting */
#define ADDRESS_MONIOR_1_7BIT       0x40
#define ADDRESS_MONIOR_1_8BIT_W     ((ADDRESS_MONIOR_1_7BIT << 1)    )
#define ADDRESS_MONIOR_1_8BIT_R     ((ADDRESS_MONIOR_1_7BIT << 1) + 1)
#define SPEED_MONIOR_1_BUS          100000
extern Monitor_Data_T au8MonitorData_1[MONITOR_MAX_CHANNEL];
/* Shunt Resistor */
#define SHUNT_RATIO_MOLECULAR_1     10
#define SHUNT_RATIO_DENOMINATOR_1   1
/* MCU report slave address */
#define ADDRESS_REPORT_1_7BIT       0x1A
#define ADDRESS_REPORT_1_8BIT_W     ((ADDRESS_REPORT_1_7BIT << 1)    )
#define ADDRESS_REPORT_1_8BIT_R     ((ADDRESS_REPORT_1_7BIT << 1) + 1)

/* Operation Status */
#define MONITOR_IDLE                0
#define MONITOR_START_I_WRITE       1
#define MONITOR_WRITE_I_REGISTER    2
#define MONITOR_START_I_READ        3
#define MONITOR_READ_I_FIRST_DATA   4
#define MONITOR_READ_I_SECOND_DATA  5
#define MONITOR_START_V_WRITE       6
#define MONITOR_WRITE_V_REGISTER    7
#define MONITOR_START_V_READ        8
#define MONITOR_READ_V_FIRST_DATA   9
#define MONITOR_READ_V_SECOND_DATA  10

/*-------------------------------------------------------------*/
/* Global Variables for I2C Communication */
extern uint32_t TimeCounterMonitorUpdate;
#define TIMER_MONITOR_UPDATE   1700 //unit ms
#define ina3221_config 0x7a47  //refer ina3221 config spec.extern volatile uint8_t u8MonitorFlag;
extern volatile uint8_t u8MonitorFlag;
/*-------------------------------------------------------------*/

/*-------------------------------------------------------------*/
typedef void (*I2C_MONITOR_FUNC)(I2C_T *i2c, uint32_t u32Status);
static I2C_MONITOR_FUNC s_I2C0HandlerFn = NULL;
typedef void (*UI2C_MONITOR_FUNC)(UI2C_T *ui2c, uint32_t u32Status);
static UI2C_MONITOR_FUNC s_UI2C1HandlerFn = NULL;

void I2C0_Init(void);
void UI2C1_Init(void);
void Read_Monitor_Data_0(void);
void Read_Monitor_Data_1(void);
/*-------------------------------------------------------------*/

#endif  /* __I2C_CONTROL_H */
