#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"

#include "config/pins.h"
#include "config/device_config.h"

#include "buzzer/buzzer.h"
#include "display/oled.h"
#include "sensors/light_sensor.h"
#include "input/buttons.h"
#include "wifi/wifi_manager.h"
#include "spotify/spotify_auth.h"
#include "spotify/spotify_client.h"
#include "app/app_controller.h"

static const char* TAG = "Main";

// Static subsystem instances
static Buzzer        s_buzzer;
static OledDisplay   s_display;
static LightSensor   s_light_sensor;
static ButtonManager s_buttons;
static WifiManager   s_wifi;
static SpotifyAuth   s_auth;
static SpotifyClient s_spotify;
static AppController s_app_controller(s_buzzer, s_display, s_light_sensor, s_wifi, s_auth, s_spotify);

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32 Spotify Controller...");

    // 1. Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Hardware Subsystems
    ESP_LOGI(TAG, "Initializing hardware subsystems...");
    s_buzzer.init(pins::BUZZER);
    s_display.init(pins::OLED_SDA, pins::OLED_SCL, config::OLED_I2C_ADDRESS);
    s_display.showStartup();
    s_buzzer.playShortBeep();

    s_light_sensor.init(pins::LIGHT_SENSOR_ADC_UNIT, pins::LIGHT_SENSOR_ADC_CH);

    // 3. Spotify Authentication & Client
    s_auth.init(config::SPOTIFY_CLIENT_ID, config::SPOTIFY_CLIENT_SECRET, config::SPOTIFY_REFRESH_TOKEN);
    s_spotify.init(&s_auth);

    // 4. Start App Controller
    s_app_controller.start();

    // 5. Button input manager with callback routed to App Controller
    s_buttons.init([](const ButtonEvent& evt) {
        s_app_controller.handleButtonEvent(evt);
    });

    // 6. Wi-Fi Manager with status routed to App Controller
    s_wifi.init(config::WIFI_SSID, config::WIFI_PASSWORD, [](WifiState state) {
        s_app_controller.handleWifiState(state);
    });

    ESP_LOGI(TAG, "ESP32 Spotify Controller initialization complete.");
}
