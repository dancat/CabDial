#pragma once

#include "ConnectionSettings.h"

class ConnectionSettingsStore
{
public:
    bool load(ConnectionSettings &settings) const;
    bool save(const ConnectionSettings &settings) const;
};
