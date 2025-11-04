#include "storage.h"

const int SDMMC_CLK  = 5;
const int SDMMC_CMD  = 4;
const int SDMMC_DATA = 6;
const int SD_CD_PIN  = 46;

static bool storageReady = false;
static uint64_t cachedCardSize = 0;

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

    if (!SD_MMC.begin("/sdcard", true)) {  // exakt wie dein Script: true = read-only
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
