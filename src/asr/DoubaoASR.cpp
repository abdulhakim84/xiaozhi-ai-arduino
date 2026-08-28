#include "DoubaoASR.h"
#include "Utils.h"
#include "Settings.h"
#include <Arduino.h>
#include <RecordingManager.h>
#include <vector>
#include "Display.h"
#include "Application.h"
#include "ArduinoJson.h"
#include "GlobalState.h"

static auto TAG = "ASR";

DoubaoASR::DoubaoASR() {
    _eventGroup = xEventGroupCreate();
    _requestBuilder = std::vector<uint8_t>();
    _firstPacket = true;
    setExtraHeaders(("Authorization: Bearer; " + Settings::getDoubaoAccessToken()).c_str());
    beginSSL("openspeech.bytedance.com", 443, "/api/v2/asr");
    onEvent([this](WStype_t type, uint8_t *payload, size_t length) {
        this->eventCallback(type, payload, length);
    });
}

void DoubaoASR::eventCallback(WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_PING:
        case WStype_ERROR:
            break;
        case WStype_CONNECTED:
            ESP_LOGD(TAG, "websocket connection success");
            break;
        case WStype_DISCONNECTED:
            ESP_LOGD(TAG, "websocket disconnected");
            break;
        case WStype_TEXT: {
            break;
        }
        case WStype_BIN:
            parseResponse(payload);
            break;
        default:
            break;
    }
}

void DoubaoASR::buildFullClientRequest() {
    JsonDocument doc;
    doc.clear();
    const JsonObject app = doc["app"].to<JsonObject>();
    app["appid"] = Settings::getDoubaoAppId();
    app["cluster"] = "volcengine_streaming_common";
    app["token"] = Settings::getDoubaoAccessToken();
    const JsonObject user = doc["user"].to<JsonObject>();
    user["uid"] = getChipId(nullptr);
    const JsonObject request = doc["request"].to<JsonObject>();
    request["reqid"] = generateTaskId();
    request["nbest"] = 1;
    request["result_type"] = "full";
    request["sequence"] = 1;
    request["workflow"] = "audio_in,resample,partition,vad,fe,decode,itn,nlu_ddc,nlu_punctuate";
    const JsonObject audio = doc["audio"].to<JsonObject>();
    audio["format"] = "raw";
    audio["codec"] = "raw";
    audio["channel"] = 1;
    audio["rate"] = AUDIO_SAMPLE_RATE;
    String payloadStr;
    serializeJson(doc, payloadStr);
    uint8_t payload[payloadStr.length() + 1];
    for (int i = 0; i < payloadStr.length(); i++) {
        payload[i] = static_cast<uint8_t>(payloadStr.charAt(i));
    }
    payload[payloadStr.length()] = '\0';
    std::vector<uint8_t> payloadSize = uint32ToUint8Array(payloadStr.length());
    _requestBuilder.clear();
    // Header (4 bytes)
    _requestBuilder.insert(_requestBuilder.end(), DoubaoTTSDefaultFullClientWsHeader,
                           DoubaoTTSDefaultFullClientWsHeader + sizeof(DoubaoTTSDefaultFullClientWsHeader));
    // Payload length (4 bytes)
    _requestBuilder.insert(_requestBuilder.end(), payloadSize.begin(), payloadSize.end());
    // Payload content
    _requestBuilder.insert(_requestBuilder.end(), payload, payload + payloadStr.length());
}

void DoubaoASR::buildAudioOnlyRequest(uint8_t *audio, const size_t size, const bool lastPacket) {
    _requestBuilder.clear();
    std::vector<uint8_t> payloadLength = uint32ToUint8Array(size);

    if (lastPacket) {
        _requestBuilder.insert(_requestBuilder.end(), DoubaoTTSDefaultLastAudioWsHeader,
                               DoubaoTTSDefaultLastAudioWsHeader + sizeof(DoubaoTTSDefaultLastAudioWsHeader));
    } else {
        _requestBuilder.insert(_requestBuilder.end(), DoubaoTTSDefaultAudioOnlyWsHeader,
                               DoubaoTTSDefaultAudioOnlyWsHeader + sizeof(DoubaoTTSDefaultAudioOnlyWsHeader));
    }

    _requestBuilder.insert(_requestBuilder.end(), payloadLength.begin(), payloadLength.end());
    _requestBuilder.insert(_requestBuilder.end(), audio, audio + size);
}

void DoubaoASR::recognize(DoubaoASRTask task) {
    ESP_LOGD(TAG, "ASR Request: length %d, first: %d, last: %d", task.data.size(), task.firstPacket, task.lastPacket);
    if (task.firstPacket) {
        xEventGroupClearBits(_eventGroup, STT_TASK_COMPLETED_EVENT);
        while (!isConnected()) {
            connect();
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        buildFullClientRequest();
        if (!sendBIN(_requestBuilder.data(), _requestBuilder.size())) {
            ESP_LOGE(TAG, "Send header failed");
        }
        loop();
    }
    buildAudioOnlyRequest(task.data.data(), task.data.size(), task.lastPacket);
    if (!sendBIN(_requestBuilder.data(), _requestBuilder.size())) {
        ESP_LOGE(TAG, "Send audio payload failed");
    }
    loop();
    if (task.lastPacket) {
        while ((xEventGroupWaitBits(_eventGroup, STT_TASK_COMPLETED_EVENT,
                                    false, true, pdMS_TO_TICKS(1)) & STT_TASK_COMPLETED_EVENT) == 0) {
            loop();
            vTaskDelay(1);
        }
        disconnect();
    }
}

void DoubaoASR::parseResponse(const uint8_t *response) {
    const uint8_t messageType = response[1] >> 4;
    const uint8_t *payload = response + 4;
    ESP_LOGV(TAG, "Websocket msg type: %d", messageType);
    switch (messageType) {
        case 0b1001: {
            const uint32_t payloadSize = readInt32(payload);
            payload += 4;
            std::string recognizeResult = readString(payload, payloadSize);
            JsonDocument jsonResult;
            const DeserializationError err = deserializeJson(jsonResult, recognizeResult);
            if (err) {
                ESP_LOGE(TAG, "Parse ASR result failed");
                return;
            }
            const String reqId = jsonResult["reqid"];
            const int32_t code = jsonResult["code"];
            const String message = jsonResult["message"];
            const int32_t sequence = jsonResult["sequence"];
            const JsonArray result = jsonResult["result"];
            ESP_LOGD(TAG, "ASR result, sequence = %d, code = %d, message = %s, size = %d",
                     sequence, code, message.c_str(), result.size());
            if (sequence < 0) {
                xEventGroupSetBits(_eventGroup, STT_TASK_COMPLETED_EVENT);
            }
            if (code == 1000 && result.size() > 0) {
                for (const auto &item: result) {
                    String text = item["text"];
                    
                    // Menampilkan hasil ucapan pengguna di OLED SSD1306
                    Display::showChatMessage("User", text);

                    if (_firstPacket) {
                        _firstPacket = false;
                    }
                    if (sequence < 0) {
                        ESP_LOGI(TAG, "ASR Final text: %s", text.c_str());
                        LLMTask task{};
                        task.message = static_cast<char *>(ps_malloc(sizeof(char) * text.length()));
                        task.length = text.length();
                        text.toCharArray(task.message, task.length);
                        Application::llm()->publishTask(task);
                        _firstPacket = true;
                    }
                }
            } else if (GlobalState::getState() != Listening) {
                GlobalState::setState(Sleep);
            }
            break;
        }
        case 0b1111: {
            const uint32_t errorCode = readInt32(payload);
            payload += 4;
            const uint32_t messageLength = readInt32(payload);
            payload += 4;
            const std::string errorMessage = readString(payload, messageLength);
            ESP_LOGE(TAG, "ASR Failed: code = %u, err = %s", errorCode, errorMessage.c_str());
            xEventGroupSetBits(_eventGroup, STT_TASK_COMPLETED_EVENT);
            break;
        }
        default: {
            break;
        }
    }
}

void DoubaoASR::connect() {
    if (isConnected() || _isConnecting) return;
    _isConnecting = true;
    xTaskCreate([](void *args) {
        auto *self = static_cast<DoubaoASR *>(args);
        while (!self->isConnected()) {
            self->loop();
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        self->_isConnecting = false;
        vTaskDelete(nullptr);
    }, "DoubaoAsrConnect", 4096, this, 1, nullptr);
}
