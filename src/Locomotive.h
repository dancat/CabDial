#pragma once

#include <Arduino.h>

static constexpr uint8_t MAX_LOCO_FUNCTIONS = 32;

struct LocoFunction
{
    bool available = false;
    bool momentary = false;
    String name;
};

struct Locomotive
{
    // Identity
    uint16_t address = 0;
    String name;
    bool fromRoster = false;

    // Runtime state
    uint8_t speed = 0;
    bool directionForward = true;

    // Functions
    bool functionStates[MAX_LOCO_FUNCTIONS] = {};
    LocoFunction functionDefinitions[MAX_LOCO_FUNCTIONS];

    // Replace identity/metadata without overwriting this throttle's runtime state.
    void applyDefinition(const Locomotive &definition)
    {
        address = definition.address;
        name = definition.name;
        fromRoster = definition.fromRoster;
        for (uint8_t function = 0; function < MAX_LOCO_FUNCTIONS; ++function)
            functionDefinitions[function] = definition.functionDefinitions[function];
    }
};
