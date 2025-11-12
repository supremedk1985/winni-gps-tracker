#include <Arduino.h>
#include "modem.h"
#include "telegram_bot.h"
#include "gps.h"
#include "storage.h"
#include "traccar.h"  

void setup()
{
  Serial.begin(115200);
  Serial.println("== Winni GPS Tracker ==");

  // Modem initialisieren
  setupModem();

  // SD-Karte initialisieren
  if (!initStorage())
    Serial.println("Keine SD-Karte erkannt");

  // GPS initialisieren
  initGPS();

  // Startmeldung senden
  sendMessage("✅ Winni GPS Tracker gestartet.\n📝 GPS-Logging läuft automatisch");
}

void loop()
{
  // Telegram-Nachrichten prüfen (alle 10 Sekunden)
  checkTelegram();
  
  // GPS-Daten loggen (wird intern durch Intervall gesteuert)
  logGPSData();
  
delay(5000);

send2Traccar(); 

  delay(5000);
}
