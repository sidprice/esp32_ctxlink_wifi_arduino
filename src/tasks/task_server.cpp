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

#include "ctxlink.h"

char rxBuffer[1024] ;

/**
 * @brief Protocol Packet Header offsets
 * 
 */
enum packet_header_offset_e {
    PACKET_HEADER_MAGIC1 = 0,  // First byte of the magic number
    PACKET_HEADER_MAGIC2,      // Second byte of the magic number
    PACKET_HEADER_LENGTH_MSB,  // Most significant byte of the length of the data
    PACKET_HEADER_LENGTH_LSB,  // Least significant byte of the length of the data
    PACKET_HEADER_SOURCE_ID,   // Message source identifier (e.g. 0x01 for GDB)
    PACKET_HEADER_DATA_START   // Start of the actual data in the packet 
} ;

static constexpr size_t PACKET_HEADER_SIZE = PACKET_HEADER_DATA_START;  // Size of the header before the data starts
static constexpr uint8_t MAGIC_NUMBER[2] = {0xAA, 0x55};  // Example magic number for packet identification

/**
 * @brief Package the passed data into a buffer for transmission
 * 
 * @param buffer            Pointer to the input/output buffer
 * @param data_length       Length of the data to be packaged
 * @param buffer_size       SIze of the buffer
 * 
 * Protocol for the packaged data:
 *      Byte 0: First byte of 'magic number'
 *      Byte 1: Second byte of 'magic number'
 *      Byte 2: MS Byte of the byte count
 *      Byte 3: LS Byte of the byte count
 *      Byte 4: Message source identifier (e.g. 0x01 for GDB)
 *      Byte 5: First data byte
 *          ...
 *      Byte (n-1)+5    : Last data byte
 * 
 * TODO Define message source identifiers
 */
size_t package_data(uint8_t * buffer, size_t data_length, size_t buffer_size) {
    memmove(buffer + PACKET_HEADER_DATA_START, buffer, data_length);  // Move the data to start 3 bytes in

    buffer[PACKET_HEADER_MAGIC1] = MAGIC_NUMBER[0];  // First byte of magic number
    buffer[PACKET_HEADER_MAGIC2] = MAGIC_NUMBER[1];  // Second byte of magic number
    buffer[PACKET_HEADER_LENGTH_MSB] = (data_length >> 8) & 0xFF;  // High byte of data length
    buffer[PACKET_HEADER_LENGTH_LSB] = data_length & 0xFF;         // Low byte of data length
    buffer[PACKET_HEADER_SOURCE_ID] = 0x01;  // Message source identifier (e.g. 0x01 for GDB)

    return data_length + PACKET_HEADER_SIZE;  // Return the total size of the packet including header
}

/**
 * @brief Task to handle the GDB Wi-Fi server
 * 
 * @param pvParameters 
 */
void task_wifi_server(void *pvParameters) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
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
            //
            //  Server data handling loop
            //
            while(true) {
                int bytes_received = read(client_fd, &rxBuffer, sizeof(rxBuffer));
                if (bytes_received > 0 ) {
                    MONITOR(print("Bytes received: ")) ; MONITOR(println(bytes_received)) ;
                    for (int i = 0; i < bytes_received; i++) {
                        MONITOR(print(rxBuffer[i])) ;
                    }
                    MONITOR(println()) ;
                    //
                    // Send input to ctxLink
                    //
                    spi_transaction((uint8_t *)rxBuffer, (uint8_t *)rxBuffer,  package_data((uint8_t *)rxBuffer, bytes_received, sizeof(rxBuffer))) ;
                } else {
                    MONITOR(println("Client disconnected"));
                    close(client_fd);
                    break;
                }
            }
        }
            
    }
}