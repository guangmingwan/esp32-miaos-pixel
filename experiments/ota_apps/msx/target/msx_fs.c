#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#undef opendir
#undef readdir
#undef fopen
#undef stat
#undef unlink
#undef rename

int UseSound, UseZoom, SyncFreq, UseEffects;
int ARGC;
char **ARGV;

static char cwd[256] = "/";
static char resolved[512];

static const char *resolve(const char *path) {
    if (path[0] == '/') return path;
    strlcpy(resolved, cwd, sizeof(resolved));
    if (strcmp(cwd, "/") != 0) strlcat(resolved, "/", sizeof(resolved));
    strlcat(resolved, path, sizeof(resolved));
    return resolved;
}

int msx_chdir(const char *path) {
    const char *absolute = resolve(path);
    if (strlen(absolute) >= sizeof(cwd)) return -1;
    strcpy(cwd, absolute);
    return 0;
}

char *msx_getcwd(char *buffer, size_t size) {
    if (buffer == NULL) return strdup(cwd);
    if (strlen(cwd) + 1 > size) return NULL;
    return strcpy(buffer, cwd);
}

DIR *msx_opendir(const char *path) { return opendir(resolve(path)); }
struct dirent *msx_readdir(DIR *directory) { return readdir(directory); }
FILE *msx_fopen(const char *path, const char *mode) { return fopen(resolve(path), mode); }
int msx_stat(const char *path, struct stat *info) { return stat(resolve(path), info); }
int msx_unlink(const char *path) { return unlink(resolve(path)); }
int msx_rename(const char *old_path, const char *new_path) {
    char old_resolved[512];
    strncpy(old_resolved, resolve(old_path), sizeof(old_resolved) - 1);
    old_resolved[sizeof(old_resolved) - 1] = '\0';
    return rename(old_resolved, resolve(new_path));
}
