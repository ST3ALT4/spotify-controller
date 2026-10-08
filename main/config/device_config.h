#pragma once

#include <cstdint>

namespace config {
    // Wi-Fi Credentials (replace or set via provision script/secrets in practice)
    constexpr const char* WIFI_SSID     = "YOUR_WIFI_SSID";
    constexpr const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

    // Spotify API Credentials (placeholders for user to configure)
    constexpr const char* SPOTIFY_CLIENT_ID     = "YOUR_SPOTIFY_CLIENT_ID";
    constexpr const char* SPOTIFY_CLIENT_SECRET = "YOUR_SPOTIFY_CLIENT_SECRET";
    constexpr const char* SPOTIFY_REFRESH_TOKEN = "YOUR_SPOTIFY_REFRESH_TOKEN";

    // Polling interval in milliseconds
    constexpr uint32_t SPOTIFY_POLL_INTERVAL_MS = 2500;

    // OLED Settings
    constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;
    constexpr uint32_t OLED_I2C_FREQ_HZ = 400000;
}
