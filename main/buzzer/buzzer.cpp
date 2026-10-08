#include "buzzer.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

static const char* TAG = "Buzzer";

static TimerHandle_t s_buzzer_timer = nullptr;

static void buzzer_timer_callback(TimerHandle_t xTimer) {
    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
}

Buzzer::~Buzzer() {
    stop();
    if (s_buzzer_timer) {
        xTimerDelete(s_buzzer_timer, portMAX_DELAY);
        s_buzzer_timer = nullptr;
    }
}

esp_err_t Buzzer::init(gpio_num_t pin) {
    pin_ = pin;

    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode       = LEDC_LOW_SPEED_MODE;
    ledc_timer.duty_resolution  = LEDC_TIMER_10_BIT;
    ledc_timer.timer_num        = LEDC_TIMER_0;
    ledc_timer.freq_hz          = 2000;
    ledc_timer.clk_cfg          = LEDC_AUTO_CLK;

    esp_err_t ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_timer_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ledc_channel_config_t ledc_channel = {};
    ledc_channel.gpio_num       = pin_;
    ledc_channel.speed_mode     = LEDC_LOW_SPEED_MODE;
    ledc_channel.channel        = LEDC_CHANNEL_0;
    ledc_channel.timer_sel      = LEDC_TIMER_0;
    ledc_channel.duty           = 0;
    ledc_channel.hpoint         = 0;

    ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_channel_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    s_buzzer_timer = xTimerCreate("BuzzTimer", pdMS_TO_TICKS(100), pdFALSE, nullptr, buzzer_timer_callback);

    initialized_ = true;
    ESP_LOGI(TAG, "Buzzer initialized on GPIO %d", pin_);
    return ESP_OK;
}

void Buzzer::playTone(uint32_t freqHz, uint32_t durationMs) {
    if (!initialized_ || freqHz == 0) return;

    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freqHz);
    // 50% duty cycle on 10-bit resolution = 512
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    if (s_buzzer_timer) {
        xTimerStop(s_buzzer_timer, 0);
        xTimerChangePeriod(s_buzzer_timer, pdMS_TO_TICKS(durationMs), 0);
        xTimerStart(s_buzzer_timer, 0);
    }
}

void Buzzer::stop() {
    if (!initialized_) return;
    if (s_buzzer_timer) {
        xTimerStop(s_buzzer_timer, 0);
    }
    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
}

void Buzzer::playShortBeep() {
    playTone(2000, 60);
}

void Buzzer::playNextTone() {
    playTone(2800, 70);
}

void Buzzer::playPrevTone() {
    playTone(1500, 70);
}

void Buzzer::playVolumeTone() {
    playTone(2200, 30);
}

void Buzzer::playSuccess() {
    // A nice rising chirpy feedback
    playTone(2400, 120);
}

static void error_tone_task(void* param) {
    auto buzzer = static_cast<Buzzer*>(param);
    buzzer->playTone(1000, 60);
    vTaskDelay(pdMS_TO_TICKS(100));
    buzzer->playTone(800, 100);
    vTaskDelete(nullptr);
}

void Buzzer::playError() {
    if (!initialized_) return;
    xTaskCreate(error_tone_task, "buzz_err", 2048, this, 3, nullptr);
}
