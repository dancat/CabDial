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
    using ReleaseCallback = void (*)();

    void begin(SelectCallback select, BackCallback back, RefreshCallback refresh,
               ReleaseCallback release);
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
    lv_obj_t *selectedLabel = nullptr;
    lv_obj_t *status = nullptr;
    lv_obj_t *listBackButton = nullptr;
    lv_obj_t *listReleaseButton = nullptr;
    lv_obj_t *listSelectButton = nullptr;
    lv_obj_t *refreshButton = nullptr;
    lv_obj_t *manualTitle = nullptr;
    static constexpr uint8_t MANUAL_DIGIT_COUNT = 5;
    static constexpr uint8_t MANUAL_ACTION_COUNT = 2;
    lv_obj_t *manualDigitLabels[MANUAL_DIGIT_COUNT] = {};
    lv_obj_t *manualDecreaseButtons[MANUAL_DIGIT_COUNT] = {};
    lv_obj_t *manualIncreaseButtons[MANUAL_DIGIT_COUNT] = {};
    lv_obj_t *manualBackButton = nullptr;
    lv_obj_t *manualSelectButton = nullptr;
    bool visible = false;
    bool manualMode = false;
    uint16_t manualAddress = 1;
    uint8_t manualDigits[MANUAL_DIGIT_COUNT] = {};
    uint8_t activeManualDigit = MANUAL_DIGIT_COUNT - 1;
    uint8_t manualFocus = 0;
    bool editingManualDigit = false;
    int8_t lastManualFocusDirection = 0;
    unsigned long lastManualFocusMoveAt = 0;
    std::vector<uint16_t> addresses;
    std::vector<String> optionLabels;
    SelectCallback selectCallback = nullptr;
    BackCallback backCallback = nullptr;
    RefreshCallback refreshCallback = nullptr;
    ReleaseCallback releaseCallback = nullptr;

    static void selectEvent(lv_event_t *event);
    static void backEvent(lv_event_t *event);
    static void refreshEvent(lv_event_t *event);
    static void manualDecreaseEvent(lv_event_t *event);
    static void manualIncreaseEvent(lv_event_t *event);
    static void manualDigitEvent(lv_event_t *event);
    static void manualReleaseEvent(lv_event_t *event);
    static void manualSelectEvent(lv_event_t *event);
    void showManual(uint16_t address);
    void setManualControlsVisible(bool visible);
    void updateManualAddress();
    void updateManualFocus();
    void returnToRoster();
    void selectManualAddress();
    void changeManualDigit(uint8_t digit, int delta);
    void updateSelectedLabel();
    static void rollerEvent(lv_event_t *event);
};
