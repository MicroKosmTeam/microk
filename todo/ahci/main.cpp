#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <cdefs.h>
#include <sys/driver.hpp>
#include <dev/pci/pci.hpp>

#include "ahci.hpp"
#include "module.hpp"

const char *MODULE_NAME = "MicroK Intel AHCI driver";
uint64_t *KRNLSYMTABLE;
Driver *ahciDriverHeader;
AHCI::AHCIDriver *ahciDriver;
PCI::PCIDeviceHeader *pciHeader;

bool isActive = false;

uint64_t Ioctl(uint64_t request, va_list ap) {
	uint64_t result = 0;

	switch (request) {
		case 0: { // Init
			pciHeader = va_arg(ap, PCI::PCIDeviceHeader*);
			if (pciHeader == NULL) {
				PrintK("Could not find PCI device pciHeader.\r\n");
				return;
			}

			isActive = true;

			PrintK("The PCI header is at 0x%x.\r\n", pciHeader);
			PrintK("AHCI device: 0x%x - 0x%x - 0x%x - 0x%x\n",
				pciHeader->VendorID,
				pciHeader->DeviceID,
				pciHeader->Subclass,
				pciHeader->ProgIF);
			}

			break;

			ahciDriver = new AHCI::AHCIDriver(pciHeader);

			break;
		case 1: {
			}
		default:
			break;
	}

	return result;

}

extern "C" Driver *ModuleInit() {
	KRNLSYMTABLE = CONFIG_SYMBOL_TABLE_BASE;
	RequestPage =  KRNLSYMTABLE[KRNLSYMTABLE_REQUESTPAGE];
	RequestPages =  KRNLSYMTABLE[KRNLSYMTABLE_REQUESTPAGES];
	Memcpy =  KRNLSYMTABLE[KRNLSYMTABLE_MEMCPY];
	Memset =  KRNLSYMTABLE[KRNLSYMTABLE_MEMSET];
	Memcmp = KRNLSYMTABLE[KRNLSYMTABLE_MEMCMP];
	PrintK = KRNLSYMTABLE[KRNLSYMTABLE_PRINTK];
	Malloc = KRNLSYMTABLE[KRNLSYMTABLE_MALLOC];
	Free = KRNLSYMTABLE[KRNLSYMTABLE_FREE];
	Strcpy = KRNLSYMTABLE[KRNLSYMTABLE_STRCPY];

	PrintK("Hello from %s.\r\n", MODULE_NAME);
	PrintK("Initializing...\r\n");

	ahciDriverHeader = new Driver;
	ahciDriverHeader->Ioctl = &Ioctl;
	Strcpy(ahciDriverHeader->Name, MODULE_NAME);

	PrintK("%s initialization is done. Returning device structure.\r\n", MODULE_NAME);

	return ahciDriverHeader;
}
