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

#include "ctxlink.h"

char net_input_buffer[1024] ;   // Data received from network
char from_ctxlink_buffer[1024] ;

QueueHandle_t gdb_server_queue ;

constexpr uint32_t gdb_server_queue_length = 4 ;

/**
 * @brief Task to handle the GDB Wi-Fi server
 * 
 * @param pvParameters 
 */
void task_wifi_server(void *pvParameters) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;

    gdb_server_queue = xQueueCreate(gdb_server_queue_length, sizeof(uint8_t *)) ; // Create the queue for the GDB server task
    socklen_t addr_len = sizeof(client_addr);

    in_port_t port = (in_port_t)((uint32_t)pvParameters);   // Recover the port number for this task

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind socket to address
    if ( bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        MONITOR(print("Socket bind failed -> ")) ; MONITOR(println(errno)) ;
        return;
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) < 0) { // TODO: Test for single client operation, set to 1 or 0?
        MONITOR(print("Socket listen failed -> ")) ; MONITOR(println(errno)) ;
        return;
    }

    //
    // Just halt here for now
    //
    MONITOR(println("Server task started")) ;
    //
    // Main server loop
    //
    while(true) {
        // Accept incoming connections
        MONITOR(print("Server listening on port ")) ; MONITOR(println(port)) ;
        while (true) {
            client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
            if (client_fd < 0) {
                MONITOR(print("Socket accept failed")); MONITOR(println(errno)) ;
                return;
            } 
            MONITOR(println("Client connected"));
            // Set the client socket to non-blocking mode
            int flags = fcntl(client_fd, F_GETFL, 0);
            fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
            //
            //  Server data handling loop
            //
            while(true) {
                BaseType_t result ;
                uint8_t *message ;
                //
                // Check for packets from SPI task
                //
                result = xQueueReceive(gdb_server_queue, &message, 0) ;  // Do not block here
                if (result == pdTRUE) {
                    size_t packet_size ;
                    protocol_packet_type_e packet_type ;
                    uint8_t *packet_data ;
                    uint32_t message_length ;
                    MONITOR(print("packet_type: ")) ; MONITOR(println(packet_type)) ;
                    MONITOR(print("packet_size: ")) ; MONITOR(println(packet_size)) ; 
                    protocol_split(message, &packet_size, &packet_type, &packet_data, &message_length) ;
                    send(client_fd, message, packet_size, 0); // Send the data to the client
                }
                int bytes_received = read(client_fd, &net_input_buffer, sizeof(net_input_buffer));
                if (bytes_received > 0 ) {
                    MONITOR(print("Bytes received: ")) ; MONITOR(println(bytes_received)) ;
                    for (int i = 0; i < bytes_received; i++) {
                        MONITOR(print(net_input_buffer[i])) ;
                    }
                    MONITOR(println()) ;
                    //
                    // Send input to the SPI task for forwarding to ctxLink
                    //
                    package_data((uint8_t *)net_input_buffer, bytes_received, PROTOCOL_PACKET_TYPE_TO_CTXLINK, sizeof(net_input_buffer)) ; // Package the data for ctxLink
                    uint8_t *input_message = (uint8_t *)net_input_buffer ; // Cast the buffer to a uint8_t pointer
                    xQueueSend(spi_comms_queue, &input_message, 0) ; // Send the data to the SPI task
                } else if (bytes_received == 0) {
                    MONITOR(println("Client disconnected"));
                    close(client_fd);
                    break;
                } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // No data available, continue the loop
                    vTaskDelay(10 / portTICK_PERIOD_MS); // Yield to other tasks
                }  else if (errno == ECONNRESET) {
                    MONITOR(println("Client disconnected abruptly (ECONNRESET)"));
                    close(client_fd);
                    break;
                } else {
                    MONITOR(print("Socket read failed: ")); MONITOR(println(errno));
                    close(client_fd);
                    break;
                }
                //
                // Check if there is data from the SPI task to send to client
                //
            }
        }
            
    }
}