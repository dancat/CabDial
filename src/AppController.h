#pragma once

#include "Device.h"
#include "ThrottleUI.h"
#include "LocomotiveSelectionUI.h"
#include "ConnectionUI.h"
#include "TurnoutUI.h"
#include "RouteUI.h"
#include "PowerUI.h"
#include "SettingsUI.h"

// Owns the application-to-device boundary. Business logic remains independent
// of the board implementation selected by DeviceFactory.
class AppController
{
public:
    // Application behavior belongs to the caller; page construction and
    // inter-page wiring belong here so a new device does not duplicate it.
    struct UiCallbacks
    {
        ThrottleUI::FunctionCallback function = nullptr;
        ThrottleUI::SelectionCallback selectLocomotive = nullptr;
        ThrottleUI::ConnectionCallback openConnection = nullptr;
        ThrottleUI::SettingsCallback openSettings = nullptr;
        ThrottleUI::TurnoutCallback openTurnouts = nullptr;
        ThrottleUI::PowerCallback openPower = nullptr;
        ThrottleUI::RouteCallback openRoutes = nullptr;
        ThrottleUI::DirectionCallback toggleDirection = nullptr;
        ThrottleUI::StopCallback stop = nullptr;
        ThrottleUI::EmergencyStopCallback emergencyStop = nullptr;
        ThrottleUI::FunctionPageCallback functionPage = nullptr;
        LocomotiveSelectionUI::SelectCallback rosterSelected = nullptr;
        LocomotiveSelectionUI::BackCallback closeSelection = nullptr;
        ConnectionUI::SaveCallback saveConnection = nullptr;
        ConnectionUI::BackCallback closeConnection = nullptr;
        ConnectionUI::RefreshCallback refreshLists = nullptr;
        TurnoutUI::SetCallback turnoutSet = nullptr;
        TurnoutUI::FavoriteCallback turnoutFavorite = nullptr;
        TurnoutUI::BackCallback closeTurnouts = nullptr;
        RouteUI::StartCallback routeStart = nullptr;
        RouteUI::BackCallback closeRoutes = nullptr;
        PowerUI::PowerCallback setPower = nullptr;
        PowerUI::BackCallback closePower = nullptr;
        SettingsUI::BrightnessCallback brightness = nullptr;
        SettingsUI::SleepCallback sleep = nullptr;
        SettingsUI::BackCallback closeSettings = nullptr;
    };

    AppController(Device &device, ThrottleUI &throttle, LocomotiveSelectionUI &selection,
        ConnectionUI &connection, TurnoutUI &turnouts, RouteUI &routes,
        PowerUI &power, SettingsUI &settings);

    bool begin(const Device::InputCallbacks &inputCallbacks);
    void initializeUi(const UiCallbacks &callbacks);
    void setBacklight(uint8_t brightness);
    const DeviceCapabilities &deviceCapabilities() const;

private:
    Device &device;
    ThrottleUI &throttle;
    LocomotiveSelectionUI &selection;
    ConnectionUI &connection;
    TurnoutUI &turnouts;
    RouteUI &routes;
    PowerUI &power;
    SettingsUI &settings;
};
