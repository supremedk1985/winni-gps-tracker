#include "gps.h"
#include "telegram_bot.h"
#include "storage.h"
#include <time.h>

// Globale Ressourcen
HardwareSerial gpsSerial(2);
TinyGPSPlus gps;

// GPS-Logging-Konfiguration
#define GPS_LOG_FILE "/gps_log.csv"
#define GPS_LOG_INTERVAL 10000  // 10 Sekunden zwischen Logs

unsigned long lastGPSLog = 0;

void initGPS()
{
  gpsSerial.begin(9600, SERIAL_8N1, 33, 21);
  Serial.println("GPS initialisiert");
  
  // CSV-Header erstellen, wenn Datei nicht existiert
  if (!SD_MMC.exists(GPS_LOG_FILE))
  {
    File f = SD_MMC.open(GPS_LOG_FILE, FILE_WRITE);
    if (f)
    {
      f.println("timestamp,date,time,lat,lon,altitude,speed,course,hdop,satellites");
      f.close();
      Serial.println("✅ GPS-Log-Datei mit Header erstellt");
    }
    else
    {
      Serial.println("❌ FEHLER: GPS-Log-Datei konnte nicht erstellt werden!");
    }
  }
  else
  {
    Serial.println("✅ GPS-Log-Datei existiert bereits");
  }
}

void updateGPS()
{
  while (gpsSerial.available() > 0)
    gps.encode(gpsSerial.read());
}

// GPS-Daten in CSV-Datei loggen (immer aktiv)
void logGPSData()
{
  if (millis() - lastGPSLog < GPS_LOG_INTERVAL)
    return;
    
  updateGPS();
  
  if (!gps.location.isValid())
  {
    // Nur alle 60 Sekunden Warnung ausgeben
    static unsigned long lastWarning = 0;
    if (millis() - lastWarning > 60000)
    {
      Serial.println("⚠️ Kein GPS-Fix - warte auf Signal...");
      lastWarning = millis();
    }
    return;
  }
    
  lastGPSLog = millis();
  
  File f = SD_MMC.open(GPS_LOG_FILE, FILE_APPEND);
  if (!f)
  {
    Serial.println("❌ FEHLER: GPS-Log-Datei kann nicht geöffnet werden!");
    Serial.print("   SD-Karte: ");
    Serial.println(SD_MMC.cardType() != CARD_NONE ? "OK" : "FEHLT");
    return;
  }
  
  // Timestamp (Sekunden seit Start)
  unsigned long timestamp = millis() / 1000;
  f.print(timestamp);
  f.print(",");
  
  // Datum
  char dateStr[16];
  snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d",
           gps.date.year(), gps.date.month(), gps.date.day());
  f.print(dateStr);
  f.print(",");
  
  // Zeit
  char timeStr[16];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
           gps.time.hour(), gps.time.minute(), gps.time.second());
  f.print(timeStr);
  f.print(",");
  
  // Position
  f.print(gps.location.lat(), 6);
  f.print(",");
  f.print(gps.location.lng(), 6);
  f.print(",");
  
  // Höhe
  f.print(gps.altitude.meters(), 1);
  f.print(",");
  
  // Geschwindigkeit (km/h)
  f.print(gps.speed.kmph(), 1);
  f.print(",");
  
  // Kurs
  f.print(gps.course.deg(), 1);
  f.print(",");
  
  // HDOP
  f.print(gps.hdop.hdop(), 2);
  f.print(",");
  
  // Satelliten
  f.println(gps.satellites.value());
  
  size_t fileSize = f.size();
  f.close();
  
  Serial.print("✅ GPS geloggt (");
  Serial.print(fileSize);
  Serial.print(" B | ");
  Serial.print(gps.location.lat(), 6);
  Serial.print(",");
  Serial.print(gps.location.lng(), 6);
  Serial.println(")");
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
  msg += "🕐 " + String(buf) + "\n";

  Serial.println(msg);
  sendMessage(msg);

  return msg;
}

// GPS-Track der letzten N Tage als GPX exportieren
bool exportTrackToGPX(int days, const char* outputFile)
{
  Serial.println("=== GPX Export Start ===");
  Serial.print("📤 Öffne CSV: ");
  Serial.println(GPS_LOG_FILE);
  
  File csvFile = SD_MMC.open(GPS_LOG_FILE, FILE_READ);
  if (!csvFile)
  {
    Serial.println("❌ GPS-Log-Datei nicht gefunden!");
    return false;
  }
  
  size_t csvSize = csvFile.size();
  Serial.print("   CSV-Größe: ");
  Serial.print(csvSize);
  Serial.println(" Bytes");
  
  if (csvSize < 100)
  {
    Serial.println("❌ CSV-Datei zu klein/leer!");
    csvFile.close();
    return false;
  }
  
  File gpxFile = SD_MMC.open(outputFile, FILE_WRITE);
  if (!gpxFile)
  {
    csvFile.close();
    Serial.println("❌ GPX nicht erstellbar!");
    return false;
  }
  
  // Zeitberechnung mit Underflow-Schutz
  unsigned long currentTime = millis() / 1000;
  unsigned long daysInSeconds = days * 24UL * 3600UL;
  unsigned long timeLimit = 0;
  
  Serial.print("   Timestamp: ");
  Serial.print(currentTime);
  Serial.println(" s");
  Serial.print("   Zeitraum: ");
  Serial.print(days);
  Serial.print(" Tage = ");
  Serial.print(daysInSeconds);
  Serial.println(" s");
  
  if (currentTime > daysInSeconds)
  {
    timeLimit = currentTime - daysInSeconds;
    Serial.print("   Filter: > ");
    Serial.println(timeLimit);
  }
  else
  {
    timeLimit = 0;
    Serial.println("   ✓ Nehme ALLE Daten");
  }
  
  // GPX-Header
  gpxFile.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
  gpxFile.println("<gpx version=\"1.1\" creator=\"Winni GPS Tracker\"");
  gpxFile.println("  xmlns=\"http://www.topografix.com/GPX/1/1\"");
  gpxFile.println("  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"");
  gpxFile.println("  xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">");
  gpxFile.println("  <trk>");
  gpxFile.println("    <n>GPS Track (letzte " + String(days) + " Tage)</n>");
  gpxFile.println("    <trkseg>");
  
  // Header überspringen
  String header = csvFile.readStringUntil('\n');
  Serial.print("   Header: ");
  Serial.println(header);
  
  int pointCount = 0;
  int skippedOld = 0;
  int skippedInvalid = 0;
  int lineNum = 1;
  
  while (csvFile.available())
  {
    String line = csvFile.readStringUntil('\n');
    lineNum++;
    
    if (line.length() < 10)
    {
      skippedInvalid++;
      continue;
    }
    
    // CSV parsen
    int idx1 = line.indexOf(',');
    int idx2 = line.indexOf(',', idx1 + 1);
    int idx3 = line.indexOf(',', idx2 + 1);
    int idx4 = line.indexOf(',', idx3 + 1);
    int idx5 = line.indexOf(',', idx4 + 1);
    int idx6 = line.indexOf(',', idx5 + 1);
    
    if (idx1 == -1 || idx2 == -1 || idx3 == -1 || idx4 == -1 || idx5 == -1 || idx6 == -1)
    {
      Serial.print("   ⚠️ Z.");
      Serial.print(lineNum);
      Serial.print(": ");
      Serial.println(line);
      skippedInvalid++;
      continue;
    }
    
    // Timestamp prüfen
    unsigned long timestamp = line.substring(0, idx1).toInt();
    
    if (timestamp < timeLimit)
    {
      skippedOld++;
      continue;
    }
    
    // Daten extrahieren
    String date = line.substring(idx1 + 1, idx2);
    String time = line.substring(idx2 + 1, idx3);
    String lat = line.substring(idx3 + 1, idx4);
    String lon = line.substring(idx4 + 1, idx5);
    String alt = line.substring(idx5 + 1, idx6);
    
    // GPX-Trackpoint
    gpxFile.println("      <trkpt lat=\"" + lat + "\" lon=\"" + lon + "\">");
    gpxFile.println("        <ele>" + alt + "</ele>");
    gpxFile.println("        <time>" + date + "T" + time + "Z</time>");
    gpxFile.println("      </trkpt>");
    
    pointCount++;
    
    // Erste 3 Punkte im Serial ausgeben
    if (pointCount <= 3)
    {
      Serial.print("   ✓ P");
      Serial.print(pointCount);
      Serial.print(": ");
      Serial.print(lat);
      Serial.print(",");
      Serial.print(lon);
      Serial.print(" @ ");
      Serial.println(timestamp);
    }
  }
  
  // GPX-Footer
  gpxFile.println("    </trkseg>");
  gpxFile.println("  </trk>");
  gpxFile.println("</gpx>");
  
  csvFile.close();
  gpxFile.close();
  
  Serial.println("=== Export-Statistik ===");
  Serial.print("✅ Exportiert: ");
  Serial.println(pointCount);
  Serial.print("   Zu alt: ");
  Serial.println(skippedOld);
  Serial.print("   Ungültig: ");
  Serial.println(skippedInvalid);
  Serial.println("========================");
  
  return pointCount > 0;
}
