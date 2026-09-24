#pragma once

#include <driver/gpio.h>

namespace viewe
{
constexpr int DISPLAY_WIDTH = 480;
constexpr int DISPLAY_HEIGHT = 480;
constexpr bool DISPLAY_IS_ROUND = true;
constexpr uint16_t SAFE_TOP = 24;
constexpr uint16_t SAFE_BOTTOM = 40;
constexpr int KNOB_PIN_A = 6;
constexpr int KNOB_PIN_B = 5;
constexpr gpio_num_t BUTTON_PIN = GPIO_NUM_0;
}
