#pragma once

#include <stdint.h>
#include <stddef.h>

struct NameType {
	bool IsRoot;

	uint8_t SegmentNumber;
	char *NameSegments;
};

void HandleNameTypeSegments(NameType *name, uint8_t *data, size_t *idx);
void HandleNameType(NameType *name, uint8_t *data, size_t *idx);
