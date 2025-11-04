#include "telegram_bot.h"
#include "modem.h"
#include "settings.h"
#include "gnss_onboard.h"
#include "storage.h"
#include "config.h"
#include <ArduinoJson.h>

long lastUpdateId = 0;
bool gpsBusy = false;

// ==================== Telegram Send ====================
void sendMessage(String text)
{
  text.replace(" ", "%20");
  String url = String("https://api.telegram.org/bot") + BOT_TOKEN +
               "/sendMessage?chat_id=" + CHAT_ID +
               "&text=" + text;

  sendAT("AT+HTTPTERM", 1500);
  sendAT("AT+HTTPINIT", 3000);
  httpGet(url);
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
  int jsonStart = resp.indexOf("{\"ok\":");
  if (jsonStart > 0)
    resp = resp.substring(jsonStart);

  DynamicJsonDocument doc(16384);
  if (deserializeJson(doc, resp))
  {
    Serial.println("JSON Fehler oder keine neuen Daten.");
    return;
  }

  JsonArray results = doc["result"];
  if (results.isNull() || results.size() == 0) return;

  for (JsonObject upd : results)
  {
    long updateId = upd["update_id"] | 0;
    if (updateId <= lastUpdateId) continue;
    lastUpdateId = updateId;

    JsonObject msg = upd["message"];
    if (msg.isNull() || msg["from"]["is_bot"]) continue;

    String cmd = String((const char *)(msg["text"] | ""));
    cmd.toLowerCase();

    Serial.print("Empfangen: ");
    Serial.println(cmd);

    if (cmd == "/info" || cmd == "info"){
      String sig = getSignalQuality();
    uint64_t sizeMB = getCardSizeMB();
    String sdInfo = (sizeMB > 0) ? "SD: " + String(sizeMB) + " MB" : "SD: nicht verfügbar";
    sendMessage(sig + " | " + sdInfo);
    }
    else if (cmd == "/gps" || cmd == "gps")
    {
      if (!gpsBusy)
        getGPS();
      else
        Serial.println("GNSS bereits aktiv, ignoriere weiteren /gps.");
    }
  }
}
