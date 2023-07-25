#pragma once
#include <stdint.h>
#include <stddef.h>

#include "aml_types.h"

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
	REGION,
	FIELD,
};

struct TokenList;

struct Token {
	TokenType Type;

	union {
		uint8_t UnknownOpcode;
		/* Zero */
		/* One */
		struct {
			NameType NameOne;
			NameType NameTwo;
		} Alias;
		
		NameType Name;

		IntegerType Int;
		
		char *String;

		struct {
			uint32_t PkgLength;
			NameType Name;
		} Scope;

		struct {
			uint32_t PkgLength;
			IntegerType BufferSize;
		} Buffer;

		struct {
			uint32_t PkgLength;
			uint8_t NumElements;
		} Package;

		struct {
			NameType Name;
			uint8_t RegionSpace;
			IntegerType RegionOffset;
			IntegerType RegionLen;
		} Region;
	};

	TokenList *Children;

	Token *Next;    // Pointer to the next token in the linked list
};

struct TokenList {
    Token *Head;    // Pointer to the first token in the list
    Token *Tail;    // Pointer to the last token in the list
};

TokenList *CreateTokenList();
void AddToken(TokenList *tokenList, TokenType type, ...);
void FreeTokenList(TokenList *tokenList);
