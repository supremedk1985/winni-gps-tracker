#include <Arduino.h>
#include <TinyGsmClient.h>

#define TINY_GSM_MODEM_SIM7600 // A7670E-kompatibel

HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);

const char APN[] = "internet"; // o2
const char USER[] = "";
const char PASS[] = "";

const char SERVER[] = "supremedk1.synology.me";
const int PORT = 1880;
const char PATH[] = "/gps";
const char TOKEN[] = "jlkdsfjhksdkf230s3490";

String latitude, longitude, imei;

bool waitForReadyResponse(unsigned long timeoutMs)
{
  unsigned long start = millis();
  String buffer;
  while (millis() - start < timeoutMs)
  {
    while (SerialAT.available())
    {
      char c = SerialAT.read();
      buffer += c;
      if (buffer.indexOf("+CGNSSPWR: READY") >= 0)
      {
        Serial.println("✅ GNSS bereit!");
        return true;
      }
    }
    delay(100);
  }
  Serial.println("❌ Timeout: +CGNSSPWR: READY! nicht empfangen");
  return false;
}

void restartModem()
{
  Serial.println("🔄 Modem wird neu gestartet...");
  modem.restart();
  delay(5000);
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== A7670E GNSS → LTE → Node-RED POST ===");

  pinMode(33, OUTPUT);
  digitalWrite(33, HIGH);
  delay(2000);

  SerialAT.begin(115200, SERIAL_8N1, 17, 18);
  delay(3000);

  modem.restart();

  // GNSS einschalten
  for (int attempt = 1; attempt <= 2; attempt++)
  {
    Serial.printf("🔹 GNSS einschalten (Versuch %d)\n", attempt);
    SerialAT.println("AT+CGNSSPWR=1");
    if (waitForReadyResponse(20000))
    {
      break; // Erfolgreich
    }
    else if (attempt == 1)
    {
      restartModem();
    }
    else
    {
      Serial.println("❌ GNSS konnte nicht aktiviert werden.");
    }
  }

  // GNSSS Ausgabe starten
  SerialAT.println("AT+CGNSSTST=1");
  delay(500);

  // IMEI abrufen
  SerialAT.println("AT+GSN");
  delay(500);
  if (SerialAT.available())
    imei = SerialAT.readStringUntil('\n');
  imei.trim();

  // GNSS-Fix abfragen
  Serial.println("Warte auf GNSS-Fix...");
  unsigned long start = millis();
  bool gotFix = false;
  while (millis() - start < 200000)
  { // 200 Sekunden
    SerialAT.println("AT+CGNSSINFO");
    delay(1000);
    if (SerialAT.available())
    {
      String resp = SerialAT.readString();
      Serial.println(resp);
      if (resp.indexOf("+CGNSSINFO:") >= 0 && resp.indexOf(",N,") > 0)
      {
        Serial.println("✅ GNSS-Fix gefunden!");
        int a = resp.indexOf(",") + 1;    // Skip first field
        int b = resp.indexOf(",", a + 1); // Längenfeld
        latitude = resp.substring(a, b);
        int c = resp.indexOf(",", b + 1);
        longitude = resp.substring(b + 1, c);
        gotFix = true;
        break;
      }
    }
  }

  if (!gotFix)
  {
    Serial.println("❌ Kein Fix, verwende 0/0");
    latitude = "0.0";
    longitude = "0.0";
  }

  // GNSS ausschalten
  SerialAT.println("AT+CGNSSPWR=0");
  delay(1000);

  // LTE verbinden
  Serial.println("Verbinde LTE...");
  if (!modem.waitForNetwork(30000) || !modem.gprsConnect(APN, USER, PASS))
  {
    Serial.println("❌ LTE fehlgeschlagen");
    return;
  }
  Serial.println("✅ LTE verbunden!");

  // JSON vorbereiten
  String json = String("{\"lat\":") + latitude + ",\"lon\":" + longitude +
                ",\"imei\":\"" + imei + "\"}";

  // HTTP-POST senden
  Serial.println("Sende POST an Node-RED...");
  if (client.connect(SERVER, PORT))
  {
    client.print(String("POST ") + PATH + " HTTP/1.1\r\n");
    client.print(String("Host: ") + SERVER + "\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print(String("X-Token: ") + TOKEN + "\r\n");
    client.print(String("Content-Length: ") + json.length() + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.print(json);
    Serial.println("✅ Gesendet:");
    Serial.println(json);
  }
  else
  {
    Serial.println("❌ Verbindung zu Server fehlgeschlagen");
  }

  // Antwort anzeigen
  delay(1000);
  while (client.available())
    Serial.write(client.read());
  client.stop();
}

void loop() {}
