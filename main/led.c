#include "bsp/led.h"
#include "esp_err.h"
#include "esp_log.h"
#include "led.h"
#include <stdint.h>
#include <string.h>

#define NUMBER_OF_LEDS 6

static const char* TAG = "led_controller";

esp_err_t led_init(void) {
    // Clear the all LEDs
    led_clear_all();
    return ESP_OK;
}

/**
 * Returns the LED buffer.
 *
 * @return Pointer to the LED buffer.
 */
uint8_t* get_led_buffer() {
    static uint8_t led_buffer[6 * 3] = {0};
    return led_buffer;
}

/**
 * Update the LED color for a specific LED.
 * @param led LED index (0-(NUMBER_OF_LEDS - 1)).
 * @param color LED color in RGB format (0xRRGGBB).
 */
void set_led_color(uint8_t led, uint32_t color) {
    uint8_t* const led_buffer = get_led_buffer();

    if (led >= NUMBER_OF_LEDS) {
        ESP_LOGI(TAG, "Invalid LED index: %d", led);
        return;
    }

    led_buffer[led * 3 + 0] = (color >> 8) & 0xFF;  // G
    led_buffer[led * 3 + 1] = (color >> 16) & 0xFF; // R
    led_buffer[led * 3 + 2] = (color >> 0) & 0xFF;  // B
}

/**
 * Clear all LEDs (black) and update the LED buffer.
 */
void led_clear_all() {
    uint8_t* const led_buffer = get_led_buffer();
    memset(led_buffer, 0, sizeof(uint8_t) * NUMBER_OF_LEDS * 3);
}

void show_led_colors(void) {
    uint8_t* const led_buffer = get_led_buffer();
    ESP_ERROR_CHECK(bsp_led_write(led_buffer, sizeof(led_buffer)));
}