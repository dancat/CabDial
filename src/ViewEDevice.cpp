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

    knob = new ESP_Knob(viewe::KNOB_PIN_A, viewe::KNOB_PIN_B);
    knob->begin();
    knob->attachLeftEventCallback(callbacks.encoderIncrease);
    knob->attachRightEventCallback(callbacks.encoderDecrease);

    button = new Button(viewe::BUTTON_PIN, false);
    button->attachSingleClickEventCb(callbacks.singleClick, nullptr);
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
