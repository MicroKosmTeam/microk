#include "aml_opcodes.h"

#include <stdint.h>
#include <stddef.h>
#include <mkmi_log.h>
#include <mkmi_memory.h>

enum TokenType {
	INTEGER,
	STRING,
	NAME,
	RETURN,
	UNKNOWN,
};

struct Token {
	TokenType Type;

	union {
		uint64_t Int;
		char Name[4];
		char *String;
	};

	Token *Next;    // Pointer to the next token in the linked list
};

struct TokenList {
    Token *Head;    // Pointer to the first token in the list
    Token *Tail;    // Pointer to the last token in the list
};

typedef void (*AML_OpcodeHandler)(TokenList *list, uint8_t *data, size_t *idx);

typedef struct {
	uint8_t OPCode;
	AML_OpcodeHandler Handler;
} AML_HashmapEntry;

typedef struct {
	size_t Size;
	AML_HashmapEntry* Entries;
} AML_Hashmap;

void DeleteHashmap(AML_Hashmap *hashmap) {
	delete[] hashmap->Entries;
	delete hashmap;
}

// Function to create a new token and add it to the token list
void AddToken(TokenList *tokenList, TokenType type, ...) {
	va_list ap;
	va_start(ap, type);

	Token *newToken = new Token;
	newToken->Type = type;

	switch(newToken->Type) {
		case INTEGER:
			newToken->Int = va_arg(ap, uint64_t);
			break;
		case NAME:
			Memcpy(newToken->Name, va_arg(ap, char*), 4);
			break;
		case STRING: {
			const char *str = va_arg(ap, char*);
			size_t len = va_arg(ap, size_t);
			newToken->String = new char[len];
			Memcpy(newToken->String, str, len);
			}
			break;
		default:
			break;
	}

	newToken->Next = NULL;

	if (tokenList->Head == NULL) {
		tokenList->Head = newToken;
		tokenList->Tail = newToken;
	} else {
		tokenList->Tail->Next = newToken;
		tokenList->Tail = newToken;
	}


	va_end(ap);

	return;
}

// Function to free the memory occupied by the token list
void FreeTokenList(TokenList *tokenList) {
	Token *current = tokenList->Head;
	while (current) {
		if (current->Type == STRING) delete current->String;
		Token *next = current->Next;
		delete current;
		current = next;
	}

	tokenList->Head = NULL;
	tokenList->Tail = NULL;
}

void HandleNameOp(TokenList *list, uint8_t *data, size_t *idx) {
	AddToken(list, TokenType::NAME, &data[*idx]);

	*idx += 4;
}

void HandleBytePrefix(TokenList *list, uint8_t *data, size_t *idx) {
	uint64_t num = 0;
	num |= data[*idx];

	AddToken(list, TokenType::INTEGER, num);
	
	*idx += 1;
	
}

void HandleWordPrefix(TokenList *list, uint8_t *data, size_t *idx) {
	MKMI_Printf("Word.\r\n");

	uint64_t num = 0;
	num |= data[*idx];
	num |= (data[*idx + 1] << 8);

	AddToken(list, TokenType::INTEGER, num);
	
	*idx += 2;
}


void HandleDWordPrefix(TokenList *list, uint8_t *data, size_t *idx) {
	MKMI_Printf("DWord.\r\n");

	uint64_t num = 0;
	num |= data[*idx];
	num |= (data[*idx + 1] << 8);
	num |= (data[*idx + 2] << 16);
	num |= (data[*idx + 3] << 24);

	AddToken(list, TokenType::INTEGER, num);

	*idx += 4;
}

void HandleStringPrefix(TokenList *list, uint8_t *data, size_t *idx) {
	MKMI_Printf("String.\r\n");

	const char *str = &data[*idx];
	size_t len = 2; /* First char + '\0' */

	while(data[*idx] != '\0') { ++len; *idx += 1; }
	
	AddToken(list, TokenType::STRING, str, len);
}

void HandleQWordPrefix(TokenList *list, uint8_t *data, size_t *idx) {
	MKMI_Printf("QWord.\r\n");

	uint64_t num;
	num |= data[*idx];
	num |= (data[*idx + 1] << 8);
	num |= (data[*idx + 2] << 16);
	num |= (data[*idx + 3] << 24);
	num |= (data[*idx + 4] << 32);
	num |= (data[*idx + 5] << 40);
	num |= (data[*idx + 6] << 48);
	num |= (data[*idx + 7] << 52);
	AddToken(list, TokenType::INTEGER, num);

	*idx += 8;
}

AML_Hashmap *CreateHashmap() {
	AML_Hashmap *hashmap = new AML_Hashmap;
	hashmap->Size = 9;
	hashmap->Entries = new AML_HashmapEntry[hashmap->Size];
	
	hashmap->Entries[0] = (AML_HashmapEntry){AML_ZERO_OP, NULL};
	hashmap->Entries[1] = (AML_HashmapEntry){AML_ONE_OP, NULL};
	hashmap->Entries[2] = (AML_HashmapEntry){AML_ALIAS_OP, NULL};
	hashmap->Entries[3] = (AML_HashmapEntry){AML_NAME_OP, HandleNameOp};
	hashmap->Entries[4] = (AML_HashmapEntry){AML_BYTEPREFIX, HandleBytePrefix};
	hashmap->Entries[5] = (AML_HashmapEntry){AML_WORDPREFIX, HandleWordPrefix};
	hashmap->Entries[6] = (AML_HashmapEntry){AML_DWORDPREFIX, HandleDWordPrefix};
	hashmap->Entries[7] = (AML_HashmapEntry){AML_QWORDPREFIX, HandleQWordPrefix};
	hashmap->Entries[8] = (AML_HashmapEntry){AML_STRINGPREFIX, HandleStringPrefix};

	return hashmap;
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

	TokenList *tokens = new TokenList;
	tokens->Head = NULL;
	tokens->Tail = NULL;

	size_t idx = 0;
	while (idx < size) {
		uint8_t byte = data[idx++];

		AML_OpcodeHandler handler = FindHandler(hashmap, byte);
		if (handler) handler(tokens, data, &idx);
		else AddToken(tokens, UNKNOWN);	
	}


	char name[5];
	name[5] = '\0';

	Token *current = tokens->Head;
	while (current) {
		MKMI_Printf("Token Type: ");
		switch (current->Type) {
			case INTEGER:
				MKMI_Printf("INTEGER, Value: %d\r\n", current->Int);
				break;
			case NAME:
				Memcpy(name, current->Name, 4);
				MKMI_Printf("NAME, Value: %s\r\n", name);
				break;
			case STRING:
				MKMI_Printf("STRING, Value: %s\r\n", current->String);
				break;
			case RETURN:
				MKMI_Printf("RETURN\r\n");
				break;
			default:
				MKMI_Printf("UNKNOWN\r\n");
				break;
		}

		current = current->Next;
	}

	// Free the memory occupied by the token list
	FreeTokenList(tokens);
	DeleteHashmap(hashmap);

	return;
}
