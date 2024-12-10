/*
 * serial_commands.h
 *
 *  Created on: Nov 16, 2024
 *      Author: etogb
 */

#ifndef INC_USB_DRIVER_H_
#define INC_USB_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h" // Replace with your STM32 series HAL header
#include "usbd_cdc_if.h"

// Constants
#define START_BYTE 0xAA
#define MAX_DATA_SIZE 8 //bytes
#define MAX_BUF_SIZE 13 //bytes
#define USB_FIFO_SIZE 16 //commands held in buffer. cleared after processed

#define START_IDX 0
#define SIZE_IDX 1
#define CMD1_IDX 2
#define CMD2_IDX 3
#define VAL1_IDX 4

#define RX_BUFFER_SIZE 30

// Command structure
typedef struct {
    uint16_t command; // Command ID
    uint8_t data[MAX_DATA_SIZE];   // data bytes
    uint8_t length; //length of data bytes
    bool invalid;
} USBCommand_t;

// USB Buffer structure
typedef struct {
    uint8_t length; // Command ID
    uint8_t buffer[MAX_BUF_SIZE];   // buffer read directly from USB input
} USBPacket_t;

typedef struct {
	uint8_t recieveIdx;
	uint8_t processIdx;
	uint8_t sendIdx;
	USBPacket_t rawPacket[USB_FIFO_SIZE];
	USBCommand_t packet[USB_FIFO_SIZE];
} USBCommandHandler_t;

// Function prototypes
void USB_Init();
void USB_HandleRecieve(uint8_t* Buf, uint32_t Len);
USBCommand_t USB_ProcessPacket(const USBPacket_t *packet);
USBCommand_t USB_GetCommand();
void USB_DoEvents();
void USB_Send(uint8_t *Buf, uint32_t Len);

#endif /* INC_USB_DRIVER_H_ */
