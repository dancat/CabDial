#pragma once

#include <Arduino.h>
#include <vector>

struct OperatingPreferences
{
    uint16_t lastLocomotiveAddress = 101;
    uint8_t functionPage = 0;
    uint8_t displayBrightness = 100;
    uint16_t displaySleepSeconds = 0;
    std::vector<int> favoriteTurnoutIds;
};

class OperatingPreferencesStore
{
public:
    bool load(OperatingPreferences &preferences) const;
    bool save(const OperatingPreferences &preferences) const;
};
