#include "bsp/device.h"
#include "bsp/display.h"
#include "bsp/input.h"
#include "display.h"
// #include "driver/gpio.h"
#include "esp_err.h"
#include "esp_lcd_types.h"
#include "esp_log.h"
#include "hal/lcd_types.h"
#include "led.h"
#include "menu/src/menu.h"
#include "nvs_flash.h"
#include "sdcard.h"
#include <nofrendo.h>
#include <stdio.h>
#include <sys/stat.h>

// Constants
static char* const TAG = "main";

// Global variables
static esp_lcd_panel_handle_t       display_lcd_panel = NULL;
static size_t                       display_h_res     = 0;
static size_t                       display_v_res     = 0;
static lcd_color_rgb_pixel_format_t display_color_format;
static QueueHandle_t                input_event_queue = NULL;

#define ASSERT_ESP_OK(returnCode, message)                          \
    if (returnCode != ESP_OK)                                       \
    {                                                               \
        printf("%s. (%s)\n", message, esp_err_to_name(returnCode)); \
        return returnCode;                                          \
    }

int selectedRomIdx;

void esp_wake_deep_sleep()
{
    esp_restart();
}

int app_main(void)
{
    // Start the GPIO interrupt service
    gpio_install_isr_service(0);

    // Initialize the Non Volatile Storage service
    esp_err_t res = nvs_flash_init();
    if (res == ESP_ERR_NVS_NO_FREE_PAGES || res == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        res = nvs_flash_init();
    }
    ESP_ERROR_CHECK(res);

    // Initialize the Board Support Package
    ESP_ERROR_CHECK(bsp_device_initialize());
    ESP_ERROR_CHECK(led_init());

    // Fetch the handle for using the screen, this works even when
    res = bsp_display_get_panel(&display_lcd_panel);
    ESP_ERROR_CHECK(res); // Check that the display handle has been initialized
    // bsp_display_get_panel_io(&display_lcd_panel_io); // Do not check result of panel IO handle: not all types of
    //                                                  // display expose a panel IO handle
    res = bsp_display_get_parameters(&display_h_res, &display_v_res, &display_color_format);
    ESP_ERROR_CHECK(res); // Check that the display parameters have been initialized

    ESP_ERROR_CHECK(bsp_input_get_queue(&input_event_queue));

    // Register SD card
    ASSERT_ESP_OK(registerSdCard(), "Unable to register SD Card");

    // init PPA display buffer
    init_display();

#ifndef SKIP_MENU
    selectedRomFilename = runMenu();
    if (selectedRomFilename == NULL)
    {
        ESP_LOGE(TAG, "No ROM selected. Exiting...\n");
        while (true)
            ;
    }
#endif

    ESP_LOGI(TAG, "NoFrendo start!\n");
    nofrendo_main(0, NULL);
    ESP_LOGE(TAG, "NoFrendo died? WtF?\n");
    return 0;
}
