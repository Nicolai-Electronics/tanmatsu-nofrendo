/* SPI Master example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/task.h"
#include "kbdcontroller.h"
#include "sdcard.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include "decode_image.h"
#include <pretty_effect.h>
// #include "driver/ledc.h"
// #include "esp_spiffs.h"
#include <display.h>
#include <led.h>
#include <menu.h>
#include <mipi_gfx.h>

static const char* const TAG = "menu_main";

int        entryCount;
MenuEntry* menuEntries;

static uint16_t* prescale_fb = NULL;
static uint16_t* fb_y_pos[240];

/*
 This code displays some fancy graphics on the 320x240 LCD on an ESP-WROVER_KIT board.
 This example demonstrates the use of both spi_device_transmit as well as
 spi_device_queue_trans/spi_device_get_trans_result and pre-transmit callbacks.

 Some info about the ILI9341/ST7789V: It has an C/D line, which is connected to a GPIO here. It expects this
 line to be low for a command and high for data. We use a pre-transmit callback here to control that
 line: every transaction has as the user-definable argument the needed state of the D/C line and just
 before the transaction is sent, the callback will set this line to the correct state.
*/

#define PIN_NUM_MISO CONFIG_HW_LCD_MISO_GPIO
#define PIN_NUM_MOSI CONFIG_HW_LCD_MOSI_GPIO
#define PIN_NUM_CLK  CONFIG_HW_LCD_CLK_GPIO
#define PIN_NUM_CS   CONFIG_HW_LCD_CS_GPIO

#define PIN_NUM_DC   CONFIG_HW_LCD_DC_GPIO
#define PIN_NUM_RST  CONFIG_HW_LCD_RESET_GPIO
#define PIN_NUM_BCKL CONFIG_HW_LCD_BL_GPIO

#define LEDC_LS_TIMER       LEDC_TIMER_1
#define LEDC_LS_MODE        LEDC_HIGH_SPEED_MODE
#define LEDC_LS_CH3_CHANNEL LEDC_CHANNEL_3

// TODO: Use the backlight brightness from the menu instead of just 100
#define LCD_BKG_ON()       bsp_set_backlight_brightness(100) // Backlight ON
#define LCD_BKG_OFF()      bsp_set_backlight_brightness(0)   // Backlight OFF
// To speed up transfers, every SPI transfer sends a bunch of rows. This define specifies how many. More means more
// memory use, but less overhead for setting up / finishing transfers. Make sure 240 is dividable by this.
#define FRAMEBUFFER_HEIGHT 4

#define MENU_FB_W 320
#define MENU_FB_H 240

#define NO_ROM_SELECTED 9999

static void send_frame(uint16_t* fb_in)
{
    // TODO: Add the PPA scaler + render code here
    mipi_blit(
        fb_in, MENU_FB_W, MENU_FB_H, 0, 0, 320, 240);
}

static void initRenderBuffers(void)
{
    if (prescale_fb != NULL)
    {
        return;
    }
    // Get the MIPI output buffer
    ESP_LOGD(TAG, "Get MIPI buffer");
    mipi_init();

    // Setup the local render buffer
    ESP_LOGD(TAG, "Setup local render buffer");
    prescale_fb = (uint16_t*)heap_caps_malloc(MENU_FB_W * MENU_FB_H * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
    assert(prescale_fb != NULL);

    // Setup the line pointer array
    ESP_LOGD(TAG, "Setup line pointer array");
    for (int y = 0; y < MENU_FB_H; y++)
    {
        fb_y_pos[y] = prescale_fb + (MENU_FB_W * y);
    }
}

static void deinitRenderBuffers(void)
{
    // Free the local render buffer
    ESP_LOGD(TAG, "Free local render buffer");
    heap_caps_free(prescale_fb);
    mipi_cls();
    prescale_fb = NULL;
}

// Show menu until a Rom is selected
// Because the SPI driver handles transactions in the background, we can calculate the next line
// while the previous one is being sent.
static char* selectRomFromMenu()
{
    ESP_LOGI(TAG, "Show menu until a Rom is selected");

    // Setup the keyboard controller event queue
    kbdControllerInit();

    while (1)
    {
        for (int y = 0; y < MENU_FB_H; y++)
        {
            // Draw a framebuffer's worth of graphics
            drawRows(fb_y_pos[y], y, 1);
            handleUserInput();
            if (getSelRom() != NO_ROM_SELECTED)
            {
                char* filename = (char*)malloc(FILENAME_LENGTH + strlen(SD_CARD_ROM_PATH));
                filename[0]    = '\0';
                strcat(filename, SD_CARD_ROM_PATH);
                strcat(filename, "/");
                strcat(filename, menuEntries[getSelRom()].fileName);
                freeMem();
                return filename;
            }
        }
        send_frame(prescale_fb);
        vTaskDelay(10);
    }

    return NULL;
}

// ledc_channel_config_t ledc_channel;

void initBl()
{
    // Ignore LED 0 as it's the power LED
    for (uint8_t i = 1; i < 6; i++)
    {
        set_led_color(i, 0x000000); // Black
    }
    // show_led_colors();
    // ledc_timer_config_t ledc_timer = {
    //     .duty_resolution = LEDC_TIMER_3_BIT,
    //     .freq_hz = 5 * 1000 * 1000,
    //     .speed_mode = LEDC_LS_MODE,
    //     .timer_num = LEDC_LS_TIMER};

    // ledc_timer_config(&ledc_timer);

    // ledc_channel.channel = LEDC_LS_CH3_CHANNEL;
    // //ledc_channel.duty       = 2000;
    // ledc_channel.gpio_num = PIN_NUM_BCKL;
    // ledc_channel.speed_mode = LEDC_LS_MODE;
    // ledc_channel.timer_sel = LEDC_LS_TIMER;

    // #if PIN_NUM_BCKL >= 0
    // ledc_channel_config(&ledc_channel);
    // #endif
}

char* runMenu()
{
    ESP_LOGI(TAG, "Init SPI bus\n");
    esp_err_t ret;
    ESP_LOGI(TAG, "Start boot menu init\n");
    // Initialize the Display output
    initRenderBuffers();
    // Initialize the menu screen
    ret = menuInit();
    ESP_LOGI(TAG, "Finished boot menu init\n");
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "No error so going on\n");

    initBl();
    // Show splash screen and prompt user to select something
    ESP_LOGI(TAG, "Showing boot menu\n");
    setSelRom(NO_ROM_SELECTED);
    char* filename = selectRomFromMenu();
    ESP_LOGI(TAG, "Menu selection is %s\n", filename);
    deinitRenderBuffers();
    return filename;
}