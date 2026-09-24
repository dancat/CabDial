#pragma once

#include <Arduino.h>

// Application-owned snapshot. UI code never keeps DCCEXProtocol Turnout pointers.
struct TurnoutDefinition
{
    int id = 0;
    String name;
    bool thrown = false;
    bool favorite = false;
};
