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
 * Assert ATTN line to indicate data is ready to be read by ctxLink
 */
void my_post_setup_cb(spi_slave_transaction_t *trans)
{
    // MONITOR(println("my_post_setup_cb")) ;
    digitalWrite(ATTN, LOW) ; // Set ATTN line low to indicate data is ready to be read by ctxLink
}

/**
 * @brief Callback function, after all data is read by ctxLink
 * 
 * @param trans 
 * 
 * Negate the ATTN line
 */

void my_post_trans_cb(spi_slave_transaction_t *trans)
{
    // MONITOR(println("my_post_trans_cb")) ;
    digitalWrite(ATTN, HIGH );
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
  
  // TODO configure ATTN output to ctxLink
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

// TODO This is a blocking function REALLY will need to have this operate using interrupts/callbacks
void testSPI(void) {
    MONITOR(println("testSPI")) ;
    uint8_t rxBuffer[128] = {0} ;
    uint8_t txBuffer[128] = {0} ;
    spi_slave_transaction_t t = {0} ;
    t.length = 6 * 8 ;
    t.rx_buffer = rxBuffer ;
    t.tx_buffer = txBuffer ;
    spi_slave_transmit(SPI2_HOST, &t, portMAX_DELAY);
    // //
    // // Now send a simple reply to ctxLink
    // //
    // txBuffer[0] = 0x01 ;
    // txBuffer[1] = 0x02 ;
    // txBuffer[2] = 0x03 ;
    // txBuffer[3] = 0x04 ;
    // txBuffer[4] = 0x05 ;
    // txBuffer[5] = 0x06 ;
    // spi_slave_transmit(SPI2_HOST, &t, portMAX_DELAY);
    // for (int i = 0; i < 6; i++) MONITOR(print(rxBuffer[i], HEX));
    //     MONITOR(println());
}