#include "bsp/display.h"
#include "esp_err.h"
#include "driver/ppa.h"

esp_lcd_panel_handle_t       display_lcd_panel;
esp_lcd_panel_io_handle_t    display_lcd_panel_io;
lcd_color_rgb_pixel_format_t display_color_format;
size_t                       display_h_res;
size_t                       display_v_res;


// PPA blitting handles
ppa_client_handle_t    ppa_srm_handle  = NULL;
ppa_client_handle_t    ppa_fill_handle = NULL;
ppa_client_config_t    ppa_srm_config;
ppa_client_config_t    ppa_fill_config;
ppa_srm_oper_config_t  active_config;
ppa_fill_oper_config_t fill_config;

esp_err_t init(void) {
    esp_err_t res = bsp_display_get_panel(&display_lcd_panel);
 

    ESP_ERROR_CHECK(res);                             // Check that the display handle has been initialized
    bsp_display_get_panel_io(&display_lcd_panel_io);  // Do not check result of panel IO handle: not all types of
                                                      // display expose a panel IO handle
    res = bsp_display_get_parameters(&display_h_res, &display_v_res, &display_color_format);
    ESP_ERROR_CHECK(res);  // Check that the display parameters have been initialized 

    return ESP_OK;
}
