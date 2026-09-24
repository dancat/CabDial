#include "ConnectionSettingsStore.h"
#include <Preferences.h>

bool ConnectionSettingsStore::load(ConnectionSettings &settings) const
{
    Preferences preferences;
    if (!preferences.begin("dcc-connection", true))
        return false;

    settings.wifiSsid = preferences.getString("ssid", "");
    settings.wifiPassword = preferences.getString("password", "");
    settings.serverAddress = preferences.getString("server", "");
    settings.serverPort = preferences.getUShort("port", 2560);
    preferences.end();
    return settings.isComplete();
}

bool ConnectionSettingsStore::save(const ConnectionSettings &settings) const
{
    if (!settings.isComplete())
        return false;

    Preferences preferences;
    if (!preferences.begin("dcc-connection", false))
        return false;

    const bool saved = preferences.putString("ssid", settings.wifiSsid) == settings.wifiSsid.length() &&
        preferences.putString("password", settings.wifiPassword) == settings.wifiPassword.length() &&
        preferences.putString("server", settings.serverAddress) > 0 &&
        preferences.putUShort("port", settings.serverPort) == sizeof(uint16_t);
    preferences.end();
    return saved;
}
