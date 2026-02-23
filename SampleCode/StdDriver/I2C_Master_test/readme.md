這是一份根據您提供的 MCU 韌體規格書（v1.2）所撰寫的 I2C Master 測試案例規格文件。這份文件以 I2C Master（主機端）的角度出發，驗證 MCU（從機端）的各項暫存器讀寫、特殊指令執行以及通訊穩定性。

I2C Master 測試案例規格書 (基於 MCU FW v1.2)
1. 測試環境與前置設定
Target Device (Slave): Nuvoton M251ZD2AE MCU

Slave Address: 0x4D (7-bit) / 寫入: 0x9A, 讀取: 0x9B (8-bit)

I2C Speed: 100kHz (Standard Mode)

Byte Order: Little-Endian (小端序)，最低有效位元組 (LSB) 在前。

Timeout 注意事項: 由於執行特殊指令（寫入 EEPROM/Flash）時可能觸發 MCU 進行 Clock Stretching 或需要數 ms 的處理時間，Master 端的 I2C Timeout 應適度放寬（建議 > 10ms）。

2. 測試案例列表
TC-01: 設備連線與韌體版本讀取 (連線測試)
測試目的: 驗證 I2C 總線連線正常，且能正確讀取單一位元組的唯讀暫存器。

測試步驟:

Master 發送 Write 請求：Slave Address 0x4D, Offset 0xF0 (FW Version)。

Master 發送 Read 請求：讀取 1 Byte。

預期結果:

I2C 應收到 ACK。

回傳值應為有效的韌體版本號（例如 0x01）。

TC-02: 即時監測數據讀取 (多位元組小端序測試)
測試目的: 驗證連續讀取多個位元組的功能，並確認小端序資料解析正確。

測試步驟:

Master 發送 Write 請求：Slave Address 0x4D, Offset 0x30 (Channel 1 Current)。

Master 發送 Read 請求：連續讀取 4 Bytes (包含 Ch1 電流與電壓)。

預期結果:

讀取到的 Byte 0 與 Byte 1 為 Ch1 電流（LSB first）。計算方式：(Byte[1] << 8 | Byte[0]) * 10mA。

讀取到的 Byte 2 與 Byte 3 為 Ch1 電壓（LSB first）。計算方式：(Byte[3] << 8 | Byte[2]) * 10mV。

數值應在合理範圍內（非 0xFFFF 或亂碼）。

TC-03: 特殊指令寫入 - 更新不平衡閥值 (UPIT)
測試目的: 驗證 Update Imbalance Threshold 指令是否能正確解析並寫入 EEPROM。

測試步驟:

Master 發送 Write 請求：發送 8 Bytes 資料序列 [0x55, 0x50, 0x49, 0x54, 0xDC, 0x05, 0x00, 0x00] (設定為 1500mA)。

延遲 10ms (等待 Flash 寫入完成)。

Master 發送 Write 請求：Offset 0x04。

Master 發送 Read 請求：連續讀取 4 Bytes。

預期結果:

讀取到的數值應為 [0xDC, 0x05, 0x00, 0x00]。

TC-04: 特殊指令寫入 - 更新倒數時間 (UPCD)
測試目的: 驗證 Update Countdown Duration 指令的執行。

測試步驟:

Master 發送 Write 請求：發送 8 Bytes 資料序列 [0x55, 0x50, 0x43, 0x44, 0x10, 0x27, 0x00, 0x00] (設定為 10000ms)。

延遲 10ms。

Master 發送 Write 請求：Offset 0x08。

Master 發送 Read 請求：連續讀取 4 Bytes。

預期結果:

讀取到的數值應為 [0x10, 0x27, 0x00, 0x00]。

TC-05: 特殊指令寫入 - 更新軟體去抖動計數 (SWDC)
測試目的: 驗證 Update SW Debounce Count 指令的執行。

測試步驟:

Master 發送 Write 請求：發送 5 Bytes 資料序列 [0x55, 0x50, 0x44, 0x43, 0x05] (設定為 5)。

延遲 10ms。

Master 發送 Write 請求：Offset 0x0C。

Master 發送 Read 請求：讀取 1 Byte。

預期結果:

讀取到的數值應為 0x05。

TC-06: 特殊指令寫入 - 更新校準資料 (UPCA)
測試目的: 驗證多參數複合指令 (Channel + Gain + Offset) 是否能正確對應目標記憶體。

測試步驟:

Master 發送 Write 請求：發送 9 Bytes 序列 [0x55, 0x50, 0x43, 0x41, 0x01, 0xE8, 0x03, 0xFB, 0xFF] (Ch 2, Gain=1000, Offset=-5)。

延遲 10ms。

Master 發送 Write 請求：Offset 0x14 (0x10 + 4 bytes * 1)。

Master 發送 Read 請求：連續讀取 4 Bytes。

預期結果:

讀取到的 Gain 數值應為 [0xE8, 0x03]。

讀取到的 Offset 數值應為 [0xFB, 0xFF]。

TC-07: 資產訊息寫入與讀取 (字串處理測試)
測試目的: 驗證 ASCII 字串的寫入，以及不足 16 Bytes 時補 0x00 (NULL) 的處理邏輯。

測試步驟:

Master 發送 Write 請求 (UPMF)：發送 20 Bytes [0x55, 0x50, 0x4D, 0x46, '2', '0', '2', '6', '0', '2', '1', '3', 0x00, 0x00, ..., 0x00]。

延遲 10ms。

Master 發送 Write 請求：Offset 0xC0。

Master 發送 Read 請求：連續讀取 16 Bytes。

預期結果:

讀取到的前 8 Bytes 解析為字串 "20260213"。

後 8 Bytes 皆為 0x00。

TC-08: 跳轉至 LDROM (ISP 模式) 指令測試
測試目的: 驗證 MCU 接收特定重啟指令後是否能正確重置並進入 Bootloader。

測試步驟:

Master 發送 Write 請求：發送 7 Bytes 序列 [0x4E, 0x05, 0x4A, 0x4D, 0x50, 0x4C, 0x44]。

Master 延遲 500ms (等待 MCU 重啟)。

Master 嘗試發送 Write 請求讀取韌體版本 (Offset 0xF0)。

預期結果:

步驟 1 應收到 ACK。

步驟 3 根據設計，進入 LDROM 後可能不再響應該 I2C Slave Address (或行為不同)，預期 I2C 讀取會發生 NACK 或 Timeout，以此證明系統已離開主程式。

TC-09: 事件日誌 (Event Log) 與狀態快照讀取
測試目的: 驗證透過 Ring Buffer 指標讀取 EEPROM 2 區域內的日誌快照結構是否正確。

前置條件: 觸發一次硬體或軟體保護 (例如讓輸入電壓超出範圍)，確保寫入至少一筆 Log。

測試步驟:

Master 發送 Write 請求：Offset 0x0E (Status Register)。

Master 發送 Read 請求：讀取 1 Byte (確認 Bit 3 或 Bit 4 被 Set)。

Master 發送 Write 請求：Offset 0x0D (Log Head)。

Master 發送 Read 請求：讀取 1 Byte (取得最新 Log Index，假設為 N)。

Master 透過自定義讀取事件日誌的方法 (規格書中提及 EEPROM 2，需確認是否透過 Offset 0x0F 或其他方式映射，假設由 I2C 直接讀取 0x0F 作為 event_index，再讀對應區塊)。

預期結果:

Status Register 應反映異常狀態。

讀取出的 20 Bytes Log 結構中，Checksum Byte (Byte 0x13) 的驗證應與前 19 Bytes 計算結果一致。