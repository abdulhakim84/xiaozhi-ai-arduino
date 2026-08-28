#include "TimeUpdater.h"
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "Display.h"

unsigned long TimeUpdater::_lastMinute = -1;
WiFiUDP udp;
auto timeClient = NTPClient(udp, "ntp.aliyun.com");

void TimeUpdater::begin() {
    timeClient.begin();
    
    // Offset Waktu Indonesia:
    // WIB  (UTC+7) = 7 * 3600 = 25200
    // WITA (UTC+8) = 8 * 3600 = 28800
    // WIT  (UTC+9) = 9 * 3600 = 32400
    timeClient.setTimeOffset(25200); 

    xTaskCreate([](void *ptr) {
        while (true) {
            timeClient.update();
            const unsigned long localEpochTime = timeClient.getEpochTime();
            const unsigned long currentHour = (localEpochTime % 86400) / 3600;
            const unsigned long currentMinute = (localEpochTime % 3600) / 60;
            
            if (currentMinute != _lastMinute) {
                char timeStr[6];
                snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu", currentHour, currentMinute);
                
                // Pembaruan waktu ke OLED
                Display::updateTime(timeStr);
                
                _lastMinute = currentMinute;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }, "timeUpdater", 1024 * 4, nullptr, 1, nullptr);
}
