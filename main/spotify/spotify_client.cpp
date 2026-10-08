#include "spotify_client.h"
#include <cstring>
#include <cstdio>
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"

static const char* TAG = "SpotifyClient";

void SpotifyClient::init(SpotifyAuth* auth) {
    auth_ = auth;
}

struct HttpBufferContext {
    char* buffer;
    size_t buffer_size;
    size_t received;
};

static esp_err_t client_http_event_handler(esp_http_client_event_t *evt) {
    auto ctx = static_cast<HttpBufferContext*>(evt->user_data);
    if (!ctx || !ctx->buffer) return ESP_OK;

    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (ctx->received + evt->data_len < ctx->buffer_size) {
                memcpy(ctx->buffer + ctx->received, evt->data, evt->data_len);
                ctx->received += evt->data_len;
                ctx->buffer[ctx->received] = '\0';
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

SpotifyResult SpotifyClient::sendRequest(esp_http_client_method_t method,
                                         const char* endpoint,
                                         const char* body,
                                         char* resp_buf,
                                         size_t resp_buf_size,
                                         int& out_http_status) {
    if (!auth_) return SpotifyResult::AUTH_ERROR;
    if (!auth_->ensureValidToken()) {
        return SpotifyResult::AUTH_ERROR;
    }

    std::string token = auth_->getAccessToken();
    if (token.empty()) {
        return SpotifyResult::AUTH_ERROR;
    }

    std::string url = "https://api.spotify.com" + std::string(endpoint);

    HttpBufferContext ctx = {
        .buffer = resp_buf,
        .buffer_size = resp_buf_size,
        .received = 0
    };
    if (resp_buf && resp_buf_size > 0) {
        memset(resp_buf, 0, resp_buf_size);
    }

    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.method = method;
    config.event_handler = client_http_event_handler;
    config.user_data = &ctx;
    config.timeout_ms = 8000;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        return SpotifyResult::NETWORK_ERROR;
    }

    std::string auth_header = "Bearer " + token;
    esp_http_client_set_header(client, "Authorization", auth_header.c_str());
    esp_http_client_set_header(client, "Content-Type", "application/json");

    if (body && strlen(body) > 0) {
        esp_http_client_set_post_field(client, body, strlen(body));
    }

    esp_err_t err = esp_http_client_perform(client);
    out_http_status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP perform error: %s", esp_err_to_name(err));
        return SpotifyResult::NETWORK_ERROR;
    }

    // Handle Spotify HTTP response codes
    if (out_http_status == 200 || out_http_status == 204) {
        return SpotifyResult::SUCCESS;
    } else if (out_http_status == 401) {
        // Token expired; try refreshing once
        ESP_LOGW(TAG, "HTTP 401 received. Forcing token refresh...");
        if (auth_->refreshToken()) {
            // One-time retry
            return sendRequest(method, endpoint, body, resp_buf, resp_buf_size, out_http_status);
        }
        return SpotifyResult::AUTH_ERROR;
    } else if (out_http_status == 404) {
        ESP_LOGW(TAG, "HTTP 404: No active Spotify device found");
        return SpotifyResult::NO_ACTIVE_DEVICE;
    } else if (out_http_status == 429) {
        ESP_LOGW(TAG, "HTTP 429: Rate limited by Spotify");
        return SpotifyResult::RATE_LIMITED;
    } else if (out_http_status >= 500) {
        ESP_LOGW(TAG, "HTTP %d: Spotify server error", out_http_status);
        return SpotifyResult::SERVER_ERROR;
    }

    ESP_LOGW(TAG, "Spotify request returned status %d", out_http_status);
    return SpotifyResult::SUCCESS;
}

SpotifyResult SpotifyClient::play() {
    int status = 0;
    return sendRequest(HTTP_METHOD_PUT, "/v1/me/player/play", "", nullptr, 0, status);
}

SpotifyResult SpotifyClient::pause() {
    int status = 0;
    return sendRequest(HTTP_METHOD_PUT, "/v1/me/player/pause", "", nullptr, 0, status);
}

SpotifyResult SpotifyClient::togglePlayback(bool currentlyPlaying) {
    return currentlyPlaying ? pause() : play();
}

SpotifyResult SpotifyClient::nextTrack() {
    int status = 0;
    return sendRequest(HTTP_METHOD_POST, "/v1/me/player/next", "", nullptr, 0, status);
}

SpotifyResult SpotifyClient::previousTrack() {
    int status = 0;
    return sendRequest(HTTP_METHOD_POST, "/v1/me/player/previous", "", nullptr, 0, status);
}

SpotifyResult SpotifyClient::setVolume(int volumePercent) {
    if (volumePercent < 0) volumePercent = 0;
    if (volumePercent > 100) volumePercent = 100;
    current_volume_ = volumePercent;

    char query[64];
    snprintf(query, sizeof(query), "/v1/me/player/volume?volume_percent=%d", volumePercent);
    int status = 0;
    return sendRequest(HTTP_METHOD_PUT, query, "", nullptr, 0, status);
}

SpotifyResult SpotifyClient::volumeUp() {
    int new_vol = current_volume_ + 10;
    if (new_vol > 100) new_vol = 100;
    return setVolume(new_vol);
}

SpotifyResult SpotifyClient::volumeDown() {
    int new_vol = current_volume_ - 10;
    if (new_vol < 0) new_vol = 0;
    return setVolume(new_vol);
}

SpotifyResult SpotifyClient::getPlaybackState(PlaybackState& state) {
    constexpr size_t BUF_SIZE = 4096;
    char* json_buf = static_cast<char*>(malloc(BUF_SIZE));
    if (!json_buf) {
        return SpotifyResult::NETWORK_ERROR;
    }

    int http_status = 0;
    SpotifyResult res = sendRequest(HTTP_METHOD_GET, "/v1/me/player", nullptr, json_buf, BUF_SIZE, http_status);

    if (res != SpotifyResult::SUCCESS) {
        free(json_buf);
        if (http_status == 204) {
            state.has_active_device = false;
            return SpotifyResult::NO_ACTIVE_DEVICE;
        }
        return res;
    }

    // 204 No Content means no active device
    if (http_status == 204 || strlen(json_buf) == 0) {
        state.has_active_device = false;
        free(json_buf);
        return SpotifyResult::NO_ACTIVE_DEVICE;
    }

    state.has_active_device = true;

    cJSON* root = cJSON_Parse(json_buf);
    if (root) {
        cJSON* is_playing_item = cJSON_GetObjectItem(root, "is_playing");
        if (cJSON_IsBool(is_playing_item)) {
            state.is_playing = cJSON_IsTrue(is_playing_item);
        }

        cJSON* device_obj = cJSON_GetObjectItem(root, "device");
        if (device_obj) {
            cJSON* vol_item = cJSON_GetObjectItem(device_obj, "volume_percent");
            if (cJSON_IsNumber(vol_item)) {
                state.volume_percent = vol_item->valueint;
                current_volume_ = state.volume_percent;
            }
        }

        cJSON* item_obj = cJSON_GetObjectItem(root, "item");
        if (item_obj) {
            cJSON* name_item = cJSON_GetObjectItem(item_obj, "name");
            if (cJSON_IsString(name_item)) {
                state.track_title = name_item->valuestring;
            }

            cJSON* artists_arr = cJSON_GetObjectItem(item_obj, "artists");
            if (cJSON_IsArray(artists_arr) && cJSON_GetArraySize(artists_arr) > 0) {
                cJSON* first_artist = cJSON_GetArrayItem(artists_arr, 0);
                if (first_artist) {
                    cJSON* artist_name = cJSON_GetObjectItem(first_artist, "name");
                    if (cJSON_IsString(artist_name)) {
                        state.artist_name = artist_name->valuestring;
                    }
                }
            }
        }
        cJSON_Delete(root);
    }

    free(json_buf);
    return SpotifyResult::SUCCESS;
}
