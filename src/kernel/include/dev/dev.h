#pragma once
#include <stdint.h>
#include <stddef.h>

class Device {
public:
	Device() {}
	virtual uint64_t ioctl(uint64_t request, ...) { return request; }
private:

};

