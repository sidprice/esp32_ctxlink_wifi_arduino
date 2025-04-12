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


 /**
  * @brief Macro to control Serialx.print
  * 
  * Comment the following line to turn off all serial output
  */
 // #define SERIAL_ON
 
 /**
  * @brief The following macros enable/disable serial output
  * 
  */
 #ifdef SERIAL_ON
 #define MONITOR(call) Serial.call
 #else
 #define MONITOR(call)
 #endif