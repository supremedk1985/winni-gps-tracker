/**
 * Konfigurationsdatei für Winni-GPS-Tracker
 *
 * WICHTIG:
 * 1. Kopiere diese Datei nach "config.h"
 * 2. Füge deine echten Zugangsdaten ein.
 * 3. Ändere EXAMPLE_CONFIG_H zu CONFIG_H.
 */

#ifndef EXAMPLE_CONFIG_H
#define EXAMPLE_CONFIG_H

// === TELEGRAM BOT ===
// Hol dir deinen Token von @BotFather in Telegram
#define BOT_TOKEN "BOT_TOKEN_HERE"
#define CHAT_ID   "CHAT_ID_HERE"

// === SIM-KARTE / MOBILFUNK ===
// APN-Einstellungen für Deutschland:
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
