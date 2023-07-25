#include "instruction_handlers.h"
#include "aml_types.h"
#include "instruction_hashmap.h"
#include "token.h"
#include "aml_opcodes.h"

#include <mkmi_log.h>

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

void HandleIntegerOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	IntegerType integer;
	HandleIntegerType(&integer, data, idx);
	AddToken(list, INTEGER, &integer);
}

void HandleStringPrefix(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	const char *str = &data[*idx];
	size_t len = 2; /* First char + '\0' */

	while(data[*idx] != '\0') { ++len; *idx += 1; }
	
	AddToken(list, STRING, str, len);
}

void HandleScopeOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	uint32_t pkgLength = 0;

	HandlePkgLengthType((uint8_t*)&pkgLength, data, idx);

	NameType name;
	HandleNameType(&name, data, idx);
	AddToken(list, SCOPE, &name, pkgLength);
}

void HandleBufferOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	uint32_t pkgLength = 0;
	HandlePkgLengthType((uint8_t*)&pkgLength, data, idx);

	/* TODO: There is still BufferSize to handle */

	AddToken(list, BUFFER, pkgLength);
}

void HandlePackageOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	uint32_t pkgLength = 0;
	HandlePkgLengthType((uint8_t*)&pkgLength, data, idx);

	uint32_t numElements = 0;
	numElements |= data[*idx];
	*idx += 1;

	AddToken(list, PACKAGE, pkgLength, numElements);

}

void HandleExtendedOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	*idx += 1;
	switch(data[*idx - 1]) {
		case AML_OPREGION:
			HandleExtOpRegion(hashmap, list, data, idx);
			break;
		default:
			uint32_t code = data[*idx - 1];
			AddToken(list, UNKNOWN, code);
			break;
	}
}

void HandleExtOpRegion(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	NameType name;
	HandleNameType(&name, data, idx);
	uint32_t regionSpace = data[*idx];
	*idx+=1;

	IntegerType regionOffset, regionLen;
	
	*idx+=1;
	HandleIntegerType(&regionOffset, data, idx);
	*idx+=1;
	HandleIntegerType(&regionLen, data, idx);


	AddToken(list, REGION, &name, regionSpace, &regionOffset, &regionLen);
}

void HandleExtOpField(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx) {
	uint32_t pkgLength = 0;
	HandlePkgLengthType((uint8_t*)&pkgLength, data, idx);

	NameType name;
	HandleNameType(&name, data, idx);

	uint8_t fieldFlags = data[*idx];
	*idx+=1;

	/* Field list here */

	AddToken(list, FIELD, pkgLength, &name, fieldFlags);
}
