#include "light_sensor.h"
#include "esp_log.h"

static const char* TAG = "LightSensor";

LightSensor::~LightSensor() {
    if (adc_handle_) {
        adc_oneshot_del_unit(adc_handle_);
        adc_handle_ = nullptr;
    }
}

esp_err_t LightSensor::init(adc_unit_t unit, adc_channel_t channel) {
    channel_ = channel;

    adc_oneshot_unit_init_cfg_t init_config = {};
    init_config.unit_id = unit;
    init_config.clk_src = ADC_RTC_CLK_SRC_DEFAULT;
    init_config.ulp_mode = ADC_ULP_MODE_DISABLE;

    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %s", esp_err_to_name(ret));
        return ret;
    }

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_oneshot_config_channel(adc_handle_, channel_, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_config_channel failed: %s", esp_err_to_name(ret));
        return ret;
    }

    initialized_ = true;
    readRaw(); // seed filtered_value_
    ESP_LOGI(TAG, "Light sensor initialized on ADC channel %d", channel_);
    return ESP_OK;
}

int LightSensor::readRaw() {
    if (!initialized_ || !adc_handle_) return 0;

    int raw_reading = 0;
    esp_err_t ret = adc_oneshot_read(adc_handle_, channel_, &raw_reading);
    if (ret != ESP_OK) {
        return static_cast<int>(filtered_value_);
    }

    // Exponential moving average filter (alpha = 0.2)
    constexpr float alpha = 0.2f;
    if (filtered_value_ == 0.0f) {
        filtered_value_ = static_cast<float>(raw_reading);
    } else {
        filtered_value_ = alpha * raw_reading + (1.0f - alpha) * filtered_value_;
    }

    return static_cast<int>(filtered_value_);
}

float LightSensor::getNormalizedLevel() {
    int raw = readRaw();
    float level = static_cast<float>(raw) / 4095.0f;
    if (level < 0.0f) level = 0.0f;
    if (level > 1.0f) level = 1.0f;
    return level;
}
