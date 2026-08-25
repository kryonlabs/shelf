#ifndef SHELF_H
#define SHELF_H

#include "kryon.h"

#define SHELF_MAX_ENTRIES 512
#define SHELF_PATH_MAX 1024

typedef struct ShelfEntry {
    char name[256];
    char path[SHELF_PATH_MAX];
    unsigned long long size;
    int is_dir;
    int readable;
} ShelfEntry;

typedef struct ShelfApp {
    char cwd[SHELF_PATH_MAX];
    char error[256];
    ShelfEntry entries[SHELF_MAX_ENTRIES];
    int entry_count;
    int selected;
    int scroll;
    int focused;
    int width;
    int height;
} ShelfApp;

void ShelfInit(ShelfApp *app, const char *start_path);
void ShelfResize(ShelfApp *app, int width, int height);
void ShelfSetFocused(ShelfApp *app, int focused);
int ShelfOpenPath(ShelfApp *app, const char *path);
void ShelfDraw(ShelfApp *app, Rectangle viewport);

#endif
