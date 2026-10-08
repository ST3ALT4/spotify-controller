#pragma once

#include <cstdint>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

class OledDisplay {
public:
    OledDisplay() = default;
    ~OledDisplay();

    esp_err_t init(gpio_num_t sda_pin, gpio_num_t scl_pin, uint8_t i2c_addr = 0x3C);

    void clear();
    void update();

    // Primitive drawing functions
    void drawPixel(int x, int y, bool color);
    void drawString(int x, int y, const char* str, bool invert = false);
    void drawLine(int x0, int y0, int x1, int y1, bool color);
    void fillRect(int x, int y, int w, int h, bool color);

    // Screen layouts specified in AGENTS.md
    void showStartup();
    void showConnecting(const char* ssid);
    void showWifiConnected();
    void showPlayback(bool isPlaying, const char* artist, const char* title, int volume);
    void showNoActiveDevice();
    void showError(const char* line1, const char* line2 = nullptr);

private:
    void sendCommand(uint8_t cmd);
    void sendData(const uint8_t* data, size_t len);

    i2c_master_bus_handle_t bus_handle_{nullptr};
    i2c_master_dev_handle_t dev_handle_{nullptr};
    bool initialized_{false};

    // 128x64 display buffer = 1024 bytes (8 pages of 128 columns)
    static constexpr int WIDTH = 128;
    static constexpr int HEIGHT = 64;
    static constexpr int PAGES = HEIGHT / 8;
    static constexpr int BUFFER_SIZE = WIDTH * PAGES;

    uint8_t buffer_[BUFFER_SIZE]{0};
};
