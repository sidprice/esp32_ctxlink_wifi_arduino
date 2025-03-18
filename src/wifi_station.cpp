/**
 * @file wifi_station.cpp
 * @author Sid Price (sid@sidprice.com)
 * @brief Wi-Fi Station for ctxLink Wi-Fi adapter
 * @version 0.1
 * @date 2025-03-17
 * 
 * @copyright Copyright Sid Price (c) 2025
 * 
 */

#include <Arduino.h>
#include <WiFi.h>

#include "serial_control.h"
#include "wifi_station.h"

// Wi-Fi credentials
// TODO These need to be set from ctxLink and saved in the preferences

const char* ssid = "Avian Ambassadors";
const char* password = "mijo498rocks";

void initWiFi(void) {
  wl_status_t wifi_status ;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  //
  // Set up the Wi-Fi Station
  //

  // TODO Set the hostname to something unique
  //
  //WiFi.setHostname("ctxLink_adapter_1") ;
  //Serial.print("Hostname = ") ; Serial.println(WiFi.getHostname()) ;
  wifi_status = WiFi.begin(ssid, password);
  MONITOR(print("Wi-Fi begin = ")); MONITOR(println(wifi_status)) ;

  MONITOR(print("Connecting to WiFi .."));

  while ((wifi_status = WiFi.status()) != WL_CONNECTED) {
    MONITOR(print("Wi-Fi status = ")); MONITOR(println(wifi_status)) ;
    delay(5000);
  }
  MONITOR(println(WiFi.localIP()));

  MONITOR(print("RSSI: "));
  MONITOR(println(WiFi.RSSI()));
} // initWiFi() end