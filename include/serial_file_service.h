#pragma once

#include <Arduino.h>

struct SerialFileServiceSnapshot {
  bool sdReady;
  bool uploadActive;
  bool downloadActive;
  uint32_t transferredBytes;
  uint32_t totalBytes;
  char status[48];
  char targetPath[96];
};

void serialFileServiceBegin(bool sdReady);
void serialFileServiceTick();
void serialFileServiceEnd();
SerialFileServiceSnapshot serialFileServiceSnapshot();

void serialFileServicePollLauncherControl();
bool serialFileServiceTakeEnterRequest();
bool serialFileServiceTakeExitRequest();
