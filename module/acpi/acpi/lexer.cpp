#include "aml_executive.h"

#include <mkmi_log.h>
#include <mkmi_memory.h>

void ParseByte(TokenList *tokens, AML_Hashmap *hashmap, uint8_t *data, size_t *idx) {
	uint8_t byte = data[*idx];
	*idx += 1;

	AML_OpcodeHandler handler = FindHandler(hashmap, byte);

	if (handler) return handler(hashmap, tokens, data, idx);
	return AddToken(tokens, UNKNOWN, byte);	
}

AMLExecutive::AMLExecutive() {
	/* Initialize the hashmap and the root token list */
	Hashmap = CreateHashmap();
	RootTokenList = CreateTokenList(); 
}

AMLExecutive::~AMLExecutive() {
	/* Free the memory occupied by the token list and the hashmap */
	DeleteHashmap(Hashmap);
	FreeTokenList(RootTokenList);
}

int AMLExecutive::Parse(uint8_t *data, size_t size) {
	size_t idx = 0;
	while (idx < size) {
		ParseByte(RootTokenList, Hashmap, data, &idx);
	}

	MKMI_Printf("Done parsing %d bytes of AML code.\r\n", size);
	return 0;

	Token *current = RootTokenList->Head;

	bool error = false;
	while (current && !error) {
//	for (int i = 0; i < 50 && current && !error; ++i) {
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
				MKMI_Printf("SCOPE. Length: %d.\r\n", current->Scope.PkgLength);
				break;
			case BUFFER:
				MKMI_Printf("BUFFER\r\n");
				break;
			case PACKAGE:
				MKMI_Printf("PACKAGE\r\n");
				break;
			case REGION:
				MKMI_Printf("REGION. Region space: %d. Region offset: %d. Region length: %d.\r\n", current->Region.RegionSpace, current->Region.RegionOffset.Data, current->Region.RegionLen.Data);
				break;
			case FIELD:
				MKMI_Printf("FIELD\r\n");
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

	return 0;
}
	
int AMLExecutive::Execute() {
	return 0;
}
