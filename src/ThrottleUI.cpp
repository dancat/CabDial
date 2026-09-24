#include "ThrottleUI.h"
#include "UiLayout.h"
#include "lvgl_v8_port.h"
#include "Locomotive.h"

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
    // Connection status - top, inside the usable circular area.
    // -------------------------------------------------

    connectionButton = lv_btn_create(screen);
    lv_obj_set_size(connectionButton, layout.width(170), layout.height(34));
    lv_obj_align(connectionButton, LV_ALIGN_TOP_MID, layout.x(-35), layout.y(32));
    lv_obj_set_style_bg_color(connectionButton, lv_color_hex(0x263746), 0);
    lv_obj_add_event_cb(connectionButton, connectionButtonEvent, LV_EVENT_CLICKED, this);
    navLabel = connectionLabel = lv_label_create(connectionButton);
    lv_obj_set_width(connectionLabel, layout.width(128));
    lv_label_set_long_mode(connectionLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(connectionLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(connectionLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(connectionLabel, lv_color_hex(0x667788), 0);
    lv_label_set_text(connectionLabel, "CONNECTION SETUP REQUIRED");
    lv_obj_align(connectionLabel, LV_ALIGN_RIGHT_MID, -5, 0);

    // Network pictogram: two endpoints joined by a link.
    lv_obj_t *networkLink = lv_obj_create(connectionButton);
    lv_obj_set_size(networkLink, 15, 2);
    lv_obj_align(networkLink, LV_ALIGN_LEFT_MID, 11, 0);
    lv_obj_set_style_bg_color(networkLink, lv_color_hex(0x29B6F6), 0);
    lv_obj_set_style_border_width(networkLink, 0, 0);
    lv_obj_t *networkLeft = lv_obj_create(connectionButton);
    lv_obj_set_size(networkLeft, 7, 7);
    lv_obj_align(networkLeft, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_set_style_bg_color(networkLeft, lv_color_hex(0x29B6F6), 0);
    lv_obj_set_style_border_width(networkLeft, 0, 0);
    lv_obj_set_style_radius(networkLeft, LV_RADIUS_CIRCLE, 0);
    lv_obj_t *networkRight = lv_obj_create(connectionButton);
    lv_obj_set_size(networkRight, 7, 7);
    lv_obj_align(networkRight, LV_ALIGN_LEFT_MID, 22, 0);
    lv_obj_set_style_bg_color(networkRight, lv_color_hex(0x29B6F6), 0);
    lv_obj_set_style_border_width(networkRight, 0, 0);
    lv_obj_set_style_radius(networkRight, LV_RADIUS_CIRCLE, 0);

    settingsButton = lv_btn_create(screen);
    lv_obj_set_size(settingsButton, layout.width(42), layout.height(34));
    lv_obj_align(settingsButton, LV_ALIGN_TOP_MID, layout.x(90), layout.y(32));
    lv_obj_set_style_bg_color(settingsButton, lv_color_hex(0x263746), 0);
    lv_obj_add_event_cb(settingsButton, settingsButtonEvent, LV_EVENT_CLICKED, this);
    auto cogPart = [](lv_obj_t *parent, int width, int height, int x, int y)
    {
        lv_obj_t *part = lv_obj_create(parent);
        lv_obj_set_size(part, width, height);
        lv_obj_align(part, LV_ALIGN_CENTER, x, y);
        lv_obj_set_style_bg_color(part, lv_color_hex(0xCCD7E0), 0);
        lv_obj_set_style_border_width(part, 0, 0);
        lv_obj_set_style_radius(part, 1, 0);
        return part;
    };
    // A compact cogwheel: hub, four teeth, and four diagonal teeth.
    lv_obj_t *cogHub = cogPart(settingsButton, 13, 13, 0, 0);
    lv_obj_set_style_radius(cogHub, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(cogHub, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cogHub, 3, 0);
    lv_obj_set_style_border_color(cogHub, lv_color_hex(0xCCD7E0), 0);
    cogPart(settingsButton, 4, 6, 0, -11);
    cogPart(settingsButton, 4, 6, 0, 11);
    cogPart(settingsButton, 6, 4, -11, 0);
    cogPart(settingsButton, 6, 4, 11, 0);
    cogPart(settingsButton, 4, 4, -8, -8);
    cogPart(settingsButton, 4, 4, 8, -8);
    cogPart(settingsButton, 4, 4, -8, 8);
    cogPart(settingsButton, 4, 4, 8, 8);

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
        25
    );

    // -------------------------------------------------
    // Main value - center
    // -------------------------------------------------

    // -------------------------------------------------
    // Speed arc
    // -------------------------------------------------

    speedArc = lv_arc_create(screen);

    lv_obj_set_size(speedArc, layout.width(245), layout.height(245));
    lv_obj_align(speedArc, LV_ALIGN_CENTER, 0, layout.y(-10));

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
        -25
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
        25
    );
    lv_obj_add_flag(rangeLabel, LV_OBJ_FLAG_HIDDEN);

    stopButton = lv_btn_create(screen);
    lv_obj_set_size(stopButton, layout.width(62), layout.height(34));
    lv_obj_align(stopButton, LV_ALIGN_TOP_MID, layout.x(-70), layout.y(335));
    lv_obj_set_style_bg_color(stopButton, lv_color_hex(0x8B1E1E), 0);
    lv_obj_add_event_cb(stopButton, stopButtonEvent, LV_EVENT_CLICKED, this);
    lv_obj_t *stopIcon = lv_obj_create(stopButton);
    lv_obj_set_size(stopIcon, 16, 16);
    lv_obj_set_style_bg_color(stopIcon, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(stopIcon, 0, 0);
    lv_obj_set_style_radius(stopIcon, 2, 0);
    lv_obj_center(stopIcon);

    emergencyStopButton = lv_btn_create(screen);
    lv_obj_set_size(emergencyStopButton, layout.width(62), layout.height(34));
    lv_obj_align(emergencyStopButton, LV_ALIGN_TOP_MID, layout.x(70), layout.y(335));
    lv_obj_set_style_bg_color(emergencyStopButton, lv_color_hex(0xD32F2F), 0);
    lv_obj_add_event_cb(emergencyStopButton, emergencyStopButtonEvent, LV_EVENT_CLICKED, this);
    lv_obj_t *emergencyStopLabel = lv_label_create(emergencyStopButton);
    lv_label_set_text(emergencyStopLabel, "!");
    lv_obj_set_style_text_font(emergencyStopLabel, &lv_font_montserrat_24, 0);
    lv_obj_center(emergencyStopLabel);

    // -------------------------------------------------
    // Locomotive - left
    // -------------------------------------------------

    lv_obj_t *selectionButton = lv_btn_create(screen);
    lv_obj_set_size(selectionButton, layout.width(106), layout.height(110));
    lv_obj_align(selectionButton, LV_ALIGN_LEFT_MID, layout.x(10), layout.y(-10));
    lv_obj_set_style_bg_opa(selectionButton, LV_OPA_TRANSP, LV_PART_MAIN);
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

    lv_obj_align(
        addressTitleLabel,
        LV_ALIGN_TOP_MID,
        0,
        10
    );

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

    nameLabel = lv_label_create(selectionButton);
    lv_obj_set_width(nameLabel, layout.width(96));
    lv_label_set_long_mode(nameLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(nameLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(nameLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(nameLabel, lv_color_hex(0x8899AA), 0);
    lv_label_set_text(nameLabel, "");
    lv_obj_align(nameLabel, LV_ALIGN_BOTTOM_MID, 0, -2);

    lv_obj_align(
        addressLabel,
        LV_ALIGN_CENTER,
        0,
        7
    );

    // -------------------------------------------------
    // Direction toggle - centered between the two stop controls
    // -------------------------------------------------

    directionButton = lv_btn_create(screen);
    lv_obj_set_size(directionButton, layout.width(62), layout.height(34));
    lv_obj_align(directionButton, LV_ALIGN_TOP_MID, 0, layout.y(335));
    lv_obj_set_style_bg_color(directionButton, lv_color_hex(0x263746), 0);
    lv_obj_add_event_cb(directionButton, directionButtonEvent, LV_EVENT_CLICKED, this);

    directionArrowLabel = lv_label_create(directionButton);

    lv_obj_set_style_text_font(
        directionArrowLabel,
        &lv_font_montserrat_24,
        0
    );

    lv_obj_set_style_text_color(
        directionArrowLabel,
        lv_color_hex(0x35E06F),
        0
    );

    lv_label_set_text(directionArrowLabel, ">");

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
        -120,
        -60,
        0,
        60,
        120
    };
    for (uint8_t i = 0; i < FUNCTION_SLOT_COUNT; i++)
    {
        FunctionSlot &slot = functionSlots[i];

        slot.button = lv_btn_create(screen);

        lv_obj_set_size(
            slot.button,
            layout.width(52),
            layout.height(40)
        );

        lv_obj_align(
            slot.button,
            LV_ALIGN_TOP_MID,
            layout.x(functionX[i]),
            layout.y(390)
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

        slot.label = lv_label_create(slot.button);

        lv_obj_set_style_text_font(
            slot.label,
            &lv_font_montserrat_16,
            0
        );

        lv_obj_set_style_text_color(
            slot.label,
            lv_color_hex(0xFFFFFF),
            0
        );

        lv_obj_center(slot.label);

        lv_obj_add_event_cb(slot.button, functionButtonEvent, LV_EVENT_ALL, this);

        // Hidden until assigned
        lv_obj_add_flag(
            slot.button,
            LV_OBJ_FLAG_HIDDEN
        );
    }

    previousPageButton = lv_btn_create(screen);
    lv_obj_set_size(previousPageButton, layout.width(48), layout.height(28));
    lv_obj_align(previousPageButton, LV_ALIGN_TOP_MID, layout.x(-88), layout.y(306));
    lv_obj_set_style_bg_color(previousPageButton, lv_color_hex(0x263746), 0);
    lv_obj_t *previousLabel = lv_label_create(previousPageButton);
    lv_label_set_text(previousLabel, "<");
    lv_obj_center(previousLabel);
    lv_obj_add_event_cb(previousPageButton, previousPageEvent, LV_EVENT_CLICKED, this);

    nextPageButton = lv_btn_create(screen);
    lv_obj_set_size(nextPageButton, layout.width(48), layout.height(28));
    lv_obj_align(nextPageButton, LV_ALIGN_TOP_MID, layout.x(88), layout.y(306));
    lv_obj_set_style_bg_color(nextPageButton, lv_color_hex(0x263746), 0);
    lv_obj_t *nextLabel = lv_label_create(nextPageButton);
    lv_label_set_text(nextLabel, ">");
    lv_obj_center(nextLabel);
    lv_obj_add_event_cb(nextPageButton, nextPageEvent, LV_EVENT_CLICKED, this);

    pageLabel = lv_label_create(screen);
    lv_obj_set_style_text_font(pageLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(pageLabel, lv_color_hex(0x8899AA), 0);
    lv_obj_align(pageLabel, LV_ALIGN_TOP_MID, 0, layout.y(312));
    
    turnoutButton = lv_btn_create(screen);
    lv_obj_set_size(turnoutButton, layout.width(80), layout.height(32));
    lv_obj_align(turnoutButton, LV_ALIGN_TOP_RIGHT, layout.x(-25), layout.y(140));
    lv_obj_set_style_bg_color(turnoutButton, lv_color_hex(0x263746), 0);
    lv_obj_add_event_cb(turnoutButton, turnoutButtonEvent, LV_EVENT_CLICKED, this);
    lv_obj_t *turnoutLabel = lv_label_create(turnoutButton);
    lv_label_set_text(turnoutLabel, "TURNOUT");
    lv_obj_set_style_text_font(turnoutLabel, &lv_font_montserrat_14, 0);
    lv_obj_center(turnoutLabel);

    powerButton = lv_btn_create(screen);
    lv_obj_set_size(powerButton, layout.width(80), layout.height(32));
    lv_obj_align(powerButton, LV_ALIGN_TOP_RIGHT, layout.x(-25), layout.y(190));
    lv_obj_set_style_bg_color(powerButton, lv_color_hex(0x263746), 0);
    lv_obj_add_event_cb(powerButton, powerButtonEvent, LV_EVENT_CLICKED, this);
    powerLabel = lv_label_create(powerButton);
    lv_label_set_text(powerLabel, "POWER");
    lv_obj_set_style_text_font(powerLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(powerLabel, lv_color_hex(0x8899AA), 0);
    lv_obj_center(powerLabel);

    routeButton = lv_btn_create(screen);
    lv_obj_set_size(routeButton, layout.width(80), layout.height(32));
    lv_obj_align(routeButton, LV_ALIGN_TOP_RIGHT, layout.x(-25), layout.y(240));
    lv_obj_set_style_bg_color(routeButton, lv_color_hex(0x263746), 0);
    lv_obj_add_event_cb(routeButton, routeButtonEvent, LV_EVENT_CLICKED, this);
    lv_obj_t *routeLabel = lv_label_create(routeButton);
    lv_label_set_text(routeLabel, "ROUTES");
    lv_obj_set_style_text_font(routeLabel, &lv_font_montserrat_14, 0);
    lv_obj_center(routeLabel);

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
        lv_label_set_text(directionArrowLabel, ">");

        lv_obj_set_style_text_color(
            directionArrowLabel,
            lv_color_hex(0x35E06F),
            0
        );

    }
    else
    {
        lv_label_set_text(directionArrowLabel, "<");

        lv_obj_set_style_text_color(
            directionArrowLabel,
            lv_color_hex(0xFF7043),
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

void ThrottleUI::updateFunctionSlots(
    const Locomotive &locomotive)
{
    if (locomotive.address != displayedAddress)
    {
        displayedAddress = locomotive.address;
        functionPage = 0;
    }
    displayedLocomotive = &locomotive;
    availableFunctionCount = 0;
    if (locomotive.fromRoster)
    {
        for (uint8_t function = 0; function < MAX_LOCO_FUNCTIONS; ++function)
        {
            if (locomotive.functionDefinitions[function].available)
                availableFunctions[availableFunctionCount++] = function;
        }
    }
    else
    {
        for (uint8_t function = 0; function < FUNCTION_SLOT_COUNT; ++function)
            availableFunctions[availableFunctionCount++] = function;
    }

    const uint8_t pageCount = (availableFunctionCount + FUNCTION_SLOT_COUNT - 1) / FUNCTION_SLOT_COUNT;
    if (functionPage >= pageCount)
        functionPage = pageCount ? pageCount - 1 : 0;
    if (pageCount > 1)
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
        char buffer[8];
        snprintf(buffer, sizeof(buffer), "F%u", slot.function);
        lv_label_set_text(slot.label, buffer);
        lv_obj_clear_flag(slot.button, LV_OBJ_FLAG_HIDDEN);
    }
}

void ThrottleUI::setLocomotive(
    const Locomotive &locomotive)
{
    lvgl_port_lock(-1);

    updateFunctionSlots(locomotive);
    lv_label_set_text(nameLabel, locomotive.name.c_str());
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
    lv_label_set_text(connectionLabel, text);
    lv_obj_set_style_bg_color(connectionButton,
        lv_color_hex(connected ? 0x1B5E3A : 0x263746), 0);
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
