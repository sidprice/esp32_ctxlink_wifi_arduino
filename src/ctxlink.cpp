/**
 * @file ctxlink.cpp
 * @author Sid Price (sid@sidprice.com)
 * @brief SPI communication with ctxLink module
 * @version 0.1
 * @date 2025-03-21
 * 
 * @copyright Copyright Sid Price (c) 2025
 * 
 * This module provides support for the SPI interface
 * between the ESP32 and ctxLink.
 */

#include <Arduino.h>
#include <SPI.h>
#include <driver/spi_slave.h>

#include "serial_control.h"

#include "ctxlink.h"

static const uint8_t ATTN = 9 ; // GPIO pin for ctxLink ATTN line

/**
 * @brief Callback function, after setup of data send transaction
 * 
 * @param trans 
 * 
 * If the transaction is a transmission, assert ATTN line to indicate data is
 * ready to be read by ctxLink.
 * 
 * A transmission is identified by the first two bytes of the passed tx buffer
 * being none-zero.`
 */
void my_post_setup_cb(spi_slave_transaction_t *trans)
{
    uint8_t * buffer = (uint8_t *)trans->tx_buffer ;
    if (*buffer != 0  && *(buffer + 1) != 0) {
        digitalWrite(ATTN, LOW) ; // Set ATTN line low to indicate data is ready to be read by ctxLink
    }
    MONITOR(println("my_post_setup_cb")) ;
}

/**
 * @brief Callback function, after all data is Transferred
 * 
 * @param trans 
 * 
 * Negate the ATTN line 
 */

void my_post_trans_cb(spi_slave_transaction_t *trans)
{
  uint8_t * buffer = (uint8_t *)trans->tx_buffer ;
  
  digitalWrite(ATTN, HIGH );
  //
  //. If this was not a transmission process the received data
  //
  if ( *buffer == 0 && *(buffer+1) == 0) {
    MONITOR(println("Received data")) ;
  } else {
    MONITOR(println("Transmitted data")) ;
  }
}

 void initCtxLink(void) {
// Configuration for the SPI bus
spi_bus_config_t buscfg = {
    .mosi_io_num = MOSI,  // MOSI pin
    .miso_io_num = MISO,  // MISO pin
    .sclk_io_num = SCK,  // SCLK pin
    .quadwp_io_num = -1,
    .quadhd_io_num = -1
  };

  // Configuration for the SPI slave interface
  spi_slave_interface_config_t slvcfg = {
    .spics_io_num = SS,  // CS pin
    .flags = 0,
    .queue_size = 3,
    .mode = 0,
    .post_setup_cb = my_post_setup_cb,
    .post_trans_cb = my_post_trans_cb
  };
  
  //
  // Configure the SPI GPIOs so spurious signals are not detected
  // when no master is connected. This is actually unlikely since
  // this MCU will be on the same PCB as ctxLink
  //
  pinMode(MOSI, OUTPUT | PULLUP );
  pinMode(MISO, OUTPUT | PULLUP );
  pinMode(SS, OUTPUT | PULLUP );
  pinMode(ATTN, OUTPUT );
  digitalWrite(ATTN, HIGH ) ;
  spi_slave_initialize(SPI2_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
}

/**
 * @brief Set up a transaction between ESP32 and ctxLink
 * 
 * @param rx_buffer 
 * @param tx_buffer 
 * @param buffer_length 
 */
void spi_transaction(uint8_t * rx_buffer, uint8_t * tx_buffer, size_t buffer_length) {
    MONITOR(println("spi_transaction")) ;
    spi_slave_transaction_t t = {0} ;
    t.length = buffer_length * 8 ;
    t.rx_buffer = rx_buffer ;
    t.tx_buffer = tx_buffer ;
    // spi_slave_transmit(SPI2_HOST, &t, portMAX_DELAY); // TODO Should the timeout be zero?
    spi_slave_queue_trans(SPI2_HOST, &t, portMAX_DELAY);
}