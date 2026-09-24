#include "ConnectionUI.h"
#include "lvgl_v8_port.h"
#include "UiLayout.h"

namespace
{
lv_obj_t *makeField(lv_obj_t *parent, const UiLayout &layout, const char *title, int y)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x8899AA), 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, layout.y(y));

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
}

void ConnectionUI::begin(SaveCallback save, BackCallback back, RefreshCallback refresh, DiagnosticsCallback diagnostics)
{
    lvgl_port_lock(-1);
    saveCallback = save;
    backCallback = back;
    refreshCallback = refresh;
    diagnosticsCallback = diagnostics;
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

    lv_obj_t *title = lv_label_create(form);
    lv_label_set_text(title, "CONNECTION");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, layout.y(20));

    ssidField = makeField(form, layout, "Wi-Fi network", 64);
    passwordField = makeField(form, layout, "Wi-Fi password", 124);
    serverField = makeField(form, layout, "DCC-EX IP address", 184);
    portField = makeField(form, layout, "DCC-EX port", 244);
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
    makeButton(form, layout, "DIAG", 90, 350, diagnosticsEvent, this);
    makeButton(form, layout, "Save", 0, 400, saveEvent, this);

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
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(editorLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(editorField, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(form, LV_OBJ_FLAG_HIDDEN);
    activeField = nullptr;
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
    setStatus(requireConfiguration ? "Enter Wi-Fi and DCC-EX connection details" : "Edit settings, then save and reconnect");
    visible = true;
    lv_scr_load(screen);
    lvgl_port_unlock();
}

void ConnectionUI::hide()
{
    finishEditing(false);
    if (visible && returnScreen)
        lv_scr_load(returnScreen);
    visible = false;
}

void ConnectionUI::setStatus(const char *text, bool error)
{
    if (!statusLabel)
        return;
    lv_label_set_text(statusLabel, text);
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(error ? 0xFF7043 : 0x8899AA), 0);
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
    if (ui->backCallback)
        ui->backCallback();
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
