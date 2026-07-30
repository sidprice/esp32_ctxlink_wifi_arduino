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

#include "ctxlink.h"
#include "helper.h"
#include "serial_control.h"
#include "tasks/task_server.h"

#include "debug.h"

#include "ESP32DMASPISlave.h"
#include "tasks/task_spi_comms.h"

static const uint8_t nREADY = 8;     // GPIO pin for ctxLink nReady input
static const uint8_t nSPI_READY = 7; // GPIO pin for ctxLink SPI ready input

static const uint8_t SPI_SS_PIN = 34; // Your custom SS pin
static const uint8_t SPI_MISO_PIN = 37;
static const uint8_t SPI_MOSI_PIN = 35;
static const uint8_t SPI_SCK_PIN = 36;

static bool is_tx = false;

ESP32DMASPI::Slave slave;

static constexpr size_t BUFFER_SIZE = 2000; // should be multiple of 4
static constexpr size_t QUEUE_SIZE = 1;

static uint8_t *tx_saved_transaction;
static uint8_t zero_transaction_buffer[BUFFER_SIZE] = {0}; // Use your max transfer size

bool system_setup_done = false;

/**
 * @brief Save the passed transaction packet pointer for later transmission
 *
 * @param transaction_buffer  Pointer to the packet to be sent
 *
 * 	If the transaction_buffer is NULL, the call is to check for any
 * transactions queued.
 * 
 *  If the spi comms output queue has a single entry:
 * 		Save the buffer pointer and assert nATTN.
 * 
 * If the spi comms output queue has more than 1 entry:
 * 		Place the buffer into the output queue.
 * 
 * Upon transaction completion the entry must be removed from the queue.
 */
void spi_save_tx_transaction_buffer(uint8_t *transaction_buffer)
{
	UBaseType_t queue_count;
	queue_count = uxQueueMessagesWaiting(spi_comms_output_queue);
	MON_PRINTF("Entry - Queue count: %d\r\n", queue_count);
	//
	// If a transcation buffer is received, queue it.
	//
	if (transaction_buffer != NULL) {
		xQueueSend(spi_comms_output_queue, &transaction_buffer, 0);
	} else if (queue_count == 0) {
		//
		// If there are no queued transactions, do nothing.
		//
		return;
	}
	//
	// If there is more than 1 queued item, we are done.
	//
	queue_count = uxQueueMessagesWaiting(spi_comms_output_queue);
	MON_PRINTF("Queue count: %d\r\n", queue_count);
	if (queue_count > 1) {
		return;
	}
	//
	// Get the next transaction buffer from the queue and save it for transmission.
	//
	// Leave the item in the queue, it is removed on completion of the transaction.
	//
	xQueuePeek(spi_comms_output_queue, &tx_saved_transaction, 0); // Get the next transaction buffer from the queue
	//
	// Signal ctxLink that there is a transaction ready to be sent.
	//
	digitalWrite(ATTN, LOW);
}

static uint8_t packet_transaction_completed[] = {
	PROTOCOL_MAGIC1, PROTOCOL_MAGIC2, PROTOCOL_TRANSACTION_COMPLETED, 0x00, 0x01, 0x01}; // 0x01 = transaction completed

/**
 * @brief Callback function on transaction completed
 *
 * @param trans Pointer to the transaction that was completed
 * @param arg   Unused user argument
 */
void IRAM_ATTR userTransactionCallback(spi_slave_transaction_t *trans, void *arg)
{
	digitalWrite(nSPI_READY, HIGH); // Transaction is done, SPI not ready
	digitalWrite(ATTN, HIGH);
	//
	UBaseType_t queue_count;
	queue_count = uxQueueMessagesWaiting(spi_comms_output_queue);
	MON_PRINTF("End Transaction - Queue count: %d\r\n", queue_count);
	if (is_tx == false) {
		xQueueSendFromISR(spi_comms_input_queue, (uint8_t *)&trans->rx_buffer, NULL);
	} else {
		uint8_t *message;
		MON_NL("TX transaction completed");
		//
		// For a TX transcation, remove the completed transaction from the output queue
		// and send a transaction completed message to the SPI task.
		xQueueReceiveFromISR(spi_comms_output_queue, &message, NULL); // Remove the completed transaction from the queue
		queue_count = uxQueueMessagesWaiting(spi_comms_output_queue);
		MON_PRINTF("Remove - Queue count: %d\r\n", queue_count);
		message = get_next_spi_buffer(); // Get a buffer for the transaction completed message
		memcpy(message, packet_transaction_completed, sizeof(packet_transaction_completed));
		xQueueSendFromISR(spi_comms_input_queue, &message, NULL);
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
	digitalWrite(nSPI_READY, LOW); // Tell ctxLink the transaction is ready to go.
}

/**
 * @brief Interrupt handler for the SPI CS input falling transition
 *
 *  If ATTN is asserted, set up a TX transaction using the saved txtransaction
 * packet.
 *
 *  Otherwise, set up an RX transaction
 *
 *  Do nothing if ESP32 is not ready!
 */
void spi_ss_activated(void)
{
	// control_esp32_ready(false); // De-assert ESP32 is ready
	if (system_setup_done) {
		if (digitalRead(ATTN) == LOW) { // Is this a TX transaction?
			// Set up a transaction to send the saved transaction buffer to ctxLink
			spi_create_pending_transaction(tx_saved_transaction, NULL,
				true); // This is a pending tx transaction
		} else {
			// Set up a transaction to receive data from ctxLink
			spi_create_pending_transaction(NULL, get_next_spi_buffer(),
				false); // This is a pending rx transaction
		}
	}
}

/**
 * @brief Initialize the SPI peripheral for ctxLink communication
 *
 */
void initCtxLink(void)
{
#ifdef DO_TOGGLE_PIN
	pinMode(PINA, OUTPUT);   // Set PINA as output
	pinMode(PINB, OUTPUT);   // Set PINB as output
	pinMode(PINC, OUTPUT);   // Set PINC as output
	digitalWrite(PINA, LOW); // Set PINA low
	digitalWrite(PINB, LOW); // Set PINB low
	digitalWrite(PINC, LOW); // Set PINC low
#endif
	// Set up the GPIO pins for ctxLink
	pinMode(nREADY, OUTPUT); // Set nREADY line to output
	digitalWrite(nREADY,
		HIGH);                      // Set nREADY line high to indicate ESP32 is not ready
	pinMode(nSPI_READY, OUTPUT);    // Set nSPI_READY line to output
	digitalWrite(nSPI_READY, HIGH); // Set nSPI_READY line high to indicate ESP32
									// SPI Transfer is not ready
	pinMode(ATTN, OUTPUT);          // Set ATTN line to output
	digitalWrite(ATTN, HIGH);       // Set ATTN line high to indicate ESP32 has no data
	// digitalWrite(SPI_SS_PIN, HIGH);
	pinMode(SPI_SS_PIN, INPUT_PULLUP); // Set SPI_SS_PIN line to input with pullup
	attachInterrupt(digitalPinToInterrupt(SPI_SS_PIN), spi_ss_activated,
		FALLING); // Attach interrupt to SPI_SS_PIN
	slave.setDataMode(SPI_MODE1);
	slave.setMaxTransferSize(BUFFER_SIZE); // default: 4092 bytes
	slave.setQueueSize(QUEUE_SIZE);        // default: 1

	// begin() after setting
	slave.begin(HSPI, SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_SS_PIN);
	slave.setUserPostSetupCbAndArg(userPostSetupCallback, NULL);
	slave.setUserPostTransCbAndArg(userTransactionCallback, NULL);
}

/**
 * @brief Create a SPI transaction with the ctxLink module
 *
 * @param dma_tx_buffer Pointer to the buffer containing data to be sent to ctxLink
 * @param dma_rx_buffer Pointer to the buffer where received data from ctxLink should be stored
 * @param isTx          Boolean indicating whether this is a TX transaction (true) or RX transaction (false)
 */
void spi_create_pending_transaction(uint8_t *dma_tx_buffer, uint8_t *dma_rx_buffer, bool isTx)
{
	//
	// TODO Move this line to the nCS processing? Seems like ALL transaction are
	// considered tx?
	//
	is_tx = isTx; // Set the transaction type

	slave.setUserPostSetupCbAndArg(userPostSetupCallback, NULL);
	slave.setUserPostTransCbAndArg(userTransactionCallback, NULL);
	//
	// Set up the transaction buffers depending upon the transfer direction
	//
	// Replace NULL pointers with pointers to buffers filled with zeroes.
	//
	// Note: Only a single zero-filled buffer is provided since one of the RX/TX
	// buffers must be valid!
	//
	// This keeps the buffers valid, even when not supplied by the caller
	//
	const uint8_t *tx_buf_to_use = (dma_tx_buffer == NULL) ? zero_transaction_buffer : dma_tx_buffer;
	uint8_t *rx_buf_to_use = (dma_rx_buffer == NULL) ? zero_transaction_buffer : dma_rx_buffer;
	slave.queue(tx_buf_to_use, rx_buf_to_use, BUFFER_SIZE);
	slave.trigger();
}

/**
 * @brief Indicate to ctxLink the ESP32 is ready
 *
 * Initially this is asserted once a wireless connection is made, however
 * in the future it may need to be asserted if there is no Wi-Fi connection.
 * This would enable ctxLink to configure the Wi-Fi.
 */
void control_esp32_ready(bool ready)
{
	digitalWrite(nREADY, ready ? LOW : HIGH);
}

/**
 * @brief A debug aid, not used as a part of the ctxLink Interface.
 * 
 */
void ctxlink_toggle_nReady(void)
{
	digitalWrite(nREADY, digitalRead(nREADY) == HIGH ? LOW : HIGH);
}