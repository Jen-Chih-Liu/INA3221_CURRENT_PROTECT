/******************************************************************************/ /**
 * @file     main.c
 * @version  V1.00
 * @brief    INA3221 6-channel current protection firmware for Nuvoton M251.
 *
 *           Features:
 *             - Reads Ch1~Ch6 current/voltage via I2C1 (INA_A) and UI2C1 (INA_B)
 *             - Detects current imbalance and overcurrent (OC) faults
 *             - Three-phase warning/protection sequence:
 *                 Phase 1 (0~20 s)   : Alarm LED blinks at 1 Hz, buzzer silent
 *                 Phase 2 (20s~3min) : LED + buzzer blink at 2 Hz
 *                 Latch   (3 min)    : PS_PGOOD asserted, system locked out
 *             - If NSP23 IC is present (PID = 0xACCCCAAA), plays bell sound
 *               N_PLAY(1) in Phase 1 and N_PLAY(2) in Phase 2 instead of buzzer
 *             - Saves event logs to Flash EEPROM (ring buffer, 11 entries max)
 *             - Communicates with host via I2C0 slave (register map = eeprom_ram)
*             - Supports ISP firmware update triggered by I2C host command
*
* @note     Hardware: Nuvoton M251, HCLK = 48 MHz
*           INA_A (Ch 1-3) : I2C1  master, addr 0x40
*           INA_B (Ch 4-6) : UI2C1 master, addr 0x40
*           I2C0           : slave,         addr 0x4D (host communication)
*
* @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
******************************************************************************/

#include "I2C_Control.h" /* I2C0 slave driver (host communication)                */
#include "Monitor_Control.h" /* I2C1 / UI2C1 master driver for INA3221 sensor reading */
#include "NuMicro.h" /* Nuvoton M251 CMSIS / BSP header                       */
#include "device.h"  /* Board-level GPIO, pin, and clock definitions          */
#include "eeprom_sim.h" /* Flash-backed EEPROM emulation driver                  */
#include "i2c_eeprom_sim.h" /* I2C register-map definitions (eeprom_ram layout)      */
#include "nsp_PlaySample.h" /* NSP23 audio clip index definitions                    */
#include "nsp_driver.h" /* NSP23 bell-sound IC driver; nsp23_id = 0xACCCCAAA     */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "firmware_version.h"

// --- Macros ---
#define INA_CHANNEL_COUNT 6
#define CURRENT_MA_TO_10MA(value) ((value) / 10)

/* NP23_pid is read by I2C1_Init(); equals nsp23_id when the NSP23 IC is present
 */
extern volatile unsigned int NP23_pid;
//#define FW_version 0x01        /* Firmware version number                     */
#define FLASH_END_ADDR 0x10000 /* End of 64 KB Flash address space */
#define EEPROM_BLOCK_SIZE                                                      \
  (5 * 1024)            /* Each emulated EEPROM partition = 5 KB       */
#define EEPROM_PAGES 10 /* Number of Flash pages per partition         */

/* eeprom_user  : user settings (thresholds, calibration, asset info) at 0xEC00
 */
#define EEPROM_2_BASE (FLASH_END_ADDR - EEPROM_BLOCK_SIZE) // 0xEC00
/* eeprom_system: event log ring buffer at 0xD800 */
#define EEPROM_1_BASE (EEPROM_2_BASE - EEPROM_BLOCK_SIZE) // 0xD800

EEPROM_Ctx_T eeprom_system; /* Event-log EEPROM partition (eeprom_1) */
EEPROM_Ctx_T eeprom_user;   /* User-settings EEPROM partition (eeprom_2) */

/* 256-byte I2C slave register map; host reads/writes this buffer via I2C0 */
extern uint8_t volatile eeprom_ram[256];

/* --- Protection Logic Globals ------------------------------------------------
 */
static enum SystemState g_system_state =
    STATE_NORMAL; /* Fault state machine: NORMAL / WARNING_COUNTDOWN / LATCHED
                   */
static uint8_t g_u8ImbalanceDebounceCounter =
    0; /* Current-imbalance fault debounce counter                  */
static uint8_t g_u8OcDebounceCounter =
    0; /* Overcurrent (OC) fault debounce counter                   */
static uint8_t g_u8UcDebounceCounter =
    0; /* Undercurrent (UC) debounce counter (reserved)             */
                  
volatile uint32_t g_u32WarningCountdownMs =
    0; /* Warning countdown timer in ms; decremented by SysTick     */
static uint8_t g_bLogSavedForLatch =
    0; /* Guards against saving the event log more than once        */
static volatile uint8_t g_u8HwWarningTriggered =
    0; /* Set by GPF_IRQHandler when INA_WARNING pin fires          */
static volatile uint8_t g_u8NP23PlayFlag =
    0; /* Set by SysTick in Phase 1; main loop calls N_PLAY(1)      */
static volatile uint8_t g_u8NP23Play2Flag =
    0; /* Set by SysTick in Phase 2; main loop calls N_PLAY(2)      */
static uint8_t g_u8ProtectionChecked =
    0; /* Allows only one protection evaluation per I2C data cycle  */
volatile uint32_t g_u32McuRunTimeSeconds =
    0; /* MCU uptime in seconds; mirrored to eeprom_ram             */
static volatile uint16_t g_u16McuRunTimeMillis =
    0; /* Millisecond sub-counter; rolls over into seconds at 1000  */
/* I2C command flags: set by I2C0_IRQHandler, cleared and acted upon in
 * Event_Log_Handler */
extern uint8_t volatile u8EVEN_INDEX_FLAG; /* Host requested a log entry read */
extern uint8_t volatile u8UPITFlag; /* UPIT cmd: update imbalance threshold */
extern volatile uint8_t u8UPCDFlag; /* UPCD cmd: update latch countdown time */
extern volatile uint8_t
    u8UPDCFlag; /* SWDC cmd: update software debounce count  */
extern volatile uint8_t
    u8UPCAFlag; /* UPCA cmd: update channel calibration data */
extern volatile uint8_t u8UPMFFlag; /* UPMF cmd: update manufacturing date */
extern volatile uint8_t u8UPLTFlag; /* UPLT cmd: update Lot ID */
extern volatile uint8_t u8UPSNFlag; /* UPSN cmd: update serial number */
extern volatile uint8_t u8UPOCFlag; /* UPOC cmd: update overcurrent threshold */
 extern volatile uint8_t u8UPUCFlag;      /* UPUC cmd: update undercurrent
// threshold (reserved) */

/* Asset information buffers; loaded from EEPROM and mirrored to eeprom_ram for
 * host access */
uint8_t g_au8SerialNumber[EEPROM_SERIAL_NUMBER_SIZE] = {
    0};                                           /* Product serial number */
uint8_t g_au8LotID[EEPROM_LOT_ID_SIZE] = {0};     /* Manufacturing lot ID  */
uint8_t g_au8MFGDate[EEPROM_MFG_DATE_SIZE] = {0}; /* Manufacturing date    */

/* Application configuration: protection thresholds, countdown, power-on count,
 * etc. */
AppConfig_T g_AppConfig;
/* Per-channel calibration data: gain and offset coefficients */
CalibData_T g_CalibData;

/*---------------------------------------------------------------------------------------------------------*/
/*  SYS_Init */
/*  Enables the HIRC internal oscillator and sets the core clock (HCLK) to
 * HCLK_CLK MHz (48 MHz).         */
/*---------------------------------------------------------------------------------------------------------*/
void SYS_Init(void) {
  SYS_UnlockReg(); /* Unlock write-protected registers before clock
                      configuration */
  /*---------------------------------------------------------------------------------------------------------*/
  /* Init System Clock */
  /*---------------------------------------------------------------------------------------------------------*/
  /* Enable HIRC clock */
  CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);

  /* Wait for HIRC clock ready */
  CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

  /* Set core clock to HCLK_CLK MHz */
  CLK_SetCoreClock(HCLK_CLK);

  /* Update the Variable SystemCoreClock */
  SystemCoreClockUpdate();
  SYS_LockReg(); /* Re-lock write-protected registers */
}

/*---------------------------------------------------------------------------------------------------------*/
/*  SysTick_Initial */
/*  Configures the SysTick timer to fire every 1 ms (HCLK / 1000 ticks). */
/*  The 1 ms ISR drives the warning LED/buzzer timing, the MCU uptime counter,
 */
/*  and the periodic INA3221 data-read flag (u8MonitorFlag). */
/*---------------------------------------------------------------------------------------------------------*/
void SysTick_Initial(void) {
  /* Clock source: HCLK (48 MHz) */
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk;

  /* Reload value for 1 ms period */
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

/*---------------------------------------------------------------------------------------------------------*/
/*  SysTick_Handler  (fires every 1 ms) */
/*  Responsibilities: */
/*    1. Drives the three-phase protection warning sequence (Phase 1 -> Phase 2
 * -> Latch):                 */
/*         - Non-NSP23 boards: directly toggles LED and buzzer GPIO pins. */
/*         - NSP23 boards    : sets a flag; main loop calls N_PLAY() when I2C1
 * is idle.                    */
/*    2. Sets u8MonitorFlag every TIMER_MONITOR_UPDATE ms to trigger INA3221
 * reads.                        */
/*    3. Maintains the MCU uptime counter and syncs it to eeprom_ram. */
/*  NOTE: Do NOT call any I2C-dependent function (e.g. N_PLAY) directly from
 * this ISR.                     */
/*---------------------------------------------------------------------------------------------------------*/
void SysTick_Handler(void) {
  /* Writing any value to VAL clears the counter and COUNTFLAG */
  SysTick->VAL = 0;

  /* Decrement countdown timer */
  if (g_u32WarningCountdownMs > 0) {
    g_u32WarningCountdownMs--; /* Decrement 1 ms each tick */
    
    if (g_u32WarningCountdownMs == 0) {
      /* Countdown expired -> latch system */
      if (g_system_state == STATE_WARNING_COUNTDOWN)
        g_system_state = STATE_LATCHED;
    }
  }

  /* Get monitor data */
  TimeCounterMonitorUpdate++;
#if ((USE_MONITOR_0 == TRUE) | (USE_MONITOR_1 == TRUE))
  if (TimeCounterMonitorUpdate >= TIMER_MONITOR_UPDATE) {
    u8MonitorFlag = 1;
    TimeCounterMonitorUpdate -= TIMER_MONITOR_UPDATE;
  }
#endif

  // --- MCU Runtime Counter ---
  // Counts milliseconds; wraps to seconds and writes to eeprom_ram for host read-back.
  g_u16McuRunTimeMillis++;

  if (g_u16McuRunTimeMillis >= 1000) {
    g_u16McuRunTimeMillis = 0;
    g_u32McuRunTimeSeconds++;
    
    /* Safely write to eeprom_ram */
    *((volatile uint32_t *)&eeprom_ram[I2C_REG_OFFSET_MCU_RUN_TIME]) = g_u32McuRunTimeSeconds;
  }
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Warning_Indication_Task (called in main loop) */
/*  Drives the three-phase protection warning sequence: Phase 1 -> Phase 2 -> Latch */
/*---------------------------------------------------------------------------------------------------------*/
void Warning_Indication_Task(void) {
  /* --- Three-phase warning sequence: Phase1(0-20s) -> Phase2(20s-3min) -> Latch --- */
  if (g_u32WarningCountdownMs > 0) {
    if ((LATCH_COUNTDOWN_MS - g_u32WarningCountdownMs) < BUZZER_DELAY_MS) {
      /* Phase 1 (0 - 20 s): LED 1 Hz blink, Buzzer silent */
      if (((LATCH_COUNTDOWN_MS - g_u32WarningCountdownMs) % 1000) < 500)
        LED_ALARM_PORT->DOUT |= LED_ALARM_PIN;
      else
        LED_ALARM_PORT->DOUT &= ~LED_ALARM_PIN;

      if (NP23_pid != nsp23_id) {
        BUZZER_PORT->DOUT &= ~BUZZER_PIN;
      } else {
        /* NSP23 present: request N_PLAY(1) from main context (once per event) */
        if (!g_u8NP23PlayFlag) g_u8NP23PlayFlag = 1;
      }
    } else {
      /* Phase 2 (20 s - 3 min): LED + Buzzer 2 Hz blink */
      if (((LATCH_COUNTDOWN_MS - g_u32WarningCountdownMs) % 500) < 250) {
        LED_ALARM_PORT->DOUT |= LED_ALARM_PIN;

        if (NP23_pid != nsp23_id) {
          BUZZER_PORT->DOUT |= BUZZER_PIN;
        } else {
          /* NSP23 present: request N_PLAY(2) from main context (once per event) */
          if (!g_u8NP23Play2Flag) g_u8NP23Play2Flag = 1;
        }
      } else {
        LED_ALARM_PORT->DOUT &= ~LED_ALARM_PIN;

        if (NP23_pid != nsp23_id) {
          BUZZER_PORT->DOUT &= ~BUZZER_PIN;
        }
      }
    }
  } else if (g_system_state == STATE_LATCHED) {
    /* After latch: LED and Buzzer continue blinking at 2 Hz */
    if ((g_u16McuRunTimeMillis % 500) < 250) {
      LED_ALARM_PORT->DOUT |= LED_ALARM_PIN;

      if (NP23_pid != nsp23_id) {
        BUZZER_PORT->DOUT |= BUZZER_PIN;
      }else {
      if (!g_u8NP23Play2Flag) g_u8NP23Play2Flag = 1;
      }
    } else {
      LED_ALARM_PORT->DOUT &= ~LED_ALARM_PIN;

      if (NP23_pid != nsp23_id) {
        BUZZER_PORT->DOUT &= ~BUZZER_PIN;
      }
    }
  }else {
    // LED_ALARM_PORT->DOUT &= ~LED_ALARM_PIN; /* All clear */
    // BUZZER_PORT->DOUT    &= ~BUZZER_PIN;
  }
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Event Log Data Structure */
/*  Each entry is 22 bytes; 11 entries fit in the 256-byte system EEPROM (11 *
 * 22 = 242 bytes).            */
/*  Entries are stored as a ring buffer; LogHead points to the next write slot.
 */
/*---------------------------------------------------------------------------------------------------------*/
// NOTE: The LogEntry_T size is 22 bytes. To fit within a 256-byte log area,
// the number of entries is limited to 11 (11 * 22 = 242 bytes).
// This differs from the 12 entries mentioned in the readme.
#define MAX_LOG_ENTRIES 11
typedef struct {
  uint32_t u32PowerOnCount; // 0x00 : power-on cycle count
  uint32_t u32RunTime;      // 0x04 : MCU uptime at fault (seconds)
  uint16_t u16Current[6];   // 0x08 : Ch1-Ch6 currents (unit: 10 mA/LSB)
  uint8_t u8ErrorCode;      // 0x14 : fault status bits at time of log
  uint8_t u8Checksum;       // 0x15 : 8-bit sum of all preceding bytes
} LogEntry_T;               // Total size: 22 bytes

/*---------------------------------------------------------------------------------------------------------*/
/*  Save_Log_Entry */
/*  Captures a snapshot of the current fault state and writes it to the system
 * EEPROM ring buffer.         */
/*  Increments and wraps LogHead after each write. */
/*---------------------------------------------------------------------------------------------------------*/
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
  // Checksum is calculated over the entire struct except the last byte (the
  // checksum itself)
  for (i = 0; i < sizeof(LogEntry_T) - 1; i++) {
    checksum += pData[i];
  }

  logEntry.u8Checksum = checksum;

  // --- 3. Write Log to EEPROM (eeprom_system) ---
  // Get the current Log Head (next entry to be written) from our app config
  logHead = g_AppConfig.u8LogHead;

  if (logHead >= MAX_LOG_ENTRIES) // Sanity check
  {
    logHead = 0;
  }

  logAddress = logHead *
               sizeof(LogEntry_T); // Calculate address offset in the log eeprom

  // Write the new log entry to the system EEPROM (for logs)
  Write_EEPROM(&eeprom_system, logAddress, (uint8_t *)&logEntry,
               sizeof(LogEntry_T));

  // --- 4. Update and Save Log Head ---
  // Increment and wrap around the log head for the ring buffer
  logHead++;

  if (logHead >= MAX_LOG_ENTRIES) {
    logHead = 0;
  }

  g_AppConfig.u8LogHead = logHead;

  // Save the updated log head to the user EEPROM (for configuration)
  Write_EEPROM(&eeprom_user, EE_OFFSET_LOG_HEAD, &g_AppConfig.u8LogHead,
               sizeof(g_AppConfig.u8LogHead));

  // Also update the value in the I2C RAM map for host to read
  eeprom_ram[EE_OFFSET_LOG_HEAD] = g_AppConfig.u8LogHead;
}

/*---------------------------------------------------------------------------------------------------------*/
/*  GPF_IRQHandler  (Port F GPIO interrupt) */
/*  Triggered by a falling edge on INA_WARNING (PF.2) when the INA3221 asserts
 * its ALERT output.          */
/*  Sets g_u8HwWarningTriggered so Protection_Handler can immediately save a log
 * and assert PGOOD.         */
/*---------------------------------------------------------------------------------------------------------*/
void GPF_IRQHandler(void) {
  /* Check if the INA_WARNING pin triggered this interrupt */
  if (GPIO_GET_INT_FLAG(INA_WARNING_PORT, INA_WARNING_PIN)) {
    GPIO_CLR_INT_FLAG(
        INA_WARNING_PORT,
        INA_WARNING_PIN); /* Clear flag to allow future interrupts */
    g_u8HwWarningTriggered =
        1; /* Signal main loop to handle the hardware warning */
  }

  /* Additional GPF pin interrupts can be handled here */
}

/*---------------------------------------------------------------------------------------------------------*/
/*  ProcessHardFault */
/*  Called when a CPU HardFault exception occurs. Resets the chip to recover
 * automatically.                */
/*  TODO: Log fault context to EEPROM before reset for post-mortem analysis. */
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ProcessHardFault(uint32_t lr, uint32_t msp, uint32_t psp) {
  /* HardFault occurred — force a chip reset to recover */
  while (1) {
    SYS_UnlockReg();
    SYS_ResetChip();
  }

  return lr;
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Init_App_Setting_U32 / Init_App_Setting_U8 */
/*  Reads a setting from EEPROM. If the value is uninitialized (0xFFFF.../0x00),
 * writes the default.      */
/*  Parameters: */
/*    ctx         - Target EEPROM partition */
/*    offset      - Byte offset within the partition */
/*    value       - Pointer to the destination variable (output) */
/*    default_val - Value to write and use when the slot is uninitialized */
/*---------------------------------------------------------------------------------------------------------*/
// Helper function to initialize a 32-bit setting from EEPROM
static void Init_App_Setting_U32(EEPROM_Ctx_T *ctx, uint32_t offset,
                                 uint32_t *value, uint32_t default_val) {
  Read_EEPROM(ctx, offset, (uint8_t *)value, sizeof(uint32_t));

  /* 0xFFFFFFFF = erased Flash; 0 = invalid — both fall back to default */
  if (*value == 0xFFFFFFFF || *value == 0) {
    *value = default_val;
    Write_EEPROM(ctx, offset, (uint8_t *)value, sizeof(uint32_t));
  }
}

// Helper function to initialize an 8-bit setting from EEPROM
static void Init_App_Setting_U8(EEPROM_Ctx_T *ctx, uint32_t offset,
                                uint8_t *value, uint8_t default_val) {
  Read_EEPROM(ctx, offset, value, sizeof(uint8_t));

  /* 0xFF = erased Flash — fall back to default */
  if (*value == 0xFF) {
    *value = default_val;
    Write_EEPROM(ctx, offset, value, sizeof(uint8_t));
  }
}

/*---------------------------------------------------------------------------------------------------------*/
/*  initial_eeprom_ram */
/*  Pre-populates fixed fields of the I2C slave register map (eeprom_ram) with
 * the firmware version        */
/*  number and the board model string "M251_GPU_CP". The host reads these at
 * 0xF0 onwards.                 */
/*---------------------------------------------------------------------------------------------------------*/
void initial_eeprom_ram(void) {
  eeprom_ram[0xf0] = g_firmware_header.version_major; /* Firmware version (0x01) */
  /* Board model string "M251_GPU_CP" at 0xF1~0xFB */
  memcpy((void *)&eeprom_ram[0xf1], g_firmware_header.board_model, 11);
}
/*---------------------------------------------------------------------------------------------------------*/
/*  Function Prototypes */
/*---------------------------------------------------------------------------------------------------------*/
void Peripherals_Init(void);
void Config_Load(void);
void Protection_Handler(void);
void ISP_Handler(void);
void Event_Log_Handler(void);
void Warning_Indication_Task(void);

/*---------------------------------------------------------------------------------------------------------*/
/*  Hardware and Peripheral Initialization */
/*---------------------------------------------------------------------------------------------------------*/
void Peripherals_Init(void) {
  initial_eeprom_ram(); /* Populate fixed fields of the I2C register map */
                        /* Unlock protected registers */
  SYS_UnlockReg();
  /* Enable GPIO port clocks */
  CLK_EnableModuleClock(GPC_MODULE);
  CLK_EnableModuleClock(GPA_MODULE);
  CLK_EnableModuleClock(GPB_MODULE);
  CLK_EnableModuleClock(GPF_MODULE);
  /* Lock protected registers */
  SYS_LockReg();
  /* Set safe default output levels before configuring GPIO modes */
  //PS_PGOOD_PORT->DOUT &=~PS_PGOOD_PIN; // PS_PGOOD low  = normal (not in protection)
  LED_ALARM_PORT->DOUT &= ~LED_ALARM_PIN; // Alarm LED off initially
  BUZZER_PORT->DOUT &= ~BUZZER_PIN;       // Buzzer off initially
  NSP23_RST_PORT->DOUT |=
      NSP23_RST_PIN; // NSP23 RST high = device running normally

  /* Configure protection-related GPIO pins as outputs */
  GPIO_SetMode(LED_ALARM_PORT, LED_ALARM_PIN, GPIO_MODE_OUTPUT);
  GPIO_SetMode(BUZZER_PORT, BUZZER_PIN, GPIO_MODE_OUTPUT);
  //GPIO_SetMode(PS_PGOOD_PORT, PS_PGOOD_PIN, GPIO_MODE_OUTPUT);
  GPIO_SetMode(NSP23_RST_PORT, NSP23_RST_PIN, GPIO_MODE_OUTPUT);

  /* Configure INA_WARNING (PF.2) as input with falling-edge interrupt */
  GPIO_SetMode(INA_WARNING_PORT, INA_WARNING_PIN, GPIO_MODE_INPUT);
  GPIO_EnableInt(INA_WARNING_PORT, INA_WARNING_PIN, GPIO_INT_FALLING);
  INA_WARNING_PORT->INTEN = 0x4; // fix pf2 inerrupt error
  NVIC_EnableIRQ(GPF_IRQn);      // GPF interrupt for INA_WARNING
  NVIC_SetPriority(GPF_IRQn,
                   INT_PRIORITY_HIGH); // High priority — must not be delayed
  GPIO_SET_DEBOUNCE_TIME(GPIO_DBCTL_DBCLKSRC_HCLK,
                         GPIO_DBCTL_DBCLKSEL_1024); // ~21 us debounce
  GPIO_ENABLE_DEBOUNCE(INA_WARNING_PORT, INA_WARNING_PIN);

  /* I2C0 slave — handles host communication (register map at eeprom_ram, addr
   * 0x4D) */
  I2C0_Init();

  /* I2C1 master — reads INA_A (Ch1-3, addr 0x40); also detects NSP23 */
  I2C1_Init();

  /* UI2C1 master — reads INA_B (Ch4-6, addr 0x40) */
  UI2C1_Init();

  /* Configure SysTick for 1 ms period interrupt */
  SysTick_Initial();

  /* Start SysTick counting */
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Load Configuration from EEPROM */
/*---------------------------------------------------------------------------------------------------------*/
void Config_Load(void) {
  uint32_t ret;
  /* Init EEPROM content */
  // Init_EEPROM_Content();
  ret = Init_EEPROM(&eeprom_system, EEPROM_1_BASE, 255, EEPROM_PAGES);
  // Assert_Test("Init Sys", ret == 0);

  ret = Init_EEPROM(&eeprom_user, EEPROM_2_BASE, 255, EEPROM_PAGES);
  // Assert_Test("Init User", ret == 0);

  /* --- Initialize Application Settings from EEPROM --- */

  // Read and update Power On Count
  Read_EEPROM(&eeprom_user, EE_OFFSET_POWER_ON_COUNT,
              (uint8_t *)&g_AppConfig.u32PowerOnCount, sizeof(uint32_t));

  if (g_AppConfig.u32PowerOnCount == 0xFFFFFFFF) {
    g_AppConfig.u32PowerOnCount = 0;
  }

  g_AppConfig.u32PowerOnCount++;
  Write_EEPROM(&eeprom_user, EE_OFFSET_POWER_ON_COUNT,
               (uint8_t *)&g_AppConfig.u32PowerOnCount, sizeof(uint32_t));

  // Read and initialize other settings with defaults if necessary
  Init_App_Setting_U32(&eeprom_user, EE_OFFSET_IMBALANCE_THRESHOLD,
                       &g_AppConfig.u32IMBALANCE_THRESHOLD,
                       DEFAULT_IMBALANCE_THRESHOLD);
  Init_App_Setting_U32(&eeprom_user, EE_OFFSET_COUNTDOWN,
                       &g_AppConfig.u32Countdown, DEFAULT_COUNTDOWN_MS);
  Init_App_Setting_U8(&eeprom_user, EE_OFFSET_SW_DEBOUNCE,
                      &g_AppConfig.u8swdebounce, DEFAULT_SW_DEBOUNCE);
  Init_App_Setting_U8(&eeprom_user, EE_OFFSET_LOG_HEAD, &g_AppConfig.u8LogHead,
                      DEFAULT_LOG_HEAD);
  Init_App_Setting_U32(&eeprom_user, EE_OFFSET_OC_THRESHOLD,
                       &g_AppConfig.u32OcThreshold, DEFAULT_OC_THRESHOLD);
   Init_App_Setting_U32(&eeprom_user, EE_OFFSET_UC_THRESHOLD,
   &g_AppConfig.u32UcThreshold, DEFAULT_UC_THRESHOLD);

	
	
  // --- Initialize Calibration Data from EEPROM ---
  Read_EEPROM(&eeprom_user, EE_OFFSET_CALIB_DATA, (uint8_t *)&g_CalibData,
              sizeof(CalibData_T));

  // Check if the first gain value is uninitialized (erased flash is 0xFFFF)
  if (g_CalibData.channels[0].u16Gain == 0xFFFF) {
    // Data is not initialized, so write default values

    for (int i = 0; i < MONITOR_MAX_CHANNEL * 2;
         i++) // Assuming 6 channels total (2 INA3221s * 3 channels)
    {
      g_CalibData.channels[i].u16Gain =
          0; // Default Gain (e.g., 1.0x represented as 1000)
      g_CalibData.channels[i].u16Offset = 0; // Default Offset
    }

    // Write the default values back to EEPROM for persistence
    Write_EEPROM(&eeprom_user, EE_OFFSET_CALIB_DATA, (uint8_t *)&g_CalibData,
                 sizeof(CalibData_T));
  }

  /* --- Read Serial Number, Lot ID, and MFG Date from EEPROM --- */
  Read_EEPROM(&eeprom_user, EE_OFFSET_SERIAL_NUMBER, g_au8SerialNumber,
              EEPROM_SERIAL_NUMBER_SIZE);

  Read_EEPROM(&eeprom_user, EE_OFFSET_LOT_ID, g_au8LotID, EEPROM_LOT_ID_SIZE);

  Read_EEPROM(&eeprom_user, EE_OFFSET_MFG_DATE, g_au8MFGDate,
              EEPROM_MFG_DATE_SIZE);
  /* --- Synchronize settings to RAM for I2C reporting --- */
  // Do NOT use a single memcpy of the whole struct: struct padding would
  // corrupt adjacent registers. Copy each member individually to its defined
  // offset in the I2C register map.
  memcpy((void *)&eeprom_ram[EE_OFFSET_POWER_ON_COUNT],
         &g_AppConfig.u32PowerOnCount, sizeof(uint32_t));
  memcpy((void *)&eeprom_ram[EE_OFFSET_IMBALANCE_THRESHOLD],
         &g_AppConfig.u32IMBALANCE_THRESHOLD, sizeof(uint32_t));
  memcpy((void *)&eeprom_ram[EE_OFFSET_COUNTDOWN], &g_AppConfig.u32Countdown,
         sizeof(uint32_t));
  eeprom_ram[EE_OFFSET_SW_DEBOUNCE] = g_AppConfig.u8swdebounce;
  eeprom_ram[EE_OFFSET_LOG_HEAD] = g_AppConfig.u8LogHead;
  memcpy((void *)&eeprom_ram[EE_OFFSET_OC_THRESHOLD],
         &g_AppConfig.u32OcThreshold, sizeof(uint32_t));
  /* NOTE: UC threshold (EE_OFFSET_UC_THRESHOLD=0x2C) is NOT mirrored to eeprom_ram
     because 0x2C-0x2E is used by OC/Imbalance/UC alert bitmaps (I2C_REG_OC_ALERT_CH etc.) */
  /* Calibration data (per-channel gain/offset) */
  memcpy((void *)&eeprom_ram[EE_OFFSET_CALIB_DATA], &g_CalibData,
         sizeof(CalibData_T));

  /* --- Copy asset info to eeprom_ram so the host can query it over I2C0 --- */
  memcpy((void *)&eeprom_ram[I2C_REG_OFFSET_SERIAL_NUMBER], g_au8SerialNumber,
         EEPROM_SERIAL_NUMBER_SIZE);
  memcpy((void *)&eeprom_ram[I2C_REG_OFFSET_LOT_ID], g_au8LotID,
         EEPROM_LOT_ID_SIZE);
  memcpy((void *)&eeprom_ram[I2C_REG_OFFSET_MFG_DATE], g_au8MFGDate,
         EEPROM_MFG_DATE_SIZE);
}
extern volatile uint8_t g_u8GetEndFlag_1;
extern volatile uint8_t g_u8GetEndFlag_0;
/*---------------------------------------------------------------------------------------------------------*/
/*  Core Protection Logic Handler */
/*---------------------------------------------------------------------------------------------------------*/
void Protection_Handler(void) {
  /* u8MonitorFlag is set by SysTick every TIMER_MONITOR_UPDATE ms. */
  /* Kick off a new I2C sensor read as soon as the flag is seen, then reset */
  /* g_u8ProtectionChecked so the protection logic runs exactly once per data
   * cycle.  */
  if (u8MonitorFlag == 1) {
    u8MonitorFlag = 0;
    g_u8ProtectionChecked =
        0; /* Allow one protection evaluation for this new data */
#if (USE_MONITOR_0 == TRUE)
    Read_Monitor_Data_0(); /* I2C1  master: read INA_A channels 1-3 */
#endif
#if (USE_MONITOR_1 == TRUE)
    Read_Monitor_Data_1(); /* UI2C1 master: read INA_B channels 4-6 */
#endif
  }

  /* Run protection logic only ONCE per I2C data cycle to correctly time the
   * debounce. */
  /* g_u8ProtectionChecked is cleared by u8MonitorFlag every
   * TIMER_MONITOR_UPDATE ms.  */
  if ((g_u8GetEndFlag_1 == 1) && (g_u8GetEndFlag_0 == 1) &&
      (g_u8ProtectionChecked == 0)) {
    g_u8ProtectionChecked = 1; /* Mark as processed for this cycle */

    //__disable_irq();
    /* --- Protection 1: Current Imbalance Check --- */
    if (g_system_state != STATE_LATCHED) {

      uint16_t currents[6];
      uint16_t max_current = 0;
      uint16_t min_current = 0xFFFF;
      int i;

      // 1. Read all 6 channel currents from eeprom_ram (unit: 10mA)
      __disable_irq();
      for (i = 0; i < INA_CHANNEL_COUNT; i++) {
        uint16_t offset = I2C_REG_MONITOR_DATA_OFFSET + (i * 4);
        currents[i] = (eeprom_ram[offset + 1] << 8) | eeprom_ram[offset];
      }
      __enable_irq();

      // 2. Find max and min current
      for (i = 0; i < INA_CHANNEL_COUNT; i++) {
        if (currents[i] > max_current)
          max_current = currents[i];

        if (currents[i] < min_current)
          min_current = currents[i];
      }

      // if((min_current!=0)&&(max_current!=0))
      // if(min_current!=0)
      {
        // 3. Imbalance debounce (Threshold in mA, currents in 10mA)
        if ((max_current - min_current) > CURRENT_MA_TO_10MA(g_AppConfig.u32IMBALANCE_THRESHOLD)) {
          if (g_u8ImbalanceDebounceCounter < IMBALANCE_DEBOUNCE_COUNT)
            g_u8ImbalanceDebounceCounter++;
        } else {
          g_u8ImbalanceDebounceCounter = 0;
        }

        // 4. Overcurrent debounce (any channel > OC threshold)
        if (max_current > CURRENT_MA_TO_10MA(g_AppConfig.u32OcThreshold)) {
          if (g_u8OcDebounceCounter < OVERCURRENT_DEBOUNCE_COUNT)
            g_u8OcDebounceCounter++;
        } else {
          g_u8OcDebounceCounter = 0;
        }


#if 1

                // 4b. Undercurrent debounce (any channel < UC threshold)
                if (min_current <= (g_AppConfig.u32UcThreshold / 10))
                {
                    if (g_u8UcDebounceCounter < UNDERCURRENT_DEBOUNCE_COUNT)
                        g_u8UcDebounceCounter++;
                }
                else
                {
                    g_u8UcDebounceCounter = 0;
                }

#endif
      }

      // 5. Update I2C status bits independently
      __disable_irq();
      if (g_u8ImbalanceDebounceCounter >= IMBALANCE_DEBOUNCE_COUNT)
        eeprom_ram[I2C_REG_STATUS_OFFSET] |= STATUS_BIT_IMBALANCE;
      else
        eeprom_ram[I2C_REG_STATUS_OFFSET] &= ~STATUS_BIT_IMBALANCE;

      if (g_u8OcDebounceCounter >= OVERCURRENT_DEBOUNCE_COUNT)
        eeprom_ram[I2C_REG_STATUS_OFFSET] |= STATUS_BIT_OVERCURRENT;
      else
        eeprom_ram[I2C_REG_STATUS_OFFSET] &= ~STATUS_BIT_OVERCURRENT;
      __enable_irq();

#if 1

            if (g_u8UcDebounceCounter >= UNDERCURRENT_DEBOUNCE_COUNT)
                eeprom_ram[I2C_REG_STATUS_OFFSET] |=  STATUS_BIT_UNDERCURRENT;
            else
                eeprom_ram[I2C_REG_STATUS_OFFSET] &= ~STATUS_BIT_UNDERCURRENT;

#endif
      // 6. Update per-channel alert registers
      //    OC_ALERT_CH        (0x2C): bit i set when currents[i] > OC threshold
      //    IMBALANCE_ALERT_CH (0x2D): bit i set when channel deviates from mean > imbalance_thresh/2
      //    UC_ALERT_CH        (0x2E): bit i set when currents[i] < UC threshold
      {
        uint8_t oc_ch_mask = 0;
        uint8_t imb_ch_mask = 0;
        uint8_t uc_ch_mask = 0;
        uint32_t mean = 0;
        uint16_t half_thresh = (uint16_t)(g_AppConfig.u32IMBALANCE_THRESHOLD /
                                          20); /* half thresh in 10mA units */
        uint16_t uc_thresh_10ma = (uint16_t)(g_AppConfig.u32UcThreshold / 10); /* UC threshold in 10mA units */
        int32_t diff;

        for (i = 0; i < INA_CHANNEL_COUNT; i++) {
          if (currents[i] > CURRENT_MA_TO_10MA(g_AppConfig.u32OcThreshold))
            oc_ch_mask |= (uint8_t)(1 << i);
          if (currents[i] < uc_thresh_10ma)
            uc_ch_mask |= (uint8_t)(1 << i);
          mean += currents[i];
        }

        mean /= INA_CHANNEL_COUNT;

        for (i = 0; i < INA_CHANNEL_COUNT; i++) {
          diff = (int32_t)currents[i] - (int32_t)mean;
          if (diff < 0)
            diff = -diff;
          if (diff > (int32_t)half_thresh)
            imb_ch_mask |= (uint8_t)(1 << i);
        }

        __disable_irq();
        eeprom_ram[I2C_REG_OC_ALERT_CH] = oc_ch_mask;
        eeprom_ram[I2C_REG_IMBALANCE_ALERT_CH] = imb_ch_mask;
        eeprom_ram[I2C_REG_UC_ALERT_CH] = uc_ch_mask;
        __enable_irq();
      }

      // 7. Unified fault state machine
      // Either fault (OC or Imbalance) triggers the same sequence:
      //   T+ 0 s  : LED 1 Hz blink (no buzzer)
      //   T+20 s  : LED + Buzzer 2 Hz blink
      //   T+ 3 min: PS_PGOOD asserted (system latched, requires power cycle)
      {
        uint8_t bAnyFaultActive =
            (g_u8ImbalanceDebounceCounter >= IMBALANCE_DEBOUNCE_COUNT) ||
            (g_u8OcDebounceCounter >= OVERCURRENT_DEBOUNCE_COUNT)||
            (g_u8UcDebounceCounter  >= UNDERCURRENT_DEBOUNCE_COUNT);

        if (bAnyFaultActive) {
          if (g_system_state == STATE_NORMAL) {
            /* Fault confirmed — start unified 3-minute countdown */
            g_system_state = STATE_WARNING_COUNTDOWN;
            g_u32WarningCountdownMs = LATCH_COUNTDOWN_MS; /* 180 000 ms */
            g_bLogSavedForLatch = 0;
          }

          /* If already in WARNING_COUNTDOWN, let countdown run */
        } else {
          if (g_system_state == STATE_WARNING_COUNTDOWN) {
            /* All faults cleared before latch — return to normal */
            g_system_state = STATE_NORMAL;
            g_u32WarningCountdownMs = 0;
            LED_ALARM_PORT->DOUT &= ~LED_ALARM_PIN;
            BUZZER_PORT->DOUT &= ~BUZZER_PIN;
          }
        }
      }
    }

    /* --- Latch: save log once and assert PGOOD --- */
    if (g_system_state == STATE_LATCHED) {
      if (g_bLogSavedForLatch == 0) {
        Save_Log_Entry();
        g_bLogSavedForLatch = 1;
      }

     // PS_PGOOD_PORT->DOUT |=PS_PGOOD_PIN; /* Assert PGOOD active high (protection active) */
    }

    //__enable_irq();
  }

  /* --- Protection 2: HW Warning Check (triggered by GPIO interrupt) --- */
  if (g_u8HwWarningTriggered) {
    if (g_bLogSavedForLatch == 0) {
      // Immediately read the latest sensor data to get a complete snapshot
      // Read_Monitor_Data_0();
      // Read_Monitor_Data_1();
      //g_system_state = STATE_WARN;
      // Now save the log with the fresh data
      Save_Log_Entry();
      g_bLogSavedForLatch = 1; // Ensure log is saved only once per latch event
      //PS_PGOOD_PORT->DOUT |=  PS_PGOOD_PIN; // Set PGOOD to high (Protection state)
    }
    g_u8HwWarningTriggered = 0; // Clear the trigger flag after handling
  }
}

/*---------------------------------------------------------------------------------------------------------*/
/*  ISP (Firmware Update) Handler */
/*---------------------------------------------------------------------------------------------------------*/
void ISP_Handler(void) {
  if (u8UpdateISPFlag == 1) {
    SYS_UnlockReg();        /* Unlock before touching FMC/SYS registers */
    outpw(&SYS->RSTSTS, 3); /* Clear reset status flags */

    FMC_Open();
    FMC_SetBootSource(1);       /* Next boot: LDROM (ISP bootloader) */
    NVIC->ICPR[0] = 0xFFFFFFFF; /* Clear all pending interrupts before reset */
    /* Remap interrupt vector table to LDROM so the bootloader starts correctly
     */
    FMC_SetVectorPageAddr(FMC_LDROM_BASE);
    SYS_ResetCPU(); /* Reset CPU; execution jumps to LDROM */

    while (1)
      ; /* Should never reach here */
  }
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Safe_Write_EEPROM                                                                                      */
/*  Reads the current value in EEPROM first. Only writes if the new value is different.                    */
/*---------------------------------------------------------------------------------------------------------*/
static void Safe_Write_EEPROM(EEPROM_Ctx_T *ctx, uint32_t offset, uint8_t *newData, uint32_t size) {
  uint8_t currentData[32]; // Max size needed is 24 bytes (CalibData_T)
  if (size > sizeof(currentData)) {
    Write_EEPROM(ctx, offset, newData, size); // Fallback for unexpectedly large writes
    return;
  }
  Read_EEPROM(ctx, offset, currentData, size);
  if (memcmp(currentData, newData, size) != 0) {
    Write_EEPROM(ctx, offset, newData, size);
  }
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Event Log Reading Handler */
/*---------------------------------------------------------------------------------------------------------*/
void Event_Log_Handler(void) {
  // Check if the host requested to read a log entry via I2C
  if (u8EVEN_INDEX_FLAG == 1) {
    u8EVEN_INDEX_FLAG = 0; // Clear the flag after processing

    // Read the requested log index from the I2C register map
    uint8_t event_index = eeprom_ram[I2C_REG_EVENT_LOG_INDEX];

    if (event_index < MAX_LOG_ENTRIES) {
      LogEntry_T log_entry;
      uint32_t log_address = event_index * sizeof(LogEntry_T);

      // Read the specified log entry from the system EEPROM
      Read_EEPROM(&eeprom_system, log_address, (uint8_t *)&log_entry,
                  sizeof(LogEntry_T));

      // Copy the log data to the I2C RAM map (20 bytes from 0x80 to 0x93)
      // This copies PowerOnCount, RunTime, and all 6 current channels.
      // It excludes the ErrorCode and Checksum fields.
      __disable_irq();
      memcpy((void *)&eeprom_ram[I2C_REG_EVENT_LOG_DATA], &log_entry, 20);
      __enable_irq();
    } else {
      // If the requested index is invalid, fill the data area with 0xFF
      __disable_irq();
      memset((void *)&eeprom_ram[I2C_REG_EVENT_LOG_DATA], 0xFF, 20);
      __enable_irq();
    }
  }

  /* --- Handle pending EEPROM write-back requests set by I2C0_IRQHandler --- */
  if (u8UPITFlag == 1) {
    u8UPITFlag = 0; /* Clear flag */
    /* UPIT: persist the new imbalance threshold to Flash EEPROM */
    Safe_Write_EEPROM(&eeprom_user, EE_OFFSET_IMBALANCE_THRESHOLD,
                 (uint8_t *)&g_AppConfig.u32IMBALANCE_THRESHOLD,
                 sizeof(uint32_t));
  }

  if (u8UPOCFlag == 1) {
    u8UPOCFlag = 0; /* Clear flag */
    /* UPOC: persist the new overcurrent threshold */
    Safe_Write_EEPROM(&eeprom_user, EE_OFFSET_OC_THRESHOLD,
                 (uint8_t *)&g_AppConfig.u32OcThreshold, sizeof(uint32_t));
  }

  if (u8UPUCFlag == 1)
  {
    u8UPUCFlag = 0;
    /* UPUC: persist the new undercurrent threshold */
    Safe_Write_EEPROM(&eeprom_user, EE_OFFSET_UC_THRESHOLD, (uint8_t *)&g_AppConfig.u32UcThreshold, sizeof(uint32_t));
  }

  if (u8UPCDFlag == 1) {
    u8UPCDFlag = 0; /* Clear flag */
    /* UPCD: persist the new latch countdown time */
    Safe_Write_EEPROM(&eeprom_user, EE_OFFSET_COUNTDOWN,
                 (uint8_t *)&g_AppConfig.u32Countdown, sizeof(uint32_t));
  }

  if (u8UPDCFlag == 1) {
    u8UPDCFlag = 0; /* Clear flag */
    /* SWDC: persist the new software debounce count */
    Safe_Write_EEPROM(&eeprom_user, EE_OFFSET_SW_DEBOUNCE, &g_AppConfig.u8swdebounce,
                 sizeof(uint8_t));
  }

  if (u8UPCAFlag == 1) {
    u8UPCAFlag = 0; /* Clear flag */
    /* UPCA: persist per-channel calibration data (gain/offset) */
    Safe_Write_EEPROM(&eeprom_user, EE_OFFSET_CALIB_DATA, (uint8_t *)&g_CalibData,
                 sizeof(CalibData_T));
  }

  if (u8UPMFFlag == 1) {
    u8UPMFFlag = 0; /* Clear flag */
    /* UPMF: persist manufacturing date */
    Safe_Write_EEPROM(&eeprom_user, EE_OFFSET_MFG_DATE, g_au8MFGDate,
                 EEPROM_MFG_DATE_SIZE);
  }

  if (u8UPLTFlag == 1) {
    u8UPLTFlag = 0; /* Clear flag */
    /* UPLT: persist lot ID */
    Safe_Write_EEPROM(&eeprom_user, EE_OFFSET_LOT_ID, g_au8LotID,
                 EEPROM_LOT_ID_SIZE);
  }

  if (u8UPSNFlag == 1) {
    u8UPSNFlag = 0; /* Clear flag */
    /* UPSN: persist serial number */
    Safe_Write_EEPROM(&eeprom_user, EE_OFFSET_SERIAL_NUMBER, g_au8SerialNumber,
                 EEPROM_SERIAL_NUMBER_SIZE);
  }
}
/*---------------------------------------------------------------------------------------------------------*/
/*  uart_init */
/*  Initialises UART0 at 115200 bps for debug output. Not called in normal
 * production flow.               */
/*---------------------------------------------------------------------------------------------------------*/
void uart_init(void) {
  SYS_UnlockReg();
  /* Enable UART0 module clock */
  CLK_EnableModuleClock(UART0_MODULE);

  /* Enable GPIO clock */
  CLK_EnableModuleClock(GPB_MODULE);
  /* Select UART clock source from HIRC */
  CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC,
                     CLK_CLKDIV0_UART0(1));

  /*---------------------------------------------------------------------------------------------------------*/
  /* Init I/O Multi-function */
  /*---------------------------------------------------------------------------------------------------------*/

  /* Reset IP */
  SYS_ResetModule(UART0_RST);
  /* Configure UART0 and set UART0 Baudrate */
  UART_Open(UART0, 115200);
  SYS_LockReg();
}
extern int eeprom_stress_test(EEPROM_Ctx_T *ctx);
/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void) {
  /* Unlock write-protected registers */
  SYS_UnlockReg();

  /* Init system and multi-funcition I/O */
  SYS_Init();

  /* Lock protected registers */

#if eeprom_mock_test
  Init_EEPROM(&eeprom_system, EEPROM_1_BASE, 255, EEPROM_PAGES);
  eeprom_stress_test(&eeprom_system);
#else
  /* Initialize hardware and load configuration */

  Peripherals_Init();
  Config_Load();

  /* Fix UC threshold to 50 mA regardless of EEPROM value */
  g_AppConfig.u32UcThreshold = DEFAULT_UC_THRESHOLD;

  /* Main application loop */
  while (1) {
    Protection_Handler();
    ISP_Handler();
    Event_Log_Handler();
    Warning_Indication_Task();

    /* NSP23: play bell sound in Phase 1 when I2C1 is not in interrupt action */
    if ((NP23_pid == nsp23_id) && g_u8NP23PlayFlag && g_u8GetEndFlag_0 == 1) {
      I2C_DisableInt(I2C1);
      NVIC_DisableIRQ(I2C1_IRQn);
      g_u8NP23PlayFlag = 0;

      if (I2C_AskStatus() == 0) // CHECK NSP32 IS BUSY OR NOT
        N_PLAY(1);

      I2C_EnableInt(I2C1);
      NVIC_EnableIRQ(I2C1_IRQn);
    }

    /* NSP23: play bell sound in Phase 2 when I2C1 is not in interrupt action */
    if ((NP23_pid == nsp23_id) && g_u8NP23Play2Flag && g_u8GetEndFlag_0 == 1) {
      I2C_DisableInt(I2C1);
      NVIC_DisableIRQ(I2C1_IRQn);
      g_u8NP23Play2Flag = 0;

      if (I2C_AskStatus() == 0) // CHECK NSP32 IS BUSY OR NOT
        N_PLAY(2);

      I2C_EnableInt(I2C1);
      NVIC_EnableIRQ(I2C1_IRQn);
    }
  }

#endif

  while (1)
    ;
}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
