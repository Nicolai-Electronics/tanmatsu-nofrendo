#include "help.h"
#include "bsp/input.h"
#include "common/display.h"
#include "freertos/idf_additions.h"
#include "gui_style.h"
#include "icons.h"
#include "message_dialog.h"
#include "pax_gfx.h"
#include "pax_matrix.h"
#include "pax_text.h"
#include "pax_types.h"
#include "theme.h"

#define FOOTER_LEFT  ((gui_element_icontext_t[]){{get_icon(ICON_ESC), "Back"}}), 1
#define FOOTER_RIGHT NULL, 0

static void render(bool partial, bool icons) {
    pax_buf_t*   buffer = display_get_pax_buffer();
    gui_theme_t* theme  = get_theme();

    int header_height = theme->header.height + (theme->header.vertical_margin * 2);
    int footer_height = theme->footer.height + (theme->footer.vertical_margin * 2);

    pax_vec2_t position = {
        .x0 = theme->menu.horizontal_margin + theme->menu.horizontal_padding,
        .y0 = header_height + theme->menu.vertical_margin + theme->menu.vertical_padding,
        .x1 = pax_buf_get_width(buffer) - theme->menu.horizontal_margin - theme->menu.horizontal_padding,
        .y1 = pax_buf_get_height(buffer) - footer_height - theme->menu.vertical_margin - theme->menu.vertical_padding,
    };

    if (!partial || icons) {
        render_base_screen_statusbar(buffer, theme, !partial, !partial || icons, !partial,
                                     ((gui_element_icontext_t[]){{get_icon(ICON_HELP), "Help"}}), 1, FOOTER_LEFT,
                                     FOOTER_RIGHT);
    }
    if (!partial) {
        gui_element_icontext_t lines1[11] = {
            {get_icon(ICON_HELP), "In game controls:"},
            {get_icon(ICON_F1), " Select"},
            {get_icon(ICON_F2), " Start"},
            {get_icon(ICON_F5), " Exit"},
            {get_icon(ICON_F6), " Emulator menu"},
            {NULL, "       Left shift: A button"},
            {NULL, "       Z: B button"},
            {NULL, ""},
            {NULL, "D-Pad: Arrow keys"},
            {NULL, "/ and right shift for"},
            {NULL, "up + left and up + right"},
        };

        gui_element_icontext_t lines2[10] = {
            {get_icon(ICON_APP), "Nofrendo NES emulator"},      {NULL, "Matt Conte <zeus@ztnet.com>"},
            {NULL, "Neil Stevens<neil @qualityassistant.com>"}, {NULL, "Firebug<firebug @cfl.rr.com>"},
            {NULL, "Benjamin C.W.Sittler<bsittler @nmt.edu>"},  {NULL, "The Mighty Mike Master<melanson @pcisys.net>"},
            {NULL, "Jeroen Domburg<jeroen @espressif.com>"},    {get_icon(ICON_EXTENSION), "Tanmatsu port"},
            {NULL, "Copyright 2025 Ranzbak Badge.Team"},        {NULL, "Copyright 2025 Nicolai Electronics"},
        };

        for (size_t i = 0; i < 11; i++) {
            gui_icontext_draw(buffer, &theme->header, position.x0, position.y0 + 32 * i, &lines1[i], 0, 32);
        }

        for (size_t i = 0; i < 10; i++) {
            gui_icontext_draw(buffer, &theme->header, position.x0 + 256, position.y0 + 32 * i, &lines2[i], 0, 32);
        }
    }
    display_blit_buffer(buffer);
}

void menu_help(void) {
    QueueHandle_t input_event_queue = NULL;
    ESP_ERROR_CHECK(bsp_input_get_queue(&input_event_queue));

    render(false, true);
    while (1) {
        bsp_input_event_t event;
        if (xQueueReceive(input_event_queue, &event, pdMS_TO_TICKS(1000)) == pdTRUE) {
            switch (event.type) {
                case INPUT_EVENT_TYPE_NAVIGATION: {
                    if (event.args_navigation.state) {
                        switch (event.args_navigation.key) {
                            case BSP_INPUT_NAVIGATION_KEY_ESC:
                            case BSP_INPUT_NAVIGATION_KEY_F1:
                            case BSP_INPUT_NAVIGATION_KEY_GAMEPAD_B:
                                return;
                            default:
                                break;
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        } else {
            render(true, true);
        }
    }
}
