#include "ModemManager.h"
#include <Arduino.h>

ModemManager::ModemManager(HardwareSerial &serial)
  : SerialAT(serial), modem(serial), client(modem) {}

void ModemManager::restart() {
  Serial.println("🔄 Modem wird neu gestartet...");
  modem.restart();
  delay(5000);
}

bool ModemManager::connectLTE(const char* apn, const char* user, const char* pass) {
  Serial.println("🌐 Verbinde LTE...");
  if (!modem.waitForNetwork(30000) || !modem.gprsConnect(apn, user, pass)) {
    Serial.println("❌ LTE fehlgeschlagen");
    return false;
  }
  Serial.println("✅ LTE verbunden!");
  return true;
}

String ModemManager::getIMEI() {
  SerialAT.println("AT+GSN");
  delay(500);
  String imei = "";
  if (SerialAT.available()) imei = SerialAT.readStringUntil('\n');
  imei.trim();
  return imei;
}