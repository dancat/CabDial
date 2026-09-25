#pragma once

#include <Arduino.h>
#include <vector>

enum class HomeShortcutAction : uint8_t
{
    F0, F1, F2, Stop, Direction, EmergencyStop
};

struct OperatingPreferences
{
    uint8_t functionPage = 0;
    uint8_t displayBrightness = 100;
    uint16_t displaySleepSeconds = 0;
    HomeShortcutAction singlePressAction = HomeShortcutAction::F0;
    HomeShortcutAction doublePressAction = HomeShortcutAction::Stop;
    HomeShortcutAction longPressAction = HomeShortcutAction::Direction;
    std::vector<int> favoriteTurnoutIds;
};

class OperatingPreferencesStore
{
public:
    bool load(OperatingPreferences &preferences) const;
    bool save(const OperatingPreferences &preferences) const;
};
