#include <cdefs.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/driver.hpp>

#include "module.hpp"

const char *MODULE_NAME = "MicroK Acqua in Vino";
uint64_t *KRNLSYMTABLE;
Driver *testDriverHeader;

uint64_t Ioctl(uint64_t request, va_list ap) {
	uint64_t result = 0;

	switch (request) {
		case 0:
			PrintK("Hello, world!\r\n");
			break;
		default:
			PrintK("Operation not valid.\r\n");
			result = 1;
			break;
	}

	return result;

}

extern "C" Driver *ModuleInit() {
	KRNLSYMTABLE = CONFIG_SYMBOL_TABLE_BASE;
	PrintK = KRNLSYMTABLE[KRNLSYMTABLE_PRINTK];
	Malloc = KRNLSYMTABLE[KRNLSYMTABLE_MALLOC];
	Free = KRNLSYMTABLE[KRNLSYMTABLE_FREE];
	Strcpy = KRNLSYMTABLE[KRNLSYMTABLE_STRCPY];

	PrintK("Hello from %s.\r\n", MODULE_NAME);
	PrintK("Initializing...\r\n");

	testDriverHeader = new Driver;
	testDriverHeader->Ioctl = &Ioctl;
	Strcpy(testDriverHeader->Name, MODULE_NAME);

	PrintK("%s initialization is done. Returning device structure.\r\n", MODULE_NAME);

	return testDriverHeader;
}
