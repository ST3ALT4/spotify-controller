#pragma once

#include <string>
#include "esp_err.h"
#include "esp_http_client.h"
#include "spotify_auth.h"

struct PlaybackState {
    bool has_active_device{false};
    bool is_playing{false};
    std::string track_title;
    std::string artist_name;
    int volume_percent{50};
};

enum class SpotifyResult {
    SUCCESS,
    NO_ACTIVE_DEVICE,
    AUTH_ERROR,
    RATE_LIMITED,
    NETWORK_ERROR,
    SERVER_ERROR
};

class SpotifyClient {
public:
    SpotifyClient() = default;
    ~SpotifyClient() = default;

    void init(SpotifyAuth* auth);

    // Core playback operations
    SpotifyResult play();
    SpotifyResult pause();
    SpotifyResult togglePlayback(bool currentlyPlaying);
    SpotifyResult nextTrack();
    SpotifyResult previousTrack();
    SpotifyResult volumeUp();
    SpotifyResult volumeDown();
    SpotifyResult setVolume(int volumePercent);

    // Polling current state
    SpotifyResult getPlaybackState(PlaybackState& state);

private:
    SpotifyResult sendRequest(esp_http_client_method_t method, const char* endpoint, const char* body, char* resp_buf, size_t resp_buf_size, int& out_http_status);

    SpotifyAuth* auth_{nullptr};
    int current_volume_{50};
};
