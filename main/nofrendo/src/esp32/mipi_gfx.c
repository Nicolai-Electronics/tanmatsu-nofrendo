// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <display.h>
#include <esp_log.h>
#include <mipi_gfx.h>
#include <nes.h>
#include <pretty_effect.h>
#include <stdio.h>
#include <string.h>
#include "bsp/display.h"
#include "common/display.h"
#include "driver/ppa.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/ppa_types.h"
#include "kbdcontroller.h"
#include "soc/spi_reg.h"

static const char* const TAG = "mipi_gfx";

// Mipi DSI display framebuffer
uint16_t*                        mipi_fb;
uint16_t*                        prescale_fb;
static esp_lcd_panel_handle_t    display_lcd_panel;
static esp_lcd_panel_io_handle_t display_lcd_panel_io;

// PPA blitting handles
static ppa_client_handle_t   ppa_srm_handle = NULL;
static ppa_client_config_t   ppa_srm_config;
static ppa_srm_oper_config_t active_config;

// External display variables
bool                         mipi_initialized     = false;
size_t                       display_h_res        = 0;
size_t                       display_v_res        = 0;
lcd_color_rgb_pixel_format_t display_color_format = LCD_COLOR_FMT_RGB565;
lcd_rgb_data_endian_t        display_data_endian  = LCD_RGB_DATA_ENDIAN_BIG;

// Input event queue

// #define U16x2toU32(m, l) ((((uint32_t)(l >> 8 | (l & 0xFF) << 8)) << 16) | (m >> 8 | (m & 0xFF) << 8))

extern uint16_t myPalette[];

#define spiWrite(register, val)                                                                            \
    SET_PERI_REG_BITS(SPI_MOSI_DLEN_REG(SPI_NUM), SPI_USR_MOSI_DBITLEN, register, SPI_USR_MOSI_DBITLEN_S); \
    WRITE_PERI_REG((SPI_W0_REG(SPI_NUM)), val);                                                            \
    SET_PERI_REG_MASK(SPI_CMD_REG(SPI_NUM), SPI_USR);                                                      \
    waitForSPIReady();

uint16_t rowCrc[NES_SCREEN_HEIGHT];
uint16_t scaleX[320];
uint16_t scaleY[240];
// This is the number of pixels on left and right that are ignored for dirty checks
// By ignoring the left and right edges, attribute glitches won't cause extra rendering
#define ignore_border 4

static int lastShowMenu = 0;

void mipi_blit(const uint16_t* data, const int pic_w, const int pic_h, const int xs, const int ys, const int width,
               const int height) {
    // Black background
    // backgroundColor                  = 0;
    active_config.in.buffer          = data;
    active_config.in.pic_w           = pic_w;
    active_config.in.pic_h           = pic_h;
    // active_config.in.block_w         = width;
    active_config.in.block_h         = height;
    active_config.out.block_offset_x = xs;
    active_config.out.block_offset_y = ys;
    // active_config.out.block_offset_y = (800 - (NES_VISIBLE_WIDTH * 2 * 1.333)) / 2; // TODO: Proper constants
    ESP_ERROR_CHECK(ppa_do_scale_rotate_mirror(ppa_srm_handle, &active_config));
    esp_lcd_panel_draw_bitmap(display_lcd_panel, 0, 0, display_h_res, display_v_res, mipi_fb);
}

/**
 * Renders a frame to the display.
 *
 * @param xs    X coordinate of the top-left corner of the frame.
 * @param ys    Y coordinate of the top-left corner of the frame.
 * @param width  Width of the frame. (default 320)
 * @param height Height of the frame. (default 240)
 * @param data  Array of pointers to the frame data for each scanline.
 * @param xStr  True if the X coordinates are scaled.
 * @param yStr  True if the Y coordinates are scaled.
 *
 * @note This function assumes that the display driver has been initialized and that the display is currently active.
 *
 * @warning This function is not thread-safe and should not be called from multiple threads simultaneously.
 *
 * @warning This function does not handle interlacing or other advanced display features. It simply writes the given
 * frame data directly to the display.
 */
void mipi_write_frame(const uint16_t xs, const uint16_t ys, const uint16_t width, const uint16_t height,
                      const uint8_t data[], bool xStr, bool yStr) {
    // int      x, y;
    // int      xx, yy;
    // int      i;
    // uint16_t x1, y1, evenPixel, oddPixel, backgroundColor;
    // uint32_t xv, yv, dc;
    // uint32_t temp[16];
    if (data == NULL) return;

    // TODO: Add show menu back in, in the future
    //
    //    dc = (1 << PIN_NUM_DC);
    //
    //    // TODO: Replace with BSP keyboard input
    //    // if (getShowMenu() != lastShowMenu)
    //    // {
    //    //     memset(rowCrc, 0, sizeof rowCrc);
    //    // }
    lastShowMenu = getShowMenu();
    // int lastY      = -1;
    // int lastYshown = 0;

    // Convert data 8 bit pallete to 16-bit 565 RGB format
    for (int i = 0; i < height * width; i++) {
        prescale_fb[i] = myPalette[(uint32_t)data[i]];
    }

    // ys is fixed for emulator output
    const uint16_t my_ys = (800 - (NES_VISIBLE_WIDTH * 2 * 1.333)) / 2;

    mipi_blit(prescale_fb, NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT, xs, my_ys, width, height);
}

static esp_err_t mipi_init_ppa(const size_t bf_h, const size_t bf_v) {

    if (ppa_srm_handle != NULL) {
        // Already initialized, return
        return ESP_OK;
    }

    // Setup frame to bitmap PPA config, which blits the frame to the MIPI buffer.
    esp_err_t res;

    size_t buffer_size = bf_h * bf_v * sizeof(uint16_t);  // 16-bit per pixel

    ESP_LOGI(TAG, "Initializing PPA srm, display buffer w %d, h %d, buffer size %zu", bf_h, bf_v, buffer_size);

    // Setup the PPA srm configuration to
    // Take the current frame buffer and blit it to the MIPI buffer
    // - Size 2x
    // - Destination frame buffer is 320x240
    // - Source frame buffer is 320x240
    // - Frame buffer size is 320x240
    ppa_in_pic_blk_config_t* in = &active_config.in;
    in->pic_w                   = NES_SCREEN_WIDTH;
    in->pic_h                   = NES_SCREEN_HEIGHT;
    in->block_w                 = NES_VISIBLE_WIDTH;
    in->block_h                 = NES_VISIBLE_HEIGHT;
    in->block_offset_x          = 0;
    in->block_offset_y          = 0;
    in->srm_cm                  = PPA_SRM_COLOR_MODE_RGB565;

    ppa_out_pic_blk_config_t* out = &active_config.out;
    out->buffer                   = mipi_fb;
    out->buffer_size              = buffer_size;
    out->pic_w                    = bf_h;
    out->pic_h                    = bf_v;
    out->block_offset_x           = 0;
    out->block_offset_y           = 0;
    out->srm_cm                   = PPA_SRM_COLOR_MODE_RGB565;

    // Initialize other configuration parameters
    active_config.rotation_angle = PPA_SRM_ROTATION_ANGLE_270;
    // active_config.rotation_angle = PPA_SRM_ROTATION_ANGLE_0;
    active_config.scale_x        = 2.0 * 1.33;
    active_config.scale_y        = 2.0;
    active_config.rgb_swap       = 0;
    active_config.byte_swap      = 0;
    active_config.mode           = PPA_TRANS_MODE_BLOCKING;

    ppa_srm_config.oper_type             = PPA_OPERATION_SRM;
    ppa_srm_config.max_pending_trans_num = 1;

    // Use the configuration above to initialize the PPA srm client
    res = ppa_register_client(&ppa_srm_config, &ppa_srm_handle);
    return res;
}

void mipi_cls() {
    memset(mipi_fb, 0, display_h_res * display_v_res * sizeof(uint16_t));
}

void mipi_init() {
    esp_err_t res;
    if (mipi_initialized) {
        ESP_LOGW(TAG, "MIPI display already initialized");
        return;
    }

    // Setup the display objects
    res = bsp_display_get_panel(&display_lcd_panel);
    ESP_ERROR_CHECK(res);
    // Not implemented yet in BSP
    bsp_display_get_panel_io(&display_lcd_panel_io);

    res = bsp_display_get_parameters(&display_h_res, &display_v_res, &display_color_format, &display_data_endian);
    ESP_ERROR_CHECK(res);  // Check that the display parameters have been initialized

    size_t buffer_size = display_h_res * display_v_res * sizeof(uint16_t);

    prescale_fb = (uint16_t*)heap_caps_malloc(320 * 240 * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
    mipi_fb     = (uint16_t*)display_get_raw_buffer();

    for (int i = 0; i < buffer_size / sizeof(uint16_t); i++) {
        mipi_fb[i] = 0x0000;  // Black color
    }

    // Register the PPA client
    ESP_ERROR_CHECK(mipi_init_ppa(display_h_res, display_v_res));

    // We are done mark the display as initialized
    mipi_initialized = true;
}
