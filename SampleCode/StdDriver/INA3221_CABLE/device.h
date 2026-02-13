/******************************************************************************
 * @file     device.h
 * @brief    device header file
 *
 * @note
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#define TRUE                     (1UL)    ///< Boolean true, define to use in API parameters or return value
#define FALSE                    (0UL)    ///< Boolean false, define to use in API parameters or return value

// <<< Use Configuration Wizard in Context Menu >>>
/*!<Define Current Monitor */
// <h> Eanble Current Monitor
// <h> Current Monitor 0
// <q.0> Use Current Monitor 0 
#define USE_MONITOR_0            0x00000001
/* Current Monitor 0 */
#if (USE_MONITOR_0 == TRUE)
// <h> Channel Selection
// <q.0> Use Channel 1
#define USE_MONITOR_0_CH1        0x00000001
// <q.0> Use Channel 2
#define USE_MONITOR_0_CH2        0x00000001
// <q.0> Use Channel 3
#define USE_MONITOR_0_CH3        0x00000001
// </h>
#else
#define USE_MONITOR_0_CH1        0x00000000
#define USE_MONITOR_0_CH2        0x00000000
#define USE_MONITOR_0_CH3        0x00000000
#endif
// </h>

// <h> Current Monitor 1
// <q.0> Use Current Monitor 1
#define USE_MONITOR_1            0x00000001
/* Current Monitor 1 */
#if (USE_MONITOR_1 == TRUE)
// <h> Channel Selection
// <q.0> Use Channel 1
#define USE_MONITOR_1_CH1        0x00000000
// <q.0> Use Channel 2
#define USE_MONITOR_1_CH2        0x00000000
// <q.0> Use Channel 3
#define USE_MONITOR_1_CH3        0x00000000
// </h>
#else
#define USE_MONITOR_1_CH1        0x00000000
#define USE_MONITOR_1_CH2        0x00000000
#define USE_MONITOR_1_CH3        0x00000000
#endif
// </h>
// </h>
// <<< end of configuration section >>>

#define HCLK_CLK                 FREQ_48MHZ

/*!<Define Interrupt priority level */
#define INT_PRIORITY_HIGHEST     0
#define INT_PRIORITY_HIGH        1
#define INT_PRIORITY_NORMAL      2
#define INT_PRIORITY_LOW         3

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
