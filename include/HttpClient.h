#pragma once
#include "TinyGsmClient.h"
#include <Arduino.h>

class HttpClient {
public:
  HttpClient(TinyGsmClient &client);
  bool postJSON(const char* server, int port, const char* path,
                const char* token, const String &json);
private:
  TinyGsmClient &client;
};