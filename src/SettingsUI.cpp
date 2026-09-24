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

void SettingsUI::begin(BrightnessCallback brightness, SleepCallback sleep, BackCallback back)
{
    lvgl_port_lock(-1);
    brightnessCallback = brightness;
    sleepCallback = sleep;
    backCallback = back;
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

    lv_obj_t *hint = lv_label_create(screen);
    lv_label_set_text(hint, "Turns the backlight off after inactivity");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x8899AA), 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, layout.y(342));

    lv_obj_t *backButton = lv_btn_create(screen);
    lv_obj_set_size(backButton, layout.width(130), layout.height(44));
    lv_obj_align(backButton, LV_ALIGN_BOTTOM_MID, 0, -layout.y(22));
    lv_obj_set_style_bg_color(backButton, lv_color_hex(0x263746), 0);
    lv_obj_t *backLabel = lv_label_create(backButton);
    lv_label_set_text(backLabel, "Back");
    lv_obj_center(backLabel);
    lv_obj_add_event_cb(backButton, backEvent, LV_EVENT_CLICKED, this);
    lvgl_port_unlock();
}

void SettingsUI::show(uint8_t brightness, uint16_t sleepSeconds)
{
    lvgl_port_lock(-1);
    if (!visible)
        returnScreen = lv_scr_act();
    lv_slider_set_value(brightnessSlider, brightness, LV_ANIM_OFF);
    updateBrightnessValue(brightness);
    lv_roller_set_selected(sleepRoller, selectionForSleepSeconds(sleepSeconds), LV_ANIM_OFF);
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
