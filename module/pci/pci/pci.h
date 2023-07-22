#pragma once
#include <stdint.h>
#include <stddef.h>

struct PCIDeviceHeader {
	uint16_t VendorID;
	uint16_t DeviceID;
	uint16_t Command;
	uint16_t Status;
	uint8_t RevisionID;
	uint8_t ProgIF;
	uint8_t Subclass;
	uint8_t Class;
	uint8_t CacheLineSize;
	uint8_t LatencyTimer;
	uint8_t HeaderType;
	uint8_t BIST;
}__attribute__((packed));

struct SDTHeader{
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

struct MCFGHeader{
	SDTHeader Header;
	uint64_t Reserved;
}__attribute__((packed));

struct DeviceConfig{
	uint64_t BaseAddress;
	uint16_t PCISegGroup;
	uint8_t StartBus;
	uint8_t EndBus;
	uint32_t Reserved;
}__attribute__((packed));

void EnumeratePCI(MCFGHeader *mcfg);
