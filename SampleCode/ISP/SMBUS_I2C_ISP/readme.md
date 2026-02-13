# M251 系列 SMBus/I2C 在線系統編程 (ISP) 範例程式

## 專案簡介

本專案是針對新唐 (Nuvoton) NuMicro® M251 系列微控制器的在線系統編程 (ISP) 範例程式碼。它示範了如何透過 SMBus/I2C 通訊協定，對 M251 目標晶片的快閃記憶體 (Flash) 進行讀取、寫入和擦除等操作。

此程式碼通常在一個「主控端」(Host) 上執行，例如個人電腦或其他微控制器，透過 I2C/SMBus 線路連接到 M251 目標板，以更新其應用程式碼 (APROM) 或資料 (Data Flash)。

M251 系列是一款基於 Arm® Cortex®-M23 核心的低功耗微控制器，支援寬電壓操作範圍，適用於物聯網等應用。 [2]


## 硬體需求

1.  **主控端 (Host):** 一台執行此 ISP 程式的設備。可能是：
    *   帶有 I2C/SMBus 介面的個人電腦 (可能需要 USB to I2C/SMBus 轉換器)。
    *   另一塊開發板 (如 Nu-Link)，作為 I2C/SMBus 主控。
2.  **目標板 (Target):** 一片搭載 Nuvoton M251 系列 MCU 的開發板或產品板。
3.  **連接線:** 用於連接主控端和目標板的 I2C/SMBus 訊號線 (SDA, SCL) 和接地線 (GND)。

## 軟體需求

*   **編譯環境:**
    *   Keil MDK
    *   IAR Embedded Workbench
    *   NuEclipse (Nuvoton 的 Eclipse IDE)
*   **驅動程式:** 如果使用 USB to I2C/SMBus 轉換器，可能需要安裝對應的驅動程式。
*   **NuMicro ISP Tool:** 新唐官方提供的圖形化 ISP 工具，可用於驗證或直接進行編程。

## 程式碼架構分析

一個典型的 ISP 專案通常包含以下幾個關鍵部分：

*   `main.c`:
    *   **功能:** 程式進入點與主迴圈。
    *   **職責:** 
        *   初始化系統時脈 (`SYS_Init`) 和 I2C 介面 (`I2C_Init`)。
        *   檢查 APROM 的校驗和 (Checksum) 來決定要停留在 LDROM 執行 ISP 流程，還是直接跳轉到 APROM 執行應用程式。
        *   在主迴圈中，根據 `i2c_transfer.c` 設定的旗標 (`u8eraseflashflag`, `u8JMPAPflag`, `u8PROGAPflag`) 來執行對應的 Flash 操作 (擦除、寫入、跳轉)。

*   `i2c_transfer.c`:
    *   **功能:** I2C 中斷服務函式 (ISR) 和通訊協定解析。
    *   **職責:** 
        *   `I2C1_IRQHandler()`: I2C 硬體中斷的進入點。
        *   `I2C_SlaveTRx()`: 核心狀態機，處理 I2C Slave 的各種狀態 (接收位址、接收資料、請求傳送資料等)。
        *   當收到 STOP 事件 (`u32Status == 0xA0`) 時，解析 `au8SlvRxData` 緩衝區中的命令，並設定對應的功能旗標給 `main.c` 使用。

*   `isp_user.c`:
    *   **功能:** 實現 ISP 核心指令 (此範例中部分功能被 `i2c_transfer.c` 的簡易協定取代)。
    *   **職責:** 包含更新 APROM、讀取 Flash、計算校驗和等高階 ISP 指令函式。在更複雜的 ISP 協定中，`ParseCmd` 函式會是主要的命令解析器。

*   `targetdev.h`:
    *   **功能:** 定義目標晶片相關的常數。
    *   **職責:** 包含 M251 系列的記憶體位址（如 APROM, LDROM, Config 的起始位址）以及 Flash 頁面大小等硬體相關設定。

## I2C/SMBus 通訊協定

本 ISP 範例程式透過 I2C/SMBus 介面接收主控端的指令。所有指令都是在 I2C 偵測到 STOP 訊號後進行解析。

以下是 `i2c_transfer.c` 中定義的主要指令：
i2c address 0x4d

### 1. 讀取 ISP 版本 (Read ISP Version)
*   **功能:** 讓主控端讀取目標板上 ISP 韌體的版本號。
*   **封包格式 (Master to Slave):**
    *   長度: 1 byte
    *   內容: `[0xA1]`
*   **回應 (Slave to Master):** 當主控端接著發起讀取操作時，從機會回傳 1 byte 的版本號 (`ldrom_fw_Verision`)。
Read ISP FW version(SMBUS Byte Read)
Write address 0x4D,  write offset 0xA1
Read address 0x4D, read data 0x03 (LDROM FW version)
c
### 2. 擦除應用程式區 (Erase APROM)
*   **功能:** 擦除 APROM (Application ROM) 區域，為寫入新韌體做準備。
*   **封包格式 (Master to Slave):**
    *   長度: 7 bytes
    *   內容: `[0xB0, 0x05, 'E', 'R', 'A', 'S', 'E']`
*   **回應:** 此指令會設定 `u8eraseflashflag` 旗標。`main.c` 中的主迴圈會根據此旗標執行擦除操作，並透過 `i2c_ack_data` 變數回報成功 (`0xAA`) 或失敗 (`0xFF`)。
ERASE APROM (SMBUS Block Write)
I2C Write address: 0x4D
Command Code: 0xB0
LENGTH: 0X05
RAW DATA: 0x45, 0x52, 0x41, 0x53, 0x45

MCU will Erase 0-0XEA00, Delay 4 sec
Check erase done (SMBUS Byte Read)
Write address 0x4D,  write offset 0xA2
Read address 0x4D, read data 0xAA (OK ) or 0xff (false)

### 3. 跳轉至應用程式區執行 (Jump to APROM)
*   **功能:** 指令 ISP 程式結束，並將程式控制權交給 APROM 中的主應用程式。
*   **封包格式 (Master to Slave):**
    *   長度: 7 bytes
    *   內容: `[0x4F, 0x05, 'J', 'M', 'P', 'A', 'P']`
*   **回應:** 此指令會設定 `u8JMPAPflag` 旗標，觸發系統重置並從 APROM 啟動。
LDROM JUMER TO APROM (SMBUS BLOCK WRITE)
I2C Write address: 0x4D
Command Code: 0x4F
LENGTH: 0X05
RAW DATA: 0x4A, 0x4D, 0x50, 0x41, 0x50
After, Delay 500ms for LDROM jumper APROM

### 4. 編程 APROM (Program APROM)
*   **功能:** 將從主控端收到的 32 bytes 資料寫入 APROM。
*   **封包格式 (Master to Slave):**
    *   長度: 34 bytes
    *   內容: `[0xB1, 0x20, <32 bytes of data>]`
*   **回應:** 此指令會將 32 bytes 的資料複製到 `Write_buff` 並設定 `u8PROGAPflag` 旗標。`main.c` 中的主迴圈會執行寫入與驗證，並透過 `i2c_ack_data` 回報結果 (`0xAA` 表示成功)。
WRITE APROM (SMBUS Block Write)
I2C Write address: 0x4D
Command Code: 0xB1
LENGTH:  0x20
RAW DATA: bin file raw data, byte0 - byte 31.
(note: software only program 0x0-0xcfff).
programming 32 byte need 5ms delay
Check write done (SMBUS Byte Read)
Write address 0x4D,  write offset 0xA2
Read address 0x4D, read data 0xAA (OK ) or 0xff (false)


## 使用說明

1.  **硬體連接:**
    *   將主控端的 I2C/SMBus 的 SDA、SCL、GND 分別連接到 M251 目標板對應的腳位。
    *   確保目標板已上電，並已進入 ISP 模式（通常是透過特定的 I/O 腳位狀態或開機設定來觸發）。

2.  **編譯專案:**
    *   使用 Keil、IAR 或 NuEclipse 開啟專案檔案。
    *   根據您的硬體配置（例如 I2C 腳位、通訊速度）修改 `main.c` 中的 `SYS_Init()` 和 `i2c_transfer.c` 中的 `I2C_Init()`。
    *   編譯專案以產生 `.hex` 或 `.bin` 檔案並燒錄至 M251 的 LDROM。

3.  **執行編程:**
    *   使用您的 I2C 主控端工具。
    *   **擦除:** 發送 "Erase APROM" 指令。
    *   **寫入:** 逐一發送 "Program APROM" 指令，每個封包包含 32 bytes 的韌體資料，直到所有資料都傳送完畢。
    *   **跳轉:** 發送 "Jump to APROM" 指令，讓目標板重置並執行新的應用程式。
    *   在每個步驟後，您可以發起 I2C 讀取操作來取得 `i2c_ack_data` 的狀態，確認操作是否成功。
