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

void setup() {
  MONITOR(begin(115200));

#ifdef SERIAL_ON
  while (!Serial)
    delay(10);     // will pause Zero, Leonardo, etc until serial console opens
#endif
// //while(1) {
//   delay(2000) ;
  MONITOR(println("ctxLink ESP32 WiFi adapter")) ;
//}

  initWiFi() ;
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000) ;
}
