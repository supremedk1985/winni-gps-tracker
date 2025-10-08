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


void handleNoChanceError(){
  Serial.println("❌❌❌ DAS HAT NICHT GEKLAPPT! DEN SELBEN SONG NOCHMAL! ❌❌❌");
  ESP.restart();
}

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

// einfache Plausibilitätsprüfung
static bool isFiniteCoord(const String &s)
{
  if (s.length() == 0)
    return false;
  bool dotSeen = false, digitSeen = false;
  int start = (s[0] == '-' || s[0] == '+') ? 1 : 0;
  for (int i = start; i < s.length(); i++)
  {
    char c = s[i];
    if (c == '.')
    {
      if (dotSeen)
        return false;
      dotSeen = true;
    }
    else if (c < '0' || c > '9')
      return false;
    else
      digitSeen = true;
  }
  return digitSeen;
}

// GNSSINFO-Zeile auswerten (liefert true nur bei VALIDER Lat/Lon)
bool parseCGNSSInfo(String resp)
{
  // Beispiel: +CGNSSINFO: 3,14,,11,00,51.8838463,N,8.2499580,E,071025,...
  int colon = resp.indexOf(':');
  if (colon < 0)
    return false;

  String data = resp.substring(colon + 1);
  data.trim();

  String fields[24];
  int idx = 0, last = 0;
  for (int i = 0; i < data.length() && idx < 23; i++)
  {
    if (data[i] == ',')
    {
      fields[idx++] = data.substring(last, i);
      last = i + 1;
    }
  }
  fields[idx] = data.substring(last);

  // Erwartete Positionen:
  // 5 = lat, 6 = N/S, 7 = lon, 8 = E/W
  if (idx < 8)
  {
    Serial.println("⚠️ Zu wenige Felder in +CGNSSINFO.");
    return false;
  }

  String lat = fields[5];
  lat.trim();
  String latHem = fields[6];
  latHem.trim();
  String lon = fields[7];
  lon.trim();
  String lonHem = fields[8];
  lonHem.trim();

  if (!(latHem == "N" || latHem == "S") || !(lonHem == "E" || lonHem == "W"))
  {
    Serial.println("⚠️ Himmelsrichtungen fehlen/ungültig.");
    return false;
  }
  if (!isFiniteCoord(lat) || !isFiniteCoord(lon))
  {
    Serial.println("⚠️ Lat/Lon nicht numerisch.");
    return false;
  }

  // Zahlen prüfen & Wertebereiche
  double latVal = lat.toDouble();
  double lonVal = lon.toDouble();
  if (latHem == "S")
    latVal = -latVal;
  if (lonHem == "W")
    lonVal = -lonVal;

  if (latVal < -90.0 || latVal > 90.0 || lonVal < -180.0 || lonVal > 180.0)
  {
    Serial.println("⚠️ Lat/Lon außerhalb gültiger Bereiche.");
    return false;
  }

  latitude = String(latVal, 7);
  longitude = String(lonVal, 7);
  Serial.printf("📍 Geparst (valid): Lat=%s, Lon=%s\n", latitude.c_str(), longitude.c_str());
  return true;
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
  for (int attempt = 1; attempt <= 3; attempt++)
  {
    Serial.printf("🔹 GNSS einschalten (Versuch %d)\n", attempt);
    SerialAT.println("AT+CGNSSPWR=1");
    if (waitForReadyResponse(20000))
    {
      break; // Erfolgreich
    }
    else if (attempt <= 2)
    {
      restartModem();
    }
    else
    {
      Serial.println("❌ GNSS konnte nicht aktiviert werden. Starte neu");
      handleNoChanceError();
    }
  }

  // GNSS-Ausgabe starten (Testausgabe)
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
  bool gotValidFix = false;

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
          Serial.println("✅ Valider GNSS-Fix gefunden!");
          gotValidFix = true;
          break;
        }
        else
        {
          Serial.println("ℹ️ GNSSINFO empfangen, aber (noch) nicht valide.");
        }
      }
    }
    else
    {
      // Falls Modem die Testausgabe stoppt, erneut anschieben
      SerialAT.println("AT+CGNSSTST=1");
      delay(1000);
    }
  }

  // GNSS ausschalten (Konflikte vermeiden)
  SerialAT.println("AT+CGNSSPWR=0");
  delay(1000);

  if (!gotValidFix)
  {
    Serial.println("⛔ Kein valider Fix — es wird NICHT an Node-RED gesendet.");
    handleNoChanceError();
  }

  // LTE verbinden (nur wenn valider Fix)
  Serial.println("Verbinde LTE...");
  if (!modem.waitForNetwork(30000) || !modem.gprsConnect(APN, USER, PASS))
  {
    Serial.println("❌ LTE fehlgeschlagen — es wird NICHT gesendet.");
    handleNoChanceError();
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
    handleNoChanceError();
  }

  // Antwort anzeigen
  delay(1000);
  while (client.available())
    Serial.write(client.read());
  client.stop();
}

void loop() {}
