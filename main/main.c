#include <stdio.h>
#include "bsp/device.h"
#include "bsp/display.h"
#include "bsp/input.h"
#include "driver/gpio.h"
// #include "driver/sdspi_host.h"
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"
#include "esp_log.h"
#include "hal/lcd_types.h"
#include "nvs_flash.h"
#include "pax_fonts.h"
#include "pax_gfx.h"
#include "pax_text.h"
// #include "sd_protocol_types.h"
#include "menu/src/menu.h"
#include "nofrendo/src/nofrendo.h"

#include "led.h"

// Constants
static char* const TAG = "main";

// Global variables
static esp_lcd_panel_handle_t       display_lcd_panel    = NULL;
static esp_lcd_panel_io_handle_t    display_lcd_panel_io = NULL;
static size_t                       display_h_res        = 0;
static size_t                       display_v_res        = 0;
static lcd_color_rgb_pixel_format_t display_color_format;
static pax_buf_t                    fb                = {0};
static QueueHandle_t                input_event_queue = NULL;

#define READ_BUFFER_SIZE 64

#define ASSERT_ESP_OK(returnCode, message)                          \
	if (returnCode != ESP_OK)                                       \
	{                                                               \
		printf("%s. (%s)\n", message, esp_err_to_name(returnCode)); \
		return returnCode;                                          \
	}

// #define SKIP_MENU
int selectedRomIdx;
#ifdef SKIP_MENU
char *selectedRomFilename = "/spiffs/unknown.nes";
#else
char *selectedRomFilename;
#endif

char *osd_getromdata()
{
	char *romdata;
	// spi_flash_mmap_handle_t handle;
	// esp_err_t err;

	// Locate our target ROM partition where the file will be loaded
	const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0xFF, "rom");
	if (part == 0)
		printf("Couldn't find rom partition!\n");

#ifndef SKIP_MENU
	// Open the file
	printf("Reading rom from %s\n", selectedRomFilename);
	FILE *rom = fopen(selectedRomFilename, "r");
	long fileSize = -1;
	if (!rom)
	{
		printf("Could not read %s\n", selectedRomFilename);
		exit(1);
	}

	// First figure out how large the file is
	fseek(rom, 0L, SEEK_END);
	fileSize = ftell(rom);
	rewind(rom);
	if (fileSize > part->size)
	{
		printf("Rom is too large!  Limit is %dk; Rom file size is %dkb\n",
			   (int)(part->size / 1024),
			   (int)(fileSize / 1024));
		exit(1);
	}

	// Copy the file contents into EEPROM memory
	char buffer[READ_BUFFER_SIZE];
	int offset = 0;
	while (fread(buffer, 1, READ_BUFFER_SIZE, rom) > 0)
	{
		if ((offset % 4096) == 0)
			esp_partition_erase_range(part, offset, 4096);
		esp_partition_write(part, offset, buffer, READ_BUFFER_SIZE);
		offset += READ_BUFFER_SIZE;
	}
	fclose(rom);
	// free(buffer);
	printf("Loaded %d bytes into ROM memory\n", offset);
#else
	size_t fileSize = part->size;
#endif
    // TODO: Replace with correct mmap code
    romdata = malloc(fileSize);

	// Memory-map the ROM partition, which results in 'data' pointer changing to memory-mapped location
	// err = esp_partition_mmap(part, 0, fileSize, SPI_FLASH_MMAP_DATA, (const void **)&romdata, &handle);
	// if (err != ESP_OK)
		// printf("Couldn't map rom partition!\n");
	// printf("Initialized. ROM@%p\n", romdata);

	return (char *)romdata;
}

void esp_wake_deep_sleep()
{
	esp_restart();
}

esp_err_t registerSdCard()
{
    // TODO: Replace with konsool64 SD card code
	// esp_err_t ret;
	// sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	// // host.command_timeout_ms=200;
	// // host.max_freq_khz = SDMMC_FREQ_PROBING;
	// sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
	// slot_config.gpio_miso =  CONFIG_SD_MISO;
	// slot_config.gpio_mosi =  CONFIG_SD_MOSI;
	// slot_config.gpio_sck =   CONFIG_SD_SCK;
	// slot_config.gpio_cs =    CONFIG_SD_CS;
	// slot_config.dma_channel = 2; //2
	// esp_vfs_fat_sdmmc_mount_config_t mount_config = {
	// 	.format_if_mount_failed = false,
	// 	.max_files = 2};

	// sdmmc_card_t *card;
	// //!TODO: Evil hack... don't use spiffs here!
	// ret = esp_vfs_fat_sdmmc_mount("/spiffs", &host, &slot_config, &mount_config, &card);
	// ASSERT_ESP_OK(ret, "Failed to mount SD card");

    // // Card has been initialized, print its properties
    // sdmmc_card_print_info(stdout, card);
	
	return ESP_OK;
}

void blit(void) {
    esp_lcd_panel_draw_bitmap(display_lcd_panel, 0, 0, display_h_res, display_v_res, pax_buf_get_pixels(&fb));
}

int app_main(void) {
    // Start the GPIO interrupt service
    gpio_install_isr_service(0);

    // Initialize the Non Volatile Storage service
    esp_err_t res = nvs_flash_init();
    if (res == ESP_ERR_NVS_NO_FREE_PAGES || res == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        res = nvs_flash_init();
    }
    ESP_ERROR_CHECK(res);

    // Initialize the Board Support Package
    ESP_ERROR_CHECK(bsp_device_initialize());
    ESP_ERROR_CHECK(led_init());

    // Fetch the handle for using the screen, this works even when
    res = bsp_display_get_panel(&display_lcd_panel);
    ESP_ERROR_CHECK(res);                             // Check that the display handle has been initialized
    bsp_display_get_panel_io(&display_lcd_panel_io);  // Do not check result of panel IO handle: not all types of
                                                      // display expose a panel IO handle
    res = bsp_display_get_parameters(&display_h_res, &display_v_res, &display_color_format);
    ESP_ERROR_CHECK(res);  // Check that the display parameters have been initialized

    ESP_ERROR_CHECK(bsp_input_get_queue(&input_event_queue));

    // Register SD card
    ASSERT_ESP_OK(registerSdCard(), "Unable to register SD Card");

    // Initialize the graphics stack
    pax_buf_init(&fb, NULL, display_h_res, display_v_res, PAX_BUF_16_565RGB);
    pax_buf_reversed(&fb, false);
    pax_buf_set_orientation(&fb, PAX_O_ROT_CW);

    pax_background(&fb, 0xFFFFFFFF);
    pax_draw_text(&fb, 0xFF000000, pax_font_sky_mono, 16, 0, 0, "Hello world!");
    blit();

    #ifndef SKIP_MENU
        selectedRomFilename = runMenu();
    #endif

	ESP_LOGI(TAG, "NoFrendo start!\n");
	nofrendo_main(0, NULL);
	ESP_LOGE(TAG, "NoFrendo died? WtF?\n");
    // Red screen of death
    pax_background(&fb, 0xFFFF0000);
    blit();
	return 0;
}
