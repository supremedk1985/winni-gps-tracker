#pragma once
#include <Arduino.h>

class WebCamManager {
public:
  bool begin();
  bool captureToFile(const String &path);
  bool isReady() const { return ready; }
private:
  bool ready = false;
};