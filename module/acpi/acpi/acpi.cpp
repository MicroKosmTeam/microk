#include "acpi.h"
#include "aml_executive.h"
#include <mkmi_memory.h>
#include <mkmi_syscall.h>
#include <mkmi_string.h>
#include <mkmi_log.h>
#include <mkmi_exit.h>
#include <cdefs.h>

ACPIManager::ACPIManager() {
	RSDP = new RSDP2;

	Syscall(SYSCALL_FILE_READ, "ACPI:RSDP", RSDP, sizeof(RSDP2), 0, 0 ,0);
	if(RSDP->Revision != 2) {
		Panic("Invalid ACPI RSDP revision");
	}

	uint8_t checksum = 0;
	uint8_t *ptr = (uint8_t*)RSDP;
	for (size_t i = 0; i < sizeof(RSDP2); ++i) checksum += ptr[i];

	if(checksum != 0) {
		Panic("Invalid ACPI RSDP checksum");
	}

	MainSDT = Malloc(RSDP->Length);
	if(RSDP->XSDTAddress != NULL) {
		MainSDTType = 8;

		void *addr = RSDP->XSDTAddress + HIGHER_HALF;
		size_t size = ((SDTHeader*)addr)->Length;

		MainSDT = Malloc(size);
		Memcpy(MainSDT, addr, size);
	} else {
		MainSDTType = 4;

		void *addr = RSDP->RSDTAddress + HIGHER_HALF;
		size_t size = ((SDTHeader*)addr)->Length;

		MainSDT = Malloc(size);
		Memcpy(MainSDT, addr, size);
	}



	MKMI_Printf(" Table  OEM    Length \r\n"
	            " -------------------------\r\n");
	PrintTable(MainSDT);

	FADT = NULL;

	int entries = (MainSDT->Length - sizeof(SDTHeader) ) / MainSDTType;
        for (int i = 0; i < entries; i++) {
		/* Getting the table header */
                uintptr_t addr = *(uintptr_t*)((uintptr_t)MainSDT + sizeof(SDTHeader) + (i * MainSDTType));
		SDTHeader *newSDTHeader = addr + HIGHER_HALF;

		PrintTable(newSDTHeader);

		if(Memcmp(newSDTHeader->Signature, "FACP", 4) == 0) {
			FADT = new FADTTable;

			Memcpy(FADT, newSDTHeader, sizeof(FADTTable));

			if(FADT->X_Dsdt != NULL) {
				SDTHeader *dsdt = FADT->X_Dsdt + HIGHER_HALF;
				DSDT = Malloc(dsdt->Length);
		
				Memcpy(DSDT, dsdt, dsdt->Length);
				
				PrintTable(DSDT);
			} else if(FADT->Dsdt != NULL) {
				SDTHeader *dsdt = FADT->X_Dsdt + HIGHER_HALF;
				DSDT = Malloc(dsdt->Length);

				Memcpy(DSDT, dsdt, dsdt->Length);

				PrintTable(DSDT);
			} else {
				Panic("No DSDT found");
			}
		}
        }

	if (FADT == NULL)
		Panic("No FADT found");

	DSDTExecutive = new AMLExecutive();
	DSDTExecutive->Parse((uint8_t*)DSDT + sizeof(SDTHeader), DSDT->Length - sizeof(SDTHeader));

	MKMI_Printf("ACPI initialized.\r\n");
}

void ACPIManager::PrintTable(SDTHeader *sdt) {
	char sig[5];
	char oem[7];

	sig[4] = '\0';
	oem[6] = '\0';

	Memcpy(sig, sdt->Signature, 4);
	Memcpy(oem, sdt->OEMID, 6);
		    
	MKMI_Printf(" %s   %s  %d\r\n", sig, oem, sdt->Length);
}

SDTHeader *ACPIManager::FindTable(SDTHeader *sdtHeader, char *signature, size_t index) {
	static size_t found = 0;

	/* Finding the number of entries */
        int entries = (sdtHeader->Length - sizeof(SDTHeader) ) / MainSDTType;
        for (int i = 0; i < entries; i++) {
		/* Getting the table header */
                SDTHeader *newSDTHeader = (SDTHeader*)*(uintptr_t*)((uintptr_t)sdtHeader + sizeof(SDTHeader) + (i * MainSDTType));

		/* Checking if the signatures match */
                for (int j = 0; j < 4; j++) {
                        if(newSDTHeader->Signature[j] != signature[j]) break;
			/* If so, then return the table header */
		
			if(++found < index) break;
			return newSDTHeader;
                }
        }

	/* The request table hasn't been found */
        return NULL;
}

void ACPIManager::Panic(const char *message) {
	MKMI_Printf("ACPI PANIC: %s\r\n", message);
	_exit(128);
}
