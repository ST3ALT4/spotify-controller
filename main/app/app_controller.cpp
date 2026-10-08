#include "app_controller.h"
#include "config/device_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char* TAG = "AppController";

enum class RemoteCommand {
    PLAY_PAUSE,
    PREVIOUS,
    NEXT,
    VOL_UP,
    VOL_DOWN,
    SYNC_STATE
};

AppController::AppController(Buzzer& buzzer,
                             OledDisplay& display,
                             LightSensor& light_sensor,
                             WifiManager& wifi_manager,
                             SpotifyAuth& auth,
                             SpotifyClient& spotify)
    : buzzer_(buzzer),
      display_(display),
      light_sensor_(light_sensor),
      wifi_manager_(wifi_manager),
      auth_(auth),
      spotify_(spotify) {}

void AppController::handleButtonEvent(const ButtonEvent& event) {
    ESP_LOGI(TAG, "Button event received: id=%d, type=%d", static_cast<int>(event.id), static_cast<int>(event.type));

    // Instant local audio/UI feedback before network call
    RemoteCommand cmd;
    switch (event.id) {
        case ButtonId::PLAY_PAUSE:
            buzzer_.playShortBeep();
            current_playback_.is_playing = !current_playback_.is_playing;
            display_.showPlayback(current_playback_.is_playing,
                                  current_playback_.artist_name.c_str(),
                                  current_playback_.track_title.c_str(),
                                  current_playback_.volume_percent);
            cmd = RemoteCommand::PLAY_PAUSE;
            break;

        case ButtonId::PREVIOUS:
            buzzer_.playPrevTone();
            cmd = RemoteCommand::PREVIOUS;
            break;

        case ButtonId::NEXT:
            buzzer_.playNextTone();
            cmd = RemoteCommand::NEXT;
            break;

        case ButtonId::VOL_UP:
            buzzer_.playVolumeTone();
            current_playback_.volume_percent = std::min(100, current_playback_.volume_percent + 10);
            display_.showPlayback(current_playback_.is_playing,
                                  current_playback_.artist_name.c_str(),
                                  current_playback_.track_title.c_str(),
                                  current_playback_.volume_percent);
            cmd = RemoteCommand::VOL_UP;
            break;

        case ButtonId::VOL_DOWN:
            buzzer_.playVolumeTone();
            current_playback_.volume_percent = std::max(0, current_playback_.volume_percent - 10);
            display_.showPlayback(current_playback_.is_playing,
                                  current_playback_.artist_name.c_str(),
                                  current_playback_.track_title.c_str(),
                                  current_playback_.volume_percent);
            cmd = RemoteCommand::VOL_DOWN;
            break;

        default:
            return;
    }

    if (command_queue_) {
        xQueueSend(command_queue_, &cmd, 0);
    }
}

void AppController::handleWifiState(WifiState state) {
    switch (state) {
        case WifiState::CONNECTING:
            wifi_connected_ = false;
            display_.showConnecting(config::WIFI_SSID);
            break;
        case WifiState::CONNECTED:
            wifi_connected_ = true;
            buzzer_.playSuccess();
            display_.showWifiConnected();
            if (command_queue_) {
                RemoteCommand sync = RemoteCommand::SYNC_STATE;
                xQueueSend(command_queue_, &sync, 0);
            }
            break;
        case WifiState::FAILED:
        case WifiState::DISCONNECTED:
            wifi_connected_ = false;
            display_.showError("Wi-Fi Disconnected", "Retrying...");
            break;
    }
}

void AppController::spotifyWorkerTask(void* arg) {
    auto self = static_cast<AppController*>(arg);
    RemoteCommand cmd;

    while (self->running_) {
        // Wait for user command or timeout to poll state
        TickType_t waitTicks = self->wifi_connected_ ? pdMS_TO_TICKS(config::SPOTIFY_POLL_INTERVAL_MS) : pdMS_TO_TICKS(1000);
        BaseType_t received = xQueueReceive(self->command_queue_, &cmd, waitTicks);

        if (!self->wifi_connected_) {
            continue;
        }

        if (received == pdTRUE) {
            SpotifyResult res = SpotifyResult::SUCCESS;
            switch (cmd) {
                case RemoteCommand::PLAY_PAUSE:
                    // Invert back current_playback_.is_playing for the command parameter
                    res = self->spotify_.togglePlayback(!self->current_playback_.is_playing);
                    break;
                case RemoteCommand::PREVIOUS:
                    res = self->spotify_.previousTrack();
                    break;
                case RemoteCommand::NEXT:
                    res = self->spotify_.nextTrack();
                    break;
                case RemoteCommand::VOL_UP:
                    res = self->spotify_.volumeUp();
                    break;
                case RemoteCommand::VOL_DOWN:
                    res = self->spotify_.volumeDown();
                    break;
                case RemoteCommand::SYNC_STATE:
                    break;
            }

            if (res != SpotifyResult::SUCCESS) {
                if (res == SpotifyResult::NO_ACTIVE_DEVICE) {
                    self->display_.showNoActiveDevice();
                } else if (res == SpotifyResult::AUTH_ERROR) {
                    self->display_.showError("Auth Error", "Check credentials");
                } else {
                    self->buzzer_.playError();
                }
            }
            vTaskDelay(pdMS_TO_TICKS(300)); // Small delay before reading updated status
        }

        // Periodic state synchronization
        if (self->wifi_connected_ && self->auth_.hasCredentials()) {
            PlaybackState newState;
            SpotifyResult pollRes = self->spotify_.getPlaybackState(newState);
            if (pollRes == SpotifyResult::SUCCESS) {
                if (!newState.has_active_device) {
                    self->display_.showNoActiveDevice();
                } else {
                    // Update display only if changed
                    if (newState.is_playing != self->current_playback_.is_playing ||
                        newState.track_title != self->current_playback_.track_title ||
                        newState.artist_name != self->current_playback_.artist_name ||
                        newState.volume_percent != self->current_playback_.volume_percent) {
                        self->current_playback_ = newState;
                        self->display_.showPlayback(newState.is_playing,
                                                    newState.artist_name.c_str(),
                                                    newState.track_title.c_str(),
                                                    newState.volume_percent);
                    }
                }
            } else if (pollRes == SpotifyResult::NO_ACTIVE_DEVICE) {
                self->display_.showNoActiveDevice();
            }
        }
    }
    vTaskDelete(nullptr);
}

void AppController::controllerTask(void* arg) {
    auto self = static_cast<AppController*>(arg);

    // Periodically sample light sensor without blocking UI/Spotify
    while (self->running_) {
        float light_level = self->light_sensor_.getNormalizedLevel();
        (void)light_level; // Available for adaptive OLED dimming in polish phase
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    vTaskDelete(nullptr);
}

void AppController::start() {
    running_ = true;
    command_queue_ = xQueueCreate(10, sizeof(RemoteCommand));

    // Spotify worker task for network requests and status polling
    xTaskCreate(spotifyWorkerTask, "spotify_task", 6144, this, 5, nullptr);

    // Sensor monitor task
    xTaskCreate(controllerTask, "sensor_task", 2048, this, 2, nullptr);

    ESP_LOGI(TAG, "Application Controller started");
}
