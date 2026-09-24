#pragma once

#include <lvgl.h>
#include <vector>
#include "Locomotive.h"
#include "Device.h"

class LocomotiveSelectionUI
{
public:
    // Address zero represents the manual-address option.
    using SelectCallback = void (*)(uint16_t address);
    using BackCallback = void (*)();
    using RefreshCallback = void (*)();

    void begin(SelectCallback select, BackCallback back, RefreshCallback refresh);
    void setDisplayProfile(const DisplayProfile &value) { profile = value; }
    void show(const std::vector<Locomotive> &roster, bool ready, uint16_t currentAddress);
    void move(int delta);
    void select();
    void hide();
    bool isVisible() const { return visible; }

private:
    DisplayProfile profile {480, 480, true, 0, 0};
    lv_obj_t *screen = nullptr;
    lv_obj_t *returnScreen = nullptr;
    lv_obj_t *roller = nullptr;
    lv_obj_t *status = nullptr;
    lv_obj_t *listBackButton = nullptr;
    lv_obj_t *listSelectButton = nullptr;
    lv_obj_t *refreshButton = nullptr;
    lv_obj_t *manualTitle = nullptr;
    lv_obj_t *manualAddressLabel = nullptr;
    lv_obj_t *manualDecreaseButton = nullptr;
    lv_obj_t *manualIncreaseButton = nullptr;
    lv_obj_t *manualBackButton = nullptr;
    lv_obj_t *manualSelectButton = nullptr;
    bool visible = false;
    bool manualMode = false;
    uint16_t manualAddress = 1;
    std::vector<uint16_t> addresses;
    SelectCallback selectCallback = nullptr;
    BackCallback backCallback = nullptr;
    RefreshCallback refreshCallback = nullptr;

    static void selectEvent(lv_event_t *event);
    static void backEvent(lv_event_t *event);
    static void refreshEvent(lv_event_t *event);
    static void manualDecreaseEvent(lv_event_t *event);
    static void manualIncreaseEvent(lv_event_t *event);
    void showManual(uint16_t address);
    void updateManualAddress();
    void changeManualAddress(int delta);
};
