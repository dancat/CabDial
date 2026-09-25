#include "ThrottleUI.h"
#include "UiLayout.h"
#include "lvgl_v8_port.h"
#include "Locomotive.h"

namespace
{
const char *functionPictogram(uint8_t function, const LocoFunction &definition)
{
    String name = definition.name;
    name.toLowerCase();

    if (name.indexOf("light") >= 0 || function == 0)
        return LV_SYMBOL_TINT;
    if (name.indexOf("horn") >= 0 || name.indexOf("whistle") >= 0 || function == 1)
        return LV_SYMBOL_VOLUME_MAX;
    if (name.indexOf("bell") >= 0 || function == 2)
        return LV_SYMBOL_BELL;
    if (name.indexOf("sound") >= 0 || name.indexOf("audio") >= 0)
        return LV_SYMBOL_AUDIO;
    if (name.indexOf("mute") >= 0)
        return LV_SYMBOL_MUTE;
    if (name.indexOf("start") >= 0)
        return LV_SYMBOL_PLAY;
    if (name.indexOf("stop") >= 0)
        return LV_SYMBOL_STOP;

    return nullptr;
}
}

void ThrottleUI::begin()
{
    lvgl_port_lock(-1);
    const UiLayout layout(profile);

    screen = lv_scr_act();

    // Background
    lv_obj_set_style_bg_color(
        screen,
        lv_color_hex(0x080C10),
        LV_PART_MAIN
    );

    lv_obj_set_style_bg_opa(
        screen,
        LV_OPA_COVER,
        LV_PART_MAIN
    );

    // -------------------------------------------------
    // Connection and settings - matching navigation tiles inside the usable
    // circular area at the top of the throttle screen.
    // -------------------------------------------------

    connectionButton = lv_btn_create(screen);
    lv_obj_set_size(connectionButton, layout.width(88), layout.height(58));
    lv_obj_align(connectionButton, LV_ALIGN_TOP_MID, layout.x(-50), layout.y(20));
    lv_obj_set_style_bg_color(connectionButton, lv_color_hex(0x263746), LV_PART_MAIN);
    lv_obj_set_style_radius(connectionButton, 14, LV_PART_MAIN);
    lv_obj_set_style_border_color(connectionButton, lv_color_hex(0x3C566B), LV_PART_MAIN);
    lv_obj_set_style_border_width(connectionButton, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(connectionButton, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(connectionButton, connectionButtonEvent, LV_EVENT_CLICKED, this);
    connectionIndicator = lv_label_create(connectionButton);
    lv_label_set_text(connectionIndicator, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(connectionIndicator, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(connectionIndicator, lv_color_hex(0x8899AA), 0);
    lv_obj_align(connectionIndicator, LV_ALIGN_TOP_MID, 0, 3);

    navLabel = connectionLabel = lv_label_create(connectionButton);
    lv_obj_set_width(connectionLabel, layout.width(82));
    lv_label_set_long_mode(connectionLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(connectionLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(connectionLabel, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(connectionLabel, lv_color_hex(0x8899AA), 0);
    lv_label_set_text(connectionLabel, "CONNECTION");
    lv_obj_align(connectionLabel, LV_ALIGN_BOTTOM_MID, 0, -5);

    settingsButton = lv_btn_create(screen);
    lv_obj_set_size(settingsButton, layout.width(88), layout.height(58));
    lv_obj_align(settingsButton, LV_ALIGN_TOP_MID, layout.x(50), layout.y(20));
    lv_obj_set_style_bg_color(settingsButton, lv_color_hex(0x263746), LV_PART_MAIN);
    lv_obj_set_style_radius(settingsButton, 14, LV_PART_MAIN);
    lv_obj_set_style_border_color(settingsButton, lv_color_hex(0x3C566B), LV_PART_MAIN);
    lv_obj_set_style_border_width(settingsButton, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(settingsButton, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(settingsButton, settingsButtonEvent, LV_EVENT_CLICKED, this);
    lv_obj_t *settingsIcon = lv_label_create(settingsButton);
    lv_label_set_text(settingsIcon, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_font(settingsIcon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(settingsIcon, lv_color_hex(0xCCD7E0), 0);
    lv_obj_align(settingsIcon, LV_ALIGN_TOP_MID, 0, 3);
    lv_obj_t *settingsLabel = lv_label_create(settingsButton);
    lv_label_set_text(settingsLabel, "SETTINGS");
    lv_obj_set_style_text_font(settingsLabel, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(settingsLabel, lv_color_hex(0xCCD7E0), 0);
    lv_obj_align(settingsLabel, LV_ALIGN_BOTTOM_MID, 0, -5);

    // Mode inside the speed arc, below the main value.
    modeLabel = lv_label_create(screen);

    lv_obj_set_style_text_font(
        modeLabel,
        &lv_font_montserrat_18,
        0
    );

    lv_obj_set_style_text_color(
        modeLabel,
        lv_color_hex(0x29B6F6),
        0
    );

    lv_label_set_text(modeLabel, "SPEED");

    lv_obj_align(
        modeLabel,
        LV_ALIGN_CENTER,
        0,
        -5
    );

    // -------------------------------------------------
    // Main value - center
    // -------------------------------------------------

    // -------------------------------------------------
    // Speed arc
    // -------------------------------------------------

    speedArc = lv_arc_create(screen);

    lv_obj_set_size(speedArc, layout.width(220), layout.height(220));
    lv_obj_align(speedArc, LV_ALIGN_CENTER, 0, layout.y(-40));

    // Speed range
    lv_arc_set_range(speedArc, 0, 126);
    lv_arc_set_value(speedArc, 0);

    // Leave a gap at the bottom
    lv_arc_set_bg_angles(speedArc, 135, 45);

    // We don't use the arc as an input control
    lv_obj_clear_flag(speedArc, LV_OBJ_FLAG_CLICKABLE);

    // Hide the normal arc knob
    lv_obj_set_style_opa(
        speedArc,
        LV_OPA_TRANSP,
        LV_PART_KNOB
    );

    // Background track
    lv_obj_set_style_arc_width(
        speedArc,
        12,
        LV_PART_MAIN
    );

    lv_obj_set_style_arc_color(
        speedArc,
        lv_color_hex(0x263746),
        LV_PART_MAIN
    );

    // Active speed
    lv_obj_set_style_arc_width(
        speedArc,
        12,
        LV_PART_INDICATOR
    );

    lv_obj_set_style_arc_color(
        speedArc,
        lv_color_hex(0x29B6F6),
        LV_PART_INDICATOR
    );

    valueLabel = lv_label_create(screen);

    lv_obj_set_style_text_font(
        valueLabel,
        &lv_font_montserrat_48,
        0
    );

    lv_obj_set_style_text_color(
        valueLabel,
        lv_color_hex(0xFFFFFF),
        0
    );

    lv_label_set_text(valueLabel, "0");

    lv_obj_align(
        valueLabel,
        LV_ALIGN_CENTER,
        0,
        -55
    );

    // Speed range
    rangeLabel = lv_label_create(screen);

    lv_obj_set_style_text_font(
        rangeLabel,
        &lv_font_montserrat_18,
        0
    );

    lv_obj_set_style_text_color(
        rangeLabel,
        lv_color_hex(0x8899AA),
        0
    );

    lv_label_set_text(rangeLabel, "/ 126");

    lv_obj_align(
        rangeLabel,
        LV_ALIGN_CENTER,
        0,
        -5
    );
    lv_obj_add_flag(rangeLabel, LV_OBJ_FLAG_HIDDEN);

    speedPreset50Button = lv_btn_create(screen);
    lv_obj_set_size(speedPreset50Button, layout.width(78), layout.height(63));
    lv_obj_align(speedPreset50Button, LV_ALIGN_TOP_MID, layout.x(-164), layout.y(317));
    lv_obj_set_style_bg_color(speedPreset50Button, lv_color_hex(0x263746), 0);
    lv_obj_add_event_cb(speedPreset50Button, speedPresetButtonEvent, LV_EVENT_CLICKED, this);
    lv_obj_t *speedPreset50Label = lv_label_create(speedPreset50Button);
    lv_label_set_text(speedPreset50Label, "50");
    lv_obj_set_style_text_font(speedPreset50Label, &lv_font_montserrat_24, 0);
    lv_obj_center(speedPreset50Label);

    speedPreset75Button = lv_btn_create(screen);
    lv_obj_set_size(speedPreset75Button, layout.width(78), layout.height(63));
    lv_obj_align(speedPreset75Button, LV_ALIGN_TOP_MID, layout.x(-82), layout.y(317));
    lv_obj_set_style_bg_color(speedPreset75Button, lv_color_hex(0x263746), 0);
    lv_obj_add_event_cb(speedPreset75Button, speedPresetButtonEvent, LV_EVENT_CLICKED, this);
    lv_obj_t *speedPreset75Label = lv_label_create(speedPreset75Button);
    lv_label_set_text(speedPreset75Label, "75");
    lv_obj_set_style_text_font(speedPreset75Label, &lv_font_montserrat_24, 0);
    lv_obj_center(speedPreset75Label);

    stopButton = lv_btn_create(screen);
    lv_obj_set_size(stopButton, layout.width(78), layout.height(63));
    lv_obj_align(stopButton, LV_ALIGN_TOP_MID, 0, layout.y(317));
    lv_obj_set_style_bg_color(stopButton, lv_color_hex(0x263746), 0);
    lv_obj_add_event_cb(stopButton, stopButtonEvent, LV_EVENT_CLICKED, this);
    lv_obj_t *stopIcon = lv_obj_create(stopButton);
    lv_obj_set_size(stopIcon, 24, 24);
    lv_obj_set_style_bg_color(stopIcon, lv_color_hex(0xFF5252), 0);
    lv_obj_set_style_border_width(stopIcon, 0, 0);
    lv_obj_set_style_radius(stopIcon, 2, 0);
    lv_obj_center(stopIcon);

    emergencyStopButton = lv_btn_create(screen);
    lv_obj_set_size(emergencyStopButton, layout.width(78), layout.height(63));
    lv_obj_align(emergencyStopButton, LV_ALIGN_TOP_MID, layout.x(164), layout.y(317));
    lv_obj_set_style_bg_color(emergencyStopButton, lv_color_hex(0xD32F2F), 0);
    lv_obj_add_event_cb(emergencyStopButton, emergencyStopButtonEvent, LV_EVENT_CLICKED, this);
    lv_obj_t *emergencyStopLabel = lv_label_create(emergencyStopButton);
    lv_label_set_text(emergencyStopLabel, "!");
    lv_obj_set_style_text_font(emergencyStopLabel, &lv_font_montserrat_36, 0);
    lv_obj_center(emergencyStopLabel);

    // -------------------------------------------------
    // Locomotive - left
    // -------------------------------------------------

    lv_obj_t *selectionButton = lv_btn_create(screen);
    // Keep the locomotive summary tall enough for its icon, address, and
    // roster name, matching the vertical card used by the round layout.
    lv_obj_set_size(selectionButton, layout.width(106), layout.height(196));
    lv_obj_align(selectionButton, LV_ALIGN_LEFT_MID, layout.x(4), layout.y(-52));
    lv_obj_set_style_bg_color(selectionButton, lv_color_hex(0x101820), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(selectionButton, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(selectionButton, lv_color_hex(0x263746), LV_PART_MAIN);
    lv_obj_set_style_border_width(selectionButton, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(selectionButton, 16, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(selectionButton, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(selectionButton, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(selectionButton, selectionButtonEvent, LV_EVENT_CLICKED, this);
    addressTitleLabel = lv_label_create(selectionButton);

    lv_obj_set_style_text_font(
        addressTitleLabel,
        &lv_font_montserrat_14,
        0
    );

    lv_obj_set_style_text_color(
        addressTitleLabel,
        lv_color_hex(0x8899AA),
        0
    );

    lv_label_set_text(addressTitleLabel, "LOCO");
    lv_obj_set_width(addressTitleLabel, layout.width(96));
    lv_label_set_long_mode(addressTitleLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(addressTitleLabel, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_align(
        addressTitleLabel,
        LV_ALIGN_TOP_MID,
        0,
        16
    );

    auto locoPart = [&](int width, int height, int x, int y, uint32_t color, int radius)
    {
        lv_obj_t *part = lv_obj_create(selectionButton);
        lv_obj_set_size(part, width, height);
        lv_obj_align(part, LV_ALIGN_TOP_MID, x, y);
        lv_obj_set_style_bg_color(part, lv_color_hex(color), 0);
        lv_obj_set_style_border_width(part, 0, 0);
        lv_obj_set_style_radius(part, radius, 0);
        lv_obj_clear_flag(part, LV_OBJ_FLAG_CLICKABLE);
        return part;
    };

    // Front-view locomotive pictogram, drawn from LVGL primitives so it stays
    // crisp at the target display resolution.
    locoPart(42, 42, 0, 38, 0xCCD7E0, 6);      // Cab front
    locoPart(26, 11, 0, 45, 0x101820, 3);      // Windshield
    locoPart(6, 6, -12, 64, 0x101820, LV_RADIUS_CIRCLE); // Left light
    locoPart(6, 6, 12, 64, 0x101820, LV_RADIUS_CIRCLE);  // Right light
    locoPart(34, 4, 0, 80, 0xCCD7E0, 1);       // Buffer beam
    locoPart(8, 8, -13, 79, 0x101820, LV_RADIUS_CIRCLE); // Left wheel
    locoPart(8, 8, 13, 79, 0x101820, LV_RADIUS_CIRCLE);  // Right wheel

    addressLabel = lv_label_create(selectionButton);

    lv_obj_set_style_text_font(
        addressLabel,
        &lv_font_montserrat_28,
        0
    );

    lv_obj_set_style_text_color(
        addressLabel,
        lv_color_hex(0xFFFFFF),
        0
    );

    lv_label_set_text(addressLabel, "101");

    moreFunctionsButton = lv_btn_create(selectionButton);
    lv_obj_set_size(moreFunctionsButton, layout.width(86), layout.height(28));
    lv_obj_align(moreFunctionsButton, LV_ALIGN_BOTTOM_MID, 0, -layout.y(10));
    lv_obj_set_style_bg_color(moreFunctionsButton, lv_color_hex(0x263746), LV_PART_MAIN);
    lv_obj_set_style_radius(moreFunctionsButton, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(moreFunctionsButton, 0, LV_PART_MAIN);
    lv_obj_t *moreLabel = lv_label_create(moreFunctionsButton);
    lv_label_set_text(moreLabel, "F+");
    lv_obj_set_style_text_font(moreLabel, &lv_font_montserrat_16, 0);
    lv_obj_center(moreLabel);
    lv_obj_add_event_cb(moreFunctionsButton, moreFunctionsEvent, LV_EVENT_CLICKED, this);

    lv_obj_align(
        addressLabel,
        LV_ALIGN_TOP_MID,
        0,
        // Keep the address clear of the locomotive pictogram and visually
        // anchor it against the divider above the roster name.
        104
    );

    lv_obj_t *nameDivider = lv_obj_create(selectionButton);
    lv_obj_set_size(nameDivider, layout.width(80), layout.height(2));
    lv_obj_align(nameDivider, LV_ALIGN_TOP_MID, 0, layout.y(142));
    lv_obj_set_style_bg_color(nameDivider, lv_color_hex(0x3C566B), 0);
    lv_obj_set_style_border_width(nameDivider, 0, 0);
    lv_obj_clear_flag(nameDivider, LV_OBJ_FLAG_CLICKABLE);

    // -------------------------------------------------
    // Direction toggle - centered between the two stop controls
    // -------------------------------------------------

    directionButton = lv_btn_create(screen);
    lv_obj_set_size(directionButton, layout.width(78), layout.height(63));
    lv_obj_align(directionButton, LV_ALIGN_TOP_MID, layout.x(82), layout.y(317));
    lv_obj_set_style_bg_color(directionButton, lv_color_hex(0x263746), 0);
    lv_obj_add_event_cb(directionButton, directionButtonEvent, LV_EVENT_CLICKED, this);

    directionArrowLabel = lv_label_create(directionButton);

    lv_obj_set_style_text_font(
        directionArrowLabel,
        &lv_font_montserrat_32,
        0
    );

    lv_obj_set_style_text_color(
        directionArrowLabel,
        lv_color_hex(0xFFFFFF),
        0
    );

    lv_label_set_text(directionArrowLabel, LV_SYMBOL_RIGHT);

    lv_obj_align(
        directionArrowLabel,
        LV_ALIGN_CENTER,
        0,
        0
    );

    // -------------------------------------------------
    // Functions touch button
    // -------------------------------------------------
    static const int16_t functionX[FUNCTION_SLOT_COUNT] =
    {
        -88,
        0,
        88
    };
    for (uint8_t i = 0; i < FUNCTION_SLOT_COUNT; i++)
    {
        FunctionSlot &slot = functionSlots[i];

        slot.button = lv_btn_create(screen);

        lv_obj_set_size(
            slot.button,
            layout.width(82),
            layout.height(62)
        );

        lv_obj_align(
            slot.button,
            LV_ALIGN_TOP_MID,
            layout.x(functionX[i]),
            layout.y(394)
        );

        lv_obj_set_style_radius(
            slot.button,
            14,
            LV_PART_MAIN
        );

        lv_obj_set_style_bg_color(
            slot.button,
            lv_color_hex(0x263746),
            LV_PART_MAIN
        );

        lv_obj_set_style_border_width(
            slot.button,
            0,
            LV_PART_MAIN
        );

        // Position the icon and text against the full card, rather than the
        // button's padded content area.
        lv_obj_set_style_pad_all(slot.button, 0, LV_PART_MAIN);

        slot.icon = lv_label_create(slot.button);
        lv_obj_set_style_text_font(slot.icon, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(slot.icon, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(slot.icon, LV_ALIGN_TOP_MID, 0, layout.y(6));

        slot.label = lv_label_create(slot.button);
        lv_obj_set_width(slot.label, layout.width(72));
        lv_label_set_long_mode(slot.label, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(slot.label, LV_TEXT_ALIGN_CENTER, 0);

        lv_obj_set_style_text_font(
            slot.label,
            &lv_font_montserrat_14,
            0
        );

        lv_obj_set_style_text_color(
            slot.label,
            lv_color_hex(0xFFFFFF),
            0
        );

        lv_obj_align(slot.label, LV_ALIGN_BOTTOM_MID, 0, layout.y(-7));

        lv_obj_add_event_cb(slot.button, functionButtonEvent, LV_EVENT_ALL, this);
        lv_obj_add_event_cb(slot.button, functionGestureEvent, LV_EVENT_GESTURE, this);

        // Hidden until assigned
        lv_obj_add_flag(
            slot.button,
            LV_OBJ_FLAG_HIDDEN
        );
    }

    previousPageButton = lv_btn_create(screen);
    lv_obj_set_size(previousPageButton, layout.width(24), layout.height(32));
    lv_obj_align(previousPageButton, LV_ALIGN_TOP_MID, layout.x(-142), layout.y(392));
    lv_obj_set_style_bg_color(previousPageButton, lv_color_hex(0x263746), 0);
    lv_obj_set_style_radius(previousPageButton, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(previousPageButton, 0, LV_PART_MAIN);
    lv_obj_t *previousLabel = lv_label_create(previousPageButton);
    lv_label_set_text(previousLabel, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(previousLabel, &lv_font_montserrat_16, 0);
    lv_obj_center(previousLabel);
    lv_obj_add_event_cb(previousPageButton, previousPageEvent, LV_EVENT_CLICKED, this);

    nextPageButton = lv_btn_create(screen);
    lv_obj_set_size(nextPageButton, layout.width(24), layout.height(32));
    lv_obj_align(nextPageButton, LV_ALIGN_TOP_MID, layout.x(142), layout.y(392));
    lv_obj_set_style_bg_color(nextPageButton, lv_color_hex(0x263746), 0);
    lv_obj_set_style_radius(nextPageButton, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(nextPageButton, 0, LV_PART_MAIN);
    lv_obj_t *nextLabel = lv_label_create(nextPageButton);
    lv_label_set_text(nextLabel, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_font(nextLabel, &lv_font_montserrat_16, 0);
    lv_obj_center(nextLabel);
    lv_obj_add_event_cb(nextPageButton, nextPageEvent, LV_EVENT_CLICKED, this);

    pageLabel = lv_label_create(screen);
    lv_obj_set_style_text_font(pageLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(pageLabel, lv_color_hex(0x8899AA), 0);
    lv_obj_align(pageLabel, LV_ALIGN_TOP_MID, 0, layout.y(312));
    lv_obj_add_flag(previousPageButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(nextPageButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(pageLabel, LV_OBJ_FLAG_HIDDEN);
    auto makeNavigationTile = [&](const char *text, int y, lv_event_cb_t callback,
                                  lv_obj_t **labelOut = nullptr)
    {
        lv_obj_t *button = lv_btn_create(screen);
        lv_obj_set_size(button, layout.width(82), layout.height(56));
        lv_obj_align(button, LV_ALIGN_TOP_RIGHT, layout.x(-28), layout.y(y));
        lv_obj_set_style_bg_color(button, lv_color_hex(0x263746), LV_PART_MAIN);
        lv_obj_set_style_radius(button, 14, LV_PART_MAIN);
        lv_obj_set_style_border_color(button, lv_color_hex(0x3C566B), LV_PART_MAIN);
        lv_obj_set_style_border_width(button, 1, LV_PART_MAIN);
        lv_obj_set_style_pad_all(button, 0, LV_PART_MAIN);
        lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, this);

        lv_obj_t *label = lv_label_create(button);
        lv_label_set_text(label, text);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 37);
        if (labelOut)
            *labelOut = label;
        return button;
    };

    turnoutButton = makeNavigationTile("TURNOUT", 90, turnoutButtonEvent);
    lv_obj_t *turnoutIcon = lv_label_create(turnoutButton);
    lv_label_set_text(turnoutIcon, "Y");
    lv_obj_set_style_text_font(turnoutIcon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(turnoutIcon, lv_color_hex(0xCCD7E0), 0);
    lv_obj_align(turnoutIcon, LV_ALIGN_TOP_MID, 0, 3);

    powerButton = makeNavigationTile("POWER", 162, powerButtonEvent, &powerLabel);
    lv_obj_set_style_text_color(powerLabel, lv_color_hex(0x8899AA), 0);
    lv_obj_t *powerRing = lv_obj_create(powerButton);
    lv_obj_set_size(powerRing, 20, 20);
    lv_obj_set_style_bg_opa(powerRing, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(powerRing, lv_color_hex(0xCCD7E0), 0);
    lv_obj_set_style_border_width(powerRing, 3, 0);
    lv_obj_set_style_radius(powerRing, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(powerRing, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_t *powerStem = lv_obj_create(powerButton);
    lv_obj_set_size(powerStem, 3, 12);
    lv_obj_set_style_bg_color(powerStem, lv_color_hex(0xCCD7E0), 0);
    lv_obj_set_style_border_width(powerStem, 0, 0);
    lv_obj_align(powerStem, LV_ALIGN_TOP_MID, 0, 1);

    routeButton = makeNavigationTile("ROUTES", 234, routeButtonEvent);
    auto routeNode = [&](int x, int y)
    {
        lv_obj_t *node = lv_obj_create(routeButton);
        lv_obj_set_size(node, 7, 7);
        lv_obj_set_style_bg_color(node, lv_color_hex(0xCCD7E0), 0);
        lv_obj_set_style_border_width(node, 0, 0);
        lv_obj_set_style_radius(node, LV_RADIUS_CIRCLE, 0);
        lv_obj_align(node, LV_ALIGN_TOP_MID, x, y);
    };
    auto routeSegment = [&](int width, int x, int y)
    {
        lv_obj_t *segment = lv_obj_create(routeButton);
        lv_obj_set_size(segment, width, 2);
        lv_obj_set_style_bg_color(segment, lv_color_hex(0xCCD7E0), 0);
        lv_obj_set_style_border_width(segment, 0, 0);
        lv_obj_align(segment, LV_ALIGN_TOP_MID, x, y);
    };
    routeNode(-14, 8);
    routeNode(0, 16);
    routeNode(14, 8);
    routeSegment(15, -7, 13);
    routeSegment(15, 7, 13);

    lvgl_port_unlock();
}

void ThrottleUI::update(
    const Locomotive &locomotive,
    bool speedMode)
{
    lvgl_port_lock(-1);

    char buffer[16];

    // ---------------------------------------------
    // Locomotive address
    // ---------------------------------------------

    snprintf(
        buffer,
        sizeof(buffer),
        "%u",
        locomotive.address
    );

    lv_label_set_text(addressLabel, buffer);

    // ---------------------------------------------
    // Direction
    // ---------------------------------------------

    if (locomotive.directionForward)
    {
        lv_label_set_text(directionArrowLabel, LV_SYMBOL_RIGHT);

        lv_obj_set_style_text_color(
            directionArrowLabel,
            lv_color_hex(0xFFFFFF),
            0
        );

    }
    else
    {
        lv_label_set_text(directionArrowLabel, LV_SYMBOL_LEFT);

        lv_obj_set_style_text_color(
            directionArrowLabel,
            lv_color_hex(0xFFFFFF),
            0
        );

    }

    updateFunctionAppearance(locomotive);

    for (uint8_t i = 0; i < FUNCTION_SLOT_COUNT; i++)
    {
        FunctionSlot &slot = functionSlots[i];

        if (!slot.assigned)
        {
            continue;
        }

        uint8_t function = slot.function;

        if (locomotive.functionStates[function])
        {
            lv_obj_set_style_bg_color(
                slot.button,
                lv_color_hex(0xFFD740),
                LV_PART_MAIN
            );

            lv_obj_set_style_text_color(
                slot.label,
                lv_color_hex(0x101010),
                0
            );
        }
        else
        {
            lv_obj_set_style_bg_color(
                slot.button,
                lv_color_hex(0x263746),
                LV_PART_MAIN
            );

            lv_obj_set_style_text_color(
                slot.label,
                lv_color_hex(0xFFFFFF),
                0
            );
        }
    }
    

    // ---------------------------------------------
    // Current control mode
    // ---------------------------------------------

    if (speedMode)
    {
        lv_label_set_text(modeLabel, "SPEED");

        lv_arc_set_value(speedArc, locomotive.speed);

        snprintf(
            buffer,
            sizeof(buffer),
            "%u",
            locomotive.speed
        );

        lv_label_set_text(valueLabel, buffer);

        lv_obj_clear_flag(speedArc, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_label_set_text(modeLabel, "ADDRESS");

        snprintf(
            buffer,
            sizeof(buffer),
            "%u",
            locomotive.address
        );

        lv_label_set_text(valueLabel, buffer);

        lv_obj_add_flag(speedArc, LV_OBJ_FLAG_HIDDEN);
    }

    lvgl_port_unlock();
}

void ThrottleUI::setFunctionCallback(FunctionCallback callback)
{
    functionCallback = callback;
}

void ThrottleUI::functionButtonEvent(lv_event_t *event)
{
    ThrottleUI *ui =
        static_cast<ThrottleUI *>(lv_event_get_user_data(event));

    if (ui == nullptr || ui->functionCallback == nullptr)
    {
        return;
    }

    lv_obj_t *button = lv_event_get_target(event);

    for (uint8_t i = 0; i < FUNCTION_SLOT_COUNT; i++)
    {
        if (ui->functionSlots[i].button == button &&
            ui->functionSlots[i].assigned)
        {
            const uint8_t function = ui->functionSlots[i].function;
            const bool momentary = ui->displayedLocomotive &&
                ui->displayedLocomotive->fromRoster &&
                ui->displayedLocomotive->functionDefinitions[function].momentary;
            const lv_event_code_t code = lv_event_get_code(event);

            if (momentary && code == LV_EVENT_PRESSED)
                ui->functionCallback(function, true);
            else if (momentary && (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST))
                ui->functionCallback(function, false);
            else if (!momentary && code == LV_EVENT_CLICKED)
                ui->functionCallback(function, true);

            return;
        }
    }
}

void ThrottleUI::functionGestureEvent(lv_event_t *event)
{
    auto *ui = static_cast<ThrottleUI *>(lv_event_get_user_data(event));
    if (ui == nullptr || ui->displayedLocomotive == nullptr)
        return;

    lv_indev_t *indev = lv_indev_get_act();
    if (indev == nullptr)
        return;

    const uint8_t pageCount =
        (ui->availableFunctionCount + FUNCTION_SLOT_COUNT - 1) / FUNCTION_SLOT_COUNT;
    const lv_dir_t direction = lv_indev_get_gesture_dir(indev);

    if (direction == LV_DIR_LEFT && ui->functionPage + 1 < pageCount)
        ++ui->functionPage;
    else if (direction == LV_DIR_RIGHT && ui->functionPage > 0)
        --ui->functionPage;
    else
        return;

    ui->updateFunctionSlots(*ui->displayedLocomotive);
    ui->updateFunctionAppearance(*ui->displayedLocomotive);
    if (ui->functionPageCallback)
        ui->functionPageCallback(ui->functionPage);
}

void ThrottleUI::updateFunctionSlots(
    const Locomotive &locomotive)
{
    const UiLayout layout(profile);

    if (locomotive.address != displayedAddress)
    {
        displayedAddress = locomotive.address;
        functionPage = 0;
    }
    displayedLocomotive = &locomotive;
    availableFunctionCount = FUNCTION_SLOT_COUNT;
    for (uint8_t function = 0; function < FUNCTION_SLOT_COUNT; ++function)
        availableFunctions[function] = function;

    const uint8_t pageCount = (availableFunctionCount + FUNCTION_SLOT_COUNT - 1) / FUNCTION_SLOT_COUNT;
    if (functionPage >= pageCount)
        functionPage = pageCount ? pageCount - 1 : 0;
    if (false)
    {
        char pageText[16];
        snprintf(pageText, sizeof(pageText), "%u / %u", functionPage + 1, pageCount);
        lv_label_set_text(pageLabel, pageText);
        lv_obj_clear_flag(previousPageButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(nextPageButton, LV_OBJ_FLAG_HIDDEN);
        if (functionPage == 0)
            lv_obj_add_state(previousPageButton, LV_STATE_DISABLED);
        else
            lv_obj_clear_state(previousPageButton, LV_STATE_DISABLED);
        if (functionPage + 1 == pageCount)
            lv_obj_add_state(nextPageButton, LV_STATE_DISABLED);
        else
            lv_obj_clear_state(nextPageButton, LV_STATE_DISABLED);
        lv_obj_clear_flag(pageLabel, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(previousPageButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(nextPageButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(pageLabel, LV_OBJ_FLAG_HIDDEN);
    }

    // Start with all slots unused
    for (uint8_t i = 0; i < FUNCTION_SLOT_COUNT; i++)
    {
        functionSlots[i].assigned = false;

        lv_obj_add_flag(
            functionSlots[i].button,
            LV_OBJ_FLAG_HIDDEN
        );
    }

    const uint8_t firstFunction = functionPage * FUNCTION_SLOT_COUNT;
    for (uint8_t slotIndex = 0;
         slotIndex < FUNCTION_SLOT_COUNT && firstFunction + slotIndex < availableFunctionCount;
         ++slotIndex)
    {
        FunctionSlot &slot = functionSlots[slotIndex];
        slot.function = availableFunctions[firstFunction + slotIndex];
        slot.assigned = true;
        const LocoFunction &definition = locomotive.functionDefinitions[slot.function];
        const char *pictogram = functionPictogram(slot.function, definition);
        String text = "F" + String(slot.function);
        if (definition.name.length() > 0)
        {
            text += " ";
            text += definition.name;
        }
        lv_label_set_text(slot.label, text.c_str());

        if (pictogram)
        {
            lv_label_set_text(slot.icon, pictogram);
            lv_obj_align(slot.icon, LV_ALIGN_TOP_MID, 0, layout.y(6));
            lv_obj_clear_flag(slot.icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(slot.label, LV_ALIGN_BOTTOM_MID, 0, layout.y(-7));
        }
        else
        {
            lv_obj_add_flag(slot.icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_center(slot.label);
        }
        lv_obj_clear_flag(slot.button, LV_OBJ_FLAG_HIDDEN);
    }
}

void ThrottleUI::setLocomotive(
    const Locomotive &locomotive)
{
    lvgl_port_lock(-1);

    updateFunctionSlots(locomotive);
    const bool hasRosterName = locomotive.fromRoster && !locomotive.name.isEmpty();
    lv_label_set_text(addressTitleLabel, hasRosterName ? locomotive.name.c_str() : "LOCO");
    lv_obj_set_style_text_color(addressTitleLabel,
        lv_color_hex(hasRosterName ? 0xE5F7ED : 0x8899AA), 0);
    updateFunctionAppearance(locomotive);

    lvgl_port_unlock();
}

void ThrottleUI::setFunctionPage(uint8_t page)
{
    functionPage = page;
    if (displayedLocomotive)
    {
        updateFunctionSlots(*displayedLocomotive);
        updateFunctionAppearance(*displayedLocomotive);
    }
}

void ThrottleUI::updateFunctionAppearance(const Locomotive &locomotive)
{
    for (uint8_t i = 0; i < FUNCTION_SLOT_COUNT; ++i)
    {
        FunctionSlot &slot = functionSlots[i];
        if (!slot.assigned)
            continue;
        const bool active = locomotive.functionStates[slot.function];
        lv_obj_set_style_bg_color(slot.button, active ? lv_color_hex(0xFFD740) : lv_color_hex(0x263746), LV_PART_MAIN);
        lv_obj_set_style_text_color(slot.label, active ? lv_color_hex(0x101010) : lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_color(slot.icon, active ? lv_color_hex(0x101010) : lv_color_hex(0xFFFFFF), 0);
    }
}

void ThrottleUI::previousPageEvent(lv_event_t *event)
{
    auto *ui = static_cast<ThrottleUI *>(lv_event_get_user_data(event));
    if (ui->functionPage > 0 && ui->displayedLocomotive)
    {
        --ui->functionPage;
        ui->updateFunctionSlots(*ui->displayedLocomotive);
        ui->updateFunctionAppearance(*ui->displayedLocomotive);
        if (ui->functionPageCallback)
            ui->functionPageCallback(ui->functionPage);
    }
}

void ThrottleUI::nextPageEvent(lv_event_t *event)
{
    auto *ui = static_cast<ThrottleUI *>(lv_event_get_user_data(event));
    const uint8_t pageCount = (ui->availableFunctionCount + FUNCTION_SLOT_COUNT - 1) / FUNCTION_SLOT_COUNT;
    if (ui->functionPage + 1 < pageCount && ui->displayedLocomotive)
    {
        ++ui->functionPage;
        ui->updateFunctionSlots(*ui->displayedLocomotive);
        ui->updateFunctionAppearance(*ui->displayedLocomotive);
        if (ui->functionPageCallback)
            ui->functionPageCallback(ui->functionPage);
    }
}

void ThrottleUI::selectionButtonEvent(lv_event_t *event)
{
    auto *ui = static_cast<ThrottleUI *>(lv_event_get_user_data(event));
    if (ui->selectionCallback)
        ui->selectionCallback();
}

void ThrottleUI::setConnectionStatus(const char *text, bool connected)
{
    lvgl_port_lock(-1);
    const char *compactStatus = connected ? "ONLINE" :
        (strstr(text, "SETUP") ? "SETUP" :
         (strstr(text, "WIFI") ? "WI-FI" : "CONNECTING"));
    lv_label_set_text(connectionLabel, compactStatus);
    lv_obj_set_style_text_color(connectionLabel,
        lv_color_hex(connected ? 0xE5F7ED : 0x8899AA), 0);
    lv_obj_set_style_text_color(connectionIndicator,
        lv_color_hex(connected ? 0x35E06F : 0x8899AA), 0);
    lvgl_port_unlock();
}

void ThrottleUI::setTrackPowerStatus(bool known, bool on)
{
    lvgl_port_lock(-1);
    const uint32_t color = !known ? 0x8899AA : on ? 0x35E06F : 0xFF7043;
    const uint32_t background = !known ? 0x263746 : on ? 0x1B5E3A : 0x8B1E1E;
    lv_obj_set_style_text_color(powerLabel, lv_color_hex(color), 0);
    lv_obj_set_style_bg_color(powerButton, lv_color_hex(background), 0);
    lvgl_port_unlock();
}

void ThrottleUI::connectionButtonEvent(lv_event_t *event)
{
    auto *ui = static_cast<ThrottleUI *>(lv_event_get_user_data(event));
    if (ui->connectionCallback)
        ui->connectionCallback();
}

void ThrottleUI::settingsButtonEvent(lv_event_t *event)
{
    auto *ui = static_cast<ThrottleUI *>(lv_event_get_user_data(event));
    if (ui->settingsCallback)
        ui->settingsCallback();
}

void ThrottleUI::turnoutButtonEvent(lv_event_t *event)
{
    auto *ui = static_cast<ThrottleUI *>(lv_event_get_user_data(event));
    if (ui->turnoutCallback)
        ui->turnoutCallback();
}

void ThrottleUI::powerButtonEvent(lv_event_t *event)
{
    auto *ui = static_cast<ThrottleUI *>(lv_event_get_user_data(event));
    if (ui->powerCallback)
        ui->powerCallback();
}

void ThrottleUI::routeButtonEvent(lv_event_t *event)
{
    auto *ui = static_cast<ThrottleUI *>(lv_event_get_user_data(event));
    if (ui->routeCallback)
        ui->routeCallback();
}

void ThrottleUI::directionButtonEvent(lv_event_t *event)
{
    auto *ui = static_cast<ThrottleUI *>(lv_event_get_user_data(event));
    if (ui->directionCallback)
        ui->directionCallback();
}

void ThrottleUI::stopButtonEvent(lv_event_t *event)
{
    auto *ui = static_cast<ThrottleUI *>(lv_event_get_user_data(event));
    if (ui->stopCallback)
        ui->stopCallback();
}

void ThrottleUI::emergencyStopButtonEvent(lv_event_t *event)
{
    auto *ui = static_cast<ThrottleUI *>(lv_event_get_user_data(event));
    if (ui->emergencyStopCallback)
        ui->emergencyStopCallback();
}

void ThrottleUI::speedPresetButtonEvent(lv_event_t *event)
{
    auto *ui = static_cast<ThrottleUI *>(lv_event_get_user_data(event));
    if (ui == nullptr || ui->speedPresetCallback == nullptr)
        return;

    const lv_obj_t *button = lv_event_get_target(event);
    if (button == ui->speedPreset50Button)
        ui->speedPresetCallback(50);
    else if (button == ui->speedPreset75Button)
        ui->speedPresetCallback(75);
}

void ThrottleUI::moreFunctionsEvent(lv_event_t *event)
{
    auto *ui = static_cast<ThrottleUI *>(lv_event_get_user_data(event));
    if (ui && ui->moreFunctionsCallback) ui->moreFunctionsCallback();
}
