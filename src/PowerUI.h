#pragma once

#include <lvgl.h>
#include "Device.h"

class PowerUI
{
public:
    using PowerCallback = void (*)(bool on);
    using BackCallback = void (*)();

    void begin(PowerCallback setPower, BackCallback back);
    void setDisplayProfile(const DisplayProfile &value) { profile = value; }
    void show();
    void hide();
    bool isVisible() const { return visible; }
    void setTrackPower(bool known, bool on);

private:
    DisplayProfile profile {480, 480, true, 0, 0};
    lv_obj_t *screen = nullptr;
    lv_obj_t *returnScreen = nullptr;
    lv_obj_t *powerStatusLabel = nullptr;
    lv_obj_t *powerButton = nullptr;
    lv_obj_t *powerButtonLabel = nullptr;
    bool visible = false;
    bool powerKnown = false;
    bool powerOn = false;
    bool confirmingPowerOff = false;
    PowerCallback powerCallback = nullptr;
    BackCallback backCallback = nullptr;

    void updatePowerControls();
    void changePower();
    static void powerEvent(lv_event_t *event);
    static void backEvent(lv_event_t *event);
};
