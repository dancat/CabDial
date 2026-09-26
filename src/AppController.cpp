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
        activeInputs.singleClick = nullptr;
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
    throttle.setSpeedPresetCallback(callbacks.speedPreset);
    throttle.setFunctionPageCallback(callbacks.functionPage);
    selection.begin(callbacks.rosterSelected, callbacks.closeSelection, callbacks.refreshRoster,
                    callbacks.releaseLocomotive);
    connection.begin(callbacks.saveConnection, callbacks.closeConnection, callbacks.refreshLists,
                     callbacks.diagnostics, callbacks.connectWifi,
                     callbacks.discoverCommandStations, callbacks.connectServer,
                     callbacks.disconnect);
    turnouts.begin(callbacks.turnoutSet, callbacks.turnoutFavorite, callbacks.closeTurnouts,
                  callbacks.refreshTurnouts);
    routes.begin(callbacks.routeStart, callbacks.closeRoutes);
    power.begin(callbacks.setPower, callbacks.closePower);
    settings.begin(callbacks.brightness, callbacks.sleep, callbacks.closeSettings, callbacks.shortcut);
}

// Application runtime: DCC state, callbacks, and synchronization.
#include <Arduino.h>
#include <esp_system.h>
#include <lvgl.h>
#include "lvgl_v8_port.h"

#include <WiFi.h>
#include "DccController.h"

#include "ThrottleUI.h"
#include "FunctionUI.h"

#include "Locomotive.h"
#include "LocomotiveFunctionStates.h"
#include "LocomotiveSelectionUI.h"
#include "ConnectionUI.h"
#include "ConnectionDiagnosticsUI.h"
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
String pendingWifiSsid;
String pendingWifiPassword;
OperatingPreferencesStore operatingPreferencesStore;
OperatingPreferences operatingPreferences;
unsigned long lastDisplayActivity = 0;
bool displaySleeping = false;
String displayedConnectionStatus;
bool displayedConnectionReady = false;
String displayedConnectionUiStatus;
bool connectionUiWasVisible = false;

// The hardware encoder callback runs outside the application loop. Keep it
// short and consume queued turns from the normal application loop.
portMUX_TYPE encoderTurnMux = portMUX_INITIALIZER_UNLOCKED;
int32_t pendingEncoderTurns = 0;
static constexpr int32_t MAX_PENDING_ENCODER_TURNS = 128;
// Apply one turn per loop so LVGL can repaint between turns instead of
// displaying a delayed multi-step jump after a queued batch.
static constexpr uint8_t MAX_ENCODER_TURNS_PER_UPDATE = 1;


// -------------------------------------------------
// Locomotive definition
// -------------------------------------------------
Locomotive locomotive;
LocomotiveFunctionStates savedFunctionStates;
std::vector<Locomotive> rosterLocomotives;
bool rosterAvailable = false;
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
FunctionUI functionUI;
LocomotiveSelectionUI selectionUI;
ConnectionUI connectionUI;
ConnectionDiagnosticsUI diagnosticsUI;
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

void closeFunctionPage()
{
    functionUI.hide();
    throttleUI.update(locomotive, mode == MODE_SPEED);
}

void openFunctionPage()
{
    noteDisplayActivity();
    functionUI.show(locomotive);
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
        operatingPreferences.displaySleepSeconds, operatingPreferences.singlePressAction,
        operatingPreferences.doublePressAction, operatingPreferences.longPressAction);
}

void setHomeShortcut(uint8_t press, HomeShortcutAction action)
{
    if (press == 0)
        operatingPreferences.singlePressAction = action;
    else if (press == 1)
        operatingPreferences.doublePressAction = action;
    else if (press == 2)
        operatingPreferences.longPressAction = action;
    operatingPreferencesStore.save(operatingPreferences);
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
    connectionUI.hide();
}

void connectWifi(const String &ssid, const String &password)
{
    if (!dcc.connectWifi(ssid, password))
    {
        connectionUI.setStatus("Choose or enter a Wi-Fi network", true);
        return;
    }
    pendingWifiSsid = ssid;
    pendingWifiPassword = password;
    connectionUI.setStatus("Connecting to Wi-Fi...");
}

bool discoverCommandStations(std::vector<CommandStationInfo> &stations)
{
    return dcc.discoverCommandStations(stations);
}

void connectServer(const String &address, uint16_t port)
{
    if (!dcc.connectServer(address, port))
    {
        connectionUI.setStatus("DCC-EX address is invalid", true);
        return;
    }

    ConnectionSettings settings;
    settings.wifiSsid = pendingWifiSsid;
    settings.wifiPassword = pendingWifiPassword;
    settings.serverAddress = address;
    settings.serverPort = port;
    if (!settings.isComplete() || !connectionStore.save(settings))
    {
        connectionUI.setStatus("Unable to save connection settings", true);
        return;
    }

    connectionSettings = settings;
    connectionConfigured = true;
    rosterAvailable = false;
    rosterLoaded = false;
    connectionUI.setStatus("Connecting to DCC-EX...");
    connectionUI.hide();
}

void disconnectDcc()
{
    dcc.disconnect();
    rosterAvailable = false;
    rosterLoaded = false;
    turnoutsAvailable = false;
    routesAvailable = false;
    connectionUI.hide();
}

void openConnectionSettings()
{
    noteDisplayActivity();
    if (dcc.connected())
        connectionUI.showConnectedDetails(connectionSettings);
    else
        connectionUI.show(connectionSettings, !connectionConfigured);
}
void openDiagnostics() { diagnosticsUI.show(); }
void closeDiagnostics() { diagnosticsUI.hide(); }

void refreshCommandStationLists()
{
    dcc.refreshLists();
    rosterAvailable = false;
    rosterLoaded = false;
    turnoutsAvailable = false;
    turnouts.clear();
    routesAvailable = false;
    routes.clear();
    connectionUI.setStatus("Refreshing roster, turnouts, and routes");
}

void refreshRosterList()
{
    dcc.refreshRoster();
    rosterAvailable = false;
    rosterLoaded = false;
}

void refreshTurnoutList()
{
    dcc.refreshTurnouts();
    turnoutsAvailable = false;
    turnouts.clear();
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
    if (locomotive.address == 0)
        return;
    locomotive.speed = 0;
    dcc.setSpeed(locomotive.address, locomotive.speed, locomotive.directionForward);
    locomotive.directionForward = !locomotive.directionForward;
    dcc.setSpeed(locomotive.address, locomotive.speed, locomotive.directionForward);
    throttleUI.update(locomotive, mode == MODE_SPEED);
}

void stopSelectedLoco()
{
    noteDisplayActivity();
    if (locomotive.address == 0)
        return;
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

void setSpeedPreset(uint8_t speed)
{
    noteDisplayActivity();
    if (locomotive.address == 0)
        return;
    locomotive.speed = speed > 126 ? 126 : speed;
    dcc.setSpeed(locomotive.address, locomotive.speed, locomotive.directionForward);
    throttleUI.update(locomotive, mode == MODE_SPEED);
}

void setTrackPower(bool on)
{
    dcc.setTrackPower(on);
}

void onTrackPowerBroadcast(bool known, bool on)
{
    lvgl_port_lock(-1);
    powerUI.setTrackPower(known, on);
    throttleUI.setTrackPowerStatus(known, on);
    lvgl_port_unlock();
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
    lvgl_port_lock(-1);
    for (TurnoutDefinition &turnout : turnouts)
    {
        if (turnout.id == id)
        {
            turnout.thrown = thrown;
            if (turnoutUI.isVisible())
                turnoutUI.show(turnouts, turnoutsAvailable);
            lvgl_port_unlock();
            return;
        }
    }
    lvgl_port_unlock();
}

// Called by DCCEXProtocol for every <l ...> broadcast, including changes made
// by another connected throttle.
void onLocoBroadcast(uint16_t address, uint8_t speed, bool directionForward,
    uint32_t functionMap)
{
    lvgl_port_lock(-1);
    if (address != locomotive.address)
    {
        savedFunctionStates.saveMask(address, functionMap);
        lvgl_port_unlock();
        return;
    }

    locomotive.speed = speed;
    locomotive.directionForward = directionForward;
    for (uint8_t function = 0; function < MAX_LOCO_FUNCTIONS; ++function)
        locomotive.functionStates[function] = (functionMap & (uint32_t{1} << function)) != 0;
    savedFunctionStates.save(locomotive.address, locomotive.functionStates);
    throttleUI.update(locomotive, mode == MODE_SPEED);
    lvgl_port_unlock();
}

void closeLocomotiveSelection()
{
    selectionUI.hide();
    throttleUI.setLocomotive(locomotive);
    throttleUI.update(locomotive, mode == MODE_SPEED);
}

void releaseSelectedLocomotive()
{
    noteDisplayActivity();
    dcc.releaseLoco();
    locomotive = Locomotive{};
    throttleUI.setLocomotive(locomotive);
    closeLocomotiveSelection();
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
    if (locomotive.address > 0)
        dcc.setSpeed(locomotive.address, 0, locomotive.directionForward);
    locomotive.speed = 0;
    throttleUI.update(locomotive, mode == MODE_SPEED);
    selectionUI.show(rosterLocomotives, rosterAvailable, locomotive.address);
}


// -------------------------------------------------
// Rotary encoder input
// -------------------------------------------------

bool applyListEncoderTurnImmediately(int direction)
{
    // The PCNT timer runs independently of TCP processing. Take the LVGL
    // mutex only when it is available, so list navigation is immediate and
    // the timer never waits behind a display update.
    if (!lvgl_port_lock(0))
        return false;

    bool applied = false;
    if (connectionUI.isVisible())
    {
        applied = connectionUI.move(direction);
    }
    else if (selectionUI.isVisible())
    {
        selectionUI.move(direction);
        applied = true;
    }
    else if (turnoutUI.isVisible())
    {
        turnoutUI.move(direction);
        applied = true;
    }
    else if (routeUI.isVisible())
    {
        routeUI.move(direction);
        applied = true;
    }

    if (applied)
        noteDisplayActivity();
    lvgl_port_unlock();
    return applied;
}

void onKnobLeftEventCallback(
    int count,
    void *usr_data
)
{
    if (applyListEncoderTurnImmediately(-1))
        return;
    portENTER_CRITICAL(&encoderTurnMux);
    if (pendingEncoderTurns > -MAX_PENDING_ENCODER_TURNS)
        --pendingEncoderTurns;
    portEXIT_CRITICAL(&encoderTurnMux);
}

void onKnobRightEventCallback(
    int count,
    void *usr_data
)
{
    if (applyListEncoderTurnImmediately(1))
        return;
    portENTER_CRITICAL(&encoderTurnMux);
    if (pendingEncoderTurns < MAX_PENDING_ENCODER_TURNS)
        ++pendingEncoderTurns;
    portEXIT_CRITICAL(&encoderTurnMux);
}

void applyEncoderTurn(int direction)
{
    noteDisplayActivity();
    if (connectionUI.isVisible())
    {
        connectionUI.move(direction);
        return;
    }
    if (selectionUI.isVisible())
    {
        selectionUI.move(direction);
        return;
    }
    if (turnoutUI.isVisible())
    {
        turnoutUI.move(direction);
        return;
    }
    if (routeUI.isVisible())
    {
        routeUI.move(direction);
        return;
    }
    if (powerUI.isVisible())
        return;
    if (settingsUI.isVisible())
        return;
    if (mode == MODE_SPEED)
    {
        const int speed = static_cast<int>(locomotive.speed) + direction;
        if (locomotive.address > 0 && speed >= 0 && speed <= 126)
        {
            locomotive.speed = static_cast<uint8_t>(speed);
            dcc.setSpeed(locomotive.address, locomotive.speed, locomotive.directionForward);
        }
    }
    else
    {
        const int address = static_cast<int>(locomotive.address) + direction;
        if (address >= 1 && address <= 9999)
            changeLocomotiveAddress(static_cast<uint16_t>(address));
        throttleUI.setLocomotive(locomotive);
    }

    throttleUI.update(locomotive, mode == MODE_SPEED);
}

void processPendingEncoderTurns()
{
    int32_t turns;
    portENTER_CRITICAL(&encoderTurnMux);
    turns = pendingEncoderTurns;
    if (turns > MAX_ENCODER_TURNS_PER_UPDATE)
        pendingEncoderTurns -= MAX_ENCODER_TURNS_PER_UPDATE;
    else if (turns < -MAX_ENCODER_TURNS_PER_UPDATE)
        pendingEncoderTurns += MAX_ENCODER_TURNS_PER_UPDATE;
    else
        pendingEncoderTurns = 0;
    portEXIT_CRITICAL(&encoderTurnMux);

    const int magnitude = static_cast<int>(turns < 0 ? -turns : turns);
    const int count = min(magnitude, static_cast<int>(MAX_ENCODER_TURNS_PER_UPDATE));
    const int direction = turns < 0 ? -1 : 1;
    for (int index = 0; index < count; ++index)
        applyEncoderTurn(direction);
}


// -------------------------------------------------
// Button - single click
// -------------------------------------------------

static void runHomeShortcut(HomeShortcutAction action)
{
    if (action == HomeShortcutAction::Disabled)
        return;
    if (action == HomeShortcutAction::EmergencyStop) { emergencyStop(); return; }
    if (locomotive.address == 0)
        return;
    if (action == HomeShortcutAction::Stop) { stopSelectedLoco(); return; }
    if (action == HomeShortcutAction::Direction) { toggleDirection(); return; }
    const uint8_t function = static_cast<uint8_t>(action);
    const bool momentary = locomotive.fromRoster && locomotive.functionDefinitions[function].momentary;
    if (momentary) {
        dcc.setFunction(locomotive.address, function, true);
        dcc.setFunction(locomotive.address, function, false);
        locomotive.functionStates[function] = false;
    } else {
        locomotive.functionStates[function] = !locomotive.functionStates[function];
        dcc.setFunction(locomotive.address, function, locomotive.functionStates[function]);
    }
    throttleUI.update(locomotive, mode == MODE_SPEED);
}

static void SingleClickCb(
    void *button_handle,
    void *usr_data
)
{
    lvgl_port_lock(-1);
    noteDisplayActivity();
    if (connectionUI.isVisible())
    {
        connectionUI.selectCurrent();
        lvgl_port_unlock();
        return;
    }
    if (selectionUI.isVisible())
    {
        selectionUI.select();
        lvgl_port_unlock();
        return;
    }
    if (turnoutUI.isVisible())
    {
        turnoutUI.toggleSelected();
        lvgl_port_unlock();
        return;
    }
    if (routeUI.isVisible())
    {
        routeUI.startSelected();
        closeRoutePage();
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
    runHomeShortcut(operatingPreferences.singlePressAction);
    lvgl_port_unlock();
}


// -------------------------------------------------
// Button - double click
// -------------------------------------------------

static void DoubleClickCb(void *button_handle, void *usr_data)
{
    lvgl_port_lock(-1);
    noteDisplayActivity();
    if (connectionUI.isEditingKeyboard())
        connectionUI.deleteKeyboardCharacter();
    else if (selectionUI.isVisible())
        closeLocomotiveSelection();
    else if (turnoutUI.isVisible())
        closeTurnoutPage();
    else if (routeUI.isVisible())
        closeRoutePage();
    else if (!connectionUI.isVisible() && !powerUI.isVisible() && !settingsUI.isVisible())
        runHomeShortcut(operatingPreferences.doublePressAction);
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
    if (connectionUI.isEditingKeyboard())
    {
        connectionUI.clearKeyboardText();
        lvgl_port_unlock();
        return;
    }
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
        closePowerPage();
        lvgl_port_unlock();
        return;
    }
    runHomeShortcut(operatingPreferences.longPressAction);
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
    if (function >= MAX_LOCO_FUNCTIONS || locomotive.address == 0)
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
        SingleClickCb,
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
        setSpeedPreset,
        saveFunctionPage,
        selectRosterLocomotive,
        closeLocomotiveSelection,
        refreshRosterList,
        releaseSelectedLocomotive,
        saveConnectionSettings,
        closeConnectionSettings,
        refreshCommandStationLists,
        openDiagnostics,
        connectWifi,
        discoverCommandStations,
        connectServer,
        disconnectDcc,
        setTurnoutState,
        toggleTurnoutFavorite,
        closeTurnoutPage,
        refreshTurnoutList,
        startRoute,
        closeRoutePage,
        setTrackPower,
        closePowerPage,
        setDisplayBrightness,
        setDisplaySleepTimeout,
        setHomeShortcut,
        closeDisplaySettings
    };
    app.initializeUi(uiCallbacks);
    diagnosticsUI.setDisplayProfile(device.displayProfile());
    diagnosticsUI.begin(closeDiagnostics);
    functionUI.setDisplayProfile(device.displayProfile());
    functionUI.begin(onUIFunction, closeFunctionPage);
    throttleUI.setMoreFunctionsCallback(openFunctionPage);
    dcc.setLocoUpdateCallback(onLocoBroadcast);
    dcc.setTurnoutUpdateCallback(onTurnoutBroadcast);
    dcc.setTrackPowerCallback(onTrackPowerBroadcast);
    operatingPreferencesStore.load(operatingPreferences);
    applyDisplayBrightness(operatingPreferences.displayBrightness);
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
    // Process physical encoder turns before network work. DCC reads can wait
    // for TCP traffic; doing this first prevents list navigation from being
    // delayed and then replayed as a burst.
    lvgl_port_lock(-1);
    processPendingEncoderTurns();
    lvgl_port_unlock();

    // Protocol processing can wait on network activity. Do not hold the LVGL
    // mutex while it runs, otherwise touch polling is stalled as well.
    dcc.update();

    const bool openCommandStationPicker = connectionUI.isVisible() &&
        connectionUI.isAwaitingWifiConnection() && dcc.wifiConnected();
    std::vector<CommandStationInfo> discoveredCommandStations;
    bool commandStationDiscoveryAvailable = false;
    if (openCommandStationPicker)
        commandStationDiscoveryAvailable = dcc.discoverCommandStations(discoveredCommandStations);

    lvgl_port_lock(-1);
    connectionUI.update();
    if (diagnosticsUI.isVisible())
        diagnosticsUI.update(dcc.wifiConnected(), dcc.connected(), dcc.serverResponded(),
            dcc.lastServerResponseAgeMs(), dcc.rosterReady(), dcc.turnoutsReady(), dcc.routesReady());
    const char *connectionStatus = dcc.connectionStatusText();
    const bool connectionReady =
        dcc.connectionStatus() == DccController::ConnectionStatus::Ready;
    if (displayedConnectionStatus != connectionStatus ||
        displayedConnectionReady != connectionReady)
    {
        throttleUI.setConnectionStatus(connectionStatus, connectionReady);
        displayedConnectionStatus = connectionStatus;
        displayedConnectionReady = connectionReady;
    }
    if (openCommandStationPicker)
        connectionUI.showCommandStationPicker(discoveredCommandStations,
            commandStationDiscoveryAvailable);
    else if (connectionUI.isVisible() &&
        (connectionConfigured || connectionUI.isWifiCredentialsMode() ||
         connectionUI.isServerPickerVisible() || connectionUI.isManualServerMode()))
    {
        if (!connectionUiWasVisible || displayedConnectionUiStatus != connectionStatus)
        {
            connectionUI.setStatus(connectionStatus,
                dcc.connectionStatus() == DccController::ConnectionStatus::NotConfigured);
            displayedConnectionUiStatus = connectionStatus;
        }
    }
    connectionUiWasVisible = connectionUI.isVisible();

    if (!dcc.rosterReady())
    {
        if (rosterAvailable && selectionUI.isVisible())
            selectionUI.show(rosterLocomotives, false, locomotive.address);
        rosterAvailable = false;
        rosterLoaded = false;
    }
    else if (!rosterLoaded)
    {
        rosterAvailable = dcc.copyRoster(rosterLocomotives);
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
    delay(10);
}
