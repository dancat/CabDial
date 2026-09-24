#pragma once

#include <lvgl.h>
#include "Device.h"

class SettingsUI
{
public:
    using BrightnessCallback = void (*)(uint8_t brightness);
    using SleepCallback = void (*)(uint16_t seconds);
    using BackCallback = void (*)();

    void begin(BrightnessCallback brightness, SleepCallback sleep, BackCallback back);
    void setDisplayProfile(const DisplayProfile &value) { profile = value; }
    void show(uint8_t brightness, uint16_t sleepSeconds);
    void hide();
    bool isVisible() const { return visible; }

private:
    DisplayProfile profile {480, 480, true, 0, 0};
    lv_obj_t *screen = nullptr;
    lv_obj_t *returnScreen = nullptr;
    lv_obj_t *brightnessSlider = nullptr;
    lv_obj_t *brightnessValue = nullptr;
    lv_obj_t *sleepRoller = nullptr;
    bool visible = false;
    BrightnessCallback brightnessCallback = nullptr;
    SleepCallback sleepCallback = nullptr;
    BackCallback backCallback = nullptr;

    void updateBrightnessValue(uint8_t brightness);
    static void brightnessEvent(lv_event_t *event);
    static void sleepEvent(lv_event_t *event);
    static void backEvent(lv_event_t *event);
};
