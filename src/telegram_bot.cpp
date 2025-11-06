#include "telegram_bot.h"
#include "modem.h"
#include "settings.h"
#include "gps.h"
#include "storage.h"
#include "config.h"
#include "utils.h"
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

    // GPS Tracking Status
    String trackStatus = isGPSTracking() ? "GPS-Tracking: AKTIV" : "GPS-Tracking: INAKTIV";

    // --- Nachricht senden ---
    String msg = sigIcon + " " + sigText + "\n" + sdIcon + "\n" + trackStatus;
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

    // --- JSON deserialisieren ---
    DynamicJsonDocument doc(16384);
    DeserializationError err = deserializeJson(doc, resp);
    if (err)
    {
        Serial.print("JSON Fehler: ");
        Serial.println(err.c_str());

        // >>> update_id trotzdem weiterzählen, um nicht zu hängen <<<
        int pos = resp.indexOf("\"update_id\":");
        if (pos >= 0)
        {
            long tmp = resp.substring(pos + 13).toInt();
            if (tmp > lastUpdateId)
            {
                lastUpdateId = tmp;
                Serial.printf("Überspringe fehlerhaftes Update %ld\n", tmp);
            }
        }
        return;
    }

    JsonArray results = doc["result"];
    if (results.isNull() || results.size() == 0)
        return;

    for (JsonObject upd : results)
    {
        long updateId = upd["update_id"] | 0;
        if (updateId <= lastUpdateId)
            continue;
        lastUpdateId = updateId;

        JsonObject msg = upd["message"];
        if (msg.isNull() || msg["from"]["is_bot"])
            continue;

        // --- Nur Textnachrichten akzeptieren ---
        if (!msg.containsKey("text"))
        {
            Serial.printf("Nicht-Text-Nachricht (z.B. Bild) bei Update %ld -> übersprungen.\n", updateId);
            continue;
        }

        String cmd = String((const char *)(msg["text"] | ""));
        cmd.toLowerCase();

        Serial.print("Empfangen: ");
        Serial.println(cmd);

        if (cmd == "/info" || cmd == "info")
        {
            getInfo();
        }
        else if (cmd == "/gps" || cmd == "gps")
        {
            getGPSLocation();
        }
        else if (cmd == "/start_tracking" || cmd == "start_tracking" || cmd == "/start" || cmd == "start")
        {
            startGPSTracking();
        }
        else if (cmd == "/stop_tracking" || cmd == "stop_tracking" || cmd == "/stop" || cmd == "stop")
        {
            stopGPSTracking();
        }
        else if (cmd == "/help" || cmd == "help")
        {
            String helpMsg = "*Verfügbare Befehle:*\n";
            helpMsg += "/info - System-Info\n";
            helpMsg += "/gps - Aktuelle GPS-Position\n";
            helpMsg += "/start_tracking - GPS-Aufzeichnung starten (alle 5m)\n";
            helpMsg += "/stop_tracking - GPS-Aufzeichnung stoppen\n";
            helpMsg += "/help - Diese Hilfe\n";
            sendMessage(helpMsg);
        }
        else
        {
            Serial.println("Unbekannter Befehl -> ignoriert.");
            sendMessage("Unbekannter Befehl. Sende /help für Hilfe.");
        }
    }
}
