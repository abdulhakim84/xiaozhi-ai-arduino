#include "WiFiManager.h"
#include "WiFi.h"
#include "Arduino.h"
#include "GlobalState.h"
#include "Settings.h"
#include "TimeUpdater.h"
#include "Display.h"

static auto TAG = "WiFiManager";

bool WiFiManager::isConnectingWifi = false;
int WiFiManager::connectRetries = 0;

struct WiFiSetupParams {
    std::string ssid;
    std::string password;
    int maxRetries;
};

void WiFiManager::setupWiFi() {
    const auto wifiInfo = Settings::getWifiInfo();
    if (wifiInfo.first.empty() || wifiInfo.second.empty()) {
        ESP_LOGI(TAG, "未发现可用WiFi配置");
        GlobalState::setState(NetworkConfigurationNotFound);
    } else {
        ESP_LOGI(TAG, "发现本地WiFi配置：%s, %s", wifiInfo.first.c_str(), wifiInfo.second.c_str());
        setupWiFiWithAnim(wifiInfo.first, wifiInfo.second, 20);
    }
}

void WiFiManager::setupWiFiWithAnim(const std::string &ssid, const std::string &password, int maxRetries) {
    auto *params = new WiFiSetupParams{ssid, password, maxRetries};
    
    // Mengganti lv_timer_create dengan FreeRTOS Task
    xTaskCreate([](void *pvParameters) {
        auto *p = static_cast<WiFiSetupParams *>(pvParameters);
        while (true) {
            const bool completed = WiFiManager::setupWiFi(p->ssid.c_str(), p->password.c_str(), p->maxRetries);
            if (completed) {
                break; // Hentikan loop jika koneksi sukses atau gagal total
            }
            vTaskDelay(pdMS_TO_TICKS(500)); // Delay polling 500ms
        }
        delete p;
        vTaskDelete(NULL); // Hapus task setelah selesai
    }, "wifiConnectTask", 4096, params, 1, NULL);
}

bool WiFiManager::setupWiFi(const char *ssid, const char *password, int maxRetries) {
    if (!isConnectingWifi) {
        isConnectingWifi = true;
        ESP_LOGI(TAG, "使用WiFi信息连接WiFI: %s, %s", ssid, password);
        GlobalState::setState(NetworkConnecting);
        WiFiClass::mode(WIFI_MODE_STA);
        WiFiClass::useStaticBuffers(true);
        WiFi.begin(ssid, password);
        return false;
    }

    if (WiFiClass::status() == WL_CONNECTED) {
        ESP_LOGI(TAG, "WiFi连接成功: %s, %s", ssid, password);
        GlobalState::setState(NetworkConnected);
        Settings::setWifiInfo(ssid, password); // Simpan kredensial WiFi
        TimeUpdater::begin();                   // Jalankan sinkronisasi waktu NTP
        isConnectingWifi = false;
        connectRetries = 0;
        return true;
    }

    connectRetries++;
    if (WiFiClass::status() != WL_CONNECTED) {
        GlobalState::setState(NetworkConnecting);
        if (connectRetries >= maxRetries) {
            ESP_LOGW(TAG, "WiFi连接超时");
            WiFi.disconnect(false, true);
            GlobalState::setState(NetworkConnectFailed);
            isConnectingWifi = false;
            connectRetries = 0;
            return true;
        }
        return false;
    }
    return false;
}
