#include "FunctionUI.h"
#include "UiLayout.h"
#include "lvgl_v8_port.h"

void FunctionUI::begin(FunctionCallback function, BackCallback back)
{
    lvgl_port_lock(-1);
    functionCallback = function; backCallback = back;
    const UiLayout layout(profile);
    screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x080C10), 0);
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "FUNCTIONS");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, layout.y(24));
    for (uint8_t i = 0; i < MAX_BUTTONS; ++i) {
        buttons[i] = lv_btn_create(screen);
        lv_obj_set_size(buttons[i], layout.width(82), layout.height(38));
        lv_obj_align(buttons[i], LV_ALIGN_TOP_LEFT, layout.x(54 + (i % 4) * 92), layout.y(70 + (i / 4) * 42));
        lv_obj_set_style_bg_color(buttons[i], lv_color_hex(0x263746), 0);
        lv_obj_set_style_pad_all(buttons[i], 0, 0);
        labels[i] = lv_label_create(buttons[i]); lv_obj_center(labels[i]);
        lv_obj_add_event_cb(buttons[i], functionEvent, LV_EVENT_ALL, this);
    }
    lv_obj_t *backButton = lv_btn_create(screen);
    lv_obj_set_size(backButton, layout.width(110), layout.height(36));
    lv_obj_align(backButton, LV_ALIGN_BOTTOM_MID, 0, -layout.y(14));
    lv_obj_t *backLabel = lv_label_create(backButton); lv_label_set_text(backLabel, "Back"); lv_obj_center(backLabel);
    lv_obj_add_event_cb(backButton, backEvent, LV_EVENT_CLICKED, this);
    lvgl_port_unlock();
}

void FunctionUI::show(const Locomotive &value)
{
    lvgl_port_lock(-1); locomotive = &value; count = 0;
    for (uint8_t function = 0; function < MAX_BUTTONS; ++function)
        if (!value.fromRoster || value.functionDefinitions[function].available) functions[count++] = function;
    for (uint8_t i = 0; i < MAX_BUTTONS; ++i) {
        if (i < count) { lv_label_set_text_fmt(labels[i], "F%u", functions[i]); lv_obj_clear_flag(buttons[i], LV_OBJ_FLAG_HIDDEN); }
        else lv_obj_add_flag(buttons[i], LV_OBJ_FLAG_HIDDEN);
    }
    updateAppearance(); if (!visible) returnScreen = lv_scr_act(); visible = true; lv_scr_load(screen); lvgl_port_unlock();
}
void FunctionUI::hide() { if (visible && returnScreen) lv_scr_load(returnScreen); visible = false; }
void FunctionUI::updateAppearance() { for (uint8_t i=0;i<count;++i) lv_obj_set_style_bg_color(buttons[i], locomotive->functionStates[functions[i]] ? lv_color_hex(0xFFD740) : lv_color_hex(0x263746), 0); }
void FunctionUI::functionEvent(lv_event_t *event) { auto *ui=static_cast<FunctionUI*>(lv_event_get_user_data(event)); const lv_event_code_t code=lv_event_get_code(event); for(uint8_t i=0;i<ui->count;++i) if(lv_event_get_target(event)==ui->buttons[i] && code==LV_EVENT_CLICKED && ui->functionCallback) { ui->functionCallback(ui->functions[i],true); ui->updateAppearance(); return; } }
void FunctionUI::backEvent(lv_event_t *event) { auto *ui=static_cast<FunctionUI*>(lv_event_get_user_data(event)); if(ui->backCallback) ui->backCallback(); }
