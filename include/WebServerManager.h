#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

class WebServerManager {
public:
  void begin(const char* ssid, const char* password);
  void updateStatus(const String &lat, const String &lon,
                    const String &lte, const String &post);
  void handle();
private:
  WebServer server{80};
  String htmlStatus();
  String _lat, _lon, _lte, _post;
};
