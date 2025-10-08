#include "MotionSensor.h"
#include <Wire.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

static Adafruit_LIS3DH g_lis = Adafruit_LIS3DH();

bool MotionSensor::begin(int sda, int scl){
  Wire.begin(sda, scl);
  if(!g_lis.begin(0x18)){
    Serial.println("⚠️ LIS3DH nicht gefunden.");
    return false;
  }
  g_lis.setRange(LIS3DH_RANGE_2_G);
  g_lis.setDataRate(LIS3DH_DATARATE_50_HZ);
  Serial.println("✅ LIS3DH bereit.");
  return true;
}

bool MotionSensor::wasMoved(float threshold_mg){
  sensors_event_t event;
  g_lis.getEvent(&event);
  float mag = sqrtf(event.acceleration.x*event.acceleration.x +
                    event.acceleration.y*event.acceleration.y +
                    event.acceleration.z*event.acceleration.z);
  float delta = fabsf(mag - 9.81f);
  return (delta*1000.0f/9.81f) > threshold_mg;
}

void MotionSensor::readAccel(float &x_g, float &y_g, float &z_g){
  sensors_event_t e;
  g_lis.getEvent(&e);
  x_g = e.acceleration.x / 9.81f;
  y_g = e.acceleration.y / 9.81f;
  z_g = e.acceleration.z / 9.81f;
}