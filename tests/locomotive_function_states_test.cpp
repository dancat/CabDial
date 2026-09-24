#include "LocomotiveFunctionStates.h"
#include <cassert>

int main()
{
    LocomotiveFunctionStates memory;
    bool functions[29] = {};
    functions[0] = true;
    functions[4] = true;
    functions[28] = true;
    memory.save(101, functions);

    // A new address must not inherit any functions from the previous address.
    memory.restore(102, functions);
    for (bool state : functions)
        assert(!state);
    functions[1] = true;
    memory.save(102, functions);

    // Returning restores the original locomotive, including its high functions.
    memory.restore(101, functions);
    for (unsigned i = 0; i < 29; ++i)
        assert(functions[i] == (i == 0 || i == 4 || i == 28));

    // Turning a previously enabled function off must also be remembered.
    functions[0] = false;
    memory.save(101, functions);
    memory.restore(102, functions);
    for (unsigned i = 0; i < 29; ++i)
        assert(functions[i] == (i == 1));

    // Browsing an unused address must not disturb either saved locomotive.
    memory.restore(103, functions);
    for (bool state : functions)
        assert(!state);
    memory.restore(101, functions);
    assert(!functions[0] && functions[4] && functions[28]);

    functions[4] = false;
    functions[28] = false;
    memory.save(101, functions);
    memory.restore(102, functions);
    assert(functions[1]);
    memory.restore(101, functions);
    for (bool state : functions)
        assert(!state);

    // A broadcast for another throttle's locomotive is retained until it is selected.
    memory.saveMask(104, (uint32_t{1} << 2) | (uint32_t{1} << 28));
    memory.restore(104, functions);
    for (unsigned i = 0; i < 29; ++i)
        assert(functions[i] == (i == 2 || i == 28));
}
