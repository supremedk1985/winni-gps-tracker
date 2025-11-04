#ifndef GNSS_ONBOARD_H
#define GNSS_ONBOARD_H

#include <Arduino.h>

// ------------------------------------------------------
// GNSS Standort- und Parserfunktionen
// ------------------------------------------------------

// --- LTE-basierte Positionsabfrage (CLBS) ---
bool parseClbs(const String &clbs, String &lat, String &lon);

// --- Struktur für GNSS-Informationen (aus +CGNSSINFO) ---
struct GNSSInfo {
  double latDeg;       // Breitengrad in Dezimalgrad
  double lonDeg;       // Längengrad in Dezimalgrad
  double altitude;     // Höhe in Metern
  double speed;        // Geschwindigkeit in km/h
  double course;       // Kursrichtung in Grad
  double hdop;         // Horizontal Dilution of Precision
  int fixMode;         // 0 = kein Fix, 1 = 2D, 2 = 3D
  int satsInView;      // sichtbare Satelliten
  int satsUsed;        // verwendete Satelliten
  String utcDate;      // UTC-Zeit/Datum im Format YYYYMMDDhhmmss
  bool valid;          // true, wenn gültige Koordinaten erkannt
};

// --- Parser für +CGNSSINFO Antwort ---
bool parseCgnssInfo(const String &resp, GNSSInfo &info);

// --- Haupt-GNSS-Funktion, liefert Text für Telegram ---
String getGPS();

#endif
