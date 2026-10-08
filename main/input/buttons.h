#pragma once

#include <cstdint>
#include <functional>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

enum class ButtonId {
    PLAY_PAUSE,
    PREVIOUS,
    NEXT,
    VOL_DOWN,
    VOL_UP
};

enum class PressType {
    SHORT_PRESS,
    LONG_PRESS
};

struct ButtonEvent {
    ButtonId id;
    PressType type;
};

using ButtonCallback = std::function<void(const ButtonEvent&)>;

class ButtonManager {
public:
    ButtonManager() = default;
    ~ButtonManager();

    esp_err_t init(ButtonCallback callback);

private:
    static void inputTask(void* arg);

    ButtonCallback callback_{nullptr};
    QueueHandle_t eventQueue_{nullptr};
    TaskHandle_t taskHandle_{nullptr};
    bool running_{false};
};
