#include <sys/symbols.h>
#include <stddef.h>
#include <mm/string.h>

#include <sys/printk.h>

const symbol_t *lookup_symbol(uint64_t addr) {
	uint64_t symbolIndex;

	for (symbolIndex = 0; symbolIndex < symbolCount; symbolIndex++) {
		uint64_t symAddr = symbols[symbolIndex].addr;
		if (symAddr > addr) break;
	}

	return &symbols[symbolIndex - 1];
}
