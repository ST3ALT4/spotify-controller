# ESP32 Spotify Controller

A physical Spotify playback controller built on the ESP32 using ESP-IDF. Control your Spotify playback with dedicated hardware buttons, see track info on an OLED display, and get haptic feedback via a buzzer — all without touching your phone.

## Features

- **Playback control** — Play/pause, next track, previous track
- **Volume control** — Volume up/down buttons
- **OLED display** — Shows current track title, artist, and volume on a 128×64 SSD1306 display
- **Buzzer feedback** — Short beep on button press
- **Light sensor** — Ambient light reading (ADC) for potential display brightness adjustment
- **Spotify OAuth** — Authenticates via a refresh token using the Spotify Web API
- **Wi-Fi** — Connects to your local network and polls Spotify every 2.5 seconds

## Hardware

| Component | Part |
|-----------|------|
| Microcontroller | ESP32 (any standard dev board) |
| Display | 128×64 OLED (SSD1306, I2C) |
| Buttons | 5× tactile push buttons (active-low, internal pull-up) |
| Buzzer | Passive buzzer |
| Light sensor | Photoresistor on ADC1 CH6 (GPIO 34) |

### Pin Map

| Signal | GPIO |
|--------|------|
| Play/Pause | 18 |
| Previous | 19 |
| Next | 23 |
| Volume Down | 25 |
| Volume Up | 26 |
| OLED SDA | 21 |
| OLED SCL | 22 |
| Buzzer | 27 |
| Light Sensor | 34 (ADC1 CH6) |

## Project Structure

```
main/
├── app/            # AppController — top-level state machine & FreeRTOS tasks
├── buzzer/         # Buzzer driver
├── config/         # Pin definitions and compile-time config (credentials, intervals)
├── display/        # SSD1306 OLED driver (I2C, framebuffer)
├── input/          # ButtonManager — GPIO ISR → FreeRTOS queue → callback
├── sensors/        # LightSensor — ADC oneshot driver
├── spotify/        # SpotifyAuth (token refresh) + SpotifyClient (Web API)
├── wifi/           # WifiManager — connects and reports state changes
└── main.cpp        # Entry point — wires all subsystems together
```

## Configuration

Edit [`main/config/device_config.h`](main/config/device_config.h) before building:

```cpp
constexpr const char* WIFI_SSID             = "YOUR_WIFI_SSID";
constexpr const char* WIFI_PASSWORD         = "YOUR_WIFI_PASSWORD";
constexpr const char* SPOTIFY_CLIENT_ID     = "YOUR_SPOTIFY_CLIENT_ID";
constexpr const char* SPOTIFY_CLIENT_SECRET = "YOUR_SPOTIFY_CLIENT_SECRET";
constexpr const char* SPOTIFY_REFRESH_TOKEN = "YOUR_SPOTIFY_REFRESH_TOKEN";
```

### Getting Spotify Credentials

1. Create an app at [developer.spotify.com](https://developer.spotify.com/dashboard).
2. Add `http://localhost/callback` as a Redirect URI.
3. Use the Authorization Code flow to obtain an initial `refresh_token` (any OAuth tool or script works).
4. Paste your `client_id`, `client_secret`, and `refresh_token` into `device_config.h`.

## Building & Flashing

Requires [ESP-IDF v5.x](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/).

```bash
# Set up the ESP-IDF environment
. $IDF_PATH/export.sh

# Configure target (if needed)
idf.py set-target esp32

# Build
idf.py build

# Flash & monitor
idf.py flash monitor
```

## Dependencies

Managed via the ESP-IDF Component Manager:

| Component | Version |
|-----------|---------|
| `espressif/cjson` | ^1.7.18 |

Run `idf.py update-dependencies` if you need to refresh them.

## License

MIT
