#pragma once

#include <Arduino.h>

struct DisplayProfile
{
    uint16_t width;
    uint16_t height;
    bool round;
    uint16_t safeTop;
    uint16_t safeBottom;
};

struct DeviceCapabilities
{
    bool touch = true;
    bool encoder = false;
    bool physicalButton = false;
    bool adjustableBacklight = false;
};

class Device
{
public:
    using EncoderCallback = void (*)(int count, void *userData);
    using ButtonCallback = void (*)(void *buttonHandle, void *userData);

    struct InputCallbacks
    {
        EncoderCallback encoderDecrease = nullptr;
        EncoderCallback encoderIncrease = nullptr;
        ButtonCallback doubleClick = nullptr;
        ButtonCallback longPress = nullptr;
    };

    virtual ~Device() = default;
    virtual bool begin(const InputCallbacks &callbacks) = 0;
    virtual void setBacklight(uint8_t brightness) = 0;
    virtual const DisplayProfile &displayProfile() const = 0;
    virtual const DeviceCapabilities &capabilities() const = 0;
};
