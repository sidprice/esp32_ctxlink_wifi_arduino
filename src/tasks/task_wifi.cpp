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
#include "ctxlink_preferences.h"

#include "task_wifi.h"
#include "tasks/task_server.h"
#include "task_spi_comms.h"

#include "ctxlink.h"
// Wi-Fi credentials
// TODO These need to be set from ctxLink and saved in the preferences

static char ssid[MAX_SSID_LENGTH] {0} ;
static char password[MAX_PASS_PHRASE_LENGTH] = {0} ;

/**
 * @brief Define the polling period for the Wi-Fi connection state
 * 
 */
 const size_t wifi_state_poll_period = 10000 ; // Expressed in milliseconds

 /**
  * @brief The current Wi-Fi status 
  * 
  */
wl_status_t wifi_status = WL_NO_SHIELD;


/**
 * @brief Structure to hold the information about the current network connection
 *
 */
static network_connection_info_s network_info;

// const char* ssid = "Avian Ambassadors";
// const char* password = "mijo498rocks";

/**
 * @brief This is the depth of the WIFI task messaging queue
 *
 */
constexpr uint32_t wifi_comms_queue_length = 4;

/**
 * @brief Handle for the GDB Server task
 *
 */
TaskHandle_t gdb_task_handle = NULL ;

/**
 * @brief The Wi-Fi task message queue
 *
 * This queue is used to send messages between the other tasks and the Wi-Fi task.
 */
QueueHandle_t wifi_comms_queue;

/**
 * @brief Previous Wi-Fi status
 *
 * This is used to detect changes in the Wi-Fi status.
 */
static wl_status_t previous_status = WL_NO_SHIELD;

/**
 * @brief Handle Wi-Fi events
 * 
 */
void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    MON_PRINTF("Wi-Fi event: %d\r\n", event);
    switch (event) {
    {
        case WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP: {
            MON_NL("Wi-Got IP Address");
            xTaskNotifyGive(wifi_task_handle);
            break;
        }
        case WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
            MON_NL("Wi-Fi Disconnected");
            previous_status = WL_DISCONNECTED;
            xTaskNotifyGive(wifi_task_handle);
            break;
        }
        default:
        break;
    }
    }
}

void disconnect_wifi(void)
{
    if (WiFi.status() == WL_CONNECTED) {
        MON_NL("Disconnecting Wi-Fi");
        WiFi.disconnect() ;
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait completed disconnect
        MON_NL("#2");
    }
} // deinitWiFi() end

wl_status_t configWiFi(void)
{
    wifi_status = WiFi.status();
    MON_NL("Configuring Wi-Fi");
    if ( wifi_status == WL_CONNECTED) {
        disconnect_wifi(); // Disconnect from any existing Wi-Fi connection
    }
    //
    // Set up the Wi-Fi Station
    //
    // TODO Set the hostname to something unique, using MAC perhaps?
    //
    WiFi.setHostname("ctxLink_adapter_1");
    MON_PRINTF("Hostname = %s\r\n", WiFi.getHostname());
    MON_NL("Connecting to WiFi.");

    uint32_t retry = 10;

    wifi_status = WiFi.begin(ssid, password);
    MON_PRINTF("SSID: %s\r\n", ssid);
    MON_PRINTF("Passphrase: %s\r\n", password);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait completed connect
    do
    {
        wifi_status = WiFi.status();
        if (wifi_status == WL_CONNECTED)
        {
            break;
        }
        vTaskDelay(1000);
        MON(".");
    } while (retry--);
    MON("\r\n");
    if (wifi_status == WL_CONNECTED)
    {
        MON_PRINTF("IP Address: %s\r\n", WiFi.localIP().toString().c_str());
        MON_PRINTF("RSSI: %d\r\n", WiFi.RSSI());
    }
    else
    {
        MON_PRINTF("Wi-Fi status = %d\r\n", wifi_status);
        MON_NL("Failed to connect to Wi-Fi");
    }
    return wifi_status;
} // configWiFi() end

void task_wifi(void *pvParameters)
{
    (void)pvParameters; // Unused parameter
    BaseType_t result ;
    static uint8_t *message;
    //
    wifi_comms_queue = xQueueCreate(wifi_comms_queue_length, sizeof(uint8_t *)); // Create the queue for the SPI task
    //
    // Get the wi-fi settings from preferences
    //
    memset(ssid, 0, MAX_SSID_LENGTH);
    memset(password, 0, MAX_PASS_PHRASE_LENGTH);
    size_t settings_count = preferences_get_wifi_parameters(ssid,password) ;
    MON_PRINTF("SSID: %s\r\n", (char*)ssid);
    MON_PRINTF("Passphrase: %s\r\n", (char*)password);
    WiFi.onEvent(onWiFiEvent, WiFiEvent_t::ARDUINO_EVENT_MAX); // Register the Wi-Fi event handler
    configWiFi(); // Attempt to connect to Wi-Fi
    while (1)
    {
        wifi_status = WiFi.status();
        //
        // Has the wifi status changed?
        //
        if (wifi_status != previous_status)
        {
            protocol_packet_command_s command = {0};
            MON_PRINTF("Wi-Fi status = %d\r\n", wifi_status);
            previous_status = wifi_status;
            if (wifi_status == WL_DISCONNECTED)
            {
                if (gdb_task_handle != NULL)
                {
                    MON_NL("Shut down Server");
                    command.type = PROTOCOL_PACKET_TYPE_CMD;
                    command.command = PROTOCOL_PACKET_TYPE_CMD_SHUTDOWN_GDB_SERVER;
                    if ( gdb_server_queue != NULL) {
                        xQueueSend(gdb_server_queue, &command, 0); // Send command to GDB server task
                    }  
               }
                wifi_status = configWiFi(); // Attempt to reconnect to Wi-Fi
            }
            if ( wifi_status == WL_CONNECTED) {
                MON_NL("Wi-Fi Reconnected");
                //
                // Update the current network information structure
                //
                memset(&network_info, 0, sizeof(network_connection_info_s));
                MON_PRINTF("Wi-Fi connected to SSID: %s\r\n", ssid);
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
                if (gdb_task_handle == NULL) {
                    //
                    // Start the GDB Server Task
                    //
                    MON_NL("Starting GDB Server Task");
                    xTaskCreate(task_wifi_server, "GDB Server", 4096, (void *)2159, 1, &gdb_task_handle);
                } else {
                    MON_NL("Restart GDB Server");
                    command.type = PROTOCOL_PACKET_TYPE_CMD;
                    command.command = PROTOCOL_PACKET_TYPE_CMD_START_GDB_SERVER;
                    xQueueSend(gdb_server_queue, &command, 0); // Send network information to GDB server task
                }
                //
                // Assert ESP32 READY to ensure ctxLink knows
                //
                set_ready();
                vTaskDelay(pdMS_TO_TICKS(1000));
                xQueueSend(spi_comms_queue, &message, 0);   // Send network information to SPI task
            }
        } else {
            //
            // Wi-Fi status has not changed, so just wait for a message from the other tasks or spi driver
            //
            result = xQueueReceive(wifi_comms_queue, &message, wifi_state_poll_period);
            if ( result == pdTRUE) {
                size_t data_length;
                size_t packet_size;
                protocol_packet_type_e packet_type;
                uint8_t *packet_data;
                packet_size = protocol_split(message, &data_length, &packet_type, &packet_data);
                //
                // Process the received packet
                //
                network_connection_info_s *conn_info = (network_connection_info_s*)packet_data ;
                MON_NL("Network info received");
                MON_PRINTF("SSID: %s\r\n", conn_info->network_ssid);
                MON_PRINTF("Passphrase: %s\r\n", conn_info->pass_phrase);
                memset(&network_info, 0, sizeof(network_connection_info_s));
                strncpy(ssid, conn_info->network_ssid, MAX_SSID_LENGTH);
                strncpy(password, conn_info->pass_phrase, MAX_PASS_PHRASE_LENGTH);
                configWiFi() ;
            }
        }
    }
}

void wifi_get_net_info(void)
{
    uint8_t *message = get_next_spi_buffer();
    MON_NL("Sending network info");
    memcpy(message, &network_info, sizeof(network_connection_info_s));
    package_data(message, sizeof(network_connection_info_s), PROTOCOL_PACKET_TYPE_NETWORK_INFO);
    //
    // Send to ctxLink via SPI task
    xQueueSend(spi_comms_queue, &message, 0);
}