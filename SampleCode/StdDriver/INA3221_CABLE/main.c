/******************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * @brief
 *           Demonstrate how to control Gen1 ARGB LED lighting.
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "device.h"
#include "NuMicro.h"
#include "I2C_Control.h"
#include "Monitor_Control.h"
#include "eeprom_sim.h"
#include "i2c_eeprom_sim.h"
#define FW_version 0x01
#define FLASH_END_ADDR      0x10000
#define EEPROM_BLOCK_SIZE   (5 * 1024) 
#define EEPROM_PAGES        10         

#define EEPROM_2_BASE       (FLASH_END_ADDR - EEPROM_BLOCK_SIZE)        // 0xEC00
#define EEPROM_1_BASE       (EEPROM_2_BASE - EEPROM_BLOCK_SIZE)         // 0xD800

EEPROM_Ctx_T eeprom_system;
EEPROM_Ctx_T eeprom_user;


extern uint8_t volatile eeprom_ram[256];

// --- Protection Logic Globals ---
static enum SystemState g_system_state = STATE_NORMAL;
static uint8_t g_u8ImbalanceDebounceCounter = 0;
volatile uint32_t g_u32WarningCountdownMs = 0;
static volatile uint8_t g_u8BuzzerPattern = BUZZER_PATTERN_OFF;
static uint8_t g_bLogSavedForLatch = 0; // Flag to ensure log is saved only once
static volatile uint8_t g_u8HwWarningTriggered = 0; // Flag for HW warning interrupt
volatile uint32_t g_u32McuRunTimeSeconds = 0; // MCU runtime in seconds
static volatile uint16_t g_u16McuRunTimeMillis = 0; // Millisecond counter for runtime
extern uint8_t volatile u8EVEN_INDEX_FLAG;

extern uint8_t volatile u8UPITFlag;
extern volatile uint8_t u8UPCDFlag;
extern volatile uint8_t u8UPDCFlag;
extern volatile uint8_t u8UPCAFlag;
extern volatile uint8_t u8UPMFFlag;
extern volatile uint8_t u8UPLTFlag;
extern volatile uint8_t u8UPSNFlag;
uint8_t g_au8SerialNumber[EEPROM_SERIAL_NUMBER_SIZE] = {0};
uint8_t g_au8LotID[EEPROM_LOT_ID_SIZE] = {0};
uint8_t g_au8MFGDate[EEPROM_MFG_DATE_SIZE] = {0};

// Global instance for application settings
AppConfig_T g_AppConfig;
// Global instance for calibration data
CalibData_T g_CalibData;


void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable HIRC clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);

    /* Wait for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* Set core clock to HCLK_CLK MHz */
    CLK_SetCoreClock(HCLK_CLK);

    /* Update the Variable SystemCoreClock */
    SystemCoreClockUpdate();
}

void SysTick_Initial(void)
{
    /* Set SysTick clock source to HCLK */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk;

    /* Set load value to make SysTick period is 1 ms */
    SysTick->LOAD = HCLK_CLK / 1000;

    /* Clear SysTick counter */
    SysTick->VAL = 0;

    /* Enable SysTick interrupt */
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

    /* Enable SysTick NVIC */
    NVIC_EnableIRQ(SysTick_IRQn);
    /* Change interrupt priority to normal */
    NVIC_SetPriority(SysTick_IRQn, INT_PRIORITY_NORMAL);
}


/*  SysTick IRQ Handler                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
void SysTick_Handler(void)
{
    /* Clear interrupt flag */
    SysTick->VAL = 0;

    // --- Handle Warning Countdown and Buzzer ---
    if (g_u32WarningCountdownMs > 0)
    {
        g_u32WarningCountdownMs--;
        if (g_u8BuzzerPattern == BUZZER_PATTERN_1HZ)
        {
            if ((g_u32WarningCountdownMs % 1000) < 500)
            {
                BUZZER_PORT->DOUT |= BUZZER_PIN; // Buzzer ON
            }
            else
            {
                BUZZER_PORT->DOUT &= ~BUZZER_PIN; // Buzzer OFF
            }
        }
        else if (g_u8BuzzerPattern == BUZZER_PATTERN_2HZ)
        {
            if ((g_u32WarningCountdownMs % 500) < 250) // 2Hz means 500ms cycle, 250ms ON, 250ms OFF
            {
                BUZZER_PORT->DOUT |= BUZZER_PIN; // Buzzer ON
            }
            else
            {
                BUZZER_PORT->DOUT &= ~BUZZER_PIN; // Buzzer OFF
            }
        }
        else
        {
            BUZZER_PORT->DOUT &= ~BUZZER_PIN; // BUZZER_PATTERN_OFF or unknown
        }

        if (g_u32WarningCountdownMs == 0)
        {
            // Countdown finished.
            BUZZER_PORT->DOUT &= ~BUZZER_PIN;
            g_u8BuzzerPattern = BUZZER_PATTERN_OFF; // Ensure buzzer is off
            
            // If the countdown finished due to an imbalance warning, latch the system and set PGOOD to low.
            if (g_system_state == STATE_WARNING_COUNTDOWN)
            {
                //
                g_system_state = STATE_LATCHED;       // Latch the system, requires power cycle to reset
            }
        }
    }
    else
    {
        BUZZER_PORT->DOUT &= ~BUZZER_PIN; // Ensure buzzer is off if no countdown
        g_u8BuzzerPattern = BUZZER_PATTERN_OFF; // Reset buzzer pattern
    }

    /* Get monitor data */
    TimeCounterMonitorUpdate++;
#if ((USE_MONITOR_0 == TRUE) | (USE_MONITOR_1 == TRUE))
    if(TimeCounterMonitorUpdate >= TIMER_MONITOR_UPDATE)
    {
        u8MonitorFlag = 1;
        TimeCounterMonitorUpdate -= TIMER_MONITOR_UPDATE;
    }
#endif

    // --- MCU Runtime Counter ---
    g_u16McuRunTimeMillis++;
    if (g_u16McuRunTimeMillis >= 1000)
    {
        g_u16McuRunTimeMillis = 0;
        g_u32McuRunTimeSeconds++;
        // Use a direct 32-bit write to the aligned address to avoid memcpy warnings with volatile.
        // The cast to a volatile pointer ensures the compiler handles the memory access correctly.
        *((volatile uint32_t*)&eeprom_ram[I2C_REG_OFFSET_MCU_RUN_TIME]) = g_u32McuRunTimeSeconds;
    }
}

// --- Log Entry Data Structure ---
// NOTE: The LogEntry_T size is 22 bytes. To fit within a 256-byte log area,
// the number of entries is limited to 11 (11 * 22 = 242 bytes).
// This differs from the 12 entries mentioned in the readme.
#define MAX_LOG_ENTRIES 11
typedef struct
{
    uint32_t u32PowerOnCount;    // 0x00
    uint32_t u32RunTime;          // 0x04
    uint16_t u16Current[6];       // 0x08 (Ch1-Ch6 currents)
    uint8_t  u8ErrorCode;         // 0x14 (0x08 + 6*2)
    uint8_t  u8Checksum;          // 0x15
} LogEntry_T; // Total size: 22 bytes

void Save_Log_Entry(void) {
    LogEntry_T logEntry;
    uint8_t *pData = (uint8_t *)&logEntry;
    uint8_t checksum = 0;
    uint8_t logHead;
    uint32_t logAddress;
    int i;

    // --- 1. Populate Log Entry Data ---
    logEntry.u32PowerOnCount = g_AppConfig.u32PowerOnCount;
    logEntry.u32RunTime = g_u32McuRunTimeSeconds;
    for (i = 0; i < 6; i++) {
        uint16_t offset = I2C_REG_MONITOR_DATA_OFFSET + (i * 4);
        // Data in eeprom_ram is little-endian
        logEntry.u16Current[i] = (eeprom_ram[offset + 1] << 8) | eeprom_ram[offset];
    }
    logEntry.u8ErrorCode = eeprom_ram[I2C_REG_STATUS_OFFSET];

    // --- 2. Calculate Checksum ---
    // Checksum is calculated over the entire struct except the last byte (the checksum itself)
    for (i = 0; i < sizeof(LogEntry_T) - 1; i++) {
        checksum += pData[i];
    }
    logEntry.u8Checksum = checksum;

    // --- 3. Write Log to EEPROM (eeprom_system) ---
    // Get the current Log Head (next entry to be written) from our app config
    logHead = g_AppConfig.u8LogHead;
    if (logHead >= MAX_LOG_ENTRIES) { // Sanity check
        logHead = 0;
    }
    logAddress = logHead * sizeof(LogEntry_T);  // Calculate address offset in the log eeprom

    // Write the new log entry to the system EEPROM (for logs)
    Write_EEPROM(&eeprom_system, logAddress, (uint8_t *)&logEntry, sizeof(LogEntry_T));

    // --- 4. Update and Save Log Head ---
    // Increment and wrap around the log head for the ring buffer
    logHead++;
    if (logHead >= MAX_LOG_ENTRIES) {
        logHead = 0;
    }
    g_AppConfig.u8LogHead = logHead;

    // Save the updated log head to the user EEPROM (for configuration)
    Write_EEPROM(&eeprom_user, EE_OFFSET_LOG_HEAD, &g_AppConfig.u8LogHead, sizeof(g_AppConfig.u8LogHead));
    
    // Also update the value in the I2C RAM map for host to read
    eeprom_ram[EE_OFFSET_LOG_HEAD] = g_AppConfig.u8LogHead;
}


// GPIO Interrupt Handler for Port C (assuming INA_WARNING is on PC.4)
void GPC_IRQHandler(void)
{
    // Check if INA_WARNING_PIN triggered the interrupt
    if (GPIO_GET_INT_FLAG(INA_WARNING_PORT, INA_WARNING_PIN))
    {
        GPIO_CLR_INT_FLAG(INA_WARNING_PORT, INA_WARNING_PIN);
        g_u8HwWarningTriggered = 1; // Signal main loop
    }
    // Handle other GPC interrupts if any
}

uint32_t ProcessHardFault(uint32_t lr, uint32_t msp, uint32_t psp)
{
    /* It is casued by hardfault. Just process the hard fault */
    /* TODO: Implement your hardfault handle code here */
    while(1)
    {
    SYS_UnlockReg();
    SYS_ResetChip();
    }
    return lr;
}


// Helper function to initialize a 32-bit setting from EEPROM
static void Init_App_Setting_U32(EEPROM_Ctx_T *ctx, uint32_t offset, uint32_t *value, uint32_t default_val)
{
    Read_EEPROM(ctx, offset, (uint8_t *)value, sizeof(uint32_t));
    if (*value == 0xFFFFFFFF || *value == 0)
    {
        *value = default_val;
        Write_EEPROM(ctx, offset, (uint8_t *)value, sizeof(uint32_t));
    }
}

// Helper function to initialize an 8-bit setting from EEPROM
static void Init_App_Setting_U8(EEPROM_Ctx_T *ctx, uint32_t offset, uint8_t *value, uint8_t default_val)
{
    Read_EEPROM(ctx, offset, value, sizeof(uint8_t));
    if (*value == 0xFF)
    {
        // If uninitialized, write default value
        *value = default_val;
        Write_EEPROM(ctx, offset, value, sizeof(uint8_t));
    }
}



void initial_eeprom_ram(void)
{
  eeprom_ram[0xf0]=FW_version; //fw verison
 eeprom_ram[0xf1]='M';
 eeprom_ram[0xf2]='2';
 eeprom_ram[0xf3]='5';
 eeprom_ram[0xf4]='1';
 eeprom_ram[0xf5]='_';
 eeprom_ram[0xf6]='G';
 eeprom_ram[0xf7]='P';
 eeprom_ram[0xf8]='U';
 eeprom_ram[0xf9]='_';
 eeprom_ram[0xfA]='C';
 eeprom_ram[0xfB]='P';
}
/*---------------------------------------------------------------------------------------------------------*/
/*  Function Prototypes                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
void Peripherals_Init(void);
void Config_Load(void);
void Protection_Handler(void);
void ISP_Handler(void);
void Event_Log_Handler(void);

/*---------------------------------------------------------------------------------------------------------*/
/*  Hardware and Peripheral Initialization                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void Peripherals_Init(void)
{
    initial_eeprom_ram();

    /* Configure protection GPIOs */
    GPIO_SetMode(LED_ALARM_PORT, LED_ALARM_PIN, GPIO_MODE_OUTPUT);
    GPIO_SetMode(BUZZER_PORT, BUZZER_PIN, GPIO_MODE_OUTPUT);
    PS_PGOOD_PORT->DOUT &= ~PS_PGOOD_PIN;   // Set PGOOD to low (Normal) initially
    GPIO_SetMode(PS_PGOOD_PORT, PS_PGOOD_PIN, GPIO_MODE_OUTPUT);
    LED_ALARM_PORT->DOUT &= ~LED_ALARM_PIN; // Turn off LED initially
    BUZZER_PORT->DOUT &= ~BUZZER_PIN;     // Turn off Buzzer initially
    
    // Configure INA_WARNING pin as input and enable falling edge interrupt
    GPIO_SetMode(INA_WARNING_PORT, INA_WARNING_PIN, GPIO_MODE_INPUT);
    GPIO_EnableInt(INA_WARNING_PORT, INA_WARNING_PIN, GPIO_INT_FALLING);
    NVIC_EnableIRQ(GPC_IRQn); // Assuming GPC for PC.4
    NVIC_SetPriority(GPC_IRQn, INT_PRIORITY_HIGH); // High priority for warning
    GPIO_SET_DEBOUNCE_TIME(GPIO_DBCTL_DBCLKSRC_HCLK,GPIO_DBCTL_DBCLKSEL_1024); // Example debounce time (1024 * HCLK_CLK / 1000)
    GPIO_ENABLE_DEBOUNCE(INA_WARNING_PORT, INA_WARNING_PIN);

    /* Init I2C0 for get monitor data */
    I2C0_Init();

    /* Init UI2C1 for get monitor data */
    UI2C1_Init();

    /* Initial SysTick, enable interrupt and 1000 interrupt tick per second to add counter */
    SysTick_Initial();

    /* Start SysTick */
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Load Configuration from EEPROM                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void Config_Load(void)
{
    uint32_t ret;
    /* Init EEPROM content */
   // Init_EEPROM_Content();
    ret = Init_EEPROM(&eeprom_system, EEPROM_1_BASE, 255, EEPROM_PAGES);
   //Assert_Test("Init Sys", ret == 0);

    ret = Init_EEPROM(&eeprom_user, EEPROM_2_BASE, 255, EEPROM_PAGES);
    //Assert_Test("Init User", ret == 0);

    /* --- Initialize Application Settings from EEPROM --- */

    // Read and update Power On Count
    Read_EEPROM(&eeprom_user, EE_OFFSET_POWER_ON_COUNT, (uint8_t *)&g_AppConfig.u32PowerOnCount, sizeof(uint32_t));
    if (g_AppConfig.u32PowerOnCount == 0xFFFFFFFF)
    {
        g_AppConfig.u32PowerOnCount = 0;
    }
    g_AppConfig.u32PowerOnCount++;
    Write_EEPROM(&eeprom_user, EE_OFFSET_POWER_ON_COUNT, (uint8_t *)&g_AppConfig.u32PowerOnCount, sizeof(uint32_t));

    // Read and initialize other settings with defaults if necessary
    Init_App_Setting_U32(&eeprom_user, EE_OFFSET_IMBALANCE_THRESHOLD, &g_AppConfig.u32IMBALANCE_THRESHOLD, DEFAULT_IMBALANCE_THRESHOLD);
    Init_App_Setting_U32(&eeprom_user, EE_OFFSET_COUNTDOWN, &g_AppConfig.u32Countdown, DEFAULT_COUNTDOWN_MS);
    Init_App_Setting_U8(&eeprom_user, EE_OFFSET_SW_DEBOUNCE, &g_AppConfig.u8swdebounce, DEFAULT_SW_DEBOUNCE);
    Init_App_Setting_U8(&eeprom_user, EE_OFFSET_LOG_HEAD, &g_AppConfig.u8LogHead, DEFAULT_LOG_HEAD);

    // --- Initialize Calibration Data from EEPROM ---
    Read_EEPROM(&eeprom_user, EE_OFFSET_CALIB_DATA, (uint8_t *)&g_CalibData, sizeof(CalibData_T));

    // Check if the first gain value is uninitialized (erased flash is 0xFFFF)
    if (g_CalibData.channels[0].u16Gain == 0xFFFF)
    {
        // Data is not initialized, so write default values
      
        for (int i = 0; i < MONITOR_MAX_CHANNEL * 2; i++) // Assuming 6 channels total (2 INA3221s * 3 channels)
        {
            g_CalibData.channels[i].u16Gain = 0; // Default Gain (e.g., 1.0x represented as 1000)
            g_CalibData.channels[i].u16Offset = 0;   // Default Offset
        }
        // Write the default values back to EEPROM for persistence
        Write_EEPROM(&eeprom_user, EE_OFFSET_CALIB_DATA, (uint8_t *)&g_CalibData, sizeof(CalibData_T));
    }

		/* --- Read Serial Number, Lot ID, and MFG Date from EEPROM --- */
    Read_EEPROM(&eeprom_user, EE_OFFSET_SERIAL_NUMBER, g_au8SerialNumber, EEPROM_SERIAL_NUMBER_SIZE);
		
	Read_EEPROM(&eeprom_user, EE_OFFSET_LOT_ID, g_au8LotID, EEPROM_LOT_ID_SIZE);
		
	Read_EEPROM(&eeprom_user, EE_OFFSET_MFG_DATE, g_au8MFGDate, EEPROM_MFG_DATE_SIZE);
    /* --- Synchronize settings to RAM for I2C reporting --- */
    // memcpy((void *)eeprom_ram, &g_AppConfig, sizeof(AppConfig_T)); // This is unsafe due to struct padding, which can overwrite adjacent registers.
    // Instead, copy each member individually to its defined location in the I2C map.
    memcpy((void *)&eeprom_ram[EE_OFFSET_POWER_ON_COUNT], &g_AppConfig.u32PowerOnCount, sizeof(uint32_t));
    memcpy((void *)&eeprom_ram[EE_OFFSET_IMBALANCE_THRESHOLD], &g_AppConfig.u32IMBALANCE_THRESHOLD, sizeof(uint32_t));
    memcpy((void *)&eeprom_ram[EE_OFFSET_COUNTDOWN], &g_AppConfig.u32Countdown, sizeof(uint32_t));
    eeprom_ram[EE_OFFSET_SW_DEBOUNCE] = g_AppConfig.u8swdebounce;
    eeprom_ram[EE_OFFSET_LOG_HEAD] = g_AppConfig.u8LogHead;
    // Copy calibration data to the correct I2C register map area in eeprom_ram
    memcpy((void *)&eeprom_ram[EE_OFFSET_CALIB_DATA], &g_CalibData, sizeof(CalibData_T));

    /* --- Copy Asset Information to eeprom_ram for I2C access --- */
    memcpy((void *)&eeprom_ram[I2C_REG_OFFSET_SERIAL_NUMBER], g_au8SerialNumber, EEPROM_SERIAL_NUMBER_SIZE);
    memcpy((void *)&eeprom_ram[I2C_REG_OFFSET_LOT_ID], g_au8LotID, EEPROM_LOT_ID_SIZE);
    memcpy((void *)&eeprom_ram[I2C_REG_OFFSET_MFG_DATE], g_au8MFGDate, EEPROM_MFG_DATE_SIZE);
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Core Protection Logic Handler                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
void Protection_Handler(void)
{
    /* Check if get monitor data */
    if(u8MonitorFlag == 1)
    {
#if (USE_MONITOR_0 == TRUE)
        /* Get monitor data */
        Read_Monitor_Data_0();
#endif

#if (USE_MONITOR_1 == TRUE)
        /* Get monitor data */
        Read_Monitor_Data_1();
#endif
        /* --- Protection 1: Current Imbalance Check --- */
        if (g_system_state != STATE_LATCHED)
        {
            uint16_t currents[6];
            uint16_t max_current = 0;
            uint16_t min_current = 0xFFFF;
            int i;

            // 1. Read all 6 channel currents from eeprom_ram (unit: 10mA)
            for (i = 0; i < 6; i++)
            {
                uint16_t offset = I2C_REG_MONITOR_DATA_OFFSET + (i * 4);
                currents[i] = (eeprom_ram[offset + 1] << 8) | eeprom_ram[offset];
            }

            // 2. Find max and min current
            for (i = 0; i < 6; i++)
            {
                if (currents[i] > max_current) max_current = currents[i];
                if (currents[i] < min_current) min_current = currents[i];
            }

            // 3. Check imbalance condition (Threshold is in mA, currents are in 10mA)
            if ((max_current - min_current) > (g_AppConfig.u32IMBALANCE_THRESHOLD / 10))
            {
                if (g_u8ImbalanceDebounceCounter < IMBALANCE_DEBOUNCE_COUNT)
                {
                    g_u8ImbalanceDebounceCounter++;
                }
            }
            else
            {
                // Condition cleared, reset debounce counter
                g_u8ImbalanceDebounceCounter = 0;
            }

            // 4. Handle state transitions based on debounce
            if (g_u8ImbalanceDebounceCounter >= IMBALANCE_DEBOUNCE_COUNT)
            {
                // Imbalance event confirmed
                if (g_system_state == STATE_NORMAL)
                {
                    // --- Trigger Warning Action ---
                    g_system_state = STATE_WARNING_COUNTDOWN;
                    g_u32WarningCountdownMs = g_AppConfig.u32Countdown;
                    g_u8BuzzerPattern = BUZZER_PATTERN_1HZ; // Set 1Hz buzzer
                    eeprom_ram[I2C_REG_STATUS_OFFSET] |= STATUS_BIT_IMBALANCE;
                    // Turn on LED
                    LED_ALARM_PORT->DOUT |= LED_ALARM_PIN;
                    // Buzzer is handled by SysTick_Handler
                }
            }
            else if (g_system_state == STATE_WARNING_COUNTDOWN)
            {
                // Imbalance event not present or cleared, return to normal
                g_system_state = STATE_NORMAL;
                g_u32WarningCountdownMs = 0;
                g_u8BuzzerPattern = BUZZER_PATTERN_OFF;
                eeprom_ram[I2C_REG_STATUS_OFFSET] &= ~STATUS_BIT_IMBALANCE;
                LED_ALARM_PORT->DOUT &= ~LED_ALARM_PIN;
                BUZZER_PORT->DOUT &= ~BUZZER_PIN;
            }
        }

        if (g_system_state == STATE_LATCHED)
        {
            if (g_bLogSavedForLatch == 0)
            {
                Save_Log_Entry();
                g_bLogSavedForLatch = 1; // Ensure log is saved only once per latch event
            }
            PS_PGOOD_PORT->DOUT |= PS_PGOOD_PIN; // Set PGOOD to high (Protection state)
        }

        u8MonitorFlag = 0;
    }

    /* --- Protection 2: HW Warning Check (triggered by GPIO interrupt) --- */
    if (g_u8HwWarningTriggered )
    {
        if (g_bLogSavedForLatch == 0)
        {
            // Immediately read the latest sensor data to get a complete snapshot
            Read_Monitor_Data_0();
            Read_Monitor_Data_1();

            // Now save the log with the fresh data
            Save_Log_Entry();
            g_bLogSavedForLatch = 1; // Ensure log is saved only once per latch event
            PS_PGOOD_PORT->DOUT |= PS_PGOOD_PIN; // Set PGOOD to high (Protection state)
        }
        g_u8HwWarningTriggered = 0; // Clear the trigger flag after handling
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/*  ISP (Firmware Update) Handler                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
void ISP_Handler(void)
{
    if (u8UpdateISPFlag== 1)
    {
        SYS_UnlockReg();
        outpw(&SYS->RSTSTS, 3);//clear bit

        FMC_Open();
        FMC_SetBootSource(1);          // Boot from LDROM
        NVIC->ICPR[0] = 0xFFFFFFFF;    // Clear Pending Interrupt
        /* Set VECMAP to LDROM for booting from LDROM */
        FMC_SetVectorPageAddr(FMC_LDROM_BASE);
        SYS_ResetCPU();
        while(1);
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Event Log Reading Handler                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void Event_Log_Handler(void)
{
    // Check if the host requested to read a log entry via I2C
    if (u8EVEN_INDEX_FLAG == 1)
    {
        u8EVEN_INDEX_FLAG = 0; // Clear the flag after processing

        // Read the requested log index from the I2C register map
        uint8_t event_index = eeprom_ram[I2C_REG_EVENT_LOG_INDEX];

        if (event_index < MAX_LOG_ENTRIES)
        {
            LogEntry_T log_entry;
            uint32_t log_address = event_index * sizeof(LogEntry_T);

            // Read the specified log entry from the system EEPROM
            Read_EEPROM(&eeprom_system, log_address, (uint8_t *)&log_entry, sizeof(LogEntry_T));

            // Copy the log data to the I2C RAM map (20 bytes from 0x80 to 0x93)
            // This copies PowerOnCount, RunTime, and all 6 current channels.
            // It excludes the ErrorCode and Checksum fields.
            memcpy((void *)&eeprom_ram[I2C_REG_EVENT_LOG_DATA], &log_entry, 20);
        }
        else
        {
            // If the requested index is invalid, fill the data area with 0xFF
            memset((void *)&eeprom_ram[I2C_REG_EVENT_LOG_DATA], 0xFF, 20);
        }
    }
     if (u8UPITFlag == 1)
        {
            u8UPITFlag = 0; // Clear the flag
            // Write the new threshold value to the physical EEPROM to make it persistent.
            Write_EEPROM(&eeprom_user, EE_OFFSET_IMBALANCE_THRESHOLD, (uint8_t *)&g_AppConfig.u32IMBALANCE_THRESHOLD, sizeof(uint32_t));
        }
        if (u8UPCDFlag == 1)
        {
            u8UPCDFlag = 0; // Clear the flag
            Write_EEPROM(&eeprom_user, EE_OFFSET_COUNTDOWN, (uint8_t *)&g_AppConfig.u32Countdown, sizeof(uint32_t));
        }
        if (u8UPDCFlag == 1)
        {
            u8UPDCFlag = 0; // Clear the flag
            Write_EEPROM(&eeprom_user, EE_OFFSET_SW_DEBOUNCE, &g_AppConfig.u8swdebounce, sizeof(uint8_t));
        }
        if (u8UPCAFlag == 1)
        {
            u8UPCAFlag = 0; // Clear the flag
            Write_EEPROM(&eeprom_user, EE_OFFSET_CALIB_DATA, (uint8_t *)&g_CalibData, sizeof(CalibData_T));
        }
        if (u8UPMFFlag == 1)
        {
            u8UPMFFlag = 0;
            Write_EEPROM(&eeprom_user, EE_OFFSET_MFG_DATE, g_au8MFGDate, EEPROM_MFG_DATE_SIZE);
        }
        if (u8UPLTFlag == 1)
        {
            u8UPLTFlag = 0;
            Write_EEPROM(&eeprom_user, EE_OFFSET_LOT_ID, g_au8LotID, EEPROM_LOT_ID_SIZE);
        }
        if (u8UPSNFlag == 1)
        {
            u8UPSNFlag = 0;
            Write_EEPROM(&eeprom_user, EE_OFFSET_SERIAL_NUMBER, g_au8SerialNumber, EEPROM_SERIAL_NUMBER_SIZE);
        }
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void)
{
    /* Unlock write-protected registers */
    SYS_UnlockReg();

    /* Init system and multi-funcition I/O */
    SYS_Init();

    /* Lock protected registers */
    SYS_LockReg();

    /* Initialize hardware and load configuration */
    Peripherals_Init();
    Config_Load();

    /* Main application loop */
    while (1)
    {
        Protection_Handler();
        ISP_Handler();
        Event_Log_Handler();
       
    }
}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
