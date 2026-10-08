#pragma once

#include <string>
#include <cstdint>
#include "esp_err.h"

class SpotifyAuth {
public:
    SpotifyAuth() = default;
    ~SpotifyAuth() = default;

    void init(const char* client_id, const char* client_secret, const char* refresh_token);

    // Checks token validity and performs refresh if expired or about to expire
    bool ensureValidToken();

    // Force a token refresh (e.g. after receiving HTTP 401 Unauthorized)
    bool refreshToken();

    // Retrieve active access token (thread-safe copy)
    std::string getAccessToken();

    bool hasCredentials() const;

private:
    std::string client_id_;
    std::string client_secret_;
    std::string refresh_token_;

    std::string access_token_;
    int64_t token_expiry_time_sec_{0};
};
