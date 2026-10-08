#include "buttons.h"
#include "config/pins.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char* TAG = "ButtonManager";

struct ButtonPinMap {
    gpio_num_t pin;
    ButtonId id;
};

static const ButtonPinMap BUTTON_MAP[] = {
    { pins::BUTTON_PLAY_PAUSE, ButtonId::PLAY_PAUSE },
    { pins::BUTTON_PREVIOUS,   ButtonId::PREVIOUS },
    { pins::BUTTON_NEXT,       ButtonId::NEXT },
    { pins::BUTTON_VOL_DOWN,   ButtonId::VOL_DOWN },
    { pins::BUTTON_VOL_UP,     ButtonId::VOL_UP },
};
static constexpr size_t NUM_BUTTONS = sizeof(BUTTON_MAP) / sizeof(BUTTON_MAP[0]);

struct RawGpioEvent {
    gpio_num_t pin;
    int level;
    int64_t timestamp_us;
};

// Global button manager queue pointer for ISR
static QueueHandle_t s_gpio_evt_queue = nullptr;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    gpio_num_t pin = static_cast<gpio_num_t>(reinterpret_cast<intptr_t>(arg));
    RawGpioEvent evt;
    evt.pin = pin;
    evt.level = gpio_get_level(pin);
    evt.timestamp_us = esp_timer_get_time();
    if (s_gpio_evt_queue) {
        xQueueSendFromISR(s_gpio_evt_queue, &evt, nullptr);
    }
}

ButtonManager::~ButtonManager() {
    running_ = false;
    if (taskHandle_) {
        vTaskDelete(taskHandle_);
        taskHandle_ = nullptr;
    }
    if (s_gpio_evt_queue) {
        vQueueDelete(s_gpio_evt_queue);
        s_gpio_evt_queue = nullptr;
    }
}

void ButtonManager::inputTask(void* arg) {
    auto self = static_cast<ButtonManager*>(arg);
    RawGpioEvent evt;

    // Track state of each button
    int64_t press_start_us[NUM_BUTTONS] = {0};
    bool is_pressed[NUM_BUTTONS] = {false};

    while (self->running_) {
        if (xQueueReceive(s_gpio_evt_queue, &evt, portMAX_DELAY) == pdTRUE) {
            // Find button index
            int btn_idx = -1;
            for (size_t i = 0; i < NUM_BUTTONS; ++i) {
                if (BUTTON_MAP[i].pin == evt.pin) {
                    btn_idx = static_cast<int>(i);
                    break;
                }
            }
            if (btn_idx < 0) continue;

            // Debounce threshold: 40ms
            constexpr int64_t DEBOUNCE_US = 40000;
            // Long press threshold: 800ms
            constexpr int64_t LONG_PRESS_US = 800000;

            if (evt.level == 0) { // Active LOW pressed
                if (!is_pressed[btn_idx]) {
                    is_pressed[btn_idx] = true;
                    press_start_us[btn_idx] = evt.timestamp_us;
                }
            } else { // Released
                if (is_pressed[btn_idx]) {
                    int64_t duration = evt.timestamp_us - press_start_us[btn_idx];
                    is_pressed[btn_idx] = false;

                    if (duration >= DEBOUNCE_US) {
                        ButtonEvent b_evt;
                        b_evt.id = BUTTON_MAP[btn_idx].id;
                        b_evt.type = (duration >= LONG_PRESS_US) ? PressType::LONG_PRESS : PressType::SHORT_PRESS;

                        if (self->callback_) {
                            self->callback_(b_evt);
                        }
                    }
                }
            }
        }
    }
    vTaskDelete(nullptr);
}

esp_err_t ButtonManager::init(ButtonCallback callback) {
    callback_ = callback;
    running_ = true;

    s_gpio_evt_queue = xQueueCreate(16, sizeof(RawGpioEvent));
    if (!s_gpio_evt_queue) {
        ESP_LOGE(TAG, "Failed to create button queue");
        return ESP_ERR_NO_MEM;
    }

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    uint64_t pin_mask = 0;
    for (size_t i = 0; i < NUM_BUTTONS; ++i) {
        pin_mask |= (1ULL << BUTTON_MAP[i].pin);
    }
    io_conf.pin_bit_mask = pin_mask;
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "gpio_install_isr_service failed: %s", esp_err_to_name(ret));
        return ret;
    }

    for (size_t i = 0; i < NUM_BUTTONS; ++i) {
        gpio_isr_handler_add(BUTTON_MAP[i].pin, gpio_isr_handler, reinterpret_cast<void*>(BUTTON_MAP[i].pin));
    }

    xTaskCreate(inputTask, "button_task", 3072, this, 10, &taskHandle_);

    ESP_LOGI(TAG, "Button manager initialized with %d buttons", NUM_BUTTONS);
    return ESP_OK;
}
