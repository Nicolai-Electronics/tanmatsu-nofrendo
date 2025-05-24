/*
   This code generates an effect that should pass the 'fancy graphics' qualification
   as set in the comment in the spi_master code.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_system.h>
// #include <math.h>
#include <decode_image.h>
#include <pretty_effect.h>
// #include "driver/gpio.h"
#include <charPixels.h>
// #include "esp_deep_sleep.h"
#include "esp_log.h"
#include <kbdcontroller.h>
#include <menu.h>

static const char* const TAG = "pretty_effect";

uint16_t** pixels;
int        newX;
int        splashScreenTimer;
int        initialStaticFrames;
int        bottomLogoOffset;
int        logoMoveSpeed;
uint8_t    whiteN;
int        colorCycle;
int        selectedIdx;
int        inputDelay;
int        lineMax;
int        selRom;

bool xStretch;
bool yStretch;

void setBright(int bright)
{
    setBr(bright);
}

bool peGetPixel(char peChar, int pe1, int pe2)
{
    return cpGetPixel(peChar, pe1, pe2);
}

bool getYStretch()
{
    return yStretch;
}

bool getXStretch()
{
    return xStretch;
}

void setXStretch(bool str)
{
    xStretch = str;
}

void setYStretch(bool str)
{
    yStretch = str;
}
void setLineMax(int lineM)
{
    lineMax = lineM;
}

void setSelRom(int selR)
{
    selRom = selR;
}

int getSelRom()
{
    return selRom;
}

/**
 * @brief Converts RGB color components to a 16-bit RGB565 color value
 *
 * This function takes 8-bit red, green, and blue color components and
 * converts them to a 16-bit RGB565 color format used by the display.
 * The conversion reduces color depth from 8 bits per channel to:
 * - 5 bits for red (0-31)
 * - 6 bits for green (0-63)
 * - 5 bits for blue (0-31)
 *
 * @param red   8-bit red component (0-255)
 * @param green 8-bit green component (0-255)
 * @param blue  8-bit blue component (0-255)
 * @return      16-bit RGB565 color value
 */
inline int rgbColor(const uint8_t red, const uint8_t green, const uint8_t blue)
{
    // Direct bit manipulation is faster than using a union with bit fields
    // Format: RRRRRGGGGGGBBBBB
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}

/**
 * @brief Xorshift32 pseudo-random number generator.
 *
 * This function generates a pseudo-random 32-bit unsigned integer using the Xorshift32 algorithm.
 * The internal state is updated with each call, so subsequent calls will produce different results.
 *
 * @return A pseudo-random 32-bit unsigned integer.
 */
static inline uint32_t xorshift32()
{
    static uint32_t state = 12345; // Seed value (can be any non-zero value)

    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;

    return state;
}

static inline uint16_t getNoise()
{
    whiteN = xorshift32() % 256;
    return rgbColor(whiteN, whiteN, whiteN);
}

// Display NES EMU splash screen with noise background
// Grab a rgb16 pixel from the esp32_tiles image, scroll part of it in
static inline uint16_t bootScreen(int x, int y, int bottomLogoOffset, int initialStaticFrames)
{
    // if (initialStaticFrames > 10)
    //     return getNoise();
    // else if (initialStaticFrames > 0)
    // {
    //     // Draw just the top of the logo
    //     if (x > 250 && x < 284 && y > 210 && y < 228)
    //     {
    //         int xAct = ((x - 250) / 2) + 96;
    //         int yAct = ((y - 210) / 2) + 210;
    //         if (pixels[yAct + 30][xAct] < 0x8000 + 1000)
    //             return 0x8000 + 31;
    //     }
    // }
    // else if (x >= 105 && x <= 216)
    // {
    //     // Draw the top of the logo
    //     if (y < 65 && pixels[y][x - 104] != 0x0000)
    //     {
    //         return pixels[y][x - 104];
    //     }
    //     else
    //     {
    //         // Draw the emu, sliding upwards using bottomLogoOffset
    //         y = y - bottomLogoOffset;

    //         if (y > 65 && pixels[y][x - 104] != 0x0000)
    //         {
    //             return pixels[y][x - 104];
    //         }
    //     }
    // }
    return getNoise();
}

// run "boot screen" (intro) and later menu to choose a rom
static inline uint16_t get_bgnd_pixel(int x, int y, int bottomLogoOffset, int bootTV, int selectedIdx)
{
    if (splashScreenTimer > 0)
    {
        return bootScreen(x, y, bottomLogoOffset, bootTV);
    }
    else
    {
        return getCharPixel(x, y, colorCycle, selectedIdx);
    }
}

#define DEFAULT_MENU_DELAY 1000
// Deal with user activity during boot screen and menu
void handleUserInput()
{
    if (inputDelay > 0)
        inputDelay -= 1;
    int input = kbdReadInput();
    if (splashScreenTimer > 0)
    {
        if (isAnyPressed(input))
        {
            // Immediately cancel splashscreen
            ESP_LOGI(TAG, "Cancelling animation\n");
            splashScreenTimer = 0;
            inputDelay        = DEFAULT_MENU_DELAY;
        }
    }
    else if (inputDelay <= 0)
    {
        if (isUpPressed(input) && selectedIdx > 0)
        {
            selectedIdx--;
            inputDelay = DEFAULT_MENU_DELAY;
        }
        if (isDownPressed(input) && selectedIdx < lineMax - 1)
        {
            selectedIdx++;
            inputDelay = DEFAULT_MENU_DELAY;
        }
        if (isAPressed(input) || isBPressed(input) || isStartPressed(input))
            selRom = selectedIdx;
        // Real emulators do not need sleep!
        // if (isSelectPressed(input))
        // {
        //     printf("Entering deep sleep\n");
        //     esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);
        //     gpio_pullup_dis(CONFIG_HW_GPIO_SELECT);
        //     gpio_pulldown_en(CONFIG_HW_GPIO_SELECT);
        //     esp_deep_sleep_enable_ext0_wakeup(CONFIG_HW_GPIO_SELECT, 1);

        //     vTaskDelay(1000);
        //     esp_deep_sleep_start();
        //     printf("Exiting deep sleep\n");
        // }
    }
}

// Instead of calculating the offsets for each pixel we grab, we pre-calculate the valueswhenever a frame changes, then
// re-use these as we go through all the pixels in the frame. This is much, much faster.
/*static int8_t xofs[320], yofs[240];
static int8_t xcomp[320], ycomp[240];*/

// Calculate the pixel data for a set of rows (with implied width of 320). Pixels go in dest, line is the Y-coordinate
// of the first line to be calculated, rowCount is the amount of rows to calculate. Frame increases by one every time
// the entire image is displayed; this is used to go to the next frame of animation.

void drawRows(uint16_t* dest, int y, int rowCount)
{
    if (y == 0)
    {
        // Change color cycle value every frame, implemented further downstream in the iconData
        colorCycle = (colorCycle + 1) % 30;
        if (initialStaticFrames > 0)
            initialStaticFrames--;
        if (bottomLogoOffset > 0 && initialStaticFrames <= 0)
        {
            bottomLogoOffset -= logoMoveSpeed;
            if (bottomLogoOffset < 0)
            {
                bottomLogoOffset = 0;
            }
        }
        // Count down splash screen frames to auto-exit the splash screen
        if (bottomLogoOffset == 0 && splashScreenTimer > 0)
            splashScreenTimer--;
    }

    // for (int yy = y; yy < y + rowCount; yy++)
    // {
    for (int x = 0; x < 320; x++)
    {
        *dest++ = get_bgnd_pixel(x,
                                 y,
                                 bottomLogoOffset,
                                 initialStaticFrames,
                                 selectedIdx);
    }
    // }
}

void freeMem()
{
    // for (int i = 0; i < 256; i++)
    // {
    //     free((pixels)[i]);
    // }
    // free(pixels);
    freeRL();
}

// initialize variables for "timers" and input, gpios and load picture
esp_err_t menuInit()
{
    initialStaticFrames = 8;
    bottomLogoOffset    = 180;
    logoMoveSpeed       = 8;
    splashScreenTimer   = 15;
    selectedIdx         = 0;
    inputDelay          = 0;
    lineMax             = 0;
    yStretch            = 0;
    xStretch            = 0;
    ESP_LOGI(TAG, "Reading rom list\n");
    initRomList();
    setLineMax(entryCount);
    ESP_LOGI(TAG, "Decoding image\n");
    int ret = decode_image(&pixels);
    ESP_LOGI(TAG, "decode returned %d\n", ret);
    return ret;
}
