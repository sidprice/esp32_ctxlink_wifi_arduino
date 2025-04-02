/**
 * @file protocol.h
 * @author Sid Price (sid@sidprice.com)
 * @brief 
 * @version 0.1
 * @date 2025-04-02
 * 
 * @copyright Copyright Sid Price (c) 2025
 * 
 * Definitions and references dealing with the protocol used for
 * communications between the ESP32 and ctxLink
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#ifndef CTXLINK
#include <Arduino.h>
#else
    // TODO insert ctxLink header
#endif

/*
    This file describes the packet protocol used between an ESP32 based
    Wi-FI module and ctxLink using SPI peripherals.

    ctxLink is the master, and ESP32 is the slave module.

    +---------------------------------+
    |  Magic Number (2 bytes)         |
    +---------------------------------+
    |  Following bytecount (2 bytes)  |
    +---------------------------------+
    |  Packet Type (1 byte)           |
    +---------------------------------+
    |  Data (variable length)         |
    +---------------------------------+

    NOTE:   The total packet size MUST be a multiple of 4-bytes as
            required by the ESP32 DMA peripheral. If required, dummy
            bytes of 0x00 should be appended.
*/

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


/**
 * @brief Definition of the protocol magic number, used in the header
 * 
 */
constexpr uint8_t PROTOCOL_MAGIC1 = 0xDE ; // First byte of the magic number
constexpr uint8_t PROTOCOL_MAGIC2 = 0xAD ; // Second byte of the magic number

typedef enum {
    PROTOCOL_PACKET_TYPE_CMD_STAT = 0x00, // Command or status
    PROTOCOL_PACKET_TYPE_GDB = 0x01,     // GDB packet type
    PROTOCOL_PACKET_TYPE_SWO = 0x02, // SWO packet type
    PROTOCOL_PACKET_TYPE_UART = 0x03, // UART data packet type
    PROTOCOL_PACKET_TYPE_MAX = 0xFF       // Maximum packet type value
} protocol_packet_type_e;

#endif // PROTOCOL_H