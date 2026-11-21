# 保留現有接線的測試版

1. 解壓到新資料夾，使用 VS Code 開啟包含 platformio.ini 的那一層。
2. BME280 保持 SDA→GP0、SCL→GP1；TFT、四組 LDR 都不移動。
3. Speaker 保持完全不接；本版預設關閉發聲，不代表完成 PWM 聲音 checkoff。
4. 燒錄仍用原本成功的 Picoprobe 接法。
5. 序列輸出已改成 Proton 自己的 USB：Proton 的 USB 口需接可傳資料的線到電腦。
   Monitor 要選 Proton 新出現的 USB COM 埠，不要直接假設原本 COM3 就是它。
6. PlatformIO → Project Tasks → ldr_test → Upload：先測四顆光敏電阻。
7. 再選 bme_test → Upload：BME_OK=1 表示成功讀到；T10=245 代表 24.5°C。
8. 再選 tft_test → Upload：每兩秒切換一般／高溫／高濕／暗的假資料畫面。
9. 三項各自通過後，選 proton → Upload，執行三者整合。

每次換測試都要 Upload 對應環境，光切換畫面不會改變板上的程式。
如果沒有 COM 埠，先確認資料線接在 Proton 本體，再檢查 Windows 裝置管理員。
詳細接線表與限制請看 README.md；檢查結果看 VALIDATION.md。
