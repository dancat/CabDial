#pragma once

#include <lvgl.h>
#include <vector>
#include "ConnectionSettings.h"
#include "Device.h"
#include "CommandStationInfo.h"

class ConnectionUI
{
public:
    using SaveCallback = void (*)(const ConnectionSettings &settings);
    using BackCallback = void (*)();
    using RefreshCallback = void (*)();
    using DiagnosticsCallback = void (*)();
    using WifiConnectCallback = void (*)(const String &ssid, const String &password);
    using DiscoverCallback = bool (*)(std::vector<CommandStationInfo> &stations);
    using ServerConnectCallback = void (*)(const String &address, uint16_t port);
    using DisconnectCallback = void (*)();
    void begin(SaveCallback save, BackCallback back, RefreshCallback refresh,
        DiagnosticsCallback diagnostics = nullptr, WifiConnectCallback wifiConnect = nullptr,
        DiscoverCallback discover = nullptr, ServerConnectCallback serverConnect = nullptr,
        DisconnectCallback disconnect = nullptr);
    void setDisplayProfile(const DisplayProfile &value) { profile = value; }
    void show(const ConnectionSettings &settings, bool requireConfiguration);
    void showConnectedDetails(const ConnectionSettings &settings);
    void hide();
    bool isVisible() const { return visible; }
    bool isWifiCredentialsMode() const { return wifiCredentialsMode; }
    bool isAwaitingWifiConnection() const { return wifiConnectRequested; }
    bool isServerPickerVisible() const { return serverPickerVisible; }
    bool isManualServerMode() const { return manualServerMode; }
    bool isEditingKeyboard() const { return keyboard && !lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN); }
    void update();
    void showCommandStationPicker(const std::vector<CommandStationInfo> &stations,
        bool discoveryAvailable);
    void setStatus(const char *text, bool error = false);
    // Called while the LVGL mutex is held by the physical-input path.
    bool move(int delta);
    bool selectCurrent();
    void deleteKeyboardCharacter();
    void clearKeyboardText();

private:
    DisplayProfile profile {480, 480, true, 0, 0};
    lv_obj_t *screen = nullptr;
    lv_obj_t *returnScreen = nullptr;
    lv_obj_t *form = nullptr;
    lv_obj_t *formTitle = nullptr;
    lv_obj_t *networkPicker = nullptr;
    lv_obj_t *serverPicker = nullptr;
    lv_obj_t *connectedDetails = nullptr;
    lv_obj_t *connectedNetworkValue = nullptr;
    lv_obj_t *connectedServerValue = nullptr;
    lv_obj_t *keyboard = nullptr;
    lv_obj_t *editorLabel = nullptr;
    lv_obj_t *editorField = nullptr;
    lv_obj_t *activeField = nullptr;
    lv_obj_t *ssidField = nullptr;
    lv_obj_t *ssidLabel = nullptr;
    lv_obj_t *passwordField = nullptr;
    lv_obj_t *passwordLabel = nullptr;
    lv_obj_t *serverField = nullptr;
    lv_obj_t *serverLabel = nullptr;
    lv_obj_t *portField = nullptr;
    lv_obj_t *portLabel = nullptr;
    lv_obj_t *statusLabel = nullptr;
    lv_obj_t *backButton = nullptr;
    lv_obj_t *refreshButton = nullptr;
    lv_obj_t *diagnosticsButton = nullptr;
    lv_obj_t *saveButton = nullptr;
    lv_obj_t *networkStatusLabel = nullptr;
    lv_obj_t *networkRoller = nullptr;
    lv_obj_t *networkSelectedLabel = nullptr;
    lv_obj_t *networkBackButton = nullptr;
    lv_obj_t *networkSelectButton = nullptr;
    lv_obj_t *networkScanButton = nullptr;
    lv_obj_t *manualSetupButton = nullptr;
    lv_obj_t *wifiConnectButton = nullptr;
    lv_obj_t *serverStatusLabel = nullptr;
    lv_obj_t *serverRoller = nullptr;
    lv_obj_t *serverSelectedLabel = nullptr;
    lv_obj_t *serverBackButton = nullptr;
    lv_obj_t *serverSelectButton = nullptr;
    lv_obj_t *serverScanButton = nullptr;
    lv_obj_t *serverManualButton = nullptr;
    bool visible = false;
    bool requireConfiguration = false;
    bool wifiCredentialsMode = false;
    bool wifiConnectRequested = false;
    bool serverPickerVisible = false;
    bool manualServerMode = false;
    bool networkScanInProgress = false;
    uint16_t keyboardSelectedButton = 0;
    std::vector<String> networkNames;
    std::vector<CommandStationInfo> commandStations;
    SaveCallback saveCallback = nullptr;
    BackCallback backCallback = nullptr;
    RefreshCallback refreshCallback = nullptr;
    DiagnosticsCallback diagnosticsCallback = nullptr;
    WifiConnectCallback wifiConnectCallback = nullptr;
    DiscoverCallback discoverCallback = nullptr;
    ServerConnectCallback serverConnectCallback = nullptr;
    DisconnectCallback disconnectCallback = nullptr;

    static void fieldEvent(lv_event_t *event);
    static void keyboardEvent(lv_event_t *event);
    static void saveEvent(lv_event_t *event);
    static void backEvent(lv_event_t *event);
    static void refreshEvent(lv_event_t *event);
    static void diagnosticsEvent(lv_event_t *event);
    static void networkSelectEvent(lv_event_t *event);
    static void networkScanEvent(lv_event_t *event);
    static void manualSetupEvent(lv_event_t *event);
    static void networkBackEvent(lv_event_t *event);
    static void networkRollerEvent(lv_event_t *event);
    static void wifiConnectEvent(lv_event_t *event);
    static void serverSelectEvent(lv_event_t *event);
    static void serverScanEvent(lv_event_t *event);
    static void serverManualEvent(lv_event_t *event);
    static void serverBackEvent(lv_event_t *event);
    static void serverRollerEvent(lv_event_t *event);
    static void detailsBackEvent(lv_event_t *event);
    static void disconnectEvent(lv_event_t *event);
    void finishEditing(bool saveValue);
    void save();
    void showNetworkPicker();
    void showManualForm();
    void showManualServerForm();
    void showWifiCredentialsForm();
    void connectWifi();
    void connectManualServer();
    void scanNetworks();
    void finishNetworkScan(int count);
    void scanCommandStations();
    void selectNetwork();
    void selectCommandStation();
    void updateNetworkSelection();
    void updateCommandStationSelection();
    void moveKeyboardSelection(int delta);
    void enterKeyboardSelection();
    uint16_t keyboardButtonCount() const;
};
