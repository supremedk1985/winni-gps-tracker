#include <Arduino.h>
#include "modem.h"
#include "telegram_bot.h"
#include "gnss_onboard.h"
#include "storage.h"

void setup()
{
  Serial.begin(115200);
  Serial.println("== Winni GPS Tracker ==");
  
  setupModem();

  if (!initStorage())
  Serial.println("Keine SD-Karte erkannt");

  sendMessage("Winni GPS Tracker gestartet.");
}

void loop()
{
  checkTelegram();
  delay(10000);
}
