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
#include "ota.h"
#include "ctxlink.h"
#include "tasks/task_spi_comms.h"
#include "tasks/task_wifi.h"

void setup() {
//   MONITOR(begin(115200));

// #ifdef SERIAL_ON
//   while (!Serial)
//     delay(10);     // will pause Zero, Leonardo, etc until serial console opens
// #endif
  Serial.begin(115200);

  while (!Serial)
    delay(10);     // will pause Zero, Leonardo, etc until serial console opens

  MONITOR(println("ctxLink ESP32 WiFi adapter")) ;
  //
  // Set up the SPI hardware for ctxLink communication
  //
  initCtxLink() ;
  //
  // Create the SPI communications task
  //
  xTaskCreate(task_spi_comms, "SPI Comms", 4096, NULL, 1, NULL) ;
  //
  // Set up Wi-Fi connection and monitor status
  //
  xTaskCreate(task_wifi, "Wi-Fi", 4096, NULL, 1, NULL) ;
}

void loop() {
  delay(1000) ;
}
