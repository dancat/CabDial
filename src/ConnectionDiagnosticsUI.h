#pragma once
#include <lvgl.h>
#include "Device.h"
class ConnectionDiagnosticsUI {
public:
    using BackCallback = void (*)();
    void begin(BackCallback back);
    void setDisplayProfile(const DisplayProfile &value) { profile=value; }
    void show(); void hide(); bool isVisible() const { return visible; }
    void update(bool wifi, bool tcp, bool roster, bool turnouts, bool routes);
private:
    DisplayProfile profile {480,480,true,0,0}; lv_obj_t *screen=nullptr; lv_obj_t *returnScreen=nullptr; lv_obj_t *labels[5]={}; bool visible=false; BackCallback backCallback=nullptr;
    static void backEvent(lv_event_t *event);
};
