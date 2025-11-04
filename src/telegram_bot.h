#ifndef TELEGRAM_BOT_H
#define TELEGRAM_BOT_H

#include <Arduino.h>

void checkTelegram();
void sendMessage(String text);

extern bool gpsBusy;

#endif
