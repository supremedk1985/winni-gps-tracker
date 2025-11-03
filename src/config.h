/**
 * Konfigurationsdatei für ESP32 LTE Telegram Bot
 * 
 * WICHTIG: Kopiere diese Datei nach "config.h" und füge deine 
 * echten Zugangsdaten ein. Die config.h sollte NICHT committed werden!
 */

#ifndef CONFIG_H
#define CONFIG_H

// === TELEGRAM BOT ===
// Hol dir deinen Token von @BotFather in Telegram
#define BOT_TOKEN "8432305875:AAHc0YuvoORlJRriOhcV16yOSymZtxvOSuc"
#define CHAT_ID  "1800609959"

#define GNNS_MAX_RETRIES 30  // Max. Versuche für GNSS Fix

// === SIM KARTE / MOBILFUNK ===
// APN Einstellungen für Deutschland:
// O2: "internet"
// Vodafone: "web.vodafone.de"
// Telekom: "internet.t-mobile"
#define APN      "internet"
#define APN_USER ""
#define APN_PASS ""

// === PINS (Standard für Waveshare Board) ===
#define RGB_PIN     38
#define PWRKEY_PIN  33
#define AT_TX_PIN   17
#define AT_RX_PIN   18
#define I2C_SDA_PIN 3
#define I2C_SCL_PIN 2

// === KAMERA PINS (falls verwendet) ===
#define CAM_PWDN    -1
#define CAM_RESET   19
#define CAM_XCLK    14
#define CAM_SIOD    4
#define CAM_SIOC    5

#define CAM_Y9      36
#define CAM_Y8      37
#define CAM_Y7      38
#define CAM_Y6      39
#define CAM_Y5      35
#define CAM_Y4      14
#define CAM_Y3      13
#define CAM_Y2      34
#define CAM_VSYNC   6
#define CAM_HREF    7
#define CAM_PCLK    8

// === WEITERE OPTIONEN ===
#define CHECK_INTERVAL    2000   // Telegram Check Intervall in ms
#define LED_BRIGHTNESS    30     // RGB LED Helligkeit (0-255)
#define SERIAL_BAUD       115200 // Serial Monitor Baudrate
#define AT_BAUD           115200 // Modem UART Baudrate

// === DEBUG ===
#define DEBUG_MODE        true   // Aktiviert Debug-Ausgaben
#define DEBUG_AT_COMMANDS false  // Zeigt AT-Kommandos im Serial Monitor

#endif // CONFIG_H
