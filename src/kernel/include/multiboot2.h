#pragma once
#include <multiboot.h>

#define MBOOT2_REPLY 0x36D76289

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

struct efi_bs {
        uint32_t type;
        uint32_t size;
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

#define MBOOT2_COMMANDLINE      1
#define MBOOT2_BOOTLOADER       2
#define MBOOT2_MODULE           3
#define MBOOT2_BASIC_MEM        4
#define MBOOT2_BOOTDEV          5
#define MBOOT2_MMAP             6
#define MBOOT2_VBE              7
#define MBOOT2_FB               8
#define MBOOT2_ELF_SEC          9
#define MBOOT2_APM              10
#define MBOOT2_EFI32            11
#define MBOOT2_EFI64            12
#define MBOOT2_SMBIOS           13
#define MBOOT2_ACPI_OLD         14
#define MBOOT2_ACPI             15
#define MBOOT2_NETWORK          16
#define MBOOT2_EFI_MMAP         17
#define MBOOT2_EFI_BS           18
#define MBOOT2_EFI_32_IH        19
#define MBOOT2_EFI_64_IH        20
#define MBOOT2_EFI_BADDR        21

int parse_multiboot2(struct taglist *tags);
int multiboot2_get_memory_area(size_t count, uintptr_t *start, uintptr_t *end, uint32_t *type);
int multiboot2_page_used(uintptr_t start);
