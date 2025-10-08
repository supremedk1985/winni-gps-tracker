#include "BatteryMonitor.h"
#include <Wire.h>
#include <Adafruit_MAX1704X.h>

static Adafruit_MAX17048 g_max17048;
static bool batteryReady = false;

bool BatteryMonitor::begin(int sda, int scl){
  Wire.begin(sda, scl);
  if(!g_max17048.begin()){
    Serial.println("⚠️ MAX17048 nicht gefunden.");
    batteryReady = false;
    return false;
  }
  batteryReady = true;
  Serial.println("✅ MAX17048 bereit.");
  return true;
}

float BatteryMonitor::voltage(){
  if(!batteryReady) return NAN;
  return g_max17048.cellVoltage();
}

float BatteryMonitor::percent(){
  if(!batteryReady) return NAN;
  return g_max17048.cellPercent();
}

bool BatteryMonitor::quickStart(){
  if(!batteryReady) return false;
  g_max17048.quickStart();
  return true;
}