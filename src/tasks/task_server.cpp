/**
 * @file task_server.cpp
 * @author Sid Price (sid@sidprice.com)
 * @brief Socket based server for ctxLink ESP32 Wi-Fi adapter
 * @version 0.1
 * @date 2025-03-24
 *
 * @copyright Copyright Sid Price (c) 2025
 *
 */

#include <Arduino.h>
#include <WiFi.h>
#include <lwip/sockets.h>

#include "serial_control.h"
#include "task_server.h"
#include "task_spi_comms.h"
#include "protocol.h"

#include "debug.h"

#include "ctxlink.h"

char net_input_buffer[2048]; // Data received from network

QueueHandle_t server_queue;

constexpr uint32_t server_queue_length = 16;

bool configure_server(in_port_t *port, int *server_fd, struct sockaddr_in *server_addr) {
    // Create socket
    *server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Set up server address
    server_addr->sin_family = AF_INET;
    server_addr->sin_addr.s_addr = INADDR_ANY;
    server_addr->sin_port = htons(*port);

    // Bind socket to address
    if (bind(*server_fd, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0)
    {
        MONITOR(print("Socket bind failed -> "));
        MONITOR(println(errno));
        return false;
    }

    // Listen for incoming connections
    if (listen(*server_fd, 5) < 0)
    { // TODO: Test for single client operation, set to 1 or 0?
        MONITOR(print("Socket listen failed -> "));
        MONITOR(println(errno));
        return false;
    }
    return true;
}

/**
 * @brief Task to handle the GDB Wi-Fi server
 *
 * @param pvParameters
 */
void task_wifi_server(void *pvParameters)
{
    server_task_params_t *server_params = (server_task_params_t *)pvParameters;
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;

    server_queue = xQueueCreate(server_queue_length, sizeof(uint8_t *)); // Create the queue for the GDB server task
    server_params->server_queue = server_queue; // Save the queue to the server task parameters
    socklen_t addr_len = sizeof(client_addr);

    in_port_t port = (in_port_t)server_params->port; // Recover the port number for this task
    //
    if (!configure_server(&port, &server_fd, &server_addr))
    {
        MON_PRINTF("Failed to configure %s server\r\n", server_params->server_name);
    }

    MON_PRINTF("%s server task started\r\n", server_params->server_name);
    //
    // Main server loop
    //
    while (true)
    {
        // Accept incoming connections
        MON_PRINTF("%s server waiting for client on port %d.\r\n", server_params->server_name, port);
        while (true)
        {
            client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
            if (client_fd < 0)
            {
                MONITOR(print("Socket accept failed"));
                MONITOR(println(errno));
                return; // TODO Deal with error, cannot return!
            }
            MONITOR(println("Client connected"));
            // Set the client socket to non-blocking mode
            int flags = fcntl(client_fd, F_GETFL, 0);
            // int flags = 1 ;
            fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
            flags = 1;
            setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags));
            //
            // Inform ctxLink GDB client connected
            //
            protocol_packet_status_s status_packet ;
            status_packet.type = server_params->server_type; // Indicate the server type
            status_packet.status = 0x01; // 0x01 = connected, 0x00 = disconnected
            uint8_t *message = get_next_spi_buffer();
            memcpy(message, &status_packet, sizeof(protocol_packet_status_s));
            package_data(message, sizeof(protocol_packet_status_s), PROTOCOL_PACKET_TYPE_STATUS);
            xQueueSend(spi_comms_queue, &message, 0);
            //
            //  Server data handling loop
            //
            while (true)
            {
                BaseType_t result;
                uint8_t *message;
                //
                // Check for packets from SPI task
                //
                result = xQueueReceive(server_queue, &message, 0); // Do not block here
                if (result == pdTRUE)
                {
                    size_t bytes_sent;
                    size_t packet_size;
                    protocol_packet_type_e packet_type;
                    uint8_t *packet_data;
                    protocol_split(message, &packet_size, &packet_type, &packet_data);

                    switch (packet_type)
                    {
                        case PROTOCOL_PACKET_TYPE_TO_CLIENT: {
                            while(packet_size > 0)
                            {
                                bytes_sent = send(client_fd, packet_data, packet_size, 0);
                                if (bytes_sent < 0)
                                {
                                    MONITOR(print("Socket send failed: "));
                                    MONITOR(println(errno));
                                    break;
                                }
                                packet_size -= bytes_sent;
                                packet_data += bytes_sent;
                            }
                            break;
                        }

                        case PROTOCOL_PACKET_TYPE_COMMAND: {
                            protocol_packet_command_s *command_packet = (protocol_packet_command_s *)packet_data;
                            if (command_packet->command == PROTOCOL_PACKET_TYPE_CMD_SHUTDOWN_GDB_SERVER)
                            {
                                MON_NL("Close Client/Server Sockets");
                                close(client_fd);
                                client_fd = -1;
                                close(server_fd);
                                server_fd = -1;
                            }
                            else if (command_packet->command == PROTOCOL_PACKET_TYPE_CMD_START_GDB_SERVER)
                            {
                                configure_server(&port, &server_fd, &server_addr);
                                MON_NL("Reconfigured Server");
                            }
                            else
                            {
                                MON_NL("Unknown command received");
                            }
                            break;
                        }

                        default:
                            MON_NL("Unknown packet type received");
                            break;
                    }
                }
                int bytes_received = read(client_fd, &net_input_buffer, sizeof(net_input_buffer));
                if (bytes_received > 0)
                {
                    size_t packed_size;
                    //
                    // Send input to the SPI task for forwarding to ctxLink
                    //
                    packed_size = package_data((uint8_t *)net_input_buffer, bytes_received, server_params->source_type);
                    // MONITOR(print("Bytes received: "));
                    // MONITOR(println(bytes_received));
                    // for (int i = 0; i < packed_size; i++)
                    // {
                    //     MONITOR(printf("%02x ", net_input_buffer[i]));
                    // }
                    // MONITOR(println());
                    uint8_t *input_message = (uint8_t *)net_input_buffer;
                    TOGGLE_PIN(PINA) ;
                    xQueueSend(spi_comms_queue, &input_message, 0);
                }
                else if (bytes_received == 0)
                {
                    MONITOR(println("Client disconnected"));
                    close(client_fd);
                    break;
                }
                else if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // No data available, continue the loop
                    // vTaskDelay(10 / portTICK_PERIOD_MS); // Yield to other tasks
                }
                else if (errno == ECONNRESET)
                {
                    MONITOR(println("Client disconnected abruptly (ECONNRESET)"));
                    close(client_fd);
                    break;
                }
                else
                {
                    MONITOR(print("Socket read failed: "));
                    MONITOR(println(errno));
                    close(client_fd);
                    break;
                }
            }
        }
    }
}