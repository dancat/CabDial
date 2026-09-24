#pragma once
#include <lvgl.h>
#include <vector>
#include "RouteDefinition.h"
#include "Device.h"
class RouteUI {
public:
    using StartCallback = void (*)(int); using BackCallback = void (*)();
    void begin(StartCallback start, BackCallback back);
    void setDisplayProfile(const DisplayProfile &value) { profile = value; }
    void show(const std::vector<RouteDefinition> &routes, bool ready);
    void move(int delta); void startSelected(); void hide();
    bool isVisible() const { return visible; }
private:
    DisplayProfile profile {480, 480, true, 0, 0};
    lv_obj_t *screen=nullptr, *returnScreen=nullptr, *roller=nullptr, *status=nullptr;
    bool visible=false; std::vector<int> ids; StartCallback startCallback=nullptr; BackCallback backCallback=nullptr;
    static void startEvent(lv_event_t *event); static void backEvent(lv_event_t *event);
};
