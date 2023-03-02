/*

	// Kernel symbol table
	KRNLSYMTABLE = PMM::RequestPage();
	memset(KRNLSYMTABLE, 0, 0x1000);

	KRNLSYMTABLE[0] = &PRINTK::PrintK;
	KRNLSYMTABLE[1] = &Malloc;
	KRNLSYMTABLE[2] = &Free;

	VMM::MapMemory(0xffffffffffff0000, KRNLSYMTABLE);

	void (*testPrintk)(char *format, ...) = KRNLSYMTABLE[0];
	void* (*testMalloc)(size_t size) = KRNLSYMTABLE[1];
	void (*testFree)(void *p) = KRNLSYMTABLE[2];

	uint64_t *test = testMalloc(10);
	*test = 0x69;
	testPrintk("Test: 0x%x\r\n", *test);
	testFree(test);

*/
