#include "HttpClient.h"

HttpClient::HttpClient(TinyGsmClient &c) : client(c) {}

bool HttpClient::postJSON(const char* server, int port, const char* path,
                          const char* token, const String &json) {
  Serial.println("Sende POST an Node-RED...");
  if (!client.connect(server, port)) {
    Serial.println("❌ Verbindung zu Server fehlgeschlagen");
    return false;
  }

  client.print(String("POST ") + path + " HTTP/1.1\r\n");
  client.print(String("Host: ") + server + "\r\n");
  client.print("Content-Type: application/json\r\n");
  client.print(String("X-Token: ") + token + "\r\n");
  client.print(String("Content-Length: ") + json.length() + "\r\n");
  client.print("Connection: close\r\n\r\n");
  client.print(json);

  Serial.println("✅ Gesendet:");
  Serial.println(json);

  delay(1000);
  while (client.available()) Serial.write(client.read());
  client.stop();
  return true;
}