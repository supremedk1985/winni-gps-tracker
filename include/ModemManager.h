#pragma once
#include "TinyGsmClient.h"

class ModemManager {
public:
  ModemManager(HardwareSerial &serial);
  void restart();
  bool connectLTE(const char* apn, const char* user, const char* pass);
  String getIMEI();

  TinyGsm modem;
  TinyGsmClient client;

private:
  HardwareSerial &SerialAT;
};