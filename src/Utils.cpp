#include "Utils.h"
#include <GlobalState.h>
#include <vector>

auto charset = "0123456789abcdef";

// Menghasilkan 32-karakter random String untuk ID Tugas/Sesi
std::string generateTaskId() {
    randomSeed(millis());
    char uuid[33];
    for (int i = 0; i < 32; i++) {
        uuid[i] = charset[random(16)];
    }
    uuid[32] = '\0';
    return {uuid};
}

// Membaca integer 32-bit dari byte array (Big-Endian)
int32_t readInt32(const uint8_t *bytes) {
    return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

// Mengubah byte array menjadi std::string
std::string readString(const uint8_t *bytes, const uint32_t length) {
    std::string result;
    for (int i = 0; i < length; i++) {
        result += static_cast<char>(bytes[i]);
    }
    return result;
}

// Mengambil MAC Address hardware ESP32 sebagai ID Unik Perangkat
std::string getChipId(const char *prefix) {
    std::string content{prefix ? prefix : ""};
    const std::size_t size = content.length();
    content.resize(size + 12); // mac address 6 * 2
    uint8_t buffer[6];
    esp_efuse_mac_get_default(buffer);
    sprintf(&content.at(size), "%02X%02X%02X%02X%02X%02X", buffer[0], buffer[1],
            buffer[2], buffer[3], buffer[4], buffer[5]);
    return content;
}

// Mengonversi uint32 ke array 4-byte (Big-Endian)
std::vector<uint8_t> uint32ToUint8Array(const uint32_t size) {
    std::vector<uint8_t> result(4);
    result[0] = (size >> 24) & 0xFF;
    result[1] = (size >> 16) & 0xFF;
    result[2] = (size >> 8) & 0xFF;
    result[3] = size & 0xFF;
    return result;
}

// Mencari tanda baca pertama untuk memotong teks balasan AI sebelum dikirim ke TTS
std::pair<int, size_t> findMinIndexOfDelimiter(const String &input) {
    std::vector<String> delimiters = {"，", "。", "！", "：", "；", "？"};

    int minIndex = -1;
    size_t minIndexDelimiterLength = 0;
    for (const String &delimiter: delimiters) {
        const int index = input.indexOf(delimiter);
        if (index > 0 && (index < minIndex || minIndex == -1)) {
            minIndex = index;
            minIndexDelimiterLength = delimiter.length();
        }
    }
    return std::make_pair(minIndex, minIndexDelimiterLength);
}

// Mengonversi string HEX Warna (#FFFFFF) ke nilai integer
uint32_t hexColorToUInt(String hex) {
    if (hex.length() != 7 || hex[0] != '#') {
        return 0xffffff;
    }
    hex.replace("#", "");
    return std::stoul(hex.c_str(), nullptr, 16);
}

// Helper konversi String ke Integer
int str2int(const String &input) {
    return std::stoi(input.c_str(), nullptr, 10);
}

// Helper konversi String ke Double
double str2double(const String &input) {
    return std::stod(input.c_str(), nullptr);
}
