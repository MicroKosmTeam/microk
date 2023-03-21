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

			PrintK("Creating buffer\r\n");
			MKMI::BUFFER::Buffer *testBuffer = BufferCreate(MKMI::BUFFER::DATA_KERNEL_GENERIC, 128);

			if(testBuffer == NULL) {
				PrintK("!!!! WARNING !!!!\r\n"
				       "Null Buffer.\r\n");
				return 0;
			}

			PrintK("Buffer 0x%x:\r\n"
			       " - Address: 0x%x\r\n"
			       " - Readable: %d\r\n"
			       " - Type: 0x%x\r\n"
			       " - Size: %d\r\n",
			       testBuffer,
			       testBuffer->address,
			       testBuffer->readable ? 1 : 0,
			       testBuffer->type,
			       testBuffer->size);

			PrintK("Doing I/O work\r\n");
			uint64_t writeTest[128] = { 0x69 };
			PrintK("Writing data...\r\n");
			BufferIOCtl(testBuffer, MKMI::BUFFER::OPERATION_WRITEDATA, &writeTest, 128);

			uint64_t readTest[128] = { 0 };
			PrintK("Reading data...\r\n");
			BufferIOCtl(testBuffer, MKMI::BUFFER::OPERATION_READDATA, &readTest, 128);
			PrintK("Result 0x%x\r\n", readTest[0]);

			PrintK("Deleting buffer.\r\n");
			BufferDelete(testBuffer);

			PrintK("Done.\r\n");

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
	BufferCreate = KRNLSYMTABLE[KRNLSYMTABLE_BUFFERCREATE];
	BufferIOCtl = KRNLSYMTABLE[KRNLSYMTABLE_BUFFERIOCTL];
	BufferDelete = KRNLSYMTABLE[KRNLSYMTABLE_BUFFERDELETE];

	PrintK("Hello from %s.\r\n", MODULE_NAME);
	PrintK("Initializing...\r\n");

	testDriverHeader = new Driver;
	testDriverHeader->Ioctl = &IOCtl;
	Strcpy(testDriverHeader->Name, MODULE_NAME);

	PrintK("%s initialization is done. Returning device structure.\r\n", MODULE_NAME);

	return testDriverHeader;
}
