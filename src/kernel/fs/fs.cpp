#include <fs/fs.h>
#include <sys/printk.h>
#include <string.h>
#include <mm/heap.h>
#include <mm/pageframe.h>
static const char *FilesystemStrings[] = {"FAT", "Unknown"};

Filesystem::FSManager GlobalFSManager;
namespace Filesystem {

void FSManager::AddAHCIDrive(AHCI::Port *port, int number, uint32_t buffer_size) {
        Drive newDrive;
        
        newDrive.driveType = DriveType::AHCI;

        newDrive.driver.ahciDriver.port = port;
        newDrive.driver.ahciDriver.port_number = number;

        newDrive.driver.ahciDriver.port->Configure();

        newDrive.driver.ahciDriver.buffer_size = buffer_size;
        newDrive.driver.ahciDriver.port->buffer = (uint8_t*)GlobalAllocator.RequestPages(buffer_size/0x1000);
        memset(newDrive.driver.ahciDriver.port->buffer, 0, buffer_size);
        newDrive.driver.ahciDriver.port->Read(0, 1, newDrive.driver.ahciDriver.port->buffer);

        uint8_t *fs_buffer = (uint8_t*)malloc(0x200);

        memcpy(fs_buffer, newDrive.driver.ahciDriver.port->buffer, 0x200);

        bool empty = true;
        for (int i = 0; i < 512; i++) {
                if(fs_buffer[i] != 0) empty = false;
        }
        
        if (!empty) {
                // We should first initialize partitions, but that's for the future
                printk("Initializing FAT Driver:\n");
        
                supportedDrives[total_drives++] = newDrive;
                if(supportedDrives[total_drives-1].partitions[0].fatDriver.DetectDrive(fs_buffer)) {
                        supportedDrives[total_drives-1].partitions[0].filesystem = Filesystem::FAT;
                        supportedDrives[total_drives-1].partitions[0].fatDriver.FindDirectory("/");
                        supportedDrives[total_drives-1].partitions[0].fatDriver.FindDirectory("/EFI");
                        supportedDrives[total_drives-1].partitions[0].fatDriver.FindDirectory("/EFI/BOOT");
                }
        } else {
                printk("Empty drive\n");
        }

        free(fs_buffer);
}

FSManager::FSManager() {
        total_drives = 0;
}

uint8_t *FSManager::ReadDrive(uint8_t drive_number, uint32_t start_sector, uint8_t number_sectors) {
        switch(supportedDrives[drive_number - 1].driveType) {
                case DriveType::AHCI: {
                        //memset(supportedDrives[drive_number - 1].driver.ahciDriver.port->buffer, 'A', supportedDrives[drive_number - 1].driver.ahciDriver.buffer_size);
                        supportedDrives[drive_number - 1].driver.ahciDriver.port->Read(start_sector, number_sectors, supportedDrives[drive_number - 1].driver.ahciDriver.port->buffer);
                        uint8_t *tmp = supportedDrives[drive_number - 1].driver.ahciDriver.port->buffer;
                        return tmp;
                }
        }
}

bool FSManager::WriteDrive(uint8_t drive_number, uint32_t start_sector, uint8_t number_sectors, uint8_t *buffer, size_t buffer_size) {
        switch(supportedDrives[drive_number - 1].driveType) {
                case DriveType::AHCI: {
                        //memset(supportedDrives[drive_number - 1].driver.ahciDriver.port->buffer, 'A', supportedDrives[drive_number - 1].driver.ahciDriver.buffer_size);
                        memcpy(supportedDrives[drive_number - 1].driver.ahciDriver.port->buffer, buffer, buffer_size);
                        return supportedDrives[drive_number - 1].driver.ahciDriver.port->Write(start_sector, number_sectors, supportedDrives[drive_number - 1].driver.ahciDriver.port->buffer);
                }
        }
}

void FSManager::ListDrives() {
        if (total_drives > 0) {
                printk("%d drives installed.\n", total_drives);
                for (int i = 0; i < total_drives; i++) {
                        switch (supportedDrives[i].driveType) {
                                case DriveType::AHCI:
                                        printk("/dev/sd%c %s\n", (supportedDrives[i].driver.ahciDriver.port_number + 97), FilesystemStrings[supportedDrives[i].partitions[0].filesystem]);
                                        break;
                                default:
                                        printk("/dev/unknown\n");
                        }
                }
        } else {
                printk("No drives installed.\n");
        }
}
}
