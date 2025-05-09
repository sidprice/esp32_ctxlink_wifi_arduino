/**
 * @file task_server.h
 * @author Sid Price (sid@sidprice.com)
 * @brief Socket based server for ctxLink ESP32 Wi-Fi adapter
 * @version 0.1
 * @date 2025-03-24
 * 
 * @copyright Copyright Sid Price (c) 2025
 * 
 */

 #ifndef TASK_SERVER_H
 #define TASK_SERVER_H

extern QueueHandle_t gdb_server_queue ;

void task_wifi_server(void *pvParameters) ;
constexpr uint8_t MAGIC_HI = 0xbe ;
constexpr uint8_t MAGIC_LO = 0xef ;
#endif