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

constexpr uint8_t ATTN = 9; // GPIO pin for ctxLink ATTN input

constexpr uint8_t PINA = 3; // Test Pin
constexpr uint8_t PINB = 4; // Test Pin
constexpr uint8_t PINC = 5; // Test Pin

extern bool system_setup_done;

void initCtxLink(void);
void control_esp32_ready(bool ready);
void spi_save_tx_transaction_buffer(uint8_t *transaction_buffer);
void spi_create_pending_transaction(uint8_t *tx_buffer, uint8_t *rx_buffer, bool isTx);
#endif // CTXLINK_H