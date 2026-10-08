#pragma once

#include <functional>
#include "esp_err.h"
#include "esp_event.h"

enum class WifiState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    FAILED
};

using WifiStateCallback = std::function<void(WifiState)>;

class WifiManager {
public:
    WifiManager() = default;
    ~WifiManager();

    esp_err_t init(const char* ssid, const char* password, WifiStateCallback callback);
    bool isConnected() const { return state_ == WifiState::CONNECTED; }

private:
    static void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

    WifiState state_{WifiState::DISCONNECTED};
    WifiStateCallback callback_{nullptr};
    int retry_num_{0};
    bool initialized_{false};
};
