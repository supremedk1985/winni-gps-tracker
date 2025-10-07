#include <Arduino.h>
#include <TinyGsmClient.h>

#define TINY_GSM_MODEM_SIM7600  // A7670E-kompatibel

HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);

String latitude = "";
String longitude = "";

// 🔧 o2 Zugangsdaten (ohne PIN)
const char apn[] = "internet";
const char user[] = "";
const char pass[] = "";

// Zielserver für Test
const char* host = "httpbin.org";
const int   port = 80;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== A7670E GNSS → LTE Upload Demo ===");

  pinMode(33, OUTPUT);
  digitalWrite(33, HIGH);
  delay(2000);

  SerialAT.begin(115200, SERIAL_8N1, 17, 18);
  delay(3000);
  modem.restart();

  // 1️⃣ GNSS aktivieren
  Serial.println("Starte GNSS...");
  SerialAT.println("AT+CGNSSPWR=1");
  delay(2000);

  // 2️⃣ Warten auf GNSS-Fix
  bool gotFix = false;
  unsigned long start = millis();
  while (millis() - start < 20000) { // 20s Timeout
    SerialAT.println("AT+CGNSSINFO");
    delay(1000);
    if (SerialAT.available()) {
      String resp = SerialAT.readString();
      if (resp.indexOf("+CGNSSINFO:") >= 0 && resp.indexOf(",N,") > 0) {
        Serial.println("✅ GNSS-Fix gefunden!");
        Serial.println(resp);
        // Einfaches Parsen: Breite, Länge extrahieren
        int firstComma = resp.indexOf(":") + 1;
        int secondComma = resp.indexOf(",", firstComma);
        latitude = resp.substring(firstComma, secondComma);
        int nextComma = resp.indexOf(",", secondComma + 1);
        longitude = resp.substring(secondComma + 1, nextComma);
        gotFix = true;
        break;
      }
    }
  }

  if (!gotFix) {
    Serial.println("❌ Kein GNSS-Fix (Timeout)");
    latitude = "0.0";
    longitude = "0.0";
  }

  // GNSS aus, um Konflikt zu vermeiden
  SerialAT.println("AT+CGNSSPWR=0");
  delay(1000);

  // 3️⃣ LTE-Verbindung herstellen
  Serial.println("Verbinde LTE...");
  if (!modem.waitForNetwork(30000)) {
    Serial.println("❌ Kein Netz");
    return;
  }
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println("❌ LTE-Verbindung fehlgeschlagen");
    return;
  }
  Serial.println("✅ LTE verbunden!");
  Serial.print("IP: "); Serial.println(modem.localIP());

  // 4️⃣ HTTP-Test – Standort senden
  TinyGsmClient client(modem);
  if (client.connect(host, port)) {
    Serial.println("Sende Standort...");
    String url = "/get?lat=" + latitude + "&lon=" + longitude;
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
  } else {
    Serial.println("❌ HTTP-Verbindung fehlgeschlagen");
  }
}

void loop() {
  // nichts
}
