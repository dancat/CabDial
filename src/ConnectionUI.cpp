#include "ConnectionUI.h"
#include "lvgl_v8_port.h"
#include "UiLayout.h"
#include <WiFi.h>

namespace
{
lv_obj_t *makeField(lv_obj_t *parent, const UiLayout &layout, const char *title, int y,
    lv_obj_t **labelOut)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x8899AA), 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, layout.y(y));
    if (labelOut)
        *labelOut = label;

    lv_obj_t *field = lv_textarea_create(parent);
    lv_obj_set_size(field, layout.width(350), layout.height(38));
    lv_obj_align(field, LV_ALIGN_TOP_MID, 0, layout.y(y + 18));
    lv_textarea_set_one_line(field, true);
    lv_obj_set_style_bg_color(field, lv_color_hex(0x17232D), LV_PART_MAIN);
    lv_obj_set_style_text_color(field, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(field, &lv_font_montserrat_16, LV_PART_MAIN);
    return field;
}

lv_obj_t *makeButton(lv_obj_t *parent, const UiLayout &layout, const char *text, int x, int y,
    lv_event_cb_t callback, void *user)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_size(button, layout.width(78), layout.height(38));
    lv_obj_align(button, LV_ALIGN_TOP_MID, layout.x(x), layout.y(y));
    lv_obj_set_style_bg_color(button, lv_color_hex(0x263746), 0);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, user);
    return button;
}

lv_obj_t *makeWideButton(lv_obj_t *parent, const UiLayout &layout, const char *text,
    int width, int height, int x, int y, lv_event_cb_t callback, void *user)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_size(button, layout.width(width), layout.height(height));
    lv_obj_align(button, LV_ALIGN_TOP_MID, layout.x(x), layout.y(y));
    lv_obj_set_style_bg_color(button, lv_color_hex(0x263746), 0);
    lv_obj_set_style_border_color(button, lv_color_hex(0x3C566B), 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_radius(button, 14, 0);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, user);
    return button;
}
}

void ConnectionUI::begin(SaveCallback save, BackCallback back, RefreshCallback refresh,
    DiagnosticsCallback diagnostics, WifiConnectCallback wifiConnect,
    DiscoverCallback discover, ServerConnectCallback serverConnect)
{
    lvgl_port_lock(-1);
    saveCallback = save;
    backCallback = back;
    refreshCallback = refresh;
    diagnosticsCallback = diagnostics;
    wifiConnectCallback = wifiConnect;
    discoverCallback = discover;
    serverConnectCallback = serverConnect;
    screen = lv_obj_create(nullptr);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x080C10), 0);

    const UiLayout layout(profile);
    form = lv_obj_create(screen);
    // The form owns the full physical display. Its controls can now be laid
    // out through UiLayout by device-specific profiles.
    lv_obj_set_size(form, profile.width, profile.height);
    lv_obj_center(form);
    lv_obj_clear_flag(form, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(form, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(form, 0, 0);
    lv_obj_set_style_pad_all(form, 0, 0);

    networkPicker = lv_obj_create(screen);
    lv_obj_set_size(networkPicker, profile.width, profile.height);
    lv_obj_center(networkPicker);
    lv_obj_clear_flag(networkPicker, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(networkPicker, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(networkPicker, 0, 0);
    lv_obj_set_style_pad_all(networkPicker, 0, 0);

    lv_obj_t *networkTitle = lv_label_create(networkPicker);
    lv_label_set_text(networkTitle, "WI-FI NETWORKS");
    lv_obj_set_style_text_font(networkTitle, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(networkTitle, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(networkTitle, LV_ALIGN_TOP_MID, 0, layout.y(40));

    networkStatusLabel = lv_label_create(networkPicker);
    lv_obj_set_width(networkStatusLabel, layout.width(340));
    lv_obj_set_style_text_align(networkStatusLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(networkStatusLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(networkStatusLabel, lv_color_hex(0x8899AA), 0);
    lv_obj_align(networkStatusLabel, LV_ALIGN_TOP_MID, 0, layout.y(78));

    networkRoller = lv_roller_create(networkPicker);
    lv_obj_set_width(networkRoller, layout.width(340));
    lv_obj_set_style_text_font(networkRoller, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_bg_color(networkRoller, lv_color_hex(0x101820), LV_PART_MAIN);
    lv_obj_set_style_text_color(networkRoller, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_border_width(networkRoller, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(networkRoller, lv_color_hex(0x266A91), LV_PART_SELECTED);
    lv_obj_set_style_text_color(networkRoller, lv_color_hex(0xFFFFFF), LV_PART_SELECTED);
    lv_obj_set_style_border_width(networkRoller, 0, LV_PART_SELECTED);
    lv_obj_set_style_radius(networkRoller, 14, LV_PART_SELECTED);
    lv_obj_set_style_text_opa(networkRoller, LV_OPA_TRANSP, LV_PART_SELECTED);
    lv_roller_set_visible_row_count(networkRoller, 5);
    lv_obj_align(networkRoller, LV_ALIGN_CENTER, 0, layout.y(-5));
    lv_obj_add_event_cb(networkRoller, networkRollerEvent, LV_EVENT_VALUE_CHANGED, this);

    networkSelectedLabel = lv_label_create(networkPicker);
    lv_obj_set_width(networkSelectedLabel, layout.width(320));
    lv_label_set_long_mode(networkSelectedLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(networkSelectedLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(networkSelectedLabel, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(networkSelectedLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(networkSelectedLabel, LV_ALIGN_CENTER, 0, layout.y(-5));

    networkBackButton = makeWideButton(networkPicker, layout, "Back", 118, 42, -70, 348,
        networkBackEvent, this);
    networkSelectButton = makeWideButton(networkPicker, layout, "Select", 118, 42, 70, 348,
        networkSelectEvent, this);
    networkScanButton = makeWideButton(networkPicker, layout, "Scan again", 112, 34, -64, 405,
        networkScanEvent, this);
    manualSetupButton = makeWideButton(networkPicker, layout, "Manual", 112, 34, 64, 405,
        manualSetupEvent, this);

    serverPicker = lv_obj_create(screen);
    lv_obj_set_size(serverPicker, profile.width, profile.height);
    lv_obj_center(serverPicker);
    lv_obj_clear_flag(serverPicker, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(serverPicker, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(serverPicker, 0, 0);
    lv_obj_set_style_pad_all(serverPicker, 0, 0);

    lv_obj_t *serverTitle = lv_label_create(serverPicker);
    lv_label_set_text(serverTitle, "DCC-EX STATIONS");
    lv_obj_set_style_text_font(serverTitle, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(serverTitle, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(serverTitle, LV_ALIGN_TOP_MID, 0, layout.y(40));

    serverStatusLabel = lv_label_create(serverPicker);
    lv_obj_set_width(serverStatusLabel, layout.width(340));
    lv_obj_set_style_text_align(serverStatusLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(serverStatusLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(serverStatusLabel, lv_color_hex(0x8899AA), 0);
    lv_obj_align(serverStatusLabel, LV_ALIGN_TOP_MID, 0, layout.y(78));

    serverRoller = lv_roller_create(serverPicker);
    lv_obj_set_width(serverRoller, layout.width(340));
    lv_obj_set_style_text_font(serverRoller, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_bg_color(serverRoller, lv_color_hex(0x101820), LV_PART_MAIN);
    lv_obj_set_style_text_color(serverRoller, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_border_width(serverRoller, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(serverRoller, lv_color_hex(0x266A91), LV_PART_SELECTED);
    lv_obj_set_style_text_color(serverRoller, lv_color_hex(0xFFFFFF), LV_PART_SELECTED);
    lv_obj_set_style_border_width(serverRoller, 0, LV_PART_SELECTED);
    lv_obj_set_style_radius(serverRoller, 14, LV_PART_SELECTED);
    lv_obj_set_style_text_opa(serverRoller, LV_OPA_TRANSP, LV_PART_SELECTED);
    lv_roller_set_visible_row_count(serverRoller, 5);
    lv_obj_align(serverRoller, LV_ALIGN_CENTER, 0, layout.y(-5));
    lv_obj_add_event_cb(serverRoller, serverRollerEvent, LV_EVENT_VALUE_CHANGED, this);

    serverSelectedLabel = lv_label_create(serverPicker);
    lv_obj_set_width(serverSelectedLabel, layout.width(320));
    lv_label_set_long_mode(serverSelectedLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(serverSelectedLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(serverSelectedLabel, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(serverSelectedLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(serverSelectedLabel, LV_ALIGN_CENTER, 0, layout.y(-5));

    serverBackButton = makeWideButton(serverPicker, layout, "Back", 118, 42, -70, 348,
        serverBackEvent, this);
    serverSelectButton = makeWideButton(serverPicker, layout, "Select", 118, 42, 70, 348,
        serverSelectEvent, this);
    serverScanButton = makeWideButton(serverPicker, layout, "Scan again", 112, 34, -64, 405,
        serverScanEvent, this);
    serverManualButton = makeWideButton(serverPicker, layout, "Manual", 112, 34, 64, 405,
        serverManualEvent, this);
    lv_obj_add_flag(serverPicker, LV_OBJ_FLAG_HIDDEN);

    formTitle = lv_label_create(form);
    lv_label_set_text(formTitle, "CONNECTION");
    lv_obj_set_style_text_font(formTitle, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(formTitle, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(formTitle, LV_ALIGN_TOP_MID, 0, layout.y(20));

    ssidField = makeField(form, layout, "Wi-Fi network", 64, &ssidLabel);
    passwordField = makeField(form, layout, "Wi-Fi password", 124, &passwordLabel);
    serverField = makeField(form, layout, "DCC-EX IP address", 184, &serverLabel);
    portField = makeField(form, layout, "DCC-EX port", 244, &portLabel);
    lv_textarea_set_password_mode(passwordField, true);
    lv_textarea_set_accepted_chars(portField, "0123456789");
    lv_textarea_set_max_length(portField, 5);

    for (lv_obj_t *field : {ssidField, passwordField, serverField, portField})
        lv_obj_add_event_cb(field, fieldEvent, LV_EVENT_FOCUSED, this);

    statusLabel = lv_label_create(form);
    lv_obj_set_width(statusLabel, layout.width(420));
    lv_obj_set_style_text_align(statusLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x8899AA), 0);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_MID, 0, layout.y(310));

    backButton = makeButton(form, layout, "Back", -90, 350, backEvent, this);
    refreshButton = makeButton(form, layout, "Refresh", 0, 350, refreshEvent, this);
    diagnosticsButton = makeButton(form, layout, "DIAG", 90, 350, diagnosticsEvent, this);
    saveButton = makeButton(form, layout, "Save", 0, 400, saveEvent, this);
    wifiConnectButton = makeWideButton(form, layout, "Connect", 118, 42, 70, 348,
        wifiConnectEvent, this);
    lv_obj_add_flag(wifiConnectButton, LV_OBJ_FLAG_HIDDEN);

    editorLabel = lv_label_create(screen);
    lv_label_set_text(editorLabel, "EDIT CONNECTION DETAIL");
    lv_obj_set_style_text_font(editorLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(editorLabel, lv_color_hex(0x8899AA), 0);
    lv_obj_align(editorLabel, LV_ALIGN_TOP_MID, 0, layout.y(54));
    lv_obj_add_flag(editorLabel, LV_OBJ_FLAG_HIDDEN);

    editorField = lv_textarea_create(screen);
    lv_obj_set_size(editorField, layout.width(360), layout.height(42));
    lv_obj_align(editorField, LV_ALIGN_TOP_MID, 0, layout.y(84));
    lv_textarea_set_one_line(editorField, true);
    lv_obj_set_style_bg_color(editorField, lv_color_hex(0x17232D), LV_PART_MAIN);
    lv_obj_set_style_text_color(editorField, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(editorField, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_add_flag(editorField, LV_OBJ_FLAG_HIDDEN);

    keyboard = lv_keyboard_create(screen);
    // Keep the keyboard inside the usable area of the round 480 px display.
    lv_obj_set_size(keyboard, layout.width(360), layout.height(170));
    lv_obj_align(keyboard, LV_ALIGN_CENTER, 0, layout.y(50));
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(keyboard, keyboardEvent, LV_EVENT_ALL, this);
    lvgl_port_unlock();
}

void ConnectionUI::show(const ConnectionSettings &settings, bool requireConfiguration)
{
    lvgl_port_lock(-1);
    if (!visible)
        returnScreen = lv_scr_act();
    lv_textarea_set_text(ssidField, settings.wifiSsid.c_str());
    lv_textarea_set_text(passwordField, settings.wifiPassword.c_str());
    lv_textarea_set_text(serverField, settings.serverAddress.c_str());
    lv_textarea_set_text(portField, String(settings.serverPort).c_str());
    this->requireConfiguration = requireConfiguration;
    visible = true;
    lv_scr_load(screen);
    showNetworkPicker();
    lvgl_port_unlock();
}

void ConnectionUI::hide()
{
    finishEditing(false);
    if (visible && returnScreen)
        lv_scr_load(returnScreen);
    visible = false;
    wifiConnectRequested = false;
    serverPickerVisible = false;
}

void ConnectionUI::setStatus(const char *text, bool error)
{
    lv_obj_t *label = serverPickerVisible ? serverStatusLabel : statusLabel;
    if (!label)
        return;
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(error ? 0xFF7043 : 0x8899AA), 0);
}

void ConnectionUI::fieldEvent(lv_event_t *event)
{
    auto *ui = static_cast<ConnectionUI *>(lv_event_get_user_data(event));
    ui->activeField = lv_event_get_target(event);
    lv_textarea_set_text(ui->editorField, lv_textarea_get_text(ui->activeField));
    lv_textarea_set_password_mode(ui->editorField, ui->activeField == ui->passwordField);
    if (ui->activeField == ui->portField)
    {
        lv_textarea_set_accepted_chars(ui->editorField, "0123456789");
        lv_textarea_set_max_length(ui->editorField, 5);
    }
    else
    {
        lv_textarea_set_accepted_chars(ui->editorField, nullptr);
        lv_textarea_set_max_length(ui->editorField, 0);
    }
    lv_obj_add_flag(ui->form, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui->editorLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui->editorField, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(ui->keyboard, ui->editorField);
    lv_obj_clear_flag(ui->keyboard, LV_OBJ_FLAG_HIDDEN);
}

void ConnectionUI::keyboardEvent(lv_event_t *event)
{
    const lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL)
    {
        auto *ui = static_cast<ConnectionUI *>(lv_event_get_user_data(event));
        ui->finishEditing(code == LV_EVENT_READY);
    }
}

void ConnectionUI::finishEditing(bool saveValue)
{
    if (saveValue && activeField)
        lv_textarea_set_text(activeField, lv_textarea_get_text(editorField));
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(editorLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(editorField, LV_OBJ_FLAG_HIDDEN);
    if (form)
        lv_obj_clear_flag(form, LV_OBJ_FLAG_HIDDEN);
    activeField = nullptr;
}

void ConnectionUI::saveEvent(lv_event_t *event)
{
    static_cast<ConnectionUI *>(lv_event_get_user_data(event))->save();
}

void ConnectionUI::backEvent(lv_event_t *event)
{
    auto *ui = static_cast<ConnectionUI *>(lv_event_get_user_data(event));
    ui->showNetworkPicker();
}

void ConnectionUI::refreshEvent(lv_event_t *event)
{
    auto *ui = static_cast<ConnectionUI *>(lv_event_get_user_data(event));
    if (ui->refreshCallback)
        ui->refreshCallback();
}
void ConnectionUI::diagnosticsEvent(lv_event_t *event) { auto *ui=static_cast<ConnectionUI*>(lv_event_get_user_data(event)); if(ui->diagnosticsCallback) ui->diagnosticsCallback(); }

void ConnectionUI::save()
{
    ConnectionSettings settings;
    settings.wifiSsid = lv_textarea_get_text(ssidField);
    settings.wifiPassword = lv_textarea_get_text(passwordField);
    settings.serverAddress = lv_textarea_get_text(serverField);
    const long port = strtol(lv_textarea_get_text(portField), nullptr, 10);
    if (!settings.wifiSsid.length() || !settings.serverAddress.length() || port < 1 || port > 65535)
    {
        setStatus("Enter a Wi-Fi network, IP address, and port", true);
        return;
    }
    settings.serverPort = static_cast<uint16_t>(port);
    if (saveCallback)
        saveCallback(settings);
}

void ConnectionUI::showNetworkPicker()
{
    wifiCredentialsMode = false;
    wifiConnectRequested = false;
    serverPickerVisible = false;
    manualServerMode = false;
    lv_obj_add_flag(form, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(serverPicker, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(editorLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(editorField, LV_OBJ_FLAG_HIDDEN);
    activeField = nullptr;
    if (requireConfiguration)
        lv_obj_add_flag(networkBackButton, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_clear_flag(networkBackButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(networkPicker, LV_OBJ_FLAG_HIDDEN);
    scanNetworks();
}

void ConnectionUI::showManualForm()
{
    wifiCredentialsMode = false;
    wifiConnectRequested = false;
    serverPickerVisible = false;
    manualServerMode = false;
    lv_obj_add_flag(networkPicker, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(serverPicker, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(form, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(formTitle, "CONNECTION");
    for (lv_obj_t *object : {ssidLabel, ssidField, passwordLabel, passwordField})
        lv_obj_clear_flag(object, LV_OBJ_FLAG_HIDDEN);
    const UiLayout layout(profile);
    lv_obj_align(serverLabel, LV_ALIGN_TOP_MID, 0, layout.y(184));
    lv_obj_align(serverField, LV_ALIGN_TOP_MID, 0, layout.y(202));
    lv_obj_align(portLabel, LV_ALIGN_TOP_MID, 0, layout.y(244));
    lv_obj_align(portField, LV_ALIGN_TOP_MID, 0, layout.y(262));
    for (lv_obj_t *object : {serverLabel, serverField, portLabel, portField, refreshButton,
                             diagnosticsButton, saveButton})
        lv_obj_clear_flag(object, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(wifiConnectButton, LV_OBJ_FLAG_HIDDEN);
    if (requireConfiguration)
    {
        lv_obj_add_flag(backButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(refreshButton, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_clear_flag(backButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(refreshButton, LV_OBJ_FLAG_HIDDEN);
    }
    setStatus("Enter Wi-Fi and DCC-EX connection details");
}

void ConnectionUI::showWifiCredentialsForm()
{
    wifiCredentialsMode = true;
    manualServerMode = false;
    lv_obj_add_flag(networkPicker, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(serverPicker, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(form, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(formTitle, "WI-FI DETAILS");
    for (lv_obj_t *object : {ssidLabel, ssidField, passwordLabel, passwordField})
        lv_obj_clear_flag(object, LV_OBJ_FLAG_HIDDEN);
    for (lv_obj_t *object : {serverLabel, serverField, portLabel, portField, refreshButton,
                             diagnosticsButton, saveButton})
        lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(backButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(wifiConnectButton, LV_OBJ_FLAG_HIDDEN);
    setStatus("Enter the password, then connect to Wi-Fi");
}

void ConnectionUI::connectWifi()
{
    const String ssid = lv_textarea_get_text(ssidField);
    if (ssid.isEmpty())
    {
        setStatus("Choose or enter a Wi-Fi network", true);
        return;
    }
    if (wifiConnectCallback)
    {
        wifiConnectRequested = true;
        wifiConnectCallback(ssid, lv_textarea_get_text(passwordField));
    }
}

void ConnectionUI::showManualServerForm()
{
    wifiCredentialsMode = false;
    wifiConnectRequested = false;
    serverPickerVisible = false;
    manualServerMode = true;
    lv_obj_add_flag(networkPicker, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(serverPicker, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(form, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(formTitle, "MANUAL DCC-EX");
    for (lv_obj_t *object : {ssidLabel, ssidField, passwordLabel, passwordField,
                             refreshButton, diagnosticsButton, saveButton})
        lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
    for (lv_obj_t *object : {serverLabel, serverField, portLabel, portField})
        lv_obj_clear_flag(object, LV_OBJ_FLAG_HIDDEN);
    const UiLayout layout(profile);
    lv_obj_align(serverLabel, LV_ALIGN_TOP_MID, 0, layout.y(110));
    lv_obj_align(serverField, LV_ALIGN_TOP_MID, 0, layout.y(128));
    lv_obj_align(portLabel, LV_ALIGN_TOP_MID, 0, layout.y(190));
    lv_obj_align(portField, LV_ALIGN_TOP_MID, 0, layout.y(208));
    lv_obj_clear_flag(backButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(wifiConnectButton, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(lv_obj_get_child(wifiConnectButton, 0), "Connect");
    setStatus("Enter the DCC-EX address and port");
}

void ConnectionUI::connectManualServer()
{
    const String address = lv_textarea_get_text(serverField);
    const long port = strtol(lv_textarea_get_text(portField), nullptr, 10);
    if (address.isEmpty() || port < 1 || port > 65535)
    {
        setStatus("Enter a DCC-EX IP address and port", true);
        return;
    }
    if (serverConnectCallback)
        serverConnectCallback(address, static_cast<uint16_t>(port));
}

void ConnectionUI::showCommandStationPicker(const std::vector<CommandStationInfo> &stations,
    bool discoveryAvailable)
{
    if (!visible)
        return;
    wifiCredentialsMode = false;
    wifiConnectRequested = false;
    serverPickerVisible = true;
    lv_obj_add_flag(form, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(networkPicker, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(editorLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(editorField, LV_OBJ_FLAG_HIDDEN);
    activeField = nullptr;
    lv_obj_clear_flag(serverPicker, LV_OBJ_FLAG_HIDDEN);
    commandStations = stations;
    lv_roller_set_options(serverRoller, "", LV_ROLLER_MODE_NORMAL);
    lv_obj_add_state(serverSelectButton, LV_STATE_DISABLED);
    if (!discoveryAvailable)
    {
        lv_label_set_text(serverStatusLabel, "Discovery unavailable - use Manual");
        lv_label_set_text(serverSelectedLabel, "");
        return;
    }
    if (commandStations.empty())
    {
        lv_label_set_text(serverStatusLabel, "No DCC-EX stations found - use Manual");
        lv_label_set_text(serverSelectedLabel, "");
        return;
    }
    String options;
    for (size_t index = 0; index < commandStations.size(); ++index)
    {
        if (index)
            options += "\n";
        const CommandStationInfo &station = commandStations[index];
        options += station.hostname.length() ? station.hostname : station.address;
    }
    lv_roller_set_options(serverRoller, options.c_str(), LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(serverRoller, 0, LV_ANIM_OFF);
    lv_obj_clear_state(serverSelectButton, LV_STATE_DISABLED);
    lv_label_set_text(serverStatusLabel, "Choose a Command Station, then select");
    updateCommandStationSelection();
}

void ConnectionUI::scanNetworks()
{
    lv_label_set_text(networkStatusLabel, "Scanning for Wi-Fi networks...");
    lv_roller_set_options(networkRoller, "", LV_ROLLER_MODE_NORMAL);
    lv_obj_add_state(networkSelectButton, LV_STATE_DISABLED);
    networkNames.clear();

    WiFi.mode(WIFI_STA);
    WiFi.scanDelete();
    WiFi.scanNetworks(true, true);
    networkScanInProgress = true;
}

void ConnectionUI::update()
{
    if (!networkScanInProgress)
        return;
    const int count = WiFi.scanComplete();
    if (count == WIFI_SCAN_RUNNING)
        return;
    networkScanInProgress = false;
    finishNetworkScan(count);
}

void ConnectionUI::finishNetworkScan(int count)
{
    for (int index = 0; index < count; ++index)
    {
        String name = WiFi.SSID(index);
        name.trim();
        if (name.isEmpty())
            continue;
        bool alreadyListed = false;
        for (const String &knownName : networkNames)
        {
            if (knownName == name)
            {
                alreadyListed = true;
                break;
            }
        }
        if (!alreadyListed)
            networkNames.push_back(name);
    }
    WiFi.scanDelete();

    if (networkNames.empty())
    {
        lv_label_set_text(networkStatusLabel, "No networks found - scan again or use Manual");
        lv_label_set_text(networkSelectedLabel, "");
        return;
    }

    String options;
    uint16_t selected = 0;
    for (size_t index = 0; index < networkNames.size(); ++index)
    {
        if (index)
            options += "\n";
        options += networkNames[index];
        if (networkNames[index] == lv_textarea_get_text(ssidField))
            selected = index;
    }
    lv_roller_set_options(networkRoller, options.c_str(), LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(networkRoller, selected, LV_ANIM_OFF);
    lv_obj_clear_state(networkSelectButton, LV_STATE_DISABLED);
    lv_label_set_text(networkStatusLabel, "Choose a network, then select");
    updateNetworkSelection();
}

void ConnectionUI::scanCommandStations()
{
    lv_label_set_text(serverStatusLabel, "Searching for DCC-EX stations...");
    lv_roller_set_options(serverRoller, "", LV_ROLLER_MODE_NORMAL);
    lv_obj_add_state(serverSelectButton, LV_STATE_DISABLED);
    commandStations.clear();
    if (!discoverCallback || !discoverCallback(commandStations))
    {
        lv_label_set_text(serverStatusLabel, "Discovery unavailable - use Manual");
        lv_label_set_text(serverSelectedLabel, "");
        return;
    }
    if (commandStations.empty())
    {
        lv_label_set_text(serverStatusLabel, "No DCC-EX stations found - use Manual");
        lv_label_set_text(serverSelectedLabel, "");
        return;
    }

    String options;
    for (size_t index = 0; index < commandStations.size(); ++index)
    {
        if (index)
            options += "\n";
        const CommandStationInfo &station = commandStations[index];
        options += station.hostname.length() ? station.hostname : station.address;
    }
    lv_roller_set_options(serverRoller, options.c_str(), LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(serverRoller, 0, LV_ANIM_OFF);
    lv_obj_clear_state(serverSelectButton, LV_STATE_DISABLED);
    lv_label_set_text(serverStatusLabel, "Choose a Command Station, then select");
    updateCommandStationSelection();
}

void ConnectionUI::selectCommandStation()
{
    const uint16_t selected = lv_roller_get_selected(serverRoller);
    if (selected >= commandStations.size() || !serverConnectCallback)
        return;
    const CommandStationInfo &station = commandStations[selected];
    serverConnectCallback(station.address, station.port);
}

void ConnectionUI::updateCommandStationSelection()
{
    const uint16_t selected = lv_roller_get_selected(serverRoller);
    if (selected >= commandStations.size())
        return;
    const CommandStationInfo &station = commandStations[selected];
    String detail = station.hostname.length() ? station.hostname + "\n" : "";
    detail += station.address;
    detail += ":";
    detail += station.port;
    lv_label_set_text(serverSelectedLabel, detail.c_str());
}

void ConnectionUI::selectNetwork()
{
    const uint16_t selected = lv_roller_get_selected(networkRoller);
    if (selected >= networkNames.size())
        return;
    lv_textarea_set_text(ssidField, networkNames[selected].c_str());
    showWifiCredentialsForm();
}

void ConnectionUI::updateNetworkSelection()
{
    const uint16_t selected = lv_roller_get_selected(networkRoller);
    if (selected < networkNames.size())
        lv_label_set_text(networkSelectedLabel, networkNames[selected].c_str());
}

void ConnectionUI::networkSelectEvent(lv_event_t *event)
{
    static_cast<ConnectionUI *>(lv_event_get_user_data(event))->selectNetwork();
}

void ConnectionUI::networkScanEvent(lv_event_t *event)
{
    static_cast<ConnectionUI *>(lv_event_get_user_data(event))->scanNetworks();
}

void ConnectionUI::manualSetupEvent(lv_event_t *event)
{
    static_cast<ConnectionUI *>(lv_event_get_user_data(event))->showManualForm();
}

void ConnectionUI::networkBackEvent(lv_event_t *event)
{
    auto *ui = static_cast<ConnectionUI *>(lv_event_get_user_data(event));
    if (ui->backCallback)
        ui->backCallback();
}

void ConnectionUI::networkRollerEvent(lv_event_t *event)
{
    static_cast<ConnectionUI *>(lv_event_get_user_data(event))->updateNetworkSelection();
}

void ConnectionUI::wifiConnectEvent(lv_event_t *event)
{
    auto *ui = static_cast<ConnectionUI *>(lv_event_get_user_data(event));
    if (ui->manualServerMode)
        ui->connectManualServer();
    else
        ui->connectWifi();
}

void ConnectionUI::serverSelectEvent(lv_event_t *event)
{
    static_cast<ConnectionUI *>(lv_event_get_user_data(event))->selectCommandStation();
}

void ConnectionUI::serverScanEvent(lv_event_t *event)
{
    static_cast<ConnectionUI *>(lv_event_get_user_data(event))->scanCommandStations();
}

void ConnectionUI::serverManualEvent(lv_event_t *event)
{
    static_cast<ConnectionUI *>(lv_event_get_user_data(event))->showManualServerForm();
}

void ConnectionUI::serverBackEvent(lv_event_t *event)
{
    static_cast<ConnectionUI *>(lv_event_get_user_data(event))->showWifiCredentialsForm();
}

void ConnectionUI::serverRollerEvent(lv_event_t *event)
{
    static_cast<ConnectionUI *>(lv_event_get_user_data(event))->updateCommandStationSelection();
}
