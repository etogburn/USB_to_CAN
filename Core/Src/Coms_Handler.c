/*
 * Coms_Handler.c
 *
 *  Created on: Dec 9, 2024
 *      Author: etogb
 */

#include "Coms_Handler.h"
#include "usbd_cdc_if.h"
#include "usart.h"

static void Coms_IncIdx(uint8_t *idx) {
	(*idx)++;
	if(*idx >= FIFO_SIZE) (*idx) = 0;
}

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

static void ComsHandler_BufToPacket(DecodedPacket_t *packet, void *buf) {

	StringBuffer_t *buffer = (StringBuffer_t *)buf;

	uint8_t length = 0;

	memset(packet->data, 0, MAX_DATA_SIZE);

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
static HAL_StatusTypeDef UART_Send(void *config, DecodedPacket_t *packet)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)config;

    StringBuffer_t buf;

    ComsHandler_PacketToBuf(packet, &buf);

    return HAL_UART_Transmit(huart, buf.data, buf.length, HAL_MAX_DELAY);
}

// UART Receive Function
//All data will be recieved in interrupt callback. this function will place the data in the appropriate buffer

static HAL_StatusTypeDef UART_SetupReceive(void *inst)
{
	ComsInterface_t *instance = (ComsInterface_t *)inst;
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)instance->config;

    memset(instance->rxBuf[instance->rxIdx].data, 0, MAX_BUF_SIZE);
    HAL_UARTEx_ReceiveToIdle_DMA(huart, instance->rxBuf[instance->rxIdx].data, MAX_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);

    return HAL_OK;
}

static HAL_StatusTypeDef UART_Receive(void *inst, uint8_t *data, uint16_t length)
{
	ComsInterface_t *instance = (ComsInterface_t *)inst;
    //UART_HandleTypeDef *huart = (UART_HandleTypeDef *)instance->config;

    instance->rxBuf[instance->rxIdx].length = length;

    Coms_IncIdx(&instance->rxIdx);

    UART_SetupReceive(inst);
    //__HAL_DMA_DISABLE_IT(hdma_usart1_rx, DMA_IT_HT);

    return HAL_OK;
}



// USB Send Function
static HAL_StatusTypeDef USB_Send(void *config, DecodedPacket_t *packet)
{

	StringBuffer_t buf;

	ComsHandler_PacketToBuf(packet, &buf);

    if (CDC_Transmit_FS(buf.data, buf.length) == USBD_OK)
        return HAL_OK;
    else
        return HAL_ERROR;
}

// USB Receive Function (not implemented for USB CDC)
//All data will be recieved in interrupt callback. this function will place the data in the appropriate buffer
static HAL_StatusTypeDef USB_Receive(void *inst, uint8_t *data, uint16_t length)
{
	ComsInterface_t *instance = (ComsInterface_t *)inst;

	instance->rxBuf[instance->rxIdx].length = length;
	memset(instance->rxBuf[instance->rxIdx].data, 0, MAX_BUF_SIZE);
	memcpy(instance->rxBuf[instance->rxIdx].data, data, length);

	Coms_IncIdx(&instance->rxIdx);

    return HAL_OK;
}

// CAN Send Function
static HAL_StatusTypeDef CAN_Send(void *config, DecodedPacket_t *packet)
{

#ifdef FDCAN
    FDCAN_HandleTypeDef *hfdcan = (FDCAN_HandleTypeDef *)config;

    if (length < 5 || data[1] > 8 || length != (5 + data[1])) {
        return HAL_ERROR;  // Invalid DLC or data length
    }

    FDCAN_TxHeaderTypeDef txHeader;
    txHeader.Identifier = (data[2] << 8) | data[3];  // Use bytes 3 and 4 as header
    txHeader.IdType = FDCAN_STANDARD_ID;
    txHeader.TxFrameType = FDCAN_DATA_FRAME;
    txHeader.DataLength = (data[1] << 16);  // DLC in byte 2
    txHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    txHeader.BitRateSwitch = FDCAN_BRS_OFF;
    txHeader.FDFormat = FDCAN_CLASSIC_CAN;
    txHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    txHeader.MessageMarker = 0;

    // Only include data bytes 5 to N-1
    return HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &txHeader, &data[4]);
#endif

	return HAL_OK;
}

// CAN Receive Function
//going to be handled in an interrupt as with the all receive functions
static HAL_StatusTypeDef CAN_Receive(void *config, uint8_t *data, uint16_t length)
{

#ifdef FDCAN
    FDCAN_HandleTypeDef *hfdcan = (FDCAN_HandleTypeDef *)config;
    FDCAN_RxHeaderTypeDef rxHeader;
    uint8_t canData[8];

    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rxHeader, canData) != HAL_OK) {
        return HAL_ERROR;
    }

    uint8_t dlc = (rxHeader.DataLength >> 16) & 0xF;
    if (length < (5 + dlc)) {
        return HAL_ERROR;  // Ensure buffer is large enough
    }

    // Populate received data in the specified format
    data[0] = 0;                 // Reserved for future use
    data[1] = dlc;               // DLC
    data[2] = (rxHeader.Identifier >> 8) & 0xFF; // Header byte 1
    data[3] = rxHeader.Identifier & 0xFF;       // Header byte 2
    for (uint8_t i = 0; i < dlc; i++) {
        data[4 + i] = canData[i];
    }
#endif
    return HAL_OK;
}

// Initialize Communication Instance
void Comm_Init(ComsInterface_t *instance, CommType type, void *config)
{
    instance->type = type;
    instance->config = config;
    instance->decodeIdx = 0;
    instance->rxIdx = 0;
    instance->processIdx = 0;

    for(uint8_t i = 0; i < FIFO_SIZE; i++) {
    	instance->rxPacket[i].invalid = true;
    }

    if (type == COMM_UART) {
        instance->interface.Send = UART_Send;
        instance->interface.Receive = UART_Receive;
        UART_SetupReceive(instance);
    } else if (type == COMM_USB) {
        instance->interface.Send = USB_Send;
        instance->interface.Receive = USB_Receive;
    } else if (type == COMM_CAN) {
        instance->interface.Send = CAN_Send;
        instance->interface.Receive = CAN_Receive;
    }

    if (type == COMM_UART || type == COMM_USB) {
    	instance->interface.ConvertToPacket = ComsHandler_BufToPacket;
    } else if (type == COMM_CAN) {

    }
}

// Wrapper for Sending Data
HAL_StatusTypeDef Comm_Send(ComsInterface_t *instance, DecodedPacket_t *packet)
{
	if(packet->invalid) return HAL_ERROR;

    return instance->interface.Send(instance->config, packet);
}

// Wrapper for Receiving Data
HAL_StatusTypeDef Comm_Receive(ComsInterface_t *instance, uint8_t *data, uint16_t length)
{
    return instance->interface.Receive(instance, data, length);
}

void Comm_Process(ComsInterface_t *instance) {

	if(instance->decodeIdx == instance->rxIdx) return;
	instance->interface.ConvertToPacket(&instance->rxPacket[instance->decodeIdx],&instance->rxBuf[instance->decodeIdx]);
	Coms_IncIdx(&instance->decodeIdx);
	Comm_Process(instance);
}

DecodedPacket_t Comm_GetPacket(ComsInterface_t *instance) {
	DecodedPacket_t invalidPacket = {
			.invalid = true
	};
	if(instance->decodeIdx == instance->processIdx) {
		return invalidPacket;
	}
	uint8_t idx = instance->processIdx;

	Coms_IncIdx(&instance->processIdx);

	return instance->rxPacket[idx];
}


