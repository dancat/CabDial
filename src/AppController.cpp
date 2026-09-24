#include "AppController.h"

AppController::AppController(Device &device, ThrottleUI &throttle,
    LocomotiveSelectionUI &selection, ConnectionUI &connection,
    TurnoutUI &turnouts, RouteUI &routes, PowerUI &power, SettingsUI &settings) :
    device(device), throttle(throttle), selection(selection), connection(connection),
    turnouts(turnouts), routes(routes), power(power), settings(settings) {}

bool AppController::begin(const Device::InputCallbacks &inputCallbacks)
{
    Device::InputCallbacks activeInputs = inputCallbacks;
    const DeviceCapabilities &capabilities = device.capabilities();
    if (!capabilities.encoder)
    {
        activeInputs.encoderDecrease = nullptr;
        activeInputs.encoderIncrease = nullptr;
    }
    if (!capabilities.physicalButton)
    {
        activeInputs.doubleClick = nullptr;
        activeInputs.longPress = nullptr;
    }

    if (!device.begin(activeInputs))
        return false;

    const DisplayProfile &profile = device.displayProfile();
    throttle.setDisplayProfile(profile);
    selection.setDisplayProfile(profile);
    connection.setDisplayProfile(profile);
    turnouts.setDisplayProfile(profile);
    routes.setDisplayProfile(profile);
    power.setDisplayProfile(profile);
    settings.setDisplayProfile(profile);
    return true;
}

void AppController::setBacklight(uint8_t brightness)
{
    if (device.capabilities().adjustableBacklight)
        device.setBacklight(brightness);
}

const DeviceCapabilities &AppController::deviceCapabilities() const
{
    return device.capabilities();
}

void AppController::initializeUi(const UiCallbacks &callbacks)
{
    throttle.begin();
    throttle.setFunctionCallback(callbacks.function);
    throttle.setSelectionCallback(callbacks.selectLocomotive);
    throttle.setConnectionCallback(callbacks.openConnection);
    throttle.setSettingsCallback(callbacks.openSettings);
    throttle.setTurnoutCallback(callbacks.openTurnouts);
    throttle.setPowerCallback(callbacks.openPower);
    throttle.setRouteCallback(callbacks.openRoutes);
    throttle.setDirectionCallback(callbacks.toggleDirection);
    throttle.setStopCallback(callbacks.stop);
    throttle.setEmergencyStopCallback(callbacks.emergencyStop);
    throttle.setFunctionPageCallback(callbacks.functionPage);
    selection.begin(callbacks.rosterSelected, callbacks.closeSelection);
    connection.begin(callbacks.saveConnection, callbacks.closeConnection, callbacks.refreshLists);
    turnouts.begin(callbacks.turnoutSet, callbacks.turnoutFavorite, callbacks.closeTurnouts);
    routes.begin(callbacks.routeStart, callbacks.closeRoutes);
    power.begin(callbacks.setPower, callbacks.closePower);
    settings.begin(callbacks.brightness, callbacks.sleep, callbacks.closeSettings);
}

// Application runtime: DCC state, callbacks, and synchronization.
#include <Arduino.h>
#include <esp_system.h>
#include <lvgl.h>
#include "lvgl_v8_port.h"

#include <WiFi.h>
#include "DccController.h"

#include "ThrottleUI.h"

#include "Locomotive.h"
#include "LocomotiveFunctionStates.h"
#include "LocomotiveSelectionUI.h"
#include "ConnectionUI.h"
#include "ConnectionSettingsStore.h"
#include "TurnoutUI.h"
#include "RouteUI.h"
#include "PowerUI.h"
#include "OperatingPreferences.h"
#include "SettingsUI.h"
#include "DeviceFactory.h"
#include "AppController.h"

bool rosterLoaded = false;

Device &device = getDevice();
DccController dcc;
ConnectionSettingsStore connectionStore;
ConnectionSettings connectionSettings;
bool connectionConfigured = false;
OperatingPreferencesStore operatingPreferencesStore;
OperatingPreferences operatingPreferences;
unsigned long lastDisplayActivity = 0;
bool displaySleeping = false;


// -------------------------------------------------
// Locomotive definition
// -------------------------------------------------
Locomotive locomotive;
LocomotiveFunctionStates savedFunctionStates;
std::vector<Locomotive> rosterLocomotives;
bool rosterAvailable = false;
bool restoredLastRosterDefinition = false;
std::vector<TurnoutDefinition> turnouts;
bool turnoutsAvailable = false;
std::vector<RouteDefinition> routes;
bool routesAvailable = false;

// Called under the LVGL lock, shared by touch and physical input callbacks.
void changeLocomotiveAddress(uint16_t address)
{
    savedFunctionStates.save(locomotive.address, locomotive.functionStates);
    Locomotive manualDefinition;
    manualDefinition.address = address;
    locomotive.applyDefinition(manualDefinition);
    savedFunctionStates.restore(address, locomotive.functionStates);
    operatingPreferences.lastLocomotiveAddress = address;
    operatingPreferencesStore.save(operatingPreferences);
}


// -------------------------------------------------
// Controller mode
// -------------------------------------------------

enum ControllerMode
{
    MODE_SPEED,
    MODE_ADDRESS
};

ControllerMode mode = MODE_SPEED;

ThrottleUI throttleUI;
LocomotiveSelectionUI selectionUI;
ConnectionUI connectionUI;
TurnoutUI turnoutUI;
RouteUI routeUI;
PowerUI powerUI;
SettingsUI settingsUI;
AppController app(device, throttleUI, selectionUI, connectionUI, turnoutUI, routeUI, powerUI, settingsUI);

void applyDisplayBrightness(uint8_t brightness)
{
    app.setBacklight(brightness);
}

void noteDisplayActivity()
{
    lastDisplayActivity = millis();
    if (displaySleeping)
    {
        applyDisplayBrightness(operatingPreferences.displayBrightness);
        displaySleeping = false;
    }
}

void closeDisplaySettings()
{
    settingsUI.hide();
}

void setDisplayBrightness(uint8_t brightness)
{
    operatingPreferences.displayBrightness = brightness;
    applyDisplayBrightness(brightness);
    displaySleeping = false;
    lastDisplayActivity = millis();
    operatingPreferencesStore.save(operatingPreferences);
}

void setDisplaySleepTimeout(uint16_t seconds)
{
    operatingPreferences.displaySleepSeconds = seconds;
    noteDisplayActivity();
    operatingPreferencesStore.save(operatingPreferences);
}

void openDisplaySettings()
{
    noteDisplayActivity();
    settingsUI.show(operatingPreferences.displayBrightness,
        operatingPreferences.displaySleepSeconds);
}

void closeConnectionSettings()
{
    if (connectionConfigured)
        connectionUI.hide();
}

void saveConnectionSettings(const ConnectionSettings &settings)
{
    if (!dcc.connect(settings))
    {
        connectionUI.setStatus("DCC-EX IP address is invalid", true);
        return;
    }
    if (!connectionStore.save(settings))
    {
        connectionUI.setStatus("Unable to save connection settings", true);
        return;
    }

    connectionSettings = settings;
    connectionConfigured = true;
    rosterAvailable = false;
    rosterLoaded = false;
    restoredLastRosterDefinition = false;
    connectionUI.hide();
}

void openConnectionSettings()
{
    noteDisplayActivity();
    connectionUI.show(connectionSettings, !connectionConfigured);
}

void refreshCommandStationLists()
{
    dcc.refreshLists();
    rosterAvailable = false;
    rosterLoaded = false;
    restoredLastRosterDefinition = false;
    turnoutsAvailable = false;
    turnouts.clear();
    routesAvailable = false;
    routes.clear();
    connectionUI.setStatus("Refreshing roster, turnouts, and routes");
}

void closeTurnoutPage()
{
    turnoutUI.hide();
    throttleUI.update(locomotive, mode == MODE_SPEED);
}

void openTurnoutPage()
{
    noteDisplayActivity();
    turnoutUI.show(turnouts, turnoutsAvailable);
}

void closeRoutePage()
{
    routeUI.hide();
    throttleUI.update(locomotive, mode == MODE_SPEED);
}

void openRoutePage()
{
    noteDisplayActivity();
    routeUI.show(routes, routesAvailable);
}

void startRoute(int id)
{
    noteDisplayActivity();
    dcc.startRoute(id);
}

void closePowerPage()
{
    powerUI.hide();
    throttleUI.update(locomotive, mode == MODE_SPEED);
}

void openPowerPage()
{
    noteDisplayActivity();
    powerUI.show();
}

void toggleDirection()
{
    noteDisplayActivity();
    locomotive.speed = 0;
    dcc.setSpeed(locomotive.address, locomotive.speed, locomotive.directionForward);
    locomotive.directionForward = !locomotive.directionForward;
    dcc.setSpeed(locomotive.address, locomotive.speed, locomotive.directionForward);
    throttleUI.update(locomotive, mode == MODE_SPEED);
}

void stopSelectedLoco()
{
    noteDisplayActivity();
    dcc.setSpeed(locomotive.address, 0, locomotive.directionForward);
    locomotive.speed = 0;
    throttleUI.update(locomotive, mode == MODE_SPEED);
}

void emergencyStop()
{
    noteDisplayActivity();
    dcc.emergencyStop();
    locomotive.speed = 0;
    throttleUI.update(locomotive, mode == MODE_SPEED);
}

void setTrackPower(bool on)
{
    dcc.setTrackPower(on);
}

void onTrackPowerBroadcast(bool known, bool on)
{
    powerUI.setTrackPower(known, on);
    throttleUI.setTrackPowerStatus(known, on);
}

void setTurnoutState(int id, bool thrown)
{
    if (!turnoutsAvailable)
        return;

    dcc.setTurnout(id, thrown);
    for (TurnoutDefinition &turnout : turnouts)
    {
        if (turnout.id == id)
        {
            turnout.thrown = thrown;
            break;
        }
    }
    turnoutUI.show(turnouts, turnoutsAvailable);
}

bool isFavoriteTurnout(int id)
{
    for (int favorite : operatingPreferences.favoriteTurnoutIds)
        if (favorite == id)
            return true;
    return false;
}

void toggleTurnoutFavorite(int id)
{
    for (auto favorite = operatingPreferences.favoriteTurnoutIds.begin();
         favorite != operatingPreferences.favoriteTurnoutIds.end(); ++favorite)
    {
        if (*favorite != id)
            continue;
        operatingPreferences.favoriteTurnoutIds.erase(favorite);
        for (TurnoutDefinition &turnout : turnouts)
            if (turnout.id == id)
                turnout.favorite = false;
        operatingPreferencesStore.save(operatingPreferences);
        turnoutUI.show(turnouts, turnoutsAvailable);
        return;
    }

    operatingPreferences.favoriteTurnoutIds.push_back(id);
    for (TurnoutDefinition &turnout : turnouts)
        if (turnout.id == id)
            turnout.favorite = true;
    operatingPreferencesStore.save(operatingPreferences);
    turnoutUI.show(turnouts, turnoutsAvailable);
}

void saveFunctionPage(uint8_t page)
{
    operatingPreferences.functionPage = page;
    operatingPreferencesStore.save(operatingPreferences);
}

void onTurnoutBroadcast(int id, bool thrown)
{
    for (TurnoutDefinition &turnout : turnouts)
    {
        if (turnout.id == id)
        {
            turnout.thrown = thrown;
            if (turnoutUI.isVisible())
                turnoutUI.show(turnouts, turnoutsAvailable);
            return;
        }
    }
}

// Called by DCCEXProtocol for every <l ...> broadcast, including changes made
// by another connected throttle. The main loop already holds the LVGL lock
// while DccController processes protocol input.
void onLocoBroadcast(uint16_t address, uint8_t speed, bool directionForward,
    uint32_t functionMap)
{
    if (address != locomotive.address)
    {
        savedFunctionStates.saveMask(address, functionMap);
        return;
    }

    locomotive.speed = speed;
    locomotive.directionForward = directionForward;
    for (uint8_t function = 0; function < MAX_LOCO_FUNCTIONS; ++function)
        locomotive.functionStates[function] = (functionMap & (uint32_t{1} << function)) != 0;
    savedFunctionStates.save(locomotive.address, locomotive.functionStates);
    throttleUI.update(locomotive, mode == MODE_SPEED);
}

void closeLocomotiveSelection()
{
    selectionUI.hide();
    throttleUI.setLocomotive(locomotive);
    throttleUI.update(locomotive, mode == MODE_SPEED);
}

void selectRosterLocomotive(uint16_t address)
{
    if (address == 0)
    {
        return;
    }

    for (const Locomotive &entry : rosterLocomotives)
    {
        if (entry.address != address)
            continue;
        changeLocomotiveAddress(address);
        locomotive.applyDefinition(entry);
        operatingPreferences.lastLocomotiveAddress = address;
        operatingPreferencesStore.save(operatingPreferences);
        locomotive.speed = 0;
        dcc.selectLoco(address);
        mode = MODE_SPEED;
        closeLocomotiveSelection();
        return;
    }

    // The address came from the manual entry screen rather than the roster.
    changeLocomotiveAddress(address);
    locomotive.speed = 0;
    dcc.selectLoco(address);
    mode = MODE_SPEED;
    closeLocomotiveSelection();
}

void openLocomotiveSelection()
{
    // Match the existing stop-before-select behavior, retaining direction.
    dcc.setSpeed(locomotive.address, 0, locomotive.directionForward);
    locomotive.speed = 0;
    throttleUI.update(locomotive, mode == MODE_SPEED);
    selectionUI.show(rosterLocomotives, rosterAvailable, locomotive.address);
}


// -------------------------------------------------
// Rotary encoder - decrease
// -------------------------------------------------

void onKnobLeftEventCallback(
    int count,
    void *usr_data
)
{
    lvgl_port_lock(-1);
    noteDisplayActivity();
    if (connectionUI.isVisible())
    {
        lvgl_port_unlock();
        return;
    }
    if (selectionUI.isVisible())
    {
        selectionUI.move(-1);
        lvgl_port_unlock();
        return;
    }
    if (turnoutUI.isVisible())
    {
        turnoutUI.move(-1);
        lvgl_port_unlock();
        return;
    }
    if (routeUI.isVisible())
    {
        routeUI.move(-1);
        lvgl_port_unlock();
        return;
    }
    if (powerUI.isVisible())
    {
        lvgl_port_unlock();
        return;
    }
    if (settingsUI.isVisible())
    {
        lvgl_port_unlock();
        return;
    }
    if (mode == MODE_SPEED)
    {
        if (locomotive.speed > 0)
        {
            locomotive.speed--;

            dcc.setSpeed(
                locomotive.address,
                locomotive.speed,
                locomotive.directionForward
            );
        }
    }
    else
    {
        if (locomotive.address > 1)
            changeLocomotiveAddress(locomotive.address - 1);
        throttleUI.setLocomotive(locomotive);
    }

    throttleUI.update(locomotive, mode == MODE_SPEED);
    
    lvgl_port_unlock();


}


// -------------------------------------------------
// Rotary encoder - increase
// -------------------------------------------------

void onKnobRightEventCallback(
    int count,
    void *usr_data
)
{
    lvgl_port_lock(-1);
    noteDisplayActivity();
    if (connectionUI.isVisible())
    {
        lvgl_port_unlock();
        return;
    }
    if (selectionUI.isVisible())
    {
        selectionUI.move(1);
        lvgl_port_unlock();
        return;
    }
    if (turnoutUI.isVisible())
    {
        turnoutUI.move(1);
        lvgl_port_unlock();
        return;
    }
    if (routeUI.isVisible())
    {
        routeUI.move(1);
        lvgl_port_unlock();
        return;
    }
    if (powerUI.isVisible())
    {
        lvgl_port_unlock();
        return;
    }
    if (settingsUI.isVisible())
    {
        lvgl_port_unlock();
        return;
    }
    if (mode == MODE_SPEED)
    {
        if (locomotive.speed < 126)
        {
            locomotive.speed++;

            dcc.setSpeed(
                locomotive.address,
                locomotive.speed,
                locomotive.directionForward
            );
        }
    }
    else
    {
        if (locomotive.address < 9999)
            changeLocomotiveAddress(locomotive.address + 1);
        throttleUI.setLocomotive(locomotive);
    }

    throttleUI.update(locomotive, mode == MODE_SPEED);

    lvgl_port_unlock();


}


// -------------------------------------------------
// Button - double click
// -------------------------------------------------

static void DoubleClickCb(
    void *button_handle,
    void *usr_data
)
{
    lvgl_port_lock(-1);
    noteDisplayActivity();
    if (connectionUI.isVisible())
    {
        lvgl_port_unlock();
        return;
    }
    if (selectionUI.isVisible())
    {
        lvgl_port_unlock();
        return;
    }
    if (turnoutUI.isVisible())
    {
        lvgl_port_unlock();
        return;
    }
    if (routeUI.isVisible())
    {
        lvgl_port_unlock();
        return;
    }
    if (powerUI.isVisible())
    {
        lvgl_port_unlock();
        return;
    }
    if (settingsUI.isVisible())
    {
        lvgl_port_unlock();
        return;
    }
    if (locomotive.fromRoster && locomotive.functionDefinitions[0].momentary)
    {
        // The physical button has no matching release event for a double click.
        // Send a short protocol pulse instead of leaving a momentary function on.
        dcc.setFunction(locomotive.address, 0, true);
        dcc.setFunction(locomotive.address, 0, false);
        locomotive.functionStates[0] = false;
    }
    else
    {
        locomotive.functionStates[0] = !locomotive.functionStates[0];
        dcc.setFunction(locomotive.address, 0, locomotive.functionStates[0]);
    }

    throttleUI.update(locomotive, mode == MODE_SPEED);
    lvgl_port_unlock();
}


// -------------------------------------------------
// Button - long press
// -------------------------------------------------

static void LongPressStartCb(
    void *button_handle,
    void *usr_data
)
{
    lvgl_port_lock(-1);
    noteDisplayActivity();
    if (connectionUI.isVisible())
    {
        closeConnectionSettings();
        lvgl_port_unlock();
        return;
    }
    if (settingsUI.isVisible())
    {
        closeDisplaySettings();
        lvgl_port_unlock();
        return;
    }
    if (selectionUI.isVisible())
    {
        closeLocomotiveSelection();
        lvgl_port_unlock();
        return;
    }
    if (turnoutUI.isVisible())
    {
        closeTurnoutPage();
        lvgl_port_unlock();
        return;
    }
    if (routeUI.isVisible())
    {
        closeRoutePage();
        lvgl_port_unlock();
        return;
    }
    if (powerUI.isVisible())
    {
        closePowerPage();
        lvgl_port_unlock();
        return;
    }
    toggleDirection();
    lvgl_port_unlock();

    Serial.printf(
        "Direction changed to: %s\n",
        locomotive.directionForward
            ? "FORWARD"
            : "REVERSE"
    );
}

void onUIFunction(uint8_t function, bool active)
{
    if (function >= MAX_LOCO_FUNCTIONS)
    {
        return;
    }

    lvgl_port_lock(-1);
    noteDisplayActivity();
    const bool momentary = locomotive.fromRoster &&
        locomotive.functionDefinitions[function].momentary;
    if (momentary)
    {
        locomotive.functionStates[function] = active;
        dcc.setFunction(locomotive.address, function, active);
    }
    else if (active)
    {
        locomotive.functionStates[function] = !locomotive.functionStates[function];
        dcc.setFunction(locomotive.address, function, locomotive.functionStates[function]);
    }

    throttleUI.update(
        locomotive,
        mode == MODE_SPEED
    );
    lvgl_port_unlock();
}

// -------------------------------------------------
// Setup
// -------------------------------------------------

void initializeApplication()
{
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("============================");
    Serial.println("CabDial starting...");
    Serial.printf("ESP reset reason: %d\n", static_cast<int>(esp_reset_reason()));
    Serial.println("============================");


    const Device::InputCallbacks inputCallbacks {
        onKnobLeftEventCallback,
        onKnobRightEventCallback,
        DoubleClickCb,
        LongPressStartCb
    };
    if (!app.begin(inputCallbacks))
    {
        Serial.println("Device initialization failed");
        return;
    }



    const AppController::UiCallbacks uiCallbacks {
        onUIFunction,
        openLocomotiveSelection,
        openConnectionSettings,
        openDisplaySettings,
        openTurnoutPage,
        openPowerPage,
        openRoutePage,
        toggleDirection,
        stopSelectedLoco,
        emergencyStop,
        saveFunctionPage,
        selectRosterLocomotive,
        closeLocomotiveSelection,
        saveConnectionSettings,
        closeConnectionSettings,
        refreshCommandStationLists,
        setTurnoutState,
        toggleTurnoutFavorite,
        closeTurnoutPage,
        startRoute,
        closeRoutePage,
        setTrackPower,
        closePowerPage,
        setDisplayBrightness,
        setDisplaySleepTimeout,
        closeDisplaySettings
    };
    app.initializeUi(uiCallbacks);
    dcc.setLocoUpdateCallback(onLocoBroadcast);
    dcc.setTurnoutUpdateCallback(onTurnoutBroadcast);
    dcc.setTrackPowerCallback(onTrackPowerBroadcast);
    operatingPreferencesStore.load(operatingPreferences);
    applyDisplayBrightness(operatingPreferences.displayBrightness);
    locomotive.address = operatingPreferences.lastLocomotiveAddress;
    throttleUI.setLocomotive(locomotive);
    throttleUI.setFunctionPage(operatingPreferences.functionPage);
    throttleUI.update(locomotive, mode == MODE_SPEED);
    lastDisplayActivity = millis();


    dcc.begin();
    connectionConfigured = connectionStore.load(connectionSettings);
    if (connectionConfigured)
        dcc.connect(connectionSettings);
    else
        connectionUI.show(connectionSettings, true);

    Serial.println();
    Serial.println("============================");
    Serial.println("Hardware initialization OK");
    Serial.println("============================");
}

// -------------------------------------------------
// Main loop
// -------------------------------------------------

void updateApplication()
{
    // Serialize protocol/list updates with the touch and physical callbacks.
    lvgl_port_lock(-1);
    dcc.update();
    throttleUI.setConnectionStatus(
        dcc.connectionStatusText(),
        dcc.connectionStatus() == DccController::ConnectionStatus::Ready
    );
    if (connectionUI.isVisible() && connectionConfigured)
        connectionUI.setStatus(dcc.connectionStatusText(),
            dcc.connectionStatus() == DccController::ConnectionStatus::NotConfigured);

    if (!dcc.rosterReady())
    {
        if (rosterAvailable && selectionUI.isVisible())
            selectionUI.show(rosterLocomotives, false, locomotive.address);
        rosterAvailable = false;
        rosterLoaded = false;
        restoredLastRosterDefinition = false;
    }
    else if (!rosterLoaded)
    {
        rosterAvailable = dcc.copyRoster(rosterLocomotives);
        if (!restoredLastRosterDefinition)
        {
            for (const Locomotive &entry : rosterLocomotives)
            {
                if (entry.address != operatingPreferences.lastLocomotiveAddress)
                    continue;
                locomotive.applyDefinition(entry);
                savedFunctionStates.restore(entry.address, locomotive.functionStates);
                dcc.selectLoco(entry.address);
                throttleUI.setLocomotive(locomotive);
                throttleUI.setFunctionPage(operatingPreferences.functionPage);
                restoredLastRosterDefinition = true;
                break;
            }
        }
        if (selectionUI.isVisible())
            selectionUI.show(rosterLocomotives, rosterAvailable, locomotive.address);

        rosterLoaded = true;
    }

    if (!dcc.turnoutsReady())
    {
        if (turnoutsAvailable && turnoutUI.isVisible())
            turnoutUI.show(turnouts, false);
        turnoutsAvailable = false;
        turnouts.clear();
    }
    else if (!turnoutsAvailable)
    {
        turnoutsAvailable = dcc.copyTurnouts(turnouts);
        for (TurnoutDefinition &turnout : turnouts)
            turnout.favorite = isFavoriteTurnout(turnout.id);
        if (turnoutUI.isVisible())
            turnoutUI.show(turnouts, turnoutsAvailable);
    }

    if (!dcc.routesReady())
    {
        if (routesAvailable && routeUI.isVisible())
            routeUI.show(routes, false);
        routesAvailable = false;
        routes.clear();
    }
    else if (!routesAvailable)
    {
        routesAvailable = dcc.copyRoutes(routes);
        if (routeUI.isVisible())
            routeUI.show(routes, routesAvailable);
    }

    const unsigned long sleepTimeout =
        static_cast<unsigned long>(operatingPreferences.displaySleepSeconds) * 1000UL;
    if (sleepTimeout && !displaySleeping && millis() - lastDisplayActivity >= sleepTimeout)
    {
        applyDisplayBrightness(0);
        displaySleeping = true;
    }

    lvgl_port_unlock();
    delay(100);
}
