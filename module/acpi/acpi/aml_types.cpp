#include "aml_types.h"
#include "aml_opcodes.h"

#include <mkmi_memory.h>

void HandleNameTypeSegments(NameType *name, uint8_t *data, size_t *idx) {
	if (data[*idx] == AML_DUAL_PREFIX) {
		/* Dual name */
		*idx += 1;

		name->SegmentNumber = 2;
		name->NameSegments = Malloc(name->SegmentNumber * 4);
		Memcpy(name->NameSegments + 0, &data[*idx], 4);
		Memcpy(name->NameSegments + 4, &data[*idx + 4], 4);

		*idx += 8;
	} else if (data[*idx] == AML_MULTI_PREFIX) {
		/* Multiple name segments */
		*idx += 1;

		name->SegmentNumber = data[*idx];

		*idx += 1;

		name->NameSegments = Malloc(name->SegmentNumber * 4);

		for (size_t i = 0; i < name->NameSegments; ++i) {
			Memcpy(name->NameSegments + (i * 4), &data[*idx], 4);
			*idx += 4;
		}
	} else if (data[*idx] == 0x00) {
		/* The name is NULL */
		*idx += 1;

		name->SegmentNumber = 0;
		name->NameSegments = NULL;
	} else {
		/* Simple name segment */
		name->SegmentNumber = 1;
		name->NameSegments = Malloc(name->SegmentNumber * 4);
		Memcpy(name->NameSegments + 0, &data[*idx], 4);

		*idx += 4;
	}
}

void HandleNameType(NameType *name, uint8_t *data, size_t *idx) {
	if(data[*idx] == AML_ROOT_CHAR) {
		name->IsRoot = true;
	} else if (data[*idx] == AML_PARENT_CHAR) {
		name->IsRoot = false;
	}

	*idx += 1;
	
	HandleNameTypeSegments(name, data, idx);
}
