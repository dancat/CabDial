#pragma once

#include <lvgl.h>
#include <vector>
#include "TurnoutDefinition.h"
#include "Device.h"

class TurnoutUI
{
public:
    using SetCallback = void (*)(int id, bool thrown);
    using FavoriteCallback = void (*)(int id);
    using BackCallback = void (*)();
    using RefreshCallback = void (*)();

    void begin(SetCallback set, FavoriteCallback favorite, BackCallback back,
               RefreshCallback refresh);
    void setDisplayProfile(const DisplayProfile &value) { profile = value; }
    void show(const std::vector<TurnoutDefinition> &turnouts, bool ready);
    void move(int delta);
    void closeSelected();
    void throwSelected();
    void toggleSelected();
    void toggleFavoriteSelected();
    void hide();
    bool isVisible() const { return visible; }

private:
    DisplayProfile profile {480, 480, true, 0, 0};
    lv_obj_t *screen = nullptr;
    lv_obj_t *returnScreen = nullptr;
    lv_obj_t *roller = nullptr;
    lv_obj_t *selectedLabel = nullptr;
    lv_obj_t *status = nullptr;
    bool visible = false;
    std::vector<int> ids;
    std::vector<String> optionLabels;
    std::vector<bool> thrownStates;
    SetCallback setCallback = nullptr;
    FavoriteCallback favoriteCallback = nullptr;
    BackCallback backCallback = nullptr;
    RefreshCallback refreshCallback = nullptr;

    void setSelected(bool thrown);
    void updateSelectedLabel();
    static void closeEvent(lv_event_t *event);
    static void throwEvent(lv_event_t *event);
    static void favoriteEvent(lv_event_t *event);
    static void backEvent(lv_event_t *event);
    static void refreshEvent(lv_event_t *event);
    static void rollerEvent(lv_event_t *event);
};
