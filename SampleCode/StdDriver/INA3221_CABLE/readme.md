MCU Firmware Specification (v1.2)

版本: v1.2
日期: 2026-02-09
修訂: 整合 INA3221 監控、電流不平衡判斷、5分鐘延時保護邏輯，並完整定義 EEPROM 配置。新增 Flash Memory Map 與 Boot Loader 規格。修正 SMBus 速度。

1. System Overview (系統概述)

1.1 核心元件

MCU: Nuvoton M251ZD2AE

Core: ARM Cortex-M23

Memory: 64 KB Flash / 12 KB SRAM
User arpom area: 0x0-0xcfff
Power: 3.3V

Sensor: Texas Instruments INA3221 (x2)

INA_A (Ch 1-3): I2C0 Addr 0x40 (A0=GND)

INA_B (Ch 4-6): USCI-I2C1 Addr 0x40 (A0=GND)

Function: 監控 6 路 12V 輸出的 Bus Voltage 與 Shunt Voltage。

Communication: I2C/SMBus Slave Interface I2C1

Target Address: 0x4d (8-bit: 0x9A)

Speed: 100kHz (Standard Mode)

PEC: Disabled

1.2 GPIO Pin Definition

|

| Function | Direction | Active Level | Description |
| I2C_SCL | Open-Drain | - | SMBus Clock |
| I2C_SDA | Open-Drain | - | SMBus Data |
| INA_WARNING | Input | Low | 來自兩顆 INA3221 的 Warning Pin (Wired-OR) |
| LED_ALARM | Output | High | 告警指示燈 (亮起代表異常) |
| BUZZER | Output | High/PWM | 蜂鳴器控制 |
| PS_VMON_PGOOD | Output | High | 電源好信號 (Normal=High, Protection=Low) |

2. Firmware Logic (功能邏輯)

A. Data Acquisition (數據擷取)

Polling Loop: MCU 每 10ms (TBD) 透過 I2C 讀取兩顆 INA3221 的 Shunt Voltage Register 與 Bus Voltage Register。

Conversion:

Current (mA): Shunt_Voltage_UV / Shunt_Resistor_mOhm.

Voltage (mV): Bus_Voltage_Raw * 8.

數值存入 SRAM 中的 Shadow Buffer，供 SMBus 讀取 (0x30~0xB)。

B. Protection 1: Current Imbalance (MCU 演算法)

觸發條件:

MCU 計算 6 路電流的 Max_Val 與 Min_Val。

若 (Max_Val - Min_Val) > IMBALANCE_THRESHOLD (預設 1A，可由暫存器 0x08 設定)。

需經過 Debounce (例如連續 3 次採樣皆異常) 以避免雜訊誤觸。

動作 (Warning Action):

LED_ALARM: ON.

BUZZER: ON (Pattern: 1Hz Beep).

Status Register: Bit 3 (Imbalance) Set to 1.

進入倒數狀態 (State: WARNING_COUNTDOWN)。


C. Protection 2: HW Warning (INA3221 硬體中斷)

觸發條件:

任一 INA3221 偵測到數值超過其內部設定的 Limit (透過 MCU 初始化時設定)，拉低 INA_WARNING Pin。

動作 (Interrupt Action):

MCU 收到中斷，讀取 INA3221 的 Mask/Enable Register 確認觸發源。

LED_ALARM: ON.

BUZZER: ON (Pattern: 2Hz Beep).

Status Register: Bit 4 (HW Warning) Set to 1.

執行 Data Log Snapshot (寫入 EEPROM 2)。

系統鎖定 (Latch)。

3. Register Map (暫存器地圖)

此裝置作為 I2C 從裝置，其內部記憶體 `eeprom_ram[256]` 對應為 I2C 的 Register Map。主機可以透過標準的 I2C 讀寫操作來存取這些暫存器。

| 位址 (Offset) | 大小 (Bytes) | 欄位名稱 | 權限 | 說明 |
| :--- | :--- | :--- | :--- | :--- |
| **配置區** | | | | |
| `0x00 - 0x03` | 4 | Power On Count | R | 系統開機次數(lsb first) |
| `0x04 - 0x07` | 4 | Imbalance Threshold | R/W | 電流不平衡閥值，單位：mA (lsb first)|
| `0x08 - 0x0B` | 4 | Countdown Duration | R/W | 警告倒數計時時間，單位：ms (lsb first)|
| `0x0C` | 1 | SW Debounce Count | R/W | 不平衡軟體去抖動計數 |
| `0x0D` | 1 | Log Head | R/W | 事件日誌指標 |
| **狀態區** | | | | |
| `0x0E` | 1 | Status Register | R | 系統狀態旗標 (Bit 3: Imbalance, Bit 4: HW Warning) |
| `0x0F` | 1 | event_indext | R/W | read eeprom event index |
| **校準資料區** | | | | |
| `0x10 - 0x27` | 24 | Calibration Data | R/W | 6 通道校準資料 (每通道：Gain 2B + Offset 2B) (lsb first)|
| **即時監測數據區** | | | | |
| `0x30 - 0x31` | 2 | Channel 1 Current | R | 單位: 10mA(lsb first) |
| `0x32 - 0x33` | 2 | Channel 1 Voltage | R | 單位: 10mV(lsb first) |
| `0x34 - 0x35` | 2 | Channel 2 Current | R | 單位: 10mA(lsb first) |
| `0x36 - 0x37` | 2 | Channel 2 Voltage | R | 單位: 10mV(lsb first) |
| `0x38 - 0x39` | 2 | Channel 3 Current | R | 單位: 10mA(lsb first) |
| `0x3A - 0x3B` | 2 | Channel 3 Voltage | R | 單位: 10mV (lsb first)|
| `0x3C - 0x3D` | 2 | Channel 4 Current | R | 單位: 10mA (lsb first)|
| `0x3E - 0x3F` | 2 | Channel 4 Voltage | R | 單位: 10mV (lsb first)|
| `0x40 - 0x41` | 2 | Channel 5 Current | R | 單位: 10mA (lsb first)|
| `0x42 - 0x43` | 2 | Channel 5 Voltage | R | 單位: 10mV (lsb first)|
| `0x44 - 0x45` | 2 | Channel 6 Current | R | 單位: 10mA (lsb first)|
| `0x46 - 0x47` | 2 | Channel 6 Voltage | R | 單位: 10mV (lsb first)|
| **運行時間區** | | | | |
| `0xB0 - 0xB3` | 4 | MCU Run Time | R | MCU 執行時間 (秒) (lsb first)|
| **資產訊息區** | | | | |
| `0xC0 - 0xCF` | 16 | Manufacturing Date | R/W | 製造日期 (ASCII) |
| `0xD0 - 0xDF` | 16 | Lot ID | R/W | 批次號碼 (ASCII) |
| `0xE0 - 0xEF` | 16 | Serial Number | R/W | 產品序號 (ASCII) |
| **裝置資訊區** | | | | |
| `0xF0` | 1 | FW Version | R | 韌體版本 (e.g., 0x01) |
| `0xF1` - `0xFB` | 11 | Device Name | R | 裝置名稱字串 "M251_GPU_CP" (ASCII) |
| `0xFC` - `0xFF` | 4 | Reserved | R | 保留 |

讀取以上的資訊
example, read verion
w 0x4d (i2c address), 0xf0(offset) 
r 0x4d (i2c address), version(information)


3.1 Special Commands (特殊指令)

除了直接讀寫暫存器外，系統還支援特定的指令序列來執行特殊操作。

#### Update Imbalance Threshold (更新不平衡閥值)

此指令允許主機更新 `Imbalance Threshold` 的值，並將其永久寫入 EEPROM。

*   **指令格式**:
    主機需發送一個 8 位元組的序列。
    `['U', 'P', 'I', 'T', <B0>, <B1>, <B2>, <B3>]`

    *   `'U', 'P', 'I', 'T'`: 固定的 ASCII 指令標頭 (0x55, 0x50, 0x49, 0x54)。
    *   `<B0>` - `<B3>`: 新的閥值，單位為 mA，以 32 位元小端序 (Little-Endian) 格式表示。
        *   `<B0>`: LSB (最低有效位元組)
        *   `<B3>`: MSB (最高有效位元組)

*   **範例**:
    若要將閥值設定為 1500 mA (等於 0x000005DC)，主機需發送以下 8 個位元組：
    `[0x55, 0x50, 0x49, 0x54, 0xDC, 0x05, 0x00, 0x00]`

#### Update Countdown Duration (更新警告倒數時間)

此指令允許主機更新 `Countdown Duration` 的值，並將其永久寫入 EEPROM。

*   **指令格式**:
    主機需發送一個 7 位元組的序列。
    `['U', 'C', 'D', <B0>, <B1>, <B2>, <B3>]`

    *   `'U', 'P', 'C', 'D'`: 固定的 ASCII 指令標頭。
    *   `<B0>` - `<B3>`: 新的倒數時間，單位為 ms，以 32 位元小端序 (Little-Endian) 格式表示。
        *   `<B0>`: LSB (最低有效位元組)
        *   `<B3>`: MSB (最高有效位元組)

*   **範例**:
    若要將倒數時間設定為 10000 ms (等於 0x00002710)，主機需發送以下 8 個位元組：
    `[0x55, 0x50, 0x43, 0x44, 0x10, 0x27, 0x00, 0x00]`

#### Update SW Debounce Count (更新軟體去抖動計數)

此指令允許主機更新 `SW Debounce Count` 的值，並將其永久寫入 EEPROM。

*   **指令格式**:
    主機需發送一個 5 位元組的序列。
    `['S', 'W', 'D', 'C', <B0>]`

    *   `'S', 'W', 'D', 'C'`: 固定的 ASCII 指令標頭。
    *   `<B0>`: 新的去抖動計數值。

*   **範例**:
    若要將去抖動計數設定為 5，主機需發送以下 5 個位元組：
    `[0x55, 0x50, 0x44, 0x43, 0x05]`

#### Update Calibration Data (更新校準資料)

此指令允許主機更新單一通道的增益 (Gain) 與偏移 (Offset) 值，並將其永久寫入 EEPROM。

*   **指令格式**:
    主機需發送一個 9 位元組的序列。
    `['U', 'P', 'C', 'A', <Ch_Idx>, <G0>, <G1>, <O0>, <O1>]`

    *   `'U', 'P', 'C', 'A'`: 固定的 ASCII 指令標頭。
    *   `<Ch_Idx>`: 要更新的通道索引 (0-5)。
    *   `<G0>, <G1>`: 16 位元增益值 (Gain)，以小端序 (Little-Endian) 格式表示。
    *   `<O0>, <O1>`: 16 位元偏移值 (Offset)，以小端序 (Little-Endian) 格式表示。

*   **範例**:
    若要將通道 2 (索引為 `0x01`) 的增益設定為 1000 (0x03E8)，偏移設定為 -5 (0xFFFB)，主機需發送以下 9 個位元組：
    `[0x55, 0x50, 0x43, 0x41, 0x01, 0xE8, 0x03, 0xFB, 0xFF]`

#### Update Manufacturing Date (更新製造日期)

此指令允許主機更新 `Manufacturing Date` 的值，並將其永久寫入 EEPROM。

*   **指令格式**:
    主機需發送一個 20 位元組的序列。
    `['U', 'P', 'M', 'F', <D0>, ..., <D15>]`

    *   `'U', 'P', 'M', 'F'`: 固定的 ASCII 指令標頭。
    *   `<D0>` - `<D15>`: 16 個位元組的製造日期 (ASCII 字串)。如果字串長度小於 16，剩餘部分應以 `0x00` (NULL) 填充。

*   **範例**:
    若要將製造日期設定為 "20260213"，主機需發送 `[0x55, 0x50, 0x4D, 0x46, '2', '0', '2', '6', '0', '2', '1', '3', 0x00, ..., 0x00]`。

#### Update Lot ID (更新批次號碼)

此指令允許主機更新 `Lot ID` 的值，並將其永久寫入 EEPROM。

*   **指令格式**:
    主機需發送一個 20 位元組的序列。
    `['U', 'P', 'L', 'T', <D0>, ..., <D15>]`

    *   `'U', 'P', 'L', 'T'`: 固定的 ASCII 指令標頭。
    *   `<D0>` - `<D15>`: 16 個位元組的批次號碼 (ASCII 字串)。如果字串長度小於 16，剩餘部分應以 `0x00` (NULL) 填充。

*   **範例**:
    若要將批次號碼設定為 "A26B-11-XYZ"，主機需發送 `[0x55, 0x50, 0x4C, 0x54, 'A', '2', '6', 'B', '-', '1', '1', '-', 'X', 'Y', 'Z', 0x00, ...]`。

#### Update Serial Number (更新產品序號)

此指令允許主機更新 `Serial Number` 的值，並將其永久寫入 EEPROM。

*   **指令格式**:
    主機需發送一個 20 位元組的序列。
    `['U', 'P', 'S', 'N', <D0>, ..., <D15>]`

    *   `'U', 'P', 'S', 'N'`: 固定的 ASCII 指令標頭。
    *   `<D0>` - `<D15>`: 16 個位元組的產品序號 (ASCII 字串)。如果字串長度小於 16，剩餘部分應以 `0x00` (NULL) 填充。

*   **範例**:
    若要將產品序號設定為 "SN-12345"，主機需發送 `[0x55, 0x50, 0x53, 0x4E, 'S', 'N', '-', '1', '2', '3', '4', '5', 0x00, ...]`。

#### Jump to LDROM (ISP Mode)

此指令使 MCU 重新啟動並進入 LDROM 模式，以準備進行韌體更新 (In-System Programming)。

*   **指令格式**:
    主機需發送一個 7 位元組的序列。
    `[0x4E, 0x05, 0x4A, 0x4D, 0x50, 0x4C, 0x44]`

*   **說明**:
    這是一個固定的二進位指令。接收到此指令後，MCU 會將開機來源設定為 LDROM 並觸發軟體重置。


4. EEPROM Memory Layout (Flash Emulation)

MCU 使用 Data Flash 模擬 EEPROM。為確保資料安全性，建議配置於兩個獨立的 Flash Page。 (實際位址定義詳見 Section 5)

4.1 EEPROM 1: Configuration & Asset (Page N)

此區域存放系統參數與資產資料，容量定義為 256 Bytes。

| Address Offset | Field Name | Size (Bytes) | Description |
| 0x00 - 0x03 | PowerOnCount | 4 | 開機次數 (每次開機 +1) |
| 0x04 - 0x07 | IMBALANCE_THRESHOLD | 4 | 不平衡判斷門檻值 (mA) |
| 0x08 - 0x0B | Countdown | 4 | 關機延遲時間 (ms) |
| 0x0C | swdebounce | 1 | 開關去抖動計數 |
| 0x0D | LogHead | 1 | Ring Buffer 最新指標 (0~11) |
| 0x0E - 0x0F | Reserved | 2 | 保留區 |
| 0x10 - 0x27 | Calib Data | 24 | 6 Channels x (Gain 2B + Offset 2B) |
| 0x28 - 0xBF | Reserved | 152 | 保留區 |
| 0xC0 - 0xCF | MFG Date | 16 | 生產日期 (YYYYMMDD) (ASCII) |
| 0xD0 - 0xDF | Lot ID | 16 | 生產批號 (ASCII) |
| 0xE0 - 0xEF | Serial Number | 16 | 產品序號 (ASCII) |


4.2 EEPROM 2: Event Logs (Page N+1)

此區域為環形緩衝區 (Ring Buffer)，存放最近 12 筆異常事件快照。 總容量 256 Bytes (12 筆 x 20 Bytes = 240 Bytes)。

| Address Offset | Field Name | Size | Description |
| 0x00 - 0x13 | Log Entry 0 | 20 | 第 0 筆事故紀錄 |
| 0x14 - 0x27 | Log Entry 1 | 20 | 第 1 筆事故紀錄 |
| 0x28 - 0x3B | Log Entry 2 | 20 | 第 2 筆事故紀錄 |
| ... | ... | ... | ... |
| 0xDC - 0xEF | Log Entry 11 | 20 | 第 11 筆事故紀錄 |
| 0xF0 - 0xFF | Reserved | 16 | 未使用 |

Log Entry Data Structure (Single Entry - 20 Bytes)

| Byte Offset | Parameter | Size | Description |
| 0x00 | p_on | 4 Bytes | 發生當下的開機次數 |
| 0x02 | RunTime | 4 Bytes | 發生當下的系統運行時間 (秒) |
| 0x06 | Current Ch1 | 2 Bytes | 電流快照 (mA) |
| 0x08 | Current Ch2 | 2 Bytes | 電流快照 (mA) |
| 0x0A | Current Ch3 | 2 Bytes | 電流快照 (mA) |
| 0x0C | Current Ch4 | 2 Bytes | 電流快照 (mA) |
| 0x0E | Current Ch5 | 2 Bytes | 電流快照 (mA) |
| 0x10 | Current Ch6 | 2 Bytes | 電流快照 (mA) |
| 0x12 | Error Code | 1 Byte | 記錄當下 Status (Bit map) |
| 0x13 | Checksum | 1 Byte | 本筆 Entry (0x00~0x12) 的 Checksum |

5. Flash Memory Configuration & Boot Sequence

5.1 Flash Memory Map

MCU 64KB Flash (0x0000_0000 ~ 0x0000_FFFF) 依據需求劃分為 APROM、Checksum 區與 Data EEPROM 區。

Total Size: 64 KB (0x10000 Bytes)

Data EEPROM Reserved: 10 KB (Located at end of Flash)

| Memory Region | Start Address | End Address | Size | Function |
| APROM (Code) | 0x0000_0000 | 0x0000_D7FB | 54 KB - 4B | 韌體程式碼存放區 |
| APROM Checksum | 0x0000_D7FC | 0x0000_D7FF | 4 Bytes | APROM 完整性檢查碼 (CRC32 or Sum) |
| Data EEPROM | 0x0000_D800 | 0x0000_FFFF | 10 KB | 模擬 EEPROM 資料區 (Section 4) |

計算公式：Data EEPROM Start = 64KB (0x10000) - 10KB (0x2800) = 0xD800。 計算公式：Checksum Address = Data EEPROM Start (0xD800) - 4 Bytes = 0xD7FC。

5.2 User Configuration (Config Words)

燒錄 MCU 時需設定以下 Config bits 以符合 Boot 需求。

Boot Selection (CBS): 設定為 LDROM Boot (系統上電後優先執行 LDROM 程式)。

Brown-out Detector (BOD): Enable (開啟掉電偵測復位，確保電壓不穩時系統安全重置)。

5.3 Boot Sequence (LDROM Logic)

LDROM 程式負責檢查 APROM 完整性與執行韌體更新。

System Reset: MCU 上電或重置。

Jump to LDROM: 依據 Config 設定，PC 指向 LDROM 起始位址。

Integrity Check:

讀取 0x0000_D7FC 的 Checksum 值。

計算 0x0000_0000 至 0x0000_D7FB 的實際 Checksum。

比較兩者是否一致。

Branching:

PASS (一致): 重映射向量表 (Vector Table Remap) 至 SRAM 或 0x0，跳轉至 APROM (0x0000_0000) 執行主程式。

FAIL (不一致): 停留在 LDROM，進入 ISP 模式等待 Host 下達更新指令。

6. Implementation Notes (開發備註)

I2C Timeout: 因 Flash 寫入時 (特別是 Page Erase) 會佔用數 ms，若此時 Host 發送指令，MCU 需利用 Clock Stretching 或在 I2C ISR 中優先處理，避免 Timeout。

Imbalance Debounce: 建議在 FW 實作一個滑動視窗 (Sliding Window) 或計數器，確認連續 500ms 內電流差異皆大於門檻值才觸發告警，避免負載動態變化時的誤報。

Warning Pin Pull-up: 確保電路板上 INA_WARNING Pin 有接上拉電阻，且 MCU GPIO 設定為 Edge Trigger (Falling Edge) 中斷。

Flash Page Alignment: 確保 Data EEPROM 的 Page N 與 Page N+1 落於 0xD800 之後的物理 Page 邊界上 (M251 Page Size 為 512B，0xD800 為 Page aligned)。