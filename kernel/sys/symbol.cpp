#include <sys/symbol.hpp>
#include <mm/string.hpp>

const Symbol *LookupSymbol(const char *name) {
	uint64_t symbolIndex;

	for (symbolIndex = 0; symbolIndex < symbolCount; symbolIndex++) {
		uint64_t symAddr = symbols[symbolIndex].addr;
		if (strcmp(symbols[symbolIndex].name, name) == 0) return &symbols[symbolIndex - 1];
	}

	return NULL;

}

const Symbol *LookupSymbol(uint64_t addr) {
	uint64_t symbolIndex;

	for (symbolIndex = 0; symbolIndex < symbolCount; symbolIndex++) {
		uint64_t symAddr = symbols[symbolIndex].addr;
		if (symAddr > addr) break;
	}

	return &symbols[symbolIndex - 1];
}
