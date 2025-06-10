
#include "sdcard.h"
#include "driver/sdmmc_host.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "hal/ldo_types.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#include "sdmmc_cmd.h"
#include "targets/tanmatsu/tanmatsu_hardware.h"
#include <string.h>
#include <sys/stat.h>

static const char* TAG = "sd_card_setup";

#define READ_BUFFER_SIZE 64

#ifdef SKIP_MENU
char* selectedRomFilename = SD_CARD_ROM_PATH "/super-mario-bros-3.nes";
#else
char* selectedRomFilename;
#endif

sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
sdmmc_host_t        host        = SDMMC_HOST_DEFAULT();
sdmmc_card_t        card;
sdmmc_card_t*       mount_card;

esp_err_t registerSdCard() {
    esp_err_t ret;

    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = LDO_UNIT_4, // SDCard powered by VO4
    };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create a new on-chip LDO power control driver");
        return ret;
    }
    host.pwr_ctrl_handle = pwr_ctrl_handle;

    vTaskDelay(500 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Setup sdio slot");

    slot_config.clk    = BSP_SDCARD_CLK;
    slot_config.cmd    = BSP_SDCARD_CMD;
    slot_config.d0     = BSP_SDCARD_D0;
    slot_config.d1     = BSP_SDCARD_D1;
    slot_config.d2     = BSP_SDCARD_D2;
    slot_config.d3     = BSP_SDCARD_D3;
    slot_config.width  = 4;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ESP_LOGI(TAG, "Mounting SDcard");
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed   = false,
        .max_files                = 5,
        .allocation_unit_size     = 16 * 1024,
        .disk_status_check_enable = false,
        .use_one_fat              = false,
    };

    static const char mount_point[] = SD_CARD_MOUNT_POINT;

    ret = esp_vfs_fat_sdmmc_mount(mount_point,
                                  &host,
                                  &slot_config,
                                  &mount_config,
                                  &mount_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        return false;
    }

    // Get some info about the card
    sdmmc_card_print_info(stdout, mount_card);

    ESP_LOGI(TAG, "SDcard initialized");

    // Make sure the C64PRG directory exists if it doesn't already exist
    ESP_LOGI(TAG, "Checking if PRG directory exists");
    struct stat st;
    if (stat(SD_CARD_ROM_PATH, &st) != 0) {
        // directory does not exist, create it
        if (mkdir(SD_CARD_ROM_PATH, 0775) != 0) {
            ESP_LOGE(TAG, "Failed to create directory %s", SD_CARD_ROM_PATH);
            return false;
        }
        ESP_LOGE(TAG, "PRG directory has been created: %s", SD_CARD_ROM_PATH);
    } else if (!S_ISDIR(st.st_mode)) {
        ESP_LOGE(TAG, "%s is not a directory", SD_CARD_ROM_PATH);
        return false;
    } else {
        ESP_LOGI(TAG, "Found NES ROM directory: %s", SD_CARD_ROM_PATH);
    }

    return ESP_OK;
}

char* osd_getromdata() {
    char* romdata;

    // Open the file
    printf("Reading rom from %s\n", selectedRomFilename);
    FILE* rom      = fopen(selectedRomFilename, "r");
    long  fileSize = -1;
    if (!rom) {
        printf("Could not read %s\n", selectedRomFilename);
        return NULL;
    }

    // First figure out how large the file is
    fseek(rom, 0L, SEEK_END);
    fileSize = ftell(rom);
    rewind(rom);

    // Allocate RAM for the ROM data
    romdata = (char*)malloc(fileSize);

    // Copy the file contents into EEPROM memory
    // Since the P4 has much more memory, we are not going to bother and just read to RAM
    fread(romdata, 1, fileSize, rom);
    fclose(rom);
    ESP_LOGI(TAG, "Initialized. ROM@%p size: %ld kb", romdata, fileSize / 1024);

    return (char*)romdata;
}