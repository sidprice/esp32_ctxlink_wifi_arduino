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
static const uint8_t nSPI_READY = 7 ; // GPIO pin for ctxLink SPI ready input

static bool is_tx = false ;

ESP32DMASPI::Slave slave;

static constexpr size_t BUFFER_SIZE = 2000; // should be multiple of 4
static constexpr size_t QUEUE_SIZE = 1;

static constexpr uint8_t empty_response[] = {0x00, 0x00, 0x00, 0x00}; // sent to spi task after transmission is done

static uint8_t  *tx_saved_transaction ;

/**
 * @brief Save the passed transaction packet pointer for later transmission
 * 
 * @param transaction_buffer  Pointer to the packet to be sent
 * 
 *  We are about to start a TX transaction, so save the packet pointer and
 *  assert the ATTN signal to ctxLink.
 */
void spi_save_tx_transaction_buffer(uint8_t *transaction_buffer) {
  tx_saved_transaction = transaction_buffer ;
  // digitalWrite(nREADY, HIGH) ;    // Will be asserted once the transaction is set up
  digitalWrite(ATTN, LOW) ;
}

/**
 * @brief Callback function on transaction completed
 * 
 * @param trans Pointer to the transaction that was completed
 * @param arg   Unused user argument
 */
void IRAM_ATTR userTransactionCallback(spi_slave_transaction_t *trans, void *arg)
{
  digitalWrite(ATTN, HIGH) ; // Set ATTN line high to indicate data is not ready to be read by ctxLink
  digitalWrite(nSPI_READY, HIGH) ; // Set nSPI_READY line high to indicate ESP32 SPI Transfer is not ready
  //
  if ( is_tx  == false) {
      xQueueSendFromISR(spi_comms_queue,(uint8_t *)&trans->rx_buffer, NULL);
  }
}

/**
 * @brief Callback function, called after transaction setup is completed
 * 
 * @param trans Pointer to the transaction that was set up
 * @param arg Unused user argument
 */
void IRAM_ATTR userPostSetupCallback(spi_slave_transaction_t *trans, void *arg)
{
  digitalWrite(nSPI_READY,LOW) ;    // Tell ctxLink the transaction is ready to go.
}

/**
 * @brief Interrupt handler for the SPI CS input falling transition
 * 
 *  If ATTN is asserted, set up a TX transaction using the saved txtransaction packet.
 * 
 *  Otherwise, set up an RX transaction
 * 
 *  Do nothing if ESP32 is not ready!
 */
void spi_ss_activated(void) {
  if ( digitalRead(nREADY) == LOW ) {
    if (digitalRead(ATTN) == LOW) { // Is this a TX transaction?
      // Set up a transaction to send the saved transaction buffer to ctxLink
      MONITOR(println("SS activated tx")) ;
      spi_create_pending_transaction(tx_saved_transaction, NULL, true) ; // This is a pending tx transaction
    } else {
      // Set up a transaction to receive data from ctxLink
      MONITOR(println("SS activated rx")) ;
      spi_create_pending_transaction(NULL, get_next_spi_buffer(), false) ; // This is a pending rx transaction
    }
  }
}

/**
 * @brief Initialize the SPI peripheral for ctxLink communication
 * 
 */
void initCtxLink(void) {
  // Set up the GPIO pins for ctxLink
  digitalWrite(nREADY, HIGH) ; // Set nREADY line high to indicate ESP32 is not ready
  pinMode(nREADY, OUTPUT); // Set nREADY line to output
  digitalWrite(nSPI_READY, HIGH) ; // Set nSPI_READY line high to indicate ESP32 SPI Transfer is not ready
  pinMode(nSPI_READY, OUTPUT); // Set nSPI_READY line to output
  digitalWrite(ATTN, HIGH) ; // Set ATTN line high to indicate ESP32 has no data
  pinMode(ATTN, OUTPUT); // Set ATTN line to output
  digitalWrite(SS, HIGH) ;
  pinMode(SS, INPUT_PULLUP); // Set SS line to input with pullup
  attachInterrupt(digitalPinToInterrupt(SS), spi_ss_activated, FALLING); // Attach interrupt to SS line
  slave.setDataMode(SPI_MODE1);
  slave.setMaxTransferSize(BUFFER_SIZE);  // default: 4092 bytes
  slave.setQueueSize(QUEUE_SIZE);         // default: 1

  // begin() after setting
  slave.begin();  // default: HSPI (please refer README for pin assignments)
  slave.setUserPostSetupCbAndArg(userTransactionCallback, NULL);
  slave.setUserPostTransCbAndArg(userTransactionCallback, NULL);
  //
  // Tell ctxLink we are ready
  //
  // TODO Figure out when esp32 is REALLY ready
  digitalWrite(nREADY, LOW) ; // Set nREADY line low to indicate ESP32 is ready
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

