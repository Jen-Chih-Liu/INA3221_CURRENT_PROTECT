#include "NuMicro.h"
#include <string.h>
#include "nsp_driver.h"
#include "nsp_PlaySample.h"

// =====================================================================
// Porting Configuration (Modify according to your M031 hardware wiring)
// =====================================================================
#define NSP_I2C_PORT        I2C1    // I2C module used (Default: I2C0)
#define NSP_I2C_SLAVE_ADDR  0x30   // 7-bit I2C Slave Address of NSP voice chip. Modify based on hardware settings.
#define I2C_TIMEOUT_COUNT   0x10000 // Hardware level timeout counter to prevent I2C bus hang

// =====================================================================
// Global variables and macros integrated from nsp_driver.c and nsp_PlaySample.c
// =====================================================================
#define HOST_TIMEOUT     (10)       // Software level I2C timeout threshold (Unit depends on SysTick, usually 10ms)
#define I2C_BUF_SIZE     (520)	    // I2C read/write buffer size (Larger buffer needed for ISP update)
#define MULTI_PLAY_MAX   32         // Maximum supported audio count for multi-play
#define FLASH_PAGE_SIZE  512        // Flash page size (For ISP update)

// --- Variables from nsp_driver.c (Used by low-level driver) ---
UINT8 u8WriteBuffer[I2C_BUF_SIZE];  // I2C TX data buffer
UINT8 u8ReadBuffer[I2C_BUF_SIZE];   // I2C RX data buffer
UINT16 u16TimeoutCounter = 0;       // I2C communication timeout counter (Incremented in SysTick_Handler)
UINT16 u16CMD_TX_BYTE = 0;          // Records the number of bytes to transmit in a single transaction
UINT16 u16CMD_RX_BYTE = 0;          // Records the number of bytes to receive in a single transaction
UINT8 u8RX_ERROR_COUNT = 0;         // Records consecutive communication errors (Used for auto-reset mechanism)

// --- Variables from nsp_PlaySample.c (Used by application examples) ---
UINT32 u32NSP_ID = 0;               // Product ID
UINT32 u32NSP_FW_VERSION = 0;       // Firmware Version
UINT8  u8NSP_STATUS = 0;            // NSP Status Flag
UINT16 u16PlayListIndex = 0;        // Current playing audio index
UINT8  u8SP_VOL = 0;                // Volume level (0~128)
UINT8  u8IO_FLAG = 0;               // IO pin direction status
UINT8  u8IO_VALUE = 0;              // IO pin level value
UINT8  u8LVD_VALUE = 0;             // Low Voltage Detection (LVD) value
UINT16 u16MAX_INDEX = 0;            // Maximum supported audio index
UINT8  CHECKSUM_BIT = 0;            // Checksum correctness flag
UINT16 CHECKSUM_BYTES = 0;          // Checksum value
UINT16 ISP_CHECKSUM_BYTES = 0;      // Checksum value of ISP data
UINT32 g_sFlash;                    // Simulated Flash operation handler
// Simulated Flash data content (For testing)
UINT8  g_au8Buf[FLASH_PAGE_SIZE] = {
	0xFF,0xFE,0xFD,0xFC,0xFB,0xFA,0xF9,0xF8,0xF7,0xF6,0xF5,0xF4,0xF3,0xF2,0xF1,0xF0,
	0xEF,0xEE,0xED,0xEC,0xEB,0xEA,0xE9,0xE8,0xE7,0xE6,0xE5,0xE4,0xE3,0xE2,0xE1,0xE0,
	0xDF,0xDE,0xDD,0xDC,0xDB,0xDA,0xD9,0xD8,0xD7,0xD6,0xD5,0xD4,0xD3,0xD2,0xD1,0xD0,
	0xCF,0xCE,0xCD,0xCC,0xCB,0xCA,0xC9,0xC8,0xC7,0xC6,0xC5,0xC4,0xC3,0xC2,0xC1,0xC0,
	0xBF,0xBE,0xBD,0xBC,0xBB,0xBA,0xB9,0xB8,0xB7,0xB6,0xB5,0xB4,0xB3,0xB2,0xB1,0xB0,
	0xAF,0xAE,0xAD,0xAC,0xAB,0xAA,0xA9,0xA8,0xA7,0xA6,0xA5,0xA4,0xA3,0xA2,0xA1,0xA0,
	0x9F,0x9E,0x9D,0x9C,0x9B,0x9A,0x99,0x98,0x97,0x96,0x95,0x94,0x93,0x92,0x91,0x90,
	0x8F,0x8E,0x8D,0x8C,0x8B,0x8A,0x89,0x88,0x87,0x86,0x85,0x84,0x83,0x82,0x81,0x80,
	0x7F,0x7E,0x7D,0x7C,0x7B,0x7A,0x79,0x78,0x77,0x76,0x75,0x74,0x73,0x72,0x71,0x70,
	0x6F,0x6E,0x6D,0x6C,0x6B,0x6A,0x69,0x68,0x67,0x66,0x65,0x64,0x63,0x62,0x61,0x60,
	0x5F,0x5E,0x5D,0x5C,0x5B,0x5A,0x59,0x58,0x57,0x56,0x55,0x54,0x53,0x52,0x51,0x50,
	0x4F,0x4E,0x4D,0x4C,0x4B,0x4A,0x49,0x48,0x47,0x46,0x45,0x44,0x43,0x42,0x41,0x40,
	0x3F,0x3E,0x3D,0x3C,0x3B,0x3A,0x39,0x38,0x37,0x36,0x35,0x34,0x33,0x32,0x31,0x30,
	0x2F,0x2E,0x2D,0x2C,0x2B,0x2A,0x29,0x28,0x27,0x26,0x25,0x24,0x23,0x22,0x21,0x20,
	0x1F,0x1E,0x1D,0x1C,0x1B,0x1A,0x19,0x18,0x17,0x16,0x15,0x14,0x13,0x12,0x11,0x10,
	0x0F,0x0E,0x0D,0x0C,0x0B,0x0A,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
	0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
	0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
	0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
	0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
	0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
	0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF,
};
UINT8  g_au8Buf_Read[FLASH_PAGE_SIZE]= {0};
#if 0
// =====================================================================
// Timer Interrupt (Used to increment u16TimeoutCounter for timeout protection)
// =====================================================================
void SysTick_Handler(void)
{
    // SysTick timer enters every 1ms
    // Increment timeout counter on each interrupt. If I2C hangs beyond HOST_TIMEOUT, it auto-breaks the loop.
    u16TimeoutCounter++; 
}
#endif
// =====================================================================
// Delay Function Implementation (Using M031 BSP SysTick for precise delay)
// =====================================================================
void HOST_I2C_WRITE_DURATION(void)
{
    // Reserved processing time after I2C write (100us)
    CLK_SysTickDelay(100); 
}

void HOST_I2C_READ_DURATION(void)
{
    // Reserved processing time after I2C read (100us)
    CLK_SysTickDelay(100); 
}

void HOST_CMD_INTERVAL(void)
{
    // Minimum interval between commands (2000us = 2ms)
    CLK_SysTickDelay(2000); 
}

void HOST_Delay500uS(void)
{
    // Interval delay time provided for polling status (500us)
    CLK_SysTickDelay(500); 
}

// =====================================================================
// I2C System Initialization
// =====================================================================
void HOST_BUS_Init(void)
{
    u16TimeoutCounter = 0; // Reset timeout counter
    
    // Enable I2C clock and initialize I2C communication (Set speed to 100kHz)
    CLK_EnableModuleClock(I2C0_MODULE);
    I2C_Open(NSP_I2C_PORT, 100000); 
}
#if 0
int32_t g_I2C_i32ErrCode = 0; // Global error code variable for I2C API debugging

// =====================================================================
// M031 I2C Multi-byte Continuous Write (Master Tx State Machine)
// =====================================================================
uint32_t I2C_WriteMultiBytes(I2C_T *i2c, uint8_t u8SlaveAddr, uint8_t data[], uint32_t u32wLen)
{
    uint8_t u8Xfering = 1U, u8Err = 0U, u8Ctrl = 0U;
    uint32_t u32txLen = 0U;
    uint32_t u32TimeOutCount;

    g_I2C_i32ErrCode = 0;

    // 1. Send START signal
    I2C_START(i2c);                                        

    // Enter state machine polling
    while (u8Xfering && (u8Err == 0U))
    {
        u32TimeOutCount = SystemCoreClock;
        // Wait for hardware I2C flag (SI) to be set
        I2C_WAIT_READY(i2c)
        {
            u32TimeOutCount--;
            if(u32TimeOutCount == 0) // Anti-hang Timeout
            {
                g_I2C_i32ErrCode = I2C_TIMEOUT_ERR;
                u8Err = 1U;
                break;
            }
        }

        // Decide the next step based on the current I2C status code returned by hardware
        switch (I2C_GET_STATUS(i2c))
        {
        case 0x08: // Status 0x08: START signal successfully sent
            // Send [Slave Address] + [Write Bit (0)]
            I2C_SET_DATA(i2c, (uint8_t)(u8SlaveAddr << 1U | 0x00U));  
            u8Ctrl = I2C_CTL_SI;                           /* Clear SI flag to let hardware continue */
            break;

        case 0x18: // Status 0x18: SLA+W sent, received ACK from Slave
        case 0x28: // Status 0x28: Data sent, received ACK from Slave
            if (u32txLen < u32wLen)
                // Still have data to send, write data to I2CDAT
                I2C_SET_DATA(i2c, data[u32txLen++]);       
            else
            {
                // All data sent, send STOP signal to terminate
                u8Ctrl = I2C_CTL_STO_SI;                   
                u8Xfering = 0U; // End transmission loop
            }
            break;

        case 0x20: // Status 0x20: SLA+W sent, but received NACK (No device responded)
        case 0x30: // Status 0x30: Data sent, but received NACK (Receiver rejected)
            u8Ctrl = I2C_CTL_STO_SI;                       /* Send STOP to terminate transmission */
            u8Err = 1U;                                    /* Mark error */
            break;

        case 0x38: // Status 0x38: Bus Arbitration Lost
        default:   // Unknown status error
            I2C_SET_CONTROL_REG(i2c, I2C_CTL_STO_SI);      /* Send STOP */
            u8Ctrl = I2C_CTL_SI;
            u8Err = 1U;
            break;
        }

        // Write the determined control command to the register for the hardware to proceed
        I2C_SET_CONTROL_REG(i2c, u8Ctrl);                  
    }

    // Ensure STOP signal has been completely sent at the hardware level
    u32TimeOutCount = SystemCoreClock;
    while ((i2c)->CTL0 & I2C_CTL0_STO_Msk)
    {
        u32TimeOutCount--;
        if(u32TimeOutCount == 0)
        {
            g_I2C_i32ErrCode = I2C_TIMEOUT_ERR;
            u8Err = 1U;
            break;
        }
    }

    return u32txLen; // Return the actual number of bytes successfully sent
}

// =====================================================================
// M031 I2C Multi-byte Continuous Read (Master Rx State Machine)
// =====================================================================
uint32_t I2C_ReadMultiBytes(I2C_T *i2c, uint8_t u8SlaveAddr, uint8_t rdata[], uint32_t u32rLen)
{
    uint8_t u8Xfering = 1U, u8Err = 0U, u8Ctrl = 0U;
    uint32_t u32rxLen = 0U;
    uint32_t u32TimeOutCount;

    g_I2C_i32ErrCode = 0;

    // 1. Send START signal
    I2C_START(i2c);                                          

    while (u8Xfering && (u8Err == 0U))
    {
        u32TimeOutCount = SystemCoreClock;
        I2C_WAIT_READY(i2c)
        {
            u32TimeOutCount--;
            if(u32TimeOutCount == 0)
            {
                g_I2C_i32ErrCode = I2C_TIMEOUT_ERR;
                u8Err = 1U;
                break;
            }
        }

        switch (I2C_GET_STATUS(i2c))
        {
        case 0x08: // Status 0x08: START signal successfully sent
            // Send [Slave Address] + [Read Bit (1)]
            I2C_SET_DATA(i2c, (uint8_t)((u8SlaveAddr << 1U) | 0x01U));  
            u8Ctrl = I2C_CTL_SI;                             
            break;

        case 0x40: // Status 0x40: SLA+R sent, received ACK from Slave
            // Decide what to reply to the Slave after receiving the first data byte
            if (u32rLen == 1)
            {
                // If only 1 byte is needed, reply NACK after reading to indicate end of transmission
                u8Ctrl = I2C_CTL_SI;                         
            }
            else
            {
                // If more data is needed, reply ACK (Set AA bit to 1) after reading this byte
                u8Ctrl = I2C_CTL_SI_AA;                      
            }
            break;

        case 0x48: // Status 0x48: SLA+R sent, but received NACK (No device responded)
            u8Ctrl = I2C_CTL_STO_SI;                         /* Send STOP to terminate */
            u8Err = 1;
            break;

        case 0x50: // Status 0x50: Data successfully received, and Master replied ACK
            rdata[u32rxLen++] = (uint8_t) I2C_GET_DATA(i2c);  // Read this data byte

            // Determine what to reply after receiving the next data byte
            if (u32rxLen < (u32rLen - 1))
            {
                u8Ctrl = I2C_CTL_SI_AA;                      // Not the last byte yet, continue replying ACK
            }
            else
            {
                u8Ctrl = I2C_CTL_SI;                         // The next byte will be the last, reply NACK
            }
            break;

        case 0x58: // Status 0x58: Data successfully received, and Master replied NACK (Read completed)
            rdata[u32rxLen++] = (uint8_t) I2C_GET_DATA(i2c);  // Read the final data byte
            u8Ctrl = I2C_CTL_STO_SI;                         // Send STOP to terminate
            u8Xfering = 0U; // End transmission loop
            break;

        case 0x38:                                                  // Arbitration lost
        default:                                                    // Unknown error
            I2C_SET_CONTROL_REG(i2c, I2C_CTL_STO_SI);               
            u8Ctrl = I2C_CTL_SI;
            u8Err = 1U;
            break;
        }

        I2C_SET_CONTROL_REG(i2c, u8Ctrl);                           
    }

    u32TimeOutCount = SystemCoreClock;
    while ((i2c)->CTL0 & I2C_CTL0_STO_Msk)
    {
        u32TimeOutCount--;
        if(u32TimeOutCount == 0)
        {
            g_I2C_i32ErrCode = I2C_TIMEOUT_ERR;
            u8Err = 1U;
            break;
        }
    }

    return u32rxLen; // Return the actual number of bytes successfully received
}
#endif
// =====================================================================
// I2C Write Wrapper Function for NSP Driver (Called by low-level commands)
// =====================================================================
UINT16 N_I2C_WRITE(UINT16 WRITE_SIZE)
{
    // Software-level timeout protection. Force abort if timed out to prevent hanging.
    //if( u16TimeoutCounter >= HOST_TIMEOUT )
    //   return 0;   
    
    // Call M031 multi-byte write API for transmission
    if (I2C_WriteMultiBytes(NSP_I2C_PORT, NSP_I2C_SLAVE_ADDR, u8WriteBuffer, WRITE_SIZE) == WRITE_SIZE)
    {
        return WRITE_SIZE; // All data successfully sent
    }
    
    return 0; // Send failed
}

// =====================================================================
// I2C Read Wrapper Function for NSP Driver (Called by low-level commands)
// =====================================================================
UINT16 N_I2C_READ(UINT16 READ_SIZE)
{
    // Software-level timeout protection
 //   if( u16TimeoutCounter >= HOST_TIMEOUT )
  //      return 0;
    
    // Call M031 multi-byte read API for reception
    if (I2C_ReadMultiBytes(NSP_I2C_PORT, NSP_I2C_SLAVE_ADDR, u8ReadBuffer, READ_SIZE) == READ_SIZE)
    {
        return READ_SIZE; // All data successfully received
    }

    return 0; // Read failed
}
#if 0
// =====================================================================
// Section 1: nsp_driver.c (NSP Specific Command Control Implementation)
// =====================================================================
void HOST_Init(UINT8* SP_VOL)
{
	*SP_VOL = 0x80;	 // Default volume set to max (128)
}
//===========================================================
// APIs to manually increment/clear timeout counter (Reserved by original manufacturer, currently incremented by interrupt)
void HOST_ADD_TIMEOUT_COUNTER(void)
{
	u16TimeoutCounter++;
}
void HOST_CLR_TIMEOUT_COUNTER(void)
{
	u16TimeoutCounter = 0;
}
#endif
//------------------------------------------------
// Verify if the received data is valid (Check Checksum or specific return codes)
UINT8 CHECK_I2C_READ_RIGHT()
{
	UINT16 i = 0;
	UINT8 CMD_CHECKSUM = 0;
	
	if (u16CMD_RX_BYTE == 1) // If reading only 1 byte (Usually status code of command execution result)
	{
		if (u8ReadBuffer[0] == RIGHT_RTN) // 0x55 (Correct return)
		{
			u8RX_ERROR_COUNT = 0; // Clear error count on success
			return 1;
		}
		if (u8ReadBuffer[0] == ERROR_RTN) // 0xAA (Error return)
		{
			u8RX_ERROR_COUNT ++;  // Accumulate error count on failure
			return 0;
		}	
		if (u8ReadBuffer[0] == UNSUPPORTED_RTN) // Unsupported command
		{
			u8RX_ERROR_COUNT = 0;
			return 0xFF;
		}
	}
	else // Reading more than 1 byte (Returns with Checksum)
	{
        // Sum up all preceding bytes, and compare with the inverted value of the last byte
		for(i = 0; i < (u16CMD_RX_BYTE-1); i++)
		{
			CMD_CHECKSUM += u8ReadBuffer[i];
		}
		if ((CMD_CHECKSUM ^ u8ReadBuffer[u16CMD_RX_BYTE-1]) == 0xFF) // Validation formula: SUM ^ ChecksumByte == 0xFF
		{
			u8RX_ERROR_COUNT = 0;
			return 1;
		}
		else
		{
			u8RX_ERROR_COUNT ++; // Checksum Error
			return 0;
		}
	}	
	return 0;
}
//----------------------------------
// Standard Host transmission command and reply reception process
UINT8 Host_CommonProcess(void)
{
	// 1. Send data (Write)
	if( N_I2C_WRITE(u16CMD_TX_BYTE) )
	{
		HOST_I2C_WRITE_DURATION(); // Short delay after transmission
		// 2. Receive data (Read)
		if( N_I2C_READ(u16CMD_RX_BYTE) )
		{
			HOST_I2C_READ_DURATION(); // Short delay after reception
		}			
		else
			return 0;		
	}
	else
		return 0;
        
	// 3. Check if received data format and Checksum are correct
	return CHECK_I2C_READ_RIGHT();
}
//------------------------------------------------
// Calculate Checksum of transmission data (8-bit array) and append to the last byte
void ADD_I2C_WRITE_CHECKSUM(UINT8 u8CMDBytes,UINT16 u16BufferBytes,UINT8 *DATA_BUFFER)
{	
	UINT16 i = 0;
	UINT8 CMD_CHECKSUM = 0;
	for(i = 0; i < u8CMDBytes; i++) // Accumulate command prefix data
	{
		CMD_CHECKSUM = ((CMD_CHECKSUM + u8WriteBuffer[i]) & 0xFF);
	}
	for(i = 0; i < u16BufferBytes; i++) // Accumulate payload data and move it
	{
		u8WriteBuffer[u8CMDBytes+i] = *DATA_BUFFER;
		CMD_CHECKSUM = ((CMD_CHECKSUM + u8WriteBuffer[u8CMDBytes+i]) & 0xFF);
		DATA_BUFFER++;
	}
    // Checksum = bitwise NOT of sum of all bytes (~SUM)
	u8WriteBuffer[u16CMD_TX_BYTE-1] = (CMD_CHECKSUM ^ 0xFF );
}
//------------------------------------------------
// Calculate Checksum of transmission data (16-bit array) and append to the last byte
void ADD_I2C_WRITE_CHECKSUM_2B(UINT8 u8CMDBytes,UINT16 u16BufferBytes,UINT16 *DATA_BUFFER)
{	
	UINT16 i = 0;
	UINT8 CMD_CHECKSUM = 0;
	for(i = 0; i < u8CMDBytes; i++)
	{
		CMD_CHECKSUM = ((CMD_CHECKSUM + u8WriteBuffer[i]) & 0xFF);
	}
	for(i = 0; i < u16BufferBytes; i++) // Split 16-bit data into two 8-bit pieces and accumulate
	{
		u8WriteBuffer[u8CMDBytes+2*i] = (*DATA_BUFFER  & 0xFF00)>>8; // High Byte
		CMD_CHECKSUM = ((CMD_CHECKSUM + u8WriteBuffer[u8CMDBytes+2*i]) & 0xFF);
		u8WriteBuffer[u8CMDBytes+2*i+1] = (*DATA_BUFFER  & 0xFF);    // Low Byte
		CMD_CHECKSUM = ((CMD_CHECKSUM + u8WriteBuffer[u8CMDBytes+2*i+1]) & 0xFF);
		DATA_BUFFER++;
	}
	u8WriteBuffer[u16CMD_TX_BYTE-1] = (CMD_CHECKSUM ^ 0xFF );
}
//------------------------------------------------
// Simple array content sum Checksum helper function
UINT8 I2C_CHECKSUM(UINT16 u16Count, PUINT8 pu8Buffer)
{	
	UINT8 u8Checksum = 0;
	while( u16Count-- > 0 )
	{
		u8Checksum += *pu8Buffer;
		pu8Buffer++;
	}
	return u8Checksum;
}
//----------------------------------
// Slave (NSP chip) soft reset fail-safe mechanism
void N_SLAVE_RESET(void)
{
    // If I2C read error count reaches 5 times, send RESET signal to NSP to reset system
	if(u8RX_ERROR_COUNT >=5) 
	{
		u8RX_ERROR_COUNT = 0;
		N_RESET();	
	}
}
//===========================================================
// NSP Command API Section
// Logic: 1. Fill write buffer array (command code, parameters, checksum)
//        2. Set Tx / Rx lengths
//        3. Call Host_CommonProcess() to execute I2C transmission
//        4. Apply Interval delay after transmission finishes
//===========================================================

// Read Product ID (Usually used to verify if hardware connection is normal)
UINT8 N_READ_ID(UINT32* PID)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_READ_ID;             // Command Code (0x10)
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF; // Checksum
	u16CMD_TX_BYTE = CMD_READ_ID_TX_BYTE;
	u16CMD_RX_BYTE = CMD_READ_ID_RX_BYTE;
	
	RTN = Host_CommonProcess();
	if (RTN == 1)
	{
         // Combine the 4 returned bytes into a 32-bit ID
		 *PID = (u8ReadBuffer[0]<<24) | (u8ReadBuffer[1]<<16) | (u8ReadBuffer[2]<<8) | u8ReadBuffer[3];
	}
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Read current NSP status (e.g. if it is currently playing)
UINT8 N_READ_STATUS(UINT8* COMMAND_STATUS)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_READ_STATUS;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_READ_STATUS_TX_BYTE;
	u16CMD_RX_BYTE = CMD_READ_STATUS_RX_BYTE;
	
	RTN = Host_CommonProcess();
	if (RTN == 1)
	{
		*COMMAND_STATUS = u8ReadBuffer[0];
	}
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Execute Low Voltage Detection (LVD) measurement
UINT8 N_DO_LVD(void)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_DO_LVD;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_DO_LVD_TX_BYTE;
	u16CMD_RX_BYTE = CMD_DO_LVD_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Get LVD measurement results
UINT8 N_GET_LVD(UINT8* LVD_STATUS)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_GET_LVD;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_GET_LVD_TX_BYTE;
	u16CMD_RX_BYTE = CMD_GET_LVD_RX_BYTE;
	
	RTN = Host_CommonProcess();
	if (RTN == 1)
	{
		*LVD_STATUS = u8ReadBuffer[0];
	}
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Verify if internal data Checksum is correct
UINT8 N_CHECKSUM_RIGHT(UINT8* CHECKSUM_BIT)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_CHECKSUM_RIGHT;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_CHECKSUM_RIGHT_TX_BYTE;
	u16CMD_RX_BYTE = CMD_CHECKSUM_RIGHT_RX_BYTE;
	
	RTN = Host_CommonProcess();
	if (RTN == 1)
	{
		*CHECKSUM_BIT = u8ReadBuffer[0];
	}
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Get the actual Checksum value
UINT8 N_GET_CHECKSUM(UINT16* CHECKSUM_BYTES)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_GET_CHECKSUM;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_GET_CHECKSUM_TX_BYTE;
	u16CMD_RX_BYTE = CMD_GET_CHECKSUM_RX_BYTE;
	
	RTN = Host_CommonProcess();
	if (RTN == 1)
	{
		*CHECKSUM_BYTES = (u8ReadBuffer[0]<<8) | u8ReadBuffer[1];
	}
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Request NSP to start Checksum calculation operation
UINT8 N_DO_CHECKSUM(void)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_DO_CHECKSUM;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_DO_CHECKSUM_TX_BYTE;
	u16CMD_RX_BYTE = CMD_DO_CHECKSUM_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Read firmware version number
UINT8 N_GET_FW_VERSION(UINT32* PFW_VERSION)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_GET_FW_VERSION;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_GET_FW_VERSION_TX_BYTE;
	u16CMD_RX_BYTE = CMD_GET_FW_VERSION_RX_BYTE;
	
	RTN = Host_CommonProcess();
	if (RTN == 1)
	{
		 *PFW_VERSION = (u8ReadBuffer[0]<<16) | (u8ReadBuffer[1]<<8) | u8ReadBuffer[2];
	}
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Set playback repeat count
UINT8 N_PLAY_REPEAT(UINT8 REPEAT_COUNT)
{
	UINT8 RTN = 0;
	
	u8WriteBuffer[0] = CMD_REPEAT;
	u8WriteBuffer[1] = REPEAT_COUNT;
	u8WriteBuffer[2] = ((u8WriteBuffer[0] + u8WriteBuffer[1]) & 0xFF) ^ 0xFF;
	u16CMD_TX_BYTE = CMD_REPEAT_TX_BYTE;
	u16CMD_RX_BYTE = CMD_REPEAT_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Stop repeat playback
UINT8 N_STOP_REPEAT()
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_STOP_REPEAT;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_STOP_REPEAT_TX_BYTE;
	u16CMD_RX_BYTE = CMD_STOP_REPEAT_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Most commonly used function: Play specific audio according to Index
UINT8 N_PLAY(UINT16 PlayListIndex)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_PLAY;
	u8WriteBuffer[1] = PlayListIndex >>8;  // Index High Byte
	u8WriteBuffer[2] = PlayListIndex & 0xFF; // Index Low Byte
	u8WriteBuffer[3] = ((u8WriteBuffer[0] + u8WriteBuffer[1]+ u8WriteBuffer[2])& 0xFF)^ 0xFF; // Checksum
	u16CMD_TX_BYTE = CMD_PLAY_TX_BYTE;
	u16CMD_RX_BYTE = CMD_PLAY_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	HOST_Delay500uS();
	HOST_Delay500uS();
	HOST_Delay500uS();
	return RTN;
}
//----------------------------------
// Play specific audio on a specific channel
UINT8 N_PLAY_CHANNEL(UINT8 ChannelMsk, UINT16* PlayIndexArry)
{
	UINT8 RTN = 0, u8Count = 0, u8Checksum = 0;
	u8WriteBuffer[0] = CMD_PLAY_CHANNEL;
	u8WriteBuffer[1] = ChannelMsk;
	u8Checksum += u8WriteBuffer[0];
	u8Checksum += u8WriteBuffer[1];
	if( ChannelMsk&BIT0 ) // Channel 0
	{
		u8WriteBuffer[2] = PlayIndexArry[0] >>8;
		u8Checksum += u8WriteBuffer[2];
		u8WriteBuffer[3] = PlayIndexArry[0] & 0xFF;	
		u8Checksum += u8WriteBuffer[3];
		u8Count = 1;
	}
	if( ChannelMsk&BIT1 ) // Channel 1
	{
		u8WriteBuffer[2+u8Count*2] = PlayIndexArry[u8Count] >>8;
		u8Checksum += u8WriteBuffer[2+u8Count*2];
		u8WriteBuffer[2+u8Count*2+1] = PlayIndexArry[u8Count] & 0xFF;	
		u8Checksum += u8WriteBuffer[2+u8Count*2+1];
		u8Count += 1;
	}	
	u8WriteBuffer[2+u8Count*2] = u8Checksum ^ 0xff;
	u16CMD_TX_BYTE = CMD_PLAY_CHANNEL_TX_BYTE + u8Count*2;
	u16CMD_RX_BYTE = CMD_PLAY_CHANNEL_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	HOST_Delay500uS();
	HOST_Delay500uS();
	HOST_Delay500uS();
	return RTN;	
}
//----------------------------------
// Automatically enter sleep mode after playback completes (Power saving)
UINT8 N_PLAY_SLEEP(UINT16 PlayListIndex)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_PLAY_SLEEP;
	u8WriteBuffer[1] = PlayListIndex >>8;
	u8WriteBuffer[2] = PlayListIndex & 0xFF;
	u8WriteBuffer[3] = ((u8WriteBuffer[0] + u8WriteBuffer[1]+ u8WriteBuffer[2])& 0xFF)^ 0xFF;
	u16CMD_TX_BYTE = CMD_PLAY_SLEEP_TX_BYTE;
	u16CMD_RX_BYTE = CMD_PLAY_SLEEP_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	HOST_Delay500uS();
	HOST_Delay500uS();
	HOST_Delay500uS();
	return RTN;
}
//----------------------------------
// Playback command with repeat functionality
UINT8 N_PLAY_WITH_REPEAT(UINT16 PlayListIndex, UINT8 REPEAT_COUNT)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_PLAY_REPEAT;
	u8WriteBuffer[1] = PlayListIndex >>8;
	u8WriteBuffer[2] = PlayListIndex & 0xFF;
	u8WriteBuffer[3] = REPEAT_COUNT;
	u8WriteBuffer[4] = ((u8WriteBuffer[0] + u8WriteBuffer[1]+ u8WriteBuffer[2] + u8WriteBuffer[3])& 0xFF)^ 0xFF;
	u16CMD_TX_BYTE = CMD_PLAY_REPEAT_TX_BYTE;
	u16CMD_RX_BYTE = CMD_PLAY_REPEAT_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	HOST_Delay500uS();
	HOST_Delay500uS();
	HOST_Delay500uS();
	return RTN;
}
//----------------------------------
// Set NSP chip IO pin functionality (Input/Output)
UINT8 N_IO_CONFIG(UINT8 IO_FLAG)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_IO_CONFIG;
	u8WriteBuffer[1] = IO_FLAG;
	u8WriteBuffer[2] = ((u8WriteBuffer[0] + u8WriteBuffer[1]) & 0xFF) ^ 0xFF;
	u16CMD_TX_BYTE = CMD_IO_CONFIG_TX_BYTE;
	u16CMD_RX_BYTE = CMD_IO_CONFIG_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Get IO pin direction configuration
UINT8 N_IO_TYPE(UINT8* READ_IO_FLAG)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_IO_TYPE;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_IO_TYPE_TX_BYTE;
	u16CMD_RX_BYTE = CMD_IO_TYPE_RX_BYTE;
	
	RTN = Host_CommonProcess();
	if (RTN == 1)
	{
		*READ_IO_FLAG = u8ReadBuffer[0];
	}
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Control output pin level (High/Low) of NSP chip
UINT8 N_SET_OUT(UINT8 IO_VALUE)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_SET_OUT;
	u8WriteBuffer[1] = IO_VALUE;
	u8WriteBuffer[2] = ((u8WriteBuffer[0] + u8WriteBuffer[1]) & 0xFF) ^ 0xFF;
	u16CMD_TX_BYTE = CMD_SET_OUT_TX_BYTE;
	u16CMD_RX_BYTE = CMD_SET_OUT_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Read input pin level status of NSP chip
UINT8 N_GET_INOUT(UINT8* READ_IO_VALUE)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_GET_INOUT;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_GET_INOUT_TX_BYTE;
	u16CMD_RX_BYTE = CMD_GET_INOUT_RX_BYTE;
	
	RTN = Host_CommonProcess();
	if (RTN == 1)
	{
		*READ_IO_VALUE = u8ReadBuffer[0];
	}
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Enable Busy Pin (Outputs busy signal during playback, can connect to LED or MCU for status checking)
UINT8 N_BZPIN_EN()
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_BZPIN_EN;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_BZPIN_EN_TX_BYTE;
	u16CMD_RX_BYTE = CMD_BZPIN_EN_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Disable Busy Pin
UINT8 N_BZPIN_DIS()
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_BZPIN_DIS;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_BZPIN_DIS_TX_BYTE;
	u16CMD_RX_BYTE = CMD_BZPIN_DIS_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Set playback volume (0~128)
UINT8 N_SET_VOL(UINT8 SP_VOL)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_SET_VOL_NSP;
	u8WriteBuffer[1] = SP_VOL;
	u8WriteBuffer[2] = ((u8WriteBuffer[0] + u8WriteBuffer[1]) & 0xFF) ^ 0xFF;
	u16CMD_TX_BYTE = CMD_SET_VOL_TX_BYTE;
	u16CMD_RX_BYTE = CMD_SET_VOL_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Get current volume setting
UINT8 N_GET_VOL(UINT8* SP_VOL)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_GET_VOL;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_GET_VOL_TX_BYTE;
	u16CMD_RX_BYTE = CMD_GET_VOL_RX_BYTE;
	
	RTN = Host_CommonProcess();
	if (RTN == 1)
	{
		*SP_VOL = u8ReadBuffer[0];
	}
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Force stop audio playback.
// After sending the stop command, it continuously polls to check if NSP has returned to Idle status
UINT8 N_STOP()
{
	UINT8 RTN = 0;
	UINT8 u8NSP_STATUS1 = 0;
	u8WriteBuffer[0] = CMD_STOP;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_STOP_TX_BYTE;
	u16CMD_RX_BYTE = CMD_STOP_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	while ((N_READ_STATUS(&u8NSP_STATUS1) != 1) || (u8NSP_STATUS1 != CMD_VALID))
	{
		HOST_Delay500uS();
		HOST_Delay500uS();
	}
	return RTN;
}
//----------------------------------
// Stop audio on specific channel
UINT8 N_STOP_CHANNEL(UINT8 ChannelMsk)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_STOP_CHANNEL;
	u8WriteBuffer[1] = ChannelMsk;
	u8WriteBuffer[2] = ((u8WriteBuffer[0] + u8WriteBuffer[1]) & 0xFF) ^ 0xFF;
	u16CMD_TX_BYTE = CMD_STOP_CHANNEL_TX_BYTE;
	u16CMD_RX_BYTE = CMD_STOP_CHANNEL_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	HOST_Delay500uS();
	HOST_Delay500uS();
	HOST_Delay500uS();	
	return RTN;
}
//----------------------------------
// Reset NSP chip.
// After sending, it polls continuously until NSP status returns to available (CMD_VALID)
UINT8 N_RESET()
{
	UINT8 RTN = 0;
	UINT8 u8NSP_STATUS1 = 0;
	u8WriteBuffer[0] = CMD_RESET;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_RESET_TX_BYTE;
	u16CMD_RX_BYTE = CMD_RESET_RX_BYTE;
	
	RTN = Host_CommonProcess();	
	HOST_CMD_INTERVAL();
	while ((N_READ_STATUS(&u8NSP_STATUS1) != 1) || (u8NSP_STATUS1 != CMD_VALID))
	{
		HOST_Delay500uS();
		HOST_Delay500uS();
	}
	return RTN;
}
//----------------------------------
// Force NSP chip into deep sleep (power-down) mode for extreme power saving
UINT8 N_PWR_DOWN()
{
	UINT8 RTN = 0;
	UINT8 n = 0;
	u8WriteBuffer[0] = CMD_PWR_DOWN;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_PWR_DOWN_TX_BYTE;
	u16CMD_RX_BYTE = CMD_PWR_DOWN_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	for (n= 0; n <= 40; n++)
	{
		HOST_Delay500uS();
	}
	return RTN;
}
//----------------------------------
// Get ISP update data Checksum (For firmware update)
UINT8 N_ISP_CHECKSUM(UINT16* ISP_CHECKSUM_BYTES)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_ISP_CHECKSUM;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_ISP_CHECKSUM_TX_BYTE;
	u16CMD_RX_BYTE = CMD_ISP_CHECKSUM_RX_BYTE;
	
	RTN = Host_CommonProcess();
	if (RTN == 1)
	{
		*ISP_CHECKSUM_BYTES = (u8ReadBuffer[0]<<8) | u8ReadBuffer[1];
	}
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Send command to start ISP programming (Verifies Chip ID and Product ID)
UINT8 N_ISP_WRITE_START(UINT32 PROC_ID,UINT32 CHIP_PDID)
{
	UINT8 RTN = 0;
	UINT8 i = 0;
	UINT8 u8NSP_STATUS1 = 0;
	u8WriteBuffer[0] = CMD_ISP_WRITE_START;
	u8WriteBuffer[1] = (PROC_ID & 0xff000000) >> 24;
	u8WriteBuffer[2] = (PROC_ID & 0x00ff0000) >> 16;
	u8WriteBuffer[3] = (CHIP_PDID & 0xff000000) >> 24;
	u8WriteBuffer[4] = (CHIP_PDID & 0xff0000) >> 16;
	u8WriteBuffer[5] = (CHIP_PDID & 0xff00) >> 8;
	u8WriteBuffer[6] = (CHIP_PDID & 0xff) ;
	u8WriteBuffer[7] = 0;
	for(i = 0; i <(CMD_ISP_WRITE_START_TX_BYTE-1); i++)
	{
		u8WriteBuffer[7] = ((u8WriteBuffer[7] + u8WriteBuffer[i]) & 0xFF);
	}
	u8WriteBuffer[7] = (u8WriteBuffer[7] ^ 0xff);
	u16CMD_TX_BYTE = CMD_ISP_WRITE_START_TX_BYTE;
	u16CMD_RX_BYTE = CMD_ISP_WRITE_START_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	while ((N_READ_STATUS(&u8NSP_STATUS1) != 1) || (u8NSP_STATUS1 != CMD_VALID))
	{
		HOST_CMD_INTERVAL();
	}
	return RTN;
}
//----------------------------------
// Send ISP programming completion command
UINT8 N_ISP_WRITE_END(void)
{
	UINT8 RTN = 0;
	UINT8 u8NSP_STATUS1 = 0;	
	u8WriteBuffer[0] = CMD_ISP_WRITE_END;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_ISP_WRITE_END_TX_BYTE;
	u16CMD_RX_BYTE = CMD_ISP_WRITE_END_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	while ((N_READ_STATUS(&u8NSP_STATUS1) != 1) || (u8NSP_STATUS1 != CMD_VALID))
	{
		HOST_CMD_INTERVAL();
	}
	return RTN;
}
//----------------------------------
// ISP Write an entire page of data (512 Bytes)
UINT8 N_ISP_WRITE_PAGE(UINT32 ISP_ADDR,PUINT8 ISP_BUFFER)
{
	UINT8 RTN = 0;
	UINT8 u8NSP_STATUS1 = 0;
	u8WriteBuffer[0] = CMD_ISP_WRITE_PAGE;
	u8WriteBuffer[1] = (ISP_ADDR & 0xFF0000)>>16;
	u8WriteBuffer[2] = (ISP_ADDR & 0xFF00)>>8;
	u8WriteBuffer[3] = (ISP_ADDR & 0xFF);
	u16CMD_TX_BYTE = CMD_ISP_WRITE_PAGE_TX_BYTE;
	u16CMD_RX_BYTE = CMD_ISP_WRITE_PAGE_RX_BYTE;
	ADD_I2C_WRITE_CHECKSUM(4, 0x200 ,ISP_BUFFER);

	RTN = Host_CommonProcess();	
	HOST_CMD_INTERVAL();
	HOST_CMD_INTERVAL();
	HOST_CMD_INTERVAL();
	HOST_CMD_INTERVAL();
	HOST_CMD_INTERVAL();
	while ((N_READ_STATUS(&u8NSP_STATUS1) != 1) || (u8NSP_STATUS1 != CMD_VALID))
	{
		HOST_CMD_INTERVAL();
		HOST_CMD_INTERVAL();
		HOST_CMD_INTERVAL();
		HOST_CMD_INTERVAL();
		HOST_CMD_INTERVAL();
	}
	return RTN;
}
//----------------------------------
// ISP Read a page of data (512 Bytes)
UINT8 N_ISP_READ_PAGE(UINT32 ISP_ADDR,PUINT8 ISP_BUFFER)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_ISP_READ_PAGE;
	u8WriteBuffer[1] = (ISP_ADDR & 0xFF0000)>>16;
	u8WriteBuffer[2] = (ISP_ADDR & 0xFF00)>>8;
	u8WriteBuffer[3] = (ISP_ADDR & 0xFF);
	u8WriteBuffer[4] = ((u8WriteBuffer[0] + u8WriteBuffer[1]+ u8WriteBuffer[2]+ u8WriteBuffer[3])& 0xFF)^ 0xFF;
	u16CMD_TX_BYTE = CMD_ISP_READ_PAGE_TX_BYTE;
	u16CMD_RX_BYTE = CMD_ISP_READ_PAGE_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// ISP Read partial data fragment
UINT8 N_ISP_READ_PARTIAL(UINT32 ISP_ADDR,UINT16 ISPSize,PUINT8 ISP_BUFFER)
{
	UINT8 RTN = 0;
	UINT8 i = 0;
	u8WriteBuffer[0] = CMD_ISP_READ_PARTIAL;
	u8WriteBuffer[1] = (ISP_ADDR & 0xFF0000)>>16;
	u8WriteBuffer[2] = (ISP_ADDR & 0xFF00)>>8;
	u8WriteBuffer[3] = (ISP_ADDR & 0xFF);
	u8WriteBuffer[4] = (ISPSize & 0xFF00)>>8;
	u8WriteBuffer[5] = (ISPSize & 0xFF);
	u8WriteBuffer[6] = 0;
	for(i = 0; i <(CMD_ISP_READ_PARTIAL_TX_BYTE-1); i++)
	{
		u8WriteBuffer[6] = u8WriteBuffer[6] + u8WriteBuffer[i];
	}
	u8WriteBuffer[6] = (u8WriteBuffer[6] ^ 0xff);
	u16CMD_TX_BYTE = CMD_ISP_READ_PARTIAL_TX_BYTE;
	u16CMD_RX_BYTE = CMD_ISP_READ_PARTIAL_RX_BYTE + ISPSize;
	
	RTN = Host_CommonProcess();	
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// ISP Read location information for a specific audio index
UINT8 N_ISP_READ_RES_INDEX(UINT16 ResourceIndex)
{
	UINT8 RTN = 0;
	UINT8 u8NSP_STATUS1 = 0;
	u8WriteBuffer[0] = CMD_ISP_READ_RES_INDEX;
	u8WriteBuffer[1] = ResourceIndex >>8;
	u8WriteBuffer[2] = ResourceIndex & 0xFF;
	u8WriteBuffer[3] = ((u8WriteBuffer[0] + u8WriteBuffer[1]+ u8WriteBuffer[2])& 0xFF)^ 0xFF;
	u16CMD_TX_BYTE = CMD_ISP_READ_RES_INDEX_TX_BYTE;
	u16CMD_RX_BYTE = CMD_ISP_READ_RES_INDEX_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	while ((N_READ_STATUS(&u8NSP_STATUS1) != 1) || (u8NSP_STATUS1 != CMD_VALID))
	{
		HOST_CMD_INTERVAL();
	}
	return RTN;
}
//----------------------------------
// Get size and address information occupied by a specific Resource (audio) in Flash
UINT8 N_ISP_GET_RES_INFO(UINT32* SpaceSize,UINT32* ResourceStartAddr,UINT16* FisrtPageDataByte,UINT16* PageCount,UINT16* LastPageDataByte)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_ISP_GET_RES_INFO;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_ISP_GET_RES_INFO_TX_BYTE;
	u16CMD_RX_BYTE = CMD_ISP_GET_RES_INFO_RX_BYTE;
	
	RTN = Host_CommonProcess();
	if (RTN == 1)
	{
		 *SpaceSize             = (u8ReadBuffer[0]<<16) | (u8ReadBuffer[1]<<8) | u8ReadBuffer[2];
		 *ResourceStartAddr     = (u8ReadBuffer[3]<<16) | (u8ReadBuffer[4]<<8) | u8ReadBuffer[5];
		 *FisrtPageDataByte     = (u8ReadBuffer[6]<<8) | u8ReadBuffer[7];
		 *PageCount              = (u8ReadBuffer[8]<<8) | u8ReadBuffer[9];
		 *LastPageDataByte      = (u8ReadBuffer[10]<<8) | u8ReadBuffer[11];
	}
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Get size and location information of User Space
UINT8 N_ISP_GET_USER_SPACE_INFO(UINT32* SpaceSize,UINT32* ResourceStartAddr,UINT16* FisrtPageDataByte,UINT16* PageCount,UINT16* LastPageDataByte)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_ISP_GET_USER_SPACE_INFO;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_ISP_GET_USER_SPACE_INFO_TX_BYTE;
	u16CMD_RX_BYTE = CMD_ISP_GET_USER_SPACE_INFO_RX_BYTE;
	
	RTN = Host_CommonProcess();
	if (RTN == 1)
	{
		 *SpaceSize             = (u8ReadBuffer[0]<<16) | (u8ReadBuffer[1]<<8) | u8ReadBuffer[2];
		 *ResourceStartAddr     = (u8ReadBuffer[3]<<16) | (u8ReadBuffer[4]<<8) | u8ReadBuffer[5];
		 *FisrtPageDataByte     = (u8ReadBuffer[6]<<8) | u8ReadBuffer[7];
		 *PageCount              = (u8ReadBuffer[8]<<8) | u8ReadBuffer[9];
		 *LastPageDataByte      = (u8ReadBuffer[10]<<8) | u8ReadBuffer[11];
	}
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
UINT8 N_ISP_WRITE_PARTIAL_START(void)
{
	UINT8 RTN = 0;
	UINT8 u8NSP_STATUS1 = 0;
	
	u8WriteBuffer[0] = CMD_ISP_WRITE_PARTIAL_START;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_ISP_WRITE_PARTIAL_START_TX_BYTE;
	u16CMD_RX_BYTE = CMD_ISP_WRITE_PARTIAL_START_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	while ((N_READ_STATUS(&u8NSP_STATUS1) != 1) || (u8NSP_STATUS1 != CMD_VALID))
	{
		HOST_CMD_INTERVAL();
	}
	return RTN;
}
//----------------------------------
UINT8 N_ISP_WRITE_PARTIAL_BAK(UINT32 ISP_ADDR)
{
	UINT8 RTN = 0;
	UINT8 u8NSP_STATUS1 = 0;	
	u8WriteBuffer[0] = CMD_ISP_WRITE_PARTIAL_BAK;
	u8WriteBuffer[1] = (ISP_ADDR & 0xFF0000)>>16;
	u8WriteBuffer[2] = (ISP_ADDR & 0xFF00)>>8;
	u8WriteBuffer[3] = (ISP_ADDR & 0xFF);
	u8WriteBuffer[4] = ((u8WriteBuffer[0] + u8WriteBuffer[1]+ u8WriteBuffer[2]+ u8WriteBuffer[3])& 0xFF)^ 0xFF;
	u16CMD_TX_BYTE = CMD_ISP_WRITE_PARTIAL_BAK_TX_BYTE;
	u16CMD_RX_BYTE = CMD_ISP_WRITE_PARTIAL_BAK_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	HOST_CMD_INTERVAL();
	HOST_CMD_INTERVAL();
	HOST_CMD_INTERVAL();
	HOST_CMD_INTERVAL();
	while ((N_READ_STATUS(&u8NSP_STATUS1) != 1) || (u8NSP_STATUS1 != CMD_VALID))
	{
		HOST_CMD_INTERVAL();
		HOST_CMD_INTERVAL();
		HOST_CMD_INTERVAL();
		HOST_CMD_INTERVAL();
		HOST_CMD_INTERVAL();
	}
	return RTN;
}
//----------------------------------
UINT8 N_ISP_WRITE_PARTIAL(UINT32 ISP_ADDR,UINT16 ISPSize,PUINT8 ISP_BUFFER)
{
	UINT8 RTN = 0;
	UINT8 u8NSP_STATUS1 = 0;
	u8WriteBuffer[0] = CMD_ISP_WRITE_PARTIAL;
	u8WriteBuffer[1] = (ISP_ADDR & 0xFF0000)>>16;
	u8WriteBuffer[2] = (ISP_ADDR & 0xFF00)>>8;
	u8WriteBuffer[3] = (ISP_ADDR & 0xFF);
	u8WriteBuffer[4] = (ISPSize & 0xFF00)>>8;
	u8WriteBuffer[5] = (ISPSize & 0xFF);
	u16CMD_TX_BYTE = CMD_ISP_WRITE_PARTIAL_TX_BYTE + ISPSize ;
	u16CMD_RX_BYTE = CMD_ISP_WRITE_PARTIAL_RX_BYTE;
	ADD_I2C_WRITE_CHECKSUM(CMD_ISP_WRITE_PARTIAL_TX_BYTE-1, ISPSize ,ISP_BUFFER);
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	HOST_CMD_INTERVAL();
	HOST_CMD_INTERVAL();
	HOST_CMD_INTERVAL();
	HOST_CMD_INTERVAL();
	while ((N_READ_STATUS(&u8NSP_STATUS1) != 1) || (u8NSP_STATUS1 != CMD_VALID))
	{
		HOST_CMD_INTERVAL();
		HOST_CMD_INTERVAL();
		HOST_CMD_INTERVAL();
		HOST_CMD_INTERVAL();
		HOST_CMD_INTERVAL();
	}
	return RTN;
}
//----------------------------------
// Chain multiple audios together for continuous playback (8-bit index array)
UINT8 N_MULTI_PLAY(UINT8 PlayListNum,PUINT8 DATA_BUFFER)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_MULTI_PLAY;
	u8WriteBuffer[1] = PlayListNum;
	u16CMD_TX_BYTE = CMD_MULTI_PLAY_TX_BYTE+PlayListNum-1;
	u16CMD_RX_BYTE = CMD_MULTI_PLAY_RX_BYTE;
	ADD_I2C_WRITE_CHECKSUM(2, PlayListNum ,DATA_BUFFER);

	RTN = Host_CommonProcess();	
	HOST_CMD_INTERVAL();
	HOST_Delay500uS();
	HOST_Delay500uS();
	HOST_Delay500uS();
	return RTN;
}
//----------------------------------
// Chain multiple audios together for continuous playback (16-bit index array)
UINT8 N_MULTI_PLAY_2B(UINT8 PlayListNum,PUINT16 DATA_BUFFER)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_MULTI_PLAY_2B;
	u8WriteBuffer[1] = PlayListNum;
	u16CMD_TX_BYTE = CMD_MULTI_PLAY_2B_TX_BYTE + PlayListNum * 2;
	u16CMD_RX_BYTE = CMD_MULTI_PLAY_2B_RX_BYTE;
	ADD_I2C_WRITE_CHECKSUM_2B(2, PlayListNum ,DATA_BUFFER);

	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	HOST_Delay500uS();
	HOST_Delay500uS();
	HOST_Delay500uS();
	return RTN;
}
//----------------------------------
// Multi-play with silence intervals
UINT8 N_MULTI_PLAY_SILENCE(UINT8 CH, UINT8 PlayListNum,PUINT8 PLAYINDEX_BUFFER,PUINT8 SILENCE_BUFFER)
{
	UINT8 RTN = 0, i;
	UINT8 MultiPlayBuffer[64]={0};
	PUINT8 DATA2_BUFFER = &MultiPlayBuffer[0];
	u8WriteBuffer[0] = CMD_MULTI_PLAY_SILENCE;
	u8WriteBuffer[1] = CH;
	u8WriteBuffer[2] = PlayListNum;
	u16CMD_TX_BYTE = CMD_MULTI_PLAY_SILENCE_TX_BYTE + PlayListNum * 2;
	u16CMD_RX_BYTE = CMD_MULTI_PLAY_SILENCE_RX_BYTE;	

	for(i = 0; i < PlayListNum; i++)
	{
		u8WriteBuffer[i*2+3] = *PLAYINDEX_BUFFER;
		u8WriteBuffer[i*2+4] = *SILENCE_BUFFER;
		PLAYINDEX_BUFFER++;
		SILENCE_BUFFER++;
	}
	u8WriteBuffer[u16CMD_TX_BYTE-1] = (I2C_CHECKSUM((u16CMD_TX_BYTE-1),u8WriteBuffer) ^ 0xFF );

	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	HOST_Delay500uS();
	HOST_Delay500uS();
	HOST_Delay500uS();
	return RTN;
}
//----------------------------------
UINT8 N_MULTI_PLAY_2B_SILENCE(UINT8 CH, UINT8 PlayListNum,PUINT16 PLAYINDEX_BUFFER,PUINT8 SILENCE_BUFFER)
{
	UINT8 RTN = 0, i;
	UINT8 MultiPlayBuffer[96]={0};
	PUINT8 DATA2_BUFFER = &MultiPlayBuffer[0];
	u8WriteBuffer[0] = CMD_MULTI_PLAY_2B_SILENCE;
	u8WriteBuffer[1] = CH;
	u8WriteBuffer[2] = PlayListNum;
	u16CMD_TX_BYTE = CMD_MULTI_PLAY_2B_SILENCE_TX_BYTE + PlayListNum * 3;
	u16CMD_RX_BYTE = CMD_MULTI_PLAY_2B_SILENCE_RX_BYTE;	

	for(i = 0; i < PlayListNum; i++)
	{
		u8WriteBuffer[i*3+3] = (*PLAYINDEX_BUFFER  & 0xFF00)>>8;
		u8WriteBuffer[i*3+4] = (*PLAYINDEX_BUFFER  & 0xFF);
		u8WriteBuffer[i*3+5] = *SILENCE_BUFFER;
		PLAYINDEX_BUFFER++;
		SILENCE_BUFFER++;
	}
	u8WriteBuffer[u16CMD_TX_BYTE-1] = (I2C_CHECKSUM((u16CMD_TX_BYTE-1),u8WriteBuffer) ^ 0xFF );
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	HOST_Delay500uS();
	HOST_Delay500uS();
	HOST_Delay500uS();
	return RTN;
}
//----------------------------------
// Enable idle auto-sleep function (Used together with GUI settings)
UINT8 N_AUTO_SLEEP_EN()
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_AUTO_SLEEP_EN;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_AUTO_SLEEP_EN_TX_BYTE;
	u16CMD_RX_BYTE = CMD_AUTO_SLEEP_EN_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	return RTN;	
}
//----------------------------------
// Disable idle auto-sleep function
UINT8 N_AUTO_SLEEP_DIS()
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_AUTO_SLEEP_DIS;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_AUTO_SLEEP_DIS_TX_BYTE;
	u16CMD_RX_BYTE = CMD_AUTO_SLEEP_DIS_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Pause current audio playback
UINT8 N_PAUSE()
{
	UINT8 RTN = 0;
	UINT8 u8NSP_STATUS1 = 0;	
	u8WriteBuffer[0] = CMD_PAUSE;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_PAUSE_TX_BYTE;
	u16CMD_RX_BYTE = CMD_PAUSE_RX_BYTE;

	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	while ((N_READ_STATUS(&u8NSP_STATUS1) != 1) || (u8NSP_STATUS1 != CMD_VALID))
	{
		HOST_CMD_INTERVAL();
	}
	return RTN;
}
//----------------------------------
// Pause audio playback on a specific channel
UINT8 N_PAUSE_CHANNEL(UINT8 ChannelMsk)
{
	UINT8 RTN = 0;
	UINT8 u8NSP_STATUS1 = 0;
	u8WriteBuffer[0] = CMD_PAUSE_CHANNEL;
	u8WriteBuffer[1] = ChannelMsk;
	u8WriteBuffer[2] = ( u8WriteBuffer[0] + u8WriteBuffer[1] ) ^ 0xFF;
	u16CMD_TX_BYTE = CMD_PAUSE_CHANNEL_TX_BYTE;
	u16CMD_RX_BYTE = CMD_PAUSE_CHANNEL_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	while ((N_READ_STATUS(&u8NSP_STATUS1) != 1))
	{
		HOST_Delay500uS();
		HOST_Delay500uS();
	}
	return RTN;	
}
//----------------------------------
// Resume the paused audio playback
UINT8 N_RESUME()
{
	UINT8 RTN = 0;
	UINT8 u8NSP_STATUS1 = 0;
	u8WriteBuffer[0] = CMD_RESUME;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_RESUME_TX_BYTE;
	u16CMD_RX_BYTE = CMD_RESUME_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	while (N_READ_STATUS(&u8NSP_STATUS1) != 1)
	{
		HOST_CMD_INTERVAL();
	}
	return RTN;
}
//----------------------------------
UINT8 N_RESUME_CHANNEL(UINT8 ChannelMsk)
{
	UINT8 RTN = 0;
	UINT8 u8NSP_STATUS1 = 0;
	u8WriteBuffer[0] = CMD_RESUME_CHANNEL;
	u8WriteBuffer[1] = ChannelMsk;
	u8WriteBuffer[2] = ( u8WriteBuffer[0] + u8WriteBuffer[1] ) ^ 0xFF;
	u16CMD_TX_BYTE = CMD_RESUME_CHANNEL_TX_BYTE;
	u16CMD_RX_BYTE = CMD_RESUME_CHANNEL_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	while (N_READ_STATUS(&u8NSP_STATUS1) != 1)
	{
		HOST_CMD_INTERVAL();
	}
	return RTN;	
}
//----------------------------------
// Check how many Resources (actual audio files) are contained under a specific playlist
UINT8 N_CHECK_INDEX_RCOUNT(UINT16 PlayListIndex)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_CHECK_INDEX_RCOUNT;
	u8WriteBuffer[1] = PlayListIndex >>8;
	u8WriteBuffer[2] = PlayListIndex & 0xFF;
	u8WriteBuffer[3] = ((u8WriteBuffer[0] + u8WriteBuffer[1]+ u8WriteBuffer[2])& 0xFF)^ 0xFF;
	u16CMD_TX_BYTE = CMD_CHECK_INDEX_RCOUNT_TX_BYTE;
	u16CMD_RX_BYTE = CMD_CHECK_INDEX_RCOUNT_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
UINT8 N_GET_INDEX_RCOUNT(UINT8* RESOURCE_COUNT)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_GET_INDEX_RCOUNT;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_GET_INDEX_RCOUNT_TX_BYTE;
	u16CMD_RX_BYTE = CMD_GET_INDEX_RCOUNT_RX_BYTE;
	
	RTN = Host_CommonProcess();
	if (RTN == 1)
	{
		*RESOURCE_COUNT = u8ReadBuffer[0];
	}
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Get the maximum Index number supported by the current firmware
UINT8 N_GET_MAX_INDEX(UINT16* MAX_INDEX)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_GET_MAX_INDEX;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_GET_MAX_INDEX_TX_BYTE;
	u16CMD_RX_BYTE = CMD_GET_MAX_INDEX_RX_BYTE;
	
	RTN = Host_CommonProcess();
	if (RTN == 1)
	{
		*MAX_INDEX = (u8ReadBuffer[0]<<8) | u8ReadBuffer[1];
	}
	HOST_CMD_INTERVAL();
	return RTN;
}//----------------------------------
// (If touch function is available) Read touch status
UINT8 N_TOUCH_READSTATUS(UINT8 COUNT,UINT8* STATUS)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_TOUCH_READ_STATUS;
	u8WriteBuffer[1] = COUNT;
	u8WriteBuffer[2] = (u8WriteBuffer[0]+u8WriteBuffer[1]) ^ 0xFF;
	u16CMD_TX_BYTE = CMD_TOUCH_READ_STATUS_TX_BYTE;
	u16CMD_RX_BYTE = CMD_TOUCH_READ_STATUS_RX_BYTE+COUNT;

	RTN = Host_CommonProcess();	
	if( RTN == 1 )
	{
		memcpy(STATUS,u8ReadBuffer,COUNT);
	}
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Update touch related setting parameters
UINT8 N_TOUCH_UPDATA(UINT8 CS, UINT16 CNT, PUINT8 DATA_BUFFER)
{
	UINT8 RTN = 0, u8i;
	u8WriteBuffer[0] = CMD_TOUCH_UPDATA;
	u8WriteBuffer[1] = CS;
	u8WriteBuffer[2] = CNT>>8;
	u8WriteBuffer[3] = CNT;
	memcpy( &u8WriteBuffer[4], DATA_BUFFER, CNT );	
	// Calculate checksum.
	u8WriteBuffer[CMD_TOUCH_UPDATA_TX_BYTE+CNT-1] = 0;
	for( u8i=0; u8i<CMD_TOUCH_UPDATA_TX_BYTE+CNT-1; u8i++ )
		u8WriteBuffer[CMD_TOUCH_UPDATA_TX_BYTE+CNT-1] += u8WriteBuffer[u8i];
	u8WriteBuffer[CMD_TOUCH_UPDATA_TX_BYTE+CNT-1] ^= 0xff;
	u16CMD_TX_BYTE = CMD_TOUCH_UPDATA_TX_BYTE+CNT;
	u16CMD_RX_BYTE = CMD_TOUCH_UPDATA_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_Delay500uS();
	HOST_Delay500uS();
	HOST_Delay500uS();
	return RTN;
}
//----------------------------------
// Load touch configuration
UINT8 N_TOUCH_LOAD(UINT8 CS, UINT16 CNT, PUINT8 DATA_BUFFER)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_TOUCH_LOAD;
	u8WriteBuffer[1] = CS;
	u8WriteBuffer[2] = CNT>>8;
	u8WriteBuffer[3] = CNT;
	u8WriteBuffer[4] = ( u8WriteBuffer[0] + u8WriteBuffer[1] + u8WriteBuffer[2] + u8WriteBuffer[3] ) ^ 0xFF;
	u16CMD_TX_BYTE = CMD_TOUCH_LOAD_TX_BYTE;
	u16CMD_RX_BYTE = CMD_TOUCH_LOAD_RX_BYTE+CNT;
	
	RTN = Host_CommonProcess();
	if( RTN == 1 )
	{
		memcpy(DATA_BUFFER,u8ReadBuffer,CNT);
	}
	HOST_Delay500uS();
	HOST_Delay500uS();
	HOST_Delay500uS();
	return RTN;
}
//----------------------------------
// Read currently configured I2C Slave Address (Some chips support dynamic change)
UINT8 N_READ_SLAVE_ADDR(UINT8* ADDR)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_READ_SLAVE_ADDR;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_READ_SLAVE_ADDR_TX_BYTE;
	u16CMD_RX_BYTE = CMD_READ_SLAVE_ADDR_RX_BYTE;
	
	RTN = Host_CommonProcess();
	if (RTN == 1)
	{
		*ADDR = u8ReadBuffer[0];
	}
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Start recording command (Exclusive to chips like NSR that support recording)
UINT8 N_REC_INDEX_START(UINT16 RecordIndex)
{
	UINT8 RTN = 0;
	UINT8 u8NSP_STATUS1 = 0;
	u8WriteBuffer[0] = CMD_REC_INDEX_START;
	u8WriteBuffer[1] = RecordIndex >>8;
	u8WriteBuffer[2] = RecordIndex & 0xFF;
	u8WriteBuffer[3] = ((u8WriteBuffer[0] + u8WriteBuffer[1]+ u8WriteBuffer[2])& 0xFF)^ 0xFF;
	u16CMD_TX_BYTE = CMD_REC_INDEX_START_TX_BYTE;
	u16CMD_RX_BYTE = CMD_REC_INDEX_START_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	while ((N_READ_STATUS(&u8NSP_STATUS1) != 1) || (u8NSP_STATUS1 != CMD_VALID))
	{
		HOST_CMD_INTERVAL();
	}
	return RTN;
}
//----------------------------------
// Stop recording
UINT8 N_REC_STOP(void)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_REC_STOP;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_REC_STOP_TX_BYTE;
	u16CMD_RX_BYTE = CMD_REC_STOP_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Set volume for individual channels
UINT8 N_SET_CHANNEL_VOL(UINT8 ChannelMsk, PUINT8 ValueArry)
{
	UINT8 RTN = 0, u8Count = 0, u8Checksum = 0;
	u8WriteBuffer[0] = CMD_SET_CHANNEL_VOL;
	u8Checksum = u8WriteBuffer[0];
	u8WriteBuffer[1] = ChannelMsk;
	u8Checksum += u8WriteBuffer[1];
	if( ChannelMsk&BIT0 )
	{
		u8WriteBuffer[2] = ValueArry[0];
		u8Checksum += u8WriteBuffer[2];
		u8Count++;
	}
	if( ChannelMsk&BIT1 )
	{
		u8WriteBuffer[3] = ValueArry[1];
		u8Checksum += u8WriteBuffer[3];
		u8Count++;
	}	
	u8WriteBuffer[2+u8Count] = u8Checksum ^ 0xff;

	u16CMD_TX_BYTE = CMD_SET_CHANNEL_VOL_TX_BYTE + u8Count;
	u16CMD_RX_BYTE = CMD_SET_CHANNEL_VOL_RX_BYTE;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	return RTN;	
}
//----------------------------------
UINT8 N_GET_CHANNEL_VOL(UINT8 ChannelMsk, PUINT8 ValueArry)
{
	UINT8 RTN = 0, u8Count = 0, u8Checksum = 0;
	u8WriteBuffer[0] = CMD_GET_CHANNEL_VOL;
	u8Checksum = u8WriteBuffer[0];
	u8WriteBuffer[1] = ChannelMsk;
	u8Checksum += u8WriteBuffer[1];
	u8WriteBuffer[2] = u8Checksum ^ 0xff;
	
	if( ChannelMsk&BIT0 )
		u8Count++;
	if( ChannelMsk&BIT1 )
		u8Count++;
	
	u16CMD_TX_BYTE = CMD_GET_CHANNEL_VOL_TX_BYTE;
	u16CMD_RX_BYTE = CMD_GET_CHANNEL_VOL_RX_BYTE + u8Count;
	
	RTN = Host_CommonProcess();
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
// Wake up the NSP chip in deep sleep mode (by reading status once)
void N_WAKUP(void)
{
	UINT8 u8NSP_STATUS1 = 0;
	N_READ_STATUS(&u8NSP_STATUS1);
	HOST_CMD_INTERVAL();
}

// =====================================================================
// Section 2: nsp_PlaySample.c (Application Scene Example Functions)
// =====================================================================

/*----- Dead loop wait for audio playback to finish (Blocking code) -----*/ 
void I2C_WaitPlayEND(void)
{
    UINT8 NSP_STATUS = 0;
    // Keep reading status until status is 'Command valid' and BIT7 (play busy flag) falls to 0
    while ((N_READ_STATUS(&NSP_STATUS) != 1) || (NSP_STATUS & 0x80))	 
    {                                                  // BIT7(SP_BUSY) 0:NSP is not playing, 1:NSP is playing
         HOST_Delay500uS();						 
    }
}

/*----- Dead loop wait for command execution to finish (e.g. used during burning, calculating) -----*/ 
UINT8 I2C_WaitExecutionEND(void)
{
    UINT8 NSP_STATUS = 0;
    while (N_READ_STATUS(&NSP_STATUS) != 1)           // Successful status read means command processing complete
    {                                                 
          HOST_Delay500uS();						 
    }
    
    if(NSP_STATUS & 0x40)                             // BIT6(CMD_VALID) 0:Command invalid, 1:Command executable
          return 1;                                   // Return 1 indicates NSP command execution finished
    else								
          return 0; 				                  // Return 0 indicates NSP command is currently unexecutable
}
                                                         
/*----- Query play status (Non-blocking, can be polled in main loop) -----*/
UINT8 I2C_AskStatus(void)
{
    UINT8 NSP_STATUS = 0;
    
    if(N_READ_STATUS(&NSP_STATUS) == 1)               // Read NSP Status
    {
        // Check if BIT7 (Busy) is 0, and BIT6 (Valid) is 1
	    if(!(NSP_STATUS & 0x80) && (NSP_STATUS & 0x40) )
	    {
             return 1;                               // Return 1 indicates NSP is not playing and ready for next command
	    }                                                  
	    else
	    {
             return 0;                               // Return 0 indicates NSP is still playing or command transmission processing
	    }                                                  
    }
    else
       return 0; // I2C communication error, treated as not ready
}

//===========================================================
/*----- Basic Single Audio Playback Example -----*/
void I2C_IndexPlaySample(void)
{
    UINT16 temp = 0;                                    
    UINT8  u8RESOURCE_COUNT = 0;                        	
    HOST_BUS_Init();                                    // Host Initialization
                                                        
    N_PLAY(1);                                          // Play Index 1 Audio 
    I2C_WaitPlayEND();                                  // Blocking wait for Index 1 to finish
                                                        
    N_PLAY(2);                                          // Play Index 2 Audio 
    N_PLAY_REPEAT(0);                                   // Set currently playing Index 2 to loop infinitely
                                                        
    for (temp= 0; temp < 16000; temp++)                 // MCU intentionally delays for 8 seconds, audio loops continuously
    {                                                   
        HOST_Delay500uS();     
    }
    
    N_STOP_REPEAT();                                    // Disable loop status (Will stop after this playback iteration)
    I2C_WaitPlayEND();                                  // Wait for the last iteration to finish
                                                        
                                                        
    N_CHECK_INDEX_RCOUNT(5000);                         // Check if Index 5000 actually has audio resources loaded
    N_GET_INDEX_RCOUNT(&u8RESOURCE_COUNT);              // Read back resource count
    if (u8RESOURCE_COUNT > 0)                           // If > 0, means this audio exists
    {
    	N_PLAY(5000);                                   // Play Index 5000 Audio 
        I2C_WaitPlayEND();                              // Wait for playback to finish
    }                                                     
    while(1);
}

/*----- Multi-series Playback Example (Seamless voice broadcast, e.g.: "Number", "Three") -----*/
void I2C_MultiPlaySample(void)
{
    UINT8 MultiPlayBuffer[10] = {1,1,2,2,3,3,4,4,5,5};  // Prepare an 8-bit playback list
    UINT16 MultiPlay2BBuffer[4] = {257,1,300,2};  	    // Prepare a 16-bit playback list (If Index exceeds 255)
    
    HOST_BUS_Init();                                    // Host Initialization
    
    N_MULTI_PLAY(6,&MultiPlayBuffer[0]);                // Instruct NSP to sequentially play the first 6 audios (Index 1->1->2->2->3->3)
    I2C_WaitPlayEND();                                  // Blocking wait for the whole series to finish
                                                        
    N_MULTI_PLAY_2B(3,&MultiPlay2BBuffer[0]);           // Instruct continuous playback of first 3 16-bit audios (Index 257->1->300)
    N_PLAY_REPEAT(2);                                   // And request this "entire combination" to repeat 2 times
    I2C_WaitPlayEND();                                  // Blocking wait for playback to finish                                                                  
    while(1);
}

/*----- Sleep Mode and Wake-up Control Example -----*/
void I2C_SleepWakeUpSample(void)
{
    UINT16 temp = 0;                                    				
    HOST_BUS_Init();                                    
    
    N_PLAY_SLEEP(1);                                    // Play Index 1 Audio, NSP auto-enters sleep mode after finishing
    for (temp= 0; temp < 14000; temp++)                 // Delay 7 seconds, you can measure NSP's low power current now
    {                                                   
         HOST_Delay500uS();
    }
    
    N_WAKUP();                                          // Wake up NSP
    N_AUTO_SLEEP_DIS();                                 // Disable idle auto-sleep function (Must use N_PWR_DOWN command to sleep)
                                                                                                            
    N_PLAY(2);                                          // Play Index 2 Audio 
    for (temp= 0; temp < 8000; temp++)                  // Let it play for 4 seconds
    {                                                   
         HOST_Delay500uS();
    }
    N_STOP();                                           // Force interrupt playback
    N_PWR_DOWN();                                       // Issue command to force deep sleep (Because auto-sleep was disabled)
                                                        
    for (temp= 0; temp < 8000; temp++)                  // Delay 4 seconds
    {                                                   
         HOST_Delay500uS();
    }    
    
    N_WAKUP();                                          // Wake up again
    N_AUTO_SLEEP_EN();                                  // Re-enable auto-sleep function. Sleeps when idle beyond the GUI configured Timeout.
    
    N_PLAY(2);                                          // Play Index 2 Audio                                                   
    I2C_WaitPlayEND();                                  // Wait for finish. Ignore it afterward, NSP will auto-sleep when time is up.
    while(1);
}

/*----- Internal Working Voltage Read Example -----*/
void I2C_LowPowerDetectionSample(void)
{
    UINT8 temp = 0;                                     
    u8LVD_VALUE=0;                                      
	
    HOST_BUS_Init();                                    
	
    N_DO_LVD();	                                        // Start ADC to detect internal voltage
    for (temp= 0; temp <16; temp++)                     // Wait for hardware detection to complete (Approx 8ms)
    {                                                   
        HOST_Delay500uS();
    }
		
    N_GET_LVD(&u8LVD_VALUE);                            // Read back voltage segmented result	
                                                        // Reference Table:
                                                        // 01H: VDD < 2.1V
                                                        // 02H: 2.1V = VDD < 2.4V
                                                        // 04H: 2.4V = VDD < 2.7V
                                                        // 08H: 2.7V = VDD < 3.0V
                                                        // 10H: 3.0V = VDD < 3.3V
                                                        // 20H: 3.3V = VDD
    while(1);
}

/*----- Remote Control NSP IO Pin Example -----*/
void I2C_NSP_IO_CtrlSample(void)
{
    u8IO_FLAG = 0;                                     
    u8IO_VALUE = 0x00;                                 
    	
    HOST_BUS_Init();                                   
    							
    N_IO_CONFIG(0xF3);                                 // Configure GPIO functions on NSP chip peripherals via M031.
                                                       // For example, set some pins as Input, some as Output.
    N_IO_TYPE(&u8IO_FLAG);                             // Verify reading back the IO mode just configured

    N_SET_OUT(0x00);                                   // Pull all pins configured as Output to Low
    N_GET_INOUT(&u8IO_VALUE);                          // Read the level status of Input pins triggered externally
    while(1);
}

/*----- ROM Data Integrity CheckSum Verification Example -----*/
void I2C_CheckSumSample(void)
{
    UINT16 temp = 0;                                   
    CHECKSUM_BIT=0;                                    
    CHECKSUM_BYTES=0;

    HOST_BUS_Init();                                   

    N_DO_CHECKSUM();                                   // Request NSP to perform full-area CheckSum calculation on internal Flash
    for (temp= 0; temp <3000; temp++)                  // Calculation takes longer time, wait about 2~3 seconds
    {
         HOST_Delay500uS();
         HOST_Delay500uS();
    }

    N_CHECKSUM_RIGHT(&CHECKSUM_BIT);                   // Query comparison result (0: Correct and intact, 1: Data error)
    N_GET_CHECKSUM(&CHECKSUM_BYTES);                   // If needed, can also directly read back the 16-bit raw result value
							
    while(1);
}

/*----- Busy Signal Pin (Busy Pin) Output Example -----*/
void I2C_BusyPinSetSample(void)
{
    HOST_BUS_Init();                                    
    N_BZPIN_EN();                                       // Enable Busy Pin hardware pin. Pulls Low when playing, High when finished.
    N_PLAY(1);                                          // Start playback, hardware pin will pull to low level
    I2C_WaitPlayEND();                                  // After finishing, hardware pin restores to high level
    N_BZPIN_DIS();                                      // Disable this function, the pin reverts to standard IO
    while(1);
}

/*----- Remote Volume Control (Fade Out) Example -----*/
void I2C_VolumeCtrlSample(void)
{
    UINT16 temp = 0;                                    
    u8SP_VOL = 0;                                       

    HOST_BUS_Init();                                    
    N_PLAY(1);                                          // Play audio
    N_PLAY_REPEAT(0);                                   // Loop playback
				
    N_GET_VOL(&u8SP_VOL);                               // Read current initial volume
				
    while ( u8SP_VOL >= 0x20 )	                        // Utilize loop to achieve volume fade-out effect
    {                                                   // When volume is greater than 0x20:
          u8SP_VOL -= 0x10 ;                            // Decrease volume by 0x10
          N_SET_VOL(u8SP_VOL);                          // Send setting command to NSP
          for (temp= 0; temp <3000; temp++)		        // Wait for 3 seconds
          {					
             HOST_Delay500uS();
             HOST_Delay500uS();
          }	
    } 
    
    N_STOP_REPEAT();                                    // Stop playback

    while(1);       
}

/*----- Read Version and Product ID Example -----*/
void I2C_ReadIdAndFwVerSample(void)
{
    HOST_BUS_Init();                                    
    N_GET_FW_VERSION(&u32NSP_FW_VERSION);               // Get firmware version number		 
    N_READ_ID(&u32NSP_ID);                              // Get hardware ID
    if(u32NSP_ID == 0xD4C3B2A1)                         // Assuming this ID is the correct verification code
    {                                                   // Anti-piracy verification protection can be implemented here
       //.......
    }
	while(1);                                              
}

/*----- Mixed Application Scenario 1: Control IO pin to achieve synchronized audio and light -----*/
void I2C_MixedSample1(void)
{
    HOST_BUS_Init();                                    
    N_IO_CONFIG(0x7F);                                  // Set BP07 to output mode
    N_PLAY(1);                                          // Start playing audio
    N_SET_OUT(0x7F);                                    // Turn on the LED connected to BP07 (Pull to low level)
    I2C_WaitPlayEND();                                  // Wait for audio to finish
    N_SET_OUT(0xFF);                                    // Audio finished, turn off LED (Pull to high level)
    while(1);
}

/*----- Mixed Application Scenario 2: Non-blocking polling, retains MCU processing capability -----*/
void I2C_MixedSample2(void)
{
    HOST_BUS_Init();                                    
    N_PLAY(1);                                          // Play audio
    while(1) // M031 Multitasking main loop
    {
        // Tasks like keypad scanning, display updating, sensor reading can be processed here
        // ...
        
        // Every time the loop passes by, check if the audio has finished playing
        if(I2C_AskStatus())                         
        {
            break;                                      // If already finished, exit this checking loop.
        }
    }
    while(1);
}

/*----- Mixed Application Scenario 3: Comprehensive Demonstration -----*/
void I2C_MixedSample3(void)
{
    PUINT8 PLAY_BUFFER = 0;
    UINT8 g_au8MultiPlayBuf[MULTI_PLAY_MAX]={3,1,2,4,8,6,5,7,10,9,0,0,0,0,0,0,
                                             0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
					
    HOST_BUS_Init();                                    

    if(N_READ_ID(&u32NSP_ID) == 1)                      // Confirm normal connection and successful ID read first
    {                                                   
         if(u32NSP_ID == 0xD4C3B2A1)                    // Confirm model
         {                                              
               UINT16 temp = 0;                         			
               N_GET_FW_VERSION(&u32NSP_FW_VERSION);    // Read version
               N_GET_MAX_INDEX(&u16MAX_INDEX);		    // Get maximum built-in audio file indices in this chip		 		 
               N_SET_VOL(0x50);                         // Adjust appropriate volume	

               PLAY_BUFFER = g_au8MultiPlayBuf;				
               N_MULTI_PLAY(4,PLAY_BUFFER);             // Combinational playback of first 4 			
               N_PLAY_REPEAT(2);                        // Repeat this playlist combination 2 times

               I2C_WaitPlayEND();                       // Wait to finish         
                                                        
               // Alternately test playing the audio under every index
               for (u16PlayListIndex= 1; u16PlayListIndex <= u16MAX_INDEX; u16PlayListIndex++) 
               {                                        
                        N_PLAY(u16PlayListIndex);       
                        I2C_WaitPlayEND();          
               }

               // Each audio only plays for 5 seconds before switching to the next
               for (u16PlayListIndex= 1; u16PlayListIndex <= u16MAX_INDEX; u16PlayListIndex++)
               {                                        
                        N_PLAY(u16PlayListIndex);       
                        for (temp= 0; temp < 10000; temp++) { HOST_Delay500uS(); }
                        N_STOP();                       
               }

               N_PWR_DOWN();                            // Power off sleep test	
               for (temp= 0; temp < 2000; temp++) { HOST_Delay500uS(); }

               N_WAKUP();                                // Wake up
               
               // Playback test integrated with sleep functionality
               for (u16PlayListIndex= 1; u16PlayListIndex <= u16MAX_INDEX; u16PlayListIndex++) 
               {									
                        N_PLAY_SLEEP(u16PlayListIndex); // Go to sleep after finishing this one
                        for (temp= 0; temp < 14000; temp++) { HOST_Delay500uS(); }
                        N_WAKUP();                      // Wake it up to play the next one after 7 seconds of sleep
               }
         }
    }
    while(1);
}

//===========================================================
// Empty ISP programming function (For implementation connecting external SPI Flash read)
void Flash_Read(UINT32 *psFlashHandler,UINT32 u32ByteAddr,PUINT8 pau8Data,UINT32 u32DataLen) { }
void Flash_Write(UINT32 *psFlashHandler,UINT32 u32WriteAddr,PUINT8 pau8Data,UINT32 u32DataLen) { }

/*----- (ISP Function) Full online firmware and audio update example -----*/
void I2C_ISPUpdateAllResourceSample(void)
{
    // ... Original manufacturer's ISP update demo code, reads bin file through external Flash and calls N_ISP_WRITE_PAGE to program into NSP step by step.
    // ... If your product does not require online cloud updates for audio, this example can be skipped.
    while(1);
}

/*----- (ISP Function) Individual audio resource update example -----*/
void I2C_ISPUpdateOneResourceSample(void)
{
    // ... Original manufacturer's ISP partial update demo code
    while(1);
}

/*----- (Touch Function) Update touch configuration parameter file example -----*/
UINT8 I2C_TouchUpdataSample(UINT16 u16ConfigSize)
{
    HOST_BUS_Init();                                    
	Flash_Read(&g_sFlash, 0, g_au8Buf, u16ConfigSize);  // Assume parameters are stored in external Flash
    N_WAKUP();                                                  
    N_AUTO_SLEEP_DIS();                                 												
	return N_TOUCH_UPDATA(0, u16ConfigSize, g_au8Buf);  // Issue touch update command
}

/*----- (Touch Function) Read current touch configuration example -----*/
UINT8 I2C_TouchLoadSample(UINT16 u16ConfigSize)
{
    UINT8 u8RightCMD = 0;
    HOST_BUS_Init();                                     
    N_WAKUP();                                                  
    N_AUTO_SLEEP_DIS();                                 													
	u8RightCMD = N_TOUCH_LOAD(0, u16ConfigSize, g_au8Buf);   
	if( u8RightCMD )
	{	
		Flash_Write(&g_sFlash, 0, g_au8Buf, u16ConfigSize); // If successfully read, back it up and store into external Flash
	}
	return u8RightCMD;
}

//===========================================================
/*----- Pause and Continue Play (Continue Play) Example -----*/
void I2C_ContinuePlaySample(void)
{
    UINT16 temp = 0;                                    
    UINT8 MultiPlayBuffer[5] = {7,5,3,4,1};             
    UINT16 MultiPlay2BBuffer[3] = {6,300,2};  	                                                                                     	
    HOST_BUS_Init();                                    
    N_AUTO_SLEEP_DIS();                                 
    
    N_PLAY(1);                                          // Play background music first (Index 1) 
    N_PLAY_REPEAT(2);                                   // Let background music play 2 times                                                    
    for (temp= 0; temp < 16000; temp++)                 // After playing for 8 seconds...
    {                                                   
        HOST_Delay500uS();     
    }    
    N_PAUSE(); 						                    // Sudden interruption! Pause background music first
                                                        
    N_MULTI_PLAY(5,&MultiPlayBuffer[0]);                // Start playing interrupted audio (Index 7->5->3->4->1)
    for (temp= 0; temp < 8000; temp++)                  
    {                                                   
        HOST_Delay500uS();     
    }   
    
    N_MULTI_PLAY_2B(3,&MultiPlay2BBuffer[0]);           // Continue interrupting with other voice
    I2C_WaitPlayEND();                                  // Wait for all interruptions to finish
                                                        
    N_RESUME(); 					                    // Call "Resume" command to continue playing the paused background music (Index 1) from the break point                                                  
    I2C_WaitPlayEND();                                  
    N_AUTO_SLEEP_EN();                                  
    while(1);
}

//===========================================================
/*----- Utilize NSP internal Flash as MCU EEPROM storage space (Write) -----*/
UINT8 I2C_UserDataWrite(UINT32 UserWrite_ADDR,PUINT8 ISP_BUFFER,UINT16 WriteSize)
{
    // ... Provided by original manufacturer, when M031's own Flash capacity is insufficient, you can borrow NSP chip space to store custom parameters
    return 0; // Refer to source code for specific implementation
}

/*----- Utilize NSP internal Flash as MCU EEPROM storage space (Read) -----*/
UINT8 I2C_UserDataRead(UINT32 UserRead_ADDR,PUINT8 ISP_BUFFER,UINT16 ReadSize)
{
    return 0; // Refer to source code for specific implementation
}

/*----- Borrowing storage space demonstration example -----*/
void I2C_ISPUserSpaceWriteAndRead(void)
{							
    UINT8 RTN = 0;					
    UINT8 g_au8Buf_Write[10]= {0x5A,0xA5,0x55,0xAA,0x9C,0x87,0x2B,0x64,0xF7,0x96};

    // Write 1 Byte, then read back 1 Byte to verify
    RTN = I2C_UserDataWrite(2,&g_au8Buf_Write[0],1);  
    RTN = I2C_UserDataRead(2,&g_au8Buf_Read[5],1);
    
    // Write 512 Bytes, then read back to verify
    RTN = I2C_UserDataWrite(0,&g_au8Buf[0],512);  
    RTN = I2C_UserDataRead(0,&g_au8Buf_Read[0],512);								
    
    while(1);
}
#if 0
// =====================================================================
// M031 System and Pin Initialization (Preparation before Main entry)
// =====================================================================
void SYS_Init(void)
{
    // Unlock protected registers
    SYS_UnlockReg();

    // Enable internal 48MHz oscillator (HIRC)
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);
    
    // Set system clock source to HIRC
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    // Configure I2C0 pins (Assuming PB.4 as SDA, PB.5 as SCL, please modify according to actual hardware)
    SYS->GPB_MFPL = (SYS->GPB_MFPL & ~(SYS_GPB_MFPL_PB4MFP_Msk | SYS_GPB_MFPL_PB5MFP_Msk)) |
                    (SYS_GPB_MFPL_PB4MFP_I2C0_SDA | SYS_GPB_MFPL_PB5MFP_I2C0_SCL);

    // Lock protected registers
    SYS_LockReg();
}

// =====================================================================
// Test and Example Main Program (Test Code & Sample Code)
// =====================================================================
int main(void)
{
    // 1. M031 System and Pin Initialization
    SYS_Init();

    // 2. Initialize SysTick to generate 1ms interrupt (Used to drive I2C timeout protection and precise delay)
    SysTick_Config(SystemCoreClock / 1000);

    // 3. I2C communication hardware initialization
    HOST_BUS_Init();
    
    // Wait for NSP chip to boot and be ready
    CLK_SysTickDelay(100000); // Delay 100ms

    // =========================================
    // Basic Test Code: Simple Playback and Force Stop
    // =========================================
    
    // Set maximum volume (Range: 0~128)
    N_SET_VOL(128);
    CLK_SysTickDelay(5000); 

    // Send command to play audio at Index 1
    N_PLAY(1);

    // MCU host continues executing other tasks...
    // Intentionally wait for 3 seconds here (Simulating MCU doing other things)
    CLK_SysTickDelay(3000000); // 3000ms

    // After 3 seconds, regardless of whether audio finished, force interrupt and stop playback
    N_STOP();

    // Delay 1 second for separation
    CLK_SysTickDelay(1000000); 

    // =========================================
    // Advanced Sample Code: Non-blocking state polling architecture
    // =========================================
    
    // Send command to play audio at Index 2
    N_PLAY(2);

    // Utilize I2C_AskStatus() function to poll play status.
    // The advantage of this coding style is that the MCU is not stuck dead waiting here, it can do other things inside the loop.
    while(I2C_AskStatus() == 0) // Status return 0 means "Currently playing" or "Command transmission in progress"
    {
        // MCU's other daily tasks can be placed here
        // Example:
        // Scan_Keypad();     // Scan external keypad
        // Update_Display();  // Update LED or LCD screen
    }
    // As soon as it exits the loop above, it means I2C_AskStatus() returned 1, which means "Playback is complete"!

    // Main program enters infinite loop
    while(1)
    {
        // ... Other background task routines
    }
}
#endif