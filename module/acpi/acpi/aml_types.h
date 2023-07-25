#pragma once

#include <stdint.h>
#include <stddef.h>

struct NameType {
	bool IsRoot;

	uint8_t SegmentNumber;
	char *NameSegments;
};

struct IntegerType {
	uint64_t Data;
	uint8_t Size;
};

void HandleNameTypeSegments(NameType *name, uint8_t *data, size_t *idx);
void HandleNameType(NameType *name, uint8_t *data, size_t *idx);
void HandleIntegerType(IntegerType *integer, uint8_t *data, size_t *idx);

void HandlePkgLengthType(uint8_t *pkgLength, uint8_t *data, size_t *idx);
