#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "kryon.h"

#include <time.h>

#define FILE_MANAGER_MAX_ENTRIES 4096
#define FILE_MANAGER_MAX_SELECTION 256
#define FILE_MANAGER_MAX_HISTORY 64
#define FILE_MANAGER_MAX_TABS 8
#define FILE_MANAGER_PATH_MAX 1024

typedef enum FileSortMode {
    FILE_SORT_NAME,
    FILE_SORT_SIZE,
    FILE_SORT_MODIFIED,
    FILE_SORT_TYPE
} FileSortMode;

typedef enum FileClipboardMode {
    FILE_CLIPBOARD_NONE,
    FILE_CLIPBOARD_COPY,
    FILE_CLIPBOARD_CUT
} FileClipboardMode;

typedef struct FileEntry {
    char name[256];
    char path[FILE_MANAGER_PATH_MAX];
    unsigned long long size;
    time_t modified;
    int is_dir;
    int readable;
    int selected;
    int hidden;
} FileEntry;

typedef struct FileClipboard {
    char paths[FILE_MANAGER_MAX_SELECTION][FILE_MANAGER_PATH_MAX];
    int count;
    FileClipboardMode mode;
} FileClipboard;

typedef struct FileTab {
    char path[FILE_MANAGER_PATH_MAX];
} FileTab;

typedef enum FileDialogKind {
    FILE_DIALOG_NONE,
    FILE_DIALOG_RENAME,
    FILE_DIALOG_NEW_FOLDER,
    FILE_DIALOG_NEW_FILE,
    FILE_DIALOG_LOCATION,
    FILE_DIALOG_SEARCH,
    FILE_DIALOG_PROPERTIES,
    FILE_DIALOG_DELETE,
    FILE_DIALOG_TRASH
} FileDialogKind;

typedef struct FileManager {
    char cwd[FILE_MANAGER_PATH_MAX];
    char error[256];
    FileEntry entries[FILE_MANAGER_MAX_ENTRIES];
    int entry_count;
    int cursor;
    int anchor;
    int scroll;
    int last_click_index;
    double last_click_time;
    int focused;
    int width;
    int height;
    int show_hidden;
    int search_active;
    char search_root[FILE_MANAGER_PATH_MAX];
    char search_query[256];
    char typeahead[128];
    double typeahead_time;
    FileSortMode sort_mode;
    int sort_reverse;
    FileTab tabs[FILE_MANAGER_MAX_TABS];
    int tab_count;
    int active_tab;
    char back[FILE_MANAGER_MAX_HISTORY][FILE_MANAGER_PATH_MAX];
    int back_count;
    char forward[FILE_MANAGER_MAX_HISTORY][FILE_MANAGER_PATH_MAX];
    int forward_count;
    FileClipboard clipboard;
    FileDialogKind dialog;
    char dialog_text[FILE_MANAGER_PATH_MAX];
    char dialog_message[2048];
    int dialog_cursor;
    int dialog_focused;
    int context_open;
    int context_x;
    int context_y;
    int menu_open;
    int menu_x;
    int menu_y;
    int menu_defer;
} FileManager;

void FileManagerInit(FileManager *manager, const char *start_path);
void FileManagerResize(FileManager *manager, int width, int height);
void FileManagerSetFocused(FileManager *manager, int focused);
int FileManagerOpenPath(FileManager *manager, const char *path);
int FileManagerOpenPathTracked(FileManager *manager, const char *path);
void FileManagerDraw(FileManager *manager, Rectangle viewport);

int FileManagerCreateFolder(FileManager *manager, const char *name);
int FileManagerCreateFile(FileManager *manager, const char *name);
int FileManagerRenameSelection(FileManager *manager, const char *name);
int FileManagerDeleteSelection(FileManager *manager, int use_trash);
int FileManagerCopySelection(FileManager *manager, int cut);
int FileManagerPaste(FileManager *manager);
int FileManagerDuplicateSelection(FileManager *manager);
int FileManagerMakeLinkSelection(FileManager *manager);
int FileManagerRestoreSelection(FileManager *manager);
int FileManagerSearch(FileManager *manager, const char *query);
int FileManagerNewTab(FileManager *manager, const char *path);
int FileManagerSwitchTab(FileManager *manager, int index);
int FileManagerCloseTab(FileManager *manager, int index);
int FileManagerNextTab(FileManager *manager);
int FileManagerSelectedCount(const FileManager *manager);
void FileManagerSelectAll(FileManager *manager);
void FileManagerClearSelection(FileManager *manager);

#endif
