#include "../src/shelf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int
check(const char *name, int ok)
{
    if(!ok)
        fprintf(stderr, "file manager test failed: %s\n", name);
    return ok ? 0 : 1;
}

static int
exists(const char *path)
{
    struct stat st;

    return stat(path, &st) == 0;
}

static int
find_entry(FileManager *manager, const char *name)
{
    int i;

    for(i = 0; i < manager->entry_count; i++)
        if(strcmp(manager->entries[i].name, name) == 0)
            return i;
    return -1;
}

static void
join(char *out, int out_size, const char *base, const char *name)
{
    snprintf(out, (size_t)out_size, "%s/%s", base, name);
}

int
main(void)
{
    FileManager manager;
    char root[] = "/tmp/shelf-test-XXXXXX";
    char trash_root[] = "/tmp/shelf-trash-XXXXXX";
    char path[FILE_MANAGER_PATH_MAX];
    int failures = 0;
    int index;

    if(mkdtemp(root) == NULL) {
        perror("mkdtemp");
        return 1;
    }
    if(mkdtemp(trash_root) == NULL) {
        perror("mkdtemp");
        return 1;
    }
    setenv("XDG_DATA_HOME", trash_root, 1);

    FileManagerInit(&manager, root);
    failures += check("opened temporary directory", strcmp(manager.cwd, root) == 0);
    failures += check("starts empty", manager.entry_count == 0);

    failures += check("create folder", FileManagerCreateFolder(&manager, "Folder"));
    failures += check("folder exists", find_entry(&manager, "Folder") >= 0);
    failures += check("create file", FileManagerCreateFile(&manager, "note.txt"));
    failures += check("file exists", find_entry(&manager, "note.txt") >= 0);

    index = find_entry(&manager, "note.txt");
    FileManagerClearSelection(&manager);
    manager.entries[index].selected = 1;
    manager.cursor = index;
    failures += check("rename file",
                      FileManagerRenameSelection(&manager, "renamed.txt"));
    failures += check("renamed entry", find_entry(&manager, "renamed.txt") >= 0);

    index = find_entry(&manager, "renamed.txt");
    FileManagerClearSelection(&manager);
    manager.entries[index].selected = 1;
    manager.cursor = index;
    failures += check("copy selection", FileManagerCopySelection(&manager, 0));
    failures += check("paste copy", FileManagerPaste(&manager));
    failures += check("copy created", manager.entry_count == 3);

    index = find_entry(&manager, "renamed.txt");
    FileManagerClearSelection(&manager);
    manager.entries[index].selected = 1;
    manager.cursor = index;
    failures += check("duplicate selection",
                      FileManagerDuplicateSelection(&manager));
    failures += check("duplicate created", manager.entry_count == 4);

    index = find_entry(&manager, "renamed.txt");
    FileManagerClearSelection(&manager);
    manager.entries[index].selected = 1;
    manager.cursor = index;
    failures += check("make link", FileManagerMakeLinkSelection(&manager));
    failures += check("link entry", find_entry(&manager, "renamed.txt link") >= 0);

    failures += check("search copies", FileManagerSearch(&manager, "copy"));
    failures += check("search active", manager.search_active);
    failures += check("search result", manager.entry_count > 0);
    failures += check("exit search", FileManagerOpenPath(&manager, root));

    index = find_entry(&manager, "Folder");
    FileManagerClearSelection(&manager);
    manager.entries[index].selected = 1;
    manager.cursor = index;
    failures += check("cut selection", FileManagerCopySelection(&manager, 1));
    join(path, sizeof(path), root, "dest");
    mkdir(path, 0755);
    failures += check("open dest", FileManagerOpenPathTracked(&manager, path));
    failures += check("paste cut", FileManagerPaste(&manager));
    failures += check("moved folder into dest", find_entry(&manager, "Folder") >= 0);

    failures += check("new tab", FileManagerNewTab(&manager, root));
    failures += check("two tabs", manager.tab_count == 2);
    failures += check("switch tab", FileManagerSwitchTab(&manager, 0));
    failures += check("close tab", FileManagerCloseTab(&manager, 1));
    failures += check("one tab remains", manager.tab_count == 1);

    failures += check("go back", manager.back_count > 0);
    failures += check("reopen root", FileManagerOpenPath(&manager, root));
    failures += check("hidden file create",
                      FileManagerCreateFile(&manager, ".hidden"));
    failures += check("hidden filtered", find_entry(&manager, ".hidden") < 0);
    manager.show_hidden = 1;
    failures += check("show hidden reopen", FileManagerOpenPath(&manager, root));
    failures += check("hidden visible", find_entry(&manager, ".hidden") >= 0);

    index = find_entry(&manager, ".hidden");
    FileManagerClearSelection(&manager);
    manager.entries[index].selected = 1;
    manager.cursor = index;
    failures += check("trash selection", FileManagerDeleteSelection(&manager, 1));
    join(path, sizeof(path), trash_root, "Trash/files/.hidden");
    failures += check("trash moved file", exists(path));
    join(path, sizeof(path), trash_root, "Trash/files");
    failures += check("open trash", FileManagerOpenPath(&manager, path));
    index = find_entry(&manager, ".hidden");
    failures += check("trashed entry visible", index >= 0);
    FileManagerClearSelection(&manager);
    manager.entries[index].selected = 1;
    manager.cursor = index;
    failures += check("restore trash", FileManagerRestoreSelection(&manager));
    join(path, sizeof(path), root, ".hidden");
    failures += check("restored file exists", exists(path));
    failures += check("reopen root after restore", FileManagerOpenPath(&manager, root));

    index = find_entry(&manager, "renamed.txt");
    FileManagerClearSelection(&manager);
    manager.entries[index].selected = 1;
    manager.cursor = index;
    failures += check("delete selection", FileManagerDeleteSelection(&manager, 0));
    join(path, sizeof(path), root, "renamed.txt");
    failures += check("deleted file gone", !exists(path));

    remove(path);
    return failures == 0 ? 0 : 1;
}
