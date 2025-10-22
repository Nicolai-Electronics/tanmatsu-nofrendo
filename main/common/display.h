#pragma once

#include "esp_err.h"
#include "pax_types.h"

esp_err_t  display_init(void);
pax_buf_t* display_get_pax_buffer(void);
uint8_t*   display_get_raw_buffer(void);
void       display_blit_buffer(pax_buf_t* fb);
void       display_blit(void);