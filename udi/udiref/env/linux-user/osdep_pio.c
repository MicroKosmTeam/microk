/*
 * File: env/linux-user/osdep_pio.c
 *
 * Implements user-land physical I/O access.
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


#include <udi_env.h>

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>

#if defined(__powerpc__) || defined(__sparc)
/* The out* functions go *(port, val), not (val, port). */
/* stub out ISA intrinsics. */
void  outb(udi_ubit8_t val, long port);
void  outb(udi_ubit8_t val, long port)
{(void)port;(void)val;}

void  outw(udi_ubit16_t val, long port);
void  outw(udi_ubit16_t val, long port)
{(void)port;(void)val;}

void  outl(udi_ubit32_t val, long port);
void  outl(udi_ubit32_t val, long port)
{(void)port;(void)val;}

udi_ubit8_t inb(long port);
udi_ubit8_t inb(long port)
{(void)port;return 0;}

udi_ubit16_t inw(long port);
udi_ubit16_t inw(long port)
{(void)port;return 0;}

udi_ubit32_t inl(long port);
udi_ubit32_t inl(long port)
{(void)port;return 0;}

extern int ioperm(long port, int length, char priv_ok);
#else
/* io defn's for userland work fine on x86 */
#include <sys/io.h>
#endif

/* Pretend we're kinda sorta kernel mode. */
#define __io_virt(x) x
#define readb(addr) (*(volatile unsigned char *) __io_virt(addr))
#define readw(addr) (*(volatile unsigned short *) __io_virt(addr))
#define readl(addr) (*(volatile unsigned int *) __io_virt(addr))

#define writeb(b,addr) (*(volatile unsigned char *) __io_virt(addr) = (b))
#define writew(b,addr) (*(volatile unsigned short *) __io_virt(addr) = (b))
#define writel(b,addr) (*(volatile unsigned int *) __io_virt(addr) = (b))


#define PIO_DEBUG 0

#define IS_ALIGNED(P, size) ( 0 == ((long)(P) & ((size) - 1)) )
#define VERIFY_ALIGNED(P, size) \
  if ((!(IS_ALIGNED(P, size)) && ((pio->flags & _UDI_PIO_UNALIGNED) == 0))) { \
        fprintf(stderr, "Alignment problem at %d: %p not %d byte aligned\n",  \
                        __LINE__, (void*)(P), (udi_ubit32_t) (size)); \
	abort(); \
	}


STATIC 
void 
mem_rd_many(udi_ubit32_t offset, 
	    void *data, 
	    udi_size_t size, 
	    _udi_pio_handle_t *pio)
{
	int i;
	_OSDEP_PIO_LONGEST_STYPE *r = data;
	_OSDEP_PIO_LONGEST_STYPE *s = (_OSDEP_PIO_LONGEST_STYPE *) &pio->pio_osinfo._u.mem.mapped_addr[offset];

	VERIFY_ALIGNED(s, sizeof(*s));
	VERIFY_ALIGNED(r, sizeof(*r));

	VERIFY_ALIGNED(offset, size);
	if (! (pio->flags & UDI_PIO_UNALIGNED)) {
		VERIFY_ALIGNED(offset, size);
	}

	/*
	 * TODO:  
	 */
	if (pio->pace)
		_OSDEP_ASSERT(0);

	if (pio->flags & _UDI_PIO_DO_SWAP) {
		udi_endian_swap(s, r, sizeof(*r), sizeof(*r), (size/sizeof(*r)));
		return;
	}
	for (i=0; i< size/sizeof(*r); i++) {
		*r++ = *s++;
	}
}

STATIC 
void 
mem_wr_many(udi_ubit32_t offset, 
	    void *data, 
	    udi_size_t size, 
	    _udi_pio_handle_t *pio)
{
	int i;
	_OSDEP_PIO_LONGEST_STYPE *r = data;
	_OSDEP_PIO_LONGEST_STYPE *s = (_OSDEP_PIO_LONGEST_STYPE *) &pio->pio_osinfo._u.mem.mapped_addr[offset];

	VERIFY_ALIGNED(s, sizeof(*s));
	VERIFY_ALIGNED(r, sizeof(*r));

	VERIFY_ALIGNED(offset, size);

	if (! (pio->flags & UDI_PIO_UNALIGNED)) {
		VERIFY_ALIGNED(offset, size);
	}

	/*
	 * TODO:  
	 */
	if (pio->pace)
		_OSDEP_ASSERT(0);

	if (pio->flags & _UDI_PIO_DO_SWAP) { 
		udi_endian_swap(r, s, sizeof(*r), sizeof(*r), (size/sizeof(*r)));
		return;
	}

	for (i=0; i< size/sizeof(*r); i++) {
		*s++ = *r++;
	}
}

typedef unsigned long ulong_t;
/* 
 * Handle I/O reads larger than we can atomically access.
 */
STATIC void
io_rd_many(udi_ubit32_t offset,
	   void *data,
	   udi_size_t size,
	   _udi_pio_handle_t *pio)
{
	int i;
	ulong_t d;
	ulong_t *addr = (ulong_t *) data;

	VERIFY_ALIGNED(offset, size);
	VERIFY_ALIGNED(addr, sizeof(*addr));

	if (! (pio->flags & UDI_PIO_UNALIGNED)) {
		VERIFY_ALIGNED(offset, size);
	}

	for (i = 0; i < (size / 4); i++) {
		d = inl(offset + pio->pio_osinfo._u.io.addr);
		*addr = d;
		offset += 4;
		addr++;
	}

	/*
	 * TODO:  
	 */
	if (pio->pace)
		_OSDEP_ASSERT(0);

	if (pio->flags & _UDI_PIO_DO_SWAP) {
		/*
		 * TODO: this could be wrong 
		 */
		udi_endian_swap(data, data, size, size, 1);
	}
}

/* 
 * Handle I/O writes larger than we can atomically access.
 */
STATIC void
io_wr_many(udi_ubit32_t offset,
	   void *data,
	   udi_size_t size,
	   _udi_pio_handle_t *pio)
{
	int i;
	ulong_t *addr = (ulong_t *) data;
	char swapped_buf[32];

	VERIFY_ALIGNED(offset, size);
	VERIFY_ALIGNED(addr, sizeof(*addr));

	if (! (pio->flags & UDI_PIO_UNALIGNED)) {
		VERIFY_ALIGNED(offset, size);
	}

	if (pio->flags & _UDI_PIO_DO_SWAP) {
		/*
		 * TODO: this could be wrong 
		 */
		udi_endian_swap(data, swapped_buf, size, size, 1);
		addr = (ulong_t *) swapped_buf;
	}

	for (i = 0; i < (size / 4); i++) {
		outl(*addr, offset + pio->pio_osinfo._u.io.addr);
		offset += 4;
		addr++;
	}

	/*
	 * TODO:  
	 */
	if (pio->pace)
		_OSDEP_ASSERT(0);

}


STATIC 
void 
pci_cfg_rd_many(udi_ubit32_t offset, 
	    void *data, 
	    udi_size_t size, 
	    _udi_pio_handle_t *pio)
{
	int i;
	udi_ubit32_t *r = data;
	udi_ubit32_t s;

	VERIFY_ALIGNED(r, sizeof(*r));

	VERIFY_ALIGNED(offset, size);
	if (! (pio->flags & UDI_PIO_UNALIGNED)) {
		VERIFY_ALIGNED(offset, size);
	}

	/*
	 * TODO:  
	 */
	if (pio->pace)
		_OSDEP_ASSERT(0);

	_OSDEP_PRINTF("pci_cfg_rd_many: warning, this hasn't been tested.\n");
	for (i=0; i< size/4; i++) {
		pread(pio->pio_osinfo._u.cfg.fd, &s, 4, pio->offset + offset);
		if (!(pio->flags & _UDI_PIO_DO_SWAP)) {
			*r++ = UDI_ENDIAN_SWAP_32(s);
		}
	}
}

STATIC 
void 
pci_cfg_wr_many(udi_ubit32_t offset, 
	    void *data, 
	    udi_size_t size, 
	    _udi_pio_handle_t *pio)
{
	int i;
	udi_ubit32_t *r = data;
	udi_ubit32_t s;

	VERIFY_ALIGNED(r, sizeof(*r));

	VERIFY_ALIGNED(offset, size);

	if (! (pio->flags & UDI_PIO_UNALIGNED)) {
		VERIFY_ALIGNED(offset, size);
	}

	/*
	 * TODO:  
	 */
	if (pio->pace)
		_OSDEP_ASSERT(0);

	_OSDEP_PRINTF("pci_cfg_wr_many: warning, this hasn't been tested.\n");
	for (i=0; i< size/4; i++) {
		pwrite(pio->pio_osinfo._u.cfg.fd, &s, 4, pio->offset + offset);
		if (!(pio->flags & _UDI_PIO_DO_SWAP)) {
			*r++ = UDI_ENDIAN_SWAP_32(s);
		}
	}
}

#include "osdep_pops.h"

void
_udi_get_pio_mapping( 
	_udi_pio_handle_t *pio,
	_udi_dev_node_t *dev)
{
	/* Implement a "bridge maplet" that's independent of a "real"
	 * bridge but has a simple concept of how to find and talk to
	 * memory and I/O space.
	 * Regset 1 == I/O space.
	 * Regset 2 == memory space.
	 * Regset 255 == PCI config space.
	 */
	void *mmapped_addr ;
	char cfgmem_dev[255];
	int cfgfd;
	unsigned long unit_address;
	int fd;

	switch (dev->node_osinfo.posix_dev_node.bus_type) {
	case bt_system:
		switch (pio->regset_idx) {
		case -UDI_SYSBUS_MEM_BUS:
			fd = open("/dev/zero", O_RDWR);
			goto sysbus;
		case UDI_SYSBUS_MEM_BUS: 
			fd = open("/dev/mem", O_RDWR);
sysbus:
			pio->address_type = _UDI_PIO_MEM_SPACE;
			if (fd < 0) {
				fprintf(stderr, "Can't open /dev/mem. "
					"Err %s\n", strerror(errno));
				abort();
			}
			pio->pio_osinfo._u.mem.fd = fd;
			mmapped_addr = mmap( 0, pio->length, 
				(PROT_READ|PROT_WRITE), 
				MAP_PRIVATE, pio->pio_osinfo._u.mem.fd, 
				0xf0000);
			assert((void *) -1 != mmapped_addr);
			pio->pio_osinfo._u.mem.mapped_addr = mmapped_addr;

			break;
				
		case UDI_SYSBUS_IO_BUS: 
			pio->address_type = _UDI_PIO_IO_SPACE;
			pio->pio_osinfo._u.io.addr = dev->node_osinfo.posix_dev_node.sysbus_data.sysbus_io_addr_lo;
			break;
		case 0:
			return;
		default:
			abort();
		}
		break;
			
	case bt_pci:
		switch (pio->regset_idx) {
		case UDI_PCI_BAR_0:
		case UDI_PCI_BAR_1:
		case UDI_PCI_BAR_2:
		case UDI_PCI_BAR_3:
		case UDI_PCI_BAR_4:
		case UDI_PCI_BAR_5:
			/* 
			 * TODO: This is (still) wrong.   We should look at
			 * bit zero of the BAR to determin if it's I/O or
			 * mem.   As a practical matter, this is here as 
			 * a crutch until I commit the changes to make
			 * udi_cmos and friends use system bus bindings.
		 	 * 07/10/01 robertl	
			 */
			if ( dev->bind_channel && 
			     0 == udi_strcmp("udi_cmos", dev->bind_channel->chan_region->reg_driver->drv_name)) {
				pio->pio_osinfo._u.io.addr = 0x70;
				pio->address_type = _UDI_PIO_IO_SPACE;
			}

			break;
		case UDI_PCI_CONFIG_SPACE:
			pio->address_type = _UDI_PIO_CFG_SPACE;
			
			unit_address = dev->node_osinfo.posix_dev_node.node_pci_unit_address;
			pio->pio_osinfo.pci_busnum = (unit_address >> 8);
			pio->pio_osinfo.pci_devnum = (unit_address >> 3) & 0x1f;
			pio->pio_osinfo.pci_fnnum  = (unit_address & 0x7);

			/* filemap the contents of 
			 * /proc/bus/pci/BUSNUMHEX/DEVNUMHEX.FNNUMHEX 
			 */
			sprintf(cfgmem_dev, "/proc/bus/pci/%02x/%02x.%01x",
				pio->pio_osinfo.pci_busnum,
				pio->pio_osinfo.pci_devnum,
				pio->pio_osinfo.pci_fnnum);
			printf("Opening PCI config memory %s for device %s\n", 
				cfgmem_dev,
				dev->bind_channel->chan_region->reg_driver->drv_name);
			cfgfd = open(cfgmem_dev, O_RDWR);
			if (cfgfd == -1) {
				perror("Are you root? Couldn't open a "
					"/proc/bus/pci/ device");
				assert(cfgfd != -1);
			}
			pio->pio_osinfo._u.cfg.fd = cfgfd;
			break;
		}
		break;
	default:
		_OSDEP_ASSERT(0);
	}

	/*
	 * Now that we've gone through the above to determine
	 * the addressing characteristics of this maping, examine
	 * the flags and the address type to determine and populate
	 * the appropriate pops function table.
	 */

	switch (pio->address_type) {
		case _UDI_PIO_IO_SPACE:
			pio->pio_ops = &_udi_pio_io_access;
			if (pio->flags & _UDI_PIO_DO_SWAP) {
				if (pio->pace)
					pio->pio_ops = &_udi_pio_io_paced_swapped_access;
				else
					pio->pio_ops = &_udi_pio_io_swapped_access;

			} else {
				if (pio->pace)
					pio->pio_ops = &_udi_pio_io_paced_access;
				else
					pio->pio_ops = &_udi_pio_io_access;
			}
			break;
		case _UDI_PIO_MEM_SPACE:
			pio->pio_ops = &_udi_pio_mem_access;
			if (pio->flags & _UDI_PIO_DO_SWAP) {
				pio->pio_ops = &_udi_pio_mem_swapped_access;
				if (pio->pace) {
					pio->pio_ops = &_udi_pio_mem_paced_swapped_access;
				} else {
					pio->pio_ops = &_udi_pio_mem_swapped_access;
				}
			} else {
				if (pio->pace) {
					pio->pio_ops = &_udi_pio_mem_paced_access;
				} else {
					pio->pio_ops = &_udi_pio_mem_access;
				}
			}
			break;
		case _UDI_PIO_INVAL_SPACE:
			/*
			 * TODO: Fix posix_bridgetest.
			 */
#if 0
			_OSDEP_ASSERT(0);
#else
			pio->pio_ops = &_udi_pio_mem_access;
#endif
			
			break;
		case _UDI_PIO_CFG_SPACE:
			pio->pio_ops = &_udi_pio_pci_cfg_access;
			if (pio->flags & _UDI_PIO_DO_SWAP) {
				pio->pio_ops = &_udi_pio_pci_cfg_swapped_access;
			}
			if (pio->pace) {  
				pio->pio_ops = &_udi_pio_pci_cfg_paced_access;
				if (pio->flags & _UDI_PIO_DO_SWAP) {
					pio->pio_ops =
						&_udi_pio_pci_cfg_paced_swapped_access;
				}
			}
			break;
	}

	return;
}

void
_udi_release_pio_mapping(
	_udi_pio_handle_t * handle)
{
	switch (handle->address_type) {
	case _UDI_PIO_MEM_SPACE:
		munmap(handle->pio_osinfo._u.mem.mapped_addr, 	
			handle->length);
		close(handle->pio_osinfo._u.mem.fd);
		break;
	case _UDI_PIO_CFG_SPACE:
		close(handle->pio_osinfo._u.cfg.fd);
		break;
	/* No other cases need special handling */
	default:
		break;
	}
}
