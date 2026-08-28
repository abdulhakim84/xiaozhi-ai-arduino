#include "GlobalState.h"
#include "Display.h"
#include <utility>

String GlobalState::conversationId = "";
MachineState GlobalState::machineState = Sleep;
String GlobalState::connectingWiFiMessage = "Menghubungkan WiFi";
EventGroupHandle_t GlobalState::eventGroup = xEventGroupCreate();

void GlobalState::setConversationId(String conversationId) {
    GlobalState::conversationId = std::move(conversationId);
}

EventGroupHandle_t GlobalState::getEventGroup() {
    return eventGroup;
}

String GlobalState::getConversationId() {
    return conversationId;
}

EventBits_t GlobalState::getEventBits(const std::vector<MachineState> &states) {
    EventBits_t result = 0;
    for (const auto &state: states) {
        result |= (1 << state);
    }
    return result;
}

MachineState GlobalState::getState() {
    return machineState;
}

void GlobalState::setState(const MachineState state) {
    xEventGroupClearBits(eventGroup, xEventGroupGetBits(eventGroup));
    xEventGroupSetBits(eventGroup, 1 << state);
    machineState = state;

    switch (state) {
        case Sleep:
            Display::updateState("Standby / Siaga");
            break;
        case NetworkConfigurationNotFound:
            Display::updateState("Menunggu WiFi Config");
            break;
        case NetworkConnecting:
            connectingWiFiMessage += ".";
            if (connectingWiFiMessage == "Menghubungkan WiFi....") {
                connectingWiFiMessage = "Menghubungkan WiFi";
            }
            Display::updateState(connectingWiFiMessage);
            break;
        case NetworkConnected:
            Display::updateState("WiFi Terhubung");
            break;
        case NetworkConnectFailed:
            Display::updateState("Gagal Koneksi WiFi");
            break;
        case Listening:
            Display::updateState("Mendengarkan...");
            break;
        case Thinking:
            Display::updateState("Berpikir...");
            break;
        case Recognizing:
            Display::updateState("Mengenali Suara...");
            break;
        case Speaking:
            Display::updateState("Berbicara...");
            break;
        default:
            break;
    }
}
