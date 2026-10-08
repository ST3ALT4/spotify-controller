#include "oled.h"
#include <cstring>
#include <cstdio>
#include "esp_log.h"

static const char* TAG = "OledDisplay";

// Basic 5x7 ASCII font (characters 32 ' ' to 126 '~')
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 ' '
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '\''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 ')'
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // 42 '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 '@'
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 'L'
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 77 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 'V'
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 87 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 'Z'
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 '['
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 '\\'
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ']'
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 '^'
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 '_'
    {0x00, 0x01, 0x02, 0x04, 0x00}, // 96 '`'
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 97 'a'
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 98 'b'
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 99 'c'
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 'd'
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 'e'
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 'f'
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 103 'g'
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 'h'
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 'i'
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 'j'
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 107 'k'
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 'l'
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 'm'
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 'n'
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 'o'
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 'p'
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 'q'
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 'r'
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 's'
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 116 't'
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 'u'
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 'v'
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 'w'
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 'x'
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 'y'
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 'z'
    {0x00, 0x08, 0x36, 0x41, 0x00}, // 123 '{'
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // 124 '|'
    {0x00, 0x41, 0x36, 0x08, 0x00}, // 125 '}'
    {0x08, 0x08, 0x2A, 0x1C, 0x08}  // 126 '~'
};

OledDisplay::~OledDisplay() {
    if (dev_handle_) {
        i2c_master_bus_rm_device(dev_handle_);
        dev_handle_ = nullptr;
    }
    if (bus_handle_) {
        i2c_del_master_bus(bus_handle_);
        bus_handle_ = nullptr;
    }
}

void OledDisplay::sendCommand(uint8_t cmd) {
    if (!dev_handle_) return;
    uint8_t buf[2] = {0x00, cmd}; // Co = 0, D/C# = 0
    i2c_master_transmit(dev_handle_, buf, 2, 100);
}

void OledDisplay::sendData(const uint8_t* data, size_t len) {
    if (!dev_handle_ || len == 0) return;
    // Buffer with 0x40 (Co=0, D/C#=1) prefix
    uint8_t tmp[129];
    while (len > 0) {
        size_t chunk = (len > 128) ? 128 : len;
        tmp[0] = 0x40;
        memcpy(&tmp[1], data, chunk);
        i2c_master_transmit(dev_handle_, tmp, chunk + 1, 100);
        data += chunk;
        len -= chunk;
    }
}

esp_err_t OledDisplay::init(gpio_num_t sda_pin, gpio_num_t scl_pin, uint8_t i2c_addr) {
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = 1,
            .allow_pd = 0
        }
    };
    esp_err_t ret = i2c_new_master_bus(&bus_cfg, &bus_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(ret));
        return ret;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = i2c_addr,
        .scl_speed_hz = 400000,
        .scl_wait_us = 0,
        .flags = { .disable_ack_check = 0 }
    };
    ret = i2c_master_bus_add_device(bus_handle_, &dev_cfg, &dev_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(ret));
        return ret;
    }

    // Initialization sequence for SH1106 / SSD1306 128x64 OLED
    static const uint8_t init_cmds[] = {
        0xAE,       // Display OFF
        0x00, 0x10, // Set low/high column address
        0x40,       // Start line address = 0
        0xB0,       // Page address = 0
        0x81, 0xCF, // Contrast control
        0xA1,       // Set segment remap
        0xA6,       // Normal display (not inverted)
        0xA8, 0x3F, // Multiplex ratio 1/64
        0xC8,       // Output scan direction
        0xD3, 0x00, // Display offset = 0
        0xD5, 0x80, // Display clock divide ratio / osc freq
        0xD9, 0xF1, // Pre-charge period
        0xDA, 0x12, // COM pins config
        0xDB, 0x40, // VCOMH deselect level
        0x8D, 0x14, // Charge pump enable (SSD1306)
        0xAF        // Display ON
    };

    for (uint8_t cmd : init_cmds) {
        sendCommand(cmd);
    }

    initialized_ = true;
    clear();
    update();

    ESP_LOGI(TAG, "OLED Display initialized on SDA=%d, SCL=%d", sda_pin, scl_pin);
    return ESP_OK;
}

void OledDisplay::clear() {
    memset(buffer_, 0x00, BUFFER_SIZE);
}

void OledDisplay::update() {
    if (!initialized_) return;

    for (int page = 0; page < PAGES; ++page) {
        sendCommand(0xB0 + page); // Set page address
        // SH1106 has 132 columns with offset 2; SSD1306 starts at 0.
        // Sending column 2 works cleanly across both 1.3" SH1106 and SSD1306.
        sendCommand(0x02);        // Set lower column address
        sendCommand(0x10);        // Set higher column address
        sendData(&buffer_[page * WIDTH], WIDTH);
    }
}

void OledDisplay::drawPixel(int x, int y, bool color) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    int page = y / 8;
    int bit = y % 8;
    int idx = page * WIDTH + x;
    if (color) {
        buffer_[idx] |= (1 << bit);
    } else {
        buffer_[idx] &= ~(1 << bit);
    }
}

void OledDisplay::drawLine(int x0, int y0, int x1, int y1, bool color) {
    int dx = (x1 >= x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 >= y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void OledDisplay::fillRect(int x, int y, int w, int h, bool color) {
    for (int i = 0; i < w; ++i) {
        for (int j = 0; j < h; ++j) {
            drawPixel(x + i, y + j, color);
        }
    }
}

void OledDisplay::drawString(int x, int y, const char* str, bool invert) {
    if (!str) return;
    int cur_x = x;
    while (*str && cur_x < WIDTH) {
        char c = *str++;
        if (c < 32 || c > 126) c = ' ';
        const uint8_t* glyph = font5x7[c - 32];
        for (int col = 0; col < 5; ++col) {
            if (cur_x + col >= WIDTH) break;
            uint8_t line = glyph[col];
            for (int row = 0; row < 7; ++row) {
                bool pixel = (line & (1 << row)) != 0;
                drawPixel(cur_x + col, y + row, invert ? !pixel : pixel);
            }
        }
        // Spacing pixel column
        for (int row = 0; row < 7; ++row) {
            drawPixel(cur_x + 5, y + row, invert ? true : false);
        }
        cur_x += 6;
    }
}

void OledDisplay::showStartup() {
    clear();
    drawString(16, 8, "ESP32 Spotify");
    drawString(40, 22, "Remote");
    drawLine(10, 36, 118, 36, true);
    drawString(22, 46, "Starting up...");
    update();
}

void OledDisplay::showConnecting(const char* ssid) {
    clear();
    drawString(16, 4, "ESP32 Spotify");
    drawString(40, 16, "Remote");
    drawLine(10, 28, 118, 28, true);
    drawString(10, 36, "Connecting to:");
    char buf[20];
    snprintf(buf, sizeof(buf), "%.18s", ssid ? ssid : "Wi-Fi");
    drawString(10, 48, buf);
    update();
}

void OledDisplay::showWifiConnected() {
    clear();
    drawString(16, 8, "ESP32 Spotify");
    drawString(40, 22, "Remote");
    drawLine(10, 36, 118, 36, true);
    drawString(34, 46, "Wi-Fi OK!");
    update();
}

void OledDisplay::showPlayback(bool isPlaying, const char* artist, const char* title, int volume) {
    clear();
    // Header
    drawString(0, 0, "Spotify");
    if (isPlaying) {
        drawString(70, 0, "> PLAYING");
    } else {
        drawString(70, 0, "|| PAUSED");
    }
    drawLine(0, 11, 127, 11, true);

    // Title (up to 20 chars per line, truncated cleanly)
    char title_buf[22];
    snprintf(title_buf, sizeof(title_buf), "%.20s", (title && strlen(title) > 0) ? title : "Unknown Title");
    drawString(0, 18, title_buf);

    // Artist
    char artist_buf[22];
    snprintf(artist_buf, sizeof(artist_buf), "%.20s", (artist && strlen(artist) > 0) ? artist : "Unknown Artist");
    drawString(0, 32, artist_buf);

    // Bottom bar: Volume indicator
    drawLine(0, 48, 127, 48, true);
    char vol_buf[16];
    snprintf(vol_buf, sizeof(vol_buf), "Vol: %d%%", volume);
    drawString(0, 54, vol_buf);

    // Simple visual volume meter bar
    int bar_width = (volume * 50) / 100;
    if (bar_width > 50) bar_width = 50;
    if (bar_width > 0) {
        fillRect(72, 55, bar_width, 6, true);
    }
    // Volume box outline
    drawLine(70, 53, 123, 53, true);
    drawLine(70, 62, 123, 62, true);
    drawLine(70, 53, 70, 62, true);
    drawLine(123, 53, 123, 62, true);

    update();
}

void OledDisplay::showNoActiveDevice() {
    clear();
    drawString(0, 0, "Spotify");
    drawLine(0, 12, 127, 12, true);
    drawString(10, 26, "No active device");
    drawString(10, 40, "Open Spotify app");
    drawString(10, 52, "and start play");
    update();
}

void OledDisplay::showError(const char* line1, const char* line2) {
    clear();
    drawString(0, 0, "Error");
    drawLine(0, 12, 127, 12, true);
    if (line1) drawString(4, 26, line1);
    if (line2) drawString(4, 42, line2);
    update();
}
