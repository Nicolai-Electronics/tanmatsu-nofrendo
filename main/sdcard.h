#pragma once

#include "driver/sdmmc_host.h"

#define SD_CARD_MOUNT_POINT "/sd"
#define SD_CARD_ROM_PATH    SD_CARD_MOUNT_POINT "/nes_roms"
// #define SKIP_MENU

extern sdmmc_slot_config_t slot_config;
extern sdmmc_host_t        host;
extern sdmmc_card_t        card;
extern sdmmc_card_t*       mount_card;

extern char selectedRomFilename[256];

esp_err_t registerSdCard();
char*     osd_getromdata();