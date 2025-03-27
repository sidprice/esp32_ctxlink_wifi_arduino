/**
 * @file main.cpp
 * @author Sid Price (sid@sidprice.com)
 * @brief Main module for ctxLink ESP32 Wi-Fi adapter.
 * @version 0.1
 * @date 2025-03-17
 * 
 * @copyright Copyright, Sid Price (c) 2025
 * 
 */

#include <Arduino.h>

#include "serial_control.h"
#include "wifi_station.h"
#include "ota.h"
#include "ctxlink.h"
#include "tasks/task_server.h"

void setup() {
  MONITOR(begin(115200));

#ifdef SERIAL_ON
  while (!Serial)
    delay(10);     // will pause Zero, Leonardo, etc until serial console opens
#endif
  MONITOR(println("ctxLink ESP32 WiFi adapter")) ;
  //
  // Set up Wi-Fi connection and monitor status
  //
  initWiFi() ;
  ota_setup() ;
  initCtxLink() ;
  //
  // Create the GDB Server task
  //
  xTaskCreate(task_wifi_server, "GDB Server", 4096, (void *)2159, 1, NULL) ;
  //
  // Create the SPI communications task
  //
  // xTaskCreate(task_spi_comms, "SPI Comms", 4096, NULL, 1, NULL) ;
}

void loop() {
  timerKick() ;
  otaKick() ;
  delay(1000) ;
}
