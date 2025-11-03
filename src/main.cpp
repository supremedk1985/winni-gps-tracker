#include <HardwareSerial.h>
#include <ArduinoJson.h>

HardwareSerial modem(1);

#define RXD2 17
#define TXD2 18

String BOT_TOKEN = "8432305875:AAHc0YuvoORlJRriOhcV16yOSymZtxvOSuc";
String CHAT_ID   = "1800609959";
long lastUpdateId = 0;

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
  sendAT("AT+HTTPTERM", 2000);
  sendAT("AT+HTTPINIT", 5000);
}

// ==================== HTTP GET ====================
String httpGet(String url) {
  String result = "";
  while (modem.available()) modem.read();

  modem.print("AT+HTTPPARA=\"URL\",\"");
  modem.print(url);
  modem.println("\"");
  delay(500);

  modem.println("AT+HTTPACTION=0");

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
          break;
        }
        line = "";
      } else {
        line += c;
      }
    }
    if (contentLen >= 0) break;
  }
  if (contentLen < 0) contentLen = 0;
  delay(1000);

  int offset = 0;
  while (offset < contentLen) {
    int block = min(512, contentLen - offset);
    modem.print("AT+HTTPREAD=");
    modem.print(offset);
    modem.print(",");
    modem.println(block);
    delay(400);

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
  String url = "https://api.telegram.org/bot" + BOT_TOKEN +
               "/sendMessage?chat_id=" + CHAT_ID +
               "&text=" + text;
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
  // jede Zeile separat prüfen
  int idx = clbs.indexOf("+CLBS:");
  if (idx < 0) return false;

  // Zeile isolieren
  String line = clbs.substring(idx);
  line.replace("\r", "");
  line.replace("\n", "");
  line.trim();

  // sollte jetzt z. B. "+CLBS: 0,51.887058,8.262678,550" sein
  int p1 = line.indexOf(',');
  int p2 = line.indexOf(',', p1 + 1);
  int p3 = line.indexOf(',', p2 + 1);
  if (p1 < 0 || p2 < 0 || p3 < 0) return false;

  String status = line.substring(7, p1);
  status.trim();
  if (status != "0") return false;  // nur Status=0 gültig

  lat = line.substring(p1 + 1, p2);
  lon = line.substring(p2 + 1, p3);

  return (lat.length() > 4 && lon.length() > 4);
}

// ==================== GPS Handler ====================
String getGPS() {
  sendMessage("Okay, hole GPS. Melde mich dann zurück...");
  Serial.println("== GNSS Start ==");
  Serial.println("-> Versuche LTE Standort (AT+CLBS=1,1)...");

  String clbs = sendAT("AT+CLBS=1,1", 12000);
  String lteMsg = "LTE-Standort nicht verfügbar.";
  String lat, lon;
  if (parseClbs(clbs, lat, lon)) {
    lteMsg = "Vorlaeufiger Standort (LTE): " + lat + "," + lon +
             " https://maps.google.com/?q=" + lat + "," + lon;
  }
  Serial.println(lteMsg);
  sendMessage(lteMsg);

  // LTE aus, GNSS ein
  Serial.println("-> Schalte LTE ab und GNSS ein...");
  sendAT("AT+CGACT=0,1", 7000);
  sendAT("AT+CGATT=0", 5000);
  sendAT("AT+CGNSSPWR=1", 4000);

  // Warten auf GNSS Reaktion
  Serial.println("-> Warte auf GNSS Antwort (+CGNSSINFO)...");
  bool ready = false;
  for (int i = 0; i < 30; i++) {  // erhöhtes Timeout
    String rdy = sendAT("AT+CGNSSINFO", 2000);
    if (rdy.indexOf("+CGNSSINFO:") >= 0) { ready = true; break; }
    Serial.print(".");
    delay(3000);
  }

  if (!ready) {
    sendMessage("GNSS nicht erreichbar. Verwende LTE-Position.");
    sendAT("AT+CGNSSPWR=0", 2000);
    sendAT("AT+CGATT=1", 4000);
    sendAT("AT+CGACT=1,1", 8000);
    sendAT("AT+HTTPTERM", 2000);
    sendAT("AT+HTTPINIT", 3000);
    return lteMsg;
  }

  // Fix-Versuch
  Serial.println("-> Lese GNSS-Daten...");
  String resp;
  bool fix = false;
  for (int i = 0; i < 15; i++) {
    resp = sendAT("AT+CGNSSINFO", 3000);
    if (resp.indexOf("+CGNSSINFO:") >= 0 && resp.indexOf(",N,") > 0) { fix = true; break; }
    Serial.printf("Kein Fix (%d/15)\n", i+1);
    delay(4000);
  }

  String msg;
  if (fix) {
    int p = resp.indexOf("+CGNSSINFO:");
    int c1 = resp.indexOf(',', p);      
    int c2 = resp.indexOf(',', c1+1);   
    int c3 = resp.indexOf(',', c2+1);   
    int c4 = resp.indexOf(',', c3+1);   
    int c5 = resp.indexOf(',', c4+1);   
    int c6 = resp.indexOf(',', c5+1);   
    int c7 = resp.indexOf(',', c6+1);   
    int c8 = resp.indexOf(',', c7+1);   

    String glat = resp.substring(c4+1, c5);   
    String glon = resp.substring(c6+1, c7);   

    msg = "GNSS Fix (raw): " + glat + "," + glon +
          " https://maps.google.com/?q=" + glat + "," + glon;
  } else {
    msg = "Kein GNSS Fix nach Timeout. Verwende LTE-Daten.";
  }

  Serial.println("-> GNSS aus, LTE wieder aktivieren...");
  sendAT("AT+CGNSSPWR=0", 2000);
  sendAT("AT+CGATT=1", 5000);
  sendAT("AT+CGACT=1,1", 10000);
  sendAT("AT+HTTPTERM", 2000);
  sendAT("AT+HTTPINIT", 4000);

  sendMessage(msg);
  Serial.println("== GNSS Ende ==");
  return msg;
}

// ==================== Telegram Poll ====================
void checkTelegram() {
  String url = "https://api.telegram.org/bot" + BOT_TOKEN + "/getUpdates?limit=1";
  if (lastUpdateId > 0) url += "&offset=" + String(lastUpdateId + 1);

  String resp = httpGet(url);

  int jsonStart = resp.indexOf("{\"ok\":");
  if (jsonStart > 0) resp = resp.substring(jsonStart);
  int jsonEnd = resp.lastIndexOf("}");
  if (jsonEnd > 0 && jsonEnd + 1 < resp.length())
    resp = resp.substring(0, jsonEnd + 1);

  if (resp.indexOf("\"result\":") < 0) {
    Serial.println("Keine neuen Updates.");
    return;
  }

  DynamicJsonDocument doc(12288);
  DeserializationError err = deserializeJson(doc, resp);
  if (err) {
    Serial.print("JSON Fehler: ");
    Serial.println(err.f_str());
    return;
  }

  JsonArray results = doc["result"];
  if (results.isNull() || results.size() == 0) {
    Serial.println("Leere result-Liste.");
    return;
  }

  JsonObject last = results[results.size() - 1];
  long updateId = last["update_id"] | 0;
  String text = last["message"]["text"] | "";

  Serial.print("Empfangen: ");
  Serial.println(text);

  lastUpdateId = updateId;
  text.toLowerCase();

  if (text == "/info" || text == "info") {
    String sig = getSignalQuality();
    sendMessage(sig);
  } else if (text == "/gps" || text == "gps") {
    getGPS();
  }
}

// ==================== Setup & Loop ====================
void setup() {
  Serial.begin(115200);
  modem.begin(115200, SERIAL_8N1, RXD2, TXD2);
  delay(3000);
  Serial.println("== Telegram Bot startet ==");

  setupModem();
  sendMessage("Bot gestartet.");
}

void loop() {
  checkTelegram();
  delay(10000);
}
