#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <TinyGsmClient.h>

#define TINY_GSM_MODEM_SIM7600 // für A76xx Familie

HardwareSerial SerialAT(1);
TinyGPSPlus gps;

TinyGsm modem(SerialAT);
TinyGsmClient net(modem);

void setup()
{
  Serial.begin(115200);

  SerialAT.begin(115200, SERIAL_8N1, 17, 18); // GNSS-Port

  pinMode(33, OUTPUT);
  digitalWrite(33, HIGH);
  delay(2000);

  Serial.println("=== A7670E GNSS Parser ===");

  // GNSS aktivieren
  SerialAT.println("AT");
  delay(200);
  SerialAT.println("AT+CGNSSPWR=1");
  delay(500);
  SerialAT.println("AT+CGNSSTST=1");
  delay(500);
  SerialAT.println("AT+CGNSSPORTSWITCH=0,1");
  delay(1000);

  modem.init();

  SerialAT.println("AT+CPIN?");
  delay(1000);
  SerialAT.println("AT+CSQ");
  delay(1000);
  SerialAT.println("AT+CREG?");
  delay(1000);

  Serial.print("Warte auf Netz...");
  if (!modem.waitForNetwork(60000L))
  { // bis zu 60s
    Serial.println(" FEHLER: kein Netz.");
    return;
  }

  // APN verbinden (normales LTE/PPP)
  Serial.print("Verbinde APN: ");
  if (!modem.gprsConnect("internet", "", ""))
  {
    Serial.println("FEHLER: APN-Verbindung fehlgeschlagen!");
    return;
  }
  Serial.println("GPRS/LTE verbunden.");

  Serial.println("Ping google.de (AT+SNPING4)...");
  modem.sendAT("+SNPING4=\"google.de\",1,16,5000");
  {
    String resp;
    modem.waitResponse(10000, resp);
    Serial.println(resp);
  }

  while (SerialAT.available())
    SerialAT.read();

  while (SerialAT.available())
  {
    char c = SerialAT.read();
    Serial.print(c);
  }

  Serial.println(">> GNSS aktiv – warte auf Daten...\n");
}

void loop()
{
  // Datenstrom in TinyGPS++ einspeisen
  while (SerialAT.available())
  {
    char c = SerialAT.read();
     Serial.print(c);
   // gps.encode(c);
  }
  // Serial.println();
  
  SerialAT.println("AT+CSQ");
  delay(1000);
  SerialAT.println("AT+CREG?");
  delay(1000);
  
  delay(1000);

  // Wenn neue Positionsdaten vorliegen
  if (gps.location.isUpdated())
  {
    Serial.println(F("=== GNSS-Daten ==="));
    Serial.print(F("Breite (Lat): "));
    Serial.println(gps.location.lat(), 6);
    Serial.print(F("Länge  (Lon): "));
    Serial.println(gps.location.lng(), 6);
    /*
     Serial.print(F("Höhe   (m):   ")); Serial.println(gps.altitude.meters(), 2);
     Serial.print(F("Satelliten:   ")); Serial.println(gps.satellites.value());
     Serial.print(F("Genauigkeit:  ")); Serial.println(gps.hdop.hdop(), 2);
     Serial.print(F("Zeit (UTC):   "));
     Serial.print(gps.time.hour()); Serial.print(":");
     Serial.print(gps.time.minute()); Serial.print(":");
     Serial.println(gps.time.second());
     Serial.println();*/
  }
}
