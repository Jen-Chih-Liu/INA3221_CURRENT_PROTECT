#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "../config.h"
/* Format Error Code	*/
#define HEADER_NO_ERR           0x0000
#define HEADER_ERR_FILE         0x0001
#define HEADER_ERR_OPENFILE     0x0002
#define HEADER_ERR_READFILE     0x0003
#define HEADER_ERR_WRITEFILE    0x0004
#define HEADER_NOT_NF           0x0005


#define  ERROR_EXIST    10
#define  ERROR_ID       11
#define  ERROR_PASSWORD 12
#define  ERROR_FORMAT   13
#define  ALL_PASS 	14
#define  FINISHED	15
#define  FAILURE        16
#define  ERROR_CHECKSUM 17
#define  ERROR_BODY     18
#define  ERROR_IC        19
#define  ERROR_IC_LENGTH   20
#define  ERROR_PRECODE	21
#define  ERROR_WRITING	22



typedef  unsigned long	 DWORD;
typedef  unsigned int    WORD;
typedef  unsigned int    BOOL;
typedef  unsigned char   BYTE;


unsigned char APROM[APROM_SIZE];
int main(int argc, char* argv[])
{
	char  filename[180], sumname[180];
	long  FileAddress;
	FILE  *obj_fp, *sum_fp;
	DWORD  SUM = 0;
	BYTE  byte1, i, j;
	DWORD length;
	unsigned int file_size_temp, file_checksum, file_totallen;
	printf("\n");
	printf("***********************************************\n");
	printf("*           File CheckSum                     *\n");
	printf("***********************************************\n");

	if (argc != 3)
	{
		return 0;
	}
	if (!(obj_fp = fopen(argv[1], "rb")))
	{
		printf("%s FileOpen Error!\n", argv[1]);
		return 0;
	}
	fseek(obj_fp, 0, SEEK_END);
	file_totallen = ftell(obj_fp);
	fseek(obj_fp, 0, SEEK_SET);
	printf("fize size:%d\n\r", file_totallen);


	for (int i = 0; i < APROM_SIZE_msi_vga; i++)
	{
		APROM[i] = 0xff;
	}

	file_size_temp = 0;
	while (!feof(obj_fp)) {
		fread(&APROM[file_size_temp], sizeof(char), 1, obj_fp);
		file_size_temp++;
	}
	fclose(obj_fp);
	file_checksum = 0;
	//for (int i = 0; i < file_totallen; i++)
	for (unsigned int i = 0; i < (APROM_SIZE_msi_vga-4); i++)
	{
		file_checksum += APROM[i];
	}
	file_checksum = file_checksum & 0xffff;
	printf("file_checksum=0x%x\n\r", file_checksum);
	//write checksum in aprom buffer
	//APROM[(APROM_SIZE)-8] = (file_totallen >> 0) & 0xff;
	//APROM[(APROM_SIZE)-7] = (file_totallen >> 8) & 0xff;
	//APROM[(APROM_SIZE)-6] = (file_totallen >> 16) & 0xff;
	//APROM[(APROM_SIZE)-5] = (file_totallen >> 24) & 0xff;
	APROM[(APROM_SIZE_msi_vga)-4] = (file_checksum >> 0) & 0xff;
	APROM[(APROM_SIZE_msi_vga)-3] = (file_checksum >> 8) & 0xff;
	APROM[(APROM_SIZE_msi_vga)-2] = (file_checksum >> 16) & 0xff;
	APROM[(APROM_SIZE_msi_vga)-1] = (file_checksum >> 24) & 0xff;

		if (!(sum_fp = fopen(argv[2], "wb")))
		{
			printf("%s FileOpen Errot!\n", argv[2]);
			return 0;
		}
		fwrite(&APROM, APROM_SIZE_msi_vga, 1, sum_fp);
		fclose(sum_fp);

	return 0;
}
