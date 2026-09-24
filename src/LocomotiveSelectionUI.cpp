#include "LocomotiveSelectionUI.h"
#include "lvgl_v8_port.h"
#include "UiLayout.h"

namespace
{
lv_obj_t *makeButton(lv_obj_t *parent, const UiLayout &layout, const char *text, int width,
                     int height, int x, int y, lv_event_cb_t callback, void *user)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_size(button, layout.width(width), layout.height(height));
    lv_obj_align(button, LV_ALIGN_TOP_MID, layout.x(x), layout.y(y));
    lv_obj_set_style_bg_color(button, lv_color_hex(0x263746), 0);
    lv_obj_set_style_border_color(button, lv_color_hex(0x3C566B), 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_radius(button, 14, 0);
    lv_obj_set_style_pad_all(button, 0, 0);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, user);
    return button;
}
}

void LocomotiveSelectionUI::begin(SelectCallback select, BackCallback back,
                                  RefreshCallback refresh)
{
    lvgl_port_lock(-1);
    selectCallback = select;
    backCallback = back;
    refreshCallback = refresh;
    const UiLayout layout(profile);
    screen = lv_obj_create(nullptr);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x080C10), 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "LOCOMOTIVES");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, layout.y(40));

    status = lv_label_create(screen);
    lv_obj_set_width(status, layout.width(340));
    lv_obj_set_style_text_align(status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status, lv_color_hex(0x8899AA), 0);
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, layout.y(78));

    roller = lv_roller_create(screen);
    lv_obj_set_width(roller, layout.width(340));
    lv_obj_set_style_text_font(roller, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_bg_color(roller, lv_color_hex(0x101820), LV_PART_MAIN);
    lv_obj_set_style_text_color(roller, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_border_width(roller, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(roller, lv_color_hex(0x266A91), LV_PART_SELECTED);
    lv_obj_set_style_text_color(roller, lv_color_hex(0xFFFFFF), LV_PART_SELECTED);
    lv_obj_set_style_border_width(roller, 0, LV_PART_SELECTED);
    lv_obj_set_style_radius(roller, 14, LV_PART_SELECTED);
    lv_obj_set_style_text_opa(roller, LV_OPA_TRANSP, LV_PART_SELECTED);
    lv_roller_set_visible_row_count(roller, 5);
    lv_obj_align(roller, LV_ALIGN_CENTER, 0, layout.y(-5));
    lv_obj_add_event_cb(roller, rollerEvent, LV_EVENT_VALUE_CHANGED, this);

    selectedLabel = lv_label_create(screen);
    lv_obj_set_width(selectedLabel, layout.width(320));
    lv_label_set_long_mode(selectedLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(selectedLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(selectedLabel, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(selectedLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(selectedLabel, LV_ALIGN_CENTER, 0, layout.y(-5));

    listBackButton = makeButton(screen, layout, "Back", 118, 42, -70, 348, backEvent, this);
    listSelectButton = makeButton(screen, layout, "Select", 118, 42, 70, 348, selectEvent, this);
    refreshButton = makeButton(screen, layout, "Refresh", 112, 34, 0, 405, refreshEvent, this);

    manualTitle = lv_label_create(screen);
    lv_label_set_text(manualTitle, "MANUAL ADDRESS");
    lv_obj_set_style_text_font(manualTitle, &lv_font_montserrat_20, 0);
    lv_obj_align(manualTitle, LV_ALIGN_TOP_MID, 0, layout.y(100));
    manualAddressLabel = lv_label_create(screen);
    lv_obj_set_style_text_font(manualAddressLabel, &lv_font_montserrat_48, 0);
    lv_obj_align(manualAddressLabel, LV_ALIGN_TOP_MID, 0, layout.y(132));
    manualDecreaseButton = makeButton(screen, layout, "-", 100, 42, -65, 220, manualDecreaseEvent, this);
    manualIncreaseButton = makeButton(screen, layout, "+", 100, 42, 65, 220, manualIncreaseEvent, this);
    manualBackButton = makeButton(screen, layout, "Back", 118, 42, -70, 348, backEvent, this);
    manualSelectButton = makeButton(screen, layout, "Select", 118, 42, 70, 348, selectEvent, this);
    for (lv_obj_t *object : {manualTitle, manualAddressLabel, manualDecreaseButton,
                             manualIncreaseButton, manualBackButton, manualSelectButton})
        lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
    lvgl_port_unlock();
}

void LocomotiveSelectionUI::show(const std::vector<Locomotive> &roster,
                                 bool ready, uint16_t currentAddress)
{
    lvgl_port_lock(-1);
    if (!visible)
        returnScreen = lv_scr_act();
    manualMode = false;
    manualAddress = currentAddress < 1 ? 1 : currentAddress;
    lv_obj_clear_flag(roller, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(selectedLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(status, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(listBackButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(listSelectButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(refreshButton, LV_OBJ_FLAG_HIDDEN);
    for (lv_obj_t *object : {manualTitle, manualAddressLabel, manualDecreaseButton,
                             manualIncreaseButton, manualBackButton, manualSelectButton})
        lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
    addresses.clear();
    optionLabels.clear();
    addresses.push_back(0);
    optionLabels.push_back("Enter Address");
    String options = "Enter Address";
    uint16_t selected = 0;
    if (ready)
    {
        for (const Locomotive &entry : roster)
        {
            // Roller options use newlines as separators, so sanitize names.
            String name = entry.name;
            name.replace('\n', ' ');
            name.replace('\r', ' ');
            options += "\n";
            String option = String(entry.address);
            if (name.length())
            {
                option += " - ";
                option += name;
            }
            options += option;
            addresses.push_back(entry.address);
            optionLabels.push_back(option);
            if (entry.address == currentAddress)
                selected = addresses.size() - 1;
        }
    }
    lv_label_set_text(status, !ready ? "Roster unavailable - enter an address" :
                      roster.empty() ? "Roster empty - enter an address" : "Turn knob or swipe, then select");
    lv_roller_set_options(roller, options.c_str(), LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(roller, selected, LV_ANIM_OFF);
    updateSelectedLabel();
    visible = true;
    lv_scr_load(screen);
    lvgl_port_unlock();
}

void LocomotiveSelectionUI::move(int delta)
{
    if (!visible)
        return;
    if (manualMode)
    {
        changeManualAddress(delta);
        return;
    }
    if (addresses.empty())
        return;
    int selected = static_cast<int>(lv_roller_get_selected(roller)) + delta;
    if (selected < 0)
        selected = 0;
    if (selected >= static_cast<int>(addresses.size()))
        selected = addresses.size() - 1;
    lv_roller_set_selected(roller, selected, LV_ANIM_OFF);
    updateSelectedLabel();
}

void LocomotiveSelectionUI::select()
{
    if (manualMode)
    {
        if (visible && selectCallback)
            selectCallback(manualAddress);
        return;
    }
    const uint16_t index = lv_roller_get_selected(roller);
    if (visible && index < addresses.size() && addresses[index] == 0)
    {
        showManual(manualAddress);
        return;
    }
    if (visible && selectCallback && index < addresses.size())
        selectCallback(addresses[index]);
}

void LocomotiveSelectionUI::hide()
{
    if (visible && returnScreen)
        lv_scr_load(returnScreen);
    visible = false;
}

void LocomotiveSelectionUI::selectEvent(lv_event_t *event)
{
    static_cast<LocomotiveSelectionUI *>(lv_event_get_user_data(event))->select();
}

void LocomotiveSelectionUI::backEvent(lv_event_t *event)
{
    auto *ui = static_cast<LocomotiveSelectionUI *>(lv_event_get_user_data(event));
    if (ui->manualMode)
    {
        ui->show({}, false, ui->manualAddress);
        return;
    }
    if (ui->backCallback)
        ui->backCallback();
}

void LocomotiveSelectionUI::refreshEvent(lv_event_t *event)
{
    auto *ui = static_cast<LocomotiveSelectionUI *>(lv_event_get_user_data(event));
    if (!ui->manualMode && ui->refreshCallback)
    {
        lv_label_set_text(ui->status, "Refreshing roster...");
        ui->refreshCallback();
    }
}

void LocomotiveSelectionUI::rollerEvent(lv_event_t *event)
{
    static_cast<LocomotiveSelectionUI *>(lv_event_get_user_data(event))->updateSelectedLabel();
}

void LocomotiveSelectionUI::updateSelectedLabel()
{
    const uint16_t selected = lv_roller_get_selected(roller);
    if (selected < optionLabels.size())
        lv_label_set_text(selectedLabel, optionLabels[selected].c_str());
}

void LocomotiveSelectionUI::showManual(uint16_t address)
{
    manualMode = true;
    manualAddress = address < 1 ? 1 : address;
    lv_obj_add_flag(roller, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(selectedLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(status, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(listBackButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(listSelectButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(refreshButton, LV_OBJ_FLAG_HIDDEN);
    for (lv_obj_t *object : {manualTitle, manualAddressLabel, manualDecreaseButton,
                             manualIncreaseButton, manualBackButton, manualSelectButton})
        lv_obj_clear_flag(object, LV_OBJ_FLAG_HIDDEN);
    updateManualAddress();
}

void LocomotiveSelectionUI::updateManualAddress()
{
    lv_label_set_text_fmt(manualAddressLabel, "%u", manualAddress);
}

void LocomotiveSelectionUI::changeManualAddress(int delta)
{
    int address = static_cast<int>(manualAddress) + delta;
    address = address < 1 ? 1 : address;
    address = address > 9999 ? 9999 : address;
    manualAddress = static_cast<uint16_t>(address);
    updateManualAddress();
}

void LocomotiveSelectionUI::manualDecreaseEvent(lv_event_t *event)
{
    static_cast<LocomotiveSelectionUI *>(lv_event_get_user_data(event))->changeManualAddress(-1);
}

void LocomotiveSelectionUI::manualIncreaseEvent(lv_event_t *event)
{
    static_cast<LocomotiveSelectionUI *>(lv_event_get_user_data(event))->changeManualAddress(1);
}
