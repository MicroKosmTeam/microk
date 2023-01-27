#include <fs/fs.h>
#include <stdio.h>
#include <sys/panik.h>
#include <string.h>
#include <mm/heap.h>
#include <mm/pageframe.h>
#include <kutil.h>

#define PREFIX "[FS] "

static const char *FilesystemStrings[] = {"FAT", "Unknown"};

Filesystem::FSManager *GlobalFSManager;
namespace Filesystem {

void FSManager::AddAHCIDrive(AHCI::Port *port, int number, uint32_t buffer_size) {
        supportedDrives[total_drives].driveType = DriveType::AHCI;
        supportedDrives[total_drives].driver.ahciDriver.port = port;
        supportedDrives[total_drives].driver.ahciDriver.port_number = number;
        supportedDrives[total_drives].driver.ahciDriver.port->Configure();
        supportedDrives[total_drives].driver.ahciDriver.buffer_size = buffer_size;
        supportedDrives[total_drives].driver.ahciDriver.port->buffer = (uint8_t*)GlobalAllocator.RequestPages(buffer_size/0x1000);
        if(supportedDrives[total_drives].driver.ahciDriver.port->buffer == NULL) {
                PANIK("Failed to create AHCI buffer.");
        }

        supportedDrives[total_drives].driver.ahciDriver.port->Read(total_drives, 1, supportedDrives[total_drives].driver.ahciDriver.port->buffer);

        uint8_t *fs_buffer = (uint8_t*)malloc(0x200);

        memcpy(fs_buffer, supportedDrives[total_drives].driver.ahciDriver.port->buffer, 0x200);

        bool empty = true;
        for (int i = 0; i < 512; i++) {
                if(fs_buffer[i] != 0) empty = false;
        }
        
        total_drives++;
        if (!empty) {
                // We should first initialize partitions, but that's for the future
                fprintf(VFS_FILE_STDLOG, PREFIX "Initializing FAT Driver:\n");
       
                if(supportedDrives[total_drives-1].partitions[0].fatDriver.DetectDrive(fs_buffer)) {
                        supportedDrives[total_drives-1].partitions[0].fatDriver.drive = 0;
                        supportedDrives[total_drives-1].partitions[0].filesystem = Filesystem::FAT;
                        supportedDrives[total_drives-1].partitions[0].fatDriver.ReadDirectory(supportedDrives[total_drives-1].partitions[0].fatDriver.FindDirectory("/MICROK"));
                        uint8_t *file = supportedDrives[total_drives-1].partitions[0].fatDriver.LoadFile("/MICROK", "INITRD");

                        kInfo.initrd = file;
                        kInfo.initrd_loaded = true;
                }
        } else {
                fprintf(VFS_FILE_STDLOG, PREFIX "Empty drive\n");
                supportedDrives[total_drives-1].partitions[0].filesystem = Filesystem::UNKNOWN;
        }

        free(fs_buffer);
}

FSManager::FSManager() {
        fprintf(VFS_FILE_STDLOG, PREFIX "Initializing the file system manager.\n");
        total_drives = 0;
}

bool FSManager::ReadDrive(uint8_t drive_number, uint32_t start_sector, uint32_t number_sectors, uint8_t **buffer, size_t buffer_size, size_t sector_size) {
        switch(supportedDrives[drive_number].driveType) {
                case DriveType::AHCI: {
                        uint16_t sector_mul = supportedDrives[drive_number].driver.ahciDriver.buffer_size / sector_size;
                        for (uint64_t base = 0; base < buffer_size; base += sector_size * sector_mul) {
                                supportedDrives[drive_number].driver.ahciDriver.port->Read(start_sector + base / sector_size, sector_mul, supportedDrives[drive_number].driver.ahciDriver.port->buffer);
                                memcpy(*buffer + base,
                                       supportedDrives[drive_number].driver.ahciDriver.port->buffer,
                                       sector_size * sector_mul > buffer_size - base ? buffer_size - base : sector_size * sector_mul);
                        }
/*
                        for (uint64_t base = 0; base < buffer_size; base += supportedDrives[drive_number].driver.ahciDriver.buffer_size) {
                                supportedDrives[drive_number].driver.ahciDriver.port->Read(start_sector + base * sector_size, number_sectors, supportedDrives[drive_number].driver.ahciDriver.port->buffer);
                                memcpy(*buffer + base,
                                       supportedDrives[drive_number].driver.ahciDriver.port->buffer,
                                       supportedDrives[drive_number].driver.ahciDriver.buffer_size > buffer_size ? buffer_size : supportedDrives[drive_number].driver.ahciDriver.buffer_size);
                        }
                        
                        supportedDrives[drive_number].driver.ahciDriver.port->Read(start_sector, number_sectors, supportedDrives[drive_number].driver.ahciDriver.port->buffer);
                        memcpy(*buffer,
                               supportedDrives[drive_number].driver.ahciDriver.port->buffer,
                               buffer_size);*/
                        return true;
                }
        }
}

bool FSManager::WriteDrive(uint8_t drive_number, uint32_t start_sector, uint8_t number_sectors, uint8_t *buffer, size_t buffer_size) {
        switch(supportedDrives[drive_number].driveType) {
                case DriveType::AHCI: {
                        memcpy(supportedDrives[drive_number].driver.ahciDriver.port->buffer,
                               buffer,
                               supportedDrives[drive_number].driver.ahciDriver.buffer_size > buffer_size ? buffer_size : supportedDrives[drive_number].driver.ahciDriver.buffer_size);
                        return supportedDrives[drive_number].driver.ahciDriver.port->Write(start_sector, number_sectors, supportedDrives[drive_number].driver.ahciDriver.port->buffer);
                }
        }
}

void FSManager::ListDrives() {
        if (total_drives > 0) {
                printf("%d drives installed.\n", total_drives);
                for (int i = 0; i < total_drives; i++) {
                        switch (supportedDrives[i].driveType) {
                                case DriveType::AHCI:
                                        printf(" /dev/sd%c %s\n", (supportedDrives[i].driver.ahciDriver.port_number + 97), FilesystemStrings[supportedDrives[i].partitions[0].filesystem]);
                                        break;
                                default:
                                        printf(" /dev/unknown\n");
                        }
                }
        } else {
                printf(PREFIX "No drives installed.\n");
        }
}
}
