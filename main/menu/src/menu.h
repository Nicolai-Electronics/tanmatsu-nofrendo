#pragma once
#include <sdcard.h>

// #include <stdint.h>
// #include "esp_err.h"

/**
 * @brief running intro and menu (choose rom/game)
 *
 * @return - integer for choosing game partition
 */
#define ROM_LIST        SD_CARD_ROM_PATH "/roms.txt"
#define FILENAME_LENGTH 128

char* runMenu();
void  setBr(int bright);
void  freeMenuResources();

typedef struct menuEntry
{
    int  entryNumber;
    char icon;
    char name[FILENAME_LENGTH + 1];
    char fileName[FILENAME_LENGTH + 1];
} MenuEntry;

extern int        entryCount;
extern MenuEntry* menuEntries;
