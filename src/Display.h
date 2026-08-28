#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

// Pin I2C Default ESP32-S3
#define DEFAULT_SDA_PIN 41
#define DEFAULT_SCL_PIN 42

class Display {
private:
    static Adafruit_SSD1306 display;
    static String currentStatus;
    static String chatMessage;
    static String currentTime;

public:
    static void begin(int sdaPin = DEFAULT_SDA_PIN, int sclPin = DEFAULT_SCL_PIN);
    static void updateState(const char* stateText);
    static void updateState(const String& stateText);
    static void showChatMessage(const String& role, const String& text);
    static void updateTime(const char* timeStr);
    static void setBrightness(int percentage);
    static void render();
    static void clear();
};

#endif // DISPLAY_H
