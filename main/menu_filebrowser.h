#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    MENU_FILEBROWSER_RESULT_CANCEL,
    MENU_FILEBROWSER_RESULT_SELECTED,
    MENU_FILEBROWSER_RESULT_EXIT,
    MENU_FILEBROWSER_RESULT_HELP,
} menu_filebrowser_result_t;

menu_filebrowser_result_t menu_filebrowser(const char* path, const char* filter[], size_t filter_length,
                                           char* out_filename, size_t filename_size, const char* title);
