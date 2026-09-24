#include "DeviceFactory.h"

#if defined(DEVICE_VIEWE_ROUND)
#include "ViewEDevice.h"

Device &getDevice()
{
    static ViewEDevice device;
    return device;
}
#else
#error "Select a device implementation with a DEVICE_* build flag."
#endif
