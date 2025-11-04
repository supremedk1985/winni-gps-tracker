#include "gnss_onboard.h"
#include "settings.h"
#include "config.h"
#include <HardwareSerial.h>

extern HardwareSerial modem;
extern bool gpsBusy;

extern String sendAT(String cmd, unsigned long timeout);
extern void sendMessage(String text);

bool parseClbs(const String &clbs, String &lat, String &lon)
{
  int idx = clbs.indexOf("+CLBS:");
  if (idx < 0)
    return false;
  String line = clbs.substring(idx);
  line.replace("\r", "");
  line.replace("\n", "");
  line.trim();
  int p1 = line.indexOf(',');
  int p2 = line.indexOf(',', p1 + 1);
  int p3 = line.indexOf(',', p2 + 1);
  if (p1 < 0 || p2 < 0 || p3 < 0)
    return false;
  String status = line.substring(7, p1);
  status.trim();
  if (status != "0")
    return false;
  lat = line.substring(p1 + 1, p2);
  lon = line.substring(p2 + 1, p3);
  return (lat.length() > 4 && lon.length() > 4);
}


bool parseCgnssInfo(const String &resp, GNSSInfo &info)
{
  info = GNSSInfo();  // reset
  info.valid = false;

  int p = resp.indexOf("+CGNSSINFO:");
  if (p < 0) return false;

  // Payload ausschneiden und normalisieren
  String s = resp.substring(p + 12);
  s.replace("\r", "");
  s.replace("\n", "");
  s.trim();

  // Tokenize
  const int MAXTOK = 40;
  String t[MAXTOK];
  int n = 0;
  int start = 0;
  while (n < MAXTOK) {
    int c = s.indexOf(',', start);
    if (c < 0) { t[n++] = s.substring(start); break; }
    t[n++] = s.substring(start, c);
    start = c + 1;
  }
  if (n < 6) return false;

  // Anker finden: N/S und E/W
  int idxNS = -1, idxEW = -1;
  for (int i = 0; i < n; i++) {
    if (t[i] == "N" || t[i] == "S") idxNS = i;
    if (t[i] == "E" || t[i] == "W") { idxEW = i; break; }
  }
  if (idxNS < 1 || idxEW < 1) return false;

  // Lat/Lon lesen (bereits in Dezimalgrad in deinem Beispiel)
  double lat = t[idxNS - 1].toDouble();
  double lon = t[idxEW - 1].toDouble();
  if (t[idxNS] == "S") lat = -lat;
  if (t[idxEW] == "W") lon = -lon;

  if (isnan(lat) || isnan(lon)) return false;

  info.latDeg = lat;
  info.lonDeg = lon;

  // Nach E/W: erwartete Reihenfolge (best effort)
  // Beispiel: ..., E, 041125, 183613.00, 134.3, 2.020, , 5.79, 4.05, 4.14, 06
  int i = idxEW + 1;

  // Datum
  if (i < n && t[i].length()) { info.utcDate = t[i++]; }
  // Zeit
  if (i < n && t[i].length()) { info.utcDate += " " + t[i++]; }

  // Höhe
  if (i < n && t[i].length()) info.altitude = t[i++].toDouble();

  // Speed
  if (i < n && t[i].length()) info.speed = t[i++].toDouble();

  // Kurs optional
  if (i < n && t[i].length()) {
    // Manche Module setzen hier leer -> überspringen
    double v = t[i].toDouble();
    if (!(t[i].length() == 0 || (t[i] == "0" && t[i].length()==0))) info.course = v;
    i++;
  }

  // HDOP suchen: nimm erstes nicht-leeres numerisches Feld ab hier
  while (i < n && t[i].length() == 0) i++;
  if (i < n && t[i].length()) { info.hdop = t[i++].toDouble(); }

  // SatsUsed als letztes ganzzahliges Feld, wenn vorhanden
  for (int k = n - 1; k >= 0; k--) {
    bool allDigits = true;
    for (size_t j = 0; j < t[k].length(); j++) {
      if (!isDigit(t[k][j])) { allDigits = false; break; }
    }
    if (allDigits && t[k].length() > 0) { info.satsUsed = t[k].toInt(); break; }
  }

  // Fix-Mode heuristisch: Höhe gültig ⇒ 3D, sonst 2D
  info.fixMode = !isnan(info.altitude) ? 2 : 1; // 2=3D, 1=2D in deiner Semantik
  info.valid = true;
  return true;
}


String getGPS()
{
  if (gpsBusy)
    return "GNSS läuft bereits.";
  gpsBusy = true;

  sendMessage("Okay, hole GPS. Melde mich dann zurück...");
  Serial.println("== GNSS Start ==");

  String clbs = sendAT("AT+CLBS=1,1", 12000);
  String lteMsg = "LTE-Standort nicht verfügbar.";
  String latS, lonS;
  bool ok = parseClbs(clbs, latS, lonS);
  if (ok)
  {
    lteMsg = String("📍 *Vorläufiger Standort (LTE)*\n") +
             "🌐 [" + latS + ", " + lonS + "](https://maps.google.com/?q=" + latS + "," + lonS + ")";
  }
  else
  {
    lteMsg = "📡 LTE-Standort nicht verfügbar.";
  }
  sendMessage(lteMsg);

  sendAT("AT+CGACT=0,1", 7000);
  sendAT("AT+CGATT=0", 5000);
  sendAT("AT+CGNSSPWR=1", 4000);

  bool ready = false;
  for (int i = 0; i < 30; i++)
  {
    String rdy = sendAT("AT+CGNSSINFO", 2500);
    if (rdy.indexOf("+CGNSSINFO:") >= 0)
    {
      ready = true;
      break;
    }
    delay(3000);
  }
  if (!ready)
  {
    sendMessage("GNSS nicht erreichbar. Verwende LTE-Position.");
    sendAT("AT+CGNSSPWR=0", 2000);
    sendAT("AT+CGATT=1", 5000);
    sendAT("AT+CGACT=1,1", 10000);
    gpsBusy = false;
    return lteMsg;
  }
  GNSSInfo gnss;

  bool fix = false;
  for (int i = 0; i < GNNS_MAX_RETRIES; i++)
  {
    String resp = sendAT("AT+CGNSSINFO", 3000);
    if (parseCgnssInfo(resp, gnss) && gnss.valid)
    {
      fix = true;
      break;
    }
    delay(4000);
  }

  String msg;
  if (fix)
  {
    String slat = String(gnss.latDeg, 6);
    String slon = String(gnss.lonDeg, 6);

    msg = "📡 *GNSS Fix erhalten*\n";
    msg += "🧭 " + String(gnss.fixMode) + "D Fix | ";
    msg += String(gnss.satsUsed) + "/" + String(gnss.satsInView) + " Satelliten\n";
    msg += "📍 [" + slat + ", " + slon + "](https://maps.google.com/?q=" + slat + "," + slon + ")\n";
    msg += "⛰️ " + String(gnss.altitude, 1) + " m | ";
    msg += "🎯 " + String(gnss.hdop, 1) + " HDOP\n";
    msg += "🚗 " + String(gnss.speed, 1) + " km/h | ";
    msg += "🧭 Kurs " + String(gnss.course, 1) + "°\n";
    msg += "🕓 UTC: " + gnss.utcDate;
  }
  else
  {
    msg = "⚠️ Kein GNSS Fix nach Timeout.\nVerwende LTE-Daten.";
  }

  sendAT("AT+CGNSSPWR=0", 2000);
  sendAT("AT+CGATT=1", 5000);
  sendAT("AT+CGACT=1,1", 10000);

  sendMessage(msg);
  gpsBusy = false;
  return msg;
}
