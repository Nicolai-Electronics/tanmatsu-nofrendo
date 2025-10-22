#include "common/display.h"
#include "bsp/display.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "hal/lcd_types.h"
#include "pax_gfx.h"
#include "pax_types.h"
#include "sdkconfig.h"

#if defined(CONFIG_BSP_TARGET_TANMATSU) || defined(CONFIG_BSP_TARGET_KONSOOL) || \
    defined(CONFIG_BSP_TARGET_HACKERHOTEL_2026) || defined(CONFIG_BSP_TARGET_ESP32_P4_FUNCTION_EV_BOARD)
#define DSI_PANEL
#endif

#ifdef DSI_PANEL
#include "esp_lcd_mipi_dsi.h"
#endif

static size_t                       display_h_res        = 0;
static size_t                       display_v_res        = 0;
static lcd_color_rgb_pixel_format_t display_color_format = LCD_COLOR_PIXEL_FORMAT_RGB565;
static lcd_rgb_data_endian_t        display_data_endian  = LCD_RGB_DATA_ENDIAN_LITTLE;
static pax_buf_t                    pax_fb               = {0};
static uint8_t*                     raw_fb               = NULL;

esp_err_t display_init(void) {
    ESP_ERROR_CHECK(
        bsp_display_get_parameters(&display_h_res, &display_v_res, &display_color_format, &display_data_endian));

    pax_buf_type_t format = PAX_BUF_24_888RGB;

    size_t raw_fb_size = 0;

    switch (display_color_format) {
        case LCD_COLOR_PIXEL_FORMAT_RGB565:
            format      = PAX_BUF_16_565RGB;
            raw_fb_size = display_h_res * display_v_res * 2;
            break;
        case LCD_COLOR_PIXEL_FORMAT_RGB888:
            format      = PAX_BUF_24_888RGB;
            raw_fb_size = display_h_res * display_v_res * 3;
            break;
        default:
            ESP_LOGE("display", "Unsupported color format %d", display_color_format);
            return ESP_FAIL;
    }

    raw_fb = (uint8_t*)heap_caps_malloc(raw_fb_size, MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);

    if (raw_fb == NULL) {
        ESP_LOGE("display", "Failed to allocate framebuffer");
        return ESP_FAIL;
    }

    pax_buf_init(&pax_fb, raw_fb, display_h_res, display_v_res, format);
    pax_buf_reversed(&pax_fb, display_data_endian == LCD_RGB_DATA_ENDIAN_BIG);

    bsp_display_rotation_t display_rotation = bsp_display_get_default_rotation();
    pax_orientation_t      orientation      = PAX_O_UPRIGHT;
    switch (display_rotation) {
        case BSP_DISPLAY_ROTATION_90:
            orientation = PAX_O_ROT_CCW;
            break;
        case BSP_DISPLAY_ROTATION_180:
            orientation = PAX_O_ROT_HALF;
            break;
        case BSP_DISPLAY_ROTATION_270:
            orientation = PAX_O_ROT_CW;
            break;
        case BSP_DISPLAY_ROTATION_0:
        default:
            orientation = PAX_O_UPRIGHT;
            break;
    }
    pax_buf_set_orientation(&pax_fb, orientation);

    return ESP_OK;
}

pax_buf_t* display_get_pax_buffer(void) {
    return &pax_fb;
}

uint8_t* display_get_raw_buffer(void) {
    return raw_fb;
}

void display_blit_buffer(pax_buf_t* fb) {
    size_t display_h_res = 0, display_v_res = 0;
    ESP_ERROR_CHECK(bsp_display_get_parameters(&display_h_res, &display_v_res, NULL, NULL));
    ESP_ERROR_CHECK(bsp_display_blit(0, 0, display_h_res, display_v_res, pax_buf_get_pixels(fb)));
}

void display_blit(void) {
    display_blit_buffer(&pax_fb);
}
