#pragma once

#include <Arduino.h>

String htmlEscape(const String &value);
bool normalizeSdPath(const String &rawPath, String &path);
String parentSdPath(const String &path);
String leafSdName(const String &path);
