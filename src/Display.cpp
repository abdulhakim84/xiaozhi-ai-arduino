#include "Display.h"
#include <Wire.h>

// Definisi variabel statis
Adafruit_SSD1306 Display::display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
String Display::currentStatus = "Standby";
String Display::chatMessage = "";

void Display::begin(int sdaPin, int sclPin) {
    if (sdaPin != -1 && sclPin != -1) {
        Wire.begin(sdaPin, sclPin);
    } else {
        Wire.begin();
    }

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("[Display] Gagal mengalokasikan SSD1306!"));
        return;
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    
    updateState("Inisialisasi...");
}

void Display::updateState(const char* stateText) {
    updateState(String(stateText));
}

void Display::updateState(const String& stateText) {
    currentStatus = stateText;
    render();
}

void Display::showChatMessage(const String& role, const String& text) {
    chatMessage = role + ": " + text;
    render();
}

void Display::clear() {
    display.clearDisplay();
    display.display();
}

void Display::setBrightness(int percentage) {
    uint8_t contrast = map(constrain(percentage, 0, 100), 0, 100, 0, 255);
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(contrast);
}

void Display::render() {
    display.clearDisplay();

    // 1. Header Bar
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Xiaozhi AI");
    display.drawLine(0, 9, 128, 9, SSD1306_WHITE);

    // 2. Status utama
    display.setCursor(0, 15);
    display.print("Status: ");
    display.println(currentStatus);

    // 3. Sub-teks / Pesan Chat (jika ada)
    if (chatMessage.length() > 0) {
        display.setCursor(0, 35);
        // Potong string jika terlalu panjang agar tidak overflow layar
        String truncatedMsg = chatMessage.substring(0, 40); 
        display.println(truncatedMsg);
    }

    display.display();
}
