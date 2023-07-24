#include "aml_opcodes.h"

#include <stdint.h>
#include <stddef.h>
#include <mkmi_log.h>
#include <mkmi_memory.h>

typedef void (*AML_OpcodeHandler)();

typedef struct {
	uint8_t OPCode;
	AML_OpcodeHandler Handler;
} AML_HashmapEntry;

typedef struct {
	size_t Size;
	AML_HashmapEntry* Entries;
} AML_Hashmap;

void HandleZeroOp() {
	MKMI_Printf("OP: Zero.\r\n");
}

void HandleOneOp() {
	MKMI_Printf("OP: One.\r\n");
}

void HandleAliasOp() {
	MKMI_Printf("OP: Alias.\r\n");
}

void HandleNameOp() {
	MKMI_Printf("OP: Name.\r\n");
}

AML_Hashmap *CreateHashmap() {
	AML_Hashmap* hashmap = new AML_Hashmap;
	hashmap->Size = 4;
	hashmap->Entries = new AML_HashmapEntry[hashmap->Size];

	hashmap->Entries[0] = (AML_HashmapEntry){AML_ZERO_OP, HandleZeroOp};
	hashmap->Entries[1] = (AML_HashmapEntry){AML_ONE_OP, HandleOneOp};
	hashmap->Entries[2] = (AML_HashmapEntry){AML_ALIAS_OP, HandleAliasOp};
	hashmap->Entries[3] = (AML_HashmapEntry){AML_NAME_OP, HandleNameOp};

	return hashmap;
}

void DeleteHashmap(AML_Hashmap *hashmap) {
	delete[] hashmap->Entries;
	delete hashmap;
}

// Find the handler function from the hashmap based on the opcode
AML_OpcodeHandler FindHandler(AML_Hashmap *hashmap, uint8_t opcode) {
	for (size_t i = 0; i < hashmap->Size; ++i) {
		if (hashmap->Entries[i].OPCode == opcode) {
			return hashmap->Entries[i].Handler;
		}
	}

	return NULL; // Handler not found
}

void Parse(uint8_t *data, size_t size) {
	// Initialize the hashmap and register opcode handlers
	AML_Hashmap *hashmap = CreateHashmap();

	// Tokenize the AML data (lexer and parser not shown here for brevity)

	// Simulated AST traversal and execution (for demonstration purposes)
	size_t offset = 0;
	while (offset < size) {
		uint8_t opcode = data[offset++];
		AML_OpcodeHandler handler = FindHandler(hashmap, opcode);
		if (handler) {
			handler(); // Execute the opcode handler
		} else {
			MKMI_Printf("Error: Unsupported opcode 0x%x\n", opcode);
		}
	}

	DeleteHashmap(hashmap);

	return 0;
}

