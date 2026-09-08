<center><img src="docs/images/logo.png" style="width:110px" /></center>

# Winni GPS Tracker — ESP32-S3 (Waveshare) + A7670E

**Modulares PlatformIO-Projekt für Wohnmobile, Wohnwagen und Fahrzeuge ohne Abo**

**☕ Wenn dir das Projekt gefällt, spendiere mir einen Kaffee: [PayPal](https://www.paypal.me/supremeDK1)**


Der Winni GPS Tracker ist ein eigenständiger, Cloud-freier Fahrzeug-Tracker auf Basis des **ESP32-S3-A7670E-4G** Moduls.  
Er nutzt eine **Daten-SIM-Karte** (keine SMS notwendig) und sendet Positionsdaten über das Mobilfunknetz an einen **Telegram-Bot**.  
Optional kann eine **18650 Li-Ion-Batterie** angeschlossen werden, um das System autark zu betreiben.

Benötigt wird eine Telegram Account um die Verbindung zum Winni GPS Tracker herzustellen.

---

## ⚙️ Hardwareanforderungen

<img src="docs/images/esp32s3a7670e4g_waveshare.png" style="width:50%" />


- **ESP32-S3-A7670E-4G Modul (Waveshare)** (https://www.waveshare.com/wiki/ESP32-S3-A7670E-4G)  
- **Nano-SIM Karte mit Datentarif**  
- **Optional:**  
  - 18650 Li-Ion-Akku  
  - Externes **NEO-6M GPS-Modul** (empfohlen)  
    > Das interne GNSS-Modul teilt sich die Leitung mit dem LTE-Modem.  
    > Daher ist gleichzeitiges Empfangen von GPS-Daten und Telegram-Kommunikation nicht möglich.

---

## 📍 Traccar (optional)

Zusätzlich zu Telegram kann der Tracker seine Position an einen eigenen
[Traccar](https://www.traccar.org/)-Server melden (osmand-Protokoll).

Server, Port und Gerätekennung stehen in der eigenen `src/config.h` — sie sind
**nicht** Teil dieses Repositories. Die Vorlage mit Erklärung steht in
`src/example_config.h`.

> **Empfehlung: HTTPS.** Beim osmand-Protokoll gehen Standort und Gerätekennung
> im Klartext durchs Netz, und die Kennung ist das einzige Merkmal, an dem der
> Server das Fahrzeug erkennt. Ein Reverse Proxy mit TLS davor (Caddy, nginx)
> kostet wenig und verhindert Mitlesen und Einspeisen falscher Positionen.
> Das Modem kann TLS, `AT+HTTPSSL=1` setzt `httpGet()` bereits.

## 💬 Telegram-Befehle

| Befehl | Beschreibung |
|:-------|:--------------|
| `/info` | Zeigt die aktuelle Signalstärke der Mobilfunkverbindung. |
| `/gps`  | Ermittelt zunächst eine ungefähre Position über LTE und versucht anschließend, einen GNSS-Fix zu erhalten. |

---


## 🧩 Geplante Erweiterungen (TODO)

- Geofencing (Benachrichtigung bei Verlassen eines definierten Bereichs)  
- Speicherung von Standortdaten auf SD-Karte  
- Aufnahme und Versand eines Kamerabilds  

---

## 🔧 Konfiguration

Vor der ersten Nutzung muss die Datei **`example_config.h`** kopiert und angepasst werden.

1. Kopiere:
   ```bash
   cp src/example_config.h src/config.h


## Lizenz

Dieses Projekt steht unter der [MIT-Lizenz](./LICENSE).  
Copyright (c) 2025 Daniel Korte.