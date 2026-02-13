#include <stdio.h>
/* 移除 stdlib.h，因為不再使用 malloc/free */
#include "eeprom_sim.h"

/* M251 硬體與演算法常數定義 
   注意：Max_Amount_of_Data 受限於程式邏輯(8-bit index)只能是 255
*/
#define Max_Amount_of_Data       255UL 
#define Max_Amount_of_flash_page 20    // 最大頁數限制
#define Min_Amount_of_flash_page  2
#define Status_Unwritten        0xFFFF

/* 靜態配置設定 */
#define MAX_EEPROM_INSTANCES     2     // 支援的最大 EEPROM 實例數量 (系統區 + 使用者區)

/* 靜態緩衝池 (Static Buffer Pool)
   佔用 SRAM 大小 = 2 * 255 = 510 Bytes
   這比 Heap 分配更安全且可預測
*/
static uint8_t gs_BufferPool[MAX_EEPROM_INSTANCES][Max_Amount_of_Data];
static uint8_t gs_u8AllocatedCount = 0; // 目前已分配的實例計數

#define Even_Addr_Pos           16UL
#define Even_Addr_Mask          0xFF0000
#define Even_Data_Pos           24UL
#define Even_Data_Mask          0xFF000000
#define Odd_Addr_Pos            0UL
#define Odd_Addr_Mask           0xFF
#define Odd_Data_Pos            8UL
#define Odd_Data_Mask           0xFF00

#define Flash_Write FMC_Write
#define Flash_Read  FMC_Read
#define Flash_Erase FMC_Erase
#define Flash_Page_Size FMC_FLASH_PAGE_SIZE

/* 內部函式宣告 */
static void Search_Valid_Page(EEPROM_Ctx_T *ctx);
static void Manage_Next_Page(EEPROM_Ctx_T *ctx);

/**
  * @brief  啟用 FMC ISP 功能
  */
void FMC_Enable(void)
{
    SYS_UnlockReg();
    FMC_Open();
    FMC_ENABLE_AP_UPDATE();
}

/**
  * @brief  初始化特定的 EEPROM 實例 (改為靜態分配)
  * @param  ctx: EEPROM 結構體指標
  * @param  base_addr: 該實例在 Flash 的實體起始位址
  * @param  data_amount: 資料變數數量 (Max 255)
  * @param  use_pages: 分配的 Flash 頁數
  */
uint32_t Init_EEPROM(EEPROM_Ctx_T *ctx, uint32_t base_addr, uint32_t data_amount, uint32_t use_pages)
{
    uint32_t i;

    /* 設定參數 */
    ctx->Flash_BaseAddr = base_addr;
    ctx->Amount_of_Data = data_amount;
    ctx->Amount_Pages = use_pages;

    /* 參數檢查 */
    if(ctx->Amount_of_Data > Max_Amount_of_Data) return Err_OverAmountData;
    if(ctx->Amount_Pages > Max_Amount_of_flash_page) return Err_OverPageAmount;
    if(ctx->Amount_Pages < Min_Amount_of_flash_page) return Err_OverPageAmount;
    
    /* 檢查 Flash 對齊 (M251 Page Size 512) */
    if(ctx->Flash_BaseAddr % Flash_Page_Size != 0) return Err_WriteBlockStatus;

    /* 靜態記憶體分配 (Static Allocation) */
    if (gs_u8AllocatedCount >= MAX_EEPROM_INSTANCES) {
        printf("EEPROM Init Error: Max Instances Reached!\n");
        return 0xFF; // 超出支援的實例數量
    }

    // 指向靜態池中的下一塊緩衝區
    ctx->Written_Data = gs_BufferPool[gs_u8AllocatedCount++];

    /* 初始化 SRAM 為 0xFF */
    for(i = 0; i < ctx->Amount_of_Data; i++)
    {
        ctx->Written_Data[i] = 0xFF;
    }
    
    /* 搜尋有效頁面與恢復數據 */
    Search_Valid_Page(ctx);
    
    return 0;
}

static void Search_Valid_Page(EEPROM_Ctx_T *ctx)
{
    uint32_t i, temp;
    uint8_t addr, data;
    
    /* 使用 Stack 上的區域陣列取代 malloc
       大小為 20 * 2 = 40 bytes，對 Stack 負擔極小
    */
    uint16_t Page_Status_Array[Max_Amount_of_flash_page]; 

    FMC_Enable();
    
    /* 讀取每個 Page 的狀態 */
    for(i = 0; i < ctx->Amount_Pages; i++)
    {   
        Page_Status_Array[i] = (uint16_t)Flash_Read(ctx->Flash_BaseAddr + (Flash_Page_Size * i));
    }

    /* 找出哪一頁有效 */
    ctx->Current_Valid_Page = 0;
    for(i = 0; i < ctx->Amount_Pages; i++)
    {
        if(Page_Status_Array[i] != Status_Unwritten)
            ctx->Current_Valid_Page = i;
    }

    /* 若為全新 Flash */
    if(Page_Status_Array[ctx->Current_Valid_Page] == Status_Unwritten)
    {
        Flash_Write(ctx->Flash_BaseAddr + (Flash_Page_Size * ctx->Current_Valid_Page), 0xFFFF0000);
        ctx->Current_Cursor = 2;
    }
    else
    {
        /* 恢復 SRAM 數據並尋找 Cursor */
        temp = Flash_Read(ctx->Flash_BaseAddr + (Flash_Page_Size * ctx->Current_Valid_Page));
        addr = (temp & Even_Addr_Mask) >> Even_Addr_Pos;
        data = (temp & Even_Data_Mask) >> Even_Data_Pos;

        if(addr == 0xFF)
        {
            ctx->Current_Cursor = 2;
        }
        else
        {
            if(addr < ctx->Amount_of_Data) ctx->Written_Data[addr] = data;

            for(i = 4; i < Flash_Page_Size; i += 4)
            {
                /* Check Odd */
                temp = Flash_Read(ctx->Flash_BaseAddr + (Flash_Page_Size * ctx->Current_Valid_Page) + i);
                addr = (temp & Odd_Addr_Mask) >> Odd_Addr_Pos;
                data = (temp & Odd_Data_Mask) >> Odd_Data_Pos;
                
                if(addr == 0xFF)
                {
                    ctx->Current_Cursor = i;
                    break;
                }
                else
                {
                    if(addr < ctx->Amount_of_Data) ctx->Written_Data[addr] = data;
                }
                
                /* Check Even */
                addr = (temp & Even_Addr_Mask) >> Even_Addr_Pos;
                data = (temp & Even_Data_Mask) >> Even_Data_Pos;
                
                if(addr == 0xFF)
                {
                    ctx->Current_Cursor = i + 2;
                    break;
                }
                else
                {
                    if(addr < ctx->Amount_of_Data) ctx->Written_Data[addr] = data;
                }
            }
        }
    }
    // 這裡不需要 free，因為 Page_Status_Array 是區域變數，函式結束自動釋放
}

uint32_t Read_Data(EEPROM_Ctx_T *ctx, uint8_t index, uint8_t *data)
{
    if(index >= ctx->Amount_of_Data) return Err_ErrorIndex;
    *data = ctx->Written_Data[index];
    return 0;
}

uint32_t Write_Data(EEPROM_Ctx_T *ctx, uint8_t index, uint8_t data)
{
    uint32_t temp = 0;
    uint32_t target_addr;

    if(index >= ctx->Amount_of_Data) return Err_ErrorIndex;
    if(ctx->Written_Data[index] == data) return 0; // 資料相同不寫入

    FMC_Enable();

    target_addr = ctx->Flash_BaseAddr + (Flash_Page_Size * ctx->Current_Valid_Page);

    if((ctx->Current_Cursor & 0x3) == 0) /* Odd position */
    {
        temp = 0xFFFF0000 | (index << Odd_Addr_Pos) | (data << Odd_Data_Pos);
        Flash_Write(target_addr + ctx->Current_Cursor, temp);
        ctx->Written_Data[index] = data;
    }
    else /* Even position */
    {
        temp = Flash_Read(target_addr + (ctx->Current_Cursor - 2));
        temp &= ~(Even_Addr_Mask | Even_Data_Mask);
        temp |= (index << Even_Addr_Pos) | (data << Even_Data_Pos);
        Flash_Write(target_addr + (ctx->Current_Cursor - 2), temp);
        ctx->Written_Data[index] = data;
    }
    
    if(ctx->Current_Cursor >= (Flash_Page_Size - 2))
    {
        Manage_Next_Page(ctx);
    }
    else
    {
        ctx->Current_Cursor += 2;
    }
    return 0;
}

static void Manage_Next_Page(EEPROM_Ctx_T *ctx)
{
    uint32_t i = 0, j, counter, temp = 0, data_flag = 0, new_page;
    uint32_t new_page_addr;

    FMC_Enable();
    
    counter = Flash_Read(ctx->Flash_BaseAddr + (Flash_Page_Size * ctx->Current_Valid_Page));

    if((ctx->Current_Valid_Page + 1) == ctx->Amount_Pages)
    {
        new_page = 0;
        counter++;
    }
    else
    {
        new_page = ctx->Current_Valid_Page + 1;
    }
    
    new_page_addr = ctx->Flash_BaseAddr + (Flash_Page_Size * new_page);

    /* --- [修正] 正確的搜尋第一筆有效資料邏輯 --- */
    /* 這裡必須使用 while(1) 配合 break，避免在找到時錯誤地提前增加索引 */
    while(1)
    {
        if (i >= ctx->Amount_of_Data) { i=0; break; } // Safety break
        
        if(ctx->Written_Data[i] == 0xFF)
        {
            i++;
        }
        else
        {
            // 找到第一筆有效資料，打包進 counter (Header)
            counter &= ~(Even_Addr_Mask | Even_Data_Mask);
            counter |= (i << Even_Addr_Pos) | (ctx->Written_Data[i] << Even_Data_Pos);
            Flash_Write(new_page_addr, counter);
            i++; // 處理完畢後才移動到下一筆
            break; 
        }
    }

    /* Copy the rest */
    for(j = 4; i < ctx->Amount_of_Data; i++)
    {
        if(ctx->Written_Data[i] == 0xFF) continue;

        if(data_flag == 0)
        {
            temp = 0xFFFF0000;
            temp |= (i << Odd_Addr_Pos) | (ctx->Written_Data[i] << Odd_Data_Pos);
            data_flag = 1;
        }
        else
        {
            temp &= ~(Even_Addr_Mask | Even_Data_Mask);
            temp |= (i << Even_Addr_Pos) | (ctx->Written_Data[i] << Even_Data_Pos);
            Flash_Write(new_page_addr + j, temp);
            temp = 0;
            data_flag = 0;
            j += 4;
        }
    }
    
    ctx->Current_Cursor = j;
    
    if(data_flag == 1)
    {
        Flash_Write(new_page_addr + j, temp);
        ctx->Current_Cursor += 2;
    }
    
    Flash_Erase(ctx->Flash_BaseAddr + (Flash_Page_Size * ctx->Current_Valid_Page));
    ctx->Current_Valid_Page = new_page;
}

uint16_t Get_Cycle_Counter(EEPROM_Ctx_T *ctx)
{
    return (uint16_t)Flash_Read(ctx->Flash_BaseAddr + (Flash_Page_Size * ctx->Current_Valid_Page));
}

/**
  * @brief  讀取多個 Bytes
  * @param  ctx: EEPROM Context
  * @param  offset: 起始位置 (0 ~ Amount_of_Data-1)
  * @param  data: 接收資料的指標
  * @param  len: 長度
  */
uint32_t Read_EEPROM(EEPROM_Ctx_T *ctx, uint32_t offset, uint8_t *data, uint32_t len)
{
    uint32_t i;

    /* 檢查範圍是否越界 */
    if ((offset + len) > ctx->Amount_of_Data)
    {
        return Err_ErrorIndex;
    }

    /* 直接從 SRAM 複製數據，速度最快 */
    for (i = 0; i < len; i++)
    {
        data[i] = ctx->Written_Data[offset + i];
    }

    return 0; // Success
}

/**
  * @brief  寫入多個 Bytes
  * @param  ctx: EEPROM Context
  * @param  offset: 起始位置 (0 ~ Amount_of_Data-1)
  * @param  data: 要寫入的資料指標
  * @param  len: 長度
  */
uint32_t Write_EEPROM(EEPROM_Ctx_T *ctx, uint32_t offset, uint8_t *data, uint32_t len)
{
    uint32_t i;
    uint32_t ret;

    /* 檢查範圍是否越界 */
    if ((offset + len) > ctx->Amount_of_Data)
    {
        return Err_ErrorIndex;
    }

    /* 逐字節寫入 */
    for (i = 0; i < len; i++)
    {
        /* Write_Data 內部會檢查資料是否改變，若相同則不寫 Flash，效率高 */
        ret = Write_Data(ctx, (uint8_t)(offset + i), data[i]);
        
        /* 如果寫入失敗 (例如 Flash 壞掉或狀態錯誤)，立即停止並回傳錯誤 */
        if (ret != 0)
        {
            return ret;
        }
    }

    return 0; // Success
}