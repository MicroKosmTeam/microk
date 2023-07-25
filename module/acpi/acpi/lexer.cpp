#include "instruction_hashmap.h"
#include "token.h"

#include <stdint.h>
#include <stddef.h>
#include <mkmi_log.h>
#include <mkmi_memory.h>

void ParseByte(TokenList *tokens, AML_Hashmap *hashmap, uint8_t *data, size_t *idx) {
	uint8_t byte = data[*idx];
	*idx += 1;

	AML_OpcodeHandler handler = FindHandler(hashmap, byte);

	if (handler) return handler(hashmap, tokens, data, idx);
	return AddToken(tokens, UNKNOWN, byte);	
}

void Parse(uint8_t *data, size_t size) {
	// Initialize the hashmap and register opcode handlers
	AML_Hashmap *hashmap = CreateHashmap();

	TokenList *tokens = CreateTokenList(); 

	size_t idx = 0;
	while (idx < size) {
		ParseByte(tokens, hashmap, data, &idx);
	}

	Token *current = tokens->Head;
	
	bool error = false;
	while (current && !error) {
	//for (int i = 0; i < 20 && current && !error; ++i) {
		MKMI_Printf("Token Type: ");
		switch (current->Type) {
			case ZERO:
				MKMI_Printf("ZERO\r\n");
				break;
			case ONE:
				MKMI_Printf("ONE\r\n");
				break;
			case ALIAS:
				MKMI_Printf("ALIAS\r\n");
				break;
			case NAME:
				MKMI_Printf("NAME\r\n");
				break;
			case INTEGER:
				MKMI_Printf("INTEGER\r\n");
				break;
			case STRING:
				MKMI_Printf("STRING\r\n");
				break;
			case SCOPE:
				MKMI_Printf("SCOPE\r\n");
				break;
			case BUFFER:
				MKMI_Printf("BUFFER\r\n");
				break;
			case PACKAGE:
				MKMI_Printf("PACKAGE\r\n");
				break;
			case UNKNOWN:
				MKMI_Printf("UNKNOWN, Opcode: 0x%x\r\n", current->UnknownOpcode);
				break;
			default:
				MKMI_Printf("UNKNOWN\r\n");
				error = true;
				break;
		}

		current = current->Next;
	}

	// Free the memory occupied by the token list
	FreeTokenList(tokens);
	DeleteHashmap(hashmap);

	return;
}
