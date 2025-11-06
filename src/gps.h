#ifndef GPS_H
#define GPS_H

#include <Arduino.h>
#include <TinyGPS++.h>
#include <FS.h>
#include <SD_MMC.h>

extern HardwareSerial gpsSerial;
extern TinyGPSPlus gps;

// Basis-GPS-Funktionen
void initGPS();
void updateGPS();
String getGPSLocation();

// GPS-Logging-Funktionen
void logGPSData();

// Track-Export-Funktionen
bool exportTrackToGPX(int days, const char* outputFile);

#endif
