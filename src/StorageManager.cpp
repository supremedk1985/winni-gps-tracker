#include "StorageManager.h"
#include "config.h"
#include <FS.h>
#if USE_SD_MMC
#include <SD_MMC.h>
#else
#include <SD.h>
#include <SPI.h>
#endif

bool StorageManager::begin(){
#if USE_SD_MMC
  if(!SD_MMC.begin()){
    Serial.println("⚠️ SD_MMC init fehlgeschlagen.");
    return false;
  }
  Serial.println("✅ TF/SD_MMC bereit.");
  return true;
#else
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI);
  if(!SD.begin(SD_CS)){
    Serial.println("⚠️ SD (SPI) init fehlgeschlagen.");
    return false;
  }
  Serial.println("✅ TF/SD (SPI) bereit.");
  return true;
#endif
}

bool StorageManager::writeText(const String &path, const String &content){
#if USE_SD_MMC
  fs::FS &fs = SD_MMC;
#else
  fs::FS &fs = SD;
#endif
  File f = fs.open(path.c_str(), FILE_WRITE);
  if(!f) return false;
  f.print(content);
  f.close();
  return true;
}

bool StorageManager::exists(const String &path){
#if USE_SD_MMC
  return SD_MMC.exists(path);
#else
  return SD.exists(path);
#endif
}