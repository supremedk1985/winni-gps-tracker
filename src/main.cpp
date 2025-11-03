#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "config.h"

HardwareSerial modem(1);

#define RXD2 17
#define TXD2 18

// ==================== Globale Variablen ====================
long lastUpdateId = 0;
bool gpsBusy = false; // Re-Entry-Schutz

// ==================== AT Helper ====================
String sendAT(String cmd, unsigned long timeout = 5000) {
  modem.println(cmd);
  String resp = "";
  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (modem.available()) resp += (char)modem.read();
  }
  Serial.println(resp);
  return resp;
}

// ==================== Modem Setup ====================
void setupModem() {
  sendAT("AT");
  sendAT("ATE1");
  sendAT("AT+CGATT=1", 10000);
  sendAT("AT+CGDCONT=1,\"IP\",\"internet\"");
  sendAT("AT+CGACT=1,1", 10000);
  sendAT("AT+CGPADDR=1", 5000);
  sendAT("AT+HTTPTERM", 2000);   // kann ERROR sein, egal
  sendAT("AT+HTTPINIT", 5000);
}

// ==================== HTTP GET ====================
String httpGet(String url) {
  String result = "";
  while (modem.available()) modem.read(); // UART-Puffer leeren

  modem.print("AT+HTTPPARA=\"URL\",\""); modem.print(url); modem.println("\"");
  delay(500);
  modem.println("AT+HTTPACTION=0");

  // Warte gezielt auf "+HTTPACTION: 0,<code>,<len>"
  String line = "";
  unsigned long t0 = millis();
  int contentLen = -1;
  while (millis() - t0 < 20000) {
    while (modem.available()) {
      char c = modem.read();
      if (c == '\n') {
        if (line.indexOf("+HTTPACTION:") >= 0) {
          int p2 = line.lastIndexOf(',');
          if (p2 > 0) contentLen = line.substring(p2 + 1).toInt();
          line = "";
          goto got_len;
        }
        line = "";
      } else {
        line += c;
      }
    }
  }
got_len:
  if (contentLen < 0) contentLen = 0;
  delay(800);

  // Body vollständig lesen
  int offset = 0;
  while (offset < contentLen) {
    int block = min(512, contentLen - offset);
    modem.print("AT+HTTPREAD="); modem.print(offset); modem.print(","); modem.println(block);
    delay(300);

    unsigned long t1 = millis();
    while (millis() - t1 < 3000) {
      while (modem.available()) result += (char)modem.read();
    }
    offset += block;
  }

  Serial.println("---- HTTPREAD Antwort ----");
  Serial.println(result);
  Serial.println("---------------------------");
  return result;
}

// ==================== Telegram Send ====================
void sendMessage(String text) {
  text.replace(" ", "%20");
  String url = String("https://api.telegram.org/bot") + BOT_TOKEN +
               "/sendMessage?chat_id=" + CHAT_ID +
               "&text=" + text;
  // robuste Re-Init vor HTTP
  String term = sendAT("AT+HTTPTERM", 1500); // ERROR ignorieren
  sendAT("AT+HTTPINIT", 3000);
  httpGet(url);
}

// ==================== Signal Info ====================
String getSignalQuality() {
  String resp = sendAT("AT+CSQ", 2000);
  int pos = resp.indexOf("+CSQ: ");
  if (pos < 0) return "Signal: unbekannt";
  int comma = resp.indexOf(",", pos);
  int val = resp.substring(pos + 6, comma).toInt();
  if (val == 99) return "Kein Netz";
  int dBm = -113 + (val * 2);
  return "Signalstaerke: " + String(dBm) + " dBm";
}

// ==================== CLBS Parser ====================
bool parseClbs(const String& clbs, String& lat, String& lon) {
  int idx = clbs.indexOf("+CLBS:");
  if (idx < 0) return false;
  String line = clbs.substring(idx);
  line.replace("\r", ""); line.replace("\n", ""); line.trim();
  // "+CLBS: 0,lat,lon,acc"
  int p1 = line.indexOf(',');                  // nach Status
  int p2 = line.indexOf(',', p1 + 1);          // nach lat
  int p3 = line.indexOf(',', p2 + 1);          // nach lon
  if (p1 < 0 || p2 < 0 || p3 < 0) return false;
  String status = line.substring(7, p1); status.trim();
  if (status != "0") return false;
  lat = line.substring(p1 + 1, p2);
  lon = line.substring(p2 + 1, p3);
  return (lat.length() > 4 && lon.length() > 4);
}

// ==================== GNSS Parser (robust) ====================
// Extrahiert lat/lon aus +CGNSSINFO per Marker ",N,"/ ",S," und ",E,"/ ",W,"
// Wandelt ddmm.mmmm -> dezimalgrad
bool parseCgnssInfo(const String& resp, double& latDeg, double& lonDeg) {
  int nPos = resp.indexOf(",N,");
  int sPos = resp.indexOf(",S,");
  int latMarker = (nPos >= 0) ? nPos : sPos;
  if (latMarker < 0) return false; // kein Lat-Block

  // Latitude-Token: zwischen letztem Komma vor Marker und Marker
  int latCommaPrev = resp.lastIndexOf(',', latMarker - 1);
  if (latCommaPrev < 0) return false;
  String latTok = resp.substring(latCommaPrev + 1, latMarker);
  latTok.trim();

  // Longitude-Token: nach Marker bis vor ,E, oder ,W,
  int ePos = resp.indexOf(",E,", latMarker + 3);
  int wPos = resp.indexOf(",W,", latMarker + 3);
  int lonMarker = (ePos >= 0) ? ePos : wPos;
  if (lonMarker < 0) return false;

  int lonCommaPrev = resp.lastIndexOf(',', lonMarker - 1);
  if (lonCommaPrev < 0) return false;
  String lonTok = resp.substring(lonCommaPrev + 1, lonMarker);
  lonTok.trim();

  auto ddmm_to_deg = [](const String& t, bool isLat) -> double {
    // lat: 2 deg, lon: 3 deg
    int d = isLat ? 2 : 3;
    if ((int)t.length() < d+1) return NAN;
    String degStr = t.substring(0, d);
    String minStr = t.substring(d);
    double deg = degStr.toInt();
    double mins = minStr.toFloat();
    return deg + (mins / 60.0);
  };

  bool latSouth = (sPos >= 0);
  bool lonWest  = (wPos >= 0);

  latDeg = ddmm_to_deg(latTok, true);
  lonDeg = ddmm_to_deg(lonTok, false);
  if (isnan(latDeg) || isnan(lonDeg)) return false;
  if (latSouth) latDeg = -latDeg;
  if (lonWest)  lonDeg = -lonDeg;
  return true;
}

// ==================== GPS Handler ====================
String getGPS() {
  if (gpsBusy) return "GNSS läuft bereits.";
  gpsBusy = true;

  sendMessage("Okay, hole GPS. Melde mich dann zurück...");
  Serial.println("== GNSS Start ==");
  Serial.println("-> Versuche LTE Standort (AT+CLBS=1,1)...");

  String clbs = sendAT("AT+CLBS=1,1", 12000);
  String lteMsg = "LTE-Standort nicht verfügbar.";
  String latS, lonS;
  if (parseClbs(clbs, latS, lonS)) {
    lteMsg = "Vorlaeufiger Standort (LTE): " + latS + "," + lonS +
             " https://maps.google.com/?q=" + latS + "," + lonS;
  }
  Serial.println(lteMsg);
  sendMessage(lteMsg);

  // LTE aus, GNSS ein
  Serial.println("-> Schalte LTE ab und GNSS ein...");
  sendAT("AT+CGACT=0,1", 7000);
  sendAT("AT+CGATT=0", 5000);
  sendAT("AT+CGNSSPWR=1", 4000);

  // Warten bis GNSS antwortet
  Serial.println("-> Warte auf GNSS Antwort (+CGNSSINFO)...");
  bool ready = false;
  for (int i = 0; i < 30; i++) {
    String rdy = sendAT("AT+CGNSSINFO", 2500);
    if (rdy.indexOf("+CGNSSINFO:") >= 0) { ready = true; break; }
    Serial.print(".");
    delay(3000);
  }
  if (!ready) {
    sendMessage("GNSS nicht erreichbar. Verwende LTE-Position.");
    sendAT("AT+CGNSSPWR=0", 2000);
    sendAT("AT+CGATT=1", 5000);
    sendAT("AT+CGACT=1,1", 10000);
    sendAT("AT+HTTPTERM", 1500); sendAT("AT+HTTPINIT", 3000);
    gpsBusy = false;
    return lteMsg;
  }

  // Fix-Versuch und sauberes Parsen
  Serial.println("-> Lese GNSS-Daten...");
  String resp; bool fix = false; double latDeg = NAN, lonDeg = NAN;
  for (int i = 0; i < GNNS_MAX_RETRIES; i++) {
    resp = sendAT("AT+CGNSSINFO", 3000);
    // Fix prüfen: es reicht, wenn wir Lat/Lon finden
    if (parseCgnssInfo(resp, latDeg, lonDeg)) { fix = true; break; }
    Serial.printf("Kein Fix/keine Koordinaten (%d/%d)\n", i+1, GNNS_MAX_RETRIES);
    delay(4000);
  }

  String msg;
  if (fix) {
    String slat = String(latDeg, 6);
    String slon = String(lonDeg, 6);
    msg = "GNSS Fix: " + slat + "," + slon +
          " https://maps.google.com/?q=" + slat + "," + slon;
  } else {
    msg = "Kein GNSS Fix nach Timeout. Verwende LTE-Daten.";
  }

  // GNSS aus, LTE wieder an
  Serial.println("-> GNSS aus, LTE wieder aktivieren...");
  sendAT("AT+CGNSSPWR=0", 2000);
  sendAT("AT+CGATT=1", 5000);
  sendAT("AT+CGACT=1,1", 10000);
  sendAT("AT+HTTPTERM", 1500); sendAT("AT+HTTPINIT", 3000);

  sendMessage(msg);
  Serial.println("== GNSS Ende ==");
  gpsBusy = false;
  return msg;
}

// ==================== Telegram Poll ====================
// --- Telegram Poll --- //
void checkTelegram() {
  String url = String("https://api.telegram.org/bot") + BOT_TOKEN + "/getUpdates";
  if (lastUpdateId > 0) {
    // Telegram bekommt offset = letzter + 1, damit kein Wiederholen
    url += "?offset=" + String(lastUpdateId + 1);
  } else {
    url += "?limit=1";
  }

  sendAT("AT+HTTPTERM", 1500);
  sendAT("AT+HTTPINIT", 3000);

  String resp = httpGet(url);
  int jsonStart = resp.indexOf("{\"ok\":");
  if (jsonStart > 0) resp = resp.substring(jsonStart);

  DynamicJsonDocument doc(16384);
  if (deserializeJson(doc, resp)) {
    Serial.println("JSON Fehler oder keine neuen Daten.");
    return;
  }

  JsonArray results = doc["result"];
  if (results.isNull() || results.size() == 0) return;

  for (JsonObject upd : results) {
    long updateId = upd["update_id"] | 0;
    if (updateId <= lastUpdateId) continue; // schon verarbeitet

    lastUpdateId = updateId; // erst hier fortschreiben

    JsonObject msg = upd["message"];
    if (msg.isNull()) continue;
    if (msg["from"]["is_bot"]) continue;

    String cmd = String((const char*)(msg["text"] | ""));
    cmd.toLowerCase();

    Serial.print("Empfangen: ");
    Serial.println(cmd);

    if (cmd == "/info" || cmd == "info") {
      String sig = getSignalQuality();
      sendMessage(sig);
    } 
    else if (cmd == "/gps" || cmd == "gps") {
      if (!gpsBusy) getGPS();
      else Serial.println("GNSS bereits aktiv, ignoriere weiteren /gps.");
    }
  }
}


// ==================== Setup & Loop ====================
void setup() {
  Serial.begin(115200);
  modem.begin(115200, SERIAL_8N1, RXD2, TXD2);
  delay(3000);
  Serial.println("== Telegram Bot startet ==");
  setupModem();

  // Nur beim ersten Start senden
  if (lastUpdateId == 0) {
    sendMessage("Bot gestartet.");
  }
}


void loop() {
  checkTelegram();
  delay(10000);
}
