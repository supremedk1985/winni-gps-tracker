#include "modem.h"
#include "config.h"
#include <HardwareSerial.h>

HardwareSerial modem(1);

#define RXD2 17
#define TXD2 18

// ==================== AT Helper ====================
String sendAT(String cmd, unsigned long timeout)
{
  modem.println(cmd);
  String resp;
  unsigned long start = millis();
  while (millis() - start < timeout)
  {
    while (modem.available())
      resp += (char)modem.read();
  }
  Serial.println(resp);
  return resp;
}

// ==================== HTTP GET ====================
String httpGet(String url)
{
  String result;
  while (modem.available()) modem.read();

  // HTTPS aktivieren (Pflicht für Telegram)
  sendAT("AT+HTTPSSL=1", 1000);

  modem.printf("AT+HTTPPARA=\"URL\",\"%s\"\r\n", url.c_str());
  delay(300);
  modem.println("AT+HTTPACTION=0");

  String line;
  unsigned long t0 = millis();
  int contentLen = -1;
  while (millis() - t0 < 20000)
  {
    while (modem.available())
    {
      char c = modem.read();
      if (c == '\n')
      {
        if (line.indexOf("+HTTPACTION:") >= 0)
        {
          int p2 = line.lastIndexOf(',');
          if (p2 > 0) contentLen = line.substring(p2 + 1).toInt();
          line = "";
          goto got_len;
        }
        line = "";
      }
      else line += c;
    }
  }
got_len:
  if (contentLen <= 0) return "";

  delay(500);
  modem.printf("AT+HTTPREAD=0,%d\r\n", contentLen);

  // --- komplette Antwort lesen ---
  String body = "";
  unsigned long t1 = millis();
  while (millis() - t1 < 8000)
  {
    while (modem.available())
    {
      char c = modem.read();
      body += c;
      t1 = millis(); // Timeout zurücksetzen solange Daten kommen
    }
  }

  // --- alle JSON-Blöcke vollständig berücksichtigen ---
  int firstBrace = body.indexOf('{');
  int lastBrace  = body.lastIndexOf('}');
  while (true)
  {
    int next = body.indexOf('}', lastBrace + 1);
    if (next == -1) break;
    lastBrace = next;
  }

  if (firstBrace >= 0 && lastBrace > firstBrace)
    body = body.substring(firstBrace, lastBrace + 1);
  else
    body = "";

  Serial.println("---- HTTPREAD Antwort (bereinigt) ----");
  Serial.println(body);
  Serial.println("--------------------------------------");

  // --- Bilder oder Nicht-Text-Nachrichten erkennen ---
  if (body.indexOf("\"photo\"") != -1)
  {
    Serial.println("Bildnachricht erkannt → übersprungen.");

    // update_id aus der Antwort extrahieren
    int pos = body.indexOf("\"update_id\":");
    if (pos >= 0)
    {
      long tmp = body.substring(pos + 13).toInt();
      extern long lastUpdateId;
      if (tmp > lastUpdateId)
      {
        lastUpdateId = tmp;
        Serial.printf("Update %ld übersprungen (Bildnachricht)\n", tmp);
      }
    }

    return ""; // keine weitere Verarbeitung
  }

  return body;
}

// ==================== Modem Setup ====================
void setupModem()
{
  modem.begin(115200, SERIAL_8N1, RXD2, TXD2);
  delay(3000);

  sendAT("AT");
  sendAT("ATE1");
  sendAT("AT+CGNSSPWR=0", 2000);
  sendAT("AT+CGATT=1", 10000);
  sendAT("AT+CGDCONT=1,\"IP\",\"internet\"");
  sendAT("AT+CGACT=1,1", 10000);
  sendAT("AT+CGPADDR=1", 5000);
  sendAT("AT+HTTPTERM", 2000);
  sendAT("AT+HTTPINIT", 5000);
}

// ==================== Signal Info ====================
String getSignalQuality()
{
  String resp = sendAT("AT+CSQ", 2000);
  int pos = resp.indexOf("+CSQ: ");
  if (pos < 0)
    return "Signal: unbekannt";
  int comma = resp.indexOf(",", pos);
  int val = resp.substring(pos + 6, comma).toInt();
  if (val == 99)
    return "Kein Netz";
  int dBm = -113 + (val * 2);
  return "Signalstaerke: " + String(dBm) + " dBm";
}
