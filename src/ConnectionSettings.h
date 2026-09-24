#pragma once

#include <Arduino.h>

struct ConnectionSettings
{
    String wifiSsid;
    String wifiPassword;
    String serverAddress;
    uint16_t serverPort = 2560;

    bool isComplete() const
    {
        return wifiSsid.length() > 0 && serverAddress.length() > 0 && serverPort > 0;
    }
};
