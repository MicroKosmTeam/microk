/*
 * File: linux/osdep_pio.c
 *
 * Linux programmed I/O handler.
 */

/*
 * $Copyright udi_reference:
 * 
 * 
 *    Copyright (c) 1995-2001; Compaq Computer Corporation; Hewlett-Packard
 *    Company; Interphase Corporation; The Santa Cruz Operation, Inc;
 *    Software Technologies Group, Inc; and Sun Microsystems, Inc
 *    (collectively, the "Copyright Holders").  All rights reserved.
 * 
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the conditions are met:
 * 
 *            Redistributions of source code must retain the above
 *            copyright notice, this list of conditions and the following
 *            disclaimer.
 * 
 *            Redistributions in binary form must reproduce the above
 *            copyright notice, this list of conditions and the following
 *            disclaimers in the documentation and/or other materials
 *            provided with the distribution.
 * 
 *            Neither the name of Project UDI nor the names of its
 *            contributors may be used to endorse or promote products
 *            derived from this software without specific prior written
 *            permission.
 * 
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *    "AS IS," AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *    HOLDERS OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *    TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 *    DAMAGE.
 * 
 *    THIS SOFTWARE IS BASED ON SOURCE CODE PROVIDED AS A SAMPLE REFERENCE
 *    IMPLEMENTATION FOR VERSION 1.01 OF THE UDI CORE SPECIFICATION AND/OR
 *    RELATED UDI SPECIFICATIONS. USE OF THIS SOFTWARE DOES NOT IN AND OF
 *    ITSELF CONSTITUTE CONFORMANCE WITH THIS OR ANY OTHER VERSION OF ANY
 *    UDI SPECIFICATION.
 * 
 * 
 * $
 */

#include "udi_osdep_opaquedefs.h"
#include <udi_env.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/ioport.h>

/*
 * The Linux PPC read/write functions do automatic endian conversion
 * for writting to PCI memory. An optimization would be to use
 * [sl]wbrx/[sl]hwbrx for swapping instead of doing bit-shifting with
 * _OSDEP_ENDIAN_SWAP_*. Or even better, remove the conditional
 * switches on _OSDEP_HOST_IS_BIG_ENDIAN, and use read/write instead
 * which come with [sl]wbrx/[sl]hbrx for free.
 */

#if DEBUG_LINUX_PIO
#define _PIO_DEBUG_PRINT(x) printk x
#else
#define _PIO_DEBUG_PRINT(x) do {} while(0)
#endif

#if !_OSDEP_HOST_IS_BIG_ENDIAN
#define udi_writeb(data, addr) \
	_PIO_DEBUG_PRINT(("writeb %p = %02X\n", (addr), (udi_ubit8_t)(data)));\
	writeb((data), (addr));
#define udi_writew(data, addr) \
	_PIO_DEBUG_PRINT(("writew %p = %04hX\n", (addr), (udi_ubit16_t)(data)));\
	writew((data), (addr));
#define udi_writel(data, addr) \
	_PIO_DEBUG_PRINT(("writel %p = %08X\n", (addr), (udi_ubit32_t)(data)));\
	writel((data), (addr));
#else
#define udi_writeb(data, addr) \
	_PIO_DEBUG_PRINT(("writeb %p = %02X\n", (addr), (udi_ubit8_t)(data)));\
	*(udi_ubit8_t*)(addr) = (data); \
	mb();
#define udi_writew(data, addr) \
	_PIO_DEBUG_PRINT(("writew %p = %04hX\n", (addr), (udi_ubit16_t)(data)));\
	*(udi_ubit16_t*)(addr) = (data); \
	mb();
#define udi_writel(data, addr) \
	_PIO_DEBUG_PRINT(("writel %p = %08X\n", (addr), (udi_ubit32_t)(data)));\
	*(udi_ubit32_t*)(addr) = (data); \
	mb();
#endif

static udi_ubit8_t udi_readb(void *addr)
{
	udi_ubit8_t result;
	_PIO_DEBUG_PRINT(("readb %p = \n", addr));
#if !_OSDEP_HOST_IS_BIG_ENDIAN
	result = readb(addr);
#else
	result = *(udi_ubit8_t*)(addr);
	mb();
#endif
	_PIO_DEBUG_PRINT(("%02X\n", (udi_ubit8_t)result));
	return result;
}

static udi_ubit16_t udi_readw(void *addr)
{
	udi_ubit16_t result;
	_PIO_DEBUG_PRINT(("readw %p = \n", addr));
#if !_OSDEP_HOST_IS_BIG_ENDIAN
	result = readw(addr);
#else
	result = *(udi_ubit16_t*)(addr);
	mb();
#endif
	_PIO_DEBUG_PRINT(("%04hX\n", (udi_ubit16_t)result));
	return result;
}

static udi_ubit32_t udi_readl(void *addr)
{
	udi_ubit32_t result;
	_PIO_DEBUG_PRINT(("readl %p = \n", addr));
#if !_OSDEP_HOST_IS_BIG_ENDIAN
	result = readl(addr);
#else
	result = *(udi_ubit32_t*)(addr);
	mb();
#endif
	_PIO_DEBUG_PRINT(("%08X\n", (udi_ubit32_t)result));
	return result;
}


#define ALWAYS_CHECK_ALIGNMENT 1

#define IS_ALIGNED(P, size) ( 0 == ((long)(P) & ((size) - 1) ) )

#if ALWAYS_CHECK_ALIGNMENT
#define VERIFY_ALIGNED(P, size) \
  if (!(IS_ALIGNED(P, size))) \
	_OSDEP_PRINTF("UDI PIO: Misaligned data access at %s:%d: %p not 0x%lx byte aligned\n", \
			__FILE__, __LINE__, (void*)(P), (unsigned long)(size));
#else
#define VERIFY_ALIGNED(P, size) \
  if ((!(IS_ALIGNED(P, size)) && ((pio->flags & _UDI_PIO_UNALIGNED) == 0)))  \
	_OSDEP_PRINTF("UDI PIO: Misaligned data access at %s:%ld: %p not 0x%lx byte aligned\n", \
			__FILE__, __LINE__, (void*)(P), (unsigned long)(size));
#endif

#define VERIFY_OFFSET(pio_handle, offset, length) \
	pio_verify_offset(pio_handle, offset, length)

void pio_verify_offset(_udi_pio_handle_t *pio, udi_ubit32_t offset, udi_size_t size)
{
	unsigned long last_byte = (offset + size) - 1;
	/* generate ranges at pio_map time! */
	_PIO_DEBUG_PRINT(("pioh(%p) effective address: %p+0x%lx to %p\n",
			pio,
			&pio->pio_osinfo.baseaddr[0], offset,
			&pio->pio_osinfo.baseaddr[last_byte]));
	if (last_byte > pio->pio_osinfo.mapped_max_offset) {
		_OSDEP_PRINTF("UDIPIO: access to %lx is not inside mapped range max %lx.\n",
			last_byte, pio->pio_osinfo.mapped_max_offset);
	}
	/* PIO access beyond bounds of the hardware! */
	if (!(last_byte < pio->pio_osinfo.hw_max_offset)) {
		_OSDEP_PRINTF("UDIPIO: access to %lx is not inside hardware range max %lx.\n",
			last_byte, pio->pio_osinfo.hw_max_offset);
		_OSDEP_ASSERT(last_byte < pio->pio_osinfo.hw_max_offset);
	}
}


/*
 *  FIXME: Delete then when UDI_PHYSIO_SUPPORTED is turned on.
 */
#ifndef UDI_PCI_CONFIG_SPACE
#  define UDI_PCI_CONFIG_SPACE 255
#endif

STATIC void
io_rd_many(udi_ubit32_t offset, void *data, udi_size_t size,
	   _udi_pio_handle_t * pio)
{
    int i;
    udi_ubit32_t *addr = data;

    _PIO_DEBUG_PRINT(("io_rd_many(offset=%x, data=%p, size=%lx, pio_hdl=%p)\n",
			offset, data, size, pio));
#if ALWAYS_CHECK_ALIGNMENT
    if (1) {
#else
    if (! (pio->flags & _UDI_PIO_UNALIGNED)) {
#endif
	VERIFY_ALIGNED(offset, size);
    }
    VERIFY_OFFSET(pio, offset, size);
    VERIFY_ALIGNED(data, size);

    for (i = 0; i < (size / 4); i++) {
        udi_ubit32_t d;
	d = inl((unsigned long)pio->pio_osinfo._u.io.addr + offset);
	*addr = d;
	offset += 4;
	addr++;
    }

    if (pio->pace) {
	udelay(pio->pace);
    }

    if (pio->flags & _UDI_PIO_DO_SWAP) {
	/* TODO: this could be wrong */
	udi_endian_swap(data, data, size, size, 1);
    }
}

STATIC void
io_wr_many(udi_ubit32_t offset, void *data, udi_size_t size,
	   _udi_pio_handle_t * pio)
{
    int i;
    udi_ubit32_t *addr = data;
    char swapped_buf[32];

    _PIO_DEBUG_PRINT(("io_wr_many(offset=%x, data=%p, size=%lx, pio_hdl=%p)\n",
			offset, data, size, pio));
#if ALWAYS_CHECK_ALIGNMENT
    if (1) {
#else
    if (! (pio->flags & _UDI_PIO_UNALIGNED)) {
#endif
	VERIFY_ALIGNED(offset, size);
    }
    VERIFY_OFFSET(pio, offset, size);
    VERIFY_ALIGNED(data, size);

    if (pio->flags & _UDI_PIO_DO_SWAP) {
	/* TODO: this could be wrong */
	udi_endian_swap(data, swapped_buf, size, size, 1);
	addr = (udi_ubit32_t *) swapped_buf;
    }

    for (i = 0; i < (size / 4); i++) {
	outl(*addr, (unsigned long)pio->pio_osinfo._u.io.addr + offset);
	offset += 4;
	addr++;
    }

    if (pio->pace) {
	udelay(pio->pace);
    }
}

/*
 * Handle memory reads larger than we can atomically access.
 */
STATIC void
mem_rd_many(udi_ubit32_t offset, void *data, udi_size_t size,
	    _udi_pio_handle_t * pio)
{
    _PIO_DEBUG_PRINT
	(("mem_rd_many(offset=%x, data=%p, size=%lx, pio_hdl=%p)\n",
	 offset, data, size, pio));
#if ALWAYS_CHECK_ALIGNMENT
    if (1) {
#else
    if (! (pio->flags & _UDI_PIO_UNALIGNED)) {
#endif
	VERIFY_ALIGNED(offset, size);
    }
    VERIFY_OFFSET(pio, offset, size);

    if (pio->flags & _UDI_PIO_DO_SWAP) {
	/* TODO: this could be wrong */
	udi_endian_swap(&pio->pio_osinfo._u.mem.mapped_addr[offset],
			data, size, size, 1);
    } else {
	memcpy_fromio(&pio->pio_osinfo._u.mem.mapped_addr[offset], data,
		      size);
    }

    if (pio->pace) {
	udelay(pio->pace);
    }
}

STATIC void
mem_wr_many(udi_ubit32_t offset, void *data, udi_size_t size,
	    _udi_pio_handle_t * pio)
{
    _PIO_DEBUG_PRINT
	(("mem_wr_many(offset=%x, data=%p, size=%x, pio_hdl=%p)\n",
	 offset, data, size, pio));
#if ALWAYS_CHECK_ALIGNMENT
    if (1) {
#else
    if (! (pio->flags & _UDI_PIO_UNALIGNED)) {
#endif
	VERIFY_ALIGNED(offset, size);
    }
    VERIFY_OFFSET(pio, offset, size);

    if (pio->flags & _UDI_PIO_DO_SWAP) {
	/* TODO: this could be wrong */
	udi_endian_swap(data,
			&pio->pio_osinfo._u.mem.mapped_addr[offset], size,
			size, 1);
    } else {
	memcpy_toio(data, &pio->pio_osinfo._u.mem.mapped_addr[offset],
		    size);
    }

    if (pio->pace) {
	udelay(pio->pace);
    }
}


STATIC void
pci_cfg_rd_many(udi_ubit32_t offset,
		void *value, udi_size_t size, _udi_pio_handle_t * pio)
{
    udi_size_t loop = size / 4;

#ifdef CONFIG_PCI
    _OSDEP_ASSERT(pci_present());
#else
    _OSDEP_ASSERT(0);
#endif

    _PIO_DEBUG_PRINT
	(("pci_cfg_rd_many(offset=%x, value=%p, size=%x, piohdl=%p, loop=%x)\n",
	 offset, value, size, pio, loop));
#if ALWAYS_CHECK_ALIGNMENT
    if (1) {
#else
    if (! (pio->flags & _UDI_PIO_UNALIGNED)) {
#endif
	VERIFY_ALIGNED(offset, size);
    }
    VERIFY_OFFSET(pio, offset, size);

    while (loop--) {
	pci_read_config_dword(pio->pio_osinfo.pio_pci_dev, offset,
			      (u32 *) value);
	_PIO_DEBUG_PRINT(("%p ", *(void**)value));
	((u32 *)value)++;
	offset += 4;
    }
    _PIO_DEBUG_PRINT(("\n"));
}

STATIC void
pci_cfg_wr_many(udi_ubit32_t offset,
		void *value, udi_size_t size, _udi_pio_handle_t * pio)
{
    udi_size_t loop = size / 4;

#ifdef CONFIG_PCI
    _OSDEP_ASSERT(pci_present());
#else
    _OSDEP_ASSERT(0);
#endif

    _PIO_DEBUG_PRINT
	(("pci_cfg_wr_many(offset=%x, value=%p, size=%x, piohdl=%p)\n",
	 offset, value, size, pio));
#if ALWAYS_CHECK_ALIGNMENT
    if (1) {
#else
    if (! (pio->flags & _UDI_PIO_UNALIGNED)) {
#endif
	VERIFY_ALIGNED(offset, size);
    }
    VERIFY_OFFSET(pio, offset, size);

    while (loop--) {
	pci_write_config_dword(pio->pio_osinfo.pio_pci_dev, offset,
			       *(u32 *) value);
	value += 4;
	offset += 4;
    }
}

#include <osdep_pops.h>

#define PCI_BAR_MEM_TYPE 0
#define PCI_BAR_IO_TYPE 1

static unsigned long pci_config_info(struct pci_dev *pci_dev, int bar_idx,
			unsigned long *physaddr_ptr, udi_ubit8_t *type_ptr);

/* Differences between 2.2 and 2.4: use the new accessor function. */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0))
#if 0
#define pci_resource_start(dev, bar)	((dev)->base_address[(bar)])
#else
unsigned long pci_resource_start(struct pci_dev *pci_dev, int whichbar)
{
	unsigned long physaddr;
	pci_config_info(pci_dev, whichbar, &physaddr, NULL);
	return physaddr;
}
#endif

#define pci_enable_device(dev)		do {} while (0)
#define request_mem_region(addr, size, devname) 1
#define release_mem_region(addr, size, devname) 1

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,2,18))
static unsigned long pci_resource_len(struct pci_dev *pci_dev, int bar_idx)
{
	return pci_config_info(pci_dev, bar_idx, NULL, NULL);
}
#endif
#endif


static udi_ubit8_t pci_resource_type(struct pci_dev *pci_dev, int bar_idx)
{
	udi_ubit8_t type;
	/* returns 1 if IO, returns 0 for memory */
	pci_config_info(pci_dev, bar_idx, NULL, &type);
	return type;
}

/* This function returns the length. */
static unsigned long pci_config_info(struct pci_dev *pci_dev, int bar_idx,
			unsigned long *physaddr_ptr, udi_ubit8_t *type_ptr)
{
    const u32 pci_addresses[] = {
	PCI_BASE_ADDRESS_0,
	PCI_BASE_ADDRESS_1,
	PCI_BASE_ADDRESS_2,
	PCI_BASE_ADDRESS_3,
	PCI_BASE_ADDRESS_4,
	PCI_BASE_ADDRESS_5
    };
    long flags;
    unsigned long bar_length;
    u32 mask, physaddr;
    /*
     * Atomicly play with configuration space. 
     * Disable Interrupts. (This isn't right for SMP.)
     *    Read current BAR physical address.
     *    Write ~0.
     *    Read back the BAR address mask.
     *    Write back the saved BAR physical address.
     * Restore interrupts.
     * This code assumes that the device does not present a
     * BAR address space that is larger than 32-bits.
     */
    physaddr = 0;
    save_flags(flags);
    cli();
    pci_read_config_dword(pci_dev, pci_addresses[bar_idx],
			  &physaddr);

    pci_write_config_dword(pci_dev, pci_addresses[bar_idx], ~0L);
    pci_read_config_dword(pci_dev, pci_addresses[bar_idx], &mask);
    pci_write_config_dword(pci_dev, pci_addresses[bar_idx],
			   physaddr);
    restore_flags(flags);
    if (type_ptr) {
	*type_ptr = (physaddr & 1L);
    }
    if (physaddr & 1) {	/* I/O space */
	bar_length = (~(mask & ~0x03L) + 1);
	physaddr &= ~0x03L;
    }
    else { /* Memory space */
	bar_length = (~(mask & ~0x0FL) + 1);
	physaddr &= ~0x0FL;
    }
    if (physaddr_ptr) {
	*physaddr_ptr = physaddr;
    }
    return bar_length;
}


static void _udi_pci_wakeup_device(struct pci_dev *pci_dev, udi_ubit8_t type)
{
	/*
	 * Type is either PCI_BAR_MEM_TYPE or PCI_BAR_IO_TYPE.
	 * This must be called AFTER mapping the device.
	 * On PowerMac hardware, this must be done, or else we'll
	 * get machine check exceptions when reading from IO/MEM.
	 */
	unsigned short old_command, new_command;
	unsigned char latency;

	/*
	 * If device is a bus master, make sure to enable it.
	 * If the device isn't a master, it will just ignore the
	 * change to the command register.
	 */
	pci_set_master(pci_dev);

	pci_read_config_word(pci_dev, PCI_COMMAND, &old_command);
	new_command = old_command;

	/* Enable the chosen memory range. */
	new_command |= (type == PCI_BAR_MEM_TYPE ? PCI_COMMAND_MEMORY : PCI_COMMAND_IO);

	if (new_command != old_command) {
		_OSDEP_PRINTF("UDI: device %04hX:%04hX has bad command: %d,"
					" new command code: %d\n",
					pci_dev->vendor, pci_dev->device,
					old_command, new_command);
		pci_write_config_word(pci_dev, PCI_COMMAND, new_command);

	}

	/*
	 * This isn't strictly necessary, but just for fun,
	 * lets play with device latency timers.
	 * Remove this after PowerMac hardware seems to work.
	 */
	pci_read_config_byte(pci_dev, PCI_LATENCY_TIMER, &latency);
	if (latency < 16) {
		/* kick it up to 64 */
		_OSDEP_PRINTF("UDI: device %04hx:%04hx had a small latency timer"
					" setting of %d.  Changing latency to 64.\n",
					pci_dev->vendor, pci_dev->device, latency);
		pci_write_config_byte(pci_dev, PCI_LATENCY_TIMER, 64);
	}
}


void _udi_get_pio_mapping(_udi_pio_handle_t * pio, _udi_dev_node_t * dev)
{
    unsigned long os_physaddr = 0;
    unsigned long bar_length;
    udi_ubit8_t bar_type;
    struct pci_dev *pci_dev;
    udi_boolean_t fake_hardware;

#ifdef CONFIG_PCI
    _OSDEP_ASSERT(pci_present());
#else
    _OSDEP_ASSERT(0);
#endif

    ASSERT(pio);
    ASSERT(dev);
    pio->pio_osinfo.pio_pci_dev = dev->node_osinfo.node_devinfo;
    pci_dev = dev->node_osinfo.node_devinfo;
    ASSERT(pci_dev);

    /* Handle Config Space mapings */
    if (pio->regset_idx == UDI_PCI_CONFIG_SPACE) {
	ASSERT(pio->offset <= 255
		&& pio->offset + pio->length <= 256);
	pio->pio_osinfo.baseaddr = pio->offset;
	pio->pio_osinfo.mapped_max_offset = pio->offset + pio->length;
	pio->pio_osinfo.hw_max_offset = 255 - pio->offset;
	pio->pio_osinfo._u.cfg.offset = pio->offset;
	pio->address_type = _UDI_PIO_CFG_SPACE;
	if (pio->pace) {
		if (pio->flags & _UDI_PIO_DO_SWAP) {
			pio->pio_ops = &_udi_pio_pci_cfg_paced_swapped_access;
		} else {
			pio->pio_ops = &_udi_pio_pci_cfg_paced_access;
		}
	} else {
		if (pio->flags & _UDI_PIO_DO_SWAP) {
			pio->pio_ops = &_udi_pio_pci_cfg_swapped_access;
		} else {
			pio->pio_ops = &_udi_pio_pci_cfg_access;
		}
	}
	return;
    }

    /* Handle memory mapped or io-mapped register sets. */
    ASSERT(pio->regset_idx < 6);
    os_physaddr = pci_resource_start(pci_dev, pio->regset_idx);
    bar_length = pci_resource_len(pci_dev, pio->regset_idx);

    /* We don't have divine knowledge of the type of BAR this is... */
    bar_type = pci_resource_type(pci_dev, pio->regset_idx);

    if (os_physaddr == 0x00) {
	/*
	 * KLUDGE: non-existent PCI mappings are mapped into 0x70 memory
	 * the udi_cmos driver needs this right now, since it is really a 
	 * "system" device that plays a pci device because we haven't 
	 * implemented the system bus yet.
	 */
	os_physaddr = 0x70;
	/*
	 * We could try to pass a bar_length of 2, but it might fail.
	 * These are the calculations for the length, since this
	 * results in the smallest IO space a PCI device can define.
	 * bar_mask = ~0x0F  ==> translate to bar_length
	 * (~(~0x0F & ~0x03) + 1) ==> reduce
	 * ~(0xFFFFFFF0 & 0xFFFFFFFC) + 1 ==> reduce
	 * ~0xFFFFFFF0 + 1 ==> reduce
	 * 0x0000000F + 1 ==> 0x10.
	 */
	bar_length = 0x10;
	bar_type = PCI_BAR_IO_TYPE;
	fake_hardware = TRUE;
    }
    else {
	fake_hardware = FALSE;
    }
    if (bar_type == PCI_BAR_IO_TYPE) {
	unsigned long range;

#ifdef DEBUG
	_OSDEP_PRINTF("_udi_get_pio_mapping: IO: pio->offset=%x, bar_length=%x, pio->length=%x, "
			"os_physaddr=%p\n",
		       (u32)pio->offset, (u32)bar_length, (u32)pio->length,
		       (void*)os_physaddr);
#endif

	ASSERT(pio->offset <= bar_length
	       && pio->offset + pio->length <= bar_length);

	pio->pio_osinfo.mapped_max_offset = pio->length;
	pio->pio_osinfo.hw_max_offset = bar_length - pio->offset;
	range = pio->pio_osinfo.mapped_max_offset;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0))
	request_region(os_physaddr + pio->offset, range,
		       dev->bind_channel->chan_region->reg_driver->
		       drv_name);
#else
	if (!request_region(os_physaddr + pio->offset, range,
		       dev->bind_channel->chan_region->reg_driver->
		       drv_name))
	{
#ifdef DEBUG
		_OSDEP_PRINTF("ERROR: could not satisfy request for IO os_physaddr=%ld\n",
				os_physaddr);
		/*
		 * The above error is annoying. Therefore, TODO:
		 * It happens because it is legal in UDI to map the same region
		 * several times simultaenously. What we really would like to do
		 * is to register a region with the kernel exactly once, and 
		 * extend and shrink that region is neccessary for additional
		 * UDI mappings. This involves tracking the mappings.
		 */
#endif
	}
#endif

	pio->address_type = _UDI_PIO_IO_SPACE;
	pio->pio_osinfo.baseaddr = os_physaddr + pio->offset;
	pio->pio_osinfo._u.io.addr = os_physaddr + pio->offset;
	if (pio->pace) {
	    if (pio->flags & _UDI_PIO_DO_SWAP) {
		pio->pio_ops = &_udi_pio_io_paced_swapped_access;
	    } else {
		pio->pio_ops = &_udi_pio_io_paced_access;
	    }
	} else {
	    if (pio->flags & _UDI_PIO_DO_SWAP) {
	        pio->pio_ops = &_udi_pio_io_swapped_access;
	    } else {
	        pio->pio_ops = &_udi_pio_io_access;
	    }
        }
    } else {			/* memory-mapped */
#ifdef DEBUG
	_OSDEP_PRINTF("_udi_get_pio_mapping: MEM: os_physaddr=%p, bar_length=%x,"
			"pio->offset=%x, pio->length=%x\n",
			(void*)os_physaddr, (u32)bar_length,
			(u32)pio->offset, (u32)pio->length);
#endif

	ASSERT(pio->offset <= bar_length
	       && pio->offset + pio->length <= bar_length);

	if (!request_mem_region(os_physaddr, pio->length,
		       dev->bind_channel->chan_region->reg_driver->
		       drv_name)) {
		_OSDEP_PRINTF("ERROR: could not satisfy request for that os_physaddr=%p\n",
				(void*)os_physaddr);
	}

	pio->address_type = _UDI_PIO_MEM_SPACE;
	pio->pio_osinfo.baseaddr = os_physaddr + pio->offset;
	pio->pio_osinfo._u.mem.mapped_addr =
	    ioremap(os_physaddr + pio->offset, pio->length);
	pio->pio_osinfo.mapped_max_offset = pio->length;
	pio->pio_osinfo.hw_max_offset = bar_length - pio->offset;

#ifdef DEBUG
	_OSDEP_PRINTF("pioh(%p) mapped regset %x (%p, length %lx) to %p.\n",
		      pio, pio->regset_idx, (char*)os_physaddr + pio->offset,
		      pio->length, pio->pio_osinfo._u.mem.mapped_addr);
#endif

	ASSERT(pio->pio_osinfo._u.mem.mapped_addr);

	if (pio->pace) {
	    if (pio->flags & _UDI_PIO_DO_SWAP) {
		pio->pio_ops = &_udi_pio_mem_paced_swapped_access;
	    } else
		pio->pio_ops = &_udi_pio_mem_paced_access;
	} else {
	    if (pio->flags & _UDI_PIO_DO_SWAP) {
		pio->pio_ops = &_udi_pio_mem_swapped_access;
	    } else
		pio->pio_ops = &_udi_pio_mem_access;
	}
    }
    if (!fake_hardware) {
	_udi_pci_wakeup_device(pci_dev, bar_type);
    }
}

/*
 * Destroy a PIO mapping.
 */

void _udi_release_pio_mapping(_udi_pio_handle_t * pio)
{
#ifdef DEBUG
    _OSDEP_PRINTF("releasing pio mapping\n");
#endif
    switch (pio->address_type) {
    case _UDI_PIO_MEM_SPACE:
	iounmap(pio->pio_osinfo._u.mem.mapped_addr);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
	release_mem_region((unsigned long)pio->pio_osinfo.baseaddr, 
			   pio->length);
#endif
	break;
    case _UDI_PIO_IO_SPACE:
	release_region(pio->pio_osinfo._u.io.addr, pio->length);
	break;
    case _UDI_PIO_CFG_SPACE:
	break;
    default:
	_OSDEP_PRINTF("_udi_release_pio_mapping: unknown address_type %d\n",
			pio->address_type);
	break;
    }
    udi_memset(&pio->pio_osinfo, 0, sizeof(pio->pio_osinfo));
}

/*
 * probe a PIO device.
 */

void
udi_pio_probe(udi_pio_probe_call_t * callback,
	      udi_cb_t * gcb,
	      udi_pio_handle_t pio_handle,
	      void *mem_ptr,
	      udi_ubit32_t pio_offset,
	      udi_ubit8_t tran_size_idx, udi_ubit8_t direction)
{
    udi_ubit32_t tran_size = 1 << tran_size_idx;
    _udi_pio_handle_t *pio = pio_handle;
    udi_status_t status;
    _udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);

    /*
     * Call the POPS functions for this mapping.   They'll do
     * any needed pacing, swapping, and so on for us.
     */
/*
 * TBD:
 * Infinite wisdom, care of robertl:
 *    It looks like the Linux version will have to call access_ok()
 *    but in order to have ready access to the kernel virtual of the access
 *    you're likely going to have to grope the osdep part of the handle so
 *    you can figure out the correct vaddrs.
 */
    switch (direction) {
    case UDI_PIO_IN:
	(*pio->pio_ops->std_ops.pio_read_ops[tran_size_idx])
	    (pio_offset, mem_ptr, tran_size, pio);
	break;
    case UDI_PIO_OUT:
	(*pio->pio_ops->std_ops.pio_write_ops[tran_size_idx])
	    (pio_offset, mem_ptr, tran_size, pio);
	break;
    default:
	_OSDEP_ASSERT(0);
    }

    /*status = END_CATCH() ? UDI_STAT_HW_PROBLEM : UDI_OK; */
    status = UDI_OK;
    _UDI_IMMED_CALLBACK(cb, _UDI_CT_PIO_PROBE,
			&_udi_alloc_ops_nop, callback, 1, (status));

}


/* Memory coherency operations */

void
_udi_osdep_pio_barrier(_udi_pio_handle_t *_pio_handle,
                _OSDEP_PIO_LONGEST_STYPE operand)
{
	if (operand)
		wmb();
	else
		mb();
}

void
_udi_osdep_pio_sync(_udi_pio_handle_t *_pio_handle,
                udi_index_t tran_size_idx, _OSDEP_PIO_LONGEST_STYPE operand)
{
#if NOT_NEEDED
	/* The PIO documentation alludes that some platforms might need this */
	volatile char *mem = &pio->pio_osinfo._u.mem.mapped_addr[operand];
	volatile char trash;
	tran_size_idx = 1 << tran_size_idx;
	while (tran_size_idx) {
		/* sync the memory by doing a quick read scan */
		trash = *mem;
		tran_size_idx--;
	}
#endif
	mb();
}


void
_udi_osdep_pio_sync_out(_udi_pio_handle_t *_pio_handle,
                udi_index_t tran_size_idx, _OSDEP_PIO_LONGEST_STYPE operand)
{
	wmb();
}

