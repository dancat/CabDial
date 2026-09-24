#pragma once

#include <cstddef>
#include <cstdint>
#include <map>

// Session memory for function states, independent of the DCC protocol objects.
class LocomotiveFunctionStates
{
public:
    template <std::size_t N>
    void save(uint16_t address, const bool (&states)[N])
    {
        static_assert(N <= 32, "Function state mask supports F0-F31");
        uint32_t mask = 0;
        for (std::size_t i = 0; i < N; ++i)
        {
            if (states[i])
                mask |= uint32_t{1} << i;
        }
        // Browsing unused addresses should not allocate a cache entry for each one.
        if (mask == 0)
            statesByAddress.erase(address);
        else
            statesByAddress[address] = mask;
    }

    template <std::size_t N>
    void restore(uint16_t address, bool (&states)[N]) const
    {
        static_assert(N <= 32, "Function state mask supports F0-F31");
        const auto entry = statesByAddress.find(address);
        const uint32_t mask = entry == statesByAddress.end() ? 0 : entry->second;
        for (std::size_t i = 0; i < N; ++i)
            states[i] = (mask & (uint32_t{1} << i)) != 0;
    }

    void saveMask(uint16_t address, uint32_t mask)
    {
        if (mask == 0)
            statesByAddress.erase(address);
        else
            statesByAddress[address] = mask;
    }

private:
    std::map<uint16_t, uint32_t> statesByAddress;
};
