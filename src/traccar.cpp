#include "traccar.h"
#include "utils.h"
#include "modem.h"
#include "gps.h"
#include "config.h"

void send2Traccar()
{
  updateGPS();

  if (!gps.location.isValid()) {
    Serial.println("Kein gültiger GPS-Fix");
    return;
  }

  String slat = String(gps.location.lat(), 6);
  String slon = String(gps.location.lng(), 6);
  String sspeed = String(gps.speed.kmph(), 1);
  String sbearing = String(gps.course.deg(), 1);
  String salt = String(gps.altitude.meters(), 1);

  // Unix-Zeit aus GPS, wenn verfügbar
  unsigned long ts = gps.time.isValid() ? gps.time.value() : millis() / 1000;
  String stime = String(ts);

  // Adresse und Gerätekennung stehen in der EIGENEN config.h (nicht im Repo,
  // siehe example_config.h). Die Kennung ist beim osmand-Protokoll das einzige
  // Merkmal, an dem der Server das Fahrzeug erkennt -- wer sie kennt, kann
  // beliebige Positionen einspeisen. Deshalb gehört sie nicht in den Quelltext.
  String url = String(TRACCAR_SCHEME) + "://" + TRACCAR_HOST + ":" + String(TRACCAR_PORT) + "/"
               "?id=" + TRACCAR_DEVICE_ID +
               "&lat=" + slat +
               "&lon=" + slon +
               "&speed=" + sspeed +
               "&bearing=" + sbearing +
               "&altitude=" + salt +
               "&timestamp=" + stime;

  Serial.println("Sende an Traccar:");
  Serial.println(url);

  sendAT("AT+HTTPTERM", 1500);
  sendAT("AT+HTTPINIT", 3000);
  httpGet(url);
}
