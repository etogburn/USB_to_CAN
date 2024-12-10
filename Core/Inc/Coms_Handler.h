/*
 * Coms_Handler.h
 *
 *  Created on: Dec 9, 2024
 *      Author: etogb
 */

#ifndef INC_COMS_HANDLER_H_
#define INC_COMS_HANDLER_H_

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "Packet_Handler.h"

#define FIFO_SIZE 8

typedef struct {
	uint8_t rxIdx;
	uint8_t decodeIdx;
	uint8_t processIdx;
	DecodedPacket_t rxPacket[FIFO_SIZE];
	StringBuffer_t rxBuf[FIFO_SIZE];
} Coms_Interface_t;



#endif /* INC_COMS_HANDLER_H_ */
