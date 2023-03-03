#include <fs/fs.h>
#include <stdio.h>
#include <sys/panik.h>
#include <string.h>
#include <mm/heap.h>
#include <mm/pageframe.h>
#include <init/kutil.h>

#define PREFIX "[FS] "

static const char *FilesystemStrings[] = {"FAT", "Unknown"};

struct MBRPartition {
	uint8_t attributes;
	uint32_t chs_addr_start:3;
	uint8_t partition_type;
	uint32_t chs_addr_end:3;
	uint32_t lba_start;
	uint32_t sectors;
}__attribute__((packed));

struct GPTHeader {
	char signature[8];
	uint32_t gpt_revision;
	uint32_t header_size;
	uint32_t crc32_checksum;
	uint32_t reserved0;
	uint64_t curr_lba;
	uint64_t alternate_lba;
	uint64_t first_usable;
	uint64_t last_usable;
	char guid[16];
	uint64_t start_lba;
	uint32_t partitions;
	uint32_t size_partition_entry;
	uint32_t crc32_array;
}__attribute__((packed));

Filesystem::FSManager *GlobalFSManager;

namespace Filesystem {
void FSManager::InitializeMBRPartitions(uint8_t *bootsector) {
	uint8_t *partitionTable = bootsector + 0x1BE;

	for (int i = 0; i < 4; i++) {
		MBRPartition *partition = (MBRPartition*)malloc(sizeof(MBRPartition));
		memcpy(partition, partitionTable +  i * sizeof(MBRPartition), sizeof(MBRPartition));
		switch(partition->partition_type) {
			case 0x01: {// All disk FAT
				dprintf(PREFIX "Initializing FAT Driver:\n");

				if(supportedDrives[total_drives-1]->partitions[i].fatDriver.DetectDrive(bootsector)) {
					supportedDrives[total_drives-1]->partitions[i].fatDriver.drive = total_drives - 1;
					supportedDrives[total_drives-1]->partitions[i].fatDriver.partition = i;
					//supportedDrives[total_drives-1].partitions[i].fatDriver.offset = partition->chs_addr_start;

				        supportedDrives[total_drives-1]->partitions[i].filesystem = Filesystem::FAT;
					//supportedDrives[total_drives-1].partitions[i].fatDriver.ReadDirectory(supportedDrives[total_drives-1].partitions[0].fatDriver.FindDirectory("/MICROK"));
					//uint8_t *file = supportedDrives[total_drives-1].partitions[i].fatDriver.LoadFile("/MICROK", "INITRD");

					//kInfo.initrd = file;
					//kInfo.initrd_loaded = true;
				}
				}
				break;
			default:
				break;
		}
	}
}

void FSManager::InitializeGPTPartitions(uint8_t *partitionTable) {
}

void FSManager::AddAHCIDrive(AHCI::Port *port, int number, uint32_t buffer_size) {
        supportedDrives[total_drives] = new Drive;
        supportedDrives[total_drives]->driveType = DriveType::AHCI;
        supportedDrives[total_drives]->driver.ahciDriver.port = port;
        supportedDrives[total_drives]->driver.ahciDriver.port_number = number;
        supportedDrives[total_drives]->driver.ahciDriver.port->Configure();
        supportedDrives[total_drives]->driver.ahciDriver.buffer_size = buffer_size;
        supportedDrives[total_drives]->driver.ahciDriver.port->buffer = (uint8_t*)GlobalAllocator.RequestPages(buffer_size/0x1000);
        if(supportedDrives[total_drives]->driver.ahciDriver.port->buffer == NULL) {
                PANIK("Failed to create AHCI buffer.");
        }

        supportedDrives[total_drives]->driver.ahciDriver.port->Read(total_drives, 1, supportedDrives[total_drives]->driver.ahciDriver.port->buffer);

        uint8_t *fs_buffer = (uint8_t*)malloc(0x200);

        memcpy(fs_buffer, supportedDrives[total_drives]->driver.ahciDriver.port->buffer, 0x200);

        bool empty = true;
        for (int i = 0; i < 512; i++) {
                if(fs_buffer[i] != 0) empty = false;
        }

        total_drives++;
        if (!empty) {
		uint8_t *partition = (uint8_t*)malloc(sizeof(MBRPartition));
		memcpy(partition, fs_buffer + 0x1BE, sizeof(MBRPartition));

		MBRPartition *first = (MBRPartition*)malloc(sizeof(MBRPartition));
		memcpy(first, partition, sizeof(MBRPartition));

		if (first->attributes == 0x00 &&
		    first->chs_addr_start == 0x000200 &&
		    first->partition_type == 0xEE &&
		    first->lba_start == 0x00000001) {
			dprintf(PREFIX "Found an MBR Protective Partition.\n");
		} else if (first->attributes == 0x00 &&
		    first->chs_addr_start == 0x000000 &&
		    first->partition_type == 0x00 &&
		    first->lba_start == 0x00000000) {
			dprintf(PREFIX "Found a GPT disk without a MBR.\n");
			GPTHeader *header = (GPTHeader*)malloc(sizeof(GPTHeader));
			memcpy(header, fs_buffer, sizeof(GPTHeader));
			if (memcmp(header->signature, "EFI PART", 8) == 0) dprintf(PREFIX "Valid GPT header found.\n");

			free(header);
		} else {
			dprintf(PREFIX "Found a legacy MBR.\n");
			InitializeMBRPartitions(fs_buffer);
		}

		free(first);
		free(partition);
        } else {
                dprintf(PREFIX "Empty drive\n");
                supportedDrives[total_drives-1]->partitions[0].filesystem = Filesystem::UNKNOWN;
        }

        free(fs_buffer);
}

FSManager::FSManager() {
        dprintf(PREFIX "Initializing the file system manager.\n");
        total_drives = 0;
}

bool FSManager::ReadDrive(uint8_t drive_number, uint32_t start_sector, uint32_t number_sectors, uint8_t **buffer, size_t buffer_size, size_t sector_size) {
        switch(supportedDrives[drive_number]->driveType) {
                case DriveType::AHCI: {
                        uint16_t sector_mul = supportedDrives[drive_number]->driver.ahciDriver.buffer_size / sector_size;
                        for (uint64_t base = 0; base < buffer_size; base += sector_size * sector_mul) {
                                supportedDrives[drive_number]->driver.ahciDriver.port->Read(start_sector + base / sector_size, sector_mul, supportedDrives[drive_number]->driver.ahciDriver.port->buffer);
                                memcpy(*buffer + base,
                                       supportedDrives[drive_number]->driver.ahciDriver.port->buffer,
                                       sector_size * sector_mul > buffer_size - base ? buffer_size - base : sector_size * sector_mul);
                        }
                        return true;
                }
        }
}

bool FSManager::WriteDrive(uint8_t drive_number, uint32_t start_sector, uint8_t number_sectors, uint8_t *buffer, size_t buffer_size) {
        switch(supportedDrives[drive_number]->driveType) {
                case DriveType::AHCI: {
                        memcpy(supportedDrives[drive_number]->driver.ahciDriver.port->buffer,
                               buffer,
                               supportedDrives[drive_number]->driver.ahciDriver.buffer_size > buffer_size ? buffer_size : supportedDrives[drive_number]->driver.ahciDriver.buffer_size);
                        return supportedDrives[drive_number]->driver.ahciDriver.port->Write(start_sector, number_sectors, supportedDrives[drive_number]->driver.ahciDriver.port->buffer);
                }
        }
}
}
