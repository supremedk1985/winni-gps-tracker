#include "gps.h"
#include "telegram_bot.h"

// Globale Ressourcen
HardwareSerial gpsSerial(2);
TinyGPSPlus gps;

void initGPS()
{
  gpsSerial.begin(9600, SERIAL_8N1, 33,21);
  Serial.println("GPS initialisiert");
}

// ----------------------------------------------------------------------

void updateGPS()
{
  while (gpsSerial.available() > 0)
    gps.encode(gpsSerial.read());
}

String getGPSLocation()
{
  Serial.println("Suche GNSS-Fix...");

  updateGPS();

  if (!(gps.location.isValid() && gps.location.isUpdated()))
  {
    Serial.println("❌ Kein GNSS-Fix vorhanden");
    sendMessage("❌ Kein GNSS-Fix vorhanden");

    return "";
  }

  String slat = String(gps.location.lat(), 6);
  String slon = String(gps.location.lng(), 6);
  String fixTxt = "kein Fix";
  if (gps.hdop.isValid())
  {
    if (gps.hdop.hdop() <= 1.5)
      fixTxt = "3D Fix";
    else if (gps.hdop.hdop() <= 3.0)
      fixTxt = "2D Fix";
  }

  double kmh = gps.speed.kmph();
  double hdop = gps.hdop.hdop();
  double altitude = gps.altitude.meters();
  double course = gps.course.deg();
  int sats = gps.satellites.value();

  char buf[64];
  snprintf(buf, sizeof(buf), "%02d.%02d.%04d %02d:%02d:%02d UTC",
           gps.date.day(), gps.date.month(), gps.date.year(),
           gps.time.hour(), gps.time.minute(), gps.time.second());

  String msg;
  msg += "📡 *GNSS Fix erhalten*\n";
  msg += "🧭 " + fixTxt + " | " + String(sats) + " Satelliten\n";
  msg += "📍 [" + slat + ", " + slon + "](https://maps.google.com/?q=" + slat + "," + slon + ")\n";
  msg += "⛰️ " + String(altitude, 1) + " m | 🎯 " + String(hdop, 2) + " HDOP\n";
  msg += "🚗 " + String(kmh, 1) + " km/h | 🧭 Kurs " + String(course, 1) + "°\n";
  msg += "🕓 " + String(buf) + "\n";

  Serial.println(msg);

  sendMessage(msg);

  return msg;
}
