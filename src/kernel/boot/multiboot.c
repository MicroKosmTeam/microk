#include <multiboot.h>
#include <mm/memory.h>
#include <stdio.h>

#define MBOOT_REPLY 0x36D76289

struct taglist{
        uint32_t total_size;
        uint32_t reserved;
}__attribute__((packed));

struct tag {
        uint32_t type;
        uint32_t size;
        uint8_t data[];
}__attribute__((packed));

struct mb_color {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
};

struct module {
        uint32_t type;
        uint32_t size;
        uint32_t mod_start;
        uint32_t mod_end;
        char cmdline[0];
}__attribute__((packed));

struct mmap_entry{
        uint64_t base;
        uint64_t len;
        uint32_t type;
        uint32_t reserved;
}__attribute__((packed));

struct mmap {
        uint32_t entry_size;
        uint32_t entry_version;
        struct mmap_entry entries[];
}__attribute__((packed));

struct vbe_info_block {
        uint8_t external_specification[512];
}__attribute__((packed));

struct vbe_mode_info_block {
        uint8_t external_specification[256];
}__attribute__((packed));

struct vbe {
        uint32_t type;
        uint32_t syze;

        uint16_t vbe_mode;
        uint16_t vbe_interface_seg;
        uint16_t vbe_interface_off;
        uint16_t vbe_interface_len;

        struct vbe_info_block vbe_control_info;
        struct vbe_mode_info_block vbe_mode_info;

}__attribute__((packed));

struct fb_common {
        uint32_t type;
        uint32_t size;

        uint64_t fb_addr;
        uint32_t fb_pitch;
        uint32_t fb_width;
        uint32_t fb_height;
        uint8_t fb_bpp;
#define MB2_FB_TYPE_INDEXED 0
#define MB2_FB_TYPE_RGB 1
#define MB2_FB_TYPE_EGA_TEXT 2
        uint8_t fb_type;
        uint16_t reserved;
}__attribute__((packed));

struct framebuffer {
        struct fb_common common;
        union {
                struct {
                        uint16_t fb_palette_num_colors;                                       
                        struct mb_color fb_palette[0];
                };

                struct {
                        uint8_t fb_red_field_position;
                        uint8_t fb_red_mask_size;
                        uint8_t fb_green_field_position;
                        uint8_t fb_green_mask_size;
                        uint8_t fb_blue_field_position;
                        uint8_t fb_blue_mask_size;
                };
        };
}__attribute__((packed));

struct efi64 {
        uint32_t type;
        uint32_t size;
        uint64_t pointer;
}__attribute__((packed));

struct smbios {
        uint32_t type;
        uint32_t size;
        uint8_t major;
        uint8_t minor;
        uint8_t reserved[6];
        uint8_t tables[0];
}__attribute__((packed));

struct acpi {
        uint32_t type;
        uint32_t size;
        uint8_t rsdp[0];
}__attribute__((packed));

struct network {
        uint32_t type;
        uint32_t size;
        uint8_t dhcpack[0];
}__attribute__((packed));

struct efi_mmap {
        uint32_t type;
        uint32_t size;
        uint32_t descr_size;
        uint32_t descr_vers;
        uint8_t efi_mmap[0];
}__attribute__((packed));

struct efi64_ih {
        uint32_t type;
        uint32_t size;
        uint64_t pointer;
}__attribute__((packed));

#define MBOOT2_COMMANDLINE 1
#define MBOOT2_BOOTLOADER 2
#define MBOOT2_MODULE 3
#define MBOOT2_MMAP 6
#define MBOOT2_VBE 7
#define MBOOT2_FB 8
#define MBOOT2_EFI64 12
#define MBOOT2_SMBIOS 13
#define MBOOT2_ACPI 15
#define MBOOT2_NETWORK 16
#define MBOOT2_EFI_MMAP 17
#define MBOOT2_EFI_BS 18
#define MBOOT2_EFI_64_IH 20

struct kernel_boot_data_st kernel_boot_data;

int parse_multiboot2(struct taglist *tags) {
        struct tag *tag = incptr(tags, sizeof(struct taglist));
        struct mmap *mmap;
        struct module *mod;
        struct framebuffer *fb;

        while(tag->type) {
                switch(tag->type) {
                        case MBOOT2_BOOTLOADER:
                                kernel_boot_data.bootloader = (char *)tag->data;
                                break;
                        case MBOOT2_COMMANDLINE:
                                kernel_boot_data.commandline = (char *)tag->data;
                                break;
                        case MBOOT2_MODULE:
                                mod = (void *)tag->data;
                                printk("Module found: 0x%x-0x%x -> %s\n", mod->mod_start, mod->mod_end, mod->cmdline);
                                break;
                        case MBOOT2_MMAP:
                                if (!kernel_boot_data.is_efi) {
                                        mmap = kernel_boot_data.mmap = (void *)tag->data;
                                        kernel_boot_data.mmap_len = (tag->size - 8)/mmap->entry_size;
                                        kernel_boot_data.mmap_size = (tag->size - 8);
                                }
                                break;
                        case MBOOT2_VBE:
                                break;
                        case MBOOT2_FB:
                                uint32_t color;
                                fb = (void *)tag->data;
                                void *fb_addr = (void *) (unsigned long) fb->common.fb_addr;
                                printk("Parsing framebuffer data.\n");
                                
                                switch (fb->common.fb_type) {
                                        case MB2_FB_TYPE_INDEXED:
                                                unsigned best_distance, distance;
                                                struct mb_color *palette;

                                                palette = fb->fb_palette;

                                                color = 0;
                                                best_distance = 4*256*256;

                                                for (unsigned i = 0; i < fb->fb_palette_num_colors; i++) {
                                                        distance = (0xff - palette[i].blue) 
                                                                * (0xff - palette[i].blue)
                                                                + palette[i].red * palette[i].red
                                                                + palette[i].green * palette[i].green;
                                                        
                                                        if (distance < best_distance) {
                                                                color = i;
                                                                best_distance = distance;
                                                        }
                                                }
                                                break;
                                        case MB2_FB_TYPE_RGB:
                                                color = ((1 << fb->fb_blue_mask_size) - 1) 
                                                        << fb->fb_blue_field_position;
                                                break;
                                        case MB2_FB_TYPE_EGA_TEXT:
                                                color = '\\' | 0x0100;
                                                break;
                                        default:
                                                color = 0xffffffff;
                                }

                                for (unsigned i = 0; i < fb->common.fb_width 
                                     && i < fb->common.fb_height; i++) {
                                        switch (fb->common.fb_bpp) {
                                                case 8: {
                                                        uint8_t *pixel = fb_addr
                                                                        + fb->common.fb_pitch * i + i;
                                                        *pixel = color;
                                                        } break;
                                                case 15:
                                                case 16: {
                                                        uint16_t *pixel
                                                                        = fb_addr + fb->common.fb_pitch * i + 2 * i;
                                                        *pixel = color;
                                                         } break;
                                                case 24: {
                                                        uint32_t *pixel
                                                                        = fb_addr + fb->common.fb_pitch * i + 3 * i;
                                                        *pixel = (color & 0xffffff) | (*pixel & 0xff000000);
                                                        } break;
                                                case 32: {
                                                        uint32_t *pixel
                                                                        = fb_addr + fb->common.fb_pitch * i + 4 * i;
                                                                *pixel = color;
                                                        } break;
                                        }
                                }
                                break;
                        case MBOOT2_EFI64:
                                kernel_boot_data.is_efi = true;
                                break;
                        case MBOOT2_SMBIOS:
                                break;
                        case MBOOT2_ACPI:
                                break;
                        case MBOOT2_NETWORK:
                                break;
                        case MBOOT2_EFI_64_IH:
                                break;
                }
                
                int padded_size = tag->size + ((tag->size % 8)?(8-(tag->size%8)):0);
                tag = incptr(tag, padded_size);
        }

        return 0;
}

int multiboot_init(uint64_t magic, void *mboot_info) {
        if(magic == MBOOT_REPLY) {
                kernel_boot_data.multiboot_version = 2;
                parse_multiboot2(mboot_info);
        } else
                return 1;

        return 0;
}

int multiboot_get_memory_area(size_t count, uintptr_t *start, uintptr_t *end, uint32_t *type) {
        if(count >= kernel_boot_data.mmap_len) return 1;

        struct mmap *mmap = kernel_boot_data.mmap;
        struct mmap_entry *entry = mmap->entries;
        entry = incptr(entry, count*mmap->entry_size);

        *start = entry->base;
        *end = entry->base + entry->len;
        *type = entry->type;
        return 0;
}

int multiboot_page_used(uintptr_t start) {
        #define overlap(st, len) ((uintptr_t)st < (start + PAGE_SIZE) && start <= ((uintptr_t)st + len))

        if(overlap(kernel_boot_data.bootloader, strlen(kernel_boot_data.bootloader)) ||
           overlap(kernel_boot_data.commandline, strlen(kernel_boot_data.commandline)) ||
           overlap(kernel_boot_data.mmap, kernel_boot_data.mmap_size) || 0)
                return 1;

        return 0;
}
