#pragma once

#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

class LightSensor {
public:
    LightSensor() = default;
    ~LightSensor();

    esp_err_t init(adc_unit_t unit, adc_channel_t channel);

    // Read smoothed raw ADC value (0 - 4095)
    int readRaw();

    // Normalized light level from 0.0 (dark) to 1.0 (bright)
    float getNormalizedLevel();

private:
    adc_oneshot_unit_handle_t adc_handle_{nullptr};
    adc_channel_t channel_{ADC_CHANNEL_0};
    bool initialized_{false};
    float filtered_value_{0.0f};
};
