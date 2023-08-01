#pragma once
#include <stdint.h>
#include <dev/acpi/acpi.hpp>
#include <dev/dev.hpp>

namespace PCI {
        struct PCIDeviceHeader {
                uint16_t VendorID;
                uint16_t DeviceID;
                uint16_t Command;
                uint16_t Status;
                uint8_t RevisionID;
                uint8_t ProgIF;
                uint8_t Subclass;
                uint8_t Class;
                uint8_t CacheLineSize;
                uint8_t LatencyTimer;
                uint8_t HeaderType;
                uint8_t BIST;
        }__attribute__((packed));

        struct PCIHeader0 {
                PCIDeviceHeader Header;
                uint32_t BAR0;
                uint32_t BAR1;
                uint32_t BAR2;
                uint32_t BAR3;
                uint32_t BAR4;
                uint32_t BAR5;
                uint32_t CardbusCISPtr;
                uint16_t SubsystemVendorID;
                uint16_t SubsystemID;
                uint16_t ExpansionROMBaseAddr;
                uint8_t CapabilitiesPtr;
                uint8_t Rsv0;
                uint16_t Rsv1;
                uint32_t Rsv2;
                uint8_t InterruptLine;
                uint8_t InterruptPin;
                uint8_t MinGrant;
                uint8_t MaxLatency;
        }__attribute__((packed));

	class PCIFunction : public Device {
	public:
		PCIFunction(uint64_t deviceAddress, uint64_t function);

		bool Exists() { return exists; }
	private:
		bool exists = true;

		uint64_t deviceAddress;
		uint64_t function;

		uint64_t functionAddress;
	};

	class PCIDevice : public Device {
	public:
		PCIDevice(uint64_t busAddress, uint64_t device);

		PCIFunction *GetFunction(uint64_t id) { if(id > 8) return 0; return functions[id]; }
		bool Exists() { return exists; }
	private:
		PCIFunction *functions[8];
		bool exists = true;

		uint64_t busAddress;
		uint64_t device;

		uint64_t deviceAddress;
	};

	class PCIBus : public Device {
	public:
		PCIBus(uint64_t baseAddress, uint64_t bus);

		PCIDevice *GetDevice(uint64_t id) { if(id > 32) return 0; return devices[id]; }

		bool Exists() { return exists; }
	private:
		void EnumerateDevice(uint64_t busAddress, uint64_t device);

		PCIDevice *devices[32];
		bool exists = true;

		uint64_t baseAddress;
		uint64_t bus;

		uint64_t busAddress;
	};

	void EnumeratePCI(ACPI::MCFGHeader *mcfg, uint64_t highMap);
	PCIDeviceHeader *GetHeader();
}
