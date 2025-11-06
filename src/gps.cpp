#include "gps.h"
#include "telegram_bot.h"
#include "storage.h"
#include <math.h>

// Globale Ressourcen
HardwareSerial gpsSerial(2);
TinyGPSPlus gps;

// Track Recording Variablen
static bool trackingActive = false;
static double lastRecordedLat = 0.0;
static double lastRecordedLon = 0.0;
static bool firstPoint = true;

// Distanz in Metern zwischen zwei GPS-Punkten berechnen (Haversine Formel)
double calculateDistance(double lat1, double lon1, double lat2, double lon2)
{
    const double R = 6371000.0; // Erdradius in Metern
    
    double lat1Rad = lat1 * PI / 180.0;
    double lat2Rad = lat2 * PI / 180.0;
    double deltaLat = (lat2 - lat1) * PI / 180.0;
    double deltaLon = (lon2 - lon1) * PI / 180.0;
    
    double a = sin(deltaLat/2.0) * sin(deltaLat/2.0) +
               cos(lat1Rad) * cos(lat2Rad) *
               sin(deltaLon/2.0) * sin(deltaLon/2.0);
    
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0-a));
    
    return R * c; // Distanz in Metern
}

void initGPS()
{
    gpsSerial.begin(9600, SERIAL_8N1, 33, 21);
    Serial.println("GPS initialisiert");
}

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

void startGPSTracking()
{
    if (trackingActive) {
        Serial.println("Tracking already active");
        sendMessage("GPS-Tracking läuft bereits!");
        return;
    }
    
    if (createGPSTrackFile()) {
        trackingActive = true;
        firstPoint = true;
        Serial.println("GPS Tracking gestartet - Aufzeichnung alle 5m");
        sendMessage("GPS-Tracking gestartet! Aufzeichnung alle 5 Meter auf SD-Karte.\nDatei: " + getCurrentTrackFilename());
    } else {
        Serial.println("Failed to start GPS tracking");
        sendMessage("Fehler: GPS-Tracking konnte nicht gestartet werden. SD-Karte prüfen!");
    }
}

void stopGPSTracking()
{
    if (!trackingActive) {
        Serial.println("Tracking not active");
        sendMessage("GPS-Tracking ist nicht aktiv.");
        return;
    }
    
    trackingActive = false;
    String filename = getCurrentTrackFilename();
    Serial.println("GPS Tracking gestoppt");
    sendMessage("GPS-Tracking gestoppt.\nDatei gespeichert: " + filename);
}

void processGPSTracking()
{
    if (!trackingActive) {
        return;
    }
    
    // GPS Daten aktualisieren
    updateGPS();
    
    // Prüfen ob GPS-Fix vorhanden
    if (!gps.location.isValid() || !gps.location.isUpdated()) {
        return;
    }
    
    double currentLat = gps.location.lat();
    double currentLon = gps.location.lng();
    
    // Erster Punkt - sofort speichern
    if (firstPoint) {
        // Zeitstempel erstellen
        char timestamp[32];
        snprintf(timestamp, sizeof(timestamp), "%02d.%02d.%04d %02d:%02d:%02d",
                 gps.date.day(), gps.date.month(), gps.date.year(),
                 gps.time.hour(), gps.time.minute(), gps.time.second());
        
        double speed_kmh = gps.speed.isValid() ? gps.speed.kmph() : 0.0;
        double altitude_m = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;
        
        appendGPSData(currentLat, currentLon, String(timestamp), speed_kmh, altitude_m);
        
        lastRecordedLat = currentLat;
        lastRecordedLon = currentLon;
        firstPoint = false;
        
        Serial.println("Erster GPS-Punkt aufgezeichnet");
        return;
    }
    
    // Distanz zum letzten gespeicherten Punkt berechnen
    double distance = calculateDistance(lastRecordedLat, lastRecordedLon, currentLat, currentLon);
    
    // Wenn >= 5 Meter zurückgelegt, neuen Punkt speichern
    if (distance >= 5.0) {
        // Zeitstempel erstellen
        char timestamp[32];
        snprintf(timestamp, sizeof(timestamp), "%02d.%02d.%04d %02d:%02d:%02d",
                 gps.date.day(), gps.date.month(), gps.date.year(),
                 gps.time.hour(), gps.time.minute(), gps.time.second());
        
        double speed_kmh = gps.speed.isValid() ? gps.speed.kmph() : 0.0;
        double altitude_m = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;
        
        if (appendGPSData(currentLat, currentLon, String(timestamp), speed_kmh, altitude_m)) {
            lastRecordedLat = currentLat;
            lastRecordedLon = currentLon;
            Serial.printf("GPS-Punkt aufgezeichnet (Distanz: %.2f m)\n", distance);
        }
    }
}

bool isGPSTracking()
{
    return trackingActive;
}
