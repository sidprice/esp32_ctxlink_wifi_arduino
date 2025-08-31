/**
 * @file task_client.cpp
 * @author Sid Price (sid@sidprice.com)
 * @brief 
 * @version 0.1
 * @date 2025-08-11
 * 
 * @copyright Copyright (c) 2025
 * 
 * The task to manage the inflow of client data and forward it
 * to ctxLink via the SPI task
 */

#include <Arduino.h>
#include "serial_control.h"
#include "task_client.h"
#include "ctxlink.h"
#include "protocol.h"
#include "tasks/task_server.h"
#include "tasks/task_spi_comms.h"
#include "debug.h"

static char net_input_buffer[2048]; // Data received from network

/**
 * @brief Send the client state to the ctxLink
 *
 * @param server_params The server task parameters
 * @param state The state of the client (0x01 = connected, 0x00 = disconnected)
 */
void server_client_state_to_ctxlink(server_task_params_t *server_params, uint8_t state)
{
	protocol_packet_status_s status_packet;
	status_packet.type = server_params->server_type; // Indicate the server type
	status_packet.status = state;                    // 0x01 = connected, 0x00 = disconnected
	uint8_t *message = get_next_spi_buffer();
	memcpy(message, &status_packet, sizeof(protocol_packet_status_s));
	package_data(message, sizeof(protocol_packet_status_s), PROTOCOL_PACKET_TYPE_STATUS);
	xQueueSend(spi_comms_queue, &message, 0);
}

/**
 * @brief Task to manage the inflow of client data and forward it to ctxLink via the SPI task
 *
 * @param pvParameters Not used
 *
 * This task is responsible for managing the inflow of client data and forwarding it to ctxLink via the SPI task.
 */
void task_client(void *pvParameters)
{
	server_task_params_t *server_params = (server_task_params_t *)pvParameters;
	int client_fd = server_params->client_fd; // Get the client file descriptor from the parameters
	//
	// Inform ctxLink GDB client connected
	//
	server_client_state_to_ctxlink(server_params, 0x01);
	while (true) {
		int bytes_received = read(client_fd, &net_input_buffer, sizeof(net_input_buffer));
		if (bytes_received > 0) {
			size_t packed_size;
			//
			// Send input to the SPI task for forwarding to ctxLink
			//
			packed_size = package_data((uint8_t *)net_input_buffer, bytes_received, server_params->source_type);
			MON_PRINTF("Bytes received: %d\r\n", bytes_received);
			// MONITOR(print("Bytes received: "));
			// MONITOR(println(bytes_received));
			// for (int i = 0; i < packed_size; i++)
			// {
			//     MONITOR(printf("%02x ", net_input_buffer[i]));
			// }
			// MONITOR(println());
			uint8_t *input_message = (uint8_t *)net_input_buffer;
			TOGGLE_PIN(PINA);
			xQueueSend(spi_comms_queue, &input_message, 0);
		} else if (bytes_received == 0) {
			MONITOR(println("Client disconnected"));
			close(client_fd);
			client_fd = -1;
			//
			// Inform ctxLink the client disconnected
			//
			server_client_state_to_ctxlink(server_params, 0x00);
			break;
		} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
			// No data available, continue the loop
			// vTaskDelay(10 / portTICK_PERIOD_MS); // Yield to other tasks
		} else if (errno == ECONNRESET) {
			MONITOR(println("Client disconnected abruptly (ECONNRESET)"));
			close(client_fd);
			client_fd = -1;
			server_client_state_to_ctxlink(server_params, 0x00);
			break;
		} else {
			MONITOR(print("Socket read failed: "));
			MONITOR(println(errno));
			close(client_fd);
			client_fd = -1;
			server_client_state_to_ctxlink(server_params, 0x00);
			break;
		}
	}
	//
	// Kill this thread, client has disconnected
	//
	MON_NL("Client task deleted");
	vTaskDelete(NULL);
}