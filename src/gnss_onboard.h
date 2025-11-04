#ifndef GNSS_ONBOARD_H
#define GNSS_ONBOARD_H

#include <Arduino.h>

// GNSS-Funktionen
bool parseClbs(const String &clbs, String &lat, String &lon);
bool parseCgnssInfo(const String &resp, double &latDeg, double &lonDeg);
String getGPS();

#endif
