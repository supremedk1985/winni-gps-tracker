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

// Warten auf "+CGNSSPWR: READY!"
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

// Modem neustarten
void restartModem()
{
  Serial.println("🔄 Modem wird neu gestartet...");
  modem.restart();
  delay(5000);
}

// GNSSINFO-Zeile auswerten
bool parseCGNSSInfo(String resp)
{
  // Beispiel: +CGNSSINFO: 3,14,,11,00,51.8838463,N,8.2499580,E,071025,...
  int firstComma = resp.indexOf(":");
  if (firstComma == -1)
    return false;

  // Alles nach dem ":" holen und splitten
  String data = resp.substring(firstComma + 1);
  data.trim();

  // Felder trennen
  int fieldIndex = 0;
  int lastPos = 0;
  String fields[20];

  for (int i = 0; i < data.length(); i++)
  {
    if (data[i] == ',')
    {
      fields[fieldIndex++] = data.substring(lastPos, i);
      lastPos = i + 1;
      if (fieldIndex >= 19)
        break;
    }
  }
  fields[fieldIndex] = data.substring(lastPos);

  // In diesem Format:
  // [0]=3 [1]=14 [2]= (leer) [3]=11 [4]=00 [5]=51.8838463 [6]=N [7]=8.2499580 [8]=E ...
  if (fields[5].length() > 0 && fields[7].length() > 0)
  {
    latitude = fields[5];
    if (fields[6] == "S")
      latitude = "-" + latitude;
    longitude = fields[7];
    if (fields[8] == "W")
      longitude = "-" + longitude;
    Serial.printf("📍 Geparst: Lat=%s, Lon=%s\n", latitude.c_str(), longitude.c_str());
    return true;
  }

  Serial.println("⚠️ Konnte Koordinaten nicht auswerten.");
  return false;
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

  // GNSS-Ausgabe starten
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

      if (resp.indexOf("+CGNSSINFO:") >= 0)
      {
        if (parseCGNSSInfo(resp))
        {
          Serial.println("✅ GNSS-Fix gefunden!");
          gotFix = true;
          break;
        }
      }
    }
    else
    {
      // GNSS-Ausgabe starten
      SerialAT.println("AT+CGNSSTST=1");
      delay(1000);
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
