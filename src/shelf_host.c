#include "shelf.h"

#include <stdlib.h>

typedef struct ShelfHost {
    AppHost host;
    ShelfApp app;
    AppScreenInfo screen;
    int initialized;
} ShelfHost;

static void
shelf_host_init(ShelfHost *host)
{
    if(host == NULL || host->initialized)
        return;
    ShelfInit(&host->app, NULL);
    host->initialized = 1;
}

static int
shelf_screen_count(void *userdata)
{
    (void)userdata;
    return 1;
}

static AppScreenInfo
shelf_screen(void *userdata, int index)
{
    ShelfHost *host = userdata;
    AppScreenInfo empty = {0};

    if(host == NULL || index != 0)
        return empty;
    return host->screen;
}

static void
shelf_select_screen(void *userdata, int index)
{
    (void)userdata;
    (void)index;
}

static int
shelf_select_source_path(void *userdata, const char *source_path)
{
    ShelfHost *host = userdata;

    if(host == NULL || source_path == NULL || source_path[0] == '\0')
        return 0;
    shelf_host_init(host);
    return ShelfOpenPath(&host->app, source_path);
}

static void
shelf_draw(void *userdata, Rectangle viewport)
{
    ShelfHost *host = userdata;

    if(host == NULL)
        return;
    shelf_host_init(host);
    ShelfDraw(&host->app, viewport);
}

static void
shelf_resize(void *userdata, int width, int height)
{
    ShelfHost *host = userdata;

    if(host == NULL)
        return;
    shelf_host_init(host);
    ShelfResize(&host->app, width, height);
}

static void
shelf_set_focused(void *userdata, int focused)
{
    ShelfHost *host = userdata;

    if(host == NULL)
        return;
    shelf_host_init(host);
    ShelfSetFocused(&host->app, focused);
}

AppHost *
ShelfCreateAppHost(int abi_version, const char *project_path)
{
    ShelfHost *host;

    (void)project_path;
    if(abi_version != APP_HOST_ABI_VERSION)
        return NULL;
    host = calloc(1, sizeof(*host));
    if(host == NULL)
        return NULL;
    host->screen.id = "files";
    host->screen.group = "Applications";
    host->screen.title = "Shelf";
    host->host.userdata = host;
    host->host.screen_count = shelf_screen_count;
    host->host.screen = shelf_screen;
    host->host.select_screen = shelf_select_screen;
    host->host.select_source_path = shelf_select_source_path;
    host->host.draw = shelf_draw;
    host->host.resize = shelf_resize;
    host->host.set_focused = shelf_set_focused;
    return &host->host;
}

void
ShelfDestroyAppHost(AppHost *app_host)
{
    free(app_host);
}

AppHost *
CreateAppHost(int abi_version, const char *project_path)
{
    return ShelfCreateAppHost(abi_version, project_path);
}

void
DestroyAppHost(AppHost *app_host)
{
    ShelfDestroyAppHost(app_host);
}
