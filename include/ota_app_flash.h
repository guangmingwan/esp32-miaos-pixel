#pragma once

#include <Arduino.h>
#include <esp_err.h>
#include <esp_partition.h>

#include "ota_app_manifest.h"

enum class OtaAppFlashStatus : uint8_t {
  Ok,
  SdUnavailable,
  InvalidPath,
  OpenFailed,
  EmptyFile,
  NoPartition,
  TooLarge,
  EraseFailed,
  ReadFailed,
  WriteFailed,
  AllocFailed,
  OtaDataFailed,
};

struct OtaAppFlashResult {
  OtaAppFlashStatus status;
  int errorCode;
  size_t bytesWritten;
};

enum class OtaAppExportStatus : uint8_t {
  Ok,
  SdUnavailable,
  InvalidPath,
  NoPartition,
  InvalidImage,
  OpenFailed,
  ReadFailed,
  WriteFailed,
  ManifestMissing,
  MkdirFailed,
  VerifyFailed,
};

struct OtaAppExportResult {
  OtaAppExportStatus status;
  int errorCode;
  size_t bytesWritten;
};

enum class OtaAppSyncStatus : uint8_t {
  UpToDate,
  Updated,
  SdUnavailable,
  OtaManifestMissing,
  OtaManifestLegacy,
  Failed,
};

using OtaAppSyncProgress = void (*)(const char *stage, const char *appName,
                                    size_t completed, size_t total, void *context);

struct OtaAppSyncResult {
  OtaAppSyncStatus status;
  OtaAppExportStatus exportStatus;
  size_t bytesWritten;
};

const esp_partition_t *miaFindOtaPartition();
const esp_partition_t *miaFindAppSlot();
esp_err_t miaForceOtaBoot(const esp_partition_t *target);
OtaAppFlashResult miaFlashAppToSlot(const char *sdPath, bool sdReady);
const char *miaOtaAppFlashStatusText(OtaAppFlashStatus status);
OtaAppExportResult miaExportAppSlotToSd(const char *sdPath, bool sdReady);
const char *miaOtaAppExportStatusText(OtaAppExportStatus status);

/* Manifest helpers */
OtaAppExportResult miaExportOtaToSd(bool sdReady);
void miaBootAppSlot();
bool miaReadOtaManifest(OtaAppManifest *manifest);
bool miaReadManifestFromFile(const char *sdPath, OtaAppManifest *manifest);
OtaAppSyncResult miaSyncNewerOtaToSd(bool sdReady, OtaAppSyncProgress progress,
                                     void *context);
