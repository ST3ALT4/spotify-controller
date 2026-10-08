#include "spotify_auth.h"
#include <cstring>
#include <ctime>
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "mbedtls/base64.h"

static const char* TAG = "SpotifyAuth";

void SpotifyAuth::init(const char* client_id, const char* client_secret, const char* refresh_token) {
    client_id_ = client_id ? client_id : "";
    client_secret_ = client_secret ? client_secret : "";
    refresh_token_ = refresh_token ? refresh_token : "";
}

bool SpotifyAuth::hasCredentials() const {
    return !client_id_.empty() && !client_secret_.empty() && !refresh_token_.empty() &&
           client_id_ != "YOUR_SPOTIFY_CLIENT_ID";
}

bool SpotifyAuth::ensureValidToken() {
    if (!hasCredentials()) {
        return false;
    }
    time_t now;
    time(&now);

    // Refresh if within 60 seconds of expiration or empty
    if (access_token_.empty() || now >= (token_expiry_time_sec_ - 60)) {
        return refreshToken();
    }
    return true;
}

std::string SpotifyAuth::getAccessToken() {
    return access_token_;
}

struct HttpResponseContext {
    char* buffer;
    int buffer_len;
    int received;
};

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    auto ctx = static_cast<HttpResponseContext*>(evt->user_data);
    if (!ctx) return ESP_OK;

    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (ctx->buffer && (ctx->received + evt->data_len < ctx->buffer_len)) {
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

bool SpotifyAuth::refreshToken() {
    if (!hasCredentials()) {
        ESP_LOGW(TAG, "Cannot refresh token: missing credentials");
        return false;
    }

    ESP_LOGI(TAG, "Refreshing Spotify access token...");

    // Create Basic Auth header value: base64(client_id:client_secret)
    std::string credentials_plain = client_id_ + ":" + client_secret_;
    unsigned char base64_buf[256];
    size_t out_len = 0;
    mbedtls_base64_encode(base64_buf, sizeof(base64_buf) - 1, &out_len,
                          reinterpret_cast<const unsigned char*>(credentials_plain.c_str()),
                          credentials_plain.length());
    base64_buf[out_len] = '\0';

    std::string auth_header = "Basic " + std::string(reinterpret_cast<char*>(base64_buf));

    // Request body
    std::string post_data = "grant_type=refresh_token&refresh_token=" + refresh_token_;

    constexpr int RESP_BUF_SIZE = 2048;
    char* resp_buffer = static_cast<char*>(malloc(RESP_BUF_SIZE));
    if (!resp_buffer) {
        ESP_LOGE(TAG, "Out of memory allocating response buffer");
        return false;
    }
    memset(resp_buffer, 0, RESP_BUF_SIZE);

    HttpResponseContext ctx = {
        .buffer = resp_buffer,
        .buffer_len = RESP_BUF_SIZE,
        .received = 0
    };

    esp_http_client_config_t config = {};
    config.url = "https://accounts.spotify.com/api/token";
    config.method = HTTP_METHOD_POST;
    config.event_handler = http_event_handler;
    config.user_data = &ctx;
    config.timeout_ms = 10000;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        free(resp_buffer);
        return false;
    }

    esp_http_client_set_header(client, "Authorization", auth_header.c_str());
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_post_field(client, post_data.c_str(), post_data.length());

    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    bool success = false;
    if (err == ESP_OK && status_code == 200) {
        cJSON* root = cJSON_Parse(resp_buffer);
        if (root) {
            cJSON* token_item = cJSON_GetObjectItem(root, "access_token");
            cJSON* expires_item = cJSON_GetObjectItem(root, "expires_in");
            if (cJSON_IsString(token_item)) {
                access_token_ = token_item->valuestring;
                int expires_in = cJSON_IsNumber(expires_item) ? expires_item->valueint : 3600;

                time_t now;
                time(&now);
                token_expiry_time_sec_ = now + expires_in;
                success = true;
                ESP_LOGI(TAG, "Access token refreshed successfully (expires in %d sec)", expires_in);
            }
            cJSON_Delete(root);
        }
    } else {
        ESP_LOGE(TAG, "Token refresh failed with HTTP %d (err: %s)", status_code, esp_err_to_name(err));
    }

    free(resp_buffer);
    return success;
}
