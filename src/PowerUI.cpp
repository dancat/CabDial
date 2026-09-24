#include "PowerUI.h"
#include "lvgl_v8_port.h"
#include "UiLayout.h"

namespace
{
lv_obj_t *makeButton(lv_obj_t *parent, const UiLayout &layout, const char *text, int width, int height,
    int y, lv_color_t color, lv_event_cb_t callback, void *user)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_size(button, layout.width(width), layout.height(height));
    lv_obj_align(button, LV_ALIGN_TOP_MID, 0, layout.y(y));
    lv_obj_set_style_bg_color(button, color, 0);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_center(label);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, user);
    return button;
}
}

void PowerUI::begin(PowerCallback setPower, BackCallback back)
{
    lvgl_port_lock(-1);
    powerCallback = setPower;
    backCallback = back;
    const UiLayout layout(profile);
    screen = lv_obj_create(nullptr);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x080C10), 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "POWER");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, layout.y(34));

    powerStatusLabel = lv_label_create(screen);
    lv_obj_set_width(powerStatusLabel, layout.width(380));
    lv_obj_set_style_text_align(powerStatusLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(powerStatusLabel, &lv_font_montserrat_16, 0);
    lv_obj_align(powerStatusLabel, LV_ALIGN_TOP_MID, 0, layout.y(110));

    powerButton = makeButton(screen, layout, "", 280, 56, 172, lv_color_hex(0x263746), powerEvent, this);
    powerButtonLabel = lv_obj_get_child(powerButton, 0);
    makeButton(screen, layout, "Back", 120, 42, 300, lv_color_hex(0x263746), backEvent, this);
    updatePowerControls();
    lvgl_port_unlock();
}

void PowerUI::show()
{
    lvgl_port_lock(-1);
    if (!visible)
        returnScreen = lv_scr_act();
    confirmingPowerOff = false;
    updatePowerControls();
    visible = true;
    lv_scr_load(screen);
    lvgl_port_unlock();
}

void PowerUI::hide()
{
    if (visible && returnScreen)
        lv_scr_load(returnScreen);
    visible = false;
}

void PowerUI::setTrackPower(bool known, bool on)
{
    powerKnown = known;
    powerOn = on;
    confirmingPowerOff = false;
    updatePowerControls();
}

void PowerUI::updatePowerControls()
{
    if (!powerStatusLabel || !powerButton || !powerButtonLabel)
        return;
    if (!powerKnown)
    {
        lv_label_set_text(powerStatusLabel, "TRACK POWER: UNKNOWN");
        lv_label_set_text(powerButtonLabel, "POWER ON");
        lv_obj_set_style_bg_color(powerButton, lv_color_hex(0x1B5E3A), 0);
    }
    else if (powerOn)
    {
        lv_label_set_text(powerStatusLabel, "TRACK POWER: ON");
        lv_label_set_text(powerButtonLabel,
            confirmingPowerOff ? "CONFIRM POWER OFF" : "POWER OFF");
        lv_obj_set_style_bg_color(powerButton, lv_color_hex(0xB71C1C), 0);
    }
    else
    {
        lv_label_set_text(powerStatusLabel, "TRACK POWER: OFF");
        lv_label_set_text(powerButtonLabel, "POWER ON");
        lv_obj_set_style_bg_color(powerButton, lv_color_hex(0x1B5E3A), 0);
    }
    lv_obj_center(powerButtonLabel);
}

void PowerUI::changePower()
{
    if (powerOn && !confirmingPowerOff)
    {
        confirmingPowerOff = true;
        updatePowerControls();
        return;
    }
    if (powerCallback)
        powerCallback(!powerOn);
}

void PowerUI::powerEvent(lv_event_t *event)
{
    static_cast<PowerUI *>(lv_event_get_user_data(event))->changePower();
}

void PowerUI::backEvent(lv_event_t *event)
{
    auto *ui = static_cast<PowerUI *>(lv_event_get_user_data(event));
    if (ui->backCallback)
        ui->backCallback();
}
