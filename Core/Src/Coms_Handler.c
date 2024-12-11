/*
 * Coms_Handler.c
 *
 *  Created on: Dec 9, 2024
 *      Author: etogb
 */

#include "Coms_Handler.h"
#include "usbd_cdc_if.h"


static void ComsHandler_PacketToBuf(DecodedPacket_t *packet, StringBuffer_t *buffer) {

	if(packet->invalid) return;
	buffer->length = 5 + packet->length;

	buffer->data[0] = START_BYTE;
	buffer->data[1] = packet->length;
	buffer->data[2] = (packet->command & 0xFF00) >> 8;
	buffer->data[3] = packet->command & 0x00FF;

	for(uint8_t i = 4; i < (buffer->length - 1); i++) {
		buffer->data[i] = packet->data[i-4];
	}

	uint8_t checksum = buffer->data[0];
	for(uint8_t i = 1; i < (buffer->length - 1); i++) {
		checksum ^= buffer->data[i];
	}

	buffer->data[buffer->length - 1] = checksum;
}

static void ComsHandler_BufToPacket(DecodedPacket_t *packet, StringBuffer_t *buffer) {
	uint8_t length = 0;

	packet->invalid = false;

    if (buffer->data[0] != START_BYTE) {
    	packet->invalid = true; // Invalid start byte, discard packet
    }

    packet->length = buffer->data[1];
    if (buffer->data[1] > MAX_DATA_SIZE) {
		packet->invalid = true;
		packet->length = 0;
	}

    for(uint8_t i = (MAX_BUF_SIZE - 1); i > 1 ; i--) {
    	if(buffer->data[i] != 0) {
    		length = i;
    		break;
    	}
    }

    uint8_t checksum = buffer->data[0];
    for(uint8_t i = 1; i < length; i++) {
		checksum ^= buffer->data[i];
	}

	if(checksum != buffer->data[length]) {
		packet->invalid = true;
	}

    // Populate the command structure
    packet->command = (buffer->data[2] << 8) | buffer->data[3];

    if(packet->length != 0) {
    	for(uint8_t i = 0; i < packet->length; i++) {
    		packet->data[i] = buffer->data[4+i];
    	}
    }
}

// UART Send Function
static HAL_StatusTypeDef UART_Send(void *config, uint8_t *data, uint16_t length)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)config;
    return HAL_UART_Transmit(huart, data, length, HAL_MAX_DELAY);
}

// UART Receive Function
static HAL_StatusTypeDef UART_Receive(void *config, uint8_t *data, uint16_t length)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)config;
    return HAL_UART_Receive(huart, data, length, HAL_MAX_DELAY);
}

// USB Send Function
static HAL_StatusTypeDef USB_Send(void *config, uint8_t *data, uint16_t length)
{
    if (CDC_Transmit_FS(data, length) == USBD_OK)
        return HAL_OK;
    else
        return HAL_ERROR;
}

// USB Receive Function (not implemented for USB CDC)
static HAL_StatusTypeDef USB_Receive(void *config, uint8_t *data, uint16_t length)
{

    return HAL_OK;
}

// CAN Send Function
static HAL_StatusTypeDef CAN_Send(void *config, uint8_t *data, uint16_t length)
{
//    FDCAN_HandleTypeDef *hfdcan = (FDCAN_HandleTypeDef *)config;
//
//    if (length < 5 || data[1] > 8 || length != (5 + data[1])) {
//        return HAL_ERROR;  // Invalid DLC or data length
//    }
//
//    FDCAN_TxHeaderTypeDef txHeader;
//    txHeader.Identifier = (data[2] << 8) | data[3];  // Use bytes 3 and 4 as header
//    txHeader.IdType = FDCAN_STANDARD_ID;
//    txHeader.TxFrameType = FDCAN_DATA_FRAME;
//    txHeader.DataLength = (data[1] << 16);  // DLC in byte 2
//    txHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
//    txHeader.BitRateSwitch = FDCAN_BRS_OFF;
//    txHeader.FDFormat = FDCAN_CLASSIC_CAN;
//    txHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
//    txHeader.MessageMarker = 0;
//
//    // Only include data bytes 5 to N-1
//    return HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &txHeader, &data[4]);

	return HAL_OK;
}

// CAN Receive Function
static HAL_StatusTypeDef CAN_Receive(void *config, uint8_t *data, uint16_t length)
{
//    FDCAN_HandleTypeDef *hfdcan = (FDCAN_HandleTypeDef *)config;
//    FDCAN_RxHeaderTypeDef rxHeader;
//    uint8_t canData[8];
//
//    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rxHeader, canData) != HAL_OK) {
//        return HAL_ERROR;
//    }
//
//    uint8_t dlc = (rxHeader.DataLength >> 16) & 0xF;
//    if (length < (5 + dlc)) {
//        return HAL_ERROR;  // Ensure buffer is large enough
//    }
//
//    // Populate received data in the specified format
//    data[0] = 0;                 // Reserved for future use
//    data[1] = dlc;               // DLC
//    data[2] = (rxHeader.Identifier >> 8) & 0xFF; // Header byte 1
//    data[3] = rxHeader.Identifier & 0xFF;       // Header byte 2
//    for (uint8_t i = 0; i < dlc; i++) {
//        data[4 + i] = canData[i];
//    }

    return HAL_OK;
}

// Initialize Communication Instance
void Comm_Init(ComsInterface_t *instance, CommType type, void *config)
{
    instance->type = type;
    instance->config = config;

    if (type == COMM_UART) {
        instance->interface.Send = UART_Send;
        instance->interface.Receive = UART_Receive;
    } else if (type == COMM_USB) {
        instance->interface.Send = USB_Send;
        instance->interface.Receive = USB_Receive;
    } else if (type == COMM_CAN) {
        instance->interface.Send = CAN_Send;
        instance->interface.Receive = CAN_Receive;
    }
}

// Wrapper for Sending Data
HAL_StatusTypeDef Comm_Send(ComsInterface_t *instance, uint8_t *data, uint16_t length)
{
    return instance->interface.Send(instance->config, data, length);
}

// Wrapper for Receiving Data
HAL_StatusTypeDef Comm_Receive(ComsInterface_t *instance, uint8_t *data, uint16_t length)
{
    return instance->interface.Receive(instance->config, data, length);
}
