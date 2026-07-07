#pragma once

#include <Arduino.h>
#include <esp_err.h>
#include <esp_partition.h>

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
};

struct OtaAppExportResult {
  OtaAppExportStatus status;
  int errorCode;
  size_t bytesWritten;
};

const esp_partition_t *miaFindOtaPartition();
esp_err_t miaForceOtaBoot(const esp_partition_t *target);
OtaAppFlashResult miaFlashAppToSlot(const char *sdPath, bool sdReady);
const char *miaOtaAppFlashStatusText(OtaAppFlashStatus status);
OtaAppExportResult miaExportAppSlotToSd(const char *sdPath, bool sdReady);
const char *miaOtaAppExportStatusText(OtaAppExportStatus status);
