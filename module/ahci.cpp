#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <cdefs.h>
#include <sys/driver.hpp>

uint64_t *KRNLSYMTABLE;

void (*PrintK)(char *format, ...);

void *(*Malloc)(size_t size);
void (*Free)(void *p);

void (*Memcpy)(void *dest, void *src, size_t n);
void (*Memset)(void *start, uint8_t value, uint64_t num);
int (*Memcmp)(const void* buf1, const void* buf2, size_t count);

char *(*Strcpy)(char *strDest, const char *strSrc);

void *(*RequestPage)();
void *(*RequestPages)(size_t pages);

void *operator new(size_t size) { return Malloc(size); }

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

Driver *ahciDriver = NULL;
extern "C" Driver *ModuleInit(void) {
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

	PrintK("Hello from AHCI module.\r\n");
	PrintK("Initializing...\r\n");

	ahciDriver = new Driver;
	if (ahciDriver == NULL) while (true) asm volatile("hlt");
	ahciDriver->Ioctl = &Ioctl;
	Strcpy(ahciDriver->Name, "MicroK Intel AHCI driver");

	PrintK("AHCI module initialization is done. Returning recall address.\r\n");

	return ahciDriver;
}
