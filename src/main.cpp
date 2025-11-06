#include <Arduino.h>
#include "modem.h"
#include "telegram_bot.h"
#include "gps.h"
#include "storage.h"

void setup()
{
    Serial.begin(115200);
    Serial.println("== Winni GPS Tracker ==");

    setupModem();

    if (!initStorage()) {
        Serial.println("Keine SD-Karte erkannt");
    } else {
        Serial.println("SD-Karte bereit für GPS-Aufzeichnung");
    }

    initGPS();

    sendMessage("Winni GPS Tracker gestartet.");
    
    // Optional: GPS-Tracking automatisch starten
    // Kommentar entfernen um automatisch zu starten:
    // delay(5000); // 5 Sekunden warten bis GPS-Fix
    // startGPSTracking();
}

void loop()
{
    // Telegram Bot Nachrichten prüfen
    checkTelegram();
    
    // GPS Tracking verarbeiten (wenn aktiv)
    processGPSTracking();
    
    // Kurze Pause
    delay(1000); // 1 Sekunde (für GPS-Tracking sollte die Loop öfter laufen)
}
