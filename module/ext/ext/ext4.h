#pragma once
#include <stdint.h>

struct EXT4Superblock {
	uint32_t TotalInodes;
	uint32_t TotalBlocks;
	uint32_t ReservedBlocks;
	uint32_t UnallocatedBlocks;
	uint32_t UnallocatedInodes;
	uint32_t StartingBlock; /* 1 if block size is 1024, 0 in others */
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

struct EXT4DynamicSuperblocks {
	uint32_t FirstFreeInode;
	uint16_t SizeofInode;
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
	uint16_t ReservedGDTEntries;
	char JournalID[16];
	uint32_t JournalInode;
	uint32_t JournalDevice;
	uint32_t HeadOfOrphanInodeList;
	uint32_t HTREEHashSeed[4];
	uint8_t DirectoryHashAlgorithm;
	uint8_t JournalBlocksFieldIsCopyOfInodeBlockArrayAndSize;
	uint16_t SizeOfGroupDescriptors; /* For 64-bit mode */
	uint32_t MountOptions;
	uint32_t FirstMetablockBlockGroup; /* If enabled */
	uint32_t FSCreationTime;
	uint32_t JournalInodeBackup[17];
}__attribute__((packed));

/*struct EXT464bitSuperblock {
	uint32_t HighTotalBlocks;
	uint32_t HighReservedBlocks;
	uint32_t HighUnallocatedBlocks;
	uint16_t MinimumInodeSize;
	uint16_t MinimumInodeReservationSize;
	uint32_t MiscFlags; 
	uint16_t Amount logical blocks read or written per disk in a RAID array.;
	uint16_t Amount of seconds to wait in Multi-mount prevention checking.;
	uint64_t Block to multi-mount prevent.;
	uint32_t Amount of blocks to read or write before returning to the current disk in a RAID array. Amount of disks * stride.;
	uint8_t log2 (groups per flex) - 10. (In other words, the number to shift 1,024 to the left by to obtain the groups per flex block group);
	uint8_t MetadataChecksumAlgorithm;
	uint8_t EncryptionVersionLevel;
	uint8_t	Reserved;
	uint64_t Amount of kilobytes written over the filesystem's lifetime.;
	uint32_t Inode number of the active snapshot.;
	uint32_t Sequential ID of active snapshot.;
	uint64_t Number of blocks reserved for active snapshot.;
	uint32_t Inode number of the head of the disk snapshot list.;
	uint32_t Amount of errors detected.;
	uint32_t First time an error occurred in POSIX time.;
	uint32_t Inode number in the first error.;
	uint64_t Block number in the first error.;
	char FunctionFirstErrorOccurred[32];
	uint32_t Line number where the first error occurred.;
	uint32_t Most recent time an error occurred in POSIX time.;
	uint32_t Inode number in the last error.;
	uint64_t Block number in the last error.;
	char FunctionMostRecentErrorOccurred[32];
	uint32_t Line number where the most recent error occurred.;
	64 	Mount options. (C-style string: characters terminated by a 0 byte);
	uint32_t Inode number for user quota file.;
	uint32_t Inode number for group quota file.;
	uint32_t Overhead blocks/clusters in filesystem. Zero means the kernel calculates it at runtime.;
	uint64_t Block groups with backup Superblocks, if the sparse superblock flag is set.;
	uint32_t Encryption algorithms used, as a array of unsigned char.;
	16 	Salt for the `string2key` algorithm.;
	uint32_t Inode number of the lost+found directory.;
	uint32_t Inode number of the project quota tracker.;
	uint32_t Checksum of the UUID, used for the checksum seed. (crc32c(~0, UUID));
	uint8_t High 8-bits of the last written time field.;
	uint8_t High 8-bits of the last mount time field.;
	uint8_t High 8-bits of the Filesystem creation time field.;
	uint8_t High 8-bits of the last consistency check time field.;
	uint8_t High 8-bits of the first time an error occurred time field.;
	uint8_t High 8-bits of the latest time an error occurred time field.;
	uint8_t Error code of the first error.;
	uint8_t Error code of the latest error.;
	uint16_t Filename charset encoding.;
	uint16_t Filename charset encoding flags.;
	char Padding[380];
	uint32_t SuperblockChecksum;
}__attribute__((packed));*/
