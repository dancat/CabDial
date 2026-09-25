#pragma once

#include "Device.h"
#include "devices/viewe/ViewEBoardConfig.h"

namespace esp_panel::board
{
class Board;
}

class Button;
struct pcnt_unit_t;
typedef struct pcnt_unit_t *pcnt_unit_handle_t;
struct esp_timer;
typedef struct esp_timer *esp_timer_handle_t;

class ViewEDevice final : public Device
{
public:
    bool begin(const InputCallbacks &callbacks) override;
    void setBacklight(uint8_t brightness) override;
    const DisplayProfile &displayProfile() const override { return profile; }
    const DeviceCapabilities &capabilities() const override { return deviceCapabilities; }

private:
    esp_panel::board::Board *board = nullptr;
    pcnt_unit_handle_t encoderUnit = nullptr;
    esp_timer_handle_t encoderTimer = nullptr;
    InputCallbacks encoderCallbacks;
    int encoderCount = 0;
    Button *button = nullptr;
    DisplayProfile profile {viewe::DISPLAY_WIDTH, viewe::DISPLAY_HEIGHT,
        viewe::DISPLAY_IS_ROUND, viewe::SAFE_TOP, viewe::SAFE_BOTTOM};
    DeviceCapabilities deviceCapabilities {true, true, true, true};
    static void pollEncoder(void *arg);
    bool beginEncoder(const InputCallbacks &callbacks);
};
