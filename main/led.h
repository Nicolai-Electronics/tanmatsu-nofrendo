#pragma once

#include <stdint.h>
#include "esp_err.h"

esp_err_t led_init(void);

void led_clear_all();

uint8_t* get_led_buffer();

void set_led_color(uint8_t led, uint32_t color);

void show_led_colors(void);