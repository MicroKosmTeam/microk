#include "acpi.h"

#include <lai/host.h>
#include <lai/core.h>
#include <lai/helpers/pm.h>

#include <mkmi_syscall.h>
#include <mkmi_memory.h>
#include <mkmi_string.h>
#include <mkmi_log.h>

void Init() {
	RSDP2 *rsdp;
	size_t size;
	Syscall(SYSCALL_FILE_OPEN, "ACPI:RSDP", &rsdp, &size, 0, 0, 0);
	VMMap(rsdp, rsdp, size, 0);

	MKMI_Printf("RSDP revision: 0x%x\r\n", rsdp->Revision);

	lai_set_acpi_revision(rsdp->Revision);
//	lai_create_namespace();
}
