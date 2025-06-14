
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

#include "kbdcontroller.h"
#include "bsp/input.h"
#include "esp_log.h"
// #include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <pretty_effect.h>
#include <stdio.h>

static const char* TAG = "kbd controller";

static QueueHandle_t input_event_queue = NULL;

#define PSX_CLK CONFIG_HW_PSX_CLK
#define PSX_DAT CONFIG_HW_PSX_DAT
#define PSX_ATT CONFIG_HW_PSX_ATT
#define PSX_CMD CONFIG_HW_PSX_CMD

// #define DELAY() asm("nop; nop; nop; nop;nop; nop; nop; nop;nop; nop; nop; nop;nop; nop; nop; nop;")

static int  volume = 60;
static int  bright = 13;
// static int  bright;
// static int  inpDelay;
// static bool shutdown;
static bool showMenu;

void setShowMenu(bool val) {
    showMenu = val;
}

bool getShowMenu() {
    return showMenu;
}

// Bit0 Bit1 Bit2 Bit3 Bit4 Bit5 Bit6 Bit7
// SLCT           STRT UP   RGHT DOWN LEFT
// Bit8 Bit9 Bt10 Bt11 Bt12 Bt13 Bt14 Bt15
// L2   R2   L1   R1    /\   O    X   |_|
//  NOTE: These mappings aren't reflected in video_audio.c
//  any changes have to be reflected in osd_getinput
//  TODO: Make these both work together
#define PSX_SELECT     1
#define PSX_START      (1 << 3)
#define PSX_UP         (1 << 4)
#define PSX_RIGHT      (1 << 5)
#define PSX_DOWN       (1 << 6)
#define PSX_LEFT       (1 << 7)
#define PSX_L2         (1 << 8)
#define PSX_R2         (1 << 9)
#define PSX_L1         (1 << 10)
#define PSX_R1         (1 << 11)
#define PSX_TRIANGLE   (1 << 12)
#define PSX_CIRCLE     (1 << 13)
#define PSX_X          (1 << 14)
#define PSX_SQUARE     (1 << 15)
#define A_BUTTON       PSX_CIRCLE
#define B_BUTTON       PSX_X
#define TURBO_A_BUTTON PSX_TRIANGLE
#define TURBO_B_BUTTON PSX_SQUARE
#define MENU_BUTTON    PSX_L1
#define POWER_BUTTON   PSX_R1

bool isSelectPressed(int ctl) {
    return !(ctl & PSX_SELECT);
}
bool isStartPressed(int ctl) {
    return !(ctl & PSX_START);
}
bool isUpPressed(int ctl) {
    return !(ctl & PSX_UP);
}
bool isRightPressed(int ctl) {
    return !(ctl & PSX_RIGHT);
}
bool isDownPressed(int ctl) {
    return !(ctl & PSX_DOWN);
}
bool isLeftPressed(int ctl) {
    return !(ctl & PSX_LEFT);
}
bool isAPressed(int ctl) {
    return !(ctl & A_BUTTON);
}
bool isBPressed(int ctl) {
    return !(ctl & B_BUTTON);
}
bool isTurboAPressed(int ctl) {
    return !(ctl & TURBO_A_BUTTON);
}
bool isTurboBPressed(int ctl) {
    return !(ctl & TURBO_B_BUTTON);
}
bool isMenuPressed(int ctl) {
    return !(ctl & MENU_BUTTON);
}
bool isPowerPressed(int ctl) {
    return !(ctl & POWER_BUTTON);
}
bool isAnyDirectionPressed(int ctl) {
    return isUpPressed(ctl) || isDownPressed(ctl) || isLeftPressed(ctl) || isRightPressed(ctl);
}

bool isAnyActionPressed(int ctl) {
    return isStartPressed(ctl) || isSelectPressed(ctl) || isMenuPressed(ctl) || isPowerPressed(ctl);
}

bool isAnyFirePressed(int ctl) {
    return isAPressed(ctl) || isBPressed(ctl) || isTurboAPressed(ctl) || isTurboBPressed(ctl);
}

bool isAnyPressed(int ctl) {
    return isAnyDirectionPressed(ctl) || isAnyActionPressed(ctl) || isAnyFirePressed(ctl);
}

// static int turboACounter       = 0;
// static int turboBCounter       = 0;
static int turboASpeed = 3;
static int turboBSpeed = 3;
// static int MAX_TURBO           = 6;
// static int TURBO_COUNTER_RESET = 210;

int getTurboA() {
    return turboASpeed;
}

int getTurboB() {
    return turboBSpeed;
}

int getBright() {
    return bright;
}

int incBright() {
    if (bright < 13)
        bright++;
    return bright;
}

int decBright() {
    if (bright > 3)
        bright--;
    return bright;
}

int incVolume() {
    if (volume < 100)
        volume += 5;
    return volume;
}

int decVolume() {
    if (volume > 0)
        volume--;
    return volume;
}

int getVolume() {
    return volume;
}

int kbToControllerState(const bool keys_pressed[]) {
    // Start with no key pressed
    int b2b1 = 0xFFFF;

    if (keys_pressed[0x48]) { // UP key code

        b2b1 -= PSX_UP;
    }

    if (keys_pressed[0x50]) { // DOWN key code

        b2b1 -= PSX_DOWN;
    }

    if (keys_pressed[0x4b]) { // LEFT key code

        b2b1 -= PSX_LEFT;
    }

    if (keys_pressed[0x4d]) { // RIGHT key code

        b2b1 -= PSX_RIGHT;
    }
    // extra keys to make playing platform games easier
    // Right shift is up + right
    if (keys_pressed[0x36]) { // RIGHT SHIFT key code
        b2b1 &= ~(PSX_UP | PSX_RIGHT);
    }
    // The '/' key is up + lift
    if (keys_pressed[0x35]) { // '/' key code
        b2b1 &= ~(PSX_UP | PSX_LEFT);
    }
    if (keys_pressed[0x2a] || keys_pressed[0x1d]) { // SHIFT key code CONTROLLER'A' button.

        b2b1 -= A_BUTTON;
    }
    if (keys_pressed[0x2c]) { // 'Z' key code CONTROLLER'B' button.
        b2b1 -= B_BUTTON;
    }
    if (keys_pressed[0x3b]) { // 'F1' key code select button.
        b2b1 -= PSX_SELECT;
    }

    if (keys_pressed[0x3c]) { // 'F2' key code start button.
        b2b1 -= PSX_START;
    }

    return b2b1;
}

int kbdReadInput() {
    static bsp_input_event_t event;
    static uint8_t           key_code;
    static bool              keys_pressed[128];

    // Read events from input queue
    while (xQueueReceive(input_event_queue, &event, pdMS_TO_TICKS(1))) {
        key_code = event.args_scancode.scancode;
        switch (event.type) {
        case INPUT_EVENT_TYPE_SCANCODE: {
            keys_pressed[key_code & 0x7f] = (key_code & 0x80) ? false : true;
            if (key_code == 0x40) {
                // Toggle show menu when the Purple diamond key is pressed (F6)
                showMenu = !showMenu;
            }
        }
        default:
            break;
        }
    }
    return kbToControllerState(keys_pressed);
}

void kbdControllerInit() {
    if (input_event_queue != NULL) {
        return;
    }
    // TODO: implement
    ESP_LOGI(TAG, "Initializing Tanmatsu keyboard controller");

    ESP_ERROR_CHECK(bsp_input_get_queue(&input_event_queue));

    ESP_LOGI(TAG, "Tanmatsu keyboard controller initialized");

    return;
}