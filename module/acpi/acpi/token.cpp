#include "token.h"

#include <stdarg.h>
#include <mkmi_memory.h>

TokenList *CreateTokenList() {
	TokenList *list = new TokenList;
	list->Head = NULL;
	list->Tail = NULL;

	return list;
}

// Function to create a new token and add it to the token list
void AddToken(TokenList *tokenList, TokenType type, ...) {
	va_list ap;
	va_start(ap, type);

	Token *newToken = new Token;
	newToken->Type = type;

	switch(newToken->Type) {
		case ALIAS: {
			NameType *nameOne = va_arg(ap, NameType*);
			newToken->Alias.NameOne.IsRoot = nameOne->IsRoot;
			newToken->Alias.NameOne.SegmentNumber = nameOne->SegmentNumber;
			newToken->Alias.NameOne.NameSegments = nameOne->NameSegments;

			NameType *nameTwo = va_arg(ap, NameType*);
			newToken->Alias.NameTwo.IsRoot = nameTwo->IsRoot;
			newToken->Alias.NameTwo.SegmentNumber = nameTwo->SegmentNumber;
			newToken->Alias.NameTwo.NameSegments = nameTwo->NameSegments;

			}
			break;
		case NAME: {
			NameType *name = va_arg(ap, NameType*);
			newToken->Name.IsRoot = name->IsRoot;
			newToken->Name.SegmentNumber = name->SegmentNumber;
			newToken->Name.NameSegments = name->NameSegments;
			}
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
		case SCOPE: {
			NameType *name = va_arg(ap, NameType*);
			newToken->Name.IsRoot = name->IsRoot;
			newToken->Name.SegmentNumber = name->SegmentNumber;
			newToken->Name.NameSegments = name->NameSegments;
			
			newToken->Scope.PkgLength = va_arg(ap, uint32_t);
			}
			break;
		case BUFFER:
			newToken->Buffer.PkgLength = va_arg(ap, uint32_t);
			break;
		case PACKAGE:
			newToken->Package.PkgLength = va_arg(ap, uint32_t);
			newToken->Package.NumElements = va_arg(ap, uint32_t);
			break;
		case UNKNOWN:
			newToken->UnknownOpcode = va_arg(ap, uint32_t);
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
