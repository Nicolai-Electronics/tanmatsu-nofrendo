#pragma once

#include <stdbool.h>

bool cpGetPixel(char cpChar, int cp1, int cp2);

void initRomList();

int getCharPixel(int x, int y, int change, int choosen);

void freeRL(void);
