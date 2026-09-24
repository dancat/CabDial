#pragma once

#include <lvgl.h>
#include "Device.h"
#include "OperatingPreferences.h"

class SettingsUI
{
public:
    using BrightnessCallback = void (*)(uint8_t brightness);
    using SleepCallback = void (*)(uint16_t seconds);
    using BackCallback = void (*)();
    using ShortcutCallback = void (*)(uint8_t press, HomeShortcutAction action);

    void begin(BrightnessCallback brightness, SleepCallback sleep, BackCallback back, ShortcutCallback shortcut);
    void setDisplayProfile(const DisplayProfile &value) { profile = value; }
    void show(uint8_t brightness, uint16_t sleepSeconds, HomeShortcutAction single, HomeShortcutAction dbl, HomeShortcutAction lng);
    void hide();
    bool isVisible() const { return visible; }

private:
    DisplayProfile profile {480, 480, true, 0, 0};
    lv_obj_t *screen = nullptr;
    lv_obj_t *returnScreen = nullptr;
    lv_obj_t *brightnessSlider = nullptr;
    lv_obj_t *brightnessValue = nullptr;
    lv_obj_t *sleepRoller = nullptr;
    lv_obj_t *shortcutScreen = nullptr;
    lv_obj_t *shortcutSelectors[3] = {};
    HomeShortcutAction shortcuts[3] = {};
    bool visible = false;
    BrightnessCallback brightnessCallback = nullptr;
    SleepCallback sleepCallback = nullptr;
    BackCallback backCallback = nullptr;
    ShortcutCallback shortcutCallback = nullptr;

    void updateBrightnessValue(uint8_t brightness);
    static void brightnessEvent(lv_event_t *event);
    static void sleepEvent(lv_event_t *event);
    static void backEvent(lv_event_t *event);
    static void shortcutEvent(lv_event_t *event);
    static void openShortcutsEvent(lv_event_t *event);
};
