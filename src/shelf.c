#include "shelf.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef KRYON_NATIVE_PLAN9
#include <strings.h>
#endif
#include <sys/stat.h>
#include <unistd.h>

#ifdef KRYON_NATIVE_PLAN9
#ifndef R_OK
#define R_OK 4
#endif

static char *
shelf_realpath(const char *path, char *resolved)
{
    if(path == NULL || resolved == NULL)
        return NULL;
    snprintf(resolved, SHELF_PATH_MAX, "%s", path);
    cleanname(resolved);
    return resolved;
}

static int
shelf_access(const char *path, int mode)
{
    Dir *dir;

    (void)mode;
    if(path == NULL)
        return -1;
    dir = dirstat((char*)path);
    if(dir == nil)
        return -1;
    free(dir);
    return 0;
}

#define realpath(path, resolved) shelf_realpath(path, resolved)
#define access(path, mode) shelf_access(path, mode)
#endif

static void
copy_text(char *dst, int size, const char *src)
{
    if(dst == NULL || size <= 0)
        return;
    snprintf(dst, (size_t)size, "%s", src != NULL ? src : "");
}

static Color
opaque_color(Color color)
{
    color.a = 255;
    return color;
}

static void
join_path(char *out, int out_size, const char *base, const char *name)
{
    int base_len;

    if(out == NULL || out_size <= 0)
        return;
    if(base == NULL || base[0] == '\0' || strcmp(base, "/") == 0) {
        snprintf(out, (size_t)out_size, "/%.*s", out_size - 2, name);
        return;
    }
    base_len = (int)strlen(base);
    if(base_len > out_size - 2)
        base_len = out_size - 2;
    snprintf(out, (size_t)out_size, "%.*s/%.*s", base_len, base,
             out_size - base_len - 2, name);
}

static void
copy_error(char *out, int out_size, const char *path)
{
#ifdef KRYON_NATIVE_PLAN9
    char err[ERRMAX];
#endif

    if(out == NULL || out_size <= 0)
        return;
#ifdef KRYON_NATIVE_PLAN9
    errstr(err, sizeof(err));
    snprintf(out, (size_t)out_size, "%.160s: %.80s", path != NULL ? path : "",
             err);
#else
    snprintf(out, (size_t)out_size, "%.160s: %.80s", path != NULL ? path : "",
             strerror(errno));
#endif
}

static void
parent_path(char *out, int out_size, const char *path)
{
    char tmp[SHELF_PATH_MAX];
    char *slash;

    copy_text(tmp, sizeof(tmp), path);
    slash = strrchr(tmp, '/');
    if(slash == NULL || slash == tmp) {
        copy_text(out, out_size, "/");
        return;
    }
    *slash = '\0';
    copy_text(out, out_size, tmp);
}

static int
entry_compare(const void *a, const void *b)
{
    const ShelfEntry *ea = a;
    const ShelfEntry *eb = b;

    if(ea->is_dir != eb->is_dir)
        return eb->is_dir - ea->is_dir;
    return strcasecmp(ea->name, eb->name);
}

static void
format_size(char *out, int out_size, unsigned long long size, int is_dir)
{
    if(is_dir) {
        copy_text(out, out_size, "Folder");
    } else if(size >= 1024ull * 1024ull * 1024ull) {
        snprintf(out, (size_t)out_size, "%llu GB",
                 size / (1024ull * 1024ull * 1024ull));
    } else if(size >= 1024ull * 1024ull) {
        snprintf(out, (size_t)out_size, "%llu MB", size / (1024ull * 1024ull));
    } else if(size >= 1024ull) {
        snprintf(out, (size_t)out_size, "%llu KB", size / 1024ull);
    } else {
        snprintf(out, (size_t)out_size, "%llu B", size);
    }
}

int
ShelfOpenPath(ShelfApp *app, const char *path)
{
    DIR *dir;
    struct dirent *de;
    char resolved[SHELF_PATH_MAX];
    int count = 0;

    if(app == NULL || path == NULL || path[0] == '\0')
        return 0;
    if(realpath(path, resolved) == NULL) {
        copy_error(app->error, sizeof(app->error), path);
        return 0;
    }
    dir = opendir(resolved);
    if(dir == NULL) {
        copy_error(app->error, sizeof(app->error), resolved);
        return 0;
    }

    while((de = readdir(dir)) != NULL && count < SHELF_MAX_ENTRIES) {
        ShelfEntry *entry;
        struct stat st;

        if(strcmp(de->d_name, ".") == 0)
            continue;
        entry = &app->entries[count];
        memset(entry, 0, sizeof(*entry));
        copy_text(entry->name, sizeof(entry->name), de->d_name);
        join_path(entry->path, sizeof(entry->path), resolved, de->d_name);
        if(stat(entry->path, &st) == 0) {
            entry->is_dir = S_ISDIR(st.st_mode);
            entry->size = (unsigned long long)st.st_size;
            entry->readable = access(entry->path, R_OK) == 0;
        }
        count++;
    }
    closedir(dir);
    qsort(app->entries, (size_t)count, sizeof(app->entries[0]), entry_compare);
    copy_text(app->cwd, sizeof(app->cwd), resolved);
    app->entry_count = count;
    app->selected = count > 0 ? 0 : -1;
    app->scroll = 0;
    app->error[0] = '\0';
    return 1;
}

void
ShelfInit(ShelfApp *app, const char *start_path)
{
    const char *home;

    if(app == NULL)
        return;
    memset(app, 0, sizeof(*app));
    app->selected = -1;
    app->width = 640;
    app->height = 420;
    home = getenv("HOME");
    if(start_path == NULL || start_path[0] == '\0')
        start_path = home != NULL && home[0] != '\0' ? home : "/";
    if(!ShelfOpenPath(app, start_path))
        (void)ShelfOpenPath(app, "/");
}

void
ShelfResize(ShelfApp *app, int width, int height)
{
    if(app == NULL)
        return;
    app->width = width;
    app->height = height;
}

void
ShelfSetFocused(ShelfApp *app, int focused)
{
    if(app != NULL)
        app->focused = focused != 0;
}

static int
hit(Rectangle r)
{
    return CheckCollisionPointRec(GetMousePosition(), r);
}

static void
draw_text_fit_right(const char *text, int x, int y, int max_width,
                    int font_size, Color color)
{
    int width;

    if(text == NULL)
        text = "";
    while(font_size > Text8 && TextWidth(text, font_size) > max_width)
        font_size -= 2;
    width = TextWidth(text, font_size);
    Text(text, x + max_width - width, y, font_size, color);
}

static void
draw_icon(Rectangle r, int is_dir)
{
    Color c = is_dir ? GetThemeLink() : GetThemeIcon();

    if(is_dir) {
        DrawRectangleRounded((Rectangle){r.x + 2, r.y + 9, r.width - 4,
                                         r.height - 11},
                             0.08f, 4, Fade(c, 0.82f));
        DrawRectangleRounded((Rectangle){r.x + 4, r.y + 5, r.width * 0.44f,
                                         8},
                             0.08f, 4, c);
    } else {
        DrawRectangleRounded((Rectangle){r.x + 5, r.y + 3, r.width - 10,
                                         r.height - 6},
                             0.04f, 4, Fade(GetThemeSurface(), 0.96f));
        DrawRectangleRoundedLinesEx((Rectangle){r.x + 5, r.y + 3,
                                                r.width - 10, r.height - 6},
                                    0.04f, 4, 1.0f, c);
    }
}

static void
open_parent(ShelfApp *app)
{
    char parent[SHELF_PATH_MAX];

    parent_path(parent, sizeof(parent), app->cwd);
    (void)ShelfOpenPath(app, parent);
}

static void
open_selected(ShelfApp *app)
{
    if(app == NULL || app->selected < 0 || app->selected >= app->entry_count)
        return;
    if(app->entries[app->selected].is_dir)
        (void)ShelfOpenPath(app, app->entries[app->selected].path);
}

void
ShelfDraw(ShelfApp *app, Rectangle viewport)
{
    Rectangle toolbar;
    Rectangle list;
    int row_h = 30;
    int visible;
    int max_scroll;
    int i;
    float wheel;

    if(app == NULL)
        return;
    toolbar = (Rectangle){viewport.x, viewport.y, viewport.width, 38};
    list = (Rectangle){viewport.x, viewport.y + 39, viewport.width,
                       viewport.height - 39};
    DrawRectangleRec(viewport, opaque_color(GetThemeBackground()));
    DrawRectangleRec(toolbar, opaque_color(GetThemeSurface()));
    DrawRectangle((int)viewport.x, (int)(toolbar.y + toolbar.height - 1),
                  (int)viewport.width, 1, Fade(GetThemeText(), 0.18f));

    if(Button((ButtonProps){(Rectangle){toolbar.x + 8, toolbar.y + 6, 32, 26},
                            "<", ButtonStyleSecondary, Text14, 1001, 0}))
        open_parent(app);
    if(Button((ButtonProps){(Rectangle){toolbar.x + 44, toolbar.y + 6, 72, 26},
                            "Reload", ButtonStyleSecondary, Text12, 1002, 0}))
        (void)ShelfOpenPath(app, app->cwd);
    BeginScissorMode((int)toolbar.x + 124, (int)toolbar.y,
                     (int)toolbar.width - 132, (int)toolbar.height);
    Text(app->cwd, (int)toolbar.x + 128, (int)toolbar.y + 11, Text12,
         GetThemeText());
    EndScissorMode();

    visible = (int)(list.height / row_h);
    if(visible < 1)
        visible = 1;
    max_scroll = app->entry_count - visible;
    if(max_scroll < 0)
        max_scroll = 0;
    if(app->focused && hit(list)) {
        wheel = GetMouseWheelMove();
        if(wheel > 0.0f)
            app->scroll -= 3;
        else if(wheel < 0.0f)
            app->scroll += 3;
    }
    if(app->scroll < 0)
        app->scroll = 0;
    if(app->scroll > max_scroll)
        app->scroll = max_scroll;
    if(app->focused && IsKeyPressed(KEY_BACKSPACE))
        open_parent(app);
    if(app->focused && IsKeyPressed(KEY_ENTER))
        open_selected(app);

    BeginScissorMode((int)list.x, (int)list.y, (int)list.width,
                     (int)list.height);
    for(i = 0; i < visible && i + app->scroll < app->entry_count; i++) {
        int index = i + app->scroll;
        ShelfEntry *entry = &app->entries[index];
        Rectangle row = {list.x, list.y + i * row_h, list.width, row_h};
        int hover = app->focused && hit(row);
        char size_text[32];

        if(index == app->selected)
            DrawRectangleRec(row, Fade(GetThemeButtonHover(), 0.70f));
        else if(hover)
            DrawRectangleRec(row, Fade(GetThemeButton(), 0.42f));
        DrawRectangle((int)row.x, (int)(row.y + row.height - 1),
                      (int)row.width, 1, Fade(GetThemeText(), 0.08f));
        draw_icon((Rectangle){row.x + 8, row.y + 3, 24, 24}, entry->is_dir);
        BeginScissorMode((int)row.x + 38, (int)row.y,
                         (int)row.width - 150, (int)row.height);
        Text(entry->name, (int)row.x + 40, (int)row.y + 8, Text12,
             entry->readable ? GetThemeText() : GetThemeIcon());
        EndScissorMode();
        format_size(size_text, sizeof(size_text), entry->size, entry->is_dir);
        draw_text_fit_right(size_text, (int)(row.x + row.width - 104),
                            (int)row.y + 8, 88, Text12, GetThemeIcon());
        if(hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            app->selected = index;
            if(entry->is_dir)
                (void)ShelfOpenPath(app, entry->path);
        }
    }
    EndScissorMode();

    if(app->entry_count == 0 && app->error[0] == '\0')
        Text("Empty folder", (int)list.x + 16, (int)list.y + 16, Text14,
             GetThemeIcon());
    if(app->error[0] != '\0')
        Text(app->error, (int)list.x + 16, (int)list.y + 16, Text14, RED);
}
