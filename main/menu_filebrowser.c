#include "menu_filebrowser.h"
#include <dirent.h>
#include <string.h>
#include "bsp/device.h"
#include "bsp/input.h"
#include "display.h"
#include "gui_menu.h"
#include "gui_style.h"
#include "icons.h"
#include "message_dialog.h"
#include "pax_gfx.h"
#include "pax_matrix.h"
#include "pax_types.h"
#include "theme.h"
#include "bsp/audio.h"

static void render(menu_t* menu, pax_vec2_t position, bool partial, bool icons, const char* title, bool sd_available) {
    pax_buf_t*   buffer = display_get_pax_buffer();
    gui_theme_t* theme  = get_theme();

    if (!partial) {
        pax_background(buffer, theme->palette.color_background);
    }

    if (!partial || icons) {
        render_base_screen_statusbar(buffer, theme, !partial, !partial || icons, !partial,
                                     ((gui_element_icontext_t[]){{get_icon(ICON_SD_CARD), (char*)title}}), 1,
                                     ((gui_element_icontext_t[]){{sd_available ? get_icon(ICON_ESC) : NULL, sd_available ? "Switch internal / SD card" : "(No SD card)"},
                                                                 {get_icon(ICON_F1), "Quit app"},
                                                                 {get_icon(ICON_F2), "Help"}}),
                                     3, ((gui_element_icontext_t[]){{NULL, "↑ / ↓ | ⏎ Select"}}), 1);
    }
    menu_render(buffer, menu, position, theme, partial);
    display_blit_buffer(buffer);
}

static size_t populate_menu(const char* path, menu_t* menu, const char* filter[], size_t filter_length) {
    DIR* dir = opendir(path);
    if (dir == NULL) {
        return 0;
    }
    struct dirent* entry;
    size_t         count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            menu_insert_item_icon(menu, entry->d_name, NULL, (void*)1, -1, get_icon(ICON_FOLDER));
        } else {
            bool matches_filter = false;
            for (size_t i = 0; i < filter_length; i++) {
                if (strlen(entry->d_name) > strlen(filter[i]) &&
                    strcmp(entry->d_name + strlen(entry->d_name) - strlen(filter[i]), filter[i]) == 0) {
                    matches_filter = true;
                    break;
                }
            }
            if (matches_filter || filter_length == 0) {
                menu_insert_item_icon(menu, entry->d_name, NULL, (void*)0, -1, get_icon(ICON_INFO));
            }
        }
    }
    closedir(dir);
    return count;
}

static bool power_button_latch = false;

menu_filebrowser_result_t menu_filebrowser(const char* in_path, const char* filter[], size_t filter_length,
                                           char* out_filename, size_t filename_size, const char* title, bool sd_available) {
    QueueHandle_t input_event_queue = NULL;
    ESP_ERROR_CHECK(bsp_input_get_queue(&input_event_queue));

    char path[260] = {0};
    strncpy(path, in_path, sizeof(path) - 1);

    while (1) {
        menu_t menu = {0};
        menu_initialize(&menu);

        if (strcmp(path, "/sd") != 0 && strcmp(path, "/int") != 0) {
            menu_insert_item_icon(&menu, "..", NULL, (void*)0, -1, get_icon(ICON_FOLDER));
        }
        populate_menu(path, &menu, filter, filter_length);

        pax_buf_t*   buffer = display_get_pax_buffer();
        gui_theme_t* theme  = get_theme();

        int header_height = theme->header.height + (theme->header.vertical_margin * 2);
        int footer_height = theme->footer.height + (theme->footer.vertical_margin * 2);

        pax_vec2_t position = {
            .x0 = theme->menu.horizontal_margin + theme->menu.horizontal_padding,
            .y0 = header_height + theme->menu.vertical_margin + theme->menu.vertical_padding,
            .x1 = pax_buf_get_width(buffer) - theme->menu.horizontal_margin - theme->menu.horizontal_padding,
            .y1 =
                pax_buf_get_height(buffer) - footer_height - theme->menu.vertical_margin - theme->menu.vertical_padding,
        };

        render(&menu, position, false, true, title, sd_available);
        bool reload = false;
        while (!reload) {
            bsp_input_event_t event;
            if (xQueueReceive(input_event_queue, &event, pdMS_TO_TICKS(1000)) == pdTRUE) {
                switch (event.type) {
                    case INPUT_EVENT_TYPE_NAVIGATION: {
                        if (event.args_navigation.state) {
                            switch (event.args_navigation.key) {
                                case BSP_INPUT_NAVIGATION_KEY_F1:
                                    return MENU_FILEBROWSER_RESULT_EXIT;
                                    break;
                                case BSP_INPUT_NAVIGATION_KEY_F2:
                                    return MENU_FILEBROWSER_RESULT_HELP;
                                    break;
                                case BSP_INPUT_NAVIGATION_KEY_F4: {
                                    uint8_t brightness = 0;
                                    bsp_input_get_backlight_brightness(&brightness);
                                    bsp_input_set_backlight_brightness((brightness > 0) ? 0 : 100);
                                    break;
                                }
                                case BSP_INPUT_NAVIGATION_KEY_ESC:
                                case BSP_INPUT_NAVIGATION_KEY_GAMEPAD_B:
                                    menu_free(&menu);
                                    return MENU_FILEBROWSER_RESULT_CANCEL;
                                case BSP_INPUT_NAVIGATION_KEY_UP:
                                    menu_navigate_previous(&menu);
                                    render(&menu, position, true, false, title, sd_available);
                                    break;
                                case BSP_INPUT_NAVIGATION_KEY_DOWN:
                                    menu_navigate_next(&menu);
                                    render(&menu, position, true, false, title, sd_available);
                                    break;
                                case BSP_INPUT_NAVIGATION_KEY_RETURN:
                                case BSP_INPUT_NAVIGATION_KEY_GAMEPAD_A:
                                case BSP_INPUT_NAVIGATION_KEY_JOYSTICK_PRESS: {
                                    void*       arg   = menu_get_callback_args(&menu, menu_get_position(&menu));
                                    const char* label = menu_get_label(&menu, menu_get_position(&menu));
                                    if (strncmp(label, "..", 2) == 0) {
                                        // Go up one directory
                                        char* last_slash = strrchr(path, '/');
                                        if (last_slash != NULL) {
                                            *last_slash = '\0';
                                        }
                                        reload = true;
                                    } else {
                                        bool is_dir = (bool)arg;
                                        if (is_dir) {
                                            // Navigate into directory
                                            if (strlen(path) + strlen(label) + 2 < sizeof(path)) {
                                                strcat(path, "/");
                                                strcat(path, label);
                                                reload = true;
                                            } else {
                                                message_dialog(get_icon(ICON_ERROR), "Error",
                                                               "Path too long, can not navigate into directory",
                                                               "Go back");
                                                render(&menu, position, false, true, title, sd_available);
                                            }
                                        } else {
                                            snprintf(out_filename, filename_size, "%s/%s", path, label);
                                            menu_free(&menu);
                                            return MENU_FILEBROWSER_RESULT_SELECTED;  // File selected
                                        }
                                    }
                                    break;
                                }
                                default:
                                    break;
                            }
                        }
                        break;
                    }
                    case INPUT_EVENT_TYPE_ACTION: {
                        switch (event.args_action.type) {
                            case BSP_INPUT_ACTION_TYPE_POWER_BUTTON:
                                if (event.args_action.state) {
                                    power_button_latch = true;
                                } else if (power_button_latch) {
                                    power_button_latch = false;
                                    bsp_device_restart_to_launcher();
                                }
                                break;
                            case BSP_INPUT_ACTION_TYPE_SD_CARD:
                                // tbd
                                break;
                            case BSP_INPUT_ACTION_TYPE_AUDIO_JACK:
                                bsp_audio_set_amplifier(!event.args_action.state);
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                    default:
                        break;
                }
            } else {
                render(&menu, position, true, true, title, sd_available);
            }
        }

        menu_free(&menu);
    }

    return MENU_FILEBROWSER_RESULT_CANCEL;
}
