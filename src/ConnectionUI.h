#pragma once

#include <lvgl.h>
#include "ConnectionSettings.h"
#include "Device.h"

class ConnectionUI
{
public:
    using SaveCallback = void (*)(const ConnectionSettings &settings);
    using BackCallback = void (*)();
    using RefreshCallback = void (*)();
    using DiagnosticsCallback = void (*)();
    void begin(SaveCallback save, BackCallback back, RefreshCallback refresh, DiagnosticsCallback diagnostics = nullptr);
    void setDisplayProfile(const DisplayProfile &value) { profile = value; }
    void show(const ConnectionSettings &settings, bool requireConfiguration);
    void hide();
    bool isVisible() const { return visible; }
    void setStatus(const char *text, bool error = false);

private:
    DisplayProfile profile {480, 480, true, 0, 0};
    lv_obj_t *screen = nullptr;
    lv_obj_t *returnScreen = nullptr;
    lv_obj_t *form = nullptr;
    lv_obj_t *keyboard = nullptr;
    lv_obj_t *editorLabel = nullptr;
    lv_obj_t *editorField = nullptr;
    lv_obj_t *activeField = nullptr;
    lv_obj_t *ssidField = nullptr;
    lv_obj_t *passwordField = nullptr;
    lv_obj_t *serverField = nullptr;
    lv_obj_t *portField = nullptr;
    lv_obj_t *statusLabel = nullptr;
    lv_obj_t *backButton = nullptr;
    lv_obj_t *refreshButton = nullptr;
    bool visible = false;
    SaveCallback saveCallback = nullptr;
    BackCallback backCallback = nullptr;
    RefreshCallback refreshCallback = nullptr;
    DiagnosticsCallback diagnosticsCallback = nullptr;

    static void fieldEvent(lv_event_t *event);
    static void keyboardEvent(lv_event_t *event);
    static void saveEvent(lv_event_t *event);
    static void backEvent(lv_event_t *event);
    static void refreshEvent(lv_event_t *event);
    static void diagnosticsEvent(lv_event_t *event);
    void finishEditing(bool saveValue);
    void save();
};
