#include "wifi_manager.h"
#include <cstring>
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "WifiManager";
static constexpr int MAX_RETRIES = 10;

WifiManager::~WifiManager() {
    if (initialized_) {
        esp_wifi_stop();
        esp_wifi_deinit();
    }
}

void WifiManager::eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    auto self = static_cast<WifiManager*>(arg);

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi Station started, connecting...");
        esp_wifi_connect();
        self->state_ = WifiState::CONNECTING;
        if (self->callback_) self->callback_(self->state_);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (self->retry_num_ < MAX_RETRIES) {
            self->retry_num_++;
            ESP_LOGW(TAG, "Wi-Fi disconnected. Retry %d/%d...", self->retry_num_, MAX_RETRIES);
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_wifi_connect();
            self->state_ = WifiState::CONNECTING;
        } else {
            ESP_LOGE(TAG, "Failed to connect to Wi-Fi after %d attempts", MAX_RETRIES);
            self->state_ = WifiState::FAILED;
            // Backoff retry after 10s
            vTaskDelay(pdMS_TO_TICKS(10000));
            self->retry_num_ = 0;
            esp_wifi_connect();
        }
        if (self->callback_) self->callback_(self->state_);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto event = static_cast<ip_event_got_ip_t*>(event_data);
        ESP_LOGI(TAG, "Wi-Fi connected! Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        self->retry_num_ = 0;
        self->state_ = WifiState::CONNECTED;
        if (self->callback_) self->callback_(self->state_);
    }
}

esp_err_t WifiManager::init(const char* ssid, const char* password, WifiStateCallback callback) {
    callback_ = callback;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &WifiManager::eventHandler,
                                                        this,
                                                        nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &WifiManager::eventHandler,
                                                        this,
                                                        nullptr));

    wifi_config_t wifi_config = {};
    strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), ssid ? ssid : "", sizeof(wifi_config.sta.ssid) - 1);
    strncpy(reinterpret_cast<char*>(wifi_config.sta.password), password ? password : "", sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    initialized_ = true;
    ESP_LOGI(TAG, "Wi-Fi initialized for SSID: %s", ssid ? ssid : "");
    return ESP_OK;
}
