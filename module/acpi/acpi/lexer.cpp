#include "aml_opcodes.h"

#include <stdint.h>
#include <stddef.h>
#include <mkmi_log.h>
#include <mkmi_memory.h>

enum TokenType {
	UNKNOWN,
	ZERO,
	ONE,
	ALIAS,
	NAME,
	INTEGER,
	STRING,
	SCOPE,
	BUFFER,
	PACKAGE,
};

struct Token {
	TokenType Type;

	union {
		/* Zero */
		/* One */
		struct {
			char NameOne[4];
			char NameTwo[4];
		} Alias;
		
		struct {
			char Name[4];
		} Name;

		uint64_t Int;
		
		char *String;

		struct {
			size_t Length;
			char Name[4];
		} Scope;
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
		case ALIAS:
			Memcpy(newToken->Alias.NameOne, va_arg(ap, char*), 4);
			Memcpy(newToken->Alias.NameTwo, va_arg(ap, char*), 4);
			break;
		case NAME:
			Memcpy(newToken->Name.Name, va_arg(ap, char*), 4);
			break;
		case INTEGER:
			newToken->Int = va_arg(ap, uint64_t);
			break;
		case STRING: {
			const char *str = va_arg(ap, char*);
			size_t len = va_arg(ap, size_t);
			newToken->String = new char[len];
			Memcpy(newToken->String, str, len);
			}
			break;
		case SCOPE:
			newToken->Scope.Length = va_arg(ap, size_t);
			Memcpy(newToken->Scope.Name, va_arg(ap, char*), 4);
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

void HandleZeroOp(TokenList *list, uint8_t *data, size_t *idx) {
	AddToken(list, ZERO);
}

void HandleOneOp(TokenList *list, uint8_t *data, size_t *idx) {
	AddToken(list, ONE);
}

void HandleAliasOp(TokenList *list, uint8_t *data, size_t *idx) {
	AddToken(list, ALIAS, &data[*idx], &data[*idx + 4]);

	*idx += 8;
}

void HandleNameOp(TokenList *list, uint8_t *data, size_t *idx) {
	AddToken(list, NAME, &data[*idx]);

	*idx += 4;
}

void HandleBytePrefix(TokenList *list, uint8_t *data, size_t *idx) {
	uint64_t num = 0;
	num |= data[*idx];

	AddToken(list, INTEGER, num);
	
	*idx += 1;
	
}

void HandleWordPrefix(TokenList *list, uint8_t *data, size_t *idx) {
	uint64_t num = 0;
	num |= data[*idx];
	num |= (data[*idx + 1] << 8);

	AddToken(list, INTEGER, num);
	
	*idx += 2;
}


void HandleDWordPrefix(TokenList *list, uint8_t *data, size_t *idx) {
	uint64_t num = 0;
	num |= data[*idx];
	num |= (data[*idx + 1] << 8);
	num |= (data[*idx + 2] << 16);
	num |= (data[*idx + 3] << 24);

	AddToken(list, INTEGER, num);

	*idx += 4;
}

void HandleStringPrefix(TokenList *list, uint8_t *data, size_t *idx) {
	const char *str = &data[*idx];
	size_t len = 2; /* First char + '\0' */

	while(data[*idx] != '\0') { ++len; *idx += 1; }
	
	AddToken(list, TokenType::STRING, str, len);
}

void HandleQWordPrefix(TokenList *list, uint8_t *data, size_t *idx) {
	uint64_t num;
	num |= data[*idx];
	num |= (data[*idx + 1] << 8);
	num |= (data[*idx + 2] << 16);
	num |= (data[*idx + 3] << 24);
	num |= (data[*idx + 4] << 32);
	num |= (data[*idx + 5] << 40);
	num |= (data[*idx + 6] << 48);
	num |= (data[*idx + 7] << 52);
	AddToken(list, INTEGER, num);

	*idx += 8;
}

void HandleScopeOp(TokenList *list, uint8_t *data, size_t *idx) {
	uint8_t leadByte = data[*idx];
	size_t pkgLength = 0;
	uint8_t byteCount = leadByte & 0b11000000;

	switch(byteCount == 0) {
		case 0:
			pkgLength |= leadByte & 0b00111111;
			*idx += 1;
			break;
		case 1:
			pkgLength |= leadByte & 0b00001111;
			pkgLength |= (data[*idx + 1] << 4);
			*idx += 2;
			break;
		case 2:
			pkgLength |= leadByte & 0b00001111;
			pkgLength |= (data[*idx + 1] << 4);
			pkgLength |= (data[*idx + 2] << 12);
			*idx += 3;
			break;
		case 3:
			pkgLength |= leadByte & 0b00001111;
			pkgLength |= (data[*idx + 1] << 4);
			pkgLength |= (data[*idx + 2] << 12);
			pkgLength |= (data[*idx + 3] << 20);
			*idx += 4;
			break;
	}
	
	const char *name = &data[*idx];
	*idx += 4;

	AddToken(list, SCOPE, pkgLength, name);
}

void HandleBufferOp(TokenList *list, uint8_t *data, size_t *idx) {
	AddToken(list, BUFFER);
}

void HandlePackageOp(TokenList *list, uint8_t *data, size_t *idx) {
	AddToken(list, PACKAGE);
}

AML_Hashmap *CreateHashmap() {
	AML_Hashmap *hashmap = new AML_Hashmap;
	hashmap->Size = 131;
	hashmap->Entries = new AML_HashmapEntry[hashmap->Size];
	
	hashmap->Entries[0] = (AML_HashmapEntry){AML_ZERO_OP, HandleZeroOp};
	hashmap->Entries[1] = (AML_HashmapEntry){AML_ONE_OP, HandleOneOp};
	hashmap->Entries[2] = (AML_HashmapEntry){AML_ALIAS_OP, HandleAliasOp};
	hashmap->Entries[3] = (AML_HashmapEntry){AML_NAME_OP, HandleNameOp};
	hashmap->Entries[4] = (AML_HashmapEntry){AML_BYTEPREFIX, HandleBytePrefix};
	hashmap->Entries[5] = (AML_HashmapEntry){AML_WORDPREFIX, HandleWordPrefix};
	hashmap->Entries[6] = (AML_HashmapEntry){AML_DWORDPREFIX, HandleDWordPrefix};
	hashmap->Entries[7] = (AML_HashmapEntry){AML_QWORDPREFIX, HandleQWordPrefix};
	hashmap->Entries[8] = (AML_HashmapEntry){AML_STRINGPREFIX, HandleStringPrefix};
	hashmap->Entries[9] = (AML_HashmapEntry){AML_SCOPE_OP, HandleScopeOp};
	hashmap->Entries[10] = (AML_HashmapEntry){AML_BUFFER_OP, NULL};
	hashmap->Entries[11] = (AML_HashmapEntry){AML_PACKAGE_OP, NULL};
	hashmap->Entries[12] = (AML_HashmapEntry){AML_VARPACKAGE_OP, NULL};
	hashmap->Entries[13] = (AML_HashmapEntry){AML_METHOD_OP, NULL};
	hashmap->Entries[14] = (AML_HashmapEntry){AML_EXTERNAL_OP, NULL};
	hashmap->Entries[15] = (AML_HashmapEntry){AML_DUAL_PREFIX, NULL};
	hashmap->Entries[16] = (AML_HashmapEntry){AML_MULTI_PREFIX, NULL};
	hashmap->Entries[17] = (AML_HashmapEntry){AML_EXTOP_PREFIX, NULL};
	hashmap->Entries[18] = (AML_HashmapEntry){AML_ROOT_CHAR, NULL};
	hashmap->Entries[19] = (AML_HashmapEntry){AML_PARENT_CHAR, NULL};
	hashmap->Entries[20] = (AML_HashmapEntry){AML_LOCAL0_OP, NULL};
	hashmap->Entries[21] = (AML_HashmapEntry){AML_LOCAL1_OP, NULL};
	hashmap->Entries[22] = (AML_HashmapEntry){AML_LOCAL2_OP, NULL};
	hashmap->Entries[23] = (AML_HashmapEntry){AML_LOCAL3_OP, NULL};
	hashmap->Entries[24] = (AML_HashmapEntry){AML_LOCAL4_OP, NULL};
	hashmap->Entries[25] = (AML_HashmapEntry){AML_LOCAL5_OP, NULL};
	hashmap->Entries[26] = (AML_HashmapEntry){AML_LOCAL6_OP, NULL};
	hashmap->Entries[27] = (AML_HashmapEntry){AML_LOCAL7_OP, NULL};
	hashmap->Entries[28] = (AML_HashmapEntry){AML_ARG0_OP, NULL};
	hashmap->Entries[29] = (AML_HashmapEntry){AML_ARG1_OP, NULL};
	hashmap->Entries[30] = (AML_HashmapEntry){AML_ARG2_OP, NULL};
	hashmap->Entries[31] = (AML_HashmapEntry){AML_ARG3_OP, NULL};
	hashmap->Entries[32] = (AML_HashmapEntry){AML_ARG4_OP, NULL};
	hashmap->Entries[33] = (AML_HashmapEntry){AML_ARG5_OP, NULL};
	hashmap->Entries[34] = (AML_HashmapEntry){AML_ARG6_OP, NULL};
	hashmap->Entries[35] = (AML_HashmapEntry){AML_STORE_OP, NULL};
	hashmap->Entries[36] = (AML_HashmapEntry){AML_REFOF_OP, NULL};
	hashmap->Entries[37] = (AML_HashmapEntry){AML_ADD_OP, NULL};
	hashmap->Entries[38] = (AML_HashmapEntry){AML_CONCAT_OP, NULL};
	hashmap->Entries[39] = (AML_HashmapEntry){AML_SUBTRACT_OP, NULL};
	hashmap->Entries[40] = (AML_HashmapEntry){AML_INCREMENT_OP, NULL};
	hashmap->Entries[41] = (AML_HashmapEntry){AML_DECREMENT_OP, NULL};
	hashmap->Entries[42] = (AML_HashmapEntry){AML_MULTIPLY_OP, NULL};
	hashmap->Entries[43] = (AML_HashmapEntry){AML_DIVIDE_OP, NULL};
	hashmap->Entries[44] = (AML_HashmapEntry){AML_SHL_OP, NULL};
	hashmap->Entries[45] = (AML_HashmapEntry){AML_SHR_OP, NULL};
	hashmap->Entries[46] = (AML_HashmapEntry){AML_AND_OP, NULL};
	hashmap->Entries[47] = (AML_HashmapEntry){AML_OR_OP, NULL};
	hashmap->Entries[48] = (AML_HashmapEntry){AML_XOR_OP, NULL};
	hashmap->Entries[49] = (AML_HashmapEntry){AML_NAND_OP, NULL};
	hashmap->Entries[50] = (AML_HashmapEntry){AML_NOR_OP, NULL};
	hashmap->Entries[51] = (AML_HashmapEntry){AML_NOT_OP, NULL};
	hashmap->Entries[52] = (AML_HashmapEntry){AML_FINDSETLEFTBIT_OP, NULL};
	hashmap->Entries[53] = (AML_HashmapEntry){AML_FINDSETRIGHTBIT_OP, NULL};
	hashmap->Entries[54] = (AML_HashmapEntry){AML_DEREF_OP, NULL};
	hashmap->Entries[55] = (AML_HashmapEntry){AML_CONCATRES_OP, NULL};
	hashmap->Entries[56] = (AML_HashmapEntry){AML_MOD_OP, NULL};
	hashmap->Entries[57] = (AML_HashmapEntry){AML_NOTIFY_OP, NULL};
	hashmap->Entries[58] = (AML_HashmapEntry){AML_SIZEOF_OP, NULL};
	hashmap->Entries[59] = (AML_HashmapEntry){AML_INDEX_OP, NULL};
	hashmap->Entries[60] = (AML_HashmapEntry){AML_MATCH_OP, NULL};
	hashmap->Entries[61] = (AML_HashmapEntry){AML_DWORDFIELD_OP, NULL};
	hashmap->Entries[62] = (AML_HashmapEntry){AML_WORDFIELD_OP, NULL};
	hashmap->Entries[63] = (AML_HashmapEntry){AML_BYTEFIELD_OP, NULL};
	hashmap->Entries[64] = (AML_HashmapEntry){AML_BITFIELD_OP, NULL};
	hashmap->Entries[65] = (AML_HashmapEntry){AML_OBJECTTYPE_OP, NULL};
	hashmap->Entries[66] = (AML_HashmapEntry){AML_QWORDFIELD_OP, NULL};
	hashmap->Entries[67] = (AML_HashmapEntry){AML_LAND_OP, NULL};
	hashmap->Entries[68] = (AML_HashmapEntry){AML_LOR_OP, NULL};
	hashmap->Entries[69] = (AML_HashmapEntry){AML_LNOT_OP, NULL};
	hashmap->Entries[70] = (AML_HashmapEntry){AML_LEQUAL_OP, NULL};
	hashmap->Entries[71] = (AML_HashmapEntry){AML_LGREATER_OP, NULL};
	hashmap->Entries[72] = (AML_HashmapEntry){AML_LLESS_OP, NULL};
	hashmap->Entries[73] = (AML_HashmapEntry){AML_TOBUFFER_OP, NULL};
	hashmap->Entries[74] = (AML_HashmapEntry){AML_TODECIMALSTRING_OP, NULL};
	hashmap->Entries[75] = (AML_HashmapEntry){AML_TOHEXSTRING_OP, NULL};
	hashmap->Entries[76] = (AML_HashmapEntry){AML_TOINTEGER_OP, NULL};
	hashmap->Entries[77] = (AML_HashmapEntry){AML_TOSTRING_OP, NULL};
	hashmap->Entries[78] = (AML_HashmapEntry){AML_MID_OP, NULL};
	hashmap->Entries[79] = (AML_HashmapEntry){AML_COPYOBJECT_OP, NULL};
	hashmap->Entries[80] = (AML_HashmapEntry){AML_CONTINUE_OP, NULL};
	hashmap->Entries[81] = (AML_HashmapEntry){AML_IF_OP, NULL};
	hashmap->Entries[82] = (AML_HashmapEntry){AML_ELSE_OP, NULL};
	hashmap->Entries[83] = (AML_HashmapEntry){AML_WHILE_OP, NULL};
	hashmap->Entries[84] = (AML_HashmapEntry){AML_NOP_OP, NULL};
	hashmap->Entries[85] = (AML_HashmapEntry){AML_RETURN_OP, NULL};
	hashmap->Entries[86] = (AML_HashmapEntry){AML_BREAK_OP, NULL};
	hashmap->Entries[87] = (AML_HashmapEntry){AML_BREAKPOINT_OP, NULL};
	hashmap->Entries[88] = (AML_HashmapEntry){AML_ONES_OP, NULL};
	hashmap->Entries[89] = (AML_HashmapEntry){AML_MUTEX, NULL};
	hashmap->Entries[90] = (AML_HashmapEntry){AML_EVENT, NULL};
	hashmap->Entries[91] = (AML_HashmapEntry){AML_CONDREF_OP, NULL};
	hashmap->Entries[92] = (AML_HashmapEntry){AML_ARBFIELD_OP, NULL};
	hashmap->Entries[93] = (AML_HashmapEntry){AML_STALL_OP, NULL};
	hashmap->Entries[94] = (AML_HashmapEntry){AML_SLEEP_OP, NULL};
	hashmap->Entries[95] = (AML_HashmapEntry){AML_ACQUIRE_OP, NULL};
	hashmap->Entries[96] = (AML_HashmapEntry){AML_RELEASE_OP, NULL};
	hashmap->Entries[97] = (AML_HashmapEntry){AML_SIGNAL_OP, NULL};
	hashmap->Entries[98] = (AML_HashmapEntry){AML_WAIT_OP, NULL};
	hashmap->Entries[99] = (AML_HashmapEntry){AML_RESET_OP, NULL};
	hashmap->Entries[100] = (AML_HashmapEntry){AML_FROM_BCD_OP, NULL};
	hashmap->Entries[101] = (AML_HashmapEntry){AML_TO_BCD_OP, NULL};
	hashmap->Entries[102] = (AML_HashmapEntry){AML_REVISION_OP, NULL};
	hashmap->Entries[103] = (AML_HashmapEntry){AML_DEBUG_OP, NULL};
	hashmap->Entries[104] = (AML_HashmapEntry){AML_FATAL_OP, NULL};
	hashmap->Entries[105] = (AML_HashmapEntry){AML_TIMER_OP, NULL};
	hashmap->Entries[106] = (AML_HashmapEntry){AML_OPREGION, NULL};
	hashmap->Entries[107] = (AML_HashmapEntry){AML_FIELD, NULL};
	hashmap->Entries[108] = (AML_HashmapEntry){AML_DEVICE, NULL};
	hashmap->Entries[109] = (AML_HashmapEntry){AML_PROCESSOR, NULL};
	hashmap->Entries[110] = (AML_HashmapEntry){AML_POWER_RES, NULL};
	hashmap->Entries[111] = (AML_HashmapEntry){AML_THERMALZONE, NULL};
	hashmap->Entries[112] = (AML_HashmapEntry){AML_INDEXFIELD, NULL};
	hashmap->Entries[113] = (AML_HashmapEntry){AML_BANKFIELD, NULL};
	hashmap->Entries[114] = (AML_HashmapEntry){AML_FIELD_ANY_ACCESS, NULL};
	hashmap->Entries[115] = (AML_HashmapEntry){AML_FIELD_BYTE_ACCESS, NULL};
	hashmap->Entries[116] = (AML_HashmapEntry){AML_FIELD_WORD_ACCESS, NULL};
	hashmap->Entries[117] = (AML_HashmapEntry){AML_FIELD_DWORD_ACCESS, NULL};
	hashmap->Entries[118] = (AML_HashmapEntry){AML_FIELD_QWORD_ACCESS, NULL};
	hashmap->Entries[119] = (AML_HashmapEntry){AML_FIELD_LOCK, NULL};
	hashmap->Entries[120] = (AML_HashmapEntry){AML_FIELD_PRESERVE, NULL};
	hashmap->Entries[121] = (AML_HashmapEntry){AML_FIELD_WRITE_ONES, NULL};
	hashmap->Entries[122] = (AML_HashmapEntry){AML_FIELD_WRITE_ZEROES, NULL};
	hashmap->Entries[123] = (AML_HashmapEntry){AML_METHOD_ARGC_MASK, NULL};
	hashmap->Entries[124] = (AML_HashmapEntry){AML_METHOD_SERIALIZED, NULL};
	hashmap->Entries[125] = (AML_HashmapEntry){AML_MATCH_MTR, NULL};
	hashmap->Entries[126] = (AML_HashmapEntry){AML_MATCH_MEQ, NULL};
	hashmap->Entries[127] = (AML_HashmapEntry){AML_MATCH_MLE, NULL};
	hashmap->Entries[128] = (AML_HashmapEntry){AML_MATCH_MLT, NULL};
	hashmap->Entries[129] = (AML_HashmapEntry){AML_MATCH_MGE, NULL};
	hashmap->Entries[130] = (AML_HashmapEntry){AML_MATCH_MGT, NULL};

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
			case ZERO:
				MKMI_Printf("ZERO\r\n");
				break;
			case ONE:
				MKMI_Printf("ONE\r\n");
				break;
			case ALIAS:
				Memcpy(name, current->Alias.NameOne, 4);
				MKMI_Printf("ALIAS, Value One: %s, Value Two: ", name);
				Memcpy(name, current->Alias.NameTwo, 4);
				MKMI_Printf("%s\r\n", name);
				break;
			case NAME:
				Memcpy(name, current->Name.Name, 4);
				MKMI_Printf("NAME, Value: %s\r\n", name);
				break;
			case INTEGER:
				MKMI_Printf("INTEGER, Value: %d\r\n", current->Int);
				break;
			case STRING:
				MKMI_Printf("STRING, Value: %s\r\n", current->String);
				break;
			case SCOPE:
				Memcpy(name, current->Scope.Name, 4);
				MKMI_Printf("SCOPE, Size: %d, Name: %s\r\n", current->Scope.Length, name);
				break;
			case BUFFER:
				MKMI_Printf("BUFFER\r\n");
				break;
			case PACKAGE:
				MKMI_Printf("PACKAGE\r\n");
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
