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

// Datei über Telegram senden (sendDocument)
bool sendDocument(const char* filePath, const char* caption)
{
  // Prüfen, ob Datei existiert
  if (!SD_MMC.exists(filePath))
  {
    Serial.println("Datei nicht gefunden: " + String(filePath));
    sendMessage("❌ Datei nicht gefunden");
    return false;
  }
  
  File file = SD_MMC.open(filePath, FILE_READ);
  if (!file)
  {
    Serial.println("Fehler beim Öffnen der Datei");
    sendMessage("❌ Fehler beim Öffnen der Datei");
    return false;
  }
  
  size_t fileSize = file.size();
  Serial.print("Dateigröße: ");
  Serial.print(fileSize);
  Serial.println(" Bytes");
  
  // Warnung bei großen Dateien
  if (fileSize > 50 * 1024 * 1024)  // 50MB Telegram-Limit
  {
    file.close();
    sendMessage("❌ Datei zu groß (max. 50 MB)");
    return false;
  }
  
  sendMessage("📤 Sende Track-Datei...");
  
  // Dateinamen extrahieren
  String fileName = String(filePath);
  int lastSlash = fileName.lastIndexOf('/');
  if (lastSlash != -1)
    fileName = fileName.substring(lastSlash + 1);
  
  // Telegram sendDocument API
  // WICHTIG: Dies ist eine vereinfachte Version. 
  // Für den vollständigen Upload müsste multipart/form-data verwendet werden,
  // was mit AT-Befehlen komplex ist.
  
  // Alternative: Datei auf Server hochladen und Link senden
  // Oder: kleinere Dateien als Base64 über HTTP POST
  
  // Für jetzt: Statusmeldung und Dateiinfo senden
  String msg = "📊 *Track-Datei erstellt*\n";
  msg += "📁 Datei: `" + fileName + "`\n";
  msg += "📦 Größe: " + String(fileSize) + " Bytes\n";
  msg += "💾 Gespeichert auf SD-Karte\n\n";
  msg += "ℹ️ _Hinweis: Direkter Upload über LTE-Modem ist eingeschränkt._\n";
  msg += "_Datei kann über USB/SD-Karte heruntergeladen werden._";
  
  if (caption && strlen(caption) > 0)
    msg += "\n📝 " + String(caption);
  
  file.close();
  sendMessage(msg);
  
  return true;
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

// Track der letzten N Tage generieren und "senden"
void sendTrack(int days)
{
  sendMessage("🔄 Erstelle Track-Datei...");
  
  const char* gpxFile = "/track.gpx";
  
  // GPX-Datei generieren
  bool success = exportTrackToGPX(days, gpxFile);
  
  if (!success)
  {
    sendMessage("❌ Keine GPS-Daten für die letzten " + String(days) + " Tage gefunden");
    return;
  }
  
  // Datei "senden" (Info über Datei)
  String caption = "GPS-Track der letzten " + String(days) + " Tage";
  sendDocument(gpxFile, caption.c_str());
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
      Serial.printf("Nicht-Text-Nachricht (z.B. Bild) bei Update %ld → übersprungen.\n", updateId);
      continue;
    }

    String cmd = String((const char *)(msg["text"] | ""));
    cmd.toLowerCase();

    Serial.print("Empfangen: ");
    Serial.println(cmd);

    // ========== Befehle verarbeiten ==========
    
    if (cmd == "/info" || cmd == "info")
    {
      getInfo();
    }
    else if (cmd == "/gps" || cmd == "gps")
    {
      getGPSLocation();
    }
    else if (cmd == "/track" || cmd == "track")
    {
      // Standard: letzte 5 Tage
      sendTrack(5);
    }
    else if (cmd.startsWith("/track "))
    {
      // z.B. "/track 7" für letzte 7 Tage
      int days = cmd.substring(7).toInt();
      if (days > 0 && days <= 30)
        sendTrack(days);
      else
        sendMessage("❌ Ungültige Anzahl Tage (1-30)");
    }
    else if (cmd == "/help" || cmd == "help")
    {
      String helpMsg = "📱 *Verfügbare Befehle:*\n\n";
      helpMsg += "📡 `/gps` - Aktuelle Position\n";
      helpMsg += "📊 `/info` - System-Status\n";
      helpMsg += "🗺️ `/track` - GPS-Track (letzte 5 Tage)\n";
      helpMsg += "🗺️ `/track N` - GPS-Track (letzte N Tage)\n";
      helpMsg += "❓ `/help` - Diese Hilfe\n";
      sendMessage(helpMsg);
    }
    else
    {
      sendMessage("❓ Unbekannter Befehl. Sende `/help` für Hilfe.");
    }
  }
}
