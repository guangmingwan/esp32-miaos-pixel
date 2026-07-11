#include "mia_storage_internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static MiaStorageStatus flush_file(FILE *file) {
    if (fflush(file) != 0) {
        return mia_storage_error(MIA_STORAGE_ERR_IO, "failed to flush save file");
    }
    if (fsync(fileno(file)) != 0) {
        return mia_storage_error(MIA_STORAGE_ERR_IO, "failed to sync save file");
    }
    return mia_storage_ok();
}

static MiaStorageStatus write_temp(const char *path, const uint8_t *data, size_t size) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return mia_storage_error(MIA_STORAGE_ERR_IO, "failed to open temp save");
    }
    if (size > 0 && fwrite(data, size, 1, file) != 1) {
        fclose(file);
        return mia_storage_error(MIA_STORAGE_ERR_IO, "failed to write temp save");
    }
    MiaStorageStatus status = flush_file(file);
    if (fclose(file) != 0 && status.code == MIA_STORAGE_OK) {
        status = mia_storage_error(MIA_STORAGE_ERR_IO, "failed to close temp save");
    }
    return status;
}

static bool injects_before_replace(const MiaStorageSaveRequest *request) {
    return request->fault != NULL && request->fault->kind == MIA_STORAGE_FAULT_BEFORE_REPLACE;
}

static bool injects_full_write(const MiaStorageSaveRequest *request) {
    return request->fault != NULL && request->fault->kind == MIA_STORAGE_FAULT_WRITE_FULL;
}

static bool injects_rename_no_replace(const MiaStorageSaveRequest *request) {
    return request->fault != NULL && request->fault->kind == MIA_STORAGE_FAULT_RENAME_NO_REPLACE;
}

static MiaStorageStatus replace_temp(const MiaStorageSaveRequest *request, const char *temp, const char *target) {
    if (!injects_rename_no_replace(request) && rename(temp, target) == 0) {
        return mia_storage_ok();
    }

    char backup[MIA_STORAGE_PATH_MAX];
    const int written = snprintf(backup, sizeof(backup), "%s.bak", target);
    if (written < 0 || (size_t)written >= sizeof(backup)) {
        remove(temp);
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "backup save path is too long");
    }
    remove(backup);
    const bool had_target = rename(target, backup) == 0;
    if (!had_target && errno != ENOENT) {
        remove(temp);
        return mia_storage_error(MIA_STORAGE_ERR_IO, "failed to back up save");
    }
    if (rename(temp, target) != 0) {
        if (had_target) (void)rename(backup, target);
        remove(temp);
        return mia_storage_error(MIA_STORAGE_ERR_IO, "failed to replace save atomically");
    }
    if (had_target) remove(backup);
    return mia_storage_ok();
}

static bool valid_flush_reason(MiaStorageFlushReason reason) {
    switch (reason) {
    case MIA_STORAGE_FLUSH_CORE_REQUEST:
    case MIA_STORAGE_FLUSH_ROM_CHANGE:
    case MIA_STORAGE_FLUSH_CLEAN_EXIT:
        return true;
    }
    return false;
}

MiaStorageStatus mia_storage_save_write(const MiaStorageContext *context, const MiaStorageSaveRequest *request, const uint8_t *data, size_t size) {
    if (request == NULL || request->target == NULL || request->save_name == NULL || (data == NULL && size > 0)) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "save request and data are required");
    }
    if (!valid_flush_reason(request->reason)) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "valid flush reason is required");
    }
    MiaStoragePath target;
    MiaStorageStatus status = mia_storage_prepare_child(context, request->target->save_root, request->save_name, &target);
    if (status.code != MIA_STORAGE_OK) {
        return status;
    }
    char temp[MIA_STORAGE_PATH_MAX];
    const int written = snprintf(temp, sizeof(temp), "%s.tmp", target.path);
    if (written < 0 || (size_t)written >= sizeof(temp)) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "temp save path is too long");
    }
    remove(temp);
    if (injects_full_write(request)) {
        return mia_storage_error(MIA_STORAGE_ERR_IO, "injected full storage write failure");
    }
    status = write_temp(temp, data, size);
    if (status.code != MIA_STORAGE_OK) {
        remove(temp);
        return status;
    }
    if (injects_before_replace(request)) {
        remove(temp);
        return mia_storage_error(MIA_STORAGE_ERR_INTERRUPTED, "injected interruption before atomic replace");
    }
    return replace_temp(request, temp, target.path);
}

MiaStorageStatus mia_storage_save_read(const MiaStorageContext *context, const MiaStorageTarget *target, const char *save_name, uint8_t *data, size_t capacity, size_t *out_size) {
    if (target == NULL || save_name == NULL || data == NULL || out_size == NULL) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "save read arguments are required");
    }
    *out_size = 0;
    MiaStoragePath path;
    MiaStorageStatus status = mia_storage_resolve_child(context, target->save_root, save_name, &path);
    if (status.code != MIA_STORAGE_OK) {
        return status;
    }
    struct stat info;
#ifndef ESP_PLATFORM
    char link_target[2];
    if (readlink(path.path, link_target, sizeof(link_target)) >= 0) {
        return mia_storage_error(MIA_STORAGE_ERR_PATH_TRAVERSAL, "save path must not be a symlink");
    }
#endif
    if (stat(path.path, &info) != 0) {
        return errno == ENOENT ? mia_storage_error(MIA_STORAGE_ERR_MISSING_REQUIRED_FILE, "save file is missing") : mia_storage_error(MIA_STORAGE_ERR_IO, "failed to inspect save file");
    }
    if (!S_ISREG(info.st_mode) || (uint64_t)info.st_size > capacity) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "save file is not a bounded regular file");
    }
    FILE *file = fopen(path.path, "rb");
    if (file == NULL) {
        return mia_storage_error(MIA_STORAGE_ERR_IO, "failed to open save file");
    }
    const size_t expected = (size_t)info.st_size;
    const size_t read_size = expected == 0 ? 0 : fread(data, 1, expected, file);
    const int close_result = fclose(file);
    if (read_size != expected || close_result != 0) {
        return mia_storage_error(MIA_STORAGE_ERR_IO, "failed to read save file");
    }
    *out_size = read_size;
    return mia_storage_ok();
}
