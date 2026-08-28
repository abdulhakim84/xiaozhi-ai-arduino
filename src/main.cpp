#include "Application.h"
#include "Arduino.h"
#include "GlobalState.h"
#include "Display.h"
#include "Settings.h"
#include "WiFiManager.h"

static auto TAG = "Main";

void setup() {
    // 1. Memuat konfigurasi lokal dari SPIFFS & NVS Flash
    Settings::begin();

    // 2. Inisialisasi Layar OLED SSD1306 (SDA = 41, SCL = 42)
    Display::begin(41, 42);

    // 3. Memulai proses koneksi jaringan Wi-Fi
    WiFiManager::setupWiFi();

    ESP_LOGD(TAG, "Mengecek status koneksi jaringan...");
    
    // 4. Menunggu sampai Wi-Fi berhasil terhubung
    xEventGroupWaitBits(GlobalState::getEventGroup(), 
                        GlobalState::getEventBits({NetworkConnected}),
                        false, true, portMAX_DELAY);

    ESP_LOGD(TAG, "Jaringan terhubung!");
    GlobalState::setState(Sleep);

    // 5. Jalankan layanan utama AI (Audio Player, Recording, TTS/ASR)
    Application::getInstance().begin();
}

void loop() {
    // FreeRTOS menangani seluruh task di background
    vTaskDelay(pdMS_TO_TICKS(1000));
}
