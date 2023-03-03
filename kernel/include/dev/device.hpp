#pragma once
#include <dev/dev.hpp>

struct DeviceNode {
	uint64_t id;
	Device *device;
	DeviceNode *next;
};

void Init();
uint64_t AddDevice(Device *device);
Device *GetDevice(uint64_t id);

void PrintDevice();
