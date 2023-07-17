#pragma once
#include <stdint.h>

struct EXT2Superblock {
	uint32_t TotalInodes;
	uint32_t TotalBlocks;
	uint32_t ReservedBlocks;
	uint32_t UnallocatedBlocks;
	uint32_t UnallocatedInodes;
	uint32_t StartingBlock;
	uint32_t BlockSize; /* Real block size is 1024 << BlockSize */
	uint32_t FragmentSize; /* Real fragment size is 1024 << FragmentSize */
	uint32_t BlocksPerGroup;
	uint32_t FragmentsPerGroup;
	uint32_t InodesPerGroup;
	uint32_t LastMountTime;
	uint32_t LastWrittenTime;
	uint16_t MountSinceLastCheck;
	uint16_t MountAllowedSinceLastCheck;
	uint32_t EXTSignature; /* Signature is 0xEF53 */
	uint16_t FilesystemState;
	uint16_t IfErrorDetected;
	uint16_t MinorVersion;
	uint32_t LastCheckTime;
	uint32_t TimeBetweenForcedChecks;
	uint32_t OSID;
	uint32_t MajorVersion;
	uint16_t ReservedUserID;
	uint16_t ReservedGroupID;
}__attribute__((packed));

struct ExtendedEXT2Superblock {
	uint32_t FirstFreeInode;
	uint16_t SizeOfInode;
	uint16_t SuperblockBlockGroup;
	uint32_t OptionalFeatures;
	uint32_t RequiredFeatures; /* If they are requuired, we must provide them or we will not read or write */
	uint32_t ReadOnlyFeatures; /* If they aren't supported, we must mount read only */
	char FilesystemID[16];
	char VolumeName[16];
	char LastMountPath[64];
	uint32_t CompressionAlgorithmUsed;
	uint8_t NumberBlockPreallocateFiles;
	uint8_t NumberBlockPreallocateDirectories;
	uint16_t Unused;
	char JournalID[16];
	uint32_t JournalInode;
	uint32_t JournalDevice;
	uint32_t HeadOfOrphanInodeList;
	/* From byte 236 to 1023 is unused */
}__attribute__((packed));
