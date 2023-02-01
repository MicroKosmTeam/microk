#include <fs/fat/fat.h>
#include <fs/fs.h>
#include <stdio.h>
#include <mm/memory.h>
#include <mm/heap.h>
#include <mm/string.h>

#define PREFIX "[FAT] "

namespace FAT {
uint32_t total_sectors;
uint32_t fat_size;
uint32_t root_dir_sectors;
uint32_t first_data_sector;
uint32_t first_fat_sector;
uint32_t data_sectors;
uint32_t total_clusters;
uint32_t first_sector_of_cluster;
uint32_t the_root_cluster;


FATFSDriver::FATFSDriver() {
}
        
bool FATFSDriver::DetectDrive(uint8_t *data) {
        bootsect = (FatBootsect*)data;
        Fat16Bootsect *fat16 = (Fat16Bootsect*)(bootsect->extended_section);
        Fat32Bootsect *fat32 = (Fat32Bootsect*)(bootsect->extended_section);

        total_sectors = (bootsect->total_sectors_16 == 0) ? 
                 bootsect->total_sectors_32 : bootsect->total_sectors_16;

        fat_size = (bootsect->table_size_16 == 0) ?
                 fat32->table_size_32 : bootsect->table_size_16;

        root_dir_sectors = ((bootsect->root_entry_count * 32) + (bootsect->bytes_per_sector - 1)) / bootsect->bytes_per_sector;

        first_data_sector = bootsect->reserved_sector_count + (bootsect->table_count * fat_size) + root_dir_sectors;

        first_fat_sector = bootsect->reserved_sector_count;

        data_sectors = total_sectors - 
                 (bootsect->reserved_sector_count + (bootsect->table_count * fat_size) + root_dir_sectors);


        total_clusters = data_sectors / bootsect->sectors_per_cluster;

        if(total_clusters < 4085) {
        	fatType = FATType::FAT12;
        } else if(total_clusters < 65525) {
        	fatType = FATType::FAT16;
        } else {
        	fatType = FATType::FAT32;
        }
        
        switch (fatType) {
                case FATType::FAT16: {
                        dprintf(PREFIX "We have a FAT 16 drive on our hands.\n");
                        dprintf(PREFIX "Volume label: ");
                        for (int i = 0; i < 11; i++) {
                                dprintf("%c", fat16->volume_label[i]);
                        }
                        dprintf("\n");
                        }
                        break;
                case FATType::FAT32: {
                        dprintf(PREFIX "We have a FAT 32 drive on our hands.\n");
                        dprintf(PREFIX "Volume label: ");
                        for (int i = 0; i < 11; i++) {
                                dprintf("%c", fat32->volume_label[i]);
                        }
                        dprintf("\n");
                        uint32_t root_cluster_32 = fat32->root_cluster;
                        dprintf(PREFIX "Root dir is in the %d cluster.\n", root_cluster_32);

                        the_root_cluster = root_cluster_32;
                        }
                        break;
                default:
                        dprintf(PREFIX "Not handled\n");
                        return false;
        }

        return true;
}

void FATFSDriver::ParseRoot(uint32_t root_cluster) {
        ReadDirectory(the_root_cluster);
}

uint8_t *FATFSDriver::LoadFile(char *directory_path, char *file_name) {
        uint32_t directory_cluster = FindDirectory(directory_path);
        if (directory_path == NULL) return NULL;

        uint32_t file_cluster = FindInDirectory(directory_cluster, file_name);
        uint32_t file_size = FindSizeInDirectory(directory_cluster, file_name);

        dprintf(PREFIX "File:\n - Cluster: %d\n - Size: %d\n", file_cluster, file_size);

        uint32_t first_sector_of_cluster = ((file_cluster - 2) * bootsect->sectors_per_cluster) + first_data_sector;
        
        uint8_t *read_data = (uint8_t*)malloc(file_size);
        GlobalFSManager->ReadDrive(drive, first_sector_of_cluster, file_size / bootsect->bytes_per_sector + 1, &read_data, file_size, bootsect->bytes_per_sector);

        return read_data;
}

uint32_t FATFSDriver::FindDirectory(char *directory_path) {
        if (directory_path[0] == '/' && strlen(directory_path) == 1) {
                ParseRoot(the_root_cluster);
                return NULL;
        }

        char *dir = directory_path;
        uint32_t cluster = the_root_cluster;

        while((dir = strtok(dir, "/")) != NULL) {
                cluster = FindInDirectory(cluster, dir);
                if (cluster == 0) { dprintf(PREFIX "Not found: %s\n", directory_path); return false; }

                dir = NULL;
        }
        
        return cluster;
}

uint32_t FATFSDriver::FindSizeInDirectory(uint32_t directory_cluster, char *find_name) {
        uint32_t first_sector_of_cluster = ((directory_cluster - 2) * bootsect->sectors_per_cluster) + first_data_sector;
        
        uint8_t *read_data = (uint8_t*)malloc(bootsect->sectors_per_cluster * bootsect->bytes_per_sector);
        GlobalFSManager->ReadDrive(drive, first_sector_of_cluster, bootsect->sectors_per_cluster, &read_data, bootsect->sectors_per_cluster * bootsect->bytes_per_sector, bootsect->bytes_per_sector);
        uint8_t *sector_data = (uint8_t*)malloc(32);

        uint32_t found_size = 0;

        for (int i = 0; ; i+=32) {
                memcpy(sector_data, read_data + i, 32);
                if(sector_data[0] == 0) {
                        break;
                } else if(sector_data[0] == 0xE5) {
                        continue;
                } else {
                        uint32_t entry_cluster = 0;
                        if(sector_data[11] == 0x0F) {
                                continue;
                        } else if (sector_data[11] != 0x10){
                                DirectoryEntry *entry = (DirectoryEntry*)sector_data;
                                uint32_t entry_size = entry->file_size;

                                // This works
                                bool incorrect = false;
                                for (int i = 0; i < 8/*TMP should be 11 but we delete extension*/; i++) {
                                        if(entry->file_name[i] == ' ') continue; // TEMP fix TODO correctly
                                        if(find_name[i] != entry->file_name[i]) { incorrect = true; break; }
                                }
                                if (!incorrect) {found_size = entry_size; break;}
                                else continue;
                        }
                }
                
        }

        free(read_data);
        free(sector_data);

        return found_size;
}

uint32_t FATFSDriver::FindInDirectory(uint32_t directory_cluster, char *find_name) {
        uint32_t first_sector_of_cluster = ((directory_cluster - 2) * bootsect->sectors_per_cluster) + first_data_sector;
       
        uint8_t *read_data = (uint8_t*)malloc(bootsect->sectors_per_cluster * bootsect->bytes_per_sector);
        GlobalFSManager->ReadDrive(drive, first_sector_of_cluster, bootsect->sectors_per_cluster, &read_data, bootsect->sectors_per_cluster * bootsect->bytes_per_sector, bootsect->bytes_per_sector);
        uint8_t *sector_data = (uint8_t*)malloc(32);

        uint32_t found_cluster = 0;

        for (int i = 0; ; i+=32) {
                memcpy(sector_data, read_data + i, 32);
                if(sector_data[0] == 0) {
                        break;
                } else if(sector_data[0] == 0xE5) {
                        continue;
                } else {
                        uint32_t entry_cluster = 0;
                        if(sector_data[11] == 0x0F) {
                                continue;
                        } else {
                                DirectoryEntry *entry = (DirectoryEntry*)sector_data;
                                entry_cluster = entry->low_bits | (entry->high_bits << 16);

                                if (entry_cluster == 0) continue;

                                // This works
                                bool incorrect = false;
                                for (int i = 0; i < 8/*TMP should be 11 but we delete extension*/; i++) {
                                        if(entry->file_name[i] == ' ') continue; // TEMP fix TODO correctly
                                        if(find_name[i] != entry->file_name[i]) { incorrect = true; break; }
                                }
                                if (!incorrect) {found_cluster = entry_cluster; break;}
                                else continue;
                        }
                }
                
        }

        free(read_data);
        free(sector_data);

        return found_cluster;
}

void FATFSDriver::ReadDirectory(uint32_t directory_cluster) {
        uint32_t first_sector_of_cluster = ((directory_cluster - 2) * bootsect->sectors_per_cluster) + first_data_sector;
        
        dprintf(PREFIX "Reading directory in cluster %d.\n", directory_cluster);
        uint8_t *read_data = (uint8_t*)malloc(bootsect->sectors_per_cluster * bootsect->bytes_per_sector);
        GlobalFSManager->ReadDrive(drive, first_sector_of_cluster, bootsect->sectors_per_cluster, &read_data, bootsect->sectors_per_cluster * bootsect->bytes_per_sector, bootsect->bytes_per_sector);
        uint8_t *sector_data = (uint8_t*)malloc(32);

        dprintf(PREFIX "Parsing the directory:\n");

        for (int i = 0;;i+=32) {
                memcpy(sector_data, read_data + i, 32);
                if(sector_data[0] == 0) {
                        dprintf(PREFIX "Done!\n");
                        break;
                } else if(sector_data[0] == 0xE5) {
                        dprintf(PREFIX "Unused\n");
                        continue;
                } else {
                        uint32_t entry_cluster = 0;
                        if(sector_data[11] == 0x0F) {
//                                LongEntry *entry;
                                dprintf(PREFIX "Long name.\n");
                                continue;
                        } else {
                                DirectoryEntry *entry = (DirectoryEntry*)sector_data;
                                entry_cluster = entry->low_bits | (entry->high_bits << 16);
                                //if (entry_cluster == 0) continue;
                                dprintf(PREFIX "Short name: ");
                                for (int i = 0; i < 8; i++) {
                                        dprintf("%c", entry->file_name[i]);
                                }
                                dprintf(" ");
                                for (int i = 8; i < 11; i++) {
                                        dprintf("%c", entry->file_name[i]);
                                }
                                dprintf(" Cluster: %d", entry_cluster);
                                if (sector_data[11] == 0x10) {
                                        dprintf(" Directory\n");
                                } else {
                                        dprintf(" Size: %d\n", entry->file_size);
                                }

                                continue;
                        }
                }
                
        }

        free(read_data);
        free(sector_data);
}

}
