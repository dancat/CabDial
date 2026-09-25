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
                                  RefreshCallback refresh, ReleaseCallback release)
{
    lvgl_port_lock(-1);
    selectCallback = select;
    backCallback = back;
    refreshCallback = refresh;
    releaseCallback = release;
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

    listSelectButton = makeButton(screen, layout, "Select", 94, 42, -110, 348, selectEvent, this);
    listReleaseButton = makeButton(screen, layout, "Release", 94, 42, 0, 348, manualReleaseEvent, this);
    listBackButton = makeButton(screen, layout, "Back", 94, 42, 110, 348, backEvent, this);
    refreshButton = makeButton(screen, layout, "Refresh", 112, 34, 0, 405, refreshEvent, this);

    manualTitle = lv_label_create(screen);
    lv_label_set_text(manualTitle, "MANUAL ADDRESS");
    lv_obj_set_style_text_font(manualTitle, &lv_font_montserrat_20, 0);
    lv_obj_align(manualTitle, LV_ALIGN_TOP_MID, 0, layout.y(78));

    static const int16_t digitX[MANUAL_DIGIT_COUNT] = {-112, -56, 0, 56, 112};
    for (uint8_t digit = 0; digit < MANUAL_DIGIT_COUNT; ++digit)
    {
        manualIncreaseButtons[digit] = makeButton(screen, layout, "+", 44, 40,
            digitX[digit], 122, manualIncreaseEvent, this);
        manualDigitLabels[digit] = lv_label_create(screen);
        lv_obj_set_size(manualDigitLabels[digit], layout.width(44), layout.height(52));
        lv_obj_set_style_text_align(manualDigitLabels[digit], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(manualDigitLabels[digit], &lv_font_montserrat_36, 0);
        lv_obj_set_style_text_color(manualDigitLabels[digit], lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(manualDigitLabels[digit], LV_ALIGN_TOP_MID, layout.x(digitX[digit]), layout.y(172));
        lv_obj_add_flag(manualDigitLabels[digit], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(manualDigitLabels[digit], manualDigitEvent, LV_EVENT_CLICKED, this);
        manualDecreaseButtons[digit] = makeButton(screen, layout, "-", 44, 40,
            digitX[digit], 232, manualDecreaseEvent, this);
    }
    manualSelectButton = makeButton(screen, layout, "Select", 118, 42, -70, 348, manualSelectEvent, this);
    manualBackButton = makeButton(screen, layout, "Back", 118, 42, 70, 348, backEvent, this);
    setManualControlsVisible(false);
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
    lv_obj_clear_flag(listReleaseButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(listSelectButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(refreshButton, LV_OBJ_FLAG_HIDDEN);
    setManualControlsVisible(false);
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
        if (editingManualDigit)
            changeManualDigit(activeManualDigit, delta);
        else
        {
            // Mechanical encoder bounce can briefly report the opposite
            // direction. Suppress that reversal so a continued turn at an
            // end stop cannot make focus jump between lower action buttons.
            const unsigned long now = millis();
            if (lastManualFocusDirection != 0 && delta != lastManualFocusDirection &&
                now - lastManualFocusMoveAt < 35)
                return;

            const int next = static_cast<int>(manualFocus) + delta;
            manualFocus = static_cast<uint8_t>(next < 0 ? 0 :
                next > MANUAL_DIGIT_COUNT + MANUAL_ACTION_COUNT - 1 ?
                MANUAL_DIGIT_COUNT + MANUAL_ACTION_COUNT - 1 : next);
            lastManualFocusDirection = delta < 0 ? -1 : 1;
            lastManualFocusMoveAt = now;
            updateManualFocus();
        }
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
        if (editingManualDigit)
        {
            editingManualDigit = false;
            updateManualFocus();
            return;
        }
        if (manualFocus < MANUAL_DIGIT_COUNT)
        {
            activeManualDigit = manualFocus;
            editingManualDigit = true;
            updateManualFocus();
            return;
        }
        if (manualFocus == MANUAL_DIGIT_COUNT)
            selectManualAddress();
        else
            returnToRoster();
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
        ui->returnToRoster();
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
    manualAddress = address < 1 ? 1 : address > 10239 ? 10239 : address;
    activeManualDigit = MANUAL_DIGIT_COUNT - 1;
    manualFocus = 0;
    editingManualDigit = false;
    lastManualFocusDirection = 0;
    lastManualFocusMoveAt = 0;
    lv_obj_add_flag(roller, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(selectedLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(status, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(listBackButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(listReleaseButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(listSelectButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(refreshButton, LV_OBJ_FLAG_HIDDEN);
    setManualControlsVisible(true);
    updateManualFocus();
}

void LocomotiveSelectionUI::setManualControlsVisible(bool show)
{
    for (lv_obj_t *object : {manualTitle, manualBackButton, manualSelectButton})
    {
        if (show)
            lv_obj_clear_flag(object, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
    }
    for (uint8_t digit = 0; digit < MANUAL_DIGIT_COUNT; ++digit)
    {
        for (lv_obj_t *object : {manualIncreaseButtons[digit], manualDigitLabels[digit],
                                 manualDecreaseButtons[digit]})
        {
            if (show)
                lv_obj_clear_flag(object, LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void LocomotiveSelectionUI::updateManualAddress()
{
    uint16_t remainder = manualAddress;
    for (int digit = MANUAL_DIGIT_COUNT - 1; digit >= 0; --digit)
    {
        manualDigits[digit] = remainder % 10;
        remainder /= 10;
    }
    for (uint8_t digit = 0; digit < MANUAL_DIGIT_COUNT; ++digit)
    {
        lv_label_set_text_fmt(manualDigitLabels[digit], "%u", manualDigits[digit]);
        const bool selected = editingManualDigit && digit == activeManualDigit;
        const bool focused = !editingManualDigit && digit == manualFocus;
        lv_obj_set_style_text_color(manualDigitLabels[digit],
            lv_color_hex(selected ? 0xFFD740 : focused ? 0x29B6F6 : 0xFFFFFF), 0);
    }
}

void LocomotiveSelectionUI::updateManualFocus()
{
    updateManualAddress();
    lv_obj_t *buttons[] = {manualSelectButton, manualBackButton};
    for (uint8_t index = 0; index < MANUAL_ACTION_COUNT; ++index)
    {
        const bool focused = !editingManualDigit && manualFocus == MANUAL_DIGIT_COUNT + index;
        lv_obj_set_style_bg_color(buttons[index],
            lv_color_hex(focused ? 0x266A91 : 0x263746), LV_PART_MAIN);
        lv_obj_set_style_border_color(buttons[index],
            lv_color_hex(focused ? 0x6CCBFF : 0x3C566B), LV_PART_MAIN);
    }
}

void LocomotiveSelectionUI::returnToRoster()
{
    manualMode = false;
    editingManualDigit = false;
    lv_obj_clear_flag(roller, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(selectedLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(status, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(listBackButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(listReleaseButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(listSelectButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(refreshButton, LV_OBJ_FLAG_HIDDEN);
    setManualControlsVisible(false);
    updateSelectedLabel();
}

void LocomotiveSelectionUI::selectManualAddress()
{
    if (visible && selectCallback)
        selectCallback(manualAddress);
}

void LocomotiveSelectionUI::changeManualDigit(uint8_t digit, int delta)
{
    if (digit >= MANUAL_DIGIT_COUNT)
        return;
    activeManualDigit = digit;
    const uint8_t previous = manualDigits[digit];
    const uint8_t next = static_cast<uint8_t>((previous + delta + 10) % 10);
    manualDigits[digit] = next;

    uint16_t candidate = 0;
    for (uint8_t index = 0; index < MANUAL_DIGIT_COUNT; ++index)
        candidate = static_cast<uint16_t>(candidate * 10 + manualDigits[index]);
    if (candidate >= 1 && candidate <= 10239)
        manualAddress = candidate;
    else
        manualDigits[digit] = previous;

    updateManualAddress();
}

void LocomotiveSelectionUI::manualDecreaseEvent(lv_event_t *event)
{
    auto *ui = static_cast<LocomotiveSelectionUI *>(lv_event_get_user_data(event));
    const lv_obj_t *button = lv_event_get_target(event);
    for (uint8_t digit = 0; digit < MANUAL_DIGIT_COUNT; ++digit)
        if (ui->manualDecreaseButtons[digit] == button)
            ui->changeManualDigit(digit, -1);
}

void LocomotiveSelectionUI::manualIncreaseEvent(lv_event_t *event)
{
    auto *ui = static_cast<LocomotiveSelectionUI *>(lv_event_get_user_data(event));
    const lv_obj_t *button = lv_event_get_target(event);
    for (uint8_t digit = 0; digit < MANUAL_DIGIT_COUNT; ++digit)
        if (ui->manualIncreaseButtons[digit] == button)
            ui->changeManualDigit(digit, 1);
}

void LocomotiveSelectionUI::manualDigitEvent(lv_event_t *event)
{
    auto *ui = static_cast<LocomotiveSelectionUI *>(lv_event_get_user_data(event));
    const lv_obj_t *label = lv_event_get_target(event);
    for (uint8_t digit = 0; digit < MANUAL_DIGIT_COUNT; ++digit)
    {
        if (ui->manualDigitLabels[digit] == label)
        {
            ui->manualFocus = digit;
            ui->activeManualDigit = digit;
            ui->editingManualDigit = true;
            ui->updateManualFocus();
            return;
        }
    }
}

void LocomotiveSelectionUI::manualReleaseEvent(lv_event_t *event)
{
    auto *ui = static_cast<LocomotiveSelectionUI *>(lv_event_get_user_data(event));
    if (ui->releaseCallback)
        ui->releaseCallback();
}

void LocomotiveSelectionUI::manualSelectEvent(lv_event_t *event)
{
    static_cast<LocomotiveSelectionUI *>(lv_event_get_user_data(event))->selectManualAddress();
}
