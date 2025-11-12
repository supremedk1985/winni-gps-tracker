#include "traccar.h"
#include "utils.h"
#include "modem.h"
#include "gps.h"

void send2Traccar()
{
  updateGPS();

  String slat = String(gps.location.lat(), 6);
  String slon = String(gps.location.lng(), 6);
  String sspeed = String(gps.speed.kmph(), 1);
  String sbearing = String(gps.course.deg(), 1);
  String salt = String(gps.altitude.meters(), 1);
  String stime = String(gps.time.value());  // oder millis()/1000, falls kein Fix

  String url = "http://supremedk1.synology.me:1881/"
               "?id=winni-gps-lea-nina"
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
