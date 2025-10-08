#pragma once
#include <Arduino.h>

class StorageManager {
public:
  bool begin();
  bool writeText(const String &path, const String &content);
  bool exists(const String &path);
};