#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <DCCEXProtocol.h>
#include <vector>
#include "Locomotive.h"
#include "ConnectionSettings.h"
#include "TurnoutDefinition.h"
#include "RouteDefinition.h"

class DccController : private DCCEXProtocolDelegate
{
public:
    using LocoUpdateCallback = void (*)(uint16_t address, uint8_t speed,
        bool directionForward, uint32_t functionMap);
    using TurnoutUpdateCallback = void (*)(int id, bool thrown);
    using TrackPowerCallback = void (*)(bool known, bool on);

    enum class ConnectionStatus : uint8_t
    {
        NotConfigured,
        ConnectingWiFi,
        ConnectingServer,
        Ready
    };

    void begin();
    bool connect(const ConnectionSettings &settings);

    void update();

    bool wifiConnected();
    bool connected();
    bool serverReady();
    ConnectionStatus connectionStatus() const;
    const char *connectionStatusText() const;

    void setSpeed(uint16_t address, uint8_t speed, bool directionForward);
    void setFunction(uint16_t address, uint8_t function, bool state);
    void selectLoco(uint16_t address);
    void stopLoco();
    bool rosterReady();
    int rosterCount();

    Loco *firstRosterLoco();
    void refreshRoster();
    void refreshTurnouts();
    void refreshLists();
    void requestRoster();
    bool copyRoster(std::vector<Locomotive> &entries);
    void setLocoUpdateCallback(LocoUpdateCallback callback);
    bool turnoutsReady();
    bool copyTurnouts(std::vector<TurnoutDefinition> &entries);
    void setTurnout(int id, bool thrown);
    void setTurnoutUpdateCallback(TurnoutUpdateCallback callback);
    bool routesReady();
    bool copyRoutes(std::vector<RouteDefinition> &entries);
    void startRoute(int id);
    void emergencyStop();
    void setTrackPower(bool on);
    void setTrackPowerCallback(TrackPowerCallback callback);
private:
    void receivedServerVersion(int major, int minor, int patch) override;
    void receivedLocoBroadcast(int address, int speed, Direction direction,
        int functionMap) override;
    void receivedTurnoutAction(int turnoutId, bool thrown) override;
    void receivedTrackPower(TrackPower state) override;

    WiFiClient client;
    DCCEXProtocol protocol;

    Loco *dccLoco = nullptr;

    IPAddress serverAddress;
    uint16_t serverPort = 2560;

    bool protocolConnected = false;
    bool versionRequested = false;
    bool sessionVersionReceived = false;
    bool wifiWasConnected = false;
    bool configured = false;
    ConnectionStatus status = ConnectionStatus::NotConfigured;
    LocoUpdateCallback locoUpdateCallback = nullptr;
    TurnoutUpdateCallback turnoutUpdateCallback = nullptr;
    TrackPowerCallback trackPowerCallback = nullptr;

    unsigned long lastConnectionAttempt = 0;
    unsigned long connectionStartedAt = 0;
    unsigned long lastVersionRequestAt = 0;
    static constexpr unsigned long CONNECTION_RETRY_INTERVAL = 5000;
    static constexpr unsigned long VERSION_REQUEST_INTERVAL = 5000;
    static constexpr unsigned long VERSION_HANDSHAKE_TIMEOUT = 15000;
};
