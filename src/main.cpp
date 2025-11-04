#include <Arduino.h>
#include "modem.h"
#include "telegram_bot.h"
#include "gnss_onboard.h"

void setup()
{
  Serial.begin(115200);
  Serial.println("== Telegram Bot startet ==");
  setupModem();
  sendMessage("Bot gestartet.");
}

void loop()
{
  checkTelegram();
  delay(10000);
}
