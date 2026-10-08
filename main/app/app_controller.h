#pragma once

#include "buzzer/buzzer.h"
#include "display/oled.h"
#include "input/buttons.h"
#include "sensors/light_sensor.h"
#include "wifi/wifi_manager.h"
#include "spotify/spotify_auth.h"
#include "spotify/spotify_client.h"

class AppController {
public:
    AppController(Buzzer& buzzer,
                  OledDisplay& display,
                  LightSensor& light_sensor,
                  WifiManager& wifi_manager,
                  SpotifyAuth& auth,
                  SpotifyClient& spotify);
    ~AppController() = default;

    void start();
    void handleButtonEvent(const ButtonEvent& event);
    void handleWifiState(WifiState state);

private:
    static void controllerTask(void* arg);
    static void spotifyWorkerTask(void* arg);

    Buzzer& buzzer_;
    OledDisplay& display_;
    LightSensor& light_sensor_;
    WifiManager& wifi_manager_;
    SpotifyAuth& auth_;
    SpotifyClient& spotify_;

    PlaybackState current_playback_{};
    QueueHandle_t command_queue_{nullptr};
    bool wifi_connected_{false};
    bool running_{false};
};
