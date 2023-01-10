#include <fs/fat/fat.h>
#include <fs/fs.h>
#include <sys/printk.h>
#include <mm/memory.h>
#include <mm/heap.h>

namespace FAT {
uint32_t total_sectors;
uint32_t fat_size;
uint32_t root_dir_sectors;
uint32_t first_data_sector;
uint32_t first_fat_sector;
uint32_t data_sectors;
uint32_t total_clusters;
uint32_t first_sector_of_cluster;


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
                        printk("We have a FAT 16 drive on our hands.\n");
                        printk("Volume label: ");
                        for (int i = 0; i < 11; i++) {
                                printk("%c", fat16->volume_label[i]);
                        }
                        printk("\n");
                        }
                        break;
                case FATType::FAT32: {
                        printk("We have a FAT 32 drive on our hands.\n");
                        printk("Volume label: ");
                        for (int i = 0; i < 11; i++) {
                                printk("%c", fat32->volume_label[i]);
                        }
                        printk("\n");
                        uint32_t root_cluster_32 = fat32->root_cluster;
                        printk("Root dir is at %d\n", root_cluster_32);
                        uint32_t cluster = root_cluster_32;
                        uint32_t first_sector_of_cluster = ((cluster - 2) * bootsect->sectors_per_cluster) + first_data_sector;
                        //uint8_t *sector_data = (uint8_t*)malloc(32*1024);
                        uint8_t *read_data = (uint8_t*)malloc(32*1024);
                        memset(read_data, 0, 32*1024);
                        read_data = GlobalFSManager.ReadDrive(1, first_sector_of_cluster, 16);
                
                        uint8_t *sector_data = (uint8_t*)malloc(32);

                        printk("Parsing the root directory:\n");

                        for (int i = 0 /*This kinda works?*/; ; i+=32) {
                                memcpy(sector_data, read_data + i, 32);
                                if(sector_data[0] == 0) {
                                        printk("Done!\n");
                                        break;
                                } else if(sector_data[0] == 0xE5) {
                                        printk("Unused\n");
                                        continue;
                                } else {
                                        if(sector_data[11] == 0x0F) {
                                                printk("Long name.\n");
                                                continue;
                                        } else {
                                                DirectoryEntry *entry = (DirectoryEntry*)sector_data;
                                                printk("Short name: ");
                                                for (int i = 0; i < 11; i++) {
                                                        printk("%c", entry->file_name[i]);
                                                }
                                                printk(" Size: %d\n", entry->file_size);
                                                continue;
                                        }
                                }
                        }
                        printk("\n");

                        free(read_data);
                        free(sector_data);
                        }
                        break;
                default:
                        printk("Not handled\n");
                        return false;
        }

        return true;
}
void FATFSDriver::ReadCluster(uint8_t cluster) {

}

}
