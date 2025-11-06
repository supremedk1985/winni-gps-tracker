#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>

// SD-Karteninitialisierung
bool initStorage();

// Gibt Kartengröße in MB zurück (0 bei Fehler)
uint64_t getCardSizeMB();

// Optional: Verzeichnisinhalt ausgeben
void listDir(fs::FS &fs, const char *dirname, uint8_t levels = 0);

// GPS-Track-Aufzeichnung
bool createGPSTrackFile();
bool appendGPSData(double lat, double lon, const String& timestamp, double speed_kmh, double altitude_m);
String getCurrentTrackFilename();
bool isTrackRecording();

#endif
