#include "WebCamManager.h"
#include "config.h"
#include <esp_camera.h>
#include <FS.h>
#if USE_SD_MMC
#include <SD_MMC.h>
#else
#include <SD.h>
#include <SPI.h>
#endif

bool WebCamManager::begin(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = CAM_PIN_D0;
  config.pin_d1       = CAM_PIN_D1;
  config.pin_d2       = CAM_PIN_D2;
  config.pin_d3       = CAM_PIN_D3;
  config.pin_d4       = CAM_PIN_D4;
  config.pin_d5       = CAM_PIN_D5;
  config.pin_d6       = CAM_PIN_D6;
  config.pin_d7       = CAM_PIN_D7;
  config.pin_xclk     = CAM_PIN_XCLK;
  config.pin_pclk     = CAM_PIN_PCLK;
  config.pin_vsync    = CAM_PIN_VSYNC;
  config.pin_href     = CAM_PIN_HREF;
  config.pin_sscb_sda = CAM_PIN_SIOD;
  config.pin_sscb_scl = CAM_PIN_SIOC;
  config.pin_pwdn     = CAM_PIN_PWDN;
  config.pin_reset    = CAM_PIN_RESET;
  config.xclk_freq_hz = CAM_XCLK_FREQ;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_SVGA;
  config.jpeg_quality = 12;
  config.fb_count     = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("⚠️ Kamera init fehlgeschlagen: 0x%X\n", err);
    ready = false;
    return false;
  }
  Serial.println("✅ Kamera bereit.");
  ready = true;
  return true;
}

bool WebCamManager::captureToFile(const String &path){
  if(!ready){
    Serial.println("⚠️ Kamera nicht bereit.");
    return false;
  }
  camera_fb_t *fb = esp_camera_fb_get();
  if(!fb){
    Serial.println("⚠️ Kamera Framebuffer null.");
    return false;
  }

#if USE_SD_MMC
  fs::FS &fs = SD_MMC;
#else
  fs::FS &fs = SD;
#endif

  File file = fs.open(path.c_str(), FILE_WRITE);
  if(!file){
    Serial.println("⚠️ Datei kann nicht geöffnet werden.");
    esp_camera_fb_return(fb);
    return false;
  }
  file.write(fb->buf, fb->len);
  file.close();
  esp_camera_fb_return(fb);
  Serial.printf("📸 Bild gespeichert: %s (%d bytes)\n", path.c_str(), fb->len);
  return true;
}