/**
 * @file ctxlink.h
 * @author Sid Price (sid@sidprice.com)
 * @brief @brief ctxLink SPI interface header file
 * @version 0.1
 * @date 2025-03-21
 * 
 * @copyright Copyright Sid Price (c) 2025
 * 
 * This module provides support for the SPI interface
 * between the ESP32 and ctxLink.
 */

 #ifndef CTXLINK_H
#define CTXLINK_H

void initCtxLink(void) ;
void spi_transaction(uint8_t * rx_buffer, uint8_t * tx_buffer, size_t buffer_length) ;
#endif // CTXLINK_H