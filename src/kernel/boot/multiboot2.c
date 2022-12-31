#include <multiboot2.h>
#include <mm/memory.h>
#include <stdio.h>

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
                                mmap = kernel_boot_data.mmap = (void *)tag->data;
                                kernel_boot_data.mmap_len = (tag->size - 8)/mmap->entry_size;
                                kernel_boot_data.mmap_size = (tag->size - 8);
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
                                printk("We are in EFI.\n");
                                kernel_boot_data.is_efi = true;
                                break;
                        case MBOOT2_EFI_BS:
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

int multiboot2_get_memory_area(size_t count, uintptr_t *start, uintptr_t *end, uint32_t *type) {
        if(count >= kernel_boot_data.mmap_len) return 1;

        struct mmap *mmap = kernel_boot_data.mmap;
        struct mmap_entry *entry = mmap->entries;
        entry = incptr(entry, count*mmap->entry_size);

        *start = entry->base;
        *end = entry->base + entry->len;
        *type = entry->type;
        return 0;
}

int multiboot2_page_used(uintptr_t start) {
        #define overlap(st, len) ((uintptr_t)st < (start + PAGE_SIZE) && start <= ((uintptr_t)st + len))

        if(overlap(kernel_boot_data.bootloader, strlen(kernel_boot_data.bootloader)) ||
           overlap(kernel_boot_data.commandline, strlen(kernel_boot_data.commandline)) ||
           overlap(kernel_boot_data.mmap, kernel_boot_data.mmap_size) || 0)
                return 1;

        return 0;
}
