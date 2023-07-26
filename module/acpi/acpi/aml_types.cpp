#include "aml_types.h"
#include "aml_opcodes.h"

#include <mkmi_memory.h>
#include <mkmi_log.h>

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

		for (size_t i = 0; i < name->SegmentNumber ; ++i) {
			Memcpy(&name->NameSegments[i * 4], &data[*idx], 4);
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
		*idx += 1;
	} else if (data[*idx] == AML_PARENT_CHAR) {
		name->IsRoot = false;
		*idx += 1;
	} else {
		name->IsRoot = false;
	}

	HandleNameTypeSegments(name, data, idx);
}

void HandleIntegerType(IntegerType *integer, uint8_t *data, size_t *idx) {
	size_t moveAmount = 0;

	integer->Data = 0;

	switch(data[*idx - 1]) {
		case AML_QWORDPREFIX:
			moveAmount += 4;
			integer->Data |= (data[*idx + 4] << 32);
			integer->Data |= (data[*idx + 5] << 40);
			integer->Data |= (data[*idx + 6] << 48);
			integer->Data |= (data[*idx + 7] << 52);
		case AML_DWORDPREFIX:
			moveAmount += 2;
			integer->Data |= (data[*idx + 2] << 16);
			integer->Data |= (data[*idx + 3] << 24);
		case AML_WORDPREFIX:
			moveAmount += 1;
			integer->Data |= (data[*idx + 1] << 8);
		case AML_BYTEPREFIX:
			moveAmount += 1;
			integer->Data |= data[*idx];

			*idx += moveAmount;
			break;
		default:
			moveAmount = 1;
			integer->Data |= data[*idx - 1];
			break;
	}

	integer->Size = moveAmount;
}

void HandlePkgLengthType(uint8_t *pkgLength, uint8_t *data, size_t *idx) {
	uint8_t leadByte = data[*idx];
	*pkgLength = 0;
	uint8_t byteCount = leadByte & 0b11000000;
	
	byteCount >>= 6;

	switch(byteCount) {
		case 0:
			*pkgLength |= leadByte & 0b00111111;
			*idx += 1;
			break;
		case 1:
			*pkgLength |= leadByte & 0b00001111;
			*pkgLength |= (data[*idx + 1] << 4);
			*idx += 2;
			break;
		case 2:
			*pkgLength |= leadByte & 0b00001111;
			*pkgLength |= (data[*idx + 1] << 4);
			*pkgLength |= (data[*idx + 2] << 12);
			*idx += 3;
			break;
		case 3:
			*pkgLength |= leadByte & 0b00001111;
			*pkgLength |= (data[*idx + 1] << 4);
			*pkgLength |= (data[*idx + 2] << 12);
			*pkgLength |= (data[*idx + 3] << 20);
			*idx += 4;
			break;
	}
}
