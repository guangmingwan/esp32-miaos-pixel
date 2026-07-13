#include "mia_storage_internal.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static bool path_is_symlink(const char *path) {
#ifdef ESP_PLATFORM
    (void)path;
    return false;
#else
    char target[2];
    return readlink(path, target, sizeof(target)) >= 0;
#endif
}

static MiaStorageStatus append_entry(MiaStoragePickerResult *result, size_t *capacity, const char *name, const char *path, MiaStorageEntryKind kind, uint64_t size) {
    if (result->count == *capacity) {
        const size_t next_capacity = *capacity == 0 ? 32u : *capacity * 2u;
        MiaStoragePickerEntry *entries = realloc(result->entries, next_capacity * sizeof(*entries));
        if (entries == NULL) {
            return mia_storage_error(MIA_STORAGE_ERR_IO, "failed to allocate picker entry");
        }
        result->entries = entries;
        *capacity = next_capacity;
    }
    char *entry_name = strdup(name);
    char *entry_path = strdup(path);
    if (entry_name == NULL || entry_path == NULL) {
        free(entry_name);
        free(entry_path);
        return mia_storage_error(MIA_STORAGE_ERR_IO, "failed to allocate picker strings");
    }
    result->entries[result->count] = (MiaStoragePickerEntry){entry_name, entry_path, kind, size};
    result->count += 1;
    return mia_storage_ok();
}

void mia_storage_picker_free(MiaStoragePickerResult *result) {
    if (result == NULL) {
        return;
    }
    for (size_t index = 0; index < result->count; ++index) {
        free(result->entries[index].name);
        free(result->entries[index].path);
    }
    free(result->entries);
    result->entries = NULL;
    result->count = 0;
}

MiaStorageStatus mia_storage_picker_list(const MiaStorageContext *context, const MiaStorageTarget *target, MiaStoragePickerResult *out_result) {
    if (target == NULL || out_result == NULL) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "target and result are required");
    }
    *out_result = (MiaStoragePickerResult){0};
    size_t capacity = 0;
    size_t roots_found = 0;
    for (size_t root_index = 0; root_index < mia_storage_rom_root_count(target); ++root_index) {
        MiaStoragePath root;
        MiaStorageStatus status = mia_storage_resolve_virtual(
            context, mia_storage_rom_root_at(target, root_index), &root);
        if (status.code != MIA_STORAGE_OK) continue;
        DIR *dir = opendir(root.path);
        if (dir == NULL) continue;
        roots_found++;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || mia_storage_is_zip(entry->d_name)) {
                continue;
            }
            MiaStoragePath child;
            const int written = snprintf(child.path, sizeof(child.path), "%s/%s", root.path, entry->d_name);
            if (written < 0 || (size_t)written >= sizeof(child.path)) {
                continue;
            }
#ifdef ESP_PLATFORM
            if (entry->d_type == DT_DIR) {
                status = append_entry(out_result, &capacity, entry->d_name, child.path, MIA_STORAGE_ENTRY_DIRECTORY, 0);
            } else if (entry->d_type == DT_REG && mia_storage_extension_matches(entry->d_name, target)) {
                status = append_entry(out_result, &capacity, entry->d_name, child.path, MIA_STORAGE_ENTRY_ROM, 0);
            } else {
                continue;
            }
#else
            struct stat info;
            if (path_is_symlink(child.path) || stat(child.path, &info) != 0) {
                continue;
            }
            if (S_ISDIR(info.st_mode)) {
                status = append_entry(out_result, &capacity, entry->d_name, child.path, MIA_STORAGE_ENTRY_DIRECTORY, 0);
            } else if (S_ISREG(info.st_mode) && mia_storage_extension_matches(entry->d_name, target)) {
                status = append_entry(out_result, &capacity, entry->d_name, child.path, MIA_STORAGE_ENTRY_ROM, (uint64_t)info.st_size);
            } else {
                continue;
            }
#endif
            if (status.code != MIA_STORAGE_OK) {
                closedir(dir);
                mia_storage_picker_free(out_result);
                return status;
            }
        }
        closedir(dir);
    }
    return roots_found > 0u ? mia_storage_ok() :
        mia_storage_error(MIA_STORAGE_ERR_MISSING_ROOT, "ROM root is missing");
}

MiaStorageStatus mia_storage_picker_select(const MiaStorageContext *context, const MiaStorageTarget *target, const char *relative_name, MiaStoragePickerResult *out_result) {
    if (target == NULL || out_result == NULL || relative_name == NULL) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "target, name, and result are required");
    }
    *out_result = (MiaStoragePickerResult){0};
    if (mia_storage_is_zip(relative_name)) {
        return mia_storage_error(MIA_STORAGE_ERR_UNSUPPORTED_ARCHIVE, "ZIP archives are not supported");
    }
    if (!mia_storage_extension_matches(relative_name, target)) {
        return mia_storage_error(MIA_STORAGE_ERR_UNSUPPORTED_FILE, "file extension does not match target");
    }
    for (size_t root_index = 0; root_index < mia_storage_rom_root_count(target); ++root_index) {
        MiaStoragePath path;
        MiaStorageStatus status = mia_storage_resolve_child(
            context, mia_storage_rom_root_at(target, root_index), relative_name, &path);
        if (status.code != MIA_STORAGE_OK) return status;
        struct stat info;
        if (path_is_symlink(path.path) || stat(path.path, &info) != 0 || !S_ISREG(info.st_mode)) {
            continue;
        }
        size_t capacity = 0;
        return append_entry(out_result, &capacity, relative_name, path.path,
                            MIA_STORAGE_ENTRY_ROM, (uint64_t)info.st_size);
    }
    return mia_storage_error(MIA_STORAGE_ERR_MISSING_ROOT, "selected ROM is missing");
}

static bool wildcard_wad_exists(const char *directory) {
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        return false;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (mia_storage_is_zip(entry->d_name)) {
            continue;
        }
        const char *dot = strrchr(entry->d_name, '.');
        if (dot != NULL && strcasecmp(dot, ".wad") == 0) {
            closedir(dir);
            return true;
        }
    }
    closedir(dir);
    return false;
}

MiaStorageStatus mia_storage_validate_requirements(const MiaStorageContext *context, const MiaStorageTarget *target) {
    if (target == NULL) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "target is required");
    }
    for (size_t index = 0; index < target->requirement_count; ++index) {
        const MiaStorageRequirement *requirement = &target->requirements[index];
        if (!requirement->required) {
            continue;
        }
        MiaStoragePath path;
        if (strstr(requirement->path, "*.wad") != NULL) {
            MiaStorageStatus status = mia_storage_resolve_virtual(context, target->rom_root, &path);
            if (status.code != MIA_STORAGE_OK || !wildcard_wad_exists(path.path)) {
                return mia_storage_error(MIA_STORAGE_ERR_MISSING_REQUIRED_FILE, "required IWAD is missing");
            }
            continue;
        }
        MiaStorageStatus status = mia_storage_resolve_virtual(context, requirement->path, &path);
        struct stat info;
        if (status.code != MIA_STORAGE_OK || stat(path.path, &info) != 0 || S_ISLNK(info.st_mode)) {
            return mia_storage_error(MIA_STORAGE_ERR_MISSING_REQUIRED_FILE, "required BIOS is missing");
        }
    }
    return mia_storage_ok();
}
