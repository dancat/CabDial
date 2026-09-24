#pragma once

#include "Device.h"
#include "devices/viewe/ViewEBoardConfig.h"

namespace esp_panel::board
{
class Board;
}

class ESP_Knob;
class Button;

class ViewEDevice final : public Device
{
public:
    bool begin(const InputCallbacks &callbacks) override;
    void setBacklight(uint8_t brightness) override;
    const DisplayProfile &displayProfile() const override { return profile; }
    const DeviceCapabilities &capabilities() const override { return deviceCapabilities; }

private:
    esp_panel::board::Board *board = nullptr;
    ESP_Knob *knob = nullptr;
    Button *button = nullptr;
    DisplayProfile profile {viewe::DISPLAY_WIDTH, viewe::DISPLAY_HEIGHT,
        viewe::DISPLAY_IS_ROUND, viewe::SAFE_TOP, viewe::SAFE_BOTTOM};
    DeviceCapabilities deviceCapabilities {true, true, true, true};
};
