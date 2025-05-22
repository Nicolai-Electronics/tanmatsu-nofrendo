#include "bsp/display.h"
#include "display.h"
#include "esp_err.h"
#include "esp_log.h"

static const char* TAG = "display";

static uint16_t*              mipi_fb = NULL;
static esp_lcd_panel_handle_t display_lcd_panel;

// External display variables
static size_t                       display_h_res        = 0;
static size_t                       display_v_res        = 0;
static lcd_color_rgb_pixel_format_t display_color_format = LCD_COLOR_FMT_RGB565;

void init_display(void)
{
    ESP_LOGI(TAG, "Initializing display");

    esp_err_t res;

    res = bsp_display_get_panel(&display_lcd_panel);
    res = bsp_display_get_parameters(&display_h_res, &display_v_res, &display_color_format);
    ESP_ERROR_CHECK(res); // Check that the display parameters have been initialized

    size_t buffer_size = display_h_res * display_v_res * sizeof(uint16_t);
    mipi_fb            = (uint16_t*)heap_caps_malloc(
        buffer_size,
        MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
}

uint16_t* get_mipi_framebuffer(void)
{
    return mipi_fb;
}
