#ifndef MODEM_H
#define MODEM_H

#include <Arduino.h>

extern HardwareSerial modem;

void setupModem();
String sendAT(String cmd, unsigned long timeout = 5000);
String httpGet(String url);
String getSignalQuality();

#endif
