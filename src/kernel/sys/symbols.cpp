#include <sys/symbols.h>
#include <stddef.h>
#include <mm/string.h>

#include <sys/printk.h>

const symbol_t *lookup_symbol(uint64_t addr) {
	return NULL;

	// This does not work yet:
	/*
	uint64_t symbolIndex;
	for (symbolIndex = 0; symbolIndex < symbolCount - 10; symbolIndex++) {
		uint64_t symAddr = atoi((char*)symbols[symbolIndex].addr);
		if (symAddr > addr) break;
	}

	return &symbols[symbolIndex - 1];
	*/
}
