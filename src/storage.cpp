#include "storage.h"

const int SDMMC_CLK  = 5;
const int SDMMC_CMD  = 4;
const int SDMMC_DATA = 6;
const int SD_CD_PIN  = 46;

static bool storageReady = false;
static uint64_t cachedCardSize = 0;
static String currentTrackFile = "";
static bool trackRecording = false;

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels) {
                listDir(fs, file.path(), levels - 1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

bool initStorage()
{
    pinMode(SD_CD_PIN, INPUT_PULLUP);
    delay(3000);

    SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA);

    // true = 1-Bit-Modus (langsamer, aber stabiler)
    // Die Karte ist trotzdem beschreibbar!
    if (!SD_MMC.begin("/sdcard", true)) {
        Serial.println("Card Mount Failed");
        return false;
    }

    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD_MMC card attached");
        return false;
    }

    Serial.print("SD_MMC Card Type: ");
    if (cardType == CARD_MMC) {
        Serial.println("MMC");
    } else if (cardType == CARD_SD) {
        Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    cachedCardSize = SD_MMC.cardSize() / (1024ULL * 1024ULL);
    Serial.printf("SD_MMC Card Size: %lluMB\n", cachedCardSize);

    listDir(SD_MMC, "/", 0);

    storageReady = true;
    return true;
}

uint64_t getCardSizeMB()
{
    if (!storageReady)
        initStorage();
    return cachedCardSize;
}

bool createGPSTrackFile()
{
    if (!storageReady) {
        Serial.println("Storage not ready");
        return false;
    }

    // Erstelle Dateinamen mit Zeitstempel
    char filename[50];
    unsigned long timestamp = millis();
    snprintf(filename, sizeof(filename), "/gps_track_%lu.csv", timestamp);
    
    currentTrackFile = String(filename);
    
    // Erstelle Datei mit CSV-Header
    File file = SD_MMC.open(currentTrackFile.c_str(), FILE_WRITE);
    if (!file) {
        Serial.println("Failed to create GPS track file");
        return false;
    }
    
    // CSV Header schreiben
    file.println("Latitude,Longitude,Timestamp,Speed_kmh,Altitude_m");
    file.close();
    
    trackRecording = true;
    Serial.printf("GPS Track file created: %s\n", currentTrackFile.c_str());
    
    return true;
}

bool appendGPSData(double lat, double lon, const String& timestamp, double speed_kmh, double altitude_m)
{
    if (!storageReady || !trackRecording) {
        return false;
    }
    
    File file = SD_MMC.open(currentTrackFile.c_str(), FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open GPS track file for writing");
        return false;
    }
    
    // CSV Zeile schreiben: Lat,Lon,Time,Speed,Altitude
    char line[200];
    snprintf(line, sizeof(line), "%.6f,%.6f,%s,%.2f,%.2f", 
             lat, lon, timestamp.c_str(), speed_kmh, altitude_m);
    
    file.println(line);
    file.close();
    
    Serial.printf("GPS data appended: %s\n", line);
    
    return true;
}

String getCurrentTrackFilename()
{
    return currentTrackFile;
}

bool isTrackRecording()
{
    return trackRecording;
}
