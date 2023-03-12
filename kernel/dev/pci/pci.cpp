/*
   File: dev/pci/pci.cpp
*/

#include <mm/vmm.hpp>
#include <mm/memory.hpp>
#include <sys/printk.hpp>
#include <dev/pci/pci.hpp>

/* Variable that stores high memory mapping offset for our convenience */
static uint64_t hhdm;

namespace PCI {
PCIDeviceHeader *ahciHeader;

PCIDeviceHeader *GetHeader() {
	return ahciHeader;
}

/* Function that passes through every PCI bus and initializes its driver */
void EnumeratePCI(ACPI::MCFGHeader *mcfg, uint64_t highMap) {
	hhdm = highMap;
	int entries = ((mcfg->Header.Length) - sizeof(ACPI::MCFGHeader)) / sizeof(ACPI::DeviceConfig);

	PRINTK::PrintK("Enumerating the PCI bus...\r\n");
	for (int i = 0; i < entries; i++) {
		/* Get the PCI device config */
		ACPI::DeviceConfig *newDeviceConfig = (ACPI::DeviceConfig*)((uint64_t)mcfg + sizeof(ACPI::MCFGHeader) + (sizeof(ACPI::DeviceConfig) * i));

		/* Now, there will be up to 256 PCI buses in a system. Usually there will be only one.
		   For now we limit the maximum number of buses to 1, we can change it later */
		for (uint64_t bus = newDeviceConfig->StartBus;
		     bus < newDeviceConfig->EndBus /* TODO: Do more than one bus */ && bus < 1;
		     bus++) {
			/* Initialize the PCI bus driver */
			PCIBus *newBus = new PCIBus(newDeviceConfig->BaseAddress, 0);

			/* If it doesn't exist we delete it */
			if(!newBus->Exists()) {
				delete newBus;
			} else {
				/* Device managmant */
				/* TODO: Do something */
				newBus->SetMajor(1);
				newBus->SetMinor(0);

			}
		}
	}
}

PCIBus::PCIBus(uint64_t baseAddress, uint64_t bus) {
	this->baseAddress = baseAddress;
	this->bus = bus;

	uint64_t offset = bus << 20;
	busAddress = baseAddress + offset;

	//VMM::MapMemory((void*)busAddress + hhdm, (void*)busAddress);

	PCIDeviceHeader *pciDeviceHeader = (PCIDeviceHeader*)busAddress;

	if(pciDeviceHeader->DeviceID == 0) {
		exists = false;
		return;
	}

	if(pciDeviceHeader->DeviceID == 0xFFFF) {
		exists = false;
		return;
	}

	for (uint64_t device = 0; device < 32; device++) {
		devices[device] = new PCIDevice(busAddress, device);

		if (!devices[device]->Exists()) {
			delete devices[device];
			devices[device] = NULL;
		} else {
			devices[device]->SetMajor(1);
			devices[device]->SetMinor(1);
		}
	}
}

PCIDevice::PCIDevice(uint64_t busAddress, uint64_t device) {
	this->busAddress = busAddress;
	this->device = device;

	uint64_t offset = device << 15;

	uint64_t deviceAddress = busAddress + offset;
	//VMM::MapMemory((void*)deviceAddress + hhdm, (void*)deviceAddress);

	PCIDeviceHeader *pciDeviceHeader = (PCIDeviceHeader*)deviceAddress;

	if(pciDeviceHeader->DeviceID == 0) {
		exists = false;
		return;
	}

	if(pciDeviceHeader->DeviceID == 0xFFFF) {
		exists = false;
		return;
	}

	for (uint64_t function = 0; function < 8; function++) {
		functions[function] = new PCIFunction(deviceAddress, function);

		if (!functions[function]->Exists()) {
			delete functions[function];
			functions[function] = NULL;
		}
	}
}

#include <sys/driver.hpp>
PCIFunction::PCIFunction(uint64_t deviceAddress, uint64_t function) {
	this->deviceAddress = deviceAddress;
	this->function = function;

	uint64_t offset = function<< 12;

	functionAddress = deviceAddress + offset;
	//VMM::MapMemory((void*)functionAddress + hhdm, (void*)functionAddress);

	PCIDeviceHeader *pciDeviceHeader = (PCIDeviceHeader*)functionAddress;

	if(pciDeviceHeader->DeviceID == 0) {
		exists = false;
		return;
	}

	if(pciDeviceHeader->DeviceID == 0xFFFF) {
		exists = false;
		return;
	}

	PRINTK::PrintK("PCI device: 0x%x - 0x%x - 0x%x - 0x%x\r\n",
			pciDeviceHeader->VendorID,
			pciDeviceHeader->DeviceID,
			pciDeviceHeader->Subclass,
			pciDeviceHeader->ProgIF);

	switch (pciDeviceHeader->Class) {
                case 0x01: //Mass storage
                        switch (pciDeviceHeader->Subclass) {
                                case 0x06: //Serial Ata
                                        switch (pciDeviceHeader->ProgIF) {
                                                case 0x01: // AHCI 1.0 device
                                                        // save this pciDeviceHeader;
							PRINTK::PrintK("AHCI 1.0 device.\r\n");
							ahciHeader = new PCIDeviceHeader;
							memcpy(ahciHeader, pciDeviceHeader, sizeof(PCIDeviceHeader));
							break;
                                        }
                        }
			break;
	}

	return;
}
}
