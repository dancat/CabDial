#include "SettingsUI.h"
#include "lvgl_v8_port.h"
#include "UiLayout.h"

namespace
{
constexpr uint16_t kSleepOptions[] = {0, 30, 60, 120, 300};

uint16_t sleepSecondsForSelection(uint16_t selection)
{
    return selection < sizeof(kSleepOptions) / sizeof(kSleepOptions[0])
        ? kSleepOptions[selection] : 0;
}

uint16_t selectionForSleepSeconds(uint16_t seconds)
{
    for (uint16_t index = 0; index < sizeof(kSleepOptions) / sizeof(kSleepOptions[0]); ++index)
        if (kSleepOptions[index] == seconds)
            return index;
    return 0;
}
}

void SettingsUI::begin(BrightnessCallback brightness, SleepCallback sleep, BackCallback back, ShortcutCallback shortcut)
{
    lvgl_port_lock(-1);
    brightnessCallback = brightness;
    sleepCallback = sleep;
    backCallback = back;
    shortcutCallback = shortcut;
    const UiLayout layout(profile);
    screen = lv_obj_create(nullptr);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x080C10), 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "SETTINGS");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, layout.y(28));

    brightnessValue = lv_label_create(screen);
    lv_obj_set_style_text_font(brightnessValue, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(brightnessValue, lv_color_hex(0xCCD7E0), 0);
    lv_obj_align(brightnessValue, LV_ALIGN_TOP_LEFT, layout.x(58), layout.y(96));

    brightnessSlider = lv_slider_create(screen);
    lv_obj_set_size(brightnessSlider, layout.width(364), layout.height(18));
    lv_obj_align(brightnessSlider, LV_ALIGN_TOP_MID, 0, layout.y(132));
    lv_slider_set_range(brightnessSlider, 10, 100);
    lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(0x263746), LV_PART_MAIN);
    lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(0x4CAF50), LV_PART_INDICATOR);
    lv_obj_add_event_cb(brightnessSlider, brightnessEvent, LV_EVENT_VALUE_CHANGED, this);

    lv_obj_t *sleepTitle = lv_label_create(screen);
    lv_label_set_text(sleepTitle, "Sleep timeout");
    lv_obj_set_style_text_font(sleepTitle, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(sleepTitle, lv_color_hex(0xCCD7E0), 0);
    lv_obj_align(sleepTitle, LV_ALIGN_TOP_LEFT, layout.x(58), layout.y(188));

    sleepRoller = lv_roller_create(screen);
    lv_roller_set_options(sleepRoller, "Off\n30 seconds\n1 minute\n2 minutes\n5 minutes", LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(sleepRoller, 3);
    lv_obj_set_width(sleepRoller, layout.width(260));
    lv_obj_align(sleepRoller, LV_ALIGN_TOP_MID, 0, layout.y(218));
    lv_obj_set_style_text_font(sleepRoller, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_add_event_cb(sleepRoller, sleepEvent, LV_EVENT_VALUE_CHANGED, this);

    lv_obj_t *shortcutsButton = lv_btn_create(screen);
    lv_obj_set_size(shortcutsButton, layout.width(220), layout.height(42));
    lv_obj_align(shortcutsButton, LV_ALIGN_TOP_MID, 0, layout.y(342));
    lv_obj_t *shortcutsLabel = lv_label_create(shortcutsButton);
    lv_label_set_text(shortcutsLabel, "BUTTON SHORTCUTS");
    lv_obj_center(shortcutsLabel);
    lv_obj_add_event_cb(shortcutsButton, openShortcutsEvent, LV_EVENT_CLICKED, this);

    lv_obj_t *backButton = lv_btn_create(screen);
    lv_obj_set_size(backButton, layout.width(130), layout.height(44));
    lv_obj_align(backButton, LV_ALIGN_BOTTOM_MID, 0, -layout.y(22));
    lv_obj_set_style_bg_color(backButton, lv_color_hex(0x263746), 0);
    lv_obj_t *backLabel = lv_label_create(backButton);
    lv_label_set_text(backLabel, "Back");
    lv_obj_center(backLabel);
    lv_obj_add_event_cb(backButton, backEvent, LV_EVENT_CLICKED, this);

    shortcutScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(shortcutScreen, lv_color_hex(0x080C10), 0);
    lv_obj_t *shortcutTitle = lv_label_create(shortcutScreen);
    lv_label_set_text(shortcutTitle, "BUTTON SHORTCUTS");
    lv_obj_set_style_text_font(shortcutTitle, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(shortcutTitle, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(shortcutTitle, LV_ALIGN_TOP_MID, 0, layout.y(32));
    const char *pressNames[] = {"Single press", "Double press", "Long press"};
    for (uint8_t index = 0; index < 3; ++index) {
        lv_obj_t *label = lv_label_create(shortcutScreen);
        lv_label_set_text(label, pressNames[index]);
        lv_obj_set_style_text_color(label, lv_color_hex(0xCCD7E0), 0);
        lv_obj_align(label, LV_ALIGN_TOP_LEFT, layout.x(62), layout.y(94 + index * 82));
        shortcutSelectors[index] = lv_dropdown_create(shortcutScreen);
        lv_dropdown_set_options(shortcutSelectors[index], "F0\nF1\nF2\nStop\nChange direction\nEmergency stop");
        lv_obj_set_width(shortcutSelectors[index], layout.width(300));
        lv_obj_set_style_bg_color(shortcutSelectors[index], lv_color_hex(0x266A91), LV_PART_MAIN);
        lv_obj_set_style_text_color(shortcutSelectors[index], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_color(shortcutSelectors[index], lv_color_hex(0xFFFFFF), LV_PART_SELECTED);
        lv_obj_align(shortcutSelectors[index], LV_ALIGN_TOP_MID, 0, layout.y(118 + index * 82));
        lv_obj_add_event_cb(shortcutSelectors[index], shortcutEvent, LV_EVENT_VALUE_CHANGED, this);
    }
    lv_obj_t *shortcutBack = lv_btn_create(shortcutScreen);
    lv_obj_set_size(shortcutBack, layout.width(130), layout.height(44));
    lv_obj_align(shortcutBack, LV_ALIGN_BOTTOM_MID, 0, -layout.y(22));
    lv_obj_t *shortcutBackLabel = lv_label_create(shortcutBack);
    lv_label_set_text(shortcutBackLabel, "Back");
    lv_obj_set_style_text_color(shortcutBackLabel, lv_color_hex(0xF2F6FA), 0);
    lv_obj_center(shortcutBackLabel);
    lv_obj_add_event_cb(shortcutBack, backEvent, LV_EVENT_CLICKED, this);
    lvgl_port_unlock();
}

void SettingsUI::show(uint8_t brightness, uint16_t sleepSeconds, HomeShortcutAction single, HomeShortcutAction dbl, HomeShortcutAction lng)
{
    lvgl_port_lock(-1);
    if (!visible)
        returnScreen = lv_scr_act();
    lv_slider_set_value(brightnessSlider, brightness, LV_ANIM_OFF);
    updateBrightnessValue(brightness);
    lv_roller_set_selected(sleepRoller, selectionForSleepSeconds(sleepSeconds), LV_ANIM_OFF);
    shortcuts[0] = single; shortcuts[1] = dbl; shortcuts[2] = lng;
    for (uint8_t index = 0; index < 3; ++index)
        lv_dropdown_set_selected(shortcutSelectors[index], static_cast<uint16_t>(shortcuts[index]));
    visible = true;
    lv_scr_load(screen);
    lvgl_port_unlock();
}

void SettingsUI::hide()
{
    if (visible && returnScreen)
        lv_scr_load(returnScreen);
    visible = false;
}

void SettingsUI::updateBrightnessValue(uint8_t brightness)
{
    lv_label_set_text_fmt(brightnessValue, "Brightness: %u%%", brightness);
}

void SettingsUI::brightnessEvent(lv_event_t *event)
{
    auto *ui = static_cast<SettingsUI *>(lv_event_get_user_data(event));
    const uint8_t brightness = lv_slider_get_value(ui->brightnessSlider);
    ui->updateBrightnessValue(brightness);
    if (ui->brightnessCallback)
        ui->brightnessCallback(brightness);
}

void SettingsUI::sleepEvent(lv_event_t *event)
{
    auto *ui = static_cast<SettingsUI *>(lv_event_get_user_data(event));
    if (ui->sleepCallback)
        ui->sleepCallback(sleepSecondsForSelection(lv_roller_get_selected(ui->sleepRoller)));
}

void SettingsUI::backEvent(lv_event_t *event)
{
    auto *ui = static_cast<SettingsUI *>(lv_event_get_user_data(event));
    if (ui->backCallback)
        ui->backCallback();
}

void SettingsUI::openShortcutsEvent(lv_event_t *event)
{
    auto *ui = static_cast<SettingsUI *>(lv_event_get_user_data(event));
    lv_scr_load(ui->shortcutScreen);
}

void SettingsUI::shortcutEvent(lv_event_t *event)
{
    auto *ui = static_cast<SettingsUI *>(lv_event_get_user_data(event));
    for (uint8_t index = 0; index < 3; ++index) {
        if (lv_event_get_target(event) == ui->shortcutSelectors[index]) {
            ui->shortcuts[index] = static_cast<HomeShortcutAction>(lv_dropdown_get_selected(ui->shortcutSelectors[index]));
            if (ui->shortcutCallback)
                ui->shortcutCallback(index, ui->shortcuts[index]);
            return;
        }
    }
}
