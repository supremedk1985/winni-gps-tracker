#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>

// Initialisiert SD-Karte, gibt true bei Erfolg zurück
bool initStorage();

// Liefert Größe der SD-Karte in MB (oder 0 bei Fehler)
uint64_t getCardSizeMB();

// Listet Verzeichnisinhalt (rekursiv falls levels > 0)
void listDir(fs::FS &fs, const char *dirname, uint8_t levels = 0);

#endif
