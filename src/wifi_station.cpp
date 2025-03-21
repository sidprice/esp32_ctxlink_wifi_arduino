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
#include "arduino-timer.h"

// Wi-Fi credentials
// TODO These need to be set from ctxLink and saved in the preferences

const char* ssid = "ctxlink_net";
const char* password = "pass_phrase";

//
// Timer used to monitor Wi-Fi connection
//
auto wifi_timer = timer_create_default() ;

void deinitWiFi(void) {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
} // deinitWiFi() end

void configWiFi(void) {
  wl_status_t wifi_status ;
  MONITOR(println("Configuring Wi-Fi")) ;
  deinitWiFi() ;
  delay(100);
  //
  // Set up the Wi-Fi Station
  //
  // TODO Set the hostname to something unique
  //
  WiFi.setHostname("ctxLink_adapter_1") ;
  MONITOR(println("Hostname = ")) ; Serial.println(WiFi.getHostname()) ;
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
} // configWiFi() end

//
// Check the Wi-Fi connection status
//
bool checkWiFi(void *) {
  wl_status_t wifi_status = WiFi.status() ;
  if (wifi_status != WL_CONNECTED) {
    MONITOR(println("Wi-Fi disconnected")) ;
    configWiFi() ;
  }
  return true;
}

void timerKick(void) {
  wifi_timer.tick() ;
} // timerKick() end

//
// Set up the Wi-Fi connection and monitor the status
//
void initWiFi(void) {

    wifi_timer.every(1000, checkWiFi);
    while(WiFi.status() != WL_CONNECTED)
      timerKick() ;
} // initWiFi() end