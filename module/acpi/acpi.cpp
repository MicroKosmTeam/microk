/*
   File: dev/acpi/acpi.cpp
*/

#include <limine.h>
#include <sys/panic.hpp>
#include <sys/printk.hpp>
#include <dev/pci/pci.hpp>
#include <dev/acpi/acpi.hpp>

/* RSDP request for Limine */
static volatile limine_rsdp_request RSDPRequest = {
	.id = LIMINE_RSDP_REQUEST,
	.revision = 0
};

namespace ACPI {
/* Function that prints available tables for debugging sake */
void PrintTables(SDTHeader *sdtHeader) {
	/* Finding the number of entries */
        int entries = (sdtHeader->Length - sizeof(ACPI::SDTHeader) ) / 8;
        for (int i = 0; i < entries; i++) {
		/* Getting the table header */
                ACPI::SDTHeader *newSDTHeader = (ACPI::SDTHeader*)*(uint64_t*)((uint64_t)sdtHeader + sizeof(ACPI::SDTHeader) + (i*8));

		/* Printing the signature */
		PRINTK::PrintK("Found table: %c%c%c%c\r\n",
				newSDTHeader->Signature[0],
				newSDTHeader->Signature[1],
				newSDTHeader->Signature[2],
				newSDTHeader->Signature[3]
				);
        }

        return;
}

/* Function that finds a table given the XSDT and the signature */
void *FindTable(SDTHeader *sdtHeader, char *signature) {
	/* Finding the number of entries */
        int entries = (sdtHeader->Length - sizeof(ACPI::SDTHeader) ) / 8;
        for (int i = 0; i < entries; i++) {
		/* Getting the table header */
                ACPI::SDTHeader *newSDTHeader = (ACPI::SDTHeader*)*(uint64_t*)((uint64_t)sdtHeader + sizeof(ACPI::SDTHeader) + (i*8));

		/* Checking if the signatures match */
                for (int j = 0; j < 4; j++) {
                        if(newSDTHeader->Signature[j] != signature[j]) break;
			/* If so, then return the table header */
                        if(j == 3) return newSDTHeader;
                }
        }

	/* The request table hasn't been found */
        return NULL;
}

/* ACPI Initialization function */
void Init(KInfo *info) {
	/* If we don't have the RSDP address, we can't continue */
	if (RSDPRequest.response == NULL) PANIC("Limine did not provide the RSDP");

	RSDP2 *rsdp = RSDPRequest.response->address;

	PRINTK::PrintK("Loading the XSDT table...\r\n");
	/* Get the XSDT form the RSDP */
	SDTHeader *xsdt = (SDTHeader*)(rsdp->XSDTAddress);

	PrintTables(xsdt);

	/* Loading essential tables */

	/* Loading the FADT */
	PRINTK::PrintK("Loading the FADT table...\r\n");
	FADT *fadt = (FADT*)FindTable(xsdt, (char*)"FACP");
	if (fadt == NULL) PANIC("Could not find FADT");

	/* Loading the MCFG */
	PRINTK::PrintK("Loading the MCFG table...\r\n");
        MCFGHeader *mcfg = (MCFGHeader*)FindTable(xsdt, (char*)"MCFG");
	if (mcfg == NULL) PANIC("Could not find MCFG");

	/* Enumerating PCI devices provided by the MCFG table */
	PRINTK::PrintK("Enumerating PCI devices...\r\n");
        PCI::EnumeratePCI(mcfg, info->higherHalf);

	PRINTK::PrintK("ACPI initialization done.\r\n");

	return;
}
}
