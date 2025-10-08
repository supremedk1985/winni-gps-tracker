#pragma once
#include <Arduino.h>

class BatteryMonitor {
public:
  bool begin(int sda, int scl);
  float voltage();
  float percent();
  bool quickStart();
};