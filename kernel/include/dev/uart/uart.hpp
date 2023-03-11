#pragma once
#include <cdefs.h>
#ifdef CONFIG_HW_SERIAL
#include <stdint.h>
#include <stddef.h>
#include <dev/dev.hpp>

enum SerialPorts {
	COM1 = 0x3f8,
	COM2 = 0x2f8,
	COM3 = 0x3e8,
	COM4 = 0x2e8,
	COM5 = 0x5f8,
	COM6 = 0x4f8,
	COM7 = 0x5e8,
	COM8 = 0x4e8
};

class UARTDevice : public Device {
public:
	UARTDevice() { active = false; }

	uint64_t Ioctl(uint64_t request, va_list ap);

	uint64_t Init(SerialPorts serialPort);
	void PutStr(const char* str);
	void PutChar(const char ch);
	int GetChar();
private:
	int port;

	bool active;

	int isTransmitEmpty();
	int serialReceived();
};

#endif
