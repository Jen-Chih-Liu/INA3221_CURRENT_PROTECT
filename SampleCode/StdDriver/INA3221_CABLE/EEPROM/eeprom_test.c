#include <stdio.h>
#include <string.h> // 用於 memcmp
#include <stdlib.h> // 用於 rand()

#include "eeprom_sim.h"
#include "eeprom_test.h"

/* --- 測試組態 --- */
// 在壓力測試中執行的總寫入次數
// M251 每頁 512 bytes。每次寫入佔 2 bytes，一頁大約可寫 (512-4)/2 = 254 次。
// 為了觸發 10 次以上的分頁滾動，我們設定一個較大的值。
#define STRESS_TEST_TOTAL_WRITES   5000

// 設定為 1 使用隨機位址和資料進行寫入，這對演算法是更嚴苛的考驗。
// 設定為 0 則使用循序位址和資料。
#define USE_RANDOM_ACCESS          1

/* --- 輔助函式 --- */
static void print_progress(int current, int total)
{
    // 大約印 20 次進度
    if ((current > 0) && (current % (total / 20)) == 0)
    {
        printf("... 進度: %d / %d (%d%%)\n", current, total, (current * 100) / total);
    }
}

/**
  * @brief  對 eeprom_sim 模組執行全面的壓力測試
  * @param  ctx: 指向已初始化的 EEPROM context
  * @return 0 代表成功, 其他值為錯誤碼
  */
int eeprom_stress_test(EEPROM_Ctx_T *ctx)
{
    // "黃金樣本"緩衝區，存放在 SRAM 中，用於比對。
    uint8_t golden_buffer[Max_Amount_of_Data];
    uint8_t read_buffer[Max_Amount_of_Data];
    uint32_t i;
    uint32_t ret;
    uint16_t start_cycle_count, end_cycle_count;

    printf("\n--- 開始 EEPROM 壓力測試 ---\n");
    printf("EEPROM 大小: %ld bytes\n", ctx->Amount_of_Data);
    printf("Flash 分頁數: %ld\n", ctx->Amount_Pages);
    printf("總寫入次數: %d\n", STRESS_TEST_TOTAL_WRITES);

    /* --- Phase 1: 初始資料完整性檢查 --- */
    printf("\n[Phase 1] 初始資料完整性檢查...\n");
    // 在黃金樣本中建立一組已知的初始狀態
    for (i = 0; i < ctx->Amount_of_Data; i++)
    {
        golden_buffer[i] = (uint8_t)i;
    }

    printf("寫入初始資料...\n");
    ret = Write_EEPROM(ctx, 0, golden_buffer, ctx->Amount_of_Data);
    if (ret != 0)
    {
        printf("錯誤: Write_EEPROM 在初始寫入時失敗! 錯誤碼: %ld\n", ret);
        return -1;
    }

    printf("讀回並驗證...\n");
    ret = Read_EEPROM(ctx, 0, read_buffer, ctx->Amount_of_Data);
    if (ret != 0)
    {
        printf("錯誤: Read_EEPROM 在初始讀取時失敗! 錯誤碼: %ld\n", ret);
        return -2;
    }

    if (memcmp(golden_buffer, read_buffer, ctx->Amount_of_Data) != 0)
    {
        printf("錯誤: 初始寫入後資料不匹配!\n");
        return -3;
    }
    printf("Phase 1 通過.\n");

    /* --- Phase 2: 分頁滾動與寫入壓力測試 --- */
    printf("\n[Phase 2] 開始進行包含分頁滾動的壓力測試...\n");
    start_cycle_count = Get_Cycle_Counter(ctx);
    printf("初始抹寫次數計數: %u\n", start_cycle_count);

    for (i = 0; i < STRESS_TEST_TOTAL_WRITES; i++)
    {
        uint8_t index;
        uint8_t data;

#if USE_RANDOM_ACCESS
        index = rand() % ctx->Amount_of_Data;
        data = rand() % 256;
#else
        index = i % ctx->Amount_of_Data;
        data = (i & 0xFF);
#endif

        // 寫入單一位元組，這是被測試的核心操作
        ret = Write_Data(ctx, index, data);
        if (ret != 0)
        {
            printf("錯誤: Write_Data 在第 %ld 次寫入時失敗! 錯誤碼: %ld\n", i, ret);
            return -4;
        }

        // 同步更新我們在 SRAM 中的黃金樣本
        golden_buffer[index] = data;

        print_progress(i, STRESS_TEST_TOTAL_WRITES);
    }
    printf("... 進度: %d / %d (100%%)\n", STRESS_TEST_TOTAL_WRITES, STRESS_TEST_TOTAL_WRITES);
    printf("壓力寫入完成.\n");

    end_cycle_count = Get_Cycle_Counter(ctx);
    printf("結束時抹寫次數計數: %u\n", end_cycle_count);
    printf("期間發生分頁滾動 (Page Rollovers): %u 次\n", end_cycle_count - start_cycle_count);

    /* --- Phase 3: 最終資料驗證 --- */
    printf("\n[Phase 3] 最終資料驗證...\n");
    ret = Read_EEPROM(ctx, 0, read_buffer, ctx->Amount_of_Data);
    if (ret != 0)
    {
        printf("錯誤: Read_EEPROM 在最終讀取時失敗! 錯誤碼: %ld\n", ret);
        return -5;
    }

    if (memcmp(golden_buffer, read_buffer, ctx->Amount_of_Data) != 0)
    {
        printf("錯誤: 壓力測試後資料不匹配!\n");
        int mismatches = 0;
        for (i = 0; i < ctx->Amount_of_Data; i++)
        {
            if (golden_buffer[i] != read_buffer[i])
            {
                if (mismatches < 20) // 只印出前 20 個錯誤
                {
                    printf("  不匹配位址 %ld: 應為 0x%02X, 讀到 0x%02X\n", i, golden_buffer[i], read_buffer[i]);
                }
                mismatches++;
            }
        }
        printf("總共發現 %d 個不匹配的資料\n", mismatches);
        return -6;
    }
    printf("Phase 3 通過.\n");

    printf("\n--- EEPROM 壓力測試成功結束 ---\n");
    printf("\n[Phase 4] 手動測試: 現在可以在程式執行期間隨時對 MCU 進行斷電再上電, \n");
    printf("然後重新執行測試。如果 Phase 1 仍然通過, 代表掉電復原功能正常。\n");

    return 0; // 成功
}
