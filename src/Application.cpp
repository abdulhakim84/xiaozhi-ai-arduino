#include "Application.h"
#include <asr/DoubaoASR.h>
#include "IOT.h"
#include "Display.h" // Integrasi kelas Display pengganti LVGL

static auto TAG = "Application";

Application::Application() {
    IOT::begin();
    IOT::turnOffRgb();
    _ttsClient = new DoubaoTTS();
    _llmAgent = new CozeLLMAgent();
    _asrClient = new DoubaoASR();
    _audioPlayer = new AudioPlayer();
    _recordingManager = new RecordingManager();
}

void Application::begin() const {
    // Inisialisasi Display SSD1306 (SDA = 41, SCL = 42)
    Display::begin(41, 42);

    _audioPlayer->begin();
    _recordingManager->begin();
}

void Application::showMemoryInfo() {
    xTaskCreate([](void *ptr) {
        while (true) {
            ESP_LOGD(TAG, "Free ram: %d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
            ESP_LOGD(TAG, "Free psram: %d", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }, "showMemoryInfo", 2048, nullptr, 1, nullptr);
}
