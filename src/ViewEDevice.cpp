#include "ViewEDevice.h"

#include <ESP_Knob.h>
#include <Button.h>
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

    profile.width = board->getLCD()->getFrameWidth();
    profile.height = board->getLCD()->getFrameHeight();

    knob = new ESP_Knob(viewe::KNOB_PIN_A, viewe::KNOB_PIN_B);
    knob->begin();
    knob->attachLeftEventCallback(callbacks.encoderIncrease);
    knob->attachRightEventCallback(callbacks.encoderDecrease);

    button = new Button(viewe::BUTTON_PIN, false);
    button->attachDoubleClickEventCb(callbacks.doubleClick, nullptr);
    button->attachLongPressStartEventCb(callbacks.longPress, nullptr);

    Serial.println("ViewE hardware initialized.");
    return true;
}

void ViewEDevice::setBacklight(uint8_t brightness)
{
    if (board && board->getBacklight())
        board->getBacklight()->setBrightness(brightness);
}
