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

#include "bsp/display.h"
#include "driver/ppa.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/ppa_types.h"
#include "rom/ets_sys.h"
#include "rom/gpio.h"
#include "sdkconfig.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_struct.h"
#include "soc/io_mux_reg.h"
#include "soc/spi_reg.h"
#include <display.h>
#include <esp_log.h>
#include <mipi_gfx.h>
#include <nes.h>
#include <pretty_effect.h>
#include <stdio.h>
#include <string.h>

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

// Input event queue

static void spi_write_byte(const uint8_t data)
{
    //     SET_PERI_REG_BITS(SPI_MOSI_DLEN_REG(SPI_NUM), SPI_USR_MOSI_DBITLEN, 0x7, SPI_USR_MOSI_DBITLEN_S);
    //     WRITE_PERI_REG((SPI_W0_REG(SPI_NUM)), data);
    //     SET_PERI_REG_MASK(SPI_CMD_REG(SPI_NUM), SPI_USR);
    //     waitForSPIReady();
}

#define U16x2toU32(m, l) ((((uint32_t)(l >> 8 | (l & 0xFF) << 8)) << 16) | (m >> 8 | (m & 0xFF) << 8))

extern uint16_t myPalette[];

char* menuText[10] = {
    "Brightness 46 0.",
    "Volume     82 9.",
    " .",
    "Turbo  3 $  1 @.",
    " .",
    "These cause lag.",
    "Horiz Scale 1 5.",
    "Vert Scale  3 7.",
    " .",
    "*"};
bool        arrow[9][9]    = {{0, 0, 0, 0, 0, 0, 0, 0, 0},
                              {0, 0, 0, 0, 1, 0, 0, 0, 0},
                              {0, 0, 0, 1, 1, 1, 0, 0, 0},
                              {0, 0, 1, 1, 1, 1, 1, 0, 0},
                              {0, 1, 1, 1, 1, 1, 1, 1, 0},
                              {0, 0, 0, 1, 1, 1, 0, 0, 0},
                              {0, 0, 0, 1, 1, 1, 0, 0, 0},
                              {0, 0, 0, 1, 1, 1, 0, 0, 0},
                              {0, 0, 0, 0, 0, 0, 0, 0, 0}};
bool        buttonA[9][9]  = {{0, 0, 0, 0, 0, 0, 0, 0, 0},
                              {0, 0, 1, 1, 1, 1, 0, 0, 0},
                              {0, 1, 1, 0, 0, 1, 1, 0, 0},
                              {1, 1, 0, 1, 1, 0, 1, 1, 0},
                              {1, 1, 0, 1, 1, 0, 1, 1, 0},
                              {1, 1, 0, 0, 0, 0, 1, 1, 0},
                              {1, 1, 0, 1, 1, 0, 1, 1, 0},
                              {0, 1, 0, 1, 1, 0, 1, 0, 0},
                              {0, 0, 1, 1, 1, 1, 0, 0, 0}};
bool        buttonB[9][9]  = {{0, 0, 0, 0, 0, 0, 0, 0, 0},
                              {0, 0, 1, 1, 1, 1, 0, 0, 0},
                              {0, 1, 0, 0, 0, 1, 1, 0, 0},
                              {1, 1, 0, 1, 1, 0, 1, 1, 0},
                              {1, 1, 0, 0, 0, 1, 1, 1, 0},
                              {1, 1, 0, 1, 1, 0, 1, 1, 0},
                              {1, 1, 0, 1, 1, 0, 1, 1, 0},
                              {0, 1, 0, 0, 0, 1, 1, 0, 0},
                              {0, 0, 1, 1, 1, 1, 0, 0, 0}};
bool        disabled[9][9] = {{0, 0, 0, 0, 0, 0, 0, 0, 0},
                              {0, 0, 1, 1, 1, 1, 0, 0, 0},
                              {0, 1, 0, 0, 0, 0, 1, 0, 0},
                              {1, 0, 0, 0, 0, 1, 0, 1, 0},
                              {1, 0, 0, 0, 1, 0, 0, 1, 0},
                              {1, 0, 0, 1, 0, 0, 0, 1, 0},
                              {1, 0, 1, 0, 0, 0, 0, 1, 0},
                              {0, 1, 0, 0, 0, 0, 1, 0, 0},
                              {0, 0, 1, 1, 1, 1, 0, 0, 0}};
bool        enabled[9][9]  = {{0, 0, 0, 0, 0, 0, 0, 0, 1},
                              {0, 0, 0, 0, 0, 0, 0, 1, 1},
                              {0, 0, 0, 0, 0, 0, 1, 1, 0},
                              {0, 0, 0, 0, 0, 1, 1, 0, 0},
                              {1, 0, 0, 0, 1, 1, 0, 0, 0},
                              {1, 1, 0, 1, 1, 0, 0, 0, 0},
                              {0, 1, 1, 1, 0, 0, 0, 0, 0},
                              {0, 0, 1, 0, 0, 0, 0, 0, 0},
                              {0, 0, 0, 0, 0, 0, 0, 0, 0}};
bool        scale[9][9]    = {{0, 0, 0, 0, 0, 0, 0, 0, 0},
                              {0, 0, 0, 0, 0, 0, 1, 1, 0},
                              {0, 0, 0, 0, 0, 0, 1, 1, 0},
                              {0, 0, 0, 0, 1, 1, 1, 1, 0},
                              {0, 0, 0, 0, 1, 1, 1, 1, 0},
                              {0, 0, 1, 1, 1, 1, 1, 1, 0},
                              {0, 0, 1, 1, 1, 1, 1, 1, 0},
                              {1, 1, 1, 1, 1, 1, 1, 1, 0},
                              {1, 1, 1, 1, 1, 1, 1, 1, 0}};
static bool lineEnd;
static bool textEnd;
#define BRIGHTNESS  '0'
#define A_BUTTON    '1'
#define DOWN_ARROW  '2'
#define B_BUTTON    '3'
#define LEFT_ARROW  '4'
#define HORIZ_SCALE '5'
#define RIGHT_ARROW '6'
#define VERT_SCALE  '7'
#define UP_ARROW    '8'
#define VOL_METER   '9'
#define TURBO_A     '@'
#define TURBO_B     '$'
#define EOL_MARKER  '.'
#define EOF_MARKER  '*'

#define spiWrite(register, val)                                                                            \
    SET_PERI_REG_BITS(SPI_MOSI_DLEN_REG(SPI_NUM), SPI_USR_MOSI_DBITLEN, register, SPI_USR_MOSI_DBITLEN_S); \
    WRITE_PERI_REG((SPI_W0_REG(SPI_NUM)), val);                                                            \
    SET_PERI_REG_MASK(SPI_CMD_REG(SPI_NUM), SPI_USR);                                                      \
    waitForSPIReady();

static uint16_t renderInGameMenu(int x, int y, uint16_t x1, uint16_t y1, bool xStr, bool yStr)
{
    if (x < 32 || x > 286 || y < 34 || y > 206)
        return x1;
    else if (x < 40 || x > 280 || y < 38 || y > 202)
        return 0x0F;
    char actChar = ' ';
    if (y == 38)
        textEnd = 0;
    if (x == 40)
        lineEnd = 0;
    int line   = (y - 38) / 18;
    int charNo = (x - 40) / 16;
    int xx     = ((x - 40) % 16) / 2;
    int yy     = ((y - 38) % 18) / 2;

    actChar = menuText[line][charNo];
    if (actChar == EOL_MARKER)
        lineEnd = 1;
    if (actChar == EOF_MARKER)
        textEnd = 1;
    if (lineEnd || textEnd)
        return 0x0F;
    // printf("char %c, x = %d, y = %d{\n",actChar,x,y);
    // color c = [b](0to31)*1 + [g](0to31)*31 + [r] (0to31)*1024 +0x8000 --> x1=y1=c; !?
    switch (actChar)
    {
    case DOWN_ARROW:
        if (arrow[8 - yy][xx])
            return 0xDDDD;
        break;
    case LEFT_ARROW:
        if (arrow[xx][yy])
            return 0xDDDD;
        break;
    case RIGHT_ARROW:
        if (arrow[8 - xx][yy])
            return 0xDDDD;
        break;
    case UP_ARROW:
        if (arrow[yy][xx])
            return 0xDDDD;
        break;
    case A_BUTTON:
        if (buttonA[yy][xx])
            return 0xDDDD;
        break;
    case B_BUTTON:
        if (buttonB[yy][xx])
            return 0xDDDD;
        break;
    case HORIZ_SCALE:
        if (xStr && enabled[yy][xx])
            return 63 << 5;
        else if (!xStr && disabled[yy][xx])
            return 31 << 11;
        break;
    case VERT_SCALE:
        if (yStr && enabled[yy][xx])
            return 63 << 5;
        else if (!yStr && disabled[yy][xx])
            return 31 << 11;
        break;
    case BRIGHTNESS:
        if (scale[yy][xx])
        {
            // return xx < getBright() * 2 ? 0xFFFF : 0xDDDD;
            // setBrightness(getBright());
        }
        break;
    case VOL_METER:
        // if (getVolume() == 0 && disabled[yy][xx])
        //     return 31 << 11;

        // if (getVolume() > 0 && scale[yy][xx])
        //     return (xx) < getVolume() * 2 ? 0xFFFF : 0xDDDD;
        break;
    case TURBO_A:
        // if (!getTurboA() && disabled[yy][xx])
        //     return 31 << 11;
        // if (getTurboA() > 0 && scale[yy][xx])
        //     return (xx) < (getTurboA() * 2 - 1) ? 0xffff : 0xdddd;
        break;
    case TURBO_B:
        // if (!getTurboB() && disabled[yy][xx])
        //     return 31 << 11;
        // if (getTurboB() > 0 && scale[yy][xx])
        //     return (xx) < (getTurboB() * 2 - 1) ? 0xffff : 0xdddd;
        break;
    default:
        if ((actChar < 47 || actChar > 57) && peGetPixel(actChar, (x - 40) % 16, (y - 38) % 18))
            return 0xFFFF;
        break;
    }
    return 0x0F;
}

uint16_t rowCrc[NES_SCREEN_HEIGHT];
uint16_t scaleX[320];
uint16_t scaleY[240];
// This is the number of pixels on left and right that are ignored for dirty checks
// By ignoring the left and right edges, attribute glitches won't cause extra rendering
#define ignore_border 4

static int lastShowMenu = 0;
// TODO: Replace with PPA P4 implementation
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
 * @warning This function does not handle interlacing or other advanced display features. It simply writes the given frame data directly to the display.
 */
void       mipi_write_frame(const uint16_t xs,
                            const uint16_t ys,
                            const uint16_t width,
                            const uint16_t height,
                            const uint8_t  data[],
                            bool           xStr,
                            bool           yStr)
{
    // int      x, y;
    // int      xx, yy;
    // int      i;
    // uint16_t x1, y1, evenPixel, oddPixel, backgroundColor;
    // uint32_t xv, yv, dc;
    // uint32_t temp[16];
    if (data == NULL)
        return;

    // TODO: Add show menu back in, in the future
    //
    //    dc = (1 << PIN_NUM_DC);
    //
    //    // TODO: Replace with BSP keyboard input
    //    // if (getShowMenu() != lastShowMenu)
    //    // {
    //    //     memset(rowCrc, 0, sizeof rowCrc);
    //    // }
    //    lastShowMenu = getShowMenu();
    //
    int lastY      = -1;
    int lastYshown = 0;

    // Convert data 8 bit pallete to 16-bit 565 RGB format
    for (int i = 0; i < height * width; i++)
    {
        prescale_fb[i] = myPalette[(uint32_t)data[i]];
    }

    // Black background
    // backgroundColor                  = 0;
    active_config.in.buffer          = prescale_fb;
    // active_config.in.pic_w           = width;
    active_config.in.pic_h           = height;
    // active_config.in.block_w         = width;
    active_config.in.block_h         = height;
    active_config.out.block_offset_x = xs;
    active_config.out.block_offset_y = (800 - (NES_VISIBLE_WIDTH * 2 * 1.333)) / 2; // TODO: Proper constants
    ESP_ERROR_CHECK(ppa_do_scale_rotate_mirror(ppa_srm_handle, &active_config));
    esp_lcd_panel_draw_bitmap(display_lcd_panel, 0, 0, display_h_res, display_v_res, mipi_fb);
}

static esp_err_t mipi_init_ppa(const size_t bf_h, const size_t bf_v)
{
    // Setup frame to bitmap PPA config, which blits the frame to the MIPI buffer.
    esp_err_t res;

    size_t buffer_size = bf_h * bf_v * sizeof(uint16_t); // 16-bit per pixel

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

// void precalculateLookupTables()
// {
//     for (int i = 0; i < 320; i++)
//     {
//         scaleX[i] = i * 0.8;
//     }
//     for (int i = 0; i < 240; i++)
//     {
//         scaleY[i] = i * 0.94;
//     }
// }
void mipi_init()
{
    esp_err_t res;
    if (mipi_initialized)
    {
        ESP_LOGW(TAG, "MIPI display already initialized");
        return;
    }

    // Setup the display objects
    res = bsp_display_get_panel(&display_lcd_panel);
    ESP_ERROR_CHECK(res);
    // Not implemented yet in BSP
    bsp_display_get_panel_io(&display_lcd_panel_io);

    res = bsp_display_get_parameters(&display_h_res, &display_v_res, &display_color_format);
    ESP_ERROR_CHECK(res); // Check that the display parameters have been initialized

    size_t buffer_size = display_h_res * display_v_res * sizeof(uint16_t);

    prescale_fb = (uint16_t*)heap_caps_malloc(320 * 240 * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
    mipi_fb     = get_mipi_framebuffer();

    for (int i = 0; i < buffer_size / sizeof(uint16_t); i++)
    {
        mipi_fb[i] = 0x0000; // Black color
    }

    // Register the PPA client
    ESP_ERROR_CHECK(mipi_init_ppa(display_h_res, display_v_res));

    // We are done mark the display as initialized
    mipi_initialized = true;
}
