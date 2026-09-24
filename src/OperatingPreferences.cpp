#include "OperatingPreferences.h"
#include <Preferences.h>

bool OperatingPreferencesStore::load(OperatingPreferences &preferences) const
{
    Preferences storage;
    if (!storage.begin("dcc-operating", true))
        return false;
    preferences.lastLocomotiveAddress = storage.getUShort("last-loco", 101);
    preferences.functionPage = storage.getUChar("function-page", 0);
    preferences.displayBrightness = storage.getUChar("display-bright", 100);
    preferences.displaySleepSeconds = storage.getUShort("display-sleep", 0);
    const String ids = storage.getString("turnout-favs", "");
    storage.end();

    preferences.favoriteTurnoutIds.clear();
    int start = 0;
    while (start < ids.length())
    {
        const int separator = ids.indexOf(',', start);
        const String value = ids.substring(start, separator < 0 ? ids.length() : separator);
        const int id = value.toInt();
        if (id > 0)
            preferences.favoriteTurnoutIds.push_back(id);
        if (separator < 0)
            break;
        start = separator + 1;
    }
    return true;
}

bool OperatingPreferencesStore::save(const OperatingPreferences &preferences) const
{
    Preferences storage;
    if (!storage.begin("dcc-operating", false))
        return false;
    String ids;
    for (int id : preferences.favoriteTurnoutIds)
    {
        if (ids.length())
            ids += ',';
        ids += String(id);
    }
    const bool saved = storage.putUShort("last-loco", preferences.lastLocomotiveAddress) == sizeof(uint16_t) &&
        storage.putUChar("function-page", preferences.functionPage) == sizeof(uint8_t) &&
        storage.putUChar("display-bright", preferences.displayBrightness) == sizeof(uint8_t) &&
        storage.putUShort("display-sleep", preferences.displaySleepSeconds) == sizeof(uint16_t) &&
        storage.putString("turnout-favs", ids) == ids.length();
    storage.end();
    return saved;
}
