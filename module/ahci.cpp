#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <cdefs.h>

uint64_t *KRNLSYMTABLE;
void (*PrintK)(char *format, ...);
void* (*Malloc)(size_t size);
void (*Free)(void *p);

uint64_t Ioctl(uint64_t request, va_list ap) {
	uint64_t result = 0;

	switch (request) {
		case 0:
			PrintK("Hello from AHCI driver Ioctl.\r\n");
			break;
		default:
			break;
	}

	return result;

}

extern "C" uint64_t _start(void) {
	KRNLSYMTABLE = CONFIG_SYMBOL_TABLE_BASE;
	PrintK = KRNLSYMTABLE[KRNLSYMTABLE_PRINTK];
	Malloc = KRNLSYMTABLE[KRNLSYMTABLE_MALLOC];
	Free = KRNLSYMTABLE[KRNLSYMTABLE_FREE];

	PrintK("Hello from AHCI module.\r\n");
	PrintK("Initializing...\r\n");

	PrintK("AHCI module initialization is done. Returning recall address.\r\n");

	return &Ioctl;
}
