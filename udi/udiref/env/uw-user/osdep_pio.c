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

#include "udi_env.h"

#define UDI_SYSBUS_VERSION 0x100
#include <udi_sysbus.h>

#define UDI_PCI_VERSION 0x101
#include <udi_pci.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/sysi86.h>
#include <sys/param.h>

#if __USLC__
# include <sys/inline.h>
#endif

#if __GNUC__

/*
 * This is hardly a good example of "portable" code.   GNU C cannot
 * grok <sys/inline.h> on  UnixWare.   So I took the asms from the GCC 
 * FAQ, swapped the order on out* to match the UnixWare Way, modified
 * the assembler syntax to match, and we're in business.
 */
inline unsigned char  inb  (unsigned short port) 
{
	unsigned char  _v;  
	__asm__ __volatile__ ("inb (%w1)#,%0" 
		: "=a" (_v) 
		: "Nd" (port)   ); 
	return _v; 
}

inline unsigned short  inw  (unsigned short port) 
{ 
	unsigned short  _v;  
	__asm__ __volatile__ ("inw (%w1)#,%0"  
		: "=a" (_v) 
		: "Nd" (port)   );
	return _v;
}

inline unsigned int  inl  (unsigned short port) 
{ 
	unsigned int  _v; 
	__asm__ __volatile__ ("inl (%w1)#,%0"  
		: "=a" (_v) 
		: "Nd" (port)   );
	return _v;
}

extern inline void
outb (unsigned short port, unsigned char value)
{
	__asm__ __volatile__ ("outb (%w1)"::"a" (value),
                        "Nd" (port));
}

extern inline void
outw (unsigned short port, unsigned short value)
{
	__asm__ __volatile__ ("outw (%w1)"::"a" (value),
                        "Nd" (port));
}

extern inline void
outl (unsigned short port, unsigned int value)
{
	__asm__ __volatile__ ("outl (%w1)"::"a" (value),
                        "Nd" (port));
}


#endif

/* TODO: These functions should be implemented. */
static void io_rd_many(udi_ubit32_t offset, void *data, udi_size_t size, _udi_pio_handle_t *xxx){_OSDEP_ASSERT(0);}
static void io_wr_many(udi_ubit32_t offset, void *data, udi_size_t size, _udi_pio_handle_t *xxx){_OSDEP_ASSERT(0);}

static 
void 
mem_rd_many(udi_ubit32_t offset, 
	    void *data, 
	    udi_size_t size, 
	    _udi_pio_handle_t *pio)
{
	int i;
	_OSDEP_PIO_LONGEST_STYPE *r = data;
	_OSDEP_PIO_LONGEST_STYPE *s = (_OSDEP_PIO_LONGEST_STYPE *) &pio->pio_osinfo._u.mem.mapped_addr[offset];

	if (pio->flags & _UDI_PIO_DO_SWAP) { 
		udi_endian_swap(s, r, sizeof(*r), sizeof(*r), size/sizeof(*r));
		return;
	}

	for (i=0; i< size/sizeof(*r); i++) {
		*r++ = *s++;
	}

	if (pio->pace) {
		_OSDEP_ASSERT(0); /* FIXME: TODO */
	}

}

static 
void 
mem_wr_many(udi_ubit32_t offset, 
	    void *data, 
	    udi_size_t size, 
	    _udi_pio_handle_t *pio)
{
	int i;
	_OSDEP_PIO_LONGEST_STYPE *r = data;
	_OSDEP_PIO_LONGEST_STYPE *s = (_OSDEP_PIO_LONGEST_STYPE *) &pio->pio_osinfo._u.mem.mapped_addr[offset];

	if (pio->flags & _UDI_PIO_DO_SWAP) { 
		udi_endian_swap(r, s, sizeof(*r), sizeof(*r), size/sizeof(*r));
		return;
	}

	for (i=0; i< size/sizeof(*r); i++) {
		*s++ = *r++;
	}

	if (pio->pace) {
		_OSDEP_ASSERT(0); /* FIXME: TODO */
	}
}

#include "osdep_pops.h"

void
_udi_get_pio_mapping( 
	_udi_pio_handle_t *pio,
	_udi_dev_node_t *dev)
{
	void *mmapped_addr;
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
			if (fd < 0) {
				fprintf(stderr, "Can't open /dev/mem.  "
					"Err %s\n", strerror(errno));
				abort();
			}
			mmapped_addr = mmap( (void *) 0, 
				pio->length + PAGEOFFSET,
				(PROT_READ|PROT_WRITE), MAP_SHARED,
				pio->pio_osinfo._u.mem.fd, 
				pio->offset & PAGEMASK);
			assert((void *) -1 != mmapped_addr);
			pio->address_type = _UDI_PIO_MEM_SPACE;
			pio->pio_osinfo._u.mem.fd = fd;
			pio->pio_osinfo._u.mem.mapped_addr = mmapped_addr;
			break;
		case UDI_SYSBUS_IO_BUS:
			pio->address_type = _UDI_PIO_IO_SPACE;
			pio->pio_osinfo._u.io.addr = dev->node_osinfo.posix_dev_node.sysbus_data.sysbus_io_addr_lo;
			break;
		default:
			_OSDEP_ASSERT(0);
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
			 * TODO: read bar; test IO vs. MEM bit.
			 * set address_type = _UDI_PIO_{MEM,IO}_SPACE;
			 */ 
			_OSDEP_ASSERT(0);
			
		case UDI_PCI_CONFIG_SPACE:
			 pio->address_type = _UDI_PIO_CFG_SPACE;
			_OSDEP_ASSERT(0);
		default:
			_OSDEP_ASSERT(0);
		}
		
	case bt_unknown:
		_OSDEP_ASSERT(0);

	}


#if 0
	switch (pio->regset_idx) {
	case 0:
		/* Not a "real" PIO mapping. */
		pio->pio_ops = NULL; 
		break;
	case 1:
		/* The IOPL will be raised, making I/O possible,  in 
		 * the (misleadingly named) UDI_PIO_MAP.
		 */
		pio->address_type = _UDI_PIO_IO_SPACE;
		pio->pio_osinfo._u.io.addr = pio->offset;
		
		/* Special kludge for CMOS driver */	
		if ( 0 == udi_strcmp("udi_cmos", 
		  	dev->bind_channel->chan_region->reg_driver->drv_name)) {
			pio->pio_osinfo._u.io.addr = 0x70;
		}
		pio->pio_ops = &_udi_pio_io_access;
		if (pio->flags & _UDI_PIO_DO_SWAP) {
			pio->pio_ops = &_udi_pio_io_swapped_access;
		}
		if (pio->pace) {
			pio->pio_ops = &_udi_pio_io_paced_access;
			if (pio->flags & _UDI_PIO_DO_SWAP) {
				pio->pio_ops = 
					&_udi_pio_io_paced_swapped_access;
			}
		}
		break;

	/*
	 * A special case to treat "normal" mem like device mem.
  	 */
	case 3: 
		pio->pio_osinfo._u.mem.fd = open("/dev/zero", O_RDWR);
		goto finish_map;
		
	case 2: 
		pio->pio_osinfo._u.mem.fd = open("/dev/mem", O_RDWR);
	finish_map:
		if (pio->pio_osinfo._u.mem.fd < 0) {
			fprintf(stderr, "Can't open /dev/mem. Err %s\n", 
				strerror(errno));
			abort();
		}
		pio->address_type = _UDI_PIO_MEM_SPACE;
		mmapped_addr = mmap( (void *) 0, pio->length + PAGEOFFSET, 
			(PROT_READ|PROT_WRITE), MAP_SHARED, 
			pio->pio_osinfo._u.mem.fd, pio->offset & PAGEMASK);
		assert((void *) -1 != mmapped_addr);

		pio->pio_osinfo._u.mem.mapped_addr = (void *) 
			((char *) mmapped_addr + (pio->offset & PAGEOFFSET));

		/* No, these shouldn't be mutually exclusive like this... */
		pio->pio_ops = &_udi_pio_mem_access;
		if (pio->flags & _UDI_PIO_DO_SWAP) {
			pio->pio_ops = &_udi_pio_mem_swapped_access;
		}
		if (pio->pace) {
			pio->pio_ops = &_udi_pio_mem_paced_access;
			if (pio->flags & _UDI_PIO_DO_SWAP) {
				pio->pio_ops = 
					&_udi_pio_mem_paced_swapped_access;
			}
		}
		break;
	default:
		abort();
	}
#endif

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
			_OSDEP_ASSERT(0);
			break;
		case _UDI_PIO_CFG_SPACE:
#if 0
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
#else
			_OSDEP_ASSERT(0);
#endif
			break;
	}

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
		/* No other cases need special handling */
	default:
		break;
	}
}
