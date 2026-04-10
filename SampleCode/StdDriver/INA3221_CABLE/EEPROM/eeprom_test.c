#include <stdio.h>
#include <string.h> // for memcmp
#include <stdlib.h> // for rand()

#include "eeprom_sim.h"
#include "eeprom_test.h"

/* --- Test Configuration --- */
#define STRESS_TEST_TOTAL_WRITES   5000
#define USE_RANDOM_ACCESS          1

/* --- Internal Helper Function Declarations --- */
static void print_progress(int current, int total);
extern uint32_t Init_EEPROM(EEPROM_Ctx_T *ctx, uint32_t base_addr, uint32_t data_amount, uint32_t use_pages);

static void print_progress(int current, int total)
{
    if ((current > 0) && (current % (total / 10)) == 0)
    {
        printf("... Progress: %d / %d (%d%%)\n", current, total, (current * 100) / total);
    }
}

/**
  * @brief  Perform comprehensive stress and boundary tests on the eeprom_sim module
  * @param  ctx: Pointer to an initialized main EEPROM context
  * @return 0 on success, non-zero error code on failure
  */
int eeprom_stress_test(EEPROM_Ctx_T *ctx)
{
    uint8_t golden_buffer[255];
    uint8_t read_buffer[255];
    uint32_t i, ret;
    uint16_t start_cycle_count, end_cycle_count;

    printf("\n============================================\n");
    printf("--- Starting EEPROM Comprehensive Automated Test ---\n");
    printf("============================================\n");
    printf("EEPROM Size: %ld bytes\n", ctx->Amount_of_Data);
    printf("Flash Pages: %ld\n", ctx->Amount_Pages);

    /* --- Phase 1: Initial Data Integrity Check --- */
    printf("\n[Phase 1] Initial write and basic read test...\n");
    for (i = 0; i < ctx->Amount_of_Data; i++) {
        golden_buffer[i] = (uint8_t)(i ^ 0xAA); // Use a slightly more complex pattern
    }

    ret = Write_EEPROM(ctx, 0, golden_buffer, ctx->Amount_of_Data);
    if (ret != 0) {
        printf("Error: Phase 1 initial write failed! Error code: %ld\n", ret);
		while(1);
        return -1;
    }

    ret = Read_EEPROM(ctx, 0, read_buffer, ctx->Amount_of_Data);
    if (memcmp(golden_buffer, read_buffer, ctx->Amount_of_Data) != 0) {
        printf("Error: Phase 1 data mismatch after initial write!\n");
        while(1);
		return -1;
    }
    printf("=> Phase 1 passed.\n");

    /* --- Phase 2: Page Rollover and Random Write Stress Test --- */
    printf("\n[Phase 2] Performing %d random writes and page rollover test...\n", STRESS_TEST_TOTAL_WRITES);
    start_cycle_count = Get_Cycle_Counter(ctx);

    for (i = 0; i < STRESS_TEST_TOTAL_WRITES; i++)
    {
        uint8_t index = rand() % ctx->Amount_of_Data;
        uint8_t data = rand() % 256;

        ret = Write_Data(ctx, index, data);
        if (ret != 0) {
            printf("Error: Phase 2 write #%ld failed! Error code: %ld\n", i, ret);
			while(1);
            return -2;
        }
        golden_buffer[index] = data; // Sync golden sample
        print_progress(i, STRESS_TEST_TOTAL_WRITES);
    }
    
    end_cycle_count = Get_Cycle_Counter(ctx);
    printf("=> Page Rollovers occurred: %u\n", end_cycle_count - start_cycle_count);

    // Phase 2 final verification
    Read_EEPROM(ctx, 0, read_buffer, ctx->Amount_of_Data);
    if (memcmp(golden_buffer, read_buffer, ctx->Amount_of_Data) != 0) {
        printf("Error: Phase 2 data mismatch after stress test!\n");
		while(1);
        return -2;
    }
    printf("=> Phase 2 passed.\n");


    /* --- Phase 3: Same-Data Write Optimization Test --- */
    printf("\n[Phase 3] Testing write optimization for identical data...\n");
    uint32_t cursor_before = ctx->Current_Cursor;
    uint32_t page_before = ctx->Current_Valid_Page;
    
    // Write exactly the same data as before
    ret = Write_Data(ctx, 5, golden_buffer[5]); 
    if (ret != 0) return -3;

    if (ctx->Current_Cursor != cursor_before || ctx->Current_Valid_Page != page_before) {
        printf("Error: Writing identical data consumed Flash space! Cursor moved.\n");
		while(1);
        return -3;
    }
    printf("=> Phase 3 passed (cursor unchanged, optimization effective).\n");


    /* --- Phase 4: Boundary Condition and Error Protection Test (Error Injection) --- */
    printf("\n[Phase 4] Boundary condition and out-of-bounds access protection test...\n");
    
    // Test 1: Write with out-of-bounds index
    ret = Write_Data(ctx, ctx->Amount_of_Data, 0xFF);
    if (ret == 0) {
        printf("Error: Out-of-bounds Write_Data was not intercepted!\n");
		while(1);
        return -4;
    }

    // Test 2: Read_EEPROM with out-of-bounds length
    ret = Read_EEPROM(ctx, ctx->Amount_of_Data - 1, read_buffer, 2);
    if (ret == 0) {
        printf("Error: Out-of-bounds Read_EEPROM was not intercepted!\n");
		while(1);
        return -4;
    }
    printf("=> Phase 4 passed (protection mechanism working correctly).\n");


    /* --- Phase 5: Software Restart / Data Reconstruction Test (Re-initialization) --- */
    /* Init_EEPROM 已支援重複呼叫：
       當 ctx->Written_Data != NULL 時，跳過靜態池分配、直接重用既有 buffer，
       不會消耗額外的 gs_u8AllocatedCount slot（無靜態池洩漏）。
       此處將 Written_Data 設為 NULL 以完整模擬首次上電狀態。
    */
    printf("\n[Phase 5] Simulating power-off restart and data reconstruction test...\n");
    
    // 模擬斷電：SRAM 內容消失，指標歸零（如同硬體重置後的初始狀態）
    ctx->Written_Data = NULL;
    
    // Re-initialize: Init_EEPROM 將重新分配 buffer 並從 Flash 重建 SRAM 數據
    printf("Performing re-initialization (rebuilding SRAM from Flash)...\n");
    Init_EEPROM(ctx, ctx->Flash_BaseAddr, ctx->Amount_of_Data, ctx->Amount_Pages);

    // Verify restored data matches the golden sample
    ret = Read_EEPROM(ctx, 0, read_buffer, ctx->Amount_of_Data);
    if (memcmp(golden_buffer, read_buffer, ctx->Amount_of_Data) != 0) {
        printf("Error: Phase 5 data recovered from Flash after simulated restart is incorrect!\n");
		while(1);
        return -5;
    }
    printf("=> Phase 5 passed (data successfully restored from Flash without loss).\n");

    printf("\n============================================\n");
    printf("=> All EEPROM tests passed successfully!\n");
    printf("============================================\n");
    
    return 0; // Success
}
