#include "TurnoutUI.h"
#include "lvgl_v8_port.h"
#include "UiLayout.h"

namespace
{
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
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, layout.y(36));

    status = lv_label_create(screen);
    lv_obj_set_width(status, layout.width(320));
    lv_obj_set_style_text_align(status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status, lv_color_hex(0x8899AA), 0);
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, layout.y(76));

    roller = lv_roller_create(screen);
    lv_obj_set_width(roller, layout.width(360));
    lv_obj_set_style_text_font(roller, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_bg_color(roller, lv_color_hex(0x101820), LV_PART_MAIN);
    lv_obj_set_style_text_color(roller, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_color(roller, lv_color_hex(0x266A91), LV_PART_SELECTED);
    lv_obj_set_style_text_color(roller, lv_color_hex(0xFFFFFF), LV_PART_SELECTED);
    lv_roller_set_visible_row_count(roller, 5);
    lv_obj_align(roller, LV_ALIGN_CENTER, 0, 0);

    makeButton(screen, layout, "Refresh", 0, 112, refreshEvent, this);
    makeButton(screen, layout, "Favorite", -135, 350, favoriteEvent, this);
    makeButton(screen, layout, "Close", -45, 350, closeEvent, this);
    makeButton(screen, layout, "Throw", 45, 350, throwEvent, this);
    makeButton(screen, layout, "Back", 135, 350, backEvent, this);
    lvgl_port_unlock();
}

void TurnoutUI::show(const std::vector<TurnoutDefinition> &turnouts, bool ready)
{
    lvgl_port_lock(-1);
    if (!visible)
        returnScreen = lv_scr_act();

    const int priorId = ids.empty() ? 0 : ids[lv_roller_get_selected(roller)];
    ids.clear();
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
        if (turnout.id == priorId)
            selected = ids.size() - 1;
    }

    if (options.length() == 0)
        options = "No turnouts";
    lv_label_set_text(status, !ready ? "Loading turnout list..." :
        turnouts.empty() ? "No turnouts configured" : "Select a turnout, then choose Close or Throw");
    lv_roller_set_options(roller, options.c_str(), LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(roller, selected, LV_ANIM_OFF);
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
}

void TurnoutUI::closeSelected() { setSelected(false); }
void TurnoutUI::throwSelected() { setSelected(true); }

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
