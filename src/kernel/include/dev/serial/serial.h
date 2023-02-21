#pragma once
#include <cdefs.h>
#include <stdint.h>
#include <stddef.h>

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

class SerialPort {
public:
	SerialPort() { active = false; }
	SerialPort(SerialPorts serialPort);

	void PrintStr(const char* str);
	void PrintChar(const char ch);
private:
	int port;

	bool active;

	int isTransmitEmpty();
};

