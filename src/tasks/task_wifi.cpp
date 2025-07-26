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

#include "protocol.h"

#include "task_wifi.h"
#include "tasks/task_server.h"
#include "task_spi_comms.h"

#include "ctxlink.h"
// Wi-Fi credentials
// TODO These need to be set from ctxLink and saved in the preferences

const char *ssid = "ctxlink_net";
const char *password = "pass_phrase";

/**
 * @brief Structure to hold the information about the current network connection
 *
 */
static network_connection_info_s network_info;

// const char* ssid = "Avian Ambassadors";
// const char* password = "mijo498rocks";

void deinitWiFi(void)
{
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
} // deinitWiFi() end

wl_status_t configWiFi(void)
{
    wl_status_t wifi_status;
    MONITOR(println("Configuring Wi-Fi"));
    deinitWiFi();
    delay(100);
    //
    // Set up the Wi-Fi Station
    //
    // TODO Set the hostname to something unique
    //
    WiFi.setHostname("ctxLink_adapter_1");
    MONITOR(print("Hostname = "));
    Serial.println(WiFi.getHostname());
    MONITOR(print("Connecting to WiFi .."));

    uint32_t retry = 5;

    wifi_status = WiFi.begin(ssid, password);
    do
    {
        wifi_status = WiFi.status();
        if (wifi_status == WL_CONNECTED)
        {
            break;
        }
        delay(1000);
        MONITOR(print("."));
    } while (retry--);
    MONITOR(println());
    if (wifi_status == WL_CONNECTED)
    {
        MONITOR(println(WiFi.localIP()));
        MONITOR(print("RSSI: "));
        MONITOR(println(WiFi.RSSI()));
    }
    else
    {
        MONITOR(print("Wi-Fi status = "));
        MONITOR(println(wifi_status));
        MONITOR(println("Failed to connect to Wi-Fi"));
    }
    return wifi_status;
} // configWiFi() end

void task_wifi(void *pvParameters)
{
    (void)pvParameters; // Unused parameter
    TaskHandle_t xHandle = NULL;
    configWiFi(); // Attempt to connect to Wi-Fi
    while (1)
    {
        static wl_status_t previous_status = WL_NO_SHIELD;
        wl_status_t wifi_status = WiFi.status();
        //
        // Has the wifi status changed?
        //
        if (wifi_status != previous_status)
        {
            MONITOR(print("Wi-Fi status = "));
            MONITOR(println(wifi_status));
            previous_status = wifi_status;
            if (wifi_status != WL_CONNECTED)
            {
                // TODO Tell ctxLink network connection lost
                //
                // Delete the GDB server task if it exists
                //
                if (xHandle != NULL)
                {
                    vTaskDelete(xHandle);
                    xHandle = NULL;
                }
                configWiFi(); // Attempt to reconnect to Wi-Fi
            }
            else
            {
                MONITOR(println("Wi-Fi connected"));
                //
                // Update the current network information structure
                //
                memset(&network_info, 0, sizeof(network_connection_info_s));
                strncpy(network_info.network_ssid, ssid, MAX_SSID_LENGTH);
                network_info.type = PROTOCOL_PACKET_STATUS_TYPE_NETWORK_CLIENT;
                network_info.connected = 0x01; // 0x01 = connected, 0x00 = disconnected
                network_info.ip_address[0] = (uint8_t)(WiFi.localIP()[0]);
                network_info.ip_address[1] = (uint8_t)(WiFi.localIP()[1]);
                network_info.ip_address[2] = (uint8_t)(WiFi.localIP()[2]);
                network_info.ip_address[3] = (uint8_t)(WiFi.localIP()[3]);
                network_info.mac_address[0] = (uint8_t)(WiFi.macAddress()[0]);
                network_info.mac_address[1] = (uint8_t)(WiFi.macAddress()[1]);
                network_info.mac_address[2] = (uint8_t)(WiFi.macAddress()[2]);
                network_info.mac_address[3] = (uint8_t)(WiFi.macAddress()[3]);
                network_info.mac_address[4] = (uint8_t)(WiFi.macAddress()[4]);
                network_info.mac_address[5] = (uint8_t)(WiFi.macAddress()[5]);
                network_info.rssi = (int8_t)(WiFi.RSSI());
                //
                uint8_t *message = get_next_spi_buffer();
                memcpy(message, &network_info, sizeof(network_connection_info_s));
                package_data(message, sizeof(network_connection_info_s), PROTOCOL_PACKET_TYPE_NETWORK_INFO);
                //
                // Start the GDB server task.
                //
                xTaskCreate(task_wifi_server, "GDB Server", 4096, (void *)2159, 1, &xHandle);
                //
                // Assert ESP32 READY to ensure ctxLink knows
                //
                set_ready();
                vTaskDelay(pdMS_TO_TICKS(1000));
                xQueueSend(spi_comms_queue, &message, 0);   // Send network information to SPI task
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10000)); // Delay for 10 seconds before checking again
    }
}