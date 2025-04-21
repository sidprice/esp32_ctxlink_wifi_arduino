/**
 * @file task_wifi.cpp
 * @author Sid Price (sid@sidprice.com)
 * @brief 
 * @version 0.1
 * @date 2025-04-20
 * 
 * @copyright Copyright Sid Price (c) 2025
 * 
 */

 #include <Arduino.h>
 #include "serial_control.h"
 #include <WiFi.h>

 #include "task_wifi.h"
 #include "tasks/task_server.h"

 // Wi-Fi credentials
// TODO These need to be set from ctxLink and saved in the preferences

const char* ssid = "ctxlink_net";
const char* password = "pass_phrase";

// const char* ssid = "Avian Ambassadors";
// const char* password = "mijo498rocks";

void deinitWiFi(void) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  } // deinitWiFi() end

  wl_status_t configWiFi(void) {
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
    MONITOR(print("Hostname = ")) ; Serial.println(WiFi.getHostname()) ;
    MONITOR(print("Connecting to WiFi .."));

    uint32_t retry = 5 ;
    
    wifi_status = WiFi.begin(ssid, password) ;
    do {
        wifi_status = WiFi.status() ;
        if (wifi_status == WL_CONNECTED) {
            break;
        }
        delay(1000);
        MONITOR(print("."));
    } while(retry--);
    MONITOR(println()) ;
    if ( wifi_status == WL_CONNECTED) {
        MONITOR(println(WiFi.localIP()));
        MONITOR(print("RSSI: "));
        MONITOR(println(WiFi.RSSI()));
    } else {
        MONITOR(print("Wi-Fi status = ")); MONITOR(println(wifi_status)) ;
        MONITOR(println("Failed to connect to Wi-Fi"));
    }
    return wifi_status ;
} // configWiFi() end

 void task_wifi(void *pvParameters) {
    (void)pvParameters; // Unused parameter
    TaskHandle_t xHandle = NULL;
    configWiFi() ; // Attempt to connect to Wi-Fi
    while(1) {
        static wl_status_t previous_status = WL_NO_SHIELD ;
        wl_status_t wifi_status = WiFi.status() ;
        //
        // Has the wifi status changed?
        //
        if (wifi_status != previous_status) {
            MONITOR(print("Wi-Fi status = ")); MONITOR(println(wifi_status)) ;
            previous_status = wifi_status ;
            if ( wifi_status != WL_CONNECTED) {
                //
                // Delete the GDB server task if it exists
                //
                if (xHandle != NULL) {
                    vTaskDelete(xHandle) ;
                    xHandle = NULL;
                }
                configWiFi() ; // Attempt to reconnect to Wi-Fi
            } else {
                MONITOR(println("Wi-Fi connected")) ;
                xTaskCreate(task_wifi_server, "GDB Server", 4096, (void *)2159, 1, &xHandle) ;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10000)); // Delay for 10 seconds before checking again
    }
 }