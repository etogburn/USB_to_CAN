/*
 * Packet_Handler.h
 *
 *  Created on: Dec 9, 2024
 *      Author: etogb
 */

#ifndef INC_PACKET_HANDLER_H_
#define INC_PACKET_HANDLER_H_

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h" // Replace with your STM32 series HAL header

#define START_BYTE 0xAA
#define MAX_DATA_SIZE 8 //bytes
#define MAX_BUF_SIZE 13 //bytes

#define START_IDX 0
#define SIZE_IDX 1
#define CMD1_IDX 2
#define CMD2_IDX 3
#define VAL1_IDX 4

typedef struct {
    uint16_t command; // Command ID
    uint8_t data[MAX_DATA_SIZE];   // data bytes
    uint8_t length; //length of data bytes
    bool invalid;
} DecodedPacket_t;

typedef struct {
    uint8_t length;
    uint8_t data[MAX_BUF_SIZE];   // buffer read directly from USB/Serial input
} StringBuffer_t;

void PacketHandler_Encode(DecodedPacket_t *packet, StringBuffer_t *buffer);
void PacketHandler_Decode(DecodedPacket_t *packet, StringBuffer_t *buffer);

#endif /* INC_PACKET_HANDLER_H_ */
