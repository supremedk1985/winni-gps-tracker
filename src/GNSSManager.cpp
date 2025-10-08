#include "GNSSManager.h"
#include "Utils.h"

GNSSManager::GNSSManager(HardwareSerial &serial) : SerialAT(serial) {}

bool GNSSManager::waitForReady(unsigned long timeoutMs) {
  unsigned long start = millis();
  String buf;
  while (millis() - start < timeoutMs) {
    while (SerialAT.available()) {
      buf += (char)SerialAT.read();
      if (buf.indexOf("+CGNSSPWR: READY") >= 0) {
        Serial.println("✅ GNSS bereit!");
        return true;
      }
    }
    delay(100);
  }
  Serial.println("❌ Timeout: +CGNSSPWR: READY! nicht empfangen");
  return false;
}

bool GNSSManager::enableGNSS() {
  Serial.printf("🔹 GNSS einschalten\n");
  SerialAT.println("AT+CGNSSPWR=1");
  return waitForReady(20000);
}

void GNSSManager::disableGNSS() {
  SerialAT.println("AT+CGNSSPWR=0");
  delay(1000);
}

bool GNSSManager::parseCGNSSInfo(String resp, String &lat, String &lon) {
  int colon = resp.indexOf(':');
  if (colon < 0) return false;
  String data = resp.substring(colon + 1);
  data.trim();

  String fields[24];
  int idx = 0, last = 0;
  for (int i = 0; i < data.length() && idx < 23; i++) {
    if (data[i] == ',') {
      fields[idx++] = data.substring(last, i);
      last = i + 1;
    }
  }
  fields[idx] = data.substring(last);

  if (idx < 8) {
    Serial.println("⚠️ Zu wenige Felder in +CGNSSINFO.");
    return false;
  }

  String latStr = fields[5]; latStr.trim();
  String latHem = fields[6]; latHem.trim();
  String lonStr = fields[7]; lonStr.trim();
  String lonHem = fields[8]; lonHem.trim();

  if (!isFiniteCoord(latStr) || !isFiniteCoord(lonStr)) {
    Serial.println("⚠️ Lat/Lon nicht numerisch.");
    return false;
  }

  double latVal = latStr.toDouble();
  double lonVal = lonStr.toDouble();
  if (latHem == "S") latVal = -latVal;
  if (lonHem == "W") lonVal = -lonVal;

  if (latVal < -90.0 || latVal > 90.0 || lonVal < -180.0 || lonVal > 180.0) {
    Serial.println("⚠️ Lat/Lon außerhalb gültiger Bereiche.");
    return false;
  }

  lat = String(latVal, 7);
  lon = String(lonVal, 7);
  Serial.printf("📍 Geparst (valid): Lat=%s, Lon=%s\n", lat.c_str(), lon.c_str());
  return true;
}

bool GNSSManager::getFix(String &lat, String &lon) {
  Serial.println("Warte auf GNSS-Fix...");
  unsigned long start = millis();
  while (millis() - start < 200000) {
    SerialAT.println("AT+CGNSSINFO");
    delay(1000);

    if (SerialAT.available()) {
      String resp = SerialAT.readString();
      Serial.println(resp);
      if (resp.indexOf("+CGNSSINFO:") >= 0) {
        if (parseCGNSSInfo(resp, lat, lon)) {
          Serial.println("✅ Valider GNSS-Fix gefunden!");
          return true;
        } else {
          Serial.println("ℹ️ GNSSINFO empfangen, aber (noch) nicht valide.");
        }
      }
    } else {
      SerialAT.println("AT+CGNSSTST=1");
      delay(1000);
    }
  }
  return false;
}