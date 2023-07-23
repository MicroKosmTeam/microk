#include "pci.h"

#include <cdefs.h>
#include <mkmi.h>
#include <mkmi_log.h>
#include <mkmi_memory.h>
#include <mkmi_syscall.h>

const char* DeviceClasses[] {
	"Unclassified",
		"Mass Storage Controller",
		"Network Controller",
		"Display Controller",
		"Multimedia Controller",
		"Memory Controller",
		"Bridge Device",
		"Simple Communication Controller",
		"Base System Peripheral",
		"Input Device Controller",
		"Docking Station", 
		"Processor",
		"Serial Bus Controller",
		"Wireless Controller",
		"Intelligent Controller",
		"Satellite Communication Controller",
		"Encryption Controller",
		"Signal Processing Controller",
		"Processing Accelerator",
		"Non Essential Instrumentation"
};

const char *GetVendorName(uint16_t VendorID) {
	switch (VendorID){
		case 0x8086:
			return "Intel Corp";
		case 0x1022:
			return "AMD";
		case 0x10DE:
			return "NVIDIA Corporation";
	}
	return "Unknown";
}

const char *GetDeviceName(uint16_t VendorID, uint16_t DeviceID){
	switch (VendorID) {
		case 0x8086: // Intel
			switch(DeviceID) {
				case 0x29C0:
					return "Express DRAM Controller";
				case 0x2918:
					return "LPC Interface Controller";
				case 0x2922:
					return "6 port SATA Controller [AHCI mode]";
				case 0x2930:
					return "SMBus Controller";
			}
	}
	return "Unknown";
}

const char* MassStorageControllerSubclassName(uint8_t SubclassCode){
	switch (SubclassCode){
		case 0x00:
			return "SCSI Bus Controller";
		case 0x01:
			return "IDE Controller";
		case 0x02:
			return "Floppy Disk Controller";
		case 0x03:
			return "IPI Bus Controller";
		case 0x04:
			return "RAID Controller";
		case 0x05:
			return "ATA Controller";
		case 0x06:
			return "Serial ATA";
		case 0x07:
			return "Serial Attached SCSI";
		case 0x08:
			return "Non-Volatile Memory Controller";
		case 0x80:
			return "Other";
	}
	return "Unknown";
}

const char* SerialBusControllerSubclassName(uint8_t SubclassCode){
	switch (SubclassCode){
		case 0x00:
			return "FireWire (IEEE 1394) Controller";
		case 0x01:
			return "ACCESS Bus";
		case 0x02:
			return "SSA";
		case 0x03:
			return "USB Controller";
		case 0x04:
			return "Fibre Channel";
		case 0x05:
			return "SMBus";
		case 0x06:
			return "Infiniband";
		case 0x07:
			return "IPMI Interface";
		case 0x08:
			return "SERCOS Interface (IEC 61491)";
		case 0x09:
			return "CANbus";
		case 0x80:
			return "SerialBusController - Other";
	}
	return "Unknown";
}

const char* BridgeDeviceSubclassName(uint8_t SubclassCode){
	switch (SubclassCode){
		case 0x00:
			return "Host Bridge";
		case 0x01:
			return "ISA Bridge";
		case 0x02:
			return "EISA Bridge";
		case 0x03:
			return "MCA Bridge";
		case 0x04:
			return "PCI-to-PCI Bridge";
		case 0x05:
			return "PCMCIA Bridge";
		case 0x06:
			return "NuBus Bridge";
		case 0x07:
			return "CardBus Bridge";
		case 0x08:
			return "RACEway Bridge";
		case 0x09:
			return "PCI-to-PCI Bridge";
		case 0x0a:
			return "InfiniBand-to-PCI Host Bridge";
		case 0x80:
			return "Other";
	}
	return "Unknown";
}

const char* GetSubclassName(uint8_t ClassCode, uint8_t SubclassCode){
	switch (ClassCode){
		case 0x01:
			return MassStorageControllerSubclassName(SubclassCode);
		case 0x03:
			switch (SubclassCode){
				case 0x00:
					return "VGA Compatible Controller";
			}
		case 0x06:
			return BridgeDeviceSubclassName(SubclassCode);
		case 0x0C:
			return SerialBusControllerSubclassName(SubclassCode);
	}
	return "Unknown";
}

const char* GetProgIFName(uint8_t ClassCode, uint8_t SubclassCode, uint8_t ProgIF){
	switch (ClassCode){
		case 0x01:
			switch (SubclassCode){
				case 0x06:
					switch (ProgIF){
						case 0x00:
							return "Vendor Specific Interface";
						case 0x01:
							return "AHCI 1.0";
						case 0x02:
							return "Serial Storage Bus";
					}
			}
		case 0x03:
			switch (SubclassCode){
				case 0x00:
					switch (ProgIF){
						case 0x00:
							return "VGA Controller";
						case 0x01:
							return "8514-Compatible Controller";
					}
			}
		case 0x0C:
			switch (SubclassCode){
				case 0x03:
					switch (ProgIF){
						case 0x00:
							return "UHCI Controller";
						case 0x10:
							return "OHCI Controller";
						case 0x20:
							return "EHCI (USB2) Controller";
						case 0x30:
							return "XHCI (USB3) Controller";
						case 0x80:
							return "Unspecified";
						case 0xFE:
							return "USB Device (Not a Host Controller)";
					}
			}    
	}
	return "Unknown";
}


void EnumerateFunction(uint64_t deviceAddress, uint64_t function){
	uint64_t offset = function << 12;

	uint64_t functionAddress = deviceAddress + offset;
	Syscall(SYSCALL_MEMORY_MMAP, functionAddress, functionAddress, 4096, 0, 0, 0);

	PCIDeviceHeader* pciDeviceHeader = (PCIDeviceHeader*)functionAddress;

	if (pciDeviceHeader->DeviceID == 0) return;
	if (pciDeviceHeader->DeviceID == 0xFFFF) return;

	MKMI_Printf(" - PCI Device:\r\n"
		    "   |- Vendor: %s (%x)\r\n"
		    "   |- Device: %s (%x)\r\n"
		    "   |- Class: %s\r\n"
		    "   |- Subclass: %s (%x)\r\n"
		    "   \\- Prog IF: %s (%x)\r\n"
		    "\r\n",
		    GetVendorName(pciDeviceHeader->VendorID),
		    pciDeviceHeader->VendorID,
		    GetDeviceName(pciDeviceHeader->VendorID, pciDeviceHeader->DeviceID),
		    pciDeviceHeader->DeviceID,
		    DeviceClasses[pciDeviceHeader->Class],
		    GetSubclassName(pciDeviceHeader->Class, pciDeviceHeader->Subclass),
		    pciDeviceHeader->Subclass,
		    GetProgIFName(pciDeviceHeader->Class, pciDeviceHeader->Subclass, pciDeviceHeader->ProgIF),
		    pciDeviceHeader->ProgIF);
}

void EnumerateDevice(uint64_t busAddress, uint64_t device){
	uint64_t offset = device << 15;

	uint64_t deviceAddress = busAddress + offset;
	Syscall(SYSCALL_MEMORY_MMAP, deviceAddress, deviceAddress, 4096, 0, 0, 0);

	PCIDeviceHeader* pciDeviceHeader = (PCIDeviceHeader*)deviceAddress;

	if (pciDeviceHeader->DeviceID == 0) return;
	if (pciDeviceHeader->DeviceID == 0xFFFF) return;

	for (uint64_t function = 0; function < 8; function++){
		EnumerateFunction(deviceAddress, function);
	}
}

void EnumerateBus(uint64_t baseAddress, uint64_t bus){
	uint64_t offset = bus << 20;

	uint64_t busAddress = baseAddress + offset;
	Syscall(SYSCALL_MEMORY_MMAP, busAddress, busAddress, 4096, 0, 0, 0);

	PCIDeviceHeader* pciDeviceHeader = (PCIDeviceHeader*)busAddress;

	if (pciDeviceHeader->DeviceID == 0) return;
	if (pciDeviceHeader->DeviceID == 0xFFFF) return;

	for (uint64_t device = 0; device < 32; device++){
		EnumerateDevice(busAddress, device);
	}
    }


void EnumeratePCI(MCFGHeader *mcfg) {
	int entries = ((mcfg->Header.Length) - sizeof(MCFGHeader)) / sizeof(DeviceConfig);

	MKMI_Printf("Enumerating the PCI bus...\r\n");
	for (int i = 0; i < entries; i++) {
		/* Get the PCI device config */
		DeviceConfig *newDeviceConfig = (DeviceConfig*)((uint64_t)mcfg + sizeof(MCFGHeader) + (sizeof(DeviceConfig) * i));
		for (uint64_t bus = newDeviceConfig->StartBus;
				bus < newDeviceConfig->EndBus;
				bus++) {
			EnumerateBus(newDeviceConfig->BaseAddress, bus);
		}
	}
}
