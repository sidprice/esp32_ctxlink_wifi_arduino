/**
 * @file ota_.cpp
 * @author Sid Price (sid@sidprice.com)
 * @brief ESP32 Over-The_Air firmware update interface code
 * @version 0.1
 * @date 2024-05-05
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <Arduino.h>
#include "ArduinoOTA.h"

#include "serial_control.h"

void ota_setup(void) {
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // TODO Set the hostname to something unique

  // ArduinoOTA.setHostname("ctxlink_adapter_1");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {  // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      MONITOR(println("Start updating " + type));
    })
    .onEnd([]() {
      MONITOR(println("\nEnd"));
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      MONITOR(printf("Progress: %u%%\r", (progress / (total / 100))));
    })
    .onError([](ota_error_t error) {
      MONITOR(printf("Error[%u]: ", error));
      if (error == OTA_AUTH_ERROR) {
        MONITOR(println("Auth Failed"));
      } else if (error == OTA_BEGIN_ERROR) {
        MONITOR(println("Begin Failed"));
      } else if (error == OTA_CONNECT_ERROR) {
        MONITOR(println("Connect Failed"));
      } else if (error == OTA_RECEIVE_ERROR) {
        MONITOR(println("Receive Failed"));
      } else if (error == OTA_END_ERROR) {
        MONITOR(println("End Failed"));
      }
    });

  ArduinoOTA.begin();
}

void otaKick(void) {
  ArduinoOTA.handle();
} // otaKick() end
