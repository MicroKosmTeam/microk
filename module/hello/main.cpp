#include <cdefs.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/driver.hpp>

#include "module.hpp"

const char *MODULE_NAME = "MicroK Acqua in Vino";
uint64_t *KRNLSYMTABLE;
Driver *testDriverHeader;

uint64_t IOCtl(uint64_t request, va_list ap) {
	uint64_t result = 0;

	switch (request) {
		case 0:  {
			PrintK("Hello, world!\r\n");

			Printk("Creating buffer\r\n");
			MKMI::BUFFER::Buffer *testBuffer = BufferCreate(MKMI::BUFFER::DATA_KERNEL_GENERIC, 128);

			PrintK("Doing I/O work\r\n");
			BufferIOCtl(testBuffer, MKMI::BUFFER::OPER

			}
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
	Create = KRNLSYMTABLE[KRNLSYMTABLE_BUFFERCREATE];
	IOCtl = KRNLSYMTABLE[KRNLSYMTABLE_BUFFERIOCTL];
	Delete = KRNLSYMTABLE[KRNLSYMTABLE_BUFFERDELETE];

	PrintK("Hello from %s.\r\n", MODULE_NAME);
	PrintK("Initializing...\r\n");

	testDriverHeader = new Driver;
	testDriverHeader->Ioctl = &IOCtl;
	Strcpy(testDriverHeader->Name, MODULE_NAME);

	PrintK("%s initialization is done. Returning device structure.\r\n", MODULE_NAME);

	return testDriverHeader;
}
