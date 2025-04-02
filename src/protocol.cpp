/**
 * @file protocol.cpp
 * @author Sid Price (sid@sidprice.com)
 * @brief 
 * @version 0.1
 * @date 2025-04-02
 * 
 * @copyright Copyright Sid Price (c) 2025
 * 
 * This file contains the implementation of the protocol used for communications between the ESP32
 * and ctxLink.
 */

#include <Arduino.h>
#include "protocol.h"

static constexpr size_t PACKET_HEADER_SIZE = PACKET_HEADER_DATA_START;  // Size of the header before the data starts

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
 */
size_t package_data(uint8_t * buffer, size_t data_length, size_t buffer_size) {
    memmove(buffer + PACKET_HEADER_DATA_START, buffer, data_length);  // Move the data to start 3 bytes in

    buffer[PACKET_HEADER_MAGIC1] = PACKET_HEADER_MAGIC1;  // First byte of magic number
    buffer[PACKET_HEADER_MAGIC2] = PACKET_HEADER_MAGIC2;  // Second byte of magic number
    buffer[PACKET_HEADER_LENGTH_MSB] = (data_length >> 8) & 0xFF;  // High byte of data length
    buffer[PACKET_HEADER_LENGTH_LSB] = data_length & 0xFF;         // Low byte of data length
    buffer[PACKET_HEADER_SOURCE_ID] = 0x01;  // Message source identifier (e.g. 0x01 for GDB)
    size_t packet_length = data_length + PACKET_HEADER_SIZE;  // Total packet length
    size_t padding = packet_length % 4;  // Calculate padding to make the total size a multiple of 4 
    if (padding > 0) {
        padding = 4 - padding;  // Calculate the number of padding bytes needed
        memset(buffer + packet_length, 0x00, padding);  // Fill the padding bytes with 0x00
    }
    return packet_length + padding;  // Return the total size of the packet including header
}
