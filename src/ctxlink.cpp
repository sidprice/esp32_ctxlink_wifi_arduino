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

#include "serial_control.h"
#include "tasks/task_server.h"
#include "ctxlink.h"
#include "helper.h"
#include "ESP32DMASPISlave.h"
#include "tasks/task_spi_comms.h"

static const uint8_t nREADY = 8 ; // GPIO pin for ctxLink nReady input

static bool is_tx = false ;

ESP32DMASPI::Slave slave;

static constexpr size_t BUFFER_SIZE = 256; // should be multiple of 4
static constexpr size_t QUEUE_SIZE = 1;

// definition of user callback
void IRAM_ATTR userTransactionCallback(spi_slave_transaction_t *trans, void *arg)
{
    // NOTE: here is an ISR Context
    //       there are significant limitations on what can be done with ISRs,
    //       so use this feature carefully!

    //
    // Send a message to the SPI task, data transaction is completed.
    //
    // If the ATTN port was asserted, this is a tx completion, otherwise
    // if is an rx completion. Send appropriate buffer pointer.
    //
    uint8_t *data = (is_tx == true) ? (uint8_t*)trans->tx_buffer : (uint8_t *)trans->rx_buffer ;
    xQueueSendFromISR(spi_comms_queue,&data, NULL);
}

void IRAM_ATTR userPostSetupCallback(spi_slave_transaction_t *trans, void *arg)
{
    // NOTE: here is an ISR Context
    //       there are significant limitations on what can be done with ISRs,
    //       so use this feature carefully!

}

void spi_ss_activated(void) {
  // This function is called when the ctxLink CS line is activated
  // It is used to wake up the ESP32 from deep sleep mode
  MONITOR(println("ctxLink CS activated")) ;
}

/**
 * @brief Initialize the SPI peripheral for ctxLink communication
 * 
 */
void initCtxLink(void) {
  slave.setDataMode(SPI_MODE1);
  slave.setMaxTransferSize(BUFFER_SIZE);  // default: 4092 bytes
  slave.setQueueSize(QUEUE_SIZE);         // default: 1

  // begin() after setting
  slave.begin();  // default: HSPI (please refer README for pin assignments)
  slave.setUserPostSetupCbAndArg(userTransactionCallback, NULL);
  slave.setUserPostTransCbAndArg(userTransactionCallback, NULL);
}

/**
 * @brief Create a SPI transaction with the ctxLink module
 * 
 * The slave must always be prepared for the master to send
 * a transaction. This pending transaction is created to service
 * a transaction from the master.
 * 
 * Note:  When the slave has a packet to send to the master, the pending
 *        transaction will be removed from the queue, and a new one
 *        created after the slave packet has been sent to the master.
 */
void spi_create_pending_transaction(uint8_t *dma_tx_buffer, uint8_t *dma_rx_buffer, bool isTx) {
  is_tx = isTx ; // Set the transaction type
  // with user-defined ISR callback that is called before/after transaction start
  // you can set these callbacks and arguments before each queue()
  slave.setUserPostSetupCbAndArg(userPostSetupCallback, NULL);
  slave.setUserPostTransCbAndArg(userTransactionCallback, NULL);
  // queue transaction and trigger it right now
  slave.queue(dma_tx_buffer, dma_rx_buffer, BUFFER_SIZE);
  slave.trigger();
}

/**
 * @brief Indicate to ctxLink the ESP32 is ready
 * 
 * Initially this is asserted once a wireless connection is made, however
 * in the future it may need to be asserted in there is no Wi-Fi connection.
 * This would enable ctxLink to configure the Wi-Fi.
 */
void set_ready(void) {
  digitalWrite(nREADY, LOW ) ;
}
