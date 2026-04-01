#define nsp23_id 0xaccccaaa

// Command ID
#define CMD_READ_ID                         0x10
#define CMD_READ_STATUS                     0x11
#define CMD_DO_LVD                          0x12
#define CMD_GET_LVD                         0x13
#define CMD_CHECKSUM_RIGHT                  0x14
#define CMD_GET_CHECKSUM                    0x15
#define CMD_DO_CHECKSUM                     0x16
#define CMD_GET_FW_VERSION                  0x17
#define CMD_READ_SLAVE_ADDR                 0x20	//(Only Support I2C protocol)
#define CMD_MULTI_PLAY_SILENCE              0x72
#define CMD_MULTI_PLAY_2B_SILENCE           0x73
#define CMD_PLAY_REPEAT                     0x74
#define CMD_GET_MAX_INDEX                   0x75
#define CMD_CHECK_INDEX_RCOUNT              0x76
#define CMD_GET_INDEX_RCOUNT                0x77
#define CMD_PAUSE                           0x78
#define CMD_RESUME                          0x79
#define CMD_MULTI_PLAY_2B                   0x7A
#define CMD_STOP_REPEAT                     0x7B
#define CMD_REPEAT                          0x7C
#define CMD_MULTI_PLAY                      0x7D
#define CMD_PLAY_SLEEP                      0x7E
#define CMD_PLAY                            0x7F
#define CMD_STOP                            0x80
#define CMD_RESET                           0x81
#define CMD_PWR_DOWN                        0x82
#define CMD_AUTO_SLEEP_EN                   0x83
#define CMD_AUTO_SLEEP_DIS                  0x84
#define CMD_IO_CONFIG                       0x88
#define CMD_IO_TYPE                         0x89
#define CMD_SET_OUT                         0x8A
#define CMD_GET_INOUT                       0x8B
#define CMD_BZPIN_EN                        0x8E
#define CMD_BZPIN_DIS                       0x8F
#define CMD_PLAY_CHANNEL                    0xA0
#define CMD_STOP_CHANNEL                    0xA1
#define CMD_PAUSE_CHANNEL                   0xA2
#define CMD_RESUME_CHANNEL                  0xA3
#define CMD_REC_INDEX_START                 0xB0
#define CMD_REC_STOP                        0xB1
#define CMD_TOUCH_READ_STATUS               0xC0
#define CMD_TOUCH_LOAD                      0xC8
#define CMD_TOUCH_UPDATA                    0xC9
#define CMD_ISP_WRITE_START                 0xD0
#define CMD_ISP_WRITE_END                   0xD1
#define CMD_ISP_WRITE_PAGE                  0xD2
#define CMD_ISP_READ_PAGE                   0xD3
#define CMD_ISP_CHECKSUM                    0xD4
#define CMD_ISP_READ_RES_INDEX              0xD5
#define CMD_ISP_GET_RES_INFO 	            0xD6
#define CMD_ISP_WRITE_PARTIAL_START         0xD7
#define CMD_ISP_WRITE_PARTIAL_BAK           0xD8
#define CMD_ISP_WRITE_PARTIAL               0xD9
#define CMD_ISP_READ_PARTIAL                0xDA
#define CMD_ISP_GET_USER_SPACE_INFO         0xDB
#define CMD_SET_VOL_NSP                     0xF0
#define CMD_GET_VOL                         0xF1
#define CMD_SET_CHANNEL_VOL                 0xF2
#define CMD_GET_CHANNEL_VOL                 0xF3
// TX Byte
#define CMD_READ_ID_TX_BYTE                 0x2 // 0x10
#define CMD_READ_STATUS_TX_BYTE             0x2 // 0x11
#define CMD_DO_LVD_TX_BYTE                  0x2 // 0x12
#define CMD_GET_LVD_TX_BYTE                 0x2 // 0x13
#define CMD_CHECKSUM_RIGHT_TX_BYTE          0x2 // 0x14
#define CMD_GET_CHECKSUM_TX_BYTE            0x2 // 0x15
#define CMD_DO_CHECKSUM_TX_BYTE             0x2 // 0x16
#define CMD_GET_FW_VERSION_TX_BYTE          0x2 // 0x17
#define CMD_READ_SLAVE_ADDR_TX_BYTE         0x2 // 0x20
#define CMD_MULTI_PLAY_SILENCE_TX_BYTE      0x4 // 0x72
#define CMD_MULTI_PLAY_2B_SILENCE_TX_BYTE   0x4 // 0x73
#define CMD_PLAY_REPEAT_TX_BYTE             0x5 // 0x74
#define CMD_GET_MAX_INDEX_TX_BYTE           0x2 // 0x75
#define CMD_CHECK_INDEX_RCOUNT_TX_BYTE      0x4 // 0x76
#define CMD_GET_INDEX_RCOUNT_TX_BYTE        0x2 // 0x77
#define CMD_PAUSE_TX_BYTE                   0x2 // 0x78
#define CMD_RESUME_TX_BYTE                  0x2 // 0x79
#define CMD_MULTI_PLAY_2B_TX_BYTE           0x3 // 0x7A
#define CMD_STOP_REPEAT_TX_BYTE             0x2 // 0x7B
#define CMD_REPEAT_TX_BYTE                  0x3 // 0x7C
#define CMD_MULTI_PLAY_TX_BYTE              0x4 // 0x7D
#define CMD_PLAY_SLEEP_TX_BYTE              0x4 // 0x7E
#define CMD_PLAY_TX_BYTE                    0x4 // 0x7F
#define CMD_STOP_TX_BYTE                    0x2 // 0x80
#define CMD_RESET_TX_BYTE                   0x2 // 0x81
#define CMD_PWR_DOWN_TX_BYTE                0x2 // 0x82
#define CMD_AUTO_SLEEP_EN_TX_BYTE           0x2 // 0x83
#define CMD_AUTO_SLEEP_DIS_TX_BYTE          0x2 // 0x84
#define CMD_IO_CONFIG_TX_BYTE               0x3 // 0x88
#define CMD_IO_TYPE_TX_BYTE                 0x2 // 0x89
#define CMD_SET_OUT_TX_BYTE                 0x3 // 0x8A
#define CMD_GET_INOUT_TX_BYTE               0x2 // 0x8B
#define CMD_BZPIN_EN_TX_BYTE                0x2 // 0x8E
#define CMD_BZPIN_DIS_TX_BYTE               0x2 // 0x8F
#define CMD_PLAY_CHANNEL_TX_BYTE            0x3 // 0xA0
#define CMD_STOP_CHANNEL_TX_BYTE            0x3 // 0xA1
#define CMD_PAUSE_CHANNEL_TX_BYTE           0x3 // 0xA2
#define CMD_RESUME_CHANNEL_TX_BYTE          0x3 // 0xA3
#define CMD_REC_INDEX_START_TX_BYTE         0x4 // 0xB0
#define CMD_REC_STOP_TX_BYTE                0x2 // 0xB1
#define CMD_TOUCH_READ_STATUS_TX_BYTE       0x3 // 0xC0
#define CMD_TOUCH_LOAD_TX_BYTE              0x5 // 0xC8
#define CMD_TOUCH_UPDATA_TX_BYTE            0x5 // 0xC9
#define CMD_ISP_WRITE_START_TX_BYTE         0x8 // 0xD0
#define CMD_ISP_WRITE_END_TX_BYTE           0x2 // 0xD1
#define CMD_ISP_WRITE_PAGE_TX_BYTE          0x205 // 0xD2
#define CMD_ISP_READ_PAGE_TX_BYTE           0x5 // 0xD3
#define CMD_ISP_CHECKSUM_TX_BYTE            0x2 // 0xD4
#define CMD_ISP_READ_RES_INDEX_TX_BYTE      0x4 // 0xD5
#define CMD_ISP_GET_RES_INFO_TX_BYTE        0x2 // 0xD6
#define CMD_ISP_WRITE_PARTIAL_START_TX_BYTE 0x2 // 0xD7
#define CMD_ISP_WRITE_PARTIAL_BAK_TX_BYTE   0x5 // 0xD8
#define CMD_ISP_WRITE_PARTIAL_TX_BYTE       0x7 // 0xD9
#define CMD_ISP_READ_PARTIAL_TX_BYTE        0x7 // 0xDA
#define CMD_ISP_GET_USER_SPACE_INFO_TX_BYTE 0x2 // 0xDB
#define CMD_SET_VOL_TX_BYTE                 0x3 // 0xF0
#define CMD_GET_VOL_TX_BYTE                 0x2 // 0xF1
#define CMD_SET_CHANNEL_VOL_TX_BYTE         0x3 // 0xF2
#define CMD_GET_CHANNEL_VOL_TX_BYTE         0x3 // 0xF3
// RX Byte
#define CMD_READ_ID_RX_BYTE                 0x5 // 0x10
#define CMD_READ_STATUS_RX_BYTE             0x2 // 0x11
#define CMD_DO_LVD_RX_BYTE                  0x1 // 0x12
#define CMD_GET_LVD_RX_BYTE                 0x2 // 0x13
#define CMD_CHECKSUM_RIGHT_RX_BYTE          0x2 // 0x14
#define CMD_GET_CHECKSUM_RX_BYTE            0x3 // 0x15
#define CMD_DO_CHECKSUM_RX_BYTE             0x1 // 0x16
#define CMD_GET_FW_VERSION_RX_BYTE          0x4 // 0x17
#define CMD_READ_SLAVE_ADDR_RX_BYTE         0x2 // 0x20
#define CMD_MULTI_PLAY_SILENCE_RX_BYTE      0x1 // 0x72
#define CMD_MULTI_PLAY_2B_SILENCE_RX_BYTE   0x1 // 0x73
#define CMD_PLAY_REPEAT_RX_BYTE             0x1 // 0x74
#define CMD_GET_MAX_INDEX_RX_BYTE           0x3 // 0x75
#define CMD_CHECK_INDEX_RCOUNT_RX_BYTE      0x1 // 0x76
#define CMD_GET_INDEX_RCOUNT_RX_BYTE        0x2 // 0x77
#define CMD_PAUSE_RX_BYTE                   0x1 // 0x78
#define CMD_RESUME_RX_BYTE                  0x1 // 0x79
#define CMD_MULTI_PLAY_2B_RX_BYTE           0x1 // 0x7A
#define CMD_STOP_REPEAT_RX_BYTE             0x1 // 0x7B
#define CMD_REPEAT_RX_BYTE                  0x1 // 0x7C
#define CMD_MULTI_PLAY_RX_BYTE              0x1 // 0x7D
#define CMD_PLAY_SLEEP_RX_BYTE              0x1 // 0x7E
#define CMD_PLAY_RX_BYTE                    0x1 // 0x7F
#define CMD_STOP_RX_BYTE                    0x1 // 0x80
#define CMD_RESET_RX_BYTE                   0x1 // 0x81
#define CMD_PWR_DOWN_RX_BYTE                0x1 // 0x82
#define CMD_AUTO_SLEEP_EN_RX_BYTE           0x1 // 0x83
#define CMD_AUTO_SLEEP_DIS_RX_BYTE          0x1 // 0x84
#define CMD_IO_CONFIG_RX_BYTE               0x1 // 0x88
#define CMD_IO_TYPE_RX_BYTE                 0x2 // 0x89
#define CMD_SET_OUT_RX_BYTE                 0x1 // 0x8A
#define CMD_GET_INOUT_RX_BYTE               0x2 // 0x8B
#define CMD_BZPIN_EN_RX_BYTE                0x1 // 0x8E
#define CMD_BZPIN_DIS_RX_BYTE               0x1 // 0x8F
#define CMD_PLAY_CHANNEL_RX_BYTE            0x1 // 0xA0
#define CMD_STOP_CHANNEL_RX_BYTE            0x1 // 0xA1
#define CMD_PAUSE_CHANNEL_RX_BYTE           0x1 // 0xA2
#define CMD_RESUME_CHANNEL_RX_BYTE          0x1 // 0xA3
#define CMD_REC_INDEX_START_RX_BYTE         0x1 // 0xB0
#define CMD_REC_STOP_RX_BYTE                0x1 // 0xB1
#define CMD_TOUCH_READ_STATUS_RX_BYTE       0x1 // 0xC0
#define CMD_TOUCH_LOAD_RX_BYTE              0x1 // 0xC8
#define CMD_TOUCH_UPDATA_RX_BYTE            0x1 // 0xC9
#define CMD_ISP_WRITE_START_RX_BYTE         0x1 // 0xD0
#define CMD_ISP_WRITE_END_RX_BYTE           0x1 // 0xD1
#define CMD_ISP_WRITE_PAGE_RX_BYTE          0x1 // 0xD2
#define CMD_ISP_READ_PAGE_RX_BYTE           0x201 // 0xD3
#define CMD_ISP_CHECKSUM_RX_BYTE            0x3 // 0xD4
#define CMD_ISP_READ_RES_INDEX_RX_BYTE      0x1 // 0xD5
#define CMD_ISP_GET_RES_INFO_RX_BYTE        0xD // 0xD6
#define CMD_ISP_WRITE_PARTIAL_START_RX_BYTE 0x1 // 0xD7
#define CMD_ISP_WRITE_PARTIAL_BAK_RX_BYTE   0x1 // 0xD8
#define CMD_ISP_WRITE_PARTIAL_RX_BYTE       0x1 // 0xD9
#define CMD_ISP_READ_PARTIAL_RX_BYTE        0x1 // 0xDA
#define CMD_ISP_GET_USER_SPACE_INFO_RX_BYTE 0xD // 0xDB
#define CMD_SET_VOL_RX_BYTE                 0x1 // 0xF0
#define CMD_GET_VOL_RX_BYTE                 0x2 // 0xF1
#define CMD_SET_CHANNEL_VOL_RX_BYTE         0x1 // 0xF2
#define CMD_GET_CHANNEL_VOL_RX_BYTE         0x1 // 0xF3

//-----------------------------------------
#define RIGHT_RTN                           0xA5
#define ERROR_RTN                           0xFA
#define UNSUPPORTED_RTN	                    0x5F
//-----------------------------------------
#if 0
#define BIT0                                0x1
#define BIT1                                0x2
#define BIT2                                0x4
#define BIT3                                0x8
#define BIT4                                0x10
#define BIT5                                0x20
#define BIT6                                0x40
#define BIT7                                0x80
#endif
//-----------------------------------------
#define SP_BUSY                             BIT7
#define CMD_VALID                           BIT6

#define FLAG_TX_DOING                       BIT5
#define FLAG_RX_DOING                       BIT4
#define FLAG_SPBZOUT_EN                     BIT0
//-----------------------------------------
typedef unsigned char UINT8;
typedef unsigned short int UINT16;
typedef unsigned int UINT32;
typedef unsigned char* PUINT8;
typedef unsigned short int* PUINT16;
//-----------------------------------------
void HOST_BUS_Init(void);
UINT16 N_I2C_READ(UINT16 READ_SIZE);
UINT16 N_I2C_WRITE(UINT16 WRITE_SIZE);
void HOST_Init(UINT8* SP_VOL);
void HOST_I2C_WRITE_DURATION(void);
void HOST_I2C_READ_DURATION(void);
void HOST_CMD_INTERVAL(void);
void HOST_Delay500uS(void);
void HOST_ADD_TIMEOUT_COUNTER(void);
void HOST_CLR_TIMEOUT_COUNTER(void);
void N_SLAVE_RESET(void);
// Backward with Other communcation.
#define HOST_SYS_Init HOST_BUS_Init
//-----------------------------------------
/*
Function Name:          N_READ_ID
Function Prototype:     UINT8 N_READ_ID(UINT32* PID)
Functional Description: Read identification typed in playlist generation.
Input Parameters:       None.
Output Parameters:      PID:Product ID in NSP.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_READ_ID(UINT32* PID);
/*
Function Name:          N_GET_FW_VERSION
Function Prototype:     UINT8 N_GET_FW_VERSION(UINT32* PFW_VERSION)
Functional Description: Get firmware version.
Input Parameters:       None.
Output Parameters:      PFW_VERSION:Version of firmware.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_GET_FW_VERSION(UINT32* PFW_VERSION);
/*
Function Name:          N_READ_STATUS
Function Prototype:     UINT8 N_READ_STATUS(UINT8* COMMAND_STATUS)
Functional Description: Query the value of status register of NSP.
Input Parameters:       None.
Output Parameters:      COMMAND_STATUS:Status of NSP.
                        - BIT7:Playback status(1:NSP is playing speech. 0:NSP has stopped playing speech.)
                        - BIT6:The result of received command in previous transaction.
                        - BIT5:Reserved.
                        - BIT4:Reserved.
                        - BIT3:Reserved.
                        - BIT2:Reserved.
                        - BIT1:Channel 2 playback status(1:NSP is playing speech on channel 2. 0:NSP has stopped playing speech on channel 2.)
                        - BIT0:Channel 1 playback status(1:NSP is playing speech on channel 1. 0:NSP has stopped playing speech on channel 1.)
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_READ_STATUS(UINT8* COMMAND_STATUS);
/*
Function Name:          N_READ_SLAVE_ADDR
Function Prototype:     UINT8 N_READ_SLAVE_ADDR(UINT8* ADDR)
Functional Description: Obtain the Slave address of NSP via global broadcast(slave address = 0).
Input Parameters:       None.
Output Parameters:      ADDR:the Slave address of NSP.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_READ_SLAVE_ADDR(UINT8* ADDR);
//-----------------------------------------
/*
Function Name:          N_PLAY
Function Prototype:     UINT8 N_PLAY(UINT16 PlayListIndex)
Functional Description: Play the selected playlist specified by the Index-High and Index-Low byte which follows the PLAY command. 
                        The Index-High and Index-Low byte indicate the playlist number.
Input Parameters:       PlayListIndex:List index of prparing play.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_PLAY(UINT16 PlayListIndex);
/*
Function Name:          N_PLAY_SLEEP
Function Prototype:     UINT8 N_PLAY_SLEEP(UINT16 PlayListIndex)
Functional Description: Play the selected playlist specified by the Index-High and Index-Low byte which follows the PLAY command. 
                        The Index-High and Index-Low byte indicate the playlist number. 
                        Then NSP enters standby mode after playback is complete.
Input Parameters:       PlayListIndex:List index of prparing play.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_PLAY_SLEEP(UINT16 PlayListIndex);
/*
Function Name:          N_PLAY_WITH_REPEAT
Function Prototype:     UINT8 N_PLAY_WITH_REPEAT(UINT16 PlayListIndex, UINT8 REPEAT_COUNT)
Functional Description: Repeat play the selected playlist specified by the Index-High and Index-Low byte which follows the PLAY command. 
                        The Index-High and Index-Low byte indicate the playlist number.
Input Parameters:       PlayListIndex:index of prparing play.
                        REPEAT_COUNT:Set repeat play times.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_PLAY_WITH_REPEAT(UINT16 PlayListIndex, UINT8 REPEAT_COUNT);
/*
Function Name:          N_MULTI_PLAY
Function Prototype:     UINT8 N_MULTI_PLAY(UINT8 PlayListNum,PUINT8 DATA_BUFFER)
Functional Description: Play the selected playlist specified by multiple (number: 1~32,index:1~255) index after the PLAY command.
Input Parameters:       PlayListNum:Number of playback list.
                        DATA_BUFFER:Content of playback list.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_MULTI_PLAY(UINT8 PlayListNum,PUINT8 DATA_BUFFER);
/*
Function Name:          N_MULTI_PLAY_2B
Function Prototype:     UINT8 N_MULTI_PLAY_2B(UINT8 PlayListNum,PUINT16 DATA_BUFFER)
Functional Description: Play the selected playlist specified by multiple (number: 1~32,index:1~65535) index after the PLAY command.
Input Parameters:       PlayListNum:Number of playback list.
                        DATA_BUFFER:Content of playback list.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_MULTI_PLAY_2B(UINT8 PlayListNum,PUINT16 DATA_BUFFER);
/*
Function Name:          N_MULTI_PLAY_SILENCE
Function Prototype:     UINT8 N_MULTI_PLAY_SILENCE(UINT8 CH, UINT8 PlayListNum,PUINT8 PLAYINDEX_BUFFER,PUINT8 SILENCE_BUFFER)
Functional Description: Play the selected playlist specified by multiple (number: 1~32,index:1~255) index on the specified channel.
                        And after each Index is played, it will silence for the setting time(ms).
Input Parameters:       CH:Play channel(BIT0,BIT1).
                        PlayListNum:Number of playback list(1~32).
                        PLAYINDEX_BUFFER:Content of playback list(every index range:1~255).
                        SILENCE_BUFFER:Takes to silence after playing the index
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_MULTI_PLAY_SILENCE(UINT8 CH, UINT8 PlayListNum,PUINT8 PLAYINDEX_BUFFER,PUINT8 SILENCE_BUFFER);
/*
Function Name:          N_MULTI_PLAY_2B_SILENCE
Function Prototype:     UINT8 N_MULTI_PLAY_2B_SILENCE(UINT8 CH, UINT8 PlayListNum,PUINT16 PLAYINDEX_BUFFER,PUINT8 SILENCE_BUFFER)
Functional Description: Play the selected playlist specified by multiple (number: 1~32,index:1~65535) index on the specified channel.
                        And after each Index is played, it will silence for the setting time(ms).
Input Parameters:       CH:Play channel(BIT0,BIT1).
                        PlayListNum:Number of playback list(1~32).
                        PLAYINDEX_BUFFER:Content of playback list(every index range:1~65535).
                        SILENCE_BUFFER:Takes to silence after playing the index
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_MULTI_PLAY_2B_SILENCE(UINT8 CH, UINT8 PlayListNum,PUINT16 PLAYINDEX_BUFFER,PUINT8 SILENCE_BUFFER);
/*
Function Name:          N_STOP
Function Prototype:     UINT8 N_STOP(void)
Functional Description: Stop playback.
Input Parameters:       None.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_STOP(void);
//-----------------------------------------
/*
Function Name:          N_STOP_REPEAT
Function Prototype:     UINT8 N_STOP_REPEAT(void)
Functional Description: Stop playback after this time.
Input Parameters:       None.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_STOP_REPEAT(void);
/*
Function Name:          N_PLAY_REPEAT
Function Prototype:     UINT8 N_PLAY_REPEAT(UINT8 REPEAT_COUNT)
Functional Description: Repeat play.
Input Parameters:       REPEAT_COUNT:Set repeat play times.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_PLAY_REPEAT(UINT8 REPEAT_COUNT);
//-----------------------------------------
/*
Function Name:          N_PAUSE
Function Prototype:     UINT8 N_PAUSE(void)
Functional Description: Pause playback.
Input Parameters:       None.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_PAUSE(void);
/*
Function Name:          N_RESUME
Function Prototype:     UINT8 N_RESUME(void)
Functional Description: Resume playback.
Input Parameters:       None.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_RESUME(void);
//-----------------------------------------
/*
Function Name:          N_SET_VOL
Function Prototype:     UINT8 N_SET_VOL(UINT8 SP_VOL)
Functional Description: Set volume.
Input Parameters:       SP_VOL:Value of volume.(Range:0~0x80)
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_SET_VOL(UINT8 SP_VOL);
/*
Function Name:          N_GET_VOL
Function Prototype:     UINT8 N_GET_VOL(UINT8* SP_VOL)
Functional Description: Get the value of volume.
Input Parameters:       None.
Output Parameters:      SP_VOL:Read value of volume.(Range:0~0x80)
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_GET_VOL(UINT8* SP_VOL);
//-----------------------------------------
/*
Function Name:          N_PLAY_CHANNEL
Function Prototype:     UINT8 N_PLAY_CHANNEL(UINT8 ChannelMsk, UINT16* PlayIndexArry)
Functional Description: Start channel play.
Input Parameters:       ChannelMsk:Channel mask of star playing.
                        PlayIndexArry:Index arrary of playing list.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_PLAY_CHANNEL(UINT8 ChannelMsk, UINT16* PlayIndexArry);
/*
Function Name:          N_STOP_CHANNEL
Function Prototype:     UINT8 N_STOP_CHANNEL(UINT8 ChannelMsk)
Functional Description: Stop channel play.
Input Parameters:       ChannelMsk:Channel mask of stop playing.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_STOP_CHANNEL(UINT8 ChannelMsk);
/*
Function Name:          N_PAUSE_CHANNEL
Function Prototype:     UINT8 N_PAUSE_CHANNEL(UINT8 ChannelMsk)
Functional Description: Pause channel play.
Input Parameters:       ChannelMsk:Channel mask of pause playing.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_PAUSE_CHANNEL(UINT8 ChannelMsk);
/*
Function Name:          N_RESUME_CHANNEL
Function Prototype:     UINT8 N_RESUME_CHANNEL(UINT8 ChannelMsk)
Functional Description: Resume channel play.
Input Parameters:       ChannelMsk:Channel mask of resume playing.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_RESUME_CHANNEL(UINT8 ChannelMsk);
//-----------------------------------------
/*
Function Name:          N_CHECK_INDEX_RCOUNT
Function Prototype:     UINT8 N_CHECK_INDEX_RCOUNT(UINT16 PlayListIndex)
Functional Description: Read the number of sound sources in the selected playlist specified by the Index-High and Index-Low byte.
Input Parameters:       PlayListIndex:Index of playback list
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_CHECK_INDEX_RCOUNT(UINT16 PlayListIndex);
/*
Function Name:          N_GET_INDEX_RCOUNT
Function Prototype:     UINT8 N_GET_INDEX_RCOUNT(UINT8* RESOURCE_COUNT)
Functional Description: Get the number of sound sources in the index.
Input Parameters:       None.
Output Parameters:      RESOURCE_COUNT:Resource count of index which provided from the before command "N_CHECK_INDEX_RCOUNT"
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_GET_INDEX_RCOUNT(UINT8* RESOURCE_COUNT);
/*
Function Name:          N_GET_MAX_INDEX
Function Prototype:     UINT8 N_GET_MAX_INDEX(UINT16* MAX_INDEX)
Functional Description: Get max playlist index.
Input Parameters:       None.
Output Parameters:      MAX_INDEX:Max index of playback list.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_GET_MAX_INDEX(UINT16* MAX_INDEX);
//-----------------------------------------
/*
Function Name:          N_AUTO_SLEEP_EN
Function Prototype:     UINT8 N_AUTO_SLEEP_EN(void)
Functional Description: If no playing, no LVD and checksum execution, no new command is received within the standby time setting in the GUI,
                        NSP will automatically enter the low-power standby mode.
Input Parameters:       None.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_AUTO_SLEEP_EN(void);
/*
Function Name:          N_AUTO_SLEEP_DIS
Function Prototype:     UINT8 N_AUTO_SLEEP_DIS(void)
Functional Description: NSP will not automatically enter the low-power standby mode.
Input Parameters:       None.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_AUTO_SLEEP_DIS(void);
//-----------------------------------------
/*
Function Name:          N_CHECKSUM_RIGHT
Function Prototype:     UINT8 N_CHECKSUM_RIGHT(UINT8* CHECKSUM_BIT)
Functional Description: Check the NSP checksum right or wrong.
Input Parameters:       None.
Output Parameters:      CHECKSUM_BIT:Representing the correctness of the ROM data(0:Correct. 1:Error.)
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_CHECKSUM_RIGHT(UINT8* CHECKSUM_BIT);
/*
Function Name:          N_GET_CHECKSUM
Function Prototype:     UINT8 N_GET_CHECKSUM(UINT16* CHECKSUM_BYTES)
Functional Description: Get the NSP checksum value.
Input Parameters:       None.
Output Parameters:      CHECKSUM_BYTES:The checksum of the ROM data.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_GET_CHECKSUM(UINT16* CHECKSUM_BYTES);
/*
Function Name:          N_DO_CHECKSUM
Function Prototype:     UINT8 N_DO_CHECKSUM(void)
Functional Description: Do checksum.
Input Parameters:       None.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_DO_CHECKSUM(void);
//-----------------------------------------
/*
Function Name:          N_DO_LVD
Function Prototype:     UINT8 N_DO_LVD(void)
Functional Description: Do LVD check.
Input Parameters:       None.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_DO_LVD(void);
/*
Function Name:          N_GET_LVD
Function Prototype:     UINT8 N_GET_LVD(UINT8* LVD_STATUS)
Functional Description: Get the range of VDD voltage.
Input Parameters:       None.
Output Parameters:      LVD_STATUS:Read back the range of VDD voltage.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_GET_LVD(UINT8* LVD_STATUS);
//-----------------------------------------
/*
Function Name:          N_IO_CONFIG
Function Prototype:     UINT8 N_IO_CONFIG(UINT8 IO_FLAG)
Functional Description: Configure I/O type.
Input Parameters:       IO_FLAG:Configuation of IO.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_IO_CONFIG(UINT8 IO_FLAG);
/*
Function Name:          N_IO_TYPE
Function Prototype:     UINT8 N_IO_TYPE(UINT8* READ_IO_FLAG)
Functional Description: Read I/O type.
Input Parameters:       None.
Output Parameters:      READ_IO_FLAG:IO type of NSP.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_IO_TYPE(UINT8* READ_IO_FLAG);
/*
Function Name:          N_SET_OUT
Function Prototype:     UINT8 N_SET_OUT(UINT8 IO_VALUE)
Functional Description: Set the value of output pin.
Input Parameters:       IO_VALUE:Configure value of output pin.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_SET_OUT(UINT8 IO_VALUE);
/*
Function Name:          N_GET_INOUT
Function Prototype:     UINT8 N_GET_INOUT(UINT8* READ_IO_VALUE)
Functional Description: Get the value of I/O.
Input Parameters:       None.
Output Parameters:      READ_IO_VALUE: Read value of output pin.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_GET_INOUT(UINT8* READ_IO_VALUE);
//-----------------------------------------
/*
Function Name:          N_BZPIN_EN
Function Prototype:     UINT8 N_BZPIN_EN(void)
Functional Description: Enable the busy-pin function.
Input Parameters:       None.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_BZPIN_EN(void);
/*
Function Name:          N_BZPIN_DIS
Function Prototype:     UINT8 N_BZPIN_DIS(void)
Functional Description: Disable the busy-pin function.
Input Parameters:       None.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_BZPIN_DIS(void);
//-----------------------------------------
/*
Function Name:          N_RESET
Function Prototype:     UINT8 N_RESET(void)
Functional Description: NSP reset.
Input Parameters:       None.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_RESET(void);
/*
Function Name:          N_PWR_DOWN
Function Prototype:     UINT8 N_PWR_DOWN(void)
Functional Description: NSP enters standby mode. NSP will consume minimum power.
Input Parameters:       None.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_PWR_DOWN(void);
/*
Function Name:          N_WAKUP
Function Prototype:     void N_WAKUP(void)
Functional Description: Wake up when NSP is in sleep state.
Input Parameters:       None.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
void N_WAKUP(void);
//-----------------------------------------
/*
Function Name:          N_ISP_WRITE_START
Function Prototype:     UINT8 N_ISP_WRITE_START(UINT32 PROC_ID,UINT32 CHIP_PDID)
Functional Description: Start ISP write function.
Input Parameters:       PROC_ID:Product ID.
                        CHIP_PDID:Chip ID.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_ISP_WRITE_START(UINT32 PROC_ID,UINT32 CHIP_PDID);
/*
Function Name:          N_ISP_WRITE_PAGE
Function Prototype:     UINT8 N_ISP_WRITE_PAGE(UINT32 ISP_ADDR,PUINT8 ISP_BUFFER)
Functional Description: ISP write 512 bytes to the specified location in the flash.
Input Parameters:       ISP_ADDR:Write start address of ISP.
                        ISP_BUFFER:Write data for ISP.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_ISP_WRITE_PAGE(UINT32 ISP_ADDR,PUINT8 ISP_BUFFER);
/*
Function Name:          N_ISP_READ_PAGE
Function Prototype:     UINT8 N_ISP_READ_PAGE(UINT32 ISP_ADDR,PUINT8 ISP_BUFFER)
Functional Description: ISP read 512 bytes from a specified location in flash.
Input Parameters:       ISP_ADDR:Read start address of ISP.
                        ISP_BUFFER:Read data for ISP.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_ISP_READ_PAGE(UINT32 ISP_ADDR,PUINT8 ISP_BUFFER);
/*
Function Name:          N_ISP_WRITE_PARTIAL_START
Function Prototype:     UINT8 N_ISP_WRITE_PARTIAL_START(void)
Functional Description: Start ISP partial write function.
Input Parameters:       None.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_ISP_WRITE_PARTIAL_START(void);
/*
Function Name:          N_ISP_WRITE_PARTIAL_BAK
Function Prototype:     UINT8 N_ISP_WRITE_PARTIAL_BAK(UINT32 ISP_ADDR)
Functional Description: Page backup for ISP partial write.
Input Parameters:       ISP_ADDR:Bakup data address for ISP.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_ISP_WRITE_PARTIAL_BAK(UINT32 ISP_ADDR);
/*
Function Name:          N_ISP_WRITE_PARTIAL
Function Prototype:     UINT8 N_ISP_WRITE_PARTIAL(UINT32 ISP_ADDR,UINT16 ISPSize,PUINT8 ISP_BUFFER)
Functional Description: ISP write less than 512 bytes to the specified location in the flash.
Input Parameters:       ISP_ADDR:Address of ISP.
                        ISPSize:Data size of ISP.
                        ISP_BUFFER:Write data content of ISP.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_ISP_WRITE_PARTIAL(UINT32 ISP_ADDR,UINT16 ISPSize,PUINT8 ISP_BUFFER);
/*
Function Name:          N_ISP_READ_PARTIAL
Function Prototype:     UINT8 N_ISP_READ_PARTIAL(UINT32 ISP_ADDR,UINT16 ISPSize,PUINT8 ISP_BUFFER)
Functional Description: ISP read less than 512 bytes from a specified location in flash.
Input Parameters:       ISP_ADDR:Address of ISP.
                        ISPSize:Data size of ISP.
Output Parameters:      ISP_BUFFER:Read data content of ISP.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_ISP_READ_PARTIAL(UINT32 ISP_ADDR,UINT16 ISPSize,PUINT8 ISP_BUFFER);
/*
Function Name:          N_ISP_WRITE_END
Function Prototype:     UINT8 N_ISP_WRITE_END(void)
Functional Description: Start ISP write function.
Input Parameters:       None.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_ISP_WRITE_END(void);
/*
Function Name:          N_ISP_CHECKSUM
Function Prototype:     UINT8 N_ISP_CHECKSUM(UINT16* ISP_CHECKSUM_BYTES)
Functional Description: Get the checksum value written by the ISP.
Input Parameters:       None.
Output Parameters:      ISP_CHECKSUM_BYTES: Checksum value of ISP.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_ISP_CHECKSUM(UINT16* ISP_CHECKSUM_BYTES);
/*
Function Name:          N_ISP_READ_RES_INDEX
Function Prototype:     UINT8 N_ISP_READ_RES_INDEX(UINT16 ResourceIndex)
Functional Description: Read the selected resource list related information by the Index_High and Index_Low byte which follows the command. 
                        Index_High and Index_Low bytes indicate the actual resource list number.
Input Parameters:       ResourceIndex:Resource index of reading info.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_ISP_READ_RES_INDEX(UINT16 ResourceIndex);
/*
Function Name:          N_ISP_GET_RES_INFO
Function Prototype:     UINT8 N_ISP_GET_RES_INFO(UINT32* SpaceSize,UINT32* ResourceStartAddr,UINT16* FisrtPageDataByte,UINT16* PageCount,UINT16* LastPageDataByte)
Functional Description: Get the resource information of ISP_READ_RES_INDEX command.
Input Parameters:       None.
Output Parameters:      SpaceSize:Data size of resource information.
                        ResourceStartAddr:Write start address of resource information.
                        FisrtPageDataByte:Start byte of the first page.
                        PageCount:Data page counts.
                        LastPageDataByte:End byte of the last page.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_ISP_GET_RES_INFO(UINT32* SpaceSize,UINT32* ResourceStartAddr,UINT16* FisrtPageDataByte,UINT16* PageCount,UINT16* LastPageDataByte);
/*
Function Name:          N_ISP_GET_USER_SPACE_INFO
Function Prototype:     UINT8 N_ISP_GET_USER_SPACE_INFO(UINT32* SpaceSize,UINT32* ResourceStartAddr,UINT16* FisrtPageDataByte,UINT16* PageCount,UINT16* LastPageDataByte)
Functional Description: Get the resource information of user space.
Input Parameters:       None.
Output Parameters:      SpaceSize:Data size of user space.
                        ResourceStartAddr:Write start address of user space.
                        FisrtPageDataByte:Start byte of the first page.
                        PageCount:Data page counts.
                        LastPageDataByte:End byte of the last page.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_ISP_GET_USER_SPACE_INFO(UINT32* SpaceSize,UINT32* ResourceStartAddr,UINT16* FisrtPageDataByte,UINT16* PageCount,UINT16* LastPageDataByte);
//-----------------------------------------
/*
Function Name:          N_TOUCH_READSTATUS
Function Prototype:     UINT8 N_TOUCH_READSTATUS(UINT8 COUNT,UINT8* STATUS)
Functional Description: Read status of touch keys
Input Parameters:       COUNT:Count of touch keys.
Output Parameters:      STATUS:Status of touch keys.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_TOUCH_READSTATUS(UINT8 COUNT,UINT8* STATUS);
/*
Function Name:          N_TOUCH_UPDATA
Function Prototype:     UINT8 N_TOUCH_UPDATA(UINT8 CS, UINT16 CNT, PUINT8 DATA_BUFFER)
Functional Description: 
Input Parameters:       CS:Slave(default = 0)
                        CNT:Data count of touch configuration.
                        DATA_BUFFER:Download data content of touch configuration.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_TOUCH_UPDATA(UINT8 CS, UINT16 CNT, PUINT8 DATA_BUFFER);
/*
Function Name:          N_TOUCH_LOAD
Function Prototype:     UINT8 N_TOUCH_LOAD(UINT8 CS, UINT16 CNT, PUINT8 DATA_BUFFER)
Functional Description: 
Input Parameters:       CS:Slave(default = 0)
                        CNT:Data count of touch configuration.
Output Parameters:      DATA_BUFFER:Upload data content of touch configuration.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_TOUCH_LOAD(UINT8 CS, UINT16 CNT, PUINT8 DATA_BUFFER);
/*
Function Name:          N_REC_INDEX_START
Function Prototype:     UINT8 N_REC_INDEX_START(UINT16 RecordIndex)
Functional Description: Record voice and write encoded data into resource list related information by the Index_High and Index_Low byte which follows the command. 
                        Index_High and Index_Low bytes indicate the actual resource list number.
Input Parameters:       RecordIndex:Record index.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_REC_INDEX_START(UINT16 RecordIndex);
/*
Function Name:          N_REC_STOP
Function Prototype:     UINT8 N_REC_STOP(void)
Functional Description: Stop to record voice.
Input Parameters:       None.
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_REC_STOP(void);
/*
Function Name:          N_SET_CHANNEL_VOL
Function Prototype:     UINT8 N_SET_CHANNEL_VOL(UINT8 ChannelMsk, PUINT8 ValueArry)
Functional Description: Set volume of channel.
Input Parameters:       ChannelMsk:Channel mask of setting volume.
                        ValueArry:Value arrary of channel's volume. (Volume range:0~128)
Output Parameters:      None.
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_SET_CHANNEL_VOL(UINT8 ChannelMsk, PUINT8 ValueArry);
/*
Function Name:          N_GET_CHANNEL_VOL
Function Prototype:     UINT8 N_GET_CHANNEL_VOL(UINT8 ChannelMsk, PUINT8 ValueArry)
Functional Description: Get volume of channel.
Input Parameters:       ChannelMsk:Channel mask of getting volume.
Output Parameters:      ValueArry:Value arrary of channel's volume. (Volume range:0~128)
Return Value:           1:Command received correctly. 0:Command received error.
*/
UINT8 N_GET_CHANNEL_VOL(UINT8 ChannelMsk, PUINT8 ValueArry);
