#include "instruction_handlers.h"
#include "instruction_hashmap.h"
#include "token.h"

void HandleZeroOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	AddToken(list, ZERO);
}

void HandleOneOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	AddToken(list, ONE);
}

void HandleAliasOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	NameType nameOne, nameTwo;

	HandleNameType(&nameOne, data, idx);
	HandleNameType(&nameTwo, data, idx);

	AddToken(list, ALIAS, &nameOne, &nameTwo);
}

void HandleNameOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	NameType name;
	HandleNameType(&name, data, idx);
	AddToken(list, NAME, &name);
}

void HandleBytePrefix(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	uint64_t num = 0;
	num |= data[*idx];

	AddToken(list, INTEGER, num);
	
	*idx += 1;
	
}

void HandleWordPrefix(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	uint64_t num = 0;
	num |= data[*idx];
	num |= (data[*idx + 1] << 8);

	AddToken(list, INTEGER, num);
	
	*idx += 2;
}


void HandleDWordPrefix(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	uint64_t num = 0;
	num |= data[*idx];
	num |= (data[*idx + 1] << 8);
	num |= (data[*idx + 2] << 16);
	num |= (data[*idx + 3] << 24);

	AddToken(list, INTEGER, num);

	*idx += 4;
}

void HandleStringPrefix(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	const char *str = &data[*idx];
	size_t len = 2; /* First char + '\0' */

	while(data[*idx] != '\0') { ++len; *idx += 1; }
	
	AddToken(list, STRING, str, len);
}

void HandleQWordPrefix(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
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

void HandleScopeOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	uint8_t leadByte = data[*idx];
	uint32_t pkgLength = 0;
	uint8_t byteCount = leadByte & 0b11000000;

	byteCount >>= 6;

	switch(byteCount) {
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


	NameType name;
	HandleNameType(&name, data, idx);
	AddToken(list, SCOPE, &name, pkgLength);

/*
	Token *ourToken = list->Tail;

	ourToken->Children = CreateTokenList();

	size_t currentIndex = *idx;
	while(*idx < currentIndex + pkgLength) {
		ParseByte(ourToken->Children, hashmap, data, idx);
	}
*/

}

void HandleBufferOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	uint8_t leadByte = data[*idx];
	uint32_t pkgLength = 0;
	uint8_t byteCount = leadByte & 0b11000000;

	switch(byteCount) {
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

	/* TODO: There is still BufferSize to handle */

	AddToken(list, BUFFER, pkgLength);
}

void HandlePackageOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	uint8_t leadByte = data[*idx];
	uint32_t pkgLength = 0;
	uint8_t byteCount = leadByte & 0b11000000;

	switch(byteCount) {
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



	size_t numElements = 0;
	numElements |= data[*idx];
	*idx += 1;

	AddToken(list, PACKAGE, pkgLength, numElements);

}

