#pragma once
#include <stdint.h>

namespace x86_64 {
	void OutB(uint16_t port, uint8_t val);
	void OutW(uint16_t port, uint16_t val);
	uint8_t InB(uint16_t port);
	uint16_t InW(uint16_t port);
	void IoWait(void);
}
