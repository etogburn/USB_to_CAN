#include "serial_commands.h"

// Static variables for internal use


static UART_HandleTypeDef *serial_huart;     // UART handle
uint8_t rx_buffer[RX_BUFFER_SIZE];       // Buffer for receiving packets


// Exposed command structure
SerialCommand_t current_command = {0, 0, false, false};   // Initialize to zero

// Initialize the serial commands system
void SerialCommands_Init(UART_HandleTypeDef *huart) {
    serial_huart = huart;                     // Save the UART handle
    current_command.command = 0;             // Clear command structure
    current_command.value = 0;
    // Start UART reception in interrupt mode
    SerialCommands_SetupRecieve();

}

// Process a complete packet
static void SerialCommands_ProcessPacket(uint8_t *packet) {
    // Validate the start byte
	current_command.invalid = false;

    if (packet[0] != START_BYTE) {
    	current_command.invalid = true; // Invalid start byte, discard packet
    }

    // Validate the checksum
    uint8_t checksum = packet[0] ^ packet[1] ^ packet[2] ^ packet[3] ^ packet[4];
    if (checksum != packet[5]) {
    	current_command.invalid = true;
        //return; // Invalid checksum, discard packet
    }

    // Populate the command structure
    current_command.command = (packet[1] << 8) | packet[2];
    current_command.value = (packet[3] << 8) | packet[4];
    current_command.newCommand = true;

}

// Handle the UART interrupt callback
void SerialCommands_HandleUARTInterrupt(void) {

	//HAL_UART_Transmit(serial_huart, rx_buffer, PACKET_SIZE, 0xFFFF);
	SerialCommands_ProcessPacket(rx_buffer);

	// Restart UART reception
	SerialCommands_SetupRecieve();
}

void SerialCommands_SetupRecieve(void) {

	HAL_UARTEx_ReceiveToIdle_DMA(serial_huart, rx_buffer, RX_BUFFER_SIZE);
//	if(HAL_UART_Receive_IT(serial_huart, rx_buffer, PACKET_SIZE) == HAL_OK) {
//	}
//	else if (HAL_UART_Receive_IT(serial_huart, rx_buffer, PACKET_SIZE) == HAL_BUSY) {
//		HAL_UART_Receive_IT(serial_huart, rx_buffer, PACKET_SIZE);
//	}
//	else if (HAL_UART_Receive_IT(serial_huart, rx_buffer, PACKET_SIZE) == HAL_ERROR) {
//		HAL_UART_Init(serial_huart);
//		HAL_UART_Receive_IT(serial_huart, rx_buffer, PACKET_SIZE);
//	}
}

void SerialCommands_PacketSend(USBCommand_t cmd) {

	if(cmd.invalid) return;

	uint8_t tx_header[5+cmd.length];
	tx_header[0] = START_BYTE;
	tx_header[1] = cmd.length;
	tx_header[2] = (cmd.command & 0xFF00) >> 8;
	tx_header[3] = cmd.command & 0x00FF;

	for(uint8_t i = 4; i < (4+cmd.length); i++) {
		tx_header[i] = cmd.data[i-4];
	}

	uint8_t checksum = tx_header[0];
	for(uint8_t i = 1; i < (4+cmd.length); i++) {
		checksum ^= tx_header[i];
	}

	tx_header[4+cmd.length] = checksum;

	HAL_UART_Transmit(serial_huart, tx_header, (5+cmd.length), 0xFFFF);
}

void SerialCommands_Send(uint16_t command, int16_t value) {
	uint8_t tx_buffer[PACKET_SIZE];

	tx_buffer[0] = START_BYTE;
	tx_buffer[1] = (command & 0xFF00) >> 8;
	tx_buffer[2] = (command & 0x00FF);
	tx_buffer[3] = (value & 0xFF00) >> 8;
	tx_buffer[4] = (value & 0x00FF);
	tx_buffer[5] = tx_buffer[0] ^ tx_buffer[1] ^ tx_buffer[2] ^ tx_buffer[3] ^ tx_buffer[4];

	HAL_UART_Transmit(serial_huart, tx_buffer, PACKET_SIZE, 0xFFFF);
}

void SerialCommands_BigSend(uint8_t *input, uint8_t length) {
	HAL_UART_Transmit(serial_huart, input, length, 0xFFFF);
}


