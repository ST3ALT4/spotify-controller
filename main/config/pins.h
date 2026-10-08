#pragma once

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

namespace pins {
    // Buttons (configured with internal pull-up, active low)
    constexpr gpio_num_t BUTTON_PLAY_PAUSE = GPIO_NUM_18;
    constexpr gpio_num_t BUTTON_PREVIOUS   = GPIO_NUM_19;
    constexpr gpio_num_t BUTTON_NEXT       = GPIO_NUM_23;
    constexpr gpio_num_t BUTTON_VOL_DOWN   = GPIO_NUM_25;
    constexpr gpio_num_t BUTTON_VOL_UP     = GPIO_NUM_26;

    // OLED I2C bus
    constexpr gpio_num_t OLED_SDA          = GPIO_NUM_21;
    constexpr gpio_num_t OLED_SCL          = GPIO_NUM_22;

    // Feedback
    constexpr gpio_num_t BUZZER            = GPIO_NUM_27;

    // Photoresistor (ADC1 Channel 6 -> GPIO 34)
    constexpr gpio_num_t LIGHT_SENSOR_PIN  = GPIO_NUM_34;
    constexpr adc_channel_t LIGHT_SENSOR_ADC_CH = ADC_CHANNEL_6;
    constexpr adc_unit_t LIGHT_SENSOR_ADC_UNIT  = ADC_UNIT_1;
}
