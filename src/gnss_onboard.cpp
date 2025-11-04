#include "gnss_onboard.h"
#include "settings.h"
#include "config.h"
#include <HardwareSerial.h>

extern HardwareSerial modem;
extern bool gpsBusy;

extern String sendAT(String cmd, unsigned long timeout);
extern void sendMessage(String text);

bool parseClbs(const String &clbs, String &lat, String &lon) {
  int idx = clbs.indexOf("+CLBS:");
  if (idx < 0) return false;
  String line = clbs.substring(idx);
  line.replace("\r", ""); line.replace("\n", ""); line.trim();
  int p1 = line.indexOf(',');
  int p2 = line.indexOf(',', p1 + 1);
  int p3 = line.indexOf(',', p2 + 1);
  if (p1 < 0 || p2 < 0 || p3 < 0) return false;
  String status = line.substring(7, p1); status.trim();
  if (status != "0") return false;
  lat = line.substring(p1 + 1, p2);
  lon = line.substring(p2 + 1, p3);
  return (lat.length() > 4 && lon.length() > 4);
}

bool parseCgnssInfo(const String &resp, double &latDeg, double &lonDeg) {
  int latStart = resp.indexOf(",") + 1;
  int nMark = resp.indexOf(",N,");
  int sMark = resp.indexOf(",S,");
  int latEnd = (nMark >= 0) ? nMark : sMark;
  if (latEnd < 0) return false;
  int latCommaPrev = resp.lastIndexOf(',', latEnd - 1);
  String latTok = resp.substring(latCommaPrev + 1, latEnd);
  latTok.trim();

  int eMark = resp.indexOf(",E,", latEnd + 3);
  int wMark = resp.indexOf(",W,", latEnd + 3);
  int lonEnd = (eMark >= 0) ? eMark : wMark;
  if (lonEnd < 0) return false;

  int lonCommaPrev = resp.lastIndexOf(',', lonEnd - 1);
  String lonTok = resp.substring(lonCommaPrev + 1, lonEnd);
  lonTok.trim();

  bool south = (sMark >= 0);
  bool west = (wMark >= 0);

  latDeg = latTok.toDouble();
  lonDeg = lonTok.toDouble();

  if (south) latDeg = -latDeg;
  if (west) lonDeg = -lonDeg;

  return !(isnan(latDeg) || isnan(lonDeg));
}

String getGPS() {
  if (gpsBusy) return "GNSS läuft bereits.";
  gpsBusy = true;

  sendMessage("Okay, hole GPS. Melde mich dann zurück...");
  Serial.println("== GNSS Start ==");

  String clbs = sendAT("AT+CLBS=1,1", 12000);
  String lteMsg = "LTE-Standort nicht verfügbar.";
  String latS, lonS;
  if (parseClbs(clbs, latS, lonS)) {
    lteMsg = "Vorlaeufiger Standort (LTE): " + latS + "," + lonS +
             " https://maps.google.com/?q=" + latS + "," + lonS;
  }
  sendMessage(lteMsg);

  sendAT("AT+CGACT=0,1", 7000);
  sendAT("AT+CGATT=0", 5000);
  sendAT("AT+CGNSSPWR=1", 4000);

  bool ready = false;
  for (int i = 0; i < 30; i++) {
    String rdy = sendAT("AT+CGNSSINFO", 2500);
    if (rdy.indexOf("+CGNSSINFO:") >= 0) {
      ready = true;
      break;
    }
    delay(3000);
  }
  if (!ready) {
    sendMessage("GNSS nicht erreichbar. Verwende LTE-Position.");
    sendAT("AT+CGNSSPWR=0", 2000);
    sendAT("AT+CGATT=1", 5000);
    sendAT("AT+CGACT=1,1", 10000);
    gpsBusy = false;
    return lteMsg;
  }

  double latDeg = NAN, lonDeg = NAN;
  bool fix = false;
  for (int i = 0; i < GNNS_MAX_RETRIES; i++) {
    String resp = sendAT("AT+CGNSSINFO", 3000);
    if (parseCgnssInfo(resp, latDeg, lonDeg)) {
      fix = true;
      break;
    }
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

  sendAT("AT+CGNSSPWR=0", 2000);
  sendAT("AT+CGATT=1", 5000);
  sendAT("AT+CGACT=1,1", 10000);

  sendMessage(msg);
  gpsBusy = false;
  return msg;
}
