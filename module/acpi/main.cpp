#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <cdefs.h>
#include <mkmi.h>
#include <mkmi_log.h>
#include <mkmi_memory.h>
#include <mkmi_syscall.h>

extern "C" uint32_t VendorID = 0xCAFEBABE;
extern "C" uint32_t ProductID = 0xA3C1C0DE;

#include "acpi/acpi.h"

extern "C" size_t OnMessage() {
	MKMI_Printf("Message!\r\n");
	return 0;
}

extern "C" size_t OnSignal() {
	MKMI_Printf("Signal!\r\n");
	return 0;
}

extern "C" size_t OnInit() {
	Syscall(SYSCALL_MODULE_SECTION_REGISTER, "ACPI", VendorID, ProductID, 0, 0 ,0);

	ACPIManager *acpi = new ACPIManager();

	return 0;
}

extern "C" size_t OnExit() {

	return 0;
}
