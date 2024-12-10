#include <USB_driver.h>

static USBCommandHandler_t usb;

static void LED_Toggle() {
	HAL_GPIO_TogglePin(nLED_GPIO_Port,nLED_Pin);
}

//static void LED_On() {
//	HAL_GPIO_WritePin(nLED_GPIO_Port,nLED_Pin, GPIO_PIN_RESET);
//}

static void LED_Off() {
	HAL_GPIO_WritePin(nLED_GPIO_Port,nLED_Pin, GPIO_PIN_SET);
}

static void USB_ClearBuffer(uint8_t idx) {
	if(idx >= USB_FIFO_SIZE) return;

	usb.rawPacket[idx].length = 0;
	memset(usb.rawPacket[idx].buffer, 0, MAX_BUF_SIZE);

}

static void USB_ClearPacket(uint8_t idx) {
	if(idx >= USB_FIFO_SIZE) return;

	usb.packet[idx].command = 0;
	usb.packet[idx].invalid = true;
	usb.packet[idx].length = 0;

	memset(usb.packet[idx].data, 0, MAX_DATA_SIZE);
}

void USB_Init() {
	for(uint8_t i = 0; i < USB_FIFO_SIZE; i++) {
		USB_ClearBuffer(i);
		USB_ClearPacket(i);
	}

	usb.recieveIdx = 0;
	usb.processIdx = 0;
	usb.sendIdx = 0;
	LED_Off();
}

void USB_HandleRecieve(uint8_t *Buf, uint32_t Len) {
	memcpy(usb.rawPacket[usb.recieveIdx].buffer, Buf, Len);
	usb.rawPacket[usb.recieveIdx].length = Len;

	usb.recieveIdx++;
	if(usb.recieveIdx >= USB_FIFO_SIZE) usb.recieveIdx = 0;
	LED_Toggle();
}

USBCommand_t USB_ProcessPacket(const USBPacket_t *packet) {
	USBCommand_t cmd = {
		.command = 0,
		.length = 0, //length of data bytes
		.invalid = false
	};

	memset(cmd.data, 0, MAX_DATA_SIZE);

	uint8_t checksum = packet->buffer[0];
	uint8_t length = packet->length;

	if(packet->buffer[START_IDX] != START_BYTE) {
		cmd.invalid = true;
	}

	if(packet->buffer[SIZE_IDX] > MAX_DATA_SIZE) {
		cmd.invalid = true;
	}

	for(uint8_t i = 1; i < length - 1; i++) {
		checksum ^= packet->buffer[i];
	}

	if(checksum != packet->buffer[length-1]) {
		cmd.invalid = true;
	}

	cmd.length = packet->buffer[SIZE_IDX];
	cmd.command = packet->buffer[CMD1_IDX] << 8
				| packet->buffer[CMD2_IDX];
	if(cmd.length != 0) memcpy(cmd.data, &packet->buffer[VAL1_IDX], cmd.length);

	return cmd;
}

void USB_DoEvents() {

	if(usb.recieveIdx == usb.processIdx) return;

	usb.packet[usb.processIdx] = USB_ProcessPacket(&usb.rawPacket[usb.processIdx]);
	USB_ClearBuffer(usb.processIdx);

	usb.processIdx++;
	if(usb.processIdx >= USB_FIFO_SIZE) usb.processIdx = 0;
}

USBCommand_t USB_GetCommand() {
	USBCommand_t cmd = {
		.invalid = true
	};

	uint8_t idx = usb.sendIdx;

	if(idx == usb.processIdx) return cmd;

	usb.sendIdx++;
	if(usb.sendIdx >= USB_FIFO_SIZE) usb.sendIdx = 0;

	return usb.packet[idx];
}

void USB_Send(uint8_t *Buf, uint32_t Len) {
	CDC_Transmit_FS(Buf, Len);
}





