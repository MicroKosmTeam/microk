#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <cdefs.h>
#include <mkmi.h>
#include <mkmi_log.h>
#include <mkmi_memory.h>
#include <mkmi_syscall.h>

extern "C" uint32_t VendorID = 0xA3C1C0DE;
extern "C" uint32_t ProductID = 0xCAFEBABE;

#include "acpi/acpi.h"

extern "C" size_t OnInit() {
	Init();

	return 0;
}

extern "C" size_t OnExit() {

	return 0;
}
