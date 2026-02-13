#include "eeprom_sim.h"


// Corrected and organized EEPROM offsets
#define EE_OFFSET_POWER_ON_COUNT        0x00    // size: 4
#define EE_OFFSET_IMBALANCE_THRESHOLD   0x04    // size: 4
#define EE_OFFSET_COUNTDOWN             0x08    // size: 4
#define EE_OFFSET_SW_DEBOUNCE           0x0C    // size: 1
#define EE_OFFSET_LOG_HEAD              0x0D    // size: 1 (Original was 0x0B, which was a bug)

// Default configuration values
#define DEFAULT_IMBALANCE_THRESHOLD     1000
#define DEFAULT_COUNTDOWN_MS            500
#define DEFAULT_SW_DEBOUNCE             2
#define DEFAULT_LOG_HEAD                0
// Asset Information Sizes (as per readme.md EEPROM layout)
#define EEPROM_SERIAL_NUMBER_SIZE       16
#define EEPROM_LOT_ID_SIZE              16 // Corrected from 16
#define EEPROM_MFG_DATE_SIZE            16 // Corrected from 16

// EEPROM (persistent storage) offset for calibration data
#define EE_OFFSET_CALIB_DATA            0x10


// I2C Register Map Offsets for Asset Data (from readme.md)
#define I2C_REG_OFFSET_SERIAL_NUMBER    0xE0
#define I2C_REG_OFFSET_LOT_ID           0xD0
#define I2C_REG_OFFSET_MFG_DATE         0xC0
#define I2C_REG_OFFSET_MCU_RUN_TIME     0xB0 // MCU runtime in seconds


#define EE_OFFSET_SERIAL_NUMBER 0xe0
#define EE_OFFSET_LOT_ID 0xD0
#define EE_OFFSET_MFG_DATE 0xC0

// I2C Register Map Offsets (from readme.md)
#define I2C_REG_STATUS_OFFSET         0x0e // Status Register is at 0x0e
#define I2C_REG_MONITOR_DATA_OFFSET   0x30

// I2C Register for reading event logs (Host writes index, MCU provides data)
#define I2C_REG_EVENT_LOG_INDEX       0x0f // Host writes the desired log index (0 to MAX_LOG_ENTRIES-1) here
#define I2C_REG_EVENT_LOG_DATA        0x80 // MCU places the requested log data here (20 bytes from 0x80 to 0x93)

// Status Register Bits
#define STATUS_BIT_IMBALANCE (1 << 3)
#define STATUS_BIT_HW_WARNING (1 << 4)

// Buzzer Frequencies
#define BUZZER_PATTERN_OFF      0
#define BUZZER_PATTERN_1HZ      1
#define BUZZER_PATTERN_2HZ      2

// Imbalance Protection Parameters
#define IMBALANCE_DEBOUNCE_COUNT 3

// --- Protection Logic Definitions ---
// GPIO definitions for alarms (User needs to define these according to the schematic)
#define LED_ALARM_PORT PC
#define LED_ALARM_PIN  BIT2
#define BUZZER_PORT    PC
#define BUZZER_PIN     BIT3
#define INA_WARNING_PORT PC
#define INA_WARNING_PIN  BIT4
#define PS_PGOOD_PORT  PC    // PLEASE VERIFY: This pin is based on readme.md, please confirm with schematic
#define PS_PGOOD_PIN   BIT5  // PLEASE VERIFY: This pin is based on readme.md, please confirm with schematic

// System State Machine
enum SystemState
{
    STATE_NORMAL,
    STATE_WARNING_COUNTDOWN,
    STATE_LATCHED
};

/// Application configuration structure
typedef struct
{
    uint32_t u32PowerOnCount;
    uint32_t u32IMBALANCE_THRESHOLD;
    uint32_t u32Countdown;
    uint8_t  u8swdebounce;
    uint8_t  u8LogHead;
} AppConfig_T;



// --- Calibration Data Structures ---
// Based on readme.md: | 0x10 - 0x27 | Calib Data | 24 | 6 Channels x (Gain 2B + Offset 2B) |

// Represents calibration data for a single channel (Gain + Offset)
typedef struct
{
    uint16_t u16Gain;
    uint16_t u16Offset;
} CalibChannel_T;

// Represents the entire 24-byte calibration data block for all 6 channels
typedef struct
{
    CalibChannel_T channels[6];
} CalibData_T;