# Konsool NES Project


## Introduction

NES emulator for the Konsool / Tanmatsu device.
Very early Alpha version, will write the documentation as the features get implemented.

## SD-Card setup

Make sure the SD-Card is formatted in FAT32 format.

### Create the ROM directory

In the root of the SD-Card create a directory names 'nes_roms'.

### Add ROMs

Add ROM (.nes) files to the directory.
Make sure to keep the file names simple as it simplifies entering them in the roms.txt

### Create a ROM index file

Create a roms.txt file in the 'nes_roms' directory,

!! The columns in this file are TAB delimited !!

Use the example roms.txt below for inspiration.

```
No.	Icon	Name	Filename
1.	$	S. Mario Bros 1	super-mario-bros.nes
2.	$	S. Mario Bros 2	super-mario-bros-2.nes
3.	"	S. Mario Bros 3	super-mario-bros-3.nes
4.	=	Legend of Zelda	legend-of-zelda.nes
5.	+	Tetris	tetris.nes
6.	}	Galaga	galaga.nes
7.	;	Metroid	metroid.nes
8.	;	Castlevania	castlevania.nes
9.	}	Bubble Bobble	bubble-bobble.nes
*#*
NOTE: The rest of this file is ignored by the program
7.	/	Contra  contra.nes
10.	$	Castlevania cv1.nes
11.	/	Simon's Quest   cv2.nes 

#   The character delimiter of the table above is tab, not spaces.
#	Don't erase this characters "*#*" or the emulator will definitely act up since that is the EOF marker.
#
#	Note the four tab-delimited columns.  All four should exist or the menu will not parse correctly.
#   There should be a period after the number, followed by an icon character, name, and file name.
#   The last column should match the ROM file name in the nes_roms directory.
#	The menu will display the Alphabet (A-z capital & lowercas)
# 	Numbers 0-9 and  ? ! . , ( ) -
```

The Icon characters point to the following icons:

| char | Icon      | description                   |
| ---- | --------- | ----------------------------- |
| ;    | Coin      | Blinking coin                 |
| /    | Megaman   | Mega man figure               |
| $    | Mario 1   | Logo for Super Mario Bros 1   |
| %    | Luigi     | Mario his brother             |
| "    | Mario 3   | Mario in the Raccoon suit     |
| =    | Link 1    | Legend of Zelda 1 icon        |
| +    | Link 2    | Legend of Zelda 2 icon        |
| }    | Quest box | Quest box (with the '?') icon |
| {    | pacman    | Pacman animated               |

### Directory structure

If all went well, the file structure should look like this.

```
nes_roms
├── bubble-bobble.nes
├── castlevania.nes
├── galaga.nes
├── legend-of-zelda.nes
├── legend-of-zelda.sav
├── metroid.nes
├── roms.txt
├── super-mario-bros-2.nes
├── super-mario-bros-3.nes
├── super-mario-bros.nes
└── tetris.nes
```

## BSP License

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Feel free to remove this file but be sure to add your own license for your app.
If you don't know what license to choose: we recommend releasing your project under terms of the MIT license.

## ESP32_nesemu license

Code in this repository is Copyright (C) 2016 Espressif Systems, licensed under the Apache License 2.0 as described in the file LICENSE. Code in the components/nofrendo is Copyright (c) 1998-2000 Matthew Conte (matt@conte.com) and licensed under the GPLv2.
Any changes in this repository are otherwise presented to you copyright myself and licensed under the same Apache 2.0 license as the Espressif Systems repository.

## Konsool adaptation license 

Copyright 2025 Ranzbak Badge.TEAM

See licenses above, GPLv2 :-)\