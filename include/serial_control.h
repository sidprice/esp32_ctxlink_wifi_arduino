/**
 * @file serial_control.h
 * @author Sid Price (sid@sidprice.com)
 * @brief Macros to enable/disable serial output
 * @version 0.1
 * @date 2024-05-05
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include "tasks/task_monitor.h"

/**
  * @brief Macro to control Serialx.print
  * 
  * Comment the following line to turn off all serial output
  */
#define SERIAL_ON

/**
  * @brief The following macros enable/disable serial output
  * 
  */
#ifdef SERIAL_ON

/**
 * @brief Direct to serial output
 * 
 * ONLY use this macro before the multi-threaded app starts. After
 * that use the MACOS defined below to send messages to the monitor task for output.
 * 
 */
#define MONITOR(call) Serial.call
#else
#define MONITOR(call)
#endif

/**
  * @brief Experimental macro to be used for message task and queuing
  *
  */

#ifdef SERIAL_ON
#define generic_send(msg)                                                             \
	do {                                                                              \
		if (xPortInIsrContext()) {                                                    \
			BaseType_t xHigherPriorityTaskWoken = pdFALSE;                            \
			xQueueSendFromISR(task_monitor_queue, &(msg), &xHigherPriorityTaskWoken); \
			if (xHigherPriorityTaskWoken) {                                           \
				portYIELD_FROM_ISR();                                                 \
			}                                                                         \
		} else {                                                                      \
			xQueueSend(task_monitor_queue, &(msg), portMAX_DELAY);                    \
		}                                                                             \
	} while (0)

#define MON(TEXT)                                                  \
	do {                                                           \
		monitor_output_message_t msg = {0};                        \
		strncpy(msg.message, TEXT, MONITOR_OUTPUT_MAX_LENGTH - 1); \
		generic_send(msg);                                         \
	} while (0)

#define MON_NL(TEXT)                                         \
	do {                                                     \
		monitor_output_message_t msg = {0};                  \
		size_t len = strlen(TEXT);                           \
		if (len > MONITOR_OUTPUT_MAX_LENGTH - 3) {           \
			len = MONITOR_OUTPUT_MAX_LENGTH - 3;             \
		}                                                    \
		memcpy(msg.message, TEXT, len);                      \
		msg.message[len] = '\r';                             \
		msg.message[len + 1] = '\n';                         \
		/* msg.message[len + 2] is already '\0' from init */ \
		generic_send(msg);                                   \
	} while (0)

#define MON_PRINTF(FORMAT, ...)                                          \
	do {                                                                 \
		monitor_output_message_t msg = {0};                              \
		snprintf(msg.message, sizeof(msg.message), FORMAT, __VA_ARGS__); \
		generic_send(msg);                                               \
	} while (0)
#endif // SERIAL_ON