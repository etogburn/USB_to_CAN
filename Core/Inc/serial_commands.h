/*
 * serial_commands.h
 *
 *  Created on: Nov 16, 2024
 *      Author: etogb
 */

#ifndef INC_SERIAL_COMMANDS_H_
#define INC_SERIAL_COMMANDS_H_

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h" // Replace with your STM32 series HAL header
#include "USB_driver.h"

// Constants
#define START_BYTE 0xAA
#define PACKET_SIZE 6

#define RX_BUFFER_SIZE 30

// Command structure
typedef struct {
    uint16_t command; // Command ID
    int16_t value;   // Associated value
    bool newCommand;
    bool invalid;
} SerialCommand_t;

// Expose the current command to the main loop
extern SerialCommand_t current_command;

// Function prototypes
void SerialCommands_Init(UART_HandleTypeDef *huart);
void SerialCommands_HandleUARTInterrupt(void);
void SerialCommands_SetupRecieve(void);
void SerialCommands_Send(uint16_t command, int16_t value);
void SerialCommands_PacketSend(USBCommand_t cmd);
void SerialCommands_BigSend(uint8_t *input, uint8_t length);

#endif /* INC_SERIAL_COMMANDS_H_ */
