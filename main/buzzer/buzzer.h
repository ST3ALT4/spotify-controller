#pragma once

#include <cstdint>
#include "driver/gpio.h"
#include "esp_err.h"

class Buzzer {
public:
    Buzzer() = default;
    ~Buzzer();

    esp_err_t init(gpio_num_t pin);

    // Play a tone non-blockingly using FreeRTOS timer/task
    void playTone(uint32_t freqHz, uint32_t durationMs);

    // High level feedback sounds per AGENTS.md
    void playShortBeep();      // Play/Pause, standard action
    void playNextTone();       // Higher pitch beep for Next
    void playPrevTone();       // Lower pitch beep for Previous
    void playVolumeTone();     // Very short beep for volume changes
    void playSuccess();        // Wi-Fi connected / OK tone
    void playError();          // Double short beeps on error

    void stop();

private:
    gpio_num_t pin_{GPIO_NUM_NC};
    bool initialized_{false};
};
