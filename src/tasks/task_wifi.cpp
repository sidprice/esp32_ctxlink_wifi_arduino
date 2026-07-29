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

#include "serial_control.h"
#include <Arduino.h>
#include "..\wifi-tools\wifi_tools.h"

#include "ctxlink_preferences.h"
#include "protocol.h"

#include "task_spi_comms.h"
#include "task_wifi.h"
#include "tasks/task_server.h"

#include "ctxlink.h"

//
// Wi-Fi credentials
//
static char ssid[MAX_SSID_LENGTH]{0};
static char password[MAX_PASS_PHRASE_LENGTH] = {0};

/**
 * @brief Define the polling period for the Wi-Fi connection state
 *
 */
const size_t wifi_state_poll_period = 10000; // Expressed in milliseconds

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

/**
 * @brief This is the depth of the WIFI task messaging queue
 *
 */
constexpr uint32_t wifi_comms_queue_length = 4;

/**
 * @brief Instantiate the GDB server parameters
 *
 * These are passed to a server instance and also used by the SPI
 * task to endure messages are routed correctly
 *
 */
server_task_params_t gdb_server_params = {
	PROTOCOL_PACKET_STATUS_TYPE_GDB_CLIENT,
	"GDB",
	GDB_SERVER_PORT,
	0,
	NULL, // Server queue
	NULL, // Server task handle
	PROTOCOL_PACKET_TYPE_FROM_GDB,
};

/**
 * @brief Handle for the GDB Server task
 *
 */
TaskHandle_t gdb_task_handle = NULL;

/**
 * @brief The Wi-Fi task message queue
 *
 * This queue is used to send messages between the other tasks and the Wi-Fi
 * task.
 */
QueueHandle_t wifi_comms_queue;

/**
 * @brief Send a command to the server task to shut down the server
 *
 */
void wifi_send_server_command(protocol_command_type_e command)
{
	if (gdb_task_handle != NULL && server_queue != NULL) {
		protocol_packet_command_s cmd_packet = {0};
		cmd_packet.type = PROTOCOL_PACKET_TYPE_CMD;
		cmd_packet.command = command;

		uint8_t *message = get_next_spi_buffer();
		memcpy(message, &cmd_packet, sizeof(cmd_packet));
		package_data(message, sizeof(cmd_packet), PROTOCOL_PACKET_TYPE_COMMAND);
		xQueueSend(server_queue, &message, 0); // Send command packet pointer to GDB server task
	}
}

void wifi_disconnect(void)
{
	if (wifi_tools.is_connected) {
		MON_NL("Disconnecting Wi-Fi");
		WiFi.disconnect();
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait completed disconnect
		MON_NL("#2");
	}
} // deinitWiFi() end

void wifi_get_net_info(void)
{
	uint8_t *message = get_next_spi_buffer();
	MON_NL("Sending network info");
	memcpy(message, &network_info, sizeof(network_connection_info_s));
	package_data(message, sizeof(network_connection_info_s), PROTOCOL_PACKET_TYPE_NETWORK_INFO);
	//
	// Send to ctxLink via SPI task
	xQueueSend(spi_comms_input_queue, &message, 0);
}

/**
 * @brief Initialize and start the Wi-Fi connection
 *
 * @param ssid The SSID of the Wi-Fi network
 * @param password The password of the Wi-Fi network
 */
void wifi_startup(const char *ssid, const char *password)
{
	wifi_tools.log_events();
	wifi_tools.begin(ssid, password);
}

/**
 * @brief Wi-Fi task
 *
 * This task handles Wi-Fi connectivity and reconnection logic.
 */
void task_wifi(void *pvParameters)
{
	(void)pvParameters; // Unused parameter
	bool wifi_connect_processed = false;
	bool wifi_disconnect_processed = false;
	//
	// Create the input message queue
	//
	wifi_comms_queue = xQueueCreate(wifi_comms_queue_length,
		sizeof(uint8_t *)); // Create the queue for the task
	//
	// Get the wi-fi settings from preferences
	//
	memset(ssid, 0, MAX_SSID_LENGTH);
	memset(password, 0, MAX_PASS_PHRASE_LENGTH);
	size_t settings_count = preferences_get_wifi_parameters(ssid, password);
	MON_PRINTF("SSID: %s\r\n", (char *)ssid);
	MON_PRINTF("Passphrase: %s\r\n", (char *)password);
	wifi_startup(ssid, password);
	//
	// Task working loop
	//
	while (true) {
		BaseType_t result;
		static uint8_t *message;
		//
		result = xQueueReceive(wifi_comms_queue, &message, pdMS_TO_TICKS(10));
		if (result == pdTRUE) {
			size_t data_length;
			size_t packet_size;
			protocol_packet_type_e packet_type;
			uint8_t *packet_data;
			packet_size = protocol_split(message, &data_length, &packet_type, &packet_data);
			//
			// Process the received packet
			//
			network_connection_info_s *conn_info = (network_connection_info_s *)packet_data;
			MON_NL("Network info received");
			MON_PRINTF("SSID: %s\r\n", conn_info->network_ssid);
			MON_PRINTF("Passphrase: %s\r\n", conn_info->pass_phrase);
			//
			// Check if the Wi-Fi is already connected
			//
			if (wifi_tools.is_connected) {
				//
				// Check if the network information has changed
				//
				if (strcmp(ssid, conn_info->network_ssid) != 0 || strcmp(password, conn_info->pass_phrase) != 0) {
					MON_NL("Wi-Fi credentials changed, reconnecting...");
					memset(&network_info, 0, sizeof(network_connection_info_s));
					strncpy(ssid, conn_info->network_ssid, MAX_SSID_LENGTH);
					strncpy(password, conn_info->pass_phrase, MAX_PASS_PHRASE_LENGTH);
					wifi_disconnect();
					wifi_startup(ssid, password);
				} else {
					MON_NL("Wi-Fi credentials unchanged");
					wifi_get_net_info();
				}
			} else {
				strncpy(ssid, conn_info->network_ssid, MAX_SSID_LENGTH);
				strncpy(password, conn_info->pass_phrase, MAX_PASS_PHRASE_LENGTH);
				wifi_startup(ssid, password);
			}
		}
		if (wifi_tools.is_connected) {
			//
			// Check if the connect code has been run
			//
			if (!wifi_connect_processed) {
				wifi_connect_processed = true;
				wifi_disconnect_processed = false;
				//
				MON_NL("Wi-Fi Connected");
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
					xTaskCreate(task_wifi_server, "GDB Server", 4096, (void *)&gdb_server_params, 1, &gdb_task_handle);
				} else {
					MON_NL("Restart GDB Server");
					wifi_send_server_command(PROTOCOL_PACKET_TYPE_CMD_START_GDB_SERVER);
				}
				//
				// Assert ESP32 READY to ensure ctxLink knows
				//
				// TODO Not sure this is the right place for this. What happens if Wi-Fi
				// is not connected?
				//
				control_esp32_ready(true);
				vTaskDelay(pdMS_TO_TICKS(1000));
				xQueueSend(spi_comms_input_queue, &message,
					0); // Send network information to SPI task
			}
		} else {
			wifi_tools.reconnect();
			//
			// Check if the disconnect code has been run
			//
			if (!wifi_disconnect_processed) {
				// run the code that depends on the network being disconnected
				wifi_disconnect_processed = true;
				wifi_connect_processed = false;
				//
				MON_NL("Wi-Fi Disconnected");
				wifi_send_server_command(PROTOCOL_PACKET_TYPE_CMD_SHUTDOWN_GDB_SERVER);
			}
		}
	}
}
