#pragma once
#include <stdint.h>

struct RSDP2 {
	unsigned char Signature[8];
	uint8_t Checksum;
	uint8_t OEMID[6];
	uint8_t Revision;
	uint32_t RSDTAddress;
	uint32_t Length;
	uint64_t XSDTAddress;
	uint8_t ExtendedChecksum;
	uint8_t Reserved[3];
}__attribute__((packed));

struct SDTHeader {
	unsigned char Signature[4];
	uint32_t Length;
	uint8_t Revision;
	uint8_t Checksum;
	uint8_t OEMID[6];
	uint8_t OEMTableID[8];
	uint32_t OEMRevision;
	uint32_t CreatorID;
	uint32_t CreatorRevision;
}__attribute__((packed));

void Init();
