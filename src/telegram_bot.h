#ifndef TELEGRAM_BOT_H
#define TELEGRAM_BOT_H

#include <Arduino.h>

// Telegram-Bot-Funktionen
void checkTelegram();
void sendMessage(String text);
bool sendDocument(const char* filePath, const char* caption = nullptr);

// Spezielle Befehle
void getInfo();
void sendTrack(int days);

extern bool gpsBusy;

#endif
