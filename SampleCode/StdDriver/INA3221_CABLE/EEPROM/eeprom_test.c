#include <stdio.h>
#include <string.h> // 用於 memcmp
#include <stdlib.h> // 用於 rand()

#include "eeprom_sim.h"
#include "eeprom_test.h"

/* --- 測試組態 --- */
#define STRESS_TEST_TOTAL_WRITES   5000
#define USE_RANDOM_ACCESS          1

/* --- 內部輔助函式宣告 --- */
static void print_progress(int current, int total);
extern uint32_t Init_EEPROM(EEPROM_Ctx_T *ctx, uint32_t base_addr, uint32_t data_amount, uint32_t use_pages);

static void print_progress(int current, int total)
{
    if ((current > 0) && (current % (total / 10)) == 0)
    {
        printf("... 進度: %d / %d (%d%%)\n", current, total, (current * 100) / total);
    }
}

/**
  * @brief  對 eeprom_sim 模組執行全面的壓力與邊界測試
  * @param  ctx: 指向已初始化的主 EEPROM context
  * @return 0 代表成功, 其他值為錯誤碼
  */
int eeprom_stress_test(EEPROM_Ctx_T *ctx)
{
    uint8_t golden_buffer[255];
    uint8_t read_buffer[255];
    uint32_t i, ret;
    uint16_t start_cycle_count, end_cycle_count;

    printf("\n============================================\n");
    printf("--- 開始 EEPROM 綜合性自動化測試 ---\n");
    printf("============================================\n");
    printf("EEPROM 大小: %ld bytes\n", ctx->Amount_of_Data);
    printf("Flash 分頁數: %ld\n", ctx->Amount_Pages);

    /* --- Phase 1: 初始資料完整性檢查 --- */
    printf("\n[Phase 1] 初始寫入與基礎讀取測試...\n");
    for (i = 0; i < ctx->Amount_of_Data; i++) {
        golden_buffer[i] = (uint8_t)(i ^ 0xAA); // 使用稍微複雜的 Pattern
    }

    ret = Write_EEPROM(ctx, 0, golden_buffer, ctx->Amount_of_Data);
    if (ret != 0) {
        printf("錯誤: Phase 1 初始寫入失敗! 錯誤碼: %ld\n", ret);
		while(1);
        return -1;
    }

    ret = Read_EEPROM(ctx, 0, read_buffer, ctx->Amount_of_Data);
    if (memcmp(golden_buffer, read_buffer, ctx->Amount_of_Data) != 0) {
        printf("錯誤: Phase 1 初始寫入後資料不匹配!\n");
        while(1);
		return -1;
    }
    printf("=> Phase 1 通過.\n");

    /* --- Phase 2: 分頁滾動與隨機寫入壓力測試 --- */
    printf("\n[Phase 2] 進行 %d 次隨機寫入與分頁滾動測試...\n", STRESS_TEST_TOTAL_WRITES);
    start_cycle_count = Get_Cycle_Counter(ctx);

    for (i = 0; i < STRESS_TEST_TOTAL_WRITES; i++)
    {
        uint8_t index = rand() % ctx->Amount_of_Data;
        uint8_t data = rand() % 256;

        ret = Write_Data(ctx, index, data);
        if (ret != 0) {
            printf("錯誤: Phase 2 第 %ld 次寫入失敗! 錯誤碼: %ld\n", i, ret);
			while(1);
            return -2;
        }
        golden_buffer[index] = data; // 同步黃金樣本
        print_progress(i, STRESS_TEST_TOTAL_WRITES);
    }
    
    end_cycle_count = Get_Cycle_Counter(ctx);
    printf("=> 發生分頁滾動 (Page Rollovers): %u 次\n", end_cycle_count - start_cycle_count);

    // Phase 2 最終驗證
    Read_EEPROM(ctx, 0, read_buffer, ctx->Amount_of_Data);
    if (memcmp(golden_buffer, read_buffer, ctx->Amount_of_Data) != 0) {
        printf("錯誤: Phase 2 壓力測試後資料不匹配!\n");
		while(1);
        return -2;
    }
    printf("=> Phase 2 通過.\n");


    /* --- Phase 3: 相同資料寫入優化測試 --- */
    printf("\n[Phase 3] 測試寫入相同資料時的優化機制...\n");
    uint32_t cursor_before = ctx->Current_Cursor;
    uint32_t page_before = ctx->Current_Valid_Page;
    
    // 寫入與原本完全相同的資料
    ret = Write_Data(ctx, 5, golden_buffer[5]); 
    if (ret != 0) return -3;

    if (ctx->Current_Cursor != cursor_before || ctx->Current_Valid_Page != page_before) {
        printf("錯誤: 寫入相同資料卻消耗了 Flash 空間！游標發生了移動。\n");
		while(1);
        return -3;
    }
    printf("=> Phase 3 通過 (游標未移動，優化生效).\n");


    /* --- Phase 4: 邊界條件與錯誤防護測試 (Error Injection) --- */
    printf("\n[Phase 4] 邊界條件與越界存取防護測試...\n");
    
    // 測試 1: 寫入越界 index
    ret = Write_Data(ctx, ctx->Amount_of_Data, 0xFF);
    if (ret == 0) {
        printf("錯誤: 越界寫入 Write_Data 沒有被攔截!\n");
		while(1);
        return -4;
    }

    // 測試 2: Read_EEPROM 長度越界
    ret = Read_EEPROM(ctx, ctx->Amount_of_Data - 1, read_buffer, 2);
    if (ret == 0) {
        printf("錯誤: 越界讀取 Read_EEPROM 沒有被攔截!\n");
		while(1);
        return -4;
    }
    printf("=> Phase 4 通過 (防護機制正常作動).\n");


    /* --- Phase 5: 軟體重啟 / 資料重建測試 (Re-initialization) --- */
    /* 注意：為了避免再次呼叫 Init_EEPROM 消耗掉 gs_u8AllocatedCount，
       建議你的 Init_EEPROM 加入判斷：
       if (ctx->Written_Data == NULL) { 申請記憶體... } 
       這裡我們模擬系統重啟，重新呼叫 Init_EEPROM 解析 Flash。
    */
    printf("\n[Phase 5] 模擬系統斷電重啟與資料重建測試...\n");
    
    // 故意將 SRAM 緩衝區清空為 0x00，模擬斷電遺失資料
    memset(ctx->Written_Data, 0x00, ctx->Amount_of_Data);
    
    // 重新初始化 (請確保你的 Init_EEPROM 支持同一個實例重複初始化不漏 Memory)
    printf("執行重新初始化 (從 Flash 重建 SRAM)...\n");
    Init_EEPROM(ctx, ctx->Flash_BaseAddr, ctx->Amount_of_Data, ctx->Amount_Pages);

    // 重建後驗證是否與黃金樣本一致
    ret = Read_EEPROM(ctx, 0, read_buffer, ctx->Amount_of_Data);
    if (memcmp(golden_buffer, read_buffer, ctx->Amount_of_Data) != 0) {
        printf("錯誤: Phase 5 模擬重啟後，解析 Flash 恢復的資料不正確!\n");
		while(1);
        return -5;
    }
    printf("=> Phase 5 通過 (資料從 Flash 成功無損恢復).\n");

    printf("\n============================================\n");
    printf("=> 所有 EEPROM 測試皆成功通過！\n");
    printf("============================================\n");
    
    return 0; // 成功
}
