#include "mia_storage_internal.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static bool has_traversal(const char *path) {
    if (path == NULL || path[0] == '\0' || path[0] == '/') {
        return true;
    }
    for (const char *cursor = path; *cursor != '\0'; ++cursor) {
        if (*cursor == '\\') {
            return true;
        }
        if (cursor[0] == '.' && cursor[1] == '.' && (cursor[2] == '/' || cursor[2] == '\0') && (cursor == path || cursor[-1] == '/')) {
            return true;
        }
    }
    return false;
}

static MiaStorageStatus join_path(const char *root, const char *suffix, MiaStoragePath *out_path) {
    const int written = snprintf(out_path->path, sizeof(out_path->path), "%s/%s", root, suffix);
    if (written < 0 || (size_t)written >= sizeof(out_path->path)) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "path is too long");
    }
    return mia_storage_ok();
}

static bool is_directory(const char *path) {
    struct stat info;
    return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

static void convert_leaf_case(char *path, int (*convert)(int)) {
    char *leaf = strrchr(path, '/');
    leaf = leaf == NULL ? path : leaf + 1;
    for (; *leaf != '\0'; ++leaf) {
        *leaf = (char)convert((unsigned char)*leaf);
    }
}

MiaStorageStatus mia_storage_resolve_virtual(const MiaStorageContext *context, const char *virtual_path, MiaStoragePath *out_path) {
    if (context == NULL || context->storage_root == NULL || virtual_path == NULL || virtual_path[0] != '/' || out_path == NULL) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "storage root and absolute virtual path are required");
    }
    MiaStorageStatus status = join_path(context->storage_root, virtual_path + 1, out_path);
    if (status.code != MIA_STORAGE_OK || is_directory(out_path->path)) {
        return status;
    }

    MiaStoragePath exact = *out_path;
    char variant[MIA_STORAGE_PATH_MAX];
    const size_t suffix_length = strlen(virtual_path + 1);
    if (suffix_length >= sizeof(variant)) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "path is too long");
    }
    memcpy(variant, virtual_path + 1, suffix_length + 1);
    convert_leaf_case(variant, tolower);
    status = join_path(context->storage_root, variant, out_path);
    if (status.code != MIA_STORAGE_OK || is_directory(out_path->path)) {
        return status;
    }

    memcpy(variant, virtual_path + 1, suffix_length + 1);
    convert_leaf_case(variant, toupper);
    status = join_path(context->storage_root, variant, out_path);
    if (status.code != MIA_STORAGE_OK || is_directory(out_path->path)) {
        return status;
    }

    *out_path = exact;
    return mia_storage_ok();
}

MiaStorageStatus mia_storage_rom_root_path(const MiaStorageContext *context,
                                           const MiaStorageTarget *target,
                                           char *out_path, size_t out_size) {
    if (target == NULL || out_path == NULL || out_size == 0) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "target and output path are required");
    }
    MiaStoragePath root;
    MiaStorageStatus status = mia_storage_resolve_virtual(context, target->rom_root, &root);
    if (status.code != MIA_STORAGE_OK) return status;
    const int written = snprintf(out_path, out_size, "%s", root.path);
    if (written < 0 || (size_t)written >= out_size) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "ROM root path is too long");
    }
    return mia_storage_ok();
}

MiaStorageStatus mia_storage_resolve_child(const MiaStorageContext *context, const char *virtual_root, const char *relative_name, MiaStoragePath *out_path) {
    if (has_traversal(relative_name)) {
        return mia_storage_error(MIA_STORAGE_ERR_PATH_TRAVERSAL, "relative path must stay inside target root");
    }
    MiaStoragePath root;
    MiaStorageStatus status = mia_storage_resolve_virtual(context, virtual_root, &root);
    if (status.code != MIA_STORAGE_OK) {
        return status;
    }
    return join_path(root.path, relative_name, out_path);
}

MiaStorageStatus mia_storage_prepare_child(const MiaStorageContext *context, const char *virtual_root, const char *relative_name, MiaStoragePath *out_path) {
    MiaStorageStatus status = mia_storage_resolve_child(context, virtual_root, relative_name, out_path);
    if (status.code != MIA_STORAGE_OK) {
        return status;
    }
    return mia_storage_make_parents(out_path->path);
}

MiaStorageStatus mia_storage_make_parents(const char *path) {
    char current[MIA_STORAGE_PATH_MAX];
    const size_t length = strlen(path);
    if (length >= sizeof(current)) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "path is too long");
    }
    memcpy(current, path, length + 1);
    for (char *cursor = current + 1; *cursor != '\0'; ++cursor) {
        if (*cursor == '/') {
            *cursor = '\0';
            if (mkdir(current, 0777) != 0 && errno != EEXIST) {
                return mia_storage_error(MIA_STORAGE_ERR_IO, "failed to create parent directory");
            }
            *cursor = '/';
        }
    }
    return mia_storage_ok();
}

bool mia_storage_is_zip(const char *name) {
    const char *dot = strrchr(name, '.');
    if (dot == NULL) {
        return false;
    }
    return tolower((unsigned char)dot[1]) == 'z' && tolower((unsigned char)dot[2]) == 'i' && tolower((unsigned char)dot[3]) == 'p' && dot[4] == '\0';
}

bool mia_storage_extension_matches(const char *name, const MiaStorageTarget *target) {
    const char *dot = strrchr(name, '.');
    if (dot == NULL || dot[1] == '\0') {
        return false;
    }
    for (size_t index = 0; index < target->extension_count; ++index) {
        const char *extension = target->extensions[index];
        if (strlen(dot + 1) == strlen(extension)) {
            bool matches = true;
            for (size_t pos = 0; extension[pos] != '\0'; ++pos) {
                if (tolower((unsigned char)dot[pos + 1]) != tolower((unsigned char)extension[pos])) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
                return true;
            }
        }
    }
    return false;
}
