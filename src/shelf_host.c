#include "shelf.h"

#include <stdlib.h>

typedef struct FileHost {
    AppHost host;
    FileManager manager;
    AppScreenInfo screen;
    int initialized;
} FileHost;

static void
file_host_init(FileHost *host)
{
    if(host == NULL || host->initialized)
        return;
    FileManagerInit(&host->manager, NULL);
    host->initialized = 1;
}

static int
file_screen_count(void *userdata)
{
    (void)userdata;
    return 1;
}

static AppScreenInfo
file_screen(void *userdata, int index)
{
    FileHost *host = userdata;
    AppScreenInfo empty = {0};

    if(host == NULL || index != 0)
        return empty;
    return host->screen;
}

static void
file_select_screen(void *userdata, int index)
{
    (void)userdata;
    (void)index;
}

static int
file_select_source_path(void *userdata, const char *source_path)
{
    FileHost *host = userdata;

    if(host == NULL || source_path == NULL || source_path[0] == '\0')
        return 0;
    file_host_init(host);
    return FileManagerOpenPathTracked(&host->manager, source_path);
}

static void
file_draw(void *userdata, Rectangle viewport)
{
    FileHost *host = userdata;

    if(host == NULL)
        return;
    file_host_init(host);
    FileManagerDraw(&host->manager, viewport);
}

static void
file_resize(void *userdata, int width, int height)
{
    FileHost *host = userdata;

    if(host == NULL)
        return;
    file_host_init(host);
    FileManagerResize(&host->manager, width, height);
}

static void
file_set_focused(void *userdata, int focused)
{
    FileHost *host = userdata;

    if(host == NULL)
        return;
    file_host_init(host);
    FileManagerSetFocused(&host->manager, focused);
}

static AppHost *
create_file_manager_host(int abi_version, const char *project_path)
{
    FileHost *host;

    (void)project_path;
    if(abi_version != APP_HOST_ABI_VERSION)
        return NULL;
    host = calloc(1, sizeof(*host));
    if(host == NULL)
        return NULL;
    host->screen.id = "files";
    host->screen.group = "Applications";
    host->screen.title = "Files";
    host->host.userdata = host;
    host->host.screen_count = file_screen_count;
    host->host.screen = file_screen;
    host->host.select_screen = file_select_screen;
    host->host.select_source_path = file_select_source_path;
    host->host.draw = file_draw;
    host->host.resize = file_resize;
    host->host.set_focused = file_set_focused;
    return &host->host;
}

static void
destroy_file_manager_host(AppHost *app_host)
{
    free(app_host);
}

AppHost *
CreateAppHost(int abi_version, const char *project_path)
{
    return create_file_manager_host(abi_version, project_path);
}

void
DestroyAppHost(AppHost *app_host)
{
    destroy_file_manager_host(app_host);
}
