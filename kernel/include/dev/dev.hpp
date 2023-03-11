#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

class Device {
public:
	Device() { }
	~Device() { }

	virtual uint64_t Ioctl(uint64_t request, va_list ap) { return request; }

	uint64_t GetMajor() { return major; }
	uint64_t GetMinor() { return minor; }

	void SetMajor(uint64_t major) { this->major = major; }
	void SetMinor(uint64_t minor) { this->minor = minor; }
private:
	uint64_t type = 0;
	uint64_t major = 0;
	uint64_t minor = 0;
};
