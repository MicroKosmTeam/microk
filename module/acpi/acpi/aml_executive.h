#pragma once
#include <stdint.h>
#include <stddef.h>

#include "instruction_hashmap.h"
#include "token.h"

void ParseByte(TokenList *tokens, AML_Hashmap *hashmap, uint8_t *data, size_t *idx);

class AMLExecutive {
public:
	AMLExecutive();
	~AMLExecutive();

	int Parse(uint8_t *data, size_t size);
	int Execute();
private:
	AML_Hashmap *Hashmap;
	TokenList *RootTokenList;
};
