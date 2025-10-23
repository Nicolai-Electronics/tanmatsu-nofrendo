#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "bitmap.h"

// Render a whole menu frame to the bitmap
void renderInGameMenuFrame(bitmap_t* fb, uint16_t x1, uint16_t y1);

// Retrieve a single pixel from the menu
uint16_t renderInGameMenu(int x, int y, uint16_t x1, uint16_t y1, bool xStr, bool yStr);