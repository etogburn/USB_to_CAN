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

#define CANSPI
//#define FDCAN

#define START_BYTE 0xAA
#define MAX_DATA_SIZE 8 //bytes
#define MAX_BUF_SIZE 13 //bytes

#define START_IDX 0
#define SIZE_IDX 1
#define CMD1_IDX 2
#define CMD2_IDX 3
#define VAL1_IDX 4

#define FIFO_SIZE 8
// Define communication types
typedef enum {
    COMM_UART,
    COMM_USB,
    COMM_CAN
} CommType;

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

// Communication function pointers
typedef struct {
    HAL_StatusTypeDef (*Send)(void *config, DecodedPacket_t *packet);
    HAL_StatusTypeDef (*Receive)(void *inst, uint8_t *data, uint16_t length);
    void (*ConvertToPacket)(DecodedPacket_t *packet, void *buffer);
} CommInterface;

typedef struct {
	uint8_t rxIdx;
	uint8_t decodeIdx;
	uint8_t processIdx;
	DecodedPacket_t rxPacket[FIFO_SIZE];
	StringBuffer_t rxBuf[FIFO_SIZE];
    CommType type;
    CommInterface interface;
    void *config;  // Pointer to configuration (e.g., UART, USB, or FDCAN handle)
} ComsInterface_t;

// Public API
void Comm_Init(ComsInterface_t *instance, CommType type, void *config);
HAL_StatusTypeDef Comm_Send(ComsInterface_t *instance, DecodedPacket_t *packet);
HAL_StatusTypeDef Comm_Receive(ComsInterface_t *instance, uint8_t *data, uint16_t length);
void Comm_Process(ComsInterface_t *instance);
DecodedPacket_t Comm_GetPacket(ComsInterface_t *instance);




#endif /* INC_COMS_HANDLER_H_ */
