MCU Firmware Specification (v1.6)
# M251 GPU Cable Current Protection Firmware

版本: v1.6
日期: 2026-04-01
修訂: v1.6 曾新增低電流（Undercurrent）保護功能；因應需求變更，目前已**停用** Undercurrent 保護邏輯（相關程式碼以 `#if 0` 包覆），保留介面定義供日後重啟使用。過電流閾值可透過 I2C 指令（`UPOC`）動態調整並持久化儲存至 EEPROM。另修正 EEPROM 實體區塊角色描述：**EEPROM 1（0xD800）= Event Log，EEPROM 2（0xEC00）= User Config**。

---

## 1. System Overview (系統概述)

### 1.1 核心元件

| 項目 | 規格 |
|------|------|
| MCU | Nuvoton M251ZD2AE |
| Core | ARM Cortex-M23 |
| Flash / SRAM | 64 KB / 12 KB |
| User APROM | 0x0000 – 0xCFFF |
| Power | 3.3 V |
| Current Sensor | Texas Instruments INA3221 × 2 |

| Function        | Pin      | Description                               |
|-----------------|----------|-------------------------------------------|
| `LED_ALARM`     | `PB.15`  | 告警指示 LED                              |
| `BUZZER`        | `PB.5`   | 蜂鳴器                                    |
| `INA_WARNING`   | `PF.2`   | 來自 INA3221 的硬體警告輸入               |
| `PS_PGOOD`      | `PF.3`   | 電源好信號輸出（Normal=low, Protection=high）|

- **INA_A (Ch 1–3)**: I2C1, Addr 0x40 (A0=GND)
- **INA_B (Ch 4–6)**: USCI-I2C1, Addr 0x40 (A0=GND)
- **功能**: 監控 6 路 12V 輸出的 Bus Voltage 與 Shunt Voltage
- **Communication**: I2C/SMBus Slave (I2C0)，Target Address: 0x4D (8-bit: 0x9A)，Speed: 100 kHz，PEC: Disabled

### 1.2 GPIO Pin Definition

| Function        | Direction  | Active Level | Description                                      |
|-----------------|------------|--------------|--------------------------------------------------|
| I2C_SCL         | Open-Drain | —            | SMBus Clock                                      |
| I2C_SDA         | Open-Drain | —            | SMBus Data                                       |
| INA_WARNING     | Input      | Low          | 兩顆 INA3221 的 Warning Pin（Wired-OR）          |
| LED_ALARM       | Output     | High         | 告警指示燈（亮起代表異常）                       |
| BUZZER          | Output     | High         | 蜂鳴器控制                                       |
| PS_PGOOD        | Output     | high         | 保護信號（Normal=low, Protection=high）          |

---

## 2. Firmware Logic (功能邏輯)

### A. Data Acquisition (數據擷取)

- MCU 每 **200 ms** 透過 I2C 讀取兩顆 INA3221 的 Shunt Voltage Register 與 Bus Voltage Register。
- **換算**:
  - Current (10mA 單位): Shunt_Voltage_UV / Shunt_Resistor_mOhm / 10
  - Voltage (10mV 單位): Bus_Voltage_Raw × 8 / 10
- 數值存入 SRAM Shadow Buffer（`eeprom_ram[0x30–0x47]`），供 SMBus 讀取。

---

### B. Protection 1: Current Imbalance — 電流不平衡保護

**觸發條件:**
- MCU 計算 6 路電流的 Max 與 Min。
- 若 `(Max - Min) > IMBALANCE_THRESHOLD / 10`（IMBALANCE_THRESHOLD 預設 **4000 mA**，換算後為 400 × 10mA 單位）。
- 需連續 **5 次**採樣皆超標（`IMBALANCE_DEBOUNCE_COUNT = 5`）才確認異常。
  - 每次採樣間隔由 `TIMER_MONITOR_UPDATE = 200 ms` 控制，實際防彈跳時間約 5 × 200 ms = **1 秒**。
  - `g_u8ProtectionChecked` 旗標確保每個採樣週期僅評估一次，避免 main loop 重複累加計數。
- Status Register **Bit 3** (`STATUS_BIT_IMBALANCE`) 置 1。

---

### C. Protection 2: Overcurrent — 過電流保護

**觸發條件:**
- 任意通道電流 > **OC 閾值**（預設 **9200 mA = 9.2 A**，可透過 `UPOC` 指令動態調整）。
- 需連續 **5 次**採樣皆超標（`OVERCURRENT_DEBOUNCE_COUNT = 5`）才確認異常。
  - 每次採樣間隔由 `TIMER_MONITOR_UPDATE = 200 ms` 控制，實際防彈跳時間約 5 × 200 ms = **1 s**。
  - `g_u8ProtectionChecked` 旗標確保每個採樣週期僅評估一次，避免 main loop 重複累加計數。
- Status Register **Bit 5** (`STATUS_BIT_OVERCURRENT`) 置 1。
- 閾值儲存於 EEPROM 2 Offset `0x28–0x2B`，斷電後保留設定值。

---

### D. Protection 3: Undercurrent — 低電流保護 ⚠️ 已停用

> **⚠️ 此保護功能目前已停用。** 相關程式碼（debounce 計數、status bit 設定、`UPUC` 指令處理、EEPROM 存取）均以 `#if 0` 包覆，不參與任何保護判斷。以下描述為原始設計規格，保留供日後重啟使用。

**觸發條件（已停用）:**
- 任意通道電流 < **UC 閾值**（預設 **500 mA**，可透過 `UPUC` 指令動態調整）。
- 需連續 **5 次**採樣皆低於閾值（`UNDERCURRENT_DEBOUNCE_COUNT = 5`）才確認異常。
  - 每次採樣間隔由 `TIMER_MONITOR_UPDATE = 200 ms` 控制，實際防彈跳時間約 5 × 200 ms = **1 s**。
  - `g_u8ProtectionChecked` 旗標確保每個採樣週期僅評估一次，避免 main loop 重複累加計數。
- Status Register **Bit 6** (`STATUS_BIT_UNDERCURRENT`) 置 1。（已停用，此位元不會被置 1）
- 閾值儲存於 EEPROM 2 Offset `0x2C–0x2F`，斷電後保留設定值。（已停用，目前不寫入）

> **比較邏輯說明**: 韌體讀取 `eeprom_ram` 的電流值單位為 **10mA**，閾值設定單位為 **mA**。比較時自動除以 10 進行單位換算（`min_current < g_AppConfig.u32UcThreshold / 10`）。

---

### E. Unified Warning Sequence — 統一三階段告警序列

**目前僅 Protection 1（電流不平衡）與 Protection 2（過電流）觸發此告警狀態機（Protection 3 Undercurrent 已停用）。任一異常確認後立即啟動 180 秒倒數。**

```
異常確認（debounce 通過）
        │
        ▼  STATE_WARNING_COUNTDOWN
  Phase 1: T +  0 s  →  T + 20 s
  ─────────────────────────────────
  LED: 1 Hz 閃爍（500ms ON / 500ms OFF）
  BUZZER: 靜音
        │
        ▼  T + 20 s  (BUZZER_DELAY_MS = 20 000 ms)
  Phase 2: T + 20 s  →  T + 180 s
  ─────────────────────────────────
  LED: 2 Hz 閃爍（250ms ON / 250ms OFF）
  BUZZER: 2 Hz 嗶嗶聲（同步）
        │
        ▼  T + 180 s  (LATCH_COUNTDOWN_MS = 180 000 ms)
  STATE_LATCHED（需斷電重置）
  ─────────────────────────────────
  PS_PGOOD: 拉low（保護動作）
  LED: 持續 2 Hz 閃爍（250ms ON / 250ms OFF）
  BUZZER: 持續 2 Hz 嗶嗶聲（與 LED 同步，不停止）
  Event Log: 儲存一次快照至 EEPROM 2

  若異常在 180 s 內消失：
  → 回到 STATE_NORMAL，LED/Buzzer 全關，PS_PGOOD 保持 Low
```

**關鍵時序常數（編譯期固定）:**

| 常數 | 值 | 說明 |
|------|----|------|
| `BUZZER_DELAY_MS` | 20 000 ms | LED-only 階段結束時間（20 秒） |
| `LATCH_COUNTDOWN_MS` | 180 000 ms | 總倒數時間（3 分鐘），到期後鎖定 |

> **注意**: 鎖定倒數時間由編譯期常數 `LATCH_COUNTDOWN_MS` 控制，不使用 I2C 暫存器 `0x08` (Countdown Duration)。

---

### F. Protection 4: HW Warning — INA3221 硬體中斷

**觸發條件:**
- 600W 限制，50A 平均分配，每通道警告上限設定為 8.33 A。
- 任一 INA3221 內部偵測超出 Limit，拉低 `INA_WARNING` Pin（PF.2，Falling Edge 中斷）。

**動作:**
- MCU 收到中斷，立即讀取最新感測數據快照。
- 執行 Event Log 快照（寫入 EEPROM 2）。
- Status Register **Bit 4** (`STATUS_BIT_HW_WARNING`) 置 1。
- PS_PGOOD 拉high（鎖定）。

---

## 3. Register Map (I2C 暫存器地圖)

裝置作為 I2C 從裝置，內部 `eeprom_ram[256]` 對應為 Register Map。

| 位址 (Offset) | 大小 | 欄位名稱 | 權限 | 說明 |
| :--- | :--- | :--- | :--- | :--- |
| **配置區** | | | | |
| `0x00–0x03` | 4 B | Power On Count | R | 系統開機次數 (LSB first) |
| `0x04–0x07` | 4 B | Imbalance Threshold | R/W | 電流不平衡閥值，單位：mA，預設 **4000** (LSB first) |
| `0x08–0x0B` | 4 B | Countdown Duration | R/W | 保留供主機讀寫，預設 60000 ms（*鎖定時間固定 3 min，此值目前不影響保護邏輯*） |
| `0x0C` | 1 B | SW Debounce Count | R/W | 軟體去抖動計數，預設 2 |
| `0x0D` | 1 B | Log Head | R/W | 事件日誌環形緩衝區指標（0–10） |
| **狀態區** | | | | |
| `0x0E` | 1 B | Status Register | R | 系統狀態旗標（見 3.1） |
| `0x0F` | 1 B | Event Log Index | R/W | 主機寫入欲讀取的 Log 索引（0–10），MCU 回填至 `0x80` |
| **校準資料區** | | | | |
| `0x10–0x27` | 24 B | Calibration Data | R/W | 6 通道：每通道 Gain (2B LE) + Offset (2B LE) |
| `0x28–0x2B` | 4 B | OC Threshold | R/W | 過電流閾值（mA），預設 **9200**，指令 `UPOC` 更新 (LSB first) |
| `0x2C–0x2F` | 4 B | UC Threshold | — | **(已停用)** 低電流閾值保留欄位，指令 `UPUC` 已停用，此欄位不被 MCU 寫入 |
| **即時監測數據區** | | | | |
| `0x30–0x31` | 2 B | Ch1 Current | R | 單位: 10 mA (LSB first) |
| `0x32–0x33` | 2 B | Ch1 Voltage | R | 單位: 10 mV (LSB first) |
| `0x34–0x35` | 2 B | Ch2 Current | R | 單位: 10 mA (LSB first) |
| `0x36–0x37` | 2 B | Ch2 Voltage | R | 單位: 10 mV (LSB first) |
| `0x38–0x39` | 2 B | Ch3 Current | R | 單位: 10 mA (LSB first) |
| `0x3A–0x3B` | 2 B | Ch3 Voltage | R | 單位: 10 mV (LSB first) |
| `0x3C–0x3D` | 2 B | Ch4 Current | R | 單位: 10 mA (LSB first) |
| `0x3E–0x3F` | 2 B | Ch4 Voltage | R | 單位: 10 mV (LSB first) |
| `0x40–0x41` | 2 B | Ch5 Current | R | 單位: 10 mA (LSB first) |
| `0x42–0x43` | 2 B | Ch5 Voltage | R | 單位: 10 mV (LSB first) |
| `0x44–0x45` | 2 B | Ch6 Current | R | 單位: 10 mA (LSB first) |
| `0x46–0x47` | 2 B | Ch6 Voltage | R | 單位: 10 mV (LSB first) |
| **事件 Log 回讀區** | | | | |
| `0x80–0x93` | 20 B | Event Log Data | R | MCU 依 `0x0F` 索引回填的 Log 內容 |
| **運行時間區** | | | | |
| `0xB0–0xB3` | 4 B | MCU Run Time | R | MCU 執行時間（秒，LSB first） |
| **資產訊息區** | | | | |
| `0xC0–0xCF` | 16 B | Manufacturing Date | R/W | 製造日期 (ASCII) |
| `0xD0–0xDF` | 16 B | Lot ID | R/W | 批次號碼 (ASCII) |
| `0xE0–0xEF` | 16 B | Serial Number | R/W | 產品序號 (ASCII) |
| **裝置資訊區** | | | | |
| `0xF0` | 1 B | FW Version | R | 韌體版本（目前 0x01） |
| `0xF1–0xFB` | 11 B | Device Name | R | "M251_GPU_CP" (ASCII) |
| `0xFC–0xFF` | 4 B | Reserved | — | 保留 |

### 3.1 Status Register (0x0E) Bit Definition

| Bit | 名稱 | 說明 |
|-----|------|------|
| 7 | Reserved | 保留 |
| 6 | `STATUS_BIT_UNDERCURRENT` | **(已停用)** Undercurrent 保護已停用，此位元不會被置 1 |
| 5 | `STATUS_BIT_OVERCURRENT` | 任一通道 > OC 閾值（預設 9300 mA = 9.3 A），debounce 確認後置 1 |
| 4 | `STATUS_BIT_HW_WARNING` | INA3221 硬體警告中斷觸發 |
| 3 | `STATUS_BIT_IMBALANCE` | 電流不平衡（Max-Min > 閥值），debounce 確認後置 1 |
| 2–0 | Reserved | 保留 |

### 3.2 I2C 讀取範例

```
# 讀取韌體版本
Write 0x9A (addr W), 0xF0 (offset)
Read  0x9B (addr R) → 1 byte (version)

# 讀取 Ch1 電流（10mA 單位，LSB first）
Write 0x9A, 0x30
Read  0x9B → 2 bytes → value = byte[1]<<8 | byte[0]
```

---

## 3.3 Special Commands (特殊指令)

### Update Imbalance Threshold (更新不平衡閥值)

格式（8 bytes）: `['U', 'P', 'I', 'T', B0, B1, B2, B3]`

- Header: `0x55 0x50 0x49 0x54`
- B0–B3: 新閥值（mA），32-bit Little-Endian

範例（設定為 4000 mA = 0x00000FA0）:
`[0x55, 0x50, 0x49, 0x54, 0xA0, 0x0F, 0x00, 0x00]`

---

### Update Overcurrent Threshold (更新過電流閾值)

格式（8 bytes）: `['U', 'P', 'O', 'C', B0, B1, B2, B3]`

- Header: `0x55 0x50 0x4F 0x43`
- B0–B3: 新閾值（mA），32-bit Little-Endian
- 更新後立即生效，並持久化儲存至 EEPROM 2 Offset `0x28–0x2B`

範例（設定為 10000 mA = 10 A）:
`[0x55, 0x50, 0x4F, 0x43, 0x10, 0x27, 0x00, 0x00]`

---

### Update Undercurrent Threshold (更新低電流閾值) ⚠️ 已停用

> **⚠️ 此指令目前已停用。** MCU 端的 `u8UPUCFlag` 處理程式碼以 `#if 0` 包覆，傳送此指令將**不會有任何效果**。

格式（8 bytes）: `['U', 'P', 'U', 'C', B0, B1, B2, B3]`

- Header: `0x55 0x50 0x55 0x43`
- B0–B3: 新閾值（mA），32-bit Little-Endian
- 更新後立即生效，並持久化儲存至 EEPROM 2 Offset `0x2C–0x2F`（停用中）

範例（設定為 300 mA）:
`[0x55, 0x50, 0x55, 0x43, 0x2C, 0x01, 0x00, 0x00]`

---

### Update Countdown Duration (更新倒數時間暫存器)

格式（8 bytes）: `['U', 'P', 'C', 'D', B0, B1, B2, B3]`

- Header: `0x55 0x50 0x43 0x44`
- B0–B3: 時間（ms），32-bit Little-Endian

> ⚠️ 此值寫入 EEPROM (`0x08`) 但目前**不影響**保護鎖定時間，鎖定時間固定為 `LATCH_COUNTDOWN_MS = 180000 ms`。

---

### Update SW Debounce Count (更新軟體去抖動計數)

格式（5 bytes）: `['S', 'W', 'D', 'C', B0]`

- Header: `0x53 0x57 0x44 0x43`
- B0: 新的計數值（uint8）

範例（設定為 5）: `[0x53, 0x57, 0x44, 0x43, 0x05]`

---

### Update Calibration Data (更新校準資料)

格式（9 bytes）: `['U', 'P', 'C', 'A', Ch_Idx, G0, G1, O0, O1]`

- Header: `0x55 0x50 0x43 0x41`
- Ch_Idx: 通道索引 0–5
- G0,G1: Gain（16-bit LE）
- O0,O1: Offset（16-bit LE）

範例（Ch2，Gain=1000=0x03E8，Offset=-5=0xFFFB）:
`[0x55, 0x50, 0x43, 0x41, 0x01, 0xE8, 0x03, 0xFB, 0xFF]`

---

### Update Manufacturing Date (更新製造日期)

格式（20 bytes）: `['U', 'P', 'M', 'F', D0, ..., D15]`

- Header: `0x55 0x50 0x4D 0x46`
- D0–D15: 16 bytes ASCII（不足補 0x00）

---

### Update Lot ID (更新批次號碼)

格式（20 bytes）: `['U', 'P', 'L', 'T', D0, ..., D15]`

- Header: `0x55 0x50 0x4C 0x54`
- D0–D15: 16 bytes ASCII（不足補 0x00）

---

### Update Serial Number (更新產品序號)

格式（20 bytes）: `['U', 'P', 'S', 'N', D0, ..., D15]`

- Header: `0x55 0x50 0x53 0x4E`
- D0–D15: 16 bytes ASCII（不足補 0x00）

---

### Jump to LDROM (ISP Mode)

格式（7 bytes，固定）: `[0x4E, 0x05, 0x4A, 0x4D, 0x50, 0x4C, 0x44]`

接收後 MCU 將開機來源設定為 LDROM 並觸發軟體重置。

---

## 4. EEPROM Memory Layout (Flash Emulation)

MCU 以 Data Flash 模擬 EEPROM，共 **10 KB**（位於 Flash 末端），分為兩個 **5 KB** 區塊。

| 區塊 | Flash 位址 | 用途 |
|------|-----------|------|
| EEPROM 1 (Event Log)   | `0xD800–0xEBFF` | 事件異常快照（Ring Buffer） |
| EEPROM 2 (User Config) | `0xEC00–0xFFFF` | 系統設定、資產資訊、校準資料 |

### 4.1 EEPROM 2: Configuration & Asset（Flash 位址：0xEC00–0xFFFF）

容量 5 KB（有效使用 256 Bytes 邏輯空間）：

| Address Offset | Field Name           | Size   | Description |
|----------------|----------------------|--------|-------------|
| `0x00–0x03`    | PowerOnCount         | 4 B    | 開機次數（每次開機 +1） |
| `0x04–0x07`    | IMBALANCE_THRESHOLD  | 4 B    | 不平衡閥值（mA），預設 **4000** |
| `0x08–0x0B`    | Countdown Duration   | 4 B    | 保留（預設 60000 ms，目前不影響保護邏輯） |
| `0x0C`         | swdebounce           | 1 B    | 軟體去抖動計數，預設 2 |
| `0x0D`         | LogHead              | 1 B    | Ring Buffer 指標（0–10） |
| `0x0E–0x0F`    | Reserved             | 2 B    | — |
| `0x10–0x27`    | Calib Data           | 24 B   | 6 通道 × (Gain 2B + Offset 2B) |
| `0x28–0x2B`    | OC_THRESHOLD         | 4 B    | 過電流閾值（mA），預設 **9200**，指令 `UPOC` 更新 |
| `0x2C–0x2F`    | UC_THRESHOLD（停用）  | 4 B    | **(已停用)** 低電流閾值保留欄位，不被 MCU 讀寫 |
| `0x30–0xBF`    | Reserved             | 144 B  | — |
| `0xC0–0xCF`    | MFG Date             | 16 B   | 製造日期 (ASCII) |
| `0xD0–0xDF`    | Lot ID               | 16 B   | 批次號碼 (ASCII) |
| `0xE0–0xEF`    | Serial Number        | 16 B   | 產品序號 (ASCII) |

### 4.2 EEPROM 1: Event Logs（Flash 位址：0xD800–0xEBFF）

容量 5 KB（有效使用 242 Bytes）：**11 筆 × 22 Bytes = 242 Bytes**（Ring Buffer）。

| Address Offset  | Field        | Size | Description |
|-----------------|--------------|------|-------------|
| `0x000–0x015`   | Log Entry 0  | 22 B | 第 0 筆事故紀錄 |
| `0x016–0x02B`   | Log Entry 1  | 22 B | 第 1 筆事故紀錄 |
| ...             | ...          | ...  | ... |
| `0x0E4–0x0F1`   | Log Entry 10 | 22 B | 第 10 筆事故紀錄 |
| `0x0F2–...`     | Unused       | —    | — |

**Log Entry Data Structure（單筆 22 Bytes）:**

| Byte Offset | 欄位         | Size | Description |
|-------------|--------------|------|-------------|
| `0x00–0x03` | PowerOnCount | 4 B  | 發生當下的開機次數 |
| `0x04–0x07` | RunTime      | 4 B  | 發生當下的 MCU 運行時間（秒） |
| `0x08–0x09` | Current Ch1  | 2 B  | 電流快照（單位 10 mA） |
| `0x0A–0x0B` | Current Ch2  | 2 B  | 電流快照（單位 10 mA） |
| `0x0C–0x0D` | Current Ch3  | 2 B  | 電流快照（單位 10 mA） |
| `0x0E–0x0F` | Current Ch4  | 2 B  | 電流快照（單位 10 mA） |
| `0x10–0x11` | Current Ch5  | 2 B  | 電流快照（單位 10 mA） |
| `0x12–0x13` | Current Ch6  | 2 B  | 電流快照（單位 10 mA） |
| `0x14`      | Error Code   | 1 B  | 發生當下的 Status Register（Bit Map） |
| `0x15`      | Checksum     | 1 B  | Byte 0x00–0x14 的累加 Checksum |

---

## 5. Flash Memory Configuration & Boot Sequence

### 5.1 Flash Memory Map

總計 64 KB（`0x0000_0000 – 0x0000_FFFF`）：

| Memory Region       | Start Addr    | End Addr      | Size   | 用途 |
|---------------------|---------------|---------------|--------|------|
| APROM (Code)        | `0x0000_0000` | `0x0000_D7FB` | ~54 KB | 韌體程式碼 |
| APROM Checksum      | `0x0000_D7FC` | `0x0000_D7FF` | 4 B    | CRC32 完整性檢查碼 |
| EEPROM 1 (Log)      | `0x0000_D800` | `0x0000_EBFF` | 5 KB   | 事件 Log Ring Buffer |
| EEPROM 2 (User)     | `0x0000_EC00` | `0x0000_FFFF` | 5 KB   | 系統設定 / 資產資料 |

### 5.2 User Configuration (Config Words)

| Config Bit | 設定值 | 說明 |
|------------|--------|------|
| Boot Selection (CBS) | LDROM Boot | 上電後執行 LDROM |
| Brown-out Detector (BOD) | Enable | 掉電偵測重置 |

### 5.3 Boot Sequence (LDROM Logic)

1. **System Reset**: MCU 上電或重置
2. **Jump to LDROM**: 依 Config 設定跳至 LDROM
3. **Integrity Check**: 讀取 `0xD7FC` Checksum，比對 `0x0000–0xD7FB` 計算值
4. **Branching**:
   - **PASS**: 重映射 Vector Table → 跳轉至 APROM 執行主程式
   - **FAIL**: 留在 LDROM，進入 ISP 模式等待更新

---

## 6. Implementation Notes (開發備註)

- **I2C Timeout**: Flash 寫入（Page Erase）需數 ms，MCU 應利用 Clock Stretching 避免 Host Timeout。
- **Imbalance Debounce**: 連續 `IMBALANCE_DEBOUNCE_COUNT = 5` 次採樣超標才觸發，實際防彈跳時間 = 5 × 200 ms = **1 s**，避免負載動態變化誤報。
- **Overcurrent Debounce**: 連續 `OVERCURRENT_DEBOUNCE_COUNT = 5` 次採樣任意通道 > OC 閾值才觸發，實際防彈跳時間 = 5 × 200 ms = **1 s**。閾值可透過 `UPOC` 指令於執行期更新，預設 **9200 mA**。
- **Undercurrent Debounce** **(已停用)**: UC 保護邏輯目前以 `#if 0` 完全停用，不參與任何保護判斷。`UNDERCURRENT_DEBOUNCE_COUNT = 5`，UC 閾值預設 **500 mA**；介面定義保留，日後重啟時可透過 `UPUC` 指令動態更新。
- **可配置閾值持久化**: OC 閾值寫入 EEPROM 2（Offset `0x28`），重新上電後自動載入，不需重新燒錄韌體。UC 閾值欄位（Offset `0x2C`）已停用，目前不被 MCU 讀寫。
- **Debounce 計時機制（v1.4 修正）**: `Protection_Handler()` 透過 `g_u8ProtectionChecked` 旗標，確保防彈跳計數器每個 `TIMER_MONITOR_UPDATE`（200 ms）週期只遞增一次。`u8MonitorFlag` 觸發時重設旗標，I2C 傳輸完成後執行一次保護評估，後續 main loop 迴圈不再重複計數。
- **STATE_LATCHED**: 一旦鎖定需**斷電重置**才能恢復正常，MCU 不提供軟體解鎖指令。鎖定後 LED 與 Buzzer **持續以 2 Hz 同步閃爍**（250ms ON / 250ms OFF），確保操作人員在斷電前持續收到視覺與聽覺警示，不再維持 LED 常亮 / Buzzer 靜音。
- **Warning Pin Pull-up**: 確保 `INA_WARNING`（PA.3）有上拉電阻，GPIO 設定為 Falling Edge 中斷。
- **Flash Page Alignment**: Data EEPROM 從 `0xD800` 開始，M251 Page Size 為 512 B，確保 Page 邊界對齊。
- **Log Ring Buffer**: 最多儲存 **11 筆**紀錄（11 × 22 = 242 Bytes），滿後從索引 0 覆蓋（`LogHead` 循環遞增）。
- **PS_PGOOD 極性**: Normal 狀態輸出 **Low**，保護觸發後輸出 **High**（與部分舊規格書相反，以此文件為準）。