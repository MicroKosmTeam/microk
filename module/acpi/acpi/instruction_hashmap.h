#pragma once
#include <stdint.h>
#include <stddef.h>

struct AML_Hashmap;
struct TokenList;

typedef void (*AML_OpcodeHandler)(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx);

struct AML_HashmapEntry {
	uint8_t OPCode;
	AML_OpcodeHandler Handler;
};

struct AML_Hashmap {
	size_t Size;
	AML_HashmapEntry* Entries;
};

AML_Hashmap *CreateHashmap();
AML_OpcodeHandler FindHandler(AML_Hashmap *hashmap, uint8_t opcode);
void DeleteHashmap(AML_Hashmap *hashmap);
