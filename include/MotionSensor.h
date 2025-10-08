#pragma once
#include <Arduino.h>

class MotionSensor {
public:
  bool begin(int sda, int scl);
  bool wasMoved(float threshold_mg = 100.0f);
  void readAccel(float &x_g, float &y_g, float &z_g);
};