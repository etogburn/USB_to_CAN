/*
 * Packet_Handler.c
 *
 *  Created on: Dec 9, 2024
 *      Author: etogb
 */
#include "Packet_Handler.h"

void PacketHandler_Encode(DecodedPacket_t *packet, StringBuffer_t *buffer) {

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

void PacketHandler_Decode(DecodedPacket_t *packet, StringBuffer_t *buffer) {
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
