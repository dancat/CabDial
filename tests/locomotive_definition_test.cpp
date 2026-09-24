#include "Locomotive.h"
#include "LocomotiveFunctionStates.h"
#include <cassert>

int main()
{
    Locomotive current;
    current.speed = 0;
    current.directionForward = false;
    current.functionStates[0] = true;
    LocomotiveFunctionStates memory;
    memory.save(current.address, current.functionStates);

    Locomotive roster;
    roster.address = 202;
    roster.fromRoster = true;
    roster.name = "Freight";
    roster.functionDefinitions[0] = {true, false, "Lights"};
    roster.functionDefinitions[7] = {true, true, "Horn"};
    roster.speed = 90; // Definition application must not import runtime state.
    current.applyDefinition(roster);
    memory.restore(current.address, current.functionStates);
    assert(current.address == 202 && current.name == "Freight" && current.fromRoster);
    assert(current.speed == 0 && !current.directionForward);
    assert(!current.functionStates[0]);
    assert(current.functionDefinitions[7].available);
    assert(current.functionDefinitions[7].momentary);
    assert(current.functionDefinitions[7].name == "Horn");
    assert(!current.functionDefinitions[1].available);

    // Clearing/replacing the roster snapshot must not invalidate the active model.
    roster.name = "Changed";
    roster.functionDefinitions[7].name = "Changed";
    assert(current.name == "Freight");
    assert(current.functionDefinitions[7].name == "Horn");

    current.functionStates[7] = true;
    memory.save(current.address, current.functionStates);
    Locomotive manual;
    manual.address = 101;
    current.applyDefinition(manual);
    memory.restore(current.address, current.functionStates);
    assert(!current.fromRoster && current.name.empty());
    assert(current.functionStates[0] && !current.functionStates[7]);
    for (const auto &definition : current.functionDefinitions)
        assert(!definition.available && !definition.momentary && definition.name.empty());
    memory.restore(202, current.functionStates);
    assert(!current.functionStates[0] && current.functionStates[7]);
}
