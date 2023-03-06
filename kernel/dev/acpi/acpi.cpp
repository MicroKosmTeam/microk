#include <dev/acpi/acpi.hpp>
#include <sys/printk.hpp>
#include <limine.h>
#include <sys/panic.hpp>
#include <dev/pci/pci.hpp>

static volatile limine_rsdp_request RSDPRequest = {
	.id = LIMINE_RSDP_REQUEST,
	.revision = 0
};

namespace ACPI {
void PrintTables(SDTHeader *sdtHeader) {
        int entries = (sdtHeader->Length - sizeof(ACPI::SDTHeader) ) / 8;
        for (int i = 0; i < entries; i++) {
                ACPI::SDTHeader *newSDTHeader = (ACPI::SDTHeader*)*(uint64_t*)((uint64_t)sdtHeader + sizeof(ACPI::SDTHeader) + (i*8));
		PRINTK::PrintK("Found table: %c%c%c%c\r\n",
				newSDTHeader->Signature[0],
				newSDTHeader->Signature[1],
				newSDTHeader->Signature[2],
				newSDTHeader->Signature[3]
				);
        }

        return;
}

void *FindTable(SDTHeader *sdtHeader, char *signature) {
        int entries = (sdtHeader->Length - sizeof(ACPI::SDTHeader) ) / 8;
        for (int i = 0; i < entries; i++) {
                ACPI::SDTHeader *newSDTHeader = (ACPI::SDTHeader*)*(uint64_t*)((uint64_t)sdtHeader + sizeof(ACPI::SDTHeader) + (i*8));
                for (int j = 0; j < 4; j++) {
                        if(newSDTHeader->Signature[j] != signature[j]) break;
                        if(j == 3) return newSDTHeader;
                }
        }

        return NULL;
}

void Init(KInfo *info) {
	if (RSDPRequest.response == NULL) PANIC("Limine did not provide the RSDP");

	RSDP2 *rsdp = RSDPRequest.response->address;

	PRINTK::PrintK("Loading the XSDT table...\r\n");
	SDTHeader *xsdt = (SDTHeader*)(rsdp->XSDTAddress);

	PrintTables(xsdt);

	PRINTK::PrintK("Loading the FADT table...\r\n");
	FADT *fadt = (FADT*)FindTable(xsdt, (char*)"FACP");
	if (fadt == NULL) PANIC("Could not find FADT");

	PRINTK::PrintK("Loading the MCFG table...\r\n");
        MCFGHeader *mcfg = (MCFGHeader*)FindTable(xsdt, (char*)"MCFG");
	if (mcfg == NULL) PANIC("Could not find MCFG");

	PRINTK::PrintK("Enumerating PCI devices...\r\n");
        PCI::EnumeratePCI(mcfg);

	PRINTK::PrintK("ACPI initialization done.\r\n");

	return;
}
}
