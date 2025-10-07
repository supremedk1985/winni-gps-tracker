#include <Arduino.h>
#include <TinyGsmClient.h>
#include <TinyGPSPlus.h>

#define TINY_GSM_MODEM_SIM7600 // A7670E kompatibel

HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);
TinyGPSPlus gps;

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("=== ESP32-S3 A7670E: LTE + GNSS Minimal ===");

  pinMode(33, OUTPUT);
  digitalWrite(33, HIGH);
  delay(2000);

  // Modem UART (Waveshare-Standard)
  SerialAT.begin(115200, SERIAL_8N1, 17, 18);
  delay(2000);

  modem.restart();
  delay(1000);

  // LTE verbinden (o2, ohne PIN)
  const char apn[] = "internet";
  Serial.println("Verbinde o2 LTE...");
  if (modem.waitForNetwork(30000) && modem.gprsConnect(apn))
  {
    Serial.println("✅ LTE verbunden!");
    Serial.print("IP: ");
    Serial.println(modem.localIP());
  }
  else
  {
    Serial.println("❌ LTE fehlgeschlagen");
  }

  modem.gprsDisconnect();
 delay(1000);
modem.restart();

  Serial.println("trenne verbindung wieder");
  delay(1000);

  // GNSS einschalten
  SerialAT.println("AT+CGNSSPWR=1");// TODO +CGNSSPWR: READY! abwarten!!
  delay(1000);

  SerialAT.println("AT+CGNSSTST=1");
  delay(1000);

  SerialAT.println("AT+CGNSSPORTSWITCH=0,1");
  delay(1000);
}

void loop()
{
  // NMEA-Daten auslesen (kommen auf demselben UART wie AT)
  while (SerialAT.available())
  {
    char c = SerialAT.read();
    gps.encode(c);
  }

  if (gps.location.isUpdated())
  {
    Serial.printf("Lat: %.6f  Lon: %.6f  Sat: %d  Alt: %.1f m\n",
                  gps.location.lat(),
                  gps.location.lng(),
                  gps.satellites.value(),
                  gps.altitude.meters());
  }

  static unsigned long lastNet = 0;
  if (millis() - lastNet > 10000)
  {
    lastNet = millis();
    Serial.print("Signal: ");
    Serial.print(modem.getSignalQuality());
    Serial.print("  Netz: ");
    Serial.println(modem.getOperator());
  }
}
