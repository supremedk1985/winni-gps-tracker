#include <Arduino.h>
#include "config.h"
#include "Utils.h"
#include "GNSSManager.h"
#include "ModemManager.h"
#include "HttpClient.h"

HardwareSerial SerialAT(1);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== A7670E GNSS → LTE → Node-RED POST (Minimal aktiv) ===");

  pinMode(MODEM_PWR_PIN, OUTPUT);
  digitalWrite(MODEM_PWR_PIN, HIGH);
  delay(2000);

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  ModemManager modemMgr(SerialAT);
  GNSSManager  gnss(SerialAT);
  HttpClient   http(modemMgr.client);

  modemMgr.restart();

  if (!gnss.enableGNSS()) handleNoChanceError();

  String lat, lon;
  if (!gnss.getFix(lat, lon)) handleNoChanceError();
  gnss.disableGNSS();

  String imei = modemMgr.getIMEI();
  if (!modemMgr.connectLTE(APN, USER, PASS)) handleNoChanceError();

  String json = String("{\"lat\":") + lat + ",\"lon\":" + lon + ",\"imei\":\"" + imei + "\"}";

  if (!http.postJSON(SERVER, PORT, PATH, TOKEN, json))
    handleNoChanceError();

  Serial.println("✅ Ablauf beendet. (loop() bleibt leer)");
}

void loop() {
  // absichtlich leer; spätere Erweiterungen kommen hier rein
}