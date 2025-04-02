/**
 * @file task_spi_comms.h
 * @author Sid Price (sid@sidprice.com)
 * @brief 
 * @version 0.1
 * @date 2025-03-24
 * 
 * @copyright Copyright Sid Price (c) 2025
 * 
 * This module handles all communications between the server tasks and the ctxLink module
 * via the SPI interface.
 */

#ifndef TASK_SPI_COMMS_H
#define TASK_SPI_COMMS_H

#include <Arduino.h>

 /**
  * @brief Structure used for inter-task messaging.
  * 
  * This structure is used to pass messages between the server tasks and the SPI communications task.
  * 
  * TODO    The structure needs to have an entry to identify the source of the message
  */

typedef struct task_spi_comms_message_t {
    uint8_t buffer_index ;  /**< Index of the buffer being used */
    uint16_t length ; /**< The number of bytes in the indexed buffer */
} task_spi_comms_message_t ;

extern QueueHandle_t spi_comms_queue ;

void task_spi_comms(void *pvParameters) ;

#endif // TASK_SPI_COMMS_H