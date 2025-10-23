#pragma once

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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "hal/lcd_types.h"

//*****************************************************************************
//
// Make sure all of the definitions in this header have a C binding.
//
//*****************************************************************************

#ifdef __cplusplus
extern "C" {
#endif

// Converts NES emulator frame to 565 RGB
void mipi_write_frame(const uint16_t x, const uint16_t y, const uint16_t width, const uint16_t height,
                      const uint8_t data[], bool xStr, bool yStr);

// Expects a frame buffer with RGB565 format
void mipi_blit(const uint16_t* data, const int pic_w, const int pic_h, const int xs, const int ys, const int width,
               const int height);

void mipi_cls(void);

void                                mipi_init();
extern bool                         mipi_initialized;
extern uint16_t*                    mipi_fb;
extern lcd_color_rgb_pixel_format_t display_color_format;
extern size_t                       display_h_res;
extern size_t                       display_v_res;

#ifdef __cplusplus
}
#endif