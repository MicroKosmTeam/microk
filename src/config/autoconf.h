/*
 * Automatically generated C config: don't edit
 */
#define AUTOCONF_INCLUDED

/*
 * Kernel & Hardware
 */

/*
 * Just accept the defaults unless you know what you are doing
 */

/*
 * System Architecture
 */
#define CONFIG_ARCH_x86_64 1
#undef  CONFIG_ARCH_AARCH64

/*
 * Processor
 */
#define CONFIG_HW_SSE 1
#undef  CONFIG_HW_COMP

/*
 * Firmware
 */
#define CONFIG_FM_UEFI 1
#undef  CONFIG_FM_BIOS
#define CONFIG_HW_GOP 1
#define CONFIG_FM_ACPI 1
#undef  CONFIG_FM_SMBIOS

/*
 * Kernel settings
 */
#define CONFIG_KERNEL_CNAME "MicroK"
#define CONFIG_KERNEL_CVER "0.0.0"

/*
 * Networking
 */
#define CONFIG_NET 1

/*
 * Drivers
 */
#define CONFIG_UDI 1
#define CONFIG_UDI_VERSION 0x101
#define CONFIG_VFS 1
#define CONFIG_VFS_MAX_DRIVES (128)
#define CONFIG_VFS_FILE_MAX_NAME_LENGTH (128)
#define CONFIG_AHCI 1
#define CONFIG_AHCI_DMA_SIZE (4194304)

/*
 * Drivers
 */
#define CONFIG_HW_SERIAL 1
