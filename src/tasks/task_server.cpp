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

char rxBuffer[1024] ;

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
    if (listen(server_fd, 5) < 0) {
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
                for (int i = 0; i < bytes_received; i++) {
                    MONITOR(print(rxBuffer[i])) ;
                }
                MONITOR(println()) ;
            }
        }
            
    }
}