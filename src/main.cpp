#include <Arduino.h>
#include <TinyGPSPlus.h>

HardwareSerial SerialAT(1); 
TinyGPSPlus gps;

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

  while (SerialAT.available()) SerialAT.read();

  Serial.println(">> GNSS aktiv – warte auf Daten...\n");
}

void loop()
{
  // Datenstrom in TinyGPS++ einspeisen
  while (SerialAT.available())
  {
    gps.encode(SerialAT.read());
  }

  // Wenn neue Positionsdaten vorliegen
  if (gps.location.isUpdated())
  {
    Serial.println(F("=== GNSS-Daten ==="));
    Serial.print(F("Breite (Lat): ")); Serial.println(gps.location.lat(), 6);
    Serial.print(F("Länge  (Lon): ")); Serial.println(gps.location.lng(), 6);
    Serial.print(F("Höhe   (m):   ")); Serial.println(gps.altitude.meters(), 2);
    Serial.print(F("Satelliten:   ")); Serial.println(gps.satellites.value());
    Serial.print(F("Genauigkeit:  ")); Serial.println(gps.hdop.hdop(), 2);
    Serial.print(F("Zeit (UTC):   "));
    Serial.print(gps.time.hour()); Serial.print(":");
    Serial.print(gps.time.minute()); Serial.print(":");
    Serial.println(gps.time.second());
    Serial.println();
  }
}
