#include "nsp_driver.h"
#define HOST_TIMEOUT     (10)  
#define I2C_BUF_SIZE     (520)	
UINT8 u8WriteBuffer[I2C_BUF_SIZE];
UINT8 u8ReadBuffer[I2C_BUF_SIZE];
UINT16 u16TimeoutCounter = 0;
UINT16 u16CMD_TX_BYTE = 0;
UINT16 u16CMD_RX_BYTE = 0;
UINT8 u8RX_ERROR_COUNT = 0;

//===========================================================
//Systerm intial, please modify according to your MCU platform
void HOST_BUS_Init(void)
{
	u16TimeoutCounter = 0;
	// 設定開啟I2C,設定頻率 and Slave address.
}

//Systerm intial, please modify according to your MCU platform
UINT16 N_I2C_READ(UINT16 READ_SIZE)
{
	if( u16TimeoutCounter >= HOST_TIMEOUT )
		return 0;
	// 設定讀取I2C資料, u8ReadBuffer 为接收FIFO数据的缓存区,u16CMD_RX_BYTE 为读取的字节数。
	return READ_SIZE;
}

//Systerm intial, please modify according to your MCU platform
UINT16 N_I2C_WRITE(UINT16 WRITE_SIZE)
{
	if( u16TimeoutCounter >= HOST_TIMEOUT )
		return 0;	
	// 设定发送I2C资料,u8WriteBuffer 为缓存区将数据发送到FIFO发送,u16CMD_TX_BYTE 为写入的字节数
	return WRITE_SIZE;
}
//----------------------------------
//Systerm intial, please modify according to your MCU platform, during which you can perform other operations on host
void HOST_I2C_WRITE_DURATION(void)
{
//设置I2C写延迟时间;  Delay time
}
//Systerm intial, please modify according to your MCU platform, during which you can perform other operations on host
void HOST_I2C_READ_DURATION(void)
{
//设置I2C读延迟时间;  Delay time
}
//Systerm intial, please modify according to your MCU platform, during which you can perform other operations on host
void HOST_CMD_INTERVAL(void)
{
//设置延迟时间2ms;  Delay time 2ms
}
//Systerm intial, please modify according to your MCU platform, during which you can perform other operations on host
void HOST_Delay500uS(void)
{
//设置延迟时间500us;  Delay time 500us
}
//===========================================================
void HOST_Init(UINT8* SP_VOL)
{
	*SP_VOL = 0x80;	
}
//===========================================================
void HOST_ADD_TIMEOUT_COUNTER(void)
{
	u16TimeoutCounter++;
}
void HOST_CLR_TIMEOUT_COUNTER(void)
{
	u16TimeoutCounter = 0;
}
//------------------------------------------------
UINT8 CHECK_I2C_READ_RIGHT()
{
	UINT16 i = 0;
	UINT8 CMD_CHECKSUM = 0;
	
	if (u16CMD_RX_BYTE == 1)
	{
		if (u8ReadBuffer[0] == RIGHT_RTN)
		{
			u8RX_ERROR_COUNT = 0;
			return 1;
		}
		if (u8ReadBuffer[0] == ERROR_RTN)
		{
			u8RX_ERROR_COUNT ++;
			return 0;
		}	
		if (u8ReadBuffer[0] == UNSUPPORTED_RTN)
		{
			u8RX_ERROR_COUNT = 0;
			return 0xFF;
		}
	}
	else
	{
		for(i = 0; i < (u16CMD_RX_BYTE-1); i++)
		{
			CMD_CHECKSUM += u8ReadBuffer[i];
		}
		if ((CMD_CHECKSUM ^ u8ReadBuffer[u16CMD_RX_BYTE-1]) == 0xFF)
		{
			u8RX_ERROR_COUNT = 0;
			return 1;
		}
		else
		{
			u8RX_ERROR_COUNT ++;
			return 0;
		}
	}	
	return 0;
}
//----------------------------------
UINT8 Host_CommonProcess(void)
{
	// Write(Send) Process.
	if( N_I2C_WRITE(u16CMD_TX_BYTE) )
	{
		HOST_I2C_WRITE_DURATION();
		// Read(Receive) Process.
		if( N_I2C_READ(u16CMD_RX_BYTE) )
		{
			HOST_I2C_READ_DURATION();	
		}			
		else
			return 0;		
	}
	else
		return 0;
	// 
	return CHECK_I2C_READ_RIGHT();
}
//------------------------------------------------
void ADD_I2C_WRITE_CHECKSUM(UINT8 u8CMDBytes,UINT16 u16BufferBytes,UINT8 *DATA_BUFFER)
{	
	UINT16 i = 0;
	UINT8 CMD_CHECKSUM = 0;
	for(i = 0; i < u8CMDBytes; i++)
	{
		CMD_CHECKSUM = ((CMD_CHECKSUM + u8WriteBuffer[i]) & 0xFF);
	}
	for(i = 0; i < u16BufferBytes; i++)
	{
		u8WriteBuffer[u8CMDBytes+i] = *DATA_BUFFER;
		CMD_CHECKSUM = ((CMD_CHECKSUM + u8WriteBuffer[u8CMDBytes+i]) & 0xFF);
		DATA_BUFFER++;
	}
	u8WriteBuffer[u16CMD_TX_BYTE-1] = (CMD_CHECKSUM ^ 0xFF );
}
//------------------------------------------------
void ADD_I2C_WRITE_CHECKSUM_2B(UINT8 u8CMDBytes,UINT16 u16BufferBytes,UINT16 *DATA_BUFFER)
{	
	UINT16 i = 0;
	UINT8 CMD_CHECKSUM = 0;
	for(i = 0; i < u8CMDBytes; i++)
	{
		CMD_CHECKSUM = ((CMD_CHECKSUM + u8WriteBuffer[i]) & 0xFF);
	}
	for(i = 0; i < u16BufferBytes; i++)
	{
		u8WriteBuffer[u8CMDBytes+2*i] = (*DATA_BUFFER  & 0xFF00)>>8;
		CMD_CHECKSUM = ((CMD_CHECKSUM + u8WriteBuffer[u8CMDBytes+2*i]) & 0xFF);
		u8WriteBuffer[u8CMDBytes+2*i+1] = (*DATA_BUFFER  & 0xFF);
		CMD_CHECKSUM = ((CMD_CHECKSUM + u8WriteBuffer[u8CMDBytes+2*i+1]) & 0xFF);
		DATA_BUFFER++;
	}
	u8WriteBuffer[u16CMD_TX_BYTE-1] = (CMD_CHECKSUM ^ 0xFF );
}
//------------------------------------------------
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
void N_SLAVE_RESET(void)
{
	if(u8RX_ERROR_COUNT >=5) 
	{
		u8RX_ERROR_COUNT = 0;
		N_RESET();	
	}
}
//===========================================================
// function command
UINT8 N_READ_ID(UINT32* PID)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_READ_ID;
	u8WriteBuffer[1] = u8WriteBuffer[0] ^ 0xFF;
	u16CMD_TX_BYTE = CMD_READ_ID_TX_BYTE;
	u16CMD_RX_BYTE = CMD_READ_ID_RX_BYTE;
	
	RTN = Host_CommonProcess();
	if (RTN == 1)
	{
		 *PID = (u8ReadBuffer[0]<<24) | (u8ReadBuffer[1]<<16) | (u8ReadBuffer[2]<<8) | u8ReadBuffer[3];
	}
	HOST_CMD_INTERVAL();
	return RTN;
}
//----------------------------------
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
UINT8 N_PLAY(UINT16 PlayListIndex)
{
	UINT8 RTN = 0;
	u8WriteBuffer[0] = CMD_PLAY;
	u8WriteBuffer[1] = PlayListIndex >>8;
	u8WriteBuffer[2] = PlayListIndex & 0xFF;
	u8WriteBuffer[3] = ((u8WriteBuffer[0] + u8WriteBuffer[1]+ u8WriteBuffer[2])& 0xFF)^ 0xFF;
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
UINT8 N_PLAY_CHANNEL(UINT8 ChannelMsk, UINT16* PlayIndexArry)
{
	UINT8 RTN = 0, u8Count = 0, u8Checksum = 0;
	u8WriteBuffer[0] = CMD_PLAY_CHANNEL;
	u8WriteBuffer[1] = ChannelMsk;
	u8Checksum += u8WriteBuffer[0];
	u8Checksum += u8WriteBuffer[1];
	if( ChannelMsk&BIT0 )
	{
		u8WriteBuffer[2] = PlayIndexArry[0] >>8;
		u8Checksum += u8WriteBuffer[2];
		u8WriteBuffer[3] = PlayIndexArry[0] & 0xFF;	
		u8Checksum += u8WriteBuffer[3];
		u8Count = 1;
	}
	if( ChannelMsk&BIT1 )
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
//After the host sends STOP cmd,
//it will send READ_STATUS cmd every interval (HOST_CMD_INTERVAL), 
//and the loop will execute until the READ_STATUS cmd gets the correct result, 
//so as to ensure that the NSP STOP operation has been completed.
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
//After the host sends RESET cmd,
//it will send READ_STATUS cmd every interval (HOST_CMD_INTERVAL), 
//and the loop will execute until the READ_STATUS cmd gets the correct result, 
//so as to ensure that the NSP stop operation has been completed.
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
//After the host sends STOP cmd,
//it will send READ_STATUS cmd 500ms, 
//and the loop will execute until the READ_STATUS cmd gets the correct result, 
//so as to ensure that the NSP STOP operation has been completed.
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
//After the host sends STOP cmd,
//it will send READ_STATUS cmd 500ms, 
//and the loop will execute until the READ_STATUS cmd gets the correct result, 
//so as to ensure that the NSP STOP operation has been completed.
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
UINT8 N_TOUCH_UPDATA(UINT8 CS, UINT16 CNT, PUINT8 DATA_BUFFER)
{
	UINT8 RTN = 0, u8i;
	u8WriteBuffer[0] = CMD_TOUCH_UPDATA;
	u8WriteBuffer[1] = CS;
	u8WriteBuffer[2] = CNT>>8;
	u8WriteBuffer[3] = CNT;
	memcpy( &u8WriteBuffer[4], DATA_BUFFER, CNT );	
	// Cal checksum.
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
void N_WAKUP(void)
{
	UINT8 u8NSP_STATUS1 = 0;
	N_READ_STATUS(&u8NSP_STATUS1);
	HOST_CMD_INTERVAL();
}
