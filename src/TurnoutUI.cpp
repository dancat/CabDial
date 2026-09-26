#include "TurnoutUI.h"
#include "lvgl_v8_port.h"
#include "UiLayout.h"

namespace
{
lv_obj_t *makeButton(lv_obj_t *parent, const UiLayout &layout, const char *text, int width, int height, int x, int y,
    lv_event_cb_t callback, void *user)
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

void TurnoutUI::begin(SetCallback set, FavoriteCallback favorite, BackCallback back,
                      RefreshCallback refresh)
{
    lvgl_port_lock(-1);
    setCallback = set;
    favoriteCallback = favorite;
    backCallback = back;
    refreshCallback = refresh;
    const UiLayout layout(profile);
    screen = lv_obj_create(nullptr);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x080C10), 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "TURNOUTS");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, layout.y(20));

    status = lv_label_create(screen);
    lv_obj_set_width(status, layout.width(320));
    lv_obj_set_style_text_align(status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status, lv_color_hex(0x8899AA), 0);
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, layout.y(58));

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
    lv_obj_align(roller, LV_ALIGN_TOP_MID, 0, layout.y(100));
    lv_obj_add_event_cb(roller, rollerEvent, LV_EVENT_VALUE_CHANGED, this);

    selectedLabel = lv_label_create(screen);
    lv_obj_set_width(selectedLabel, layout.width(320));
    lv_label_set_long_mode(selectedLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(selectedLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(selectedLabel, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(selectedLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(selectedLabel, LV_ALIGN_TOP_MID, 0, layout.y(176));

    makeButton(screen, layout, "Refresh", 156, 42, 0, 398, refreshEvent, this);
    makeButton(screen, layout, "Favorite", 94, 54, -144, 322, favoriteEvent, this);
    makeButton(screen, layout, "Close", 94, 54, -48, 322, closeEvent, this);
    makeButton(screen, layout, "Throw", 94, 54, 48, 322, throwEvent, this);
    makeButton(screen, layout, "Back", 94, 54, 144, 322, backEvent, this);
    lvgl_port_unlock();
}

void TurnoutUI::show(const std::vector<TurnoutDefinition> &turnouts, bool ready)
{
    lvgl_port_lock(-1);
    if (!visible)
        returnScreen = lv_scr_act();

    const int priorId = ids.empty() ? 0 : ids[lv_roller_get_selected(roller)];
    ids.clear();
    optionLabels.clear();
    thrownStates.clear();
    String options;
    uint16_t selected = 0;
    for (const TurnoutDefinition &turnout : turnouts)
    {
        if (options.length())
            options += "\n";
        String name = turnout.name;
        name.replace('\n', ' ');
        name.replace('\r', ' ');
        options += String(turnout.id);
        if (name.length())
        {
            options += " - ";
            options += name;
        }
        options += turnout.thrown ? "  [THROWN]" : "  [CLOSED]";
        if (turnout.favorite)
            options += "  [FAV]";
        ids.push_back(turnout.id);
        thrownStates.push_back(turnout.thrown);
        optionLabels.push_back(options.substring(options.lastIndexOf('\n') + 1));
        if (turnout.id == priorId)
            selected = ids.size() - 1;
    }

    if (options.length() == 0)
    {
        options = "No turnouts";
        optionLabels.push_back(options);
    }
    lv_label_set_text(status, !ready ? "Loading turnout list..." :
        turnouts.empty() ? "No turnouts configured" : "Press the encoder to toggle, or choose Close or Throw");
    lv_roller_set_options(roller, options.c_str(), LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(roller, selected, LV_ANIM_OFF);
    updateSelectedLabel();
    visible = true;
    lv_scr_load(screen);
    lvgl_port_unlock();
}

void TurnoutUI::move(int delta)
{
    if (!visible || ids.empty())
        return;
    int selected = static_cast<int>(lv_roller_get_selected(roller)) + delta;
    if (selected < 0)
        selected = 0;
    if (selected >= static_cast<int>(ids.size()))
        selected = ids.size() - 1;
    lv_roller_set_selected(roller, selected, LV_ANIM_OFF);
    updateSelectedLabel();
}

void TurnoutUI::closeSelected() { setSelected(false); }
void TurnoutUI::throwSelected() { setSelected(true); }

void TurnoutUI::toggleSelected()
{
    const uint16_t selected = lv_roller_get_selected(roller);
    if (visible && selected < ids.size() && selected < thrownStates.size())
        setSelected(!thrownStates[selected]);
}

void TurnoutUI::toggleFavoriteSelected()
{
    const uint16_t selected = lv_roller_get_selected(roller);
    if (visible && favoriteCallback && selected < ids.size())
        favoriteCallback(ids[selected]);
}

void TurnoutUI::hide()
{
    if (visible && returnScreen)
        lv_scr_load(returnScreen);
    visible = false;
}

void TurnoutUI::setSelected(bool thrown)
{
    const uint16_t selected = lv_roller_get_selected(roller);
    if (visible && setCallback && selected < ids.size())
        setCallback(ids[selected], thrown);
}

void TurnoutUI::closeEvent(lv_event_t *event)
{
    static_cast<TurnoutUI *>(lv_event_get_user_data(event))->closeSelected();
}

void TurnoutUI::throwEvent(lv_event_t *event)
{
    static_cast<TurnoutUI *>(lv_event_get_user_data(event))->throwSelected();
}

void TurnoutUI::favoriteEvent(lv_event_t *event)
{
    static_cast<TurnoutUI *>(lv_event_get_user_data(event))->toggleFavoriteSelected();
}

void TurnoutUI::backEvent(lv_event_t *event)
{
    auto *ui = static_cast<TurnoutUI *>(lv_event_get_user_data(event));
    if (ui->backCallback)
        ui->backCallback();
}

void TurnoutUI::refreshEvent(lv_event_t *event)
{
    auto *ui = static_cast<TurnoutUI *>(lv_event_get_user_data(event));
    if (ui->refreshCallback)
    {
        lv_label_set_text(ui->status, "Refreshing turnout list...");
        ui->refreshCallback();
    }
}

void TurnoutUI::rollerEvent(lv_event_t *event)
{
    static_cast<TurnoutUI *>(lv_event_get_user_data(event))->updateSelectedLabel();
}

void TurnoutUI::updateSelectedLabel()
{
    const uint16_t selected = lv_roller_get_selected(roller);
    if (selected < optionLabels.size())
        lv_label_set_text(selectedLabel, optionLabels[selected].c_str());
}
