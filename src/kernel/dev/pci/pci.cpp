#include <dev/pci/pci.h>
#include <mm/vmm.h>
#include <sys/printk.h>
#include <mm/heap.h>

#define PREFIX "[PCI] "

namespace PCI {
	PCIDevice::PCIDevice(uint64_t deviceAddress, uint64_t function) {
		this->deviceAddress = deviceAddress;
		this->function = function;
	}

        void EnumerateFunction(uint64_t device_address, uint64_t function) {
                uint64_t offset = function<< 12;

                uint64_t function_address = device_address + offset;
		VMM::MapMemory((void*)function_address, (void*)function_address);

                PCIDeviceHeader *pciDeviceHeader = (PCIDeviceHeader*)function_address;

                if(pciDeviceHeader->DeviceID == 0) return;
                if(pciDeviceHeader->DeviceID == 0xFFFF) return;

                printk(PREFIX "PCI device: %s -> %s (0x%x) - %s (0x%x) - %s (0x%x) - %s (0x%x)\n",
                       DeviceClasses[pciDeviceHeader->Class],
                       GetVendorName(pciDeviceHeader->VendorID),
                       pciDeviceHeader->VendorID,
                       GetDeviceName(pciDeviceHeader->VendorID, pciDeviceHeader->DeviceID),
                       pciDeviceHeader->DeviceID,
                       GetSubclassName(pciDeviceHeader->Class, pciDeviceHeader->Subclass),
                       pciDeviceHeader->Subclass,
                       GetProgIFName( pciDeviceHeader->Class, pciDeviceHeader->Subclass, pciDeviceHeader->ProgIF),
                       pciDeviceHeader->ProgIF);


		static uint64_t pciMinor = 0;
		PCIDevice *newDevice = new PCIDevice(69, 69);
		newDevice->SetMajor(10);
		newDevice->SetMinor(pciMinor++);;
		AddDevice(newDevice);

                printk(PREFIX "Done enumerating function.\n");
        }

        void EnumerateDevice(uint64_t bus_address, uint64_t device) {
                uint64_t offset = device << 15;

                uint64_t device_address = bus_address + offset;
                VMM::MapMemory((void*)device_address, (void*)device_address);

                PCIDeviceHeader *pciDeviceHeader = (PCIDeviceHeader*)device_address;

                if(pciDeviceHeader->DeviceID == 0) return;
                if(pciDeviceHeader->DeviceID == 0xFFFF) return;

                for (uint64_t function = 0; function < 8; function++) {
                        EnumerateFunction(device_address, function);
                }
        }

        void EnumerateBus(uint64_t base_address, uint64_t bus) {
                uint64_t offset = bus << 20;

                uint64_t bus_address = base_address + offset;
                VMM::MapMemory((void*)bus_address, (void*)bus_address);

                PCIDeviceHeader *pciDeviceHeader = (PCIDeviceHeader*)bus_address;

                if(pciDeviceHeader->DeviceID == 0) return;
                if(pciDeviceHeader->DeviceID == 0xFFFF) return;

                for (uint64_t device = 0; device < 32; device++) {
                        EnumerateDevice(bus_address, device);
                }
        }

        void EnumeratePCI(ACPI::MCFGHeader *mcfg) {
                int entries = ((mcfg->Header.Length) - sizeof(ACPI::MCFGHeader)) / sizeof(ACPI::DeviceConfig);

                for (int i = 0; i < entries; i++) {
                        ACPI::DeviceConfig *newDeviceConfig = (ACPI::DeviceConfig*)((uint64_t)mcfg + sizeof(ACPI::MCFGHeader) + (sizeof(ACPI::DeviceConfig) * i));
                        for (uint64_t bus = newDeviceConfig->StartBus; bus < newDeviceConfig->EndBus; bus++) {
                                EnumerateBus(newDeviceConfig->BaseAddress, bus);
                        }
                }
        }
}
