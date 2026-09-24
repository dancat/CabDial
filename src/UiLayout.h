#pragma once

#include "Device.h"

// Coordinates in the existing UI are expressed against a 480 x 480 design
// canvas. This helper preserves the ViewE layout exactly and maps it to a
// future device's physical canvas. Safe-area-aware layouts can use the
// content helpers when a target profile defines those requirements.
class UiLayout
{
public:
    explicit UiLayout(const DisplayProfile &profile) : profile(profile) {}

    int x(int designPixels) const { return designPixels * profile.width / 480; }
    int y(int designPixels) const { return designPixels * profile.height / 480; }
    int width(int designPixels) const { return designPixels * profile.width / 480; }
    int height(int designPixels) const { return designPixels * profile.height / 480; }
    int contentY(int designPixels) const
    {
        const int usableHeight = profile.height - profile.safeTop - profile.safeBottom;
        return profile.safeTop + designPixels * usableHeight / 480;
    }
    int contentHeight(int designPixels) const
    {
        return designPixels * (profile.height - profile.safeTop - profile.safeBottom) / 480;
    }
    const DisplayProfile &display() const { return profile; }

private:
    const DisplayProfile &profile;
};
