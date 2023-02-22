#pragma once
#include <dev/pci/pci.h>
#include <dev/ahci/ahci.h>
#include <fs/fat/fat.h>
#include <stddef.h>
#include <cdefs.h>

namespace Filesystem {
        enum DriveType {
                AHCI,
        };

        enum PartitionType {
                MBR,
                GPT,
        };

        enum Filesystem {
                FAT,
                UNKNOWN,
        };

        struct Partition {
                uint8_t partition_number;
                Filesystem filesystem;
                FAT::FATFSDriver fatDriver;
        };

        struct AHCIDriver {
                AHCI::Port *port;
                uint8_t port_number;
                uint32_t buffer_size;
        };

        union Driver {
                AHCIDriver ahciDriver;
        };

        struct Drive {
                DriveType driveType;
                Driver driver;
                PartitionType partitionType;
                Partition partitions[128];
        };

        class FSManager {
        public:
                FSManager();
                void InstallDrive();
                void AddAHCIDrive(AHCI::Port *port, int number, uint32_t buffer_size);
                bool ReadDrive(uint8_t drive_number, uint32_t start_sector, uint32_t number_sectors, uint8_t **buffer, size_t buffer_size, size_t sector_size);
                bool WriteDrive(uint8_t drive_number, uint32_t start_sector, uint8_t number_sectors, uint8_t *buffer, size_t buffer_size);
                Drive *supportedDrives[CONFIG_VFS_MAX_DRIVES];
                uint16_t total_drives;
	private:
		void InitializeMBRPartitions(uint8_t *bootsector);
		void InitializeGPTPartitions(uint8_t *partitionTable);
        };
}

extern Filesystem::FSManager *GlobalFSManager;

