#include <emumenu.h>
#include <pretty_effect.h>
#include <stdbool.h>
#include <stdint.h>
#include "bitmap.h"
#include "kbdcontroller.h"
#include "nes.h"

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
#define SEL_BUTTON  '}'
#define TURBO_A     '@'
#define TURBO_B     '$'
#define EOL_MARKER  ';'
#define EOF_MARKER  '*'

static bool lineEnd;
static bool textEnd;

bool arrow[9][9]   = {{0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 1, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 1, 0, 0, 0},
                      {0, 0, 1, 1, 1, 1, 1, 0, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 0}, {0, 0, 0, 1, 1, 1, 0, 0, 0},
                      {0, 0, 0, 1, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 1, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}};
bool buttonA[9][9] = {{0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 1, 1, 1, 1, 0, 0, 0}, {0, 1, 1, 0, 0, 1, 1, 0, 0},
                      {1, 1, 0, 1, 1, 0, 1, 1, 0}, {1, 1, 0, 1, 1, 0, 1, 1, 0}, {1, 1, 0, 0, 0, 0, 1, 1, 0},
                      {1, 1, 0, 1, 1, 0, 1, 1, 0}, {0, 1, 0, 1, 1, 0, 1, 0, 0}, {0, 0, 1, 1, 1, 1, 0, 0, 0}};
bool buttonB[9][9] = {{0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 1, 1, 1, 1, 0, 0, 0}, {0, 1, 0, 0, 0, 1, 1, 0, 0},
                      {1, 1, 0, 1, 1, 0, 1, 1, 0}, {1, 1, 0, 0, 0, 1, 1, 1, 0}, {1, 1, 0, 1, 1, 0, 1, 1, 0},
                      {1, 1, 0, 1, 1, 0, 1, 1, 0}, {0, 1, 0, 0, 0, 1, 1, 0, 0}, {0, 0, 1, 1, 1, 1, 0, 0, 0}};

bool buttonSel[9][9] = {{0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 1, 1, 1, 1, 0, 0, 0}, {0, 1, 1, 0, 0, 0, 1, 0, 0},
                        {1, 1, 0, 1, 1, 1, 1, 1, 0}, {1, 1, 1, 0, 0, 1, 1, 1, 0}, {1, 1, 1, 1, 1, 0, 1, 1, 0},
                        {1, 1, 1, 1, 1, 0, 1, 1, 0}, {0, 1, 0, 0, 0, 1, 1, 0, 0}, {0, 0, 1, 1, 1, 1, 0, 0, 0}};

bool disabled[9][9] = {{0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 1, 1, 1, 1, 0, 0, 0}, {0, 1, 0, 0, 0, 0, 1, 0, 0},
                       {1, 0, 0, 0, 0, 1, 0, 1, 0}, {1, 0, 0, 0, 1, 0, 0, 1, 0}, {1, 0, 0, 1, 0, 0, 0, 1, 0},
                       {1, 0, 1, 0, 0, 0, 0, 1, 0}, {0, 1, 0, 0, 0, 0, 1, 0, 0}, {0, 0, 1, 1, 1, 1, 0, 0, 0}};
bool enabled[9][9]  = {{0, 0, 0, 0, 0, 0, 0, 0, 1}, {0, 0, 0, 0, 0, 0, 0, 1, 1}, {0, 0, 0, 0, 0, 0, 1, 1, 0},
                       {0, 0, 0, 0, 0, 1, 1, 0, 0}, {1, 0, 0, 0, 1, 1, 0, 0, 0}, {1, 1, 0, 1, 1, 0, 0, 0, 0},
                       {0, 1, 1, 1, 0, 0, 0, 0, 0}, {0, 0, 1, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}};
bool scale[9][9]    = {{0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 1, 1, 0}, {0, 0, 0, 0, 0, 0, 1, 1, 0},
                       {0, 0, 0, 0, 1, 1, 1, 1, 0}, {0, 0, 0, 0, 1, 1, 1, 1, 0}, {0, 0, 1, 1, 1, 1, 1, 1, 0},
                       {0, 0, 1, 1, 1, 1, 1, 1, 0}, {1, 1, 1, 1, 1, 1, 1, 1, 0}, {1, 1, 1, 1, 1, 1, 1, 1, 0}};

// Scale indicator with 0 as disabled
bool scale_steps[8][9][9] = {
    // 0 / disabled
    {{0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 1, 1, 1, 1, 0, 0, 0},
     {0, 1, 0, 0, 0, 0, 1, 0, 0},
     {1, 0, 0, 0, 0, 1, 0, 1, 0},
     {1, 0, 0, 0, 1, 0, 0, 1, 0},
     {1, 0, 0, 1, 0, 0, 0, 1, 0},
     {1, 0, 1, 0, 0, 0, 0, 1, 0},
     {0, 1, 0, 0, 0, 0, 1, 0, 0},
     {0, 0, 1, 1, 1, 1, 0, 0, 0}},

    // 1
    {{0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {1, 1, 0, 0, 0, 0, 0, 0, 0},
     {1, 1, 0, 0, 0, 0, 0, 0, 0}},

    // 2
    {{0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 1, 0, 0, 0, 0, 0, 0},
     {0, 0, 1, 0, 0, 0, 0, 0, 0},
     {1, 1, 1, 0, 0, 0, 0, 0, 0},
     {1, 1, 1, 0, 0, 0, 0, 0, 0}},

    // 3
    {{0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 1, 1, 0, 0, 0, 0, 0},
     {0, 0, 1, 1, 0, 0, 0, 0, 0},
     {1, 1, 1, 1, 0, 0, 0, 0, 0},
     {1, 1, 1, 1, 0, 0, 0, 0, 0}},

    // 4
    {{0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 1, 0, 0, 0, 0},
     {0, 0, 0, 0, 1, 0, 0, 0, 0},
     {0, 0, 1, 1, 1, 0, 0, 0, 0},
     {0, 0, 1, 1, 1, 0, 0, 0, 0},
     {1, 1, 1, 1, 1, 0, 0, 0, 0},
     {1, 1, 1, 1, 1, 0, 0, 0, 0}},

    // 5
    {{0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 1, 1, 0, 0, 0},
     {0, 0, 0, 0, 1, 1, 0, 0, 0},
     {0, 0, 1, 1, 1, 1, 0, 0, 0},
     {0, 0, 1, 1, 1, 1, 0, 0, 0},
     {1, 1, 1, 1, 1, 1, 0, 0, 0},
     {1, 1, 1, 1, 1, 1, 0, 0, 0}},

    // 6
    {{0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 1, 0, 0},
     {0, 0, 0, 0, 0, 0, 1, 0, 0},
     {0, 0, 0, 0, 1, 1, 1, 0, 0},
     {0, 0, 0, 0, 1, 1, 1, 0, 0},
     {0, 0, 1, 1, 1, 1, 1, 0, 0},
     {0, 0, 1, 1, 1, 1, 1, 0, 0},
     {1, 1, 1, 1, 1, 1, 1, 0, 0},
     {1, 1, 1, 1, 1, 1, 1, 0, 0}},

    // 7
    {{0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 1, 1, 0},
     {0, 0, 0, 0, 0, 0, 1, 1, 0},
     {0, 0, 0, 0, 1, 1, 1, 1, 0},
     {0, 0, 0, 0, 1, 1, 1, 1, 0},
     {0, 0, 1, 1, 1, 1, 1, 1, 0},
     {0, 0, 1, 1, 1, 1, 1, 1, 0},
     {1, 1, 1, 1, 1, 1, 1, 1, 0},
     {1, 1, 1, 1, 1, 1, 1, 1, 0}},

};

/**
 * Returns the scale step for the given scale and max value.
 *
 * @param scale The current scale value.
 * @param max The maximum possible scale value.
 * @return A pointer to the scale step array.
 */
bool (*getScaleChar(uint16_t scale, uint16_t max))[9] {
    // 0 to 7 steps
    int index = ((scale * 8) / max);
    if (index > 7) {
        index = 7;
    }
    return scale_steps[index];
}

char* menuText[11] = {
    "Brightness 46 0;", "Volume     82 9;", " ;",          "Reset };", " ;", "NoFrendo for Tanmatsu;",
    "Badge.Team;",      "Ported by;",       "Paul Honig;", " ;",       "*",
};

void renderInGameMenuFrame(bitmap_t* fb, uint16_t x1, uint16_t y1) {
    int x, y;

    static int const height = NES_SCREEN_HEIGHT;
    static int const width  = NES_SCREEN_WIDTH;
    static uint8_t*  fb_ptr = NULL;
    static uint32_t  pixel  = 0;

    uint16_t** fb_line = (uint16_t**)fb->line;

    for (y = 0; y < height; y++) {
        fb_ptr = (uint8_t*)fb_line[y];
        for (x = 0; x < width; x++) {
            pixel = renderInGameMenu(x, y, x1, y1, true, true);
            if (pixel != 0) {
                fb_ptr[x] = pixel;
            }
        }
    }
}

uint16_t renderInGameMenu(int x, int y, uint16_t x1, uint16_t y1, bool xStr, bool yStr) {
    static const int y_offset    = 10;
    static const int x_offset    = 16;
    static const int page_width  = NES_SCREEN_WIDTH - (x_offset * 2);
    static const int page_height = NES_SCREEN_HEIGHT - (y_offset * 2);
    static const int char_width  = 18 / 2;
    static const int char_height = 19 / 2;

    int retval = 0;
    if ((x > (x_offset - 8) && x < (page_width + x_offset)) && (y > (y_offset - 4) && y < (page_height + y_offset)))
        retval = 0x20;
    // if (x < x_offset || x > 280 || y < y_offset || y > 202)
    //     // return 0x01f; // Blue color
    //     retval = 0x0000; // Blue color
    char actChar = ' ';
    if (y == y_offset) textEnd = 0;
    if (x == x_offset) lineEnd = 0;
    int line   = (y - y_offset) / char_height;
    int charNo = (x - x_offset) / char_width;
    int yy     = ((y - y_offset) % char_height) / 1;
    int xx     = ((x - x_offset) % char_width) / 1;

    if (textEnd || lineEnd) {
        return retval;
    }

    actChar = line < sizeof(menuText) ? menuText[line][charNo] : '*';
    if (actChar == EOL_MARKER) lineEnd = 1;
    if (actChar == EOF_MARKER) textEnd = 1;
    if (lineEnd || textEnd) retval = 0x20;
    // printf("char %c, x = %d, y = %d{\n",actChar,x,y);
    // color c = [b](0to31)*1 + [g](0to31)*31 + [r] (0to31)*1024 +0x8000 --> x1=y1=c; !?
    switch (actChar) {
        case DOWN_ARROW:
            if (arrow[8 - yy][xx]) retval = 0xDDDD;
            break;
        case LEFT_ARROW:
            if (arrow[xx][yy]) retval = 0xDDDD;
            break;
        case RIGHT_ARROW:
            if (arrow[8 - xx][yy]) retval = 0xDDDD;
            break;
        case UP_ARROW:
            if (arrow[yy][xx]) retval = 0xDDDD;
            break;
        case A_BUTTON:
            if (buttonA[yy][xx]) retval = 0xDDDD;
            break;
        case B_BUTTON:
            if (buttonB[yy][xx]) retval = 0xDDDD;
            break;
        case SEL_BUTTON:
            if (buttonSel[yy][xx]) retval = 0xDDDD;
            break;
        case HORIZ_SCALE:
            if (xStr && enabled[yy][xx])
                retval = 63 << 5;
            else if (!xStr && disabled[yy][xx])
                retval = 31 << 11;
            break;
        case VERT_SCALE:
            if (yStr && enabled[yy][xx])
                retval = 63 << 5;
            else if (!yStr && disabled[yy][xx])
                retval = 31 << 11;
            break;
        case BRIGHTNESS:
            if (scale[yy][xx]) {
                // return xx < getBright() * 2 ? 0xFFFF : 0xDDDD;
                // setBrightness(getBright());
            }
            break;
        case VOL_METER:
            if (getScaleChar(getVolume(), 100)[yy][xx]) {
                retval = 31 << 11;
            }
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
            if ((actChar < 47 || actChar > 57) &&
                peGetPixel(actChar, ((x - x_offset) % char_width) * 2, ((y - y_offset) % char_height) * 2))
                retval = 0xFFFF;
            break;
    }
    return retval;
}