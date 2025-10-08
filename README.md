# Winni GPS — ESP32-S3 (Waveshare) + A7670E — Modular PlatformIO Project (v2)

## Aktiv in dieser Version
- GNSS → LTE → HTTP Upload (Node-RED)
- Alle weiteren Manager vorhanden, aber **in main deaktiviert**

## Setup
1. Öffne den Ordner in VS Code (PlatformIO).
2. Prüfe `include/config.h` (Pins, APN, Server).
3. Build & Upload.
4. Serieller Monitor 115200 Baud.

## Deaktivierte Module (bereit, aber nicht genutzt)
- BatteryMonitor (MAX17048)
- MotionSensor (LIS3DH)
- StorageManager (TF-Card, SD_MMC)
- WebCamManager (esp32-camera)

## Hinweise
- In unseren Headers verwenden wir `"TinyGsmClient.h"` für robuste Includes.
- Die BatteryMonitor-Version nutzt keinen `initialized()`-Call mehr.