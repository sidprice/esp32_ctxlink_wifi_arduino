/**
 * @file debug.h
 * @author Sid Price (sid@sidprice.com)
 * @brief
 * @version 0.1
 * @date 2025-05-09
 *
 * @copyright Copyright Sid Price (c) 2025
 *
 * Header file with MACROs for debugging and timing measurements
 *
 */

#ifndef DEBUG_H
#define DEBUG_H
#include <Arduino.h>
/**
 * @brief Comment the following line to exclude pin toggling
 *
 */
#define DO_TOGGLE_PIN

#ifdef DO_TOGGLE_PIN

/**
 * @brief Toggle the specified pin
 *
 * @param pin The pin to be toggled
 *
 * This macro toggles the specified pin. It is used for debugging purposes
 */
#define TOGGLE_PIN(pin) digitalWrite(pin, !digitalRead(pin));

#else
#define TOGGLE_PIN(pin)
#endif

#endif