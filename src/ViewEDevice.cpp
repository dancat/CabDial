#include "ViewEDevice.h"

#include <Button.h>
#include <driver/gpio.h>
#include <driver/pulse_cnt.h>
#include <esp_timer.h>
#include <esp_display_panel.hpp>
#include "lvgl_v8_port.h"

bool ViewEDevice::begin(const InputCallbacks &callbacks)
{
    using namespace esp_panel::drivers;
    using namespace esp_panel::board;

    Serial.println("Initializing ViewE display...");
    board = new Board();
    board->init();

#if LVGL_PORT_AVOID_TEARING_MODE
    auto *lcd = board->getLCD();
    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);

#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto *lcdBus = lcd->getBus();
    if (lcdBus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB)
    {
        static_cast<BusRGB *>(lcdBus)->configRGB_BounceBufferSize(
            lcd->getFrameWidth() * 10);
    }
#endif
#endif

    if (!board->begin())
        return false;
    if (!lvgl_port_init(board->getLCD(), board->getTouch()))
        return false;

#if LVGL_PORT_ROTATION_DEGREE == 180
    // ESP32_Display_Panel owns the linked LVGL port for this board. Apply the
    // rotation after that port has registered its display driver so the LCD
    // transformation and the touch transformation use the same orientation.
    lv_disp_set_rotation(lv_disp_get_default(), LV_DISP_ROT_180);

    // The bundled port does not apply the build-time rotation to this board's
    // touch controller. Mirror both touch axes to match the rotated display.
    auto *touch = board->getTouch();
    if (touch != nullptr) {
        auto &touchTransformation = touch->getTransformation();
        touch->mirrorX(!touchTransformation.mirror_x);
        touch->mirrorY(!touchTransformation.mirror_y);
    }
#endif

    profile.width = board->getLCD()->getFrameWidth();
    profile.height = board->getLCD()->getFrameHeight();

    if (!beginEncoder(callbacks))
        return false;

    button = new Button(viewe::BUTTON_PIN, false);
    button->attachSingleClickEventCb(callbacks.singleClick, nullptr);
    button->attachDoubleClickEventCb(callbacks.doubleClick, nullptr);
    button->attachLongPressStartEventCb(callbacks.longPress, nullptr);

    Serial.println("ViewE hardware initialized.");
    return true;
}

bool ViewEDevice::beginEncoder(const InputCallbacks &callbacks)
{
    encoderCallbacks = callbacks;
    gpio_set_pull_mode(static_cast<gpio_num_t>(viewe::KNOB_PIN_A), GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(static_cast<gpio_num_t>(viewe::KNOB_PIN_B), GPIO_PULLUP_ONLY);

    const pcnt_unit_config_t unitConfig = {
        .low_limit = -1000,
        .high_limit = 1000,
        .intr_priority = 0,
        .flags = {.accum_count = 0},
    };
    if (pcnt_new_unit(&unitConfig, &encoderUnit) != ESP_OK)
        return false;
    const pcnt_chan_config_t channelConfig = {
        .edge_gpio_num = viewe::KNOB_PIN_A,
        .level_gpio_num = viewe::KNOB_PIN_B,
    };
    pcnt_channel_handle_t channel;
    if (pcnt_new_channel(encoderUnit, &channelConfig, &channel) != ESP_OK)
        return false;
    // Count one edge of A per encoder detent; B selects the direction.
    pcnt_channel_set_edge_action(channel, PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_HOLD);
    pcnt_channel_set_level_action(channel, PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    const pcnt_glitch_filter_config_t filter = {.max_glitch_ns = 10000};
    pcnt_unit_set_glitch_filter(encoderUnit, &filter);
    if (pcnt_unit_enable(encoderUnit) != ESP_OK || pcnt_unit_clear_count(encoderUnit) != ESP_OK ||
        pcnt_unit_start(encoderUnit) != ESP_OK)
        return false;
    const esp_timer_create_args_t timerArgs = {.callback = &ViewEDevice::pollEncoder,
        .arg = this, .name = "encoder"};
    if (esp_timer_create(&timerArgs, &encoderTimer) != ESP_OK ||
        esp_timer_start_periodic(encoderTimer, 2000) != ESP_OK)
        return false;
    return true;
}

void ViewEDevice::pollEncoder(void *arg)
{
    auto *device = static_cast<ViewEDevice *>(arg);
    int count = 0;
    if (pcnt_unit_get_count(device->encoderUnit, &count) != ESP_OK)
        return;
    while (device->encoderCount < count)
    {
        ++device->encoderCount;
        if (device->encoderCallbacks.encoderIncrease)
            device->encoderCallbacks.encoderIncrease(count, nullptr);
    }
    while (device->encoderCount > count)
    {
        --device->encoderCount;
        if (device->encoderCallbacks.encoderDecrease)
            device->encoderCallbacks.encoderDecrease(count, nullptr);
    }
}

void ViewEDevice::setBacklight(uint8_t brightness)
{
    if (board && board->getBacklight())
        board->getBacklight()->setBrightness(brightness);
}
