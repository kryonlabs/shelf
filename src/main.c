#include "shelf.h"

int
main(int argc, char **argv)
{
    FileManager manager;

    InitWindow(900, 620, "Files");
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
#ifdef KRYON_NATIVE_PLAN9
    FileManagerInit(&manager, argc > 1 ? argv[1] : getenv("home"));
#else
    FileManagerInit(&manager, argc > 1 ? argv[1] : NULL);
#endif
    FileManagerSetFocused(&manager, 1);

    while(!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetThemeBackground());
        BeginUIFrame(GetScreenWidth(), GetScreenHeight(), 1.0f);
        BeginUI(0x46494c45);
        FileManagerResize(&manager, GetScreenWidth(), GetScreenHeight());
        FileManagerDraw(&manager,
                        (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()});
        EndUI();
        EndUIFrame();
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
