#pragma once
#include <Arduino.h>
#include "TinyGsmClient.h"
class GNSSManager {
public:
  GNSSManager(HardwareSerial &serial);
  bool enableGNSS();
  bool waitForReady(unsigned long timeoutMs);
  bool getFix(String &latitude, String &longitude);
  void disableGNSS();
private:
  bool parseCGNSSInfo(String resp, String &lat, String &lon);
  HardwareSerial &SerialAT;
};