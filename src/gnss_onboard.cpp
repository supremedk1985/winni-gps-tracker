#include "gnss_onboard.h"
#include "settings.h"
#include "config.h"
#include "utils.h"
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
  info = GNSSInfo();
  info.valid = false;

  int p = resp.indexOf("+CGNSSINFO:");
  if (p < 0)
    return false;

  String s = resp.substring(p + 12);
  s.replace("\r", "");
  s.replace("\n", "");
  s.trim();

  // Tokenize ohne STL
  const int MAXTOK = 48;
  String t[MAXTOK];
  int n = 0, start = 0;
  while (n < MAXTOK)
  {
    int c = s.indexOf(',', start);
    if (c < 0)
    {
      t[n++] = s.substring(start);
      break;
    }
    t[n++] = s.substring(start, c);
    start = c + 1;
  }
  if (n < 12)
    return false;

  // Anker finden
  int idxNS = -1, idxEW = -1;
  for (int i = 0; i < n; ++i)
  {
    if (t[i] == "N" || t[i] == "S")
      idxNS = i;
    if (t[i] == "E" || t[i] == "W")
    {
      idxEW = i;
      break;
    }
  }
  if (idxNS < 1 || idxEW < 1)
    return false;

  // Fix-Mode ist immer am Anfang
  int fixModeRaw = t[0].toInt(); // 2=2D, 3=3D (SIMCom)
  info.fixMode = fixModeRaw;

  // Satelliten-Zähler vor der Latitude aufsummieren (GPS, GLONASS, BEIDOU, evtl. QZSS)
  int satsInView = 0;
  for (int i = 1; i <= idxNS - 2; ++i)
  { // alle Felder zwischen mode und lat
    if (t[i].length())
      satsInView += t[i].toInt();
  }
  info.satsInView = satsInView;

  // Latitude/Longitude (dezimale Grad in deinen Beispielen)
  double lat = t[idxNS - 1].toDouble();
  double lon = t[idxEW - 1].toDouble();
  if (t[idxNS] == "S")
    lat = -lat;
  if (t[idxEW] == "W")
    lon = -lon;
  if (isnan(lat) || isnan(lon))
    return false;
  info.latDeg = lat;
  info.lonDeg = lon;

  // Felder nach E/W in fester Reihenfolge:
  // date, time, alt(m), speed(knots), course(deg), PDOP, HDOP, VDOP, [optional: satsUsed]
  int i = idxEW + 1;
  if (i < n)
    info.utcDate = t[i++]; // ddmmyy
  if (i < n && t[i].length())
  {
    info.utcDate += " " + t[i];
  } // hhmmss.s
  i++;

  if (i < n && t[i].length())
    info.altitude = t[i++].toDouble();
  if (i < n && t[i].length())
    info.speed = t[i++].toDouble(); // in Knoten
  if (i < n && t[i].length())
    info.course = t[i++].toDouble();
  if (i < n && t[i].length()) /* PDOP */
    i++;
  if (i < n && t[i].length())
    info.hdop = t[i++].toDouble();
  if (i < n && t[i].length()) /* VDOP */
    i++;

  // Satelliten USED: wenn letztes Feld numerisch klein ist, nutze es
  int satsUsed = 0;
  if (n > 0)
  {
    bool numeric = true;
    for (unsigned j = 0; j < t[n - 1].length(); ++j)
      if (!isDigit(t[n - 1][j]))
      {
        numeric = false;
        break;
      }
    if (numeric)
    {
      satsUsed = t[n - 1].toInt();
      if (satsUsed > 64)
        satsUsed = 0; // Plausibilitätsgrenze
    }
  }
  info.satsUsed = satsUsed;

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
    // nach erfolgreichem parse
    String slat = String(gnss.latDeg, 6);
    String slon = String(gnss.lonDeg, 6);

    // Fix-Text
    String fixTxt = (gnss.fixMode >= 3) ? "3D Fix" : (gnss.fixMode == 2 ? "2D Fix" : "kein Fix");

    // Geschwindigkeit in km/h (1 kn = 1.852 km/h)
    double kmh = gnss.speed * 1.852;

    String msg;
    msg = "📡 *GNSS Fix erhalten*\n";
    msg += "🧭 " + fixTxt + " | " + String(gnss.satsUsed) + "/" + String(gnss.satsInView) + " Satelliten\n";
    msg += "📍 [" + slat + ", " + slon + "](https://maps.google.com/?q=" + slat + "," + slon + ")\n";
    msg += "⛰️ " + String(gnss.altitude, 1) + " m | 🎯 " + String(gnss.hdop, 2) + " HDOP\n";
    msg += "🚗 " + String(kmh, 1) + " km/h | 🧭 Kurs " + String(gnss.course, 1) + "°\n";
    msg += "🕓 UTC: " + fmtUTC(gnss.utcDate);
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
