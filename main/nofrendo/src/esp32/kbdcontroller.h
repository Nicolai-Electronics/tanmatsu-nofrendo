#pragma once

#include <stdbool.h>
int  kbdReadInput();
void kbdControllerInit();
bool getShowMenu();
void setShowMenu(bool val);
int  incVolume();
int  decVolume();
int  getBright();
int  incBright();
int  decBright();
int  getVolume();
bool getShutdown();
bool isSelectPressed(int ctl);
bool isStartPressed(int ctl);
bool isUpPressed(int ctl);
bool isRightPressed(int ctl);
bool isDownPressed(int ctl);
bool isLeftPressed(int ctl);
bool isAPressed(int ctl);
bool isBPressed(int ctl);
bool isTurboAPressed(int ctl);
bool isTurboBPressed(int ctl);
bool isMenuPressed(int ctl);
bool isPowerPressed(int ctl);
bool isAnyPressed(int ctl);
bool isAnyFirePressed(int ctl);
int  getTurboA();
int  getTurboB();
int  getVolume();