#pragma once

#include <lvgl.h>
#include "Device.h"
#include "Locomotive.h"

class FunctionUI
{
public:
    using FunctionCallback = void (*)(uint8_t, bool);
    using BackCallback = void (*)();
    void begin(FunctionCallback function, BackCallback back);
    void setDisplayProfile(const DisplayProfile &value) { profile = value; }
    void show(const Locomotive &locomotive);
    void hide();
    bool isVisible() const { return visible; }

private:
    static constexpr uint8_t MAX_BUTTONS = 32;
    DisplayProfile profile {480, 480, true, 0, 0};
    lv_obj_t *screen = nullptr;
    lv_obj_t *returnScreen = nullptr;
    lv_obj_t *buttons[MAX_BUTTONS] = {};
    lv_obj_t *labels[MAX_BUTTONS] = {};
    uint8_t functions[MAX_BUTTONS] = {};
    uint8_t count = 0;
    const Locomotive *locomotive = nullptr;
    bool visible = false;
    FunctionCallback functionCallback = nullptr;
    BackCallback backCallback = nullptr;
    static void functionEvent(lv_event_t *event);
    static void backEvent(lv_event_t *event);
    void updateAppearance();
};
