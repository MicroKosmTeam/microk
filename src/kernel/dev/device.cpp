#include <dev/device.h>
#include <mm/heap.h>

#define PREFIX "[DEV] "

Device *rootSerial; // Do not touch for debugging sake.

DeviceNode *rootNode;

DeviceNode *last;
uint64_t lastID;

void Init() {
	rootNode = new DeviceNode;
	rootNode->id = lastID++;
	rootNode->device = new Device();
	rootNode->next = NULL;

	last = rootNode;
}

uint64_t AddDevice(Device *device) {
	last->next = new DeviceNode;
	last->next-> id = lastID++;
	last->next->device = device;
	last->next->next = NULL;

	last = last->next;

	return lastID - 1;
}

Device *GetDevice(uint64_t id) {
	DeviceNode *dev = rootNode;

	while (dev != NULL) {
		if (dev->id == id) return dev->device;
		dev = dev->next;
	}

	return 0;
}

#include <sys/printk.h>
void PrintDevice() {
	DeviceNode *dev = rootNode;

	while (dev != NULL) {
		printk(PREFIX "Device ID: %d Major: %d Minor: %d\n", dev->id, dev->device->GetMajor(), dev->device->GetMinor());
		dev = dev->next;
	}
}
