#pragma once

#include <lvgl.h>
#include "Device.h"
#include "Locomotive.h"

class ThrottleUI
{
public:
    void begin();
    void setDisplayProfile(const DisplayProfile &value) { profile = value; }
    void update(const Locomotive &locomotive, bool speedMode);

    // active is true on press and false on release for momentary functions.
    // For latching functions, active is true for a completed tap.
    using FunctionCallback = void (*)(uint8_t function, bool active);
    void setFunctionCallback(FunctionCallback callback);
    void setLocomotive(const Locomotive &locomotive);
    using SelectionCallback = void (*)();
    void setSelectionCallback(SelectionCallback callback) { selectionCallback = callback; }
    using ConnectionCallback = void (*)();
    void setConnectionCallback(ConnectionCallback callback) { connectionCallback = callback; }
    using SettingsCallback = void (*)();
    void setSettingsCallback(SettingsCallback callback) { settingsCallback = callback; }
    using TurnoutCallback = void (*)();
    void setTurnoutCallback(TurnoutCallback callback) { turnoutCallback = callback; }
    using PowerCallback = void (*)();
    void setPowerCallback(PowerCallback callback) { powerCallback = callback; }
    using RouteCallback = void (*)();
    void setRouteCallback(RouteCallback callback) { routeCallback = callback; }
    using DirectionCallback = void (*)();
    void setDirectionCallback(DirectionCallback callback) { directionCallback = callback; }
    using StopCallback = void (*)();
    void setStopCallback(StopCallback callback) { stopCallback = callback; }
    using EmergencyStopCallback = void (*)();
    void setEmergencyStopCallback(EmergencyStopCallback callback) { emergencyStopCallback = callback; }
    using SpeedPresetCallback = void (*)(uint8_t speed);
    void setSpeedPresetCallback(SpeedPresetCallback callback) { speedPresetCallback = callback; }
    using FunctionPageCallback = void (*)(uint8_t page);
    void setFunctionPageCallback(FunctionPageCallback callback) { functionPageCallback = callback; }
    void setFunctionPage(uint8_t page);
    using MoreFunctionsCallback = void (*)();
    void setMoreFunctionsCallback(MoreFunctionsCallback callback) { moreFunctionsCallback = callback; }
    void setConnectionStatus(const char *text, bool connected);
    void setTrackPowerStatus(bool known, bool on);

private:
    DisplayProfile profile {480, 480, true, 0, 0};
    lv_obj_t *screen = nullptr;

    // Main throttle information
    lv_obj_t *modeLabel = nullptr;
    lv_obj_t *valueLabel = nullptr;
    lv_obj_t *rangeLabel = nullptr;
    lv_obj_t *speedArc = nullptr;
    lv_obj_t *stopButton = nullptr;
    lv_obj_t *emergencyStopButton = nullptr;
    lv_obj_t *speedPreset50Button = nullptr;
    lv_obj_t *speedPreset75Button = nullptr;

    // Locomotive
    lv_obj_t *addressTitleLabel = nullptr;
    lv_obj_t *addressLabel = nullptr;
    lv_obj_t *connectionButton = nullptr;
    lv_obj_t *connectionLabel = nullptr;
    lv_obj_t *connectionIndicator = nullptr;
    lv_obj_t *settingsButton = nullptr;
    lv_obj_t *turnoutButton = nullptr;
    lv_obj_t *powerButton = nullptr;
    lv_obj_t *powerLabel = nullptr;
    lv_obj_t *routeButton = nullptr;

    // Direction
    lv_obj_t *directionArrowLabel = nullptr;
    lv_obj_t *directionButton = nullptr;

    // Function controls
    static constexpr uint8_t FUNCTION_SLOT_COUNT = 3;
    static constexpr uint8_t MANUAL_FUNCTION_COUNT = 5;
    struct FunctionSlot
    {
        lv_obj_t *button = nullptr;
        lv_obj_t *icon = nullptr;
        lv_obj_t *label = nullptr;
        uint8_t function = 0;
        bool assigned = false;
    };

    FunctionSlot functionSlots[FUNCTION_SLOT_COUNT];
    lv_obj_t *previousPageButton = nullptr;
    lv_obj_t *nextPageButton = nullptr;
    lv_obj_t *moreFunctionsButton = nullptr;
    lv_obj_t *pageLabel = nullptr;
    uint8_t functionPage = 0;
    uint8_t availableFunctionCount = 0;
    uint8_t availableFunctions[MAX_LOCO_FUNCTIONS] = {};
    const Locomotive *displayedLocomotive = nullptr;
    uint16_t displayedAddress = 0;

    // Future navigation
    lv_obj_t *navLabel = nullptr;

    FunctionCallback functionCallback = nullptr;
    SelectionCallback selectionCallback = nullptr;
    ConnectionCallback connectionCallback = nullptr;
    SettingsCallback settingsCallback = nullptr;
    TurnoutCallback turnoutCallback = nullptr;
    PowerCallback powerCallback = nullptr;
    RouteCallback routeCallback = nullptr;
    DirectionCallback directionCallback = nullptr;
    StopCallback stopCallback = nullptr;
    EmergencyStopCallback emergencyStopCallback = nullptr;
    SpeedPresetCallback speedPresetCallback = nullptr;
    FunctionPageCallback functionPageCallback = nullptr;
    MoreFunctionsCallback moreFunctionsCallback = nullptr;

    static void functionButtonEvent(lv_event_t *event);
    static void functionGestureEvent(lv_event_t *event);
    static void previousPageEvent(lv_event_t *event);
    static void nextPageEvent(lv_event_t *event);
    static void selectionButtonEvent(lv_event_t *event);
    static void connectionButtonEvent(lv_event_t *event);
    static void settingsButtonEvent(lv_event_t *event);
    static void turnoutButtonEvent(lv_event_t *event);
    static void powerButtonEvent(lv_event_t *event);
    static void routeButtonEvent(lv_event_t *event);
    static void directionButtonEvent(lv_event_t *event);
    static void stopButtonEvent(lv_event_t *event);
    static void emergencyStopButtonEvent(lv_event_t *event);
    static void speedPresetButtonEvent(lv_event_t *event);
    static void moreFunctionsEvent(lv_event_t *event);
    void updateFunctionSlots(const Locomotive &locomotive);
    void updateFunctionAppearance(const Locomotive &locomotive);
};
