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
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#if !defined(sparc)
#include <sys/sysi86.h>
#include <sys/psw.h>
#endif	/* !defined(sparc) */
#include <sys/param.h>

/*
 * Wrapper functions for Sparc architecture to provide correct i/o
 * functionality. This needs to be fixed to do 'the right thing'
 */
#if defined(sparc) 
unsigned char inb( unsigned long addr )
{
	return 0xff;
}

unsigned short inw( unsigned long addr )
{
	return 0xffff;
}

unsigned long inl( unsigned long addr )
{
	return 0xffffffff;
}

void outb( unsigned long addr, unsigned char val )
{
}

void outw( unsigned long addr, unsigned short val )
{
}

void outl( unsigned long addr, unsigned long val )
{
}
#else	/* !sparc */
/*
 * The following are provided in osdep_pio_iout.s
 */
extern unsigned char inb(unsigned long port);
extern unsigned short inw(unsigned long port);
extern unsigned long inl(unsigned long port);
extern void outb(unsigned long port, unsigned char value);
extern void outw(unsigned long port, unsigned short value);
extern void outl(unsigned long port, unsigned long value);
#endif	/* defined(sparc) */

#define PIO_DEBUG 0

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

	/* Implement a "bridge maplet" that's independent of a "real"
	 * bridge but has a simple concept of how to find and talk to
	 * memory and I/O space.
	 * Regset 1 == I/O space.
	 * Regset 2 == memory space.
	 */

	void *mmapped_addr;

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
