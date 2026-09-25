#pragma once

#include <Arduino.h>

struct CommandStationInfo
{
    String hostname;
    String address;
    uint16_t port = 2560;
};
