#ifndef _NSP_PLAY_SAMPLE_H_
#define _NSP_PLAY_SAMPLE_H_

/* exact-width unsigned integer types */
//typedef unsigned          char UINT8;
//typedef unsigned short     int UINT16;
//typedef unsigned           int UINT32;
//typedef unsigned char *		PUINT8;
//typedef unsigned short int  *	PUINT16;

/*----- Waiting for playback to finish -----*/
void I2C_WaitPlayEND(void);
/*----- Waiting for playback to finish -----*/
UINT8 I2C_WaitExecutionEND(void);
/*----- Ask for status -----*/
UINT8 I2C_AskStatus(void);

/*----- Index Play Sample -----*/
void I2C_IndexPlaySample(void);
/*----- MultiPlay Sample -----*/
void I2C_MultiPlaySample(void);
/*----- Sleep Wake Up Sample -----*/
void I2C_SleepWakeUpSample(void);
/*----- Low Power Detection -----*/
void I2C_LowPowerDetectionSample(void);
/*----- NSP IO Control Sample -----*/
void I2C_NSP_IO_CtrlSample(void);
/*----- CheckSum Sample -----*/
void I2C_CheckSumSample(void);
/*----- Busy Pin Set Sample -----*/
void I2C_BusyPinSetSample(void);
/*----- Volume Control Sample -----*/
void I2C_VolumeCtrlSample(void);
/*----- Read ID & FW Version Sample -----*/
void I2C_ReadIdAndFwVerSample(void);
/*----- Mixed Sample 1 -----*/
void I2C_MixedSample1(void);
/*----- Mixed Sample 2 -----*/
void I2C_MixedSample2(void);
/*----- Mixed Sample 3 -----*/
void I2C_MixedSample3(void);
/*----- NSP ISP Update all resource Sample -----*/
void I2C_ISPUpdateAllResourceSample(void);
/*----- NSP ISP Update one resource Sample -----*/
void I2C_ISPUpdateOneResourceSample(void);
/*----- Continue Play Sample -----*/
void I2C_ContinuePlaySample(void);

/*----- NSP ISP Update user data Sample -----*/
UINT8 I2C_UserDataWrite(UINT32 UserWrite_ADDR,PUINT8 ISP_BUFFER,UINT16 WriteSize);
/*----- NSP ISP read user data Sample -----*/
UINT8 I2C_UserDataRead(UINT32 UserRead_ADDR,PUINT8 ISP_BUFFER,UINT16 ReadSize);
/*----- NSP ISP Update user space Sample -----*/
void I2C_ISPUserSpaceWriteAndRead(void);

/*----- NSP Updata touch config -----*/
UINT8 I2C_TouchUpdataSample(UINT16 u16ConfigSize);
/*----- NSP Load touch config -----*/
UINT8 I2C_TouchLoadSample(UINT16 u16ConfigSize);

#endif




