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

// === TRACCAR (GPS-Server) ===
// Wohin die Positionen gemeldet werden. Traccar nimmt sie über das
// osmand-Protokoll entgegen (Standard-Port 5055, unverschlüsselt).
//
// EMPFOHLEN IST HTTPS über einen Reverse Proxy (z. B. Caddy oder nginx) davor:
// Über das osmand-Protokoll gehen Standort UND Gerätekennung im Klartext durchs
// Netz, und die Kennung ist das einzige Merkmal, an dem der Server das Fahrzeug
// erkennt. Wer im selben Mobilfunk- oder WLAN mitliest, kennt danach beides und
// kann falsche Positionen einspeisen. Das Modem beherrscht TLS (AT+HTTPSSL=1
// steht in httpGet), es genügt also "https" und der Port des Proxys.
//
// TRACCAR_SCHEME:    "https" (empfohlen) oder "http"
// TRACCAR_HOST:      Name des Servers, kein DynDNS-Name eines Geräts, das
//                    abgeschaltet werden könnte -- sonst zeigt er irgendwann
//                    auf einen fremden Anschluss, und die Meldungen laufen
//                    unbemerkt ins Leere.
// TRACCAR_PORT:      Port des Proxys (bei https) oder 5055 (bei http)
// TRACCAR_DEVICE_ID: die in Traccar angelegte Gerätekennung
#define TRACCAR_SCHEME    "https"
#define TRACCAR_HOST      "dein-server.example.net"
#define TRACCAR_PORT      5443
#define TRACCAR_DEVICE_ID "DEINE_GERAETEKENNUNG"

// === DEBUG ===
#define DEBUG_MODE        true   // Aktiviert Debug-Ausgaben
#define DEBUG_AT_COMMANDS false  // Zeigt AT-Kommandos im Serial Monitor

#endif // CONFIG_H
