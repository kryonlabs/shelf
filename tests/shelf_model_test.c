#include "../src/shelf.h"

#include <stdio.h>
#include <string.h>

static int
check(const char *name, int ok)
{
    if(!ok)
        fprintf(stderr, "shelf test failed: %s\n", name);
    return ok ? 0 : 1;
}

int
main(void)
{
    ShelfApp app;
    int failures = 0;

    ShelfInit(&app, ".");
    failures += check("opened current directory", app.cwd[0] == '/');
    failures += check("has entries", app.entry_count > 0);
    failures += check("selected valid",
                      app.selected >= -1 && app.selected < app.entry_count);
    failures += check("bad path rejected",
                      !ShelfOpenPath(&app, "/path/that/does/not/exist"));
    failures += check("error set", app.error[0] != '\0');
    failures += check("can reopen root", ShelfOpenPath(&app, "/"));
    failures += check("root path", strcmp(app.cwd, "/") == 0);
    return failures == 0 ? 0 : 1;
}
