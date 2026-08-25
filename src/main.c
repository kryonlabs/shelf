#include "shelf.h"

int
main(int argc, char **argv)
{
    ShelfApp app;

    InitWindow(900, 620, "Shelf");
    SetTargetFPS(60);
    SetUIDefaultFontAutoLoad(1);
    RefreshSystemTheme();
    SetThemeSource(THEME_SOURCE_SYSTEM);
    SetThemeMode(THEME_MODE_SYSTEM);
    SetThemeStyle(THEME_STYLE_SYSTEM);
    SetCurrentTheme(GetDefaultThemeForThemeStyle(THEME_STYLE_SYSTEM),
                    SystemThemePrefersDark());
    ApplyCurrentUITheme();
    EnsureUIDefaultFont();
    ShelfInit(&app, argc > 1 ? argv[1] : NULL);
    ShelfSetFocused(&app, 1);

    while(!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetThemeBackground());
        BeginUIFrame(GetScreenWidth(), GetScreenHeight(), 1.0f);
        BeginUI(0x5348454c);
        ShelfResize(&app, GetScreenWidth(), GetScreenHeight());
        ShelfDraw(&app, (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()});
        EndUI();
        EndUIFrame();
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
