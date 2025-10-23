#include <nofrendo.h>
#include <stdio.h>
#include <sys/stat.h>
#include "bsp/device.h"
#include "bsp/display.h"
#include "bsp/input.h"
#include "chakrapetchmedium.h"
#include "display.h"
#include "esp_err.h"
#include "esp_lcd_types.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/lcd_types.h"
#include "icons.h"
#include "led.h"
#include "menu/src/menu.h"
#include "menu_filebrowser.h"
#include "message_dialog.h"
#include "nvs_flash.h"
#include "pax_codecs.h"
#include "pax_fonts.h"
#include "pax_gfx.h"
#include "pax_text.h"
#include "sdcard.h"
#include "shapes/pax_misc.h"

static char* const TAG = "main";

static QueueHandle_t input_event_queue = NULL;
static wl_handle_t   wl_handle         = WL_INVALID_HANDLE;

int app_main(void) {
    // Start the GPIO interrupt service
    gpio_install_isr_service(0);

    // Initialize the Non Volatile Storage service
    esp_err_t res = nvs_flash_init();
    if (res == ESP_ERR_NVS_NO_FREE_PAGES || res == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        res = nvs_flash_init();
    }
    ESP_ERROR_CHECK(res);

    // Initialize the Board Support Package
    ESP_ERROR_CHECK(bsp_device_initialize());
    ESP_ERROR_CHECK(led_init());

    // Initialize the display
    ESP_ERROR_CHECK(display_init());

    // Fetch the input event queue
    ESP_ERROR_CHECK(bsp_input_get_queue(&input_event_queue));

    // Initialize icons
    // load_icons();

    // Display welcome message
    pax_buf_t* pax_buf = display_get_pax_buffer();
    pax_background(pax_buf, 0xFF000000);  // Black background
    pax_draw_text(pax_buf, 0xFFFFFFFF, &chakrapetchmedium, 22, 0, 0, "Tanmatsu NoFrendo NES Emulator");

    pax_draw_text(pax_buf, 0xFFFFFFFF, &chakrapetchmedium, 22, 0, 24 * 2, "[START] orange triangle");
    pax_draw_text(pax_buf, 0xFFFFFFFF, &chakrapetchmedium, 22, 0, 24 * 3, "[MENU ] purple diamond");
    pax_draw_text(pax_buf, 0xFFFFFFFF, &chakrapetchmedium, 22, 0, 24 * 4, "[A    ] left shift");
    pax_draw_text(pax_buf, 0xFFFFFFFF, &chakrapetchmedium, 22, 0, 24 * 5, "[B    ] z");
    pax_draw_text(pax_buf, 0xFFFFFFFF, &chakrapetchmedium, 22, 0, 24 * 6, "[D-PAD] up/down/left/right arrows");
    pax_draw_text(pax_buf, 0xFFFFFFFF, &chakrapetchmedium, 22, 0, 24 * 7, "        / = left and up arrows");
    pax_draw_text(pax_buf, 0xFFFFFFFF, &chakrapetchmedium, 22, 0, 24 * 8, "        right shift = right and up arrows");
    pax_draw_text(pax_buf, 0xFFFFFFFF, &chakrapetchmedium, 22, 0, 24 * 10,
                  "[ESC  ] switch between SD card and internal storage");
    display_blit();

    vTaskDelay(pdMS_TO_TICKS(2000));

    // Mount the internal FAT filesystem
    esp_vfs_fat_mount_config_t fat_mount_config = {
        .format_if_mount_failed   = false,
        .max_files                = 10,
        .allocation_unit_size     = CONFIG_WL_SECTOR_SIZE,
        .disk_status_check_enable = false,
        .use_one_fat              = false,
    };

    res = esp_vfs_fat_spiflash_mount_rw_wl("/int", "locfd", &fat_mount_config, &wl_handle);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FAT filesystem: %s", esp_err_to_name(res));
        message_dialog(NULL, "Error", "Failed to mount FAT filesystem", "OK");
    }

    // Register SD card
    res = registerSdCard();
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SD card: %s", esp_err_to_name(res));
        message_dialog(NULL, "Error", "Failed to mount SD card", "OK");
    }

    while (true) {
        if (menu_filebrowser("/int", (const char*[]){"nes"}, 1, selectedRomFilename, 256,
                             "Select NES ROM [internal]")) {
            break;
        }
        if (menu_filebrowser("/sd", (const char*[]){"nes"}, 1, selectedRomFilename, 256, "Select NES ROM [SD card]")) {
            break;
        }
    }

    ESP_LOGI(TAG, "Starting NoFrendo");
    pax_background(pax_buf, 0xFF000000);  // Black background
    pax_draw_text(pax_buf, 0xFFFFFFFF, &chakrapetchmedium, 22, 0, 0, "Starting NoFrendo...");
    display_blit();

    nofrendo_main(0, NULL);

    ESP_LOGE(TAG, "NoFrendo exited");
    pax_background(pax_buf, 0xFF000000);  // Black background
    pax_draw_text(pax_buf, 0xFFFFFFFF, &chakrapetchmedium, 22, 0, 0, "NoFrendo exited");
    display_blit();
    return 0;
}
