#include "DccController.h"

void DccController::begin()
{
    protocol.setDelegate(this);
}

void DccController::setLocoUpdateCallback(LocoUpdateCallback callback)
{
    locoUpdateCallback = callback;
}

void DccController::setTurnoutUpdateCallback(TurnoutUpdateCallback callback)
{
    turnoutUpdateCallback = callback;
}

void DccController::setTrackPowerCallback(TrackPowerCallback callback)
{
    trackPowerCallback = callback;
}

bool DccController::connect(const ConnectionSettings &settings)
{
    if (!settings.isComplete() || !serverAddress.fromString(settings.serverAddress))
    {
        Serial.println("Invalid connection settings");
        configured = false;
        status = ConnectionStatus::NotConfigured;
        return false;
    }

    serverPort = settings.serverPort;
    configured = true;
    protocolConnected = false;
    versionRequested = false;
    sessionVersionReceived = false;
    wifiWasConnected = false;
    client.stop();

    Serial.println("Starting WiFi...");
    WiFi.mode(WIFI_STA);
    // Prevent station power saving from delaying or dropping the persistent
    // TCP session to the Command Station.
    WiFi.setSleep(false);
    WiFi.disconnect(false, false);
    WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPassword.c_str());
    status = ConnectionStatus::ConnectingWiFi;
    return true;
}


void DccController::update()
{
    if (!configured)
        return;
    // --------------------------------------------------
    // WiFi connection
    // --------------------------------------------------

    const bool wifiIsConnected = wifiConnected();
    if (wifiIsConnected != wifiWasConnected)
    {
        Serial.println(wifiIsConnected ? "WiFi connected" : "WiFi disconnected");
        wifiWasConnected = wifiIsConnected;
    }

    if (!wifiIsConnected)
    {
        status = ConnectionStatus::ConnectingWiFi;
        if (protocolConnected)
        {
            Serial.println("EX-CommandStation connection lost: WiFi unavailable");
            protocolConnected = false;
            sessionVersionReceived = false;
            versionRequested = false;
            client.stop();
        }
        return;
    }

    // --------------------------------------------------
    // Connect to EX-CommandStation
    // --------------------------------------------------

    if (!client.connected())
    {
        if (protocolConnected)
        {
            Serial.println("EX-CommandStation TCP disconnected while WiFi is connected");
            protocolConnected = false;
            sessionVersionReceived = false;
            versionRequested = false;
        }
        unsigned long now = millis();
        status = ConnectionStatus::ConnectingServer;

        if (now - lastConnectionAttempt < CONNECTION_RETRY_INTERVAL)
        {
            return;
        }

        lastConnectionAttempt = now;

        Serial.print("Connecting to EX-CommandStation at ");
        Serial.print(serverAddress);
        Serial.print(":");
        Serial.println(serverPort);

        if (client.connect(serverAddress, serverPort))
        {
            Serial.println("Connected to EX-CommandStation");
            // DCC-EX commands are tiny frames. Send each one immediately
            // instead of waiting for TCP's Nagle coalescing delay.
            client.setNoDelay(true);

            protocol.connect(&client);
            protocol.setLogStream(&Serial);
            protocol.setDebug(false);
            // Keep the TCP session alive without sending a heartbeat on every
            // update cycle. The default delay is zero, which can otherwise
            // flood the Command Station while its object lists are loading.
            protocol.enableHeartbeat(5000);
            // These methods reset the library's list state. getLists(), below,
            // sends the actual requests in roster/turnout/route order once the
            // Command Station version handshake has completed.
            protocol.refreshRoster();
            protocol.refreshTurnoutList();
            protocol.refreshRouteList();

            protocolConnected = true;
            versionRequested = false;
            sessionVersionReceived = false;
            connectionStartedAt = millis();
            lastVersionRequestAt = 0;
            lastObservedServerResponseAt = connectionStartedAt;
            serverRespondedSinceConnection = false;
            resetListLoadRetries();
        }
        else
        {
            Serial.println("EX-CommandStation connection failed");
            protocolConnected = false;
        }

        return;
    }

    // --------------------------------------------------
    // DCCEXProtocol processing
    // --------------------------------------------------

    if (protocolConnected)
    {
        protocol.check();
        const unsigned long lastResponseAt = protocol.getLastServerResponseTime();
        if (lastResponseAt != lastObservedServerResponseAt)
        {
            lastObservedServerResponseAt = lastResponseAt;
            serverRespondedSinceConnection = true;
        }

        if (!sessionVersionReceived)
        {
            const unsigned long now = millis();
            if (!versionRequested || now - lastVersionRequestAt >= VERSION_REQUEST_INTERVAL)
            {
                protocol.requestServerVersion();
                versionRequested = true;
                lastVersionRequestAt = now;
                Serial.println("Requested EX-CommandStation version");
            }

            if (now - connectionStartedAt >= VERSION_HANDSHAKE_TIMEOUT)
            {
                // Some Command Station / network combinations keep the TCP
                // session alive but do not answer <s>. Do not discard that
                // working session in a reconnect loop: roster, turnout, and
                // route requests are independent of the version reply.
                Serial.println("EX-CommandStation version handshake timed out; continuing without version");
                sessionVersionReceived = true;
                status = ConnectionStatus::Ready;
            }
        }
        else
        {
            // DCCEXProtocol deliberately gates list requests: it sends the
            // roster request first, then advances to turnouts and routes after
            // each response. Keep calling it until all requested lists arrive.
            // This belongs to the connection lifecycle, rather than depending
            // on a particular UI screen or main-loop consumer.
            requestListsWithFallback();
        }
    }
}

void DccController::receivedServerVersion(int major, int minor, int patch)
{
    if (!sessionVersionReceived)
    {
        Serial.printf("EX-CommandStation version: %d.%d.%d\n", major, minor, patch);
    }
    sessionVersionReceived = true;
    status = ConnectionStatus::Ready;
}

void DccController::receivedLocoBroadcast(int address, int speed,
    Direction direction, int functionMap)
{
    if (locoUpdateCallback == nullptr || address < 1 || speed < 0)
        return;

    locoUpdateCallback(
        static_cast<uint16_t>(address),
        static_cast<uint8_t>(speed),
        direction == Forward,
        static_cast<uint32_t>(functionMap)
    );
}

void DccController::receivedTurnoutAction(int turnoutId, bool thrown)
{
    if (turnoutUpdateCallback)
        turnoutUpdateCallback(turnoutId, thrown);
}

void DccController::receivedTrackPower(TrackPower state)
{
    if (trackPowerCallback)
        trackPowerCallback(state != PowerUnknown, state == PowerOn);
}

bool DccController::wifiConnected()
{
    return WiFi.status() == WL_CONNECTED;
}


bool DccController::connected()
{
    return wifiConnected() && protocolConnected && client.connected();
}


bool DccController::serverReady()
{
    // The library's receivedVersion flag survives reconnects; this flag does not.
    return connected() && sessionVersionReceived;
}

bool DccController::serverResponded()
{
    return connected() && serverRespondedSinceConnection;
}

unsigned long DccController::lastServerResponseAgeMs()
{
    if (!connected() || !serverRespondedSinceConnection)
        return 0;
    return millis() - lastObservedServerResponseAt;
}

void DccController::resetListLoadRetries()
{
    listLoadingStartedAt = millis();
    lastRosterFallbackAt = 0;
    lastTurnoutFallbackAt = 0;
    lastRouteFallbackAt = 0;
}

void DccController::requestListsWithFallback()
{
    protocol.getLists(true, true, true, false);

    const unsigned long now = millis();
    if (now - listLoadingStartedAt < LIST_FALLBACK_DELAY)
        return;

    // DCCEXProtocol deliberately waits for a roster response before it sends
    // turnouts and routes. Retry the roster and request the other lists
    // independently when a Command Station leaves that first request pending.
    if (!protocol.receivedRoster() &&
        (lastRosterFallbackAt == 0 || now - lastRosterFallbackAt >= LIST_FALLBACK_RETRY_INTERVAL))
    {
        protocol.sendCommand("JR");
        lastRosterFallbackAt = now;
        Serial.println("Retrying DCC-EX roster request (<JR>)");
    }

    if (!protocol.receivedTurnoutList() &&
        (lastTurnoutFallbackAt == 0 || now - lastTurnoutFallbackAt >= LIST_FALLBACK_RETRY_INTERVAL))
    {
        protocol.sendCommand("JT");
        lastTurnoutFallbackAt = now;
        Serial.println("Requesting DCC-EX turnout list independently (<JT>)");
    }

    if (!protocol.receivedRouteList() &&
        (lastRouteFallbackAt == 0 || now - lastRouteFallbackAt >= LIST_FALLBACK_RETRY_INTERVAL))
    {
        protocol.sendCommand("JA");
        lastRouteFallbackAt = now;
        Serial.println("Requesting DCC-EX route list independently (<JA>)");
    }
}

DccController::ConnectionStatus DccController::connectionStatus() const
{
    return status;
}

const char *DccController::connectionStatusText() const
{
    switch (status)
    {
    case ConnectionStatus::NotConfigured: return "CONNECTION SETUP REQUIRED";
    case ConnectionStatus::ConnectingWiFi: return "CONNECTING WIFI";
    case ConnectionStatus::ConnectingServer: return "CONNECTING DCC-EX";
    case ConnectionStatus::Ready: return "DCC-EX CONNECTED";
    }
    return "CONNECTION UNKNOWN";
}

void DccController::setSpeed(
    uint16_t address,
    uint8_t speed,
    bool directionForward)
{
    if (!serverReady())
    {
        return;
    }

    selectLoco(address);

    if (dccLoco == nullptr)
    {
        return;
    }

    Direction direction = directionForward ? Forward : Reverse;

    protocol.setThrottle(dccLoco, speed, direction);
}

void DccController::setFunction(
    uint16_t address,
    uint8_t function,
    bool state)
{
    if (!serverReady())
    {
        return;
    }

    selectLoco(address);

    if (dccLoco == nullptr)
    {
        return;
    }

    if (state)
    {
        protocol.functionOn(dccLoco, function);
    }
    else
    {
        protocol.functionOff(dccLoco, function);
    }
}

void DccController::selectLoco(uint16_t address)
{
    if (!serverReady())
    {
        return;
    }

    if (dccLoco != nullptr && dccLoco->getAddress() == address)
    {
        return;
    }

    // setThrottle queues updates. Preserve the old loco's pending stop/change
    // before deleting it, even if selection occurs before the next check().
    if (dccLoco != nullptr && dccLoco->getUserChangePending())
    {
        char command[40];
        snprintf(command, sizeof(command), "t %d %d %d", dccLoco->getAddress(),
                 dccLoco->getUserSpeed(), static_cast<int>(dccLoco->getUserDirection()));
        protocol.sendCommand(command);
    }
    delete dccLoco;
    dccLoco = new Loco(address, LocoSourceEntry);

    Serial.print("Selected locomotive address: ");
    Serial.println(address);
}

void DccController::stopLoco()
{
    if (!serverReady() || dccLoco == nullptr)
    {
        return;
    }

    Direction direction = Forward;

    protocol.setThrottle(dccLoco, 0, direction);

    Serial.print("Stopped locomotive address: ");
    Serial.println(dccLoco->getAddress());
}

bool DccController::rosterReady()
{
    return serverReady() && protocol.receivedRoster();
}

int DccController::rosterCount()
{
    return protocol.getRosterCount();
}

Loco *DccController::firstRosterLoco()
{
    return Loco::getFirst();
}

void DccController::refreshRoster()
{
    if (!serverReady())
    {
        Serial.println("Roster refresh ignored: DCC-EX is not connected");
        return;
    }

    Serial.println("Refreshing DCC-EX roster");
    protocol.refreshRoster();
    resetListLoadRetries();
    protocol.getLists(true, false, false, false);
    Serial.println("Requested DCC-EX roster (<JR>)");
}

void DccController::refreshTurnouts()
{
    if (!serverReady())
    {
        Serial.println("Turnout refresh ignored: DCC-EX is not connected");
        return;
    }

    Serial.println("Refreshing DCC-EX turnout list");
    protocol.refreshTurnoutList();
    resetListLoadRetries();
    if (protocol.receivedRoster())
    {
        protocol.getLists(false, true, false, false);
    }
    else
    {
        // DCCEXProtocol serializes object-list loading behind the roster. A
        // Command Station that does not answer <JR> would otherwise prevent
        // a turnout refresh forever. Its parser still handles <jT...>
        // responses correctly when this request is sent directly.
        protocol.sendCommand("JT");
    }
    Serial.println("Requested DCC-EX turnout list (<JT>)");
}

void DccController::refreshLists()
{
    if (!serverReady())
        return;
    protocol.refreshRoster();
    protocol.refreshTurnoutList();
    protocol.refreshRouteList();
    resetListLoadRetries();
}

void DccController::requestRoster()
{
    if (serverReady())
    {
        protocol.getLists(true, true, true, false);
    }
}

bool DccController::turnoutsReady()
{
    return serverReady() && protocol.receivedTurnoutList();
}

bool DccController::copyTurnouts(std::vector<TurnoutDefinition> &entries)
{
    entries.clear();
    if (!turnoutsReady())
        return false;

    for (Turnout *source = Turnout::getFirst(); source; source = source->getNext())
    {
        TurnoutDefinition entry;
        entry.id = source->getId();
        entry.name = source->getName() ? source->getName() : "";
        entry.thrown = source->getThrown();
        entries.push_back(entry);
    }
    return true;
}

void DccController::setTurnout(int id, bool thrown)
{
    if (!turnoutsReady())
        return;
    if (thrown)
        protocol.throwTurnout(id);
    else
        protocol.closeTurnout(id);
}

bool DccController::routesReady() { return serverReady() && protocol.receivedRouteList(); }
bool DccController::copyRoutes(std::vector<RouteDefinition> &entries) {
    entries.clear(); if (!routesReady()) return false;
    for (Route *source = Route::getFirst(); source; source = source->getNext()) {
        RouteDefinition entry; entry.id=source->getId(); entry.name=source->getName()?source->getName():"";
        entry.automation=source->getType()==RouteTypeAutomation; entries.push_back(entry);
    } return true;
}
void DccController::startRoute(int id) { if (routesReady()) protocol.startRoute(id); }

void DccController::emergencyStop()
{
    if (serverReady())
        protocol.emergencyStop();
}

void DccController::setTrackPower(bool on)
{
    if (!serverReady())
        return;
    if (on)
        protocol.powerOn();
    else
        protocol.powerOff();
}

bool DccController::copyRoster(std::vector<Locomotive> &entries)
{
    entries.clear();
    if (!rosterReady())
        return false;

    for (Loco *source = Loco::getFirst(); source; source = source->getNext())
    {
        Locomotive entry;
        entry.address = source->getAddress();
        entry.name = source->getName() ? source->getName() : "";
        entry.fromRoster = true;
        for (uint8_t function = 0; function < MAX_LOCO_FUNCTIONS; ++function)
        {
            const char *name = source->getFunctionName(function);
            LocoFunction &definition = entry.functionDefinitions[function];
            definition.available = name && name[0] != '\0';
            definition.name = name ? name : "";
            definition.momentary = source->isFunctionMomentary(function);
        }
        entries.push_back(entry);
    }
    return true;
}
