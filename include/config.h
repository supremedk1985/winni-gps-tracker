#pragma once

#define TINY_GSM_MODEM_SIM7600

static const char APN[]    = "internet";
static const char USER[]   = "";
static const char PASS[]   = "";
// ACHTUNG: Diese vier Werte werden im ganzen Projekt NICHT MEHR benutzt (Stand
// 2026-09-08 nachgeprüft). Sie stammen aus einem früheren Uplink-Versuch. Der
// Server für Traccar wird in der eigenen src/config.h gesetzt, siehe
// src/example_config.h. Hier stand bis zum 08.09.2026 ein echter DynDNS-Name --
// solche Werte gehören nicht in ein öffentliches Repository.
static const char SERVER[] = "dein-server.example.net";
static const int  PORT     = 1880;
static const char PATH[]   = "/gps";
static const char TOKEN[]  = "DEIN_TOKEN";

#define MODEM_PWR_PIN 33
#define MODEM_RX 17
#define MODEM_TX 18

#define I2C_SDA 10
#define I2C_SCL 11

#define CAM_PIN_PWDN    -1
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK    15
#define CAM_PIN_SIOD    4
#define CAM_PIN_SIOC    5
#define CAM_PIN_D7      16
#define CAM_PIN_D6      17
#define CAM_PIN_D5      18
#define CAM_PIN_D4      12
#define CAM_PIN_D3      10
#define CAM_PIN_D2      8
#define CAM_PIN_D1      9
#define CAM_PIN_D0      11
#define CAM_PIN_VSYNC   6
#define CAM_PIN_HREF    7
#define CAM_PIN_PCLK    13
#define CAM_XCLK_FREQ   20000000

#define USE_SD_MMC 1
#define SD_CS   5
#define SD_SCLK 14
#define SD_MISO 12
#define SD_MOSI 13