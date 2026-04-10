#ifndef __FIRMWARE_VERSION_H__
#define __FIRMWARE_VERSION_H__

#include <stdint.h>

typedef struct {
    uint32_t magic_word;        // 識別碼 (例如 0xABCD1234)    
    uint8_t  version_major;     // 主版本號
    uint8_t  version_minor;     // 次版本號
    uint8_t  version_patch;     // 修訂號
    uint8_t  reserved1;         // 對齊保留用
    char     build_date[12];    // 編譯日期字串
    char     build_time[8];     // 編譯時間字串
    uint8_t  board_model[12];   // 板子型號
} firmware_header_t;

extern const firmware_header_t g_firmware_header;

#endif /* __FIRMWARE_VERSION_H__ */
