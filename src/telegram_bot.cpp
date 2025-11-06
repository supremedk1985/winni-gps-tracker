#include "telegram_bot.h"
#include "modem.h"
#include "settings.h"
#include "gps.h"
#include "storage.h"
#include "config.h"
#include "utils.h"
#include "gps.h"
#include <ArduinoJson.h>

long lastUpdateId = 0;

// ==================== Telegram Send ====================
void sendMessage(String text)
{
  String encodedText = urlEncode(text);

  String url = String("https://api.telegram.org/bot") + BOT_TOKEN +
               "/sendMessage?chat_id=" + CHAT_ID +
               "&parse_mode=Markdown&text=" + encodedText;

  Serial.println("Sende Telegram Nachricht:");
  Serial.println(url);

  sendAT("AT+HTTPTERM", 1500);
  sendAT("AT+HTTPINIT", 3000);
  httpGet(url);
}

void getInfo()
{
  // --- Signalqualität auswerten ---
  String resp = sendAT("AT+CSQ", 2000);
  int pos = resp.indexOf("+CSQ: ");
  int val = -1;
  if (pos >= 0)
  {
    int comma = resp.indexOf(",", pos);
    val = resp.substring(pos + 6, comma).toInt();
  }

  int dBm = -113 + (val * 2);
  String sigIcon, sigText;
  if (val == 99 || val < 0)
  {
    sigIcon = "❌";
    sigText = "Kein Netz";
  }
  else if (val < 10)
  {
    sigIcon = "📶▫️";
    sigText = String(dBm) + " dBm (schwach)";
  }
  else if (val < 20)
  {
    sigIcon = "📶▴";
    sigText = String(dBm) + " dBm (mittel)";
  }
  else
  {
    sigIcon = "📶📈";
    sigText = String(dBm) + " dBm (gut)";
  }

  // --- SD-Info ---
  uint64_t totalMB = getCardSizeMB();
  uint64_t used = SD_MMC.usedBytes() / (1024ULL * 1024ULL);
  uint64_t freeMB = totalMB > used ? totalMB - used : 0;

  String sdIcon;
  double freeRatio = (totalMB > 0) ? (double)freeMB / totalMB : 0;
  if (totalMB == 0)
  {
    sdIcon = "💾❌ SD nicht erkannt";
  }
  else if (freeRatio < 0.1)
  {
    sdIcon = "💾🔴 " + String(freeMB) + "MB frei von " + String(totalMB) + "MB";
  }
  else if (freeRatio < 0.5)
  {
    sdIcon = "💾🟠 " + String(freeMB) + "MB frei von " + String(totalMB) + "MB";
  }
  else
  {
    sdIcon = "💾🟢 " + String(freeMB) + "MB frei von " + String(totalMB) + "MB";
  }

  // --- Nachricht senden ---
  String msg = sigIcon + " " + sigText + "\n" + sdIcon;
  Serial.println(msg);
  sendMessage(msg);
}

// ==================== Telegram Poll ====================
void checkTelegram()
{
  String url = String("https://api.telegram.org/bot") + BOT_TOKEN + "/getUpdates";
  if (lastUpdateId > 0)
    url += "?offset=" + String(lastUpdateId + 1);
  else
    url += "?limit=1";

  sendAT("AT+HTTPTERM", 1500);
  sendAT("AT+HTTPINIT", 3000);

  String resp = httpGet(url);

  // --- Wenn httpGet leer zurückgibt ---
  if (resp.length() == 0)
  {
    Serial.println("Keine Antwort vom Server");
    return;
  }

  // --- Nur das erste gültige JSON-Objekt extrahieren ---
  int firstBrace = resp.indexOf('{');
  int lastBrace = resp.lastIndexOf('}');
  if (firstBrace >= 0 && lastBrace > firstBrace)
    resp = resp.substring(firstBrace, lastBrace + 1);
  else
  {
    Serial.println("Keine gültige JSON-Struktur gefunden");
    return;
  }

  // --- JSON deserialisieren mit GRÖßEREM BUFFER (32KB für Bildnachrichten) ---
  DynamicJsonDocument doc(32768);
  DeserializationError err = deserializeJson(doc, resp);
  if (err)
  {
    Serial.print("JSON Fehler: ");
    Serial.println(err.c_str());
    Serial.printf("JSON Länge: %d bytes\n", resp.length());

    // >>> ROBUSTE update_id Extraktion bei Fehlern <<<
    long maxUpdateId = lastUpdateId;
    int searchPos = 0;
    
    while (true)
    {
      int pos = resp.indexOf("\"update_id\":", searchPos);
      if (pos == -1) break;
      
      // Finde den Wert zwischen : und dem nächsten Komma/Newline
      int startValue = pos + 12; // nach "update_id":
      while (startValue < resp.length() && (resp.charAt(startValue) == ' ' || resp.charAt(startValue) == ':'))
        startValue++;
      
      int endValue = startValue;
      while (endValue < resp.length() && 
             resp.charAt(endValue) >= '0' && 
             resp.charAt(endValue) <= '9')
      {
        endValue++;
      }
      
      if (endValue > startValue)
      {
        String idStr = resp.substring(startValue, endValue);
        long tmp = idStr.toInt();
        if (tmp > maxUpdateId)
        {
          maxUpdateId = tmp;
          Serial.printf("Gefunden: update_id = %ld\n", tmp);
        }
      }
      
      searchPos = endValue;
    }
    
    if (maxUpdateId > lastUpdateId)
    {
      lastUpdateId = maxUpdateId;
      Serial.printf("→ Überspringe alle Updates bis %ld\n", maxUpdateId);
    }
    return;
  }

  JsonArray results = doc["result"];
  if (results.isNull() || results.size() == 0)
  {
    Serial.println("Keine neuen Updates");
    return;
  }

  Serial.printf("Verarbeite %d Update(s)\n", results.size());

  for (JsonObject upd : results)
  {
    long updateId = upd["update_id"] | 0;
    if (updateId <= lastUpdateId)
      continue;
    
    // WICHTIG: update_id IMMER aktualisieren, auch wenn die Nachricht übersprungen wird
    lastUpdateId = updateId;

    JsonObject msg = upd["message"];
    if (msg.isNull() || msg["from"]["is_bot"])
    {
      Serial.printf("Update %ld: Bot oder leere Nachricht → übersprungen.\n", updateId);
      continue;
    }

    // --- Mediennachrichten (Bilder, Videos, Dokumente etc.) überspringen ---
    if (msg.containsKey("photo") || msg.containsKey("video") || 
        msg.containsKey("document") || msg.containsKey("audio") ||
        msg.containsKey("voice") || msg.containsKey("sticker"))
    {
      Serial.printf("Update %ld: Mediennachricht → übersprungen.\n", updateId);
      continue;
    }

    // --- Nur Textnachrichten verarbeiten ---
    if (!msg.containsKey("text"))
    {
      Serial.printf("Update %ld: Keine Text-Nachricht → übersprungen.\n", updateId);
      continue;
    }

    String cmd = String((const char *)(msg["text"] | ""));
    cmd.toLowerCase();

    Serial.printf("Update %ld empfangen: %s\n", updateId, cmd.c_str());

    if (cmd == "/info" || cmd == "info")
    {
      getInfo();
    }
    else if (cmd == "/gps" || cmd == "gps")
    {
      getGPSLocation();
    }
    else if (cmd == "/start" || cmd == "start")
    {
      sendMessage("Winni GPS Tracker aktiv!\n\nBefehle:\n/info - System-Info\n/gps - GPS Position");
    }
    else
    {
      Serial.println("Unbekannter Befehl → ignoriert.");
    }
  }
}
