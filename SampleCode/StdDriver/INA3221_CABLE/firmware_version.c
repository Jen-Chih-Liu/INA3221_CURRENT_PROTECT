#include "firmware_version.h"

// 定義韌體資訊並給予初始設定值
// 利用編譯器的 __DATE__ 與 __TIME__ 巨集，在編譯時自動填入日期與時間
// 加入 __attribute__((used)) 告訴編譯器與連結器(Linker)保留這個變數，不要把它最佳化(拔除)
__attribute__((used, section(".ARM.__at_0xcfd0"))) const firmware_header_t g_firmware_header = {
    .magic_word    = 0xABCD1234,   // 識別碼
    .version_major = 1,            // 主版本號
    .version_minor = 0,            // 次版本號
    .version_patch = 0,            // 修訂號
    .reserved1     = 0,            // 對齊保留用
    .build_date    = __DATE__,     // 編譯日期字串 (格式："Mmm dd yyyy")
    .build_time    = __TIME__ ,     // 編譯時間字串 (格式："hh:mm:ss")
    .board_model   = "M251_GPU_CP",      // 板子型號
};
