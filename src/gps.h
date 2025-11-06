#ifndef GPS_H
#define GPS_H

#include <Arduino.h>
#include <TinyGPSPlus.h>

extern HardwareSerial gpsSerial;
extern TinyGPSPlus gps;
extern bool gpsBusy;

void initGPS();
void updateGPS();
String getGPSLocation();

// GPS Track Recording (alle 5 Meter)
void startGPSTracking();
void stopGPSTracking();
void processGPSTracking();
bool isGPSTracking();

#endif
