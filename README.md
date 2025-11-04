# Winni GPS — ESP32-S3 (Waveshare) + A7670E — Modular PlatformIO Project 
WIP

Inhalt config_h:
<code>
**
 * Konfigurationsdatei für Winni-GPS-Tracker
 * 
 * WICHTIG: Kopiere diese Datei nach "config.h" und füge deine 
 * echten Zugangsdaten ein. (Änderer EXAMPLE_CONFIG_H in CONFIG_H)
 */

#ifndef EXAMPLE_CONFIG_H
#define EXAMPLE_CONFIG_H

// === TELEGRAM BOT ===
// Hol dir deinen Token von @BotFather in Telegram
#define BOT_TOKEN "BOT_TOKEN_HERE"
#define CHAT_ID  "CHAT_ID_HERE"

// === SIM KARTE / MOBILFUNK ===
// APN Einstellungen für Deutschland:
// O2: "internet"
// Vodafone: "web.vodafone.de"
// Telekom: "internet.t-mobile"
#define APN      ""
#define APN_USER ""
#define APN_PASS ""

// === DEBUG ===
#define DEBUG_MODE        true   // Aktiviert Debug-Ausgaben
#define DEBUG_AT_COMMANDS false  // Zeigt AT-Kommandos im Serial Monitor

#endif // CONFIG_H
</code>