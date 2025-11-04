#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>

// SD-Karteninitialisierung (wie im Testscript)
bool initStorage();

// Gibt Kartengröße in MB zurück (0 bei Fehler)
uint64_t getCardSizeMB();

// Optional: Verzeichnisinhalt ausgeben
void listDir(fs::FS &fs, const char *dirname, uint8_t levels = 0);

#endif
