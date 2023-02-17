#include <udi_env.h>

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <sys/types.h>

/* freebsd stuff here */
/* XXX: Add support for Alpha and IA64 ports */
#include <machine/cpufunc.h>
#include <machine/sysarch.h>
#define ioperm(ptr,len,huh) i386_set_ioperm(ptr, len, huh)

/*
#define PIO_DEBUG 0
*/

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
static void
io_rd_many(udi_ubit32_t offset,
	   void *data,
	   udi_size_t size,
	   _udi_pio_handle_t *pio)
{
	int i;
	ulong_t d;
	ulong_t *addr = (ulong_t *) data;

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
static void
io_wr_many(udi_ubit32_t offset,
	   void *data,
	   udi_size_t size,
	   _udi_pio_handle_t *pio)
{
	int i;
	ulong_t d;
	ulong_t *addr = (ulong_t *) data;
	char swapped_buf[32];

	if (pio->flags & _UDI_PIO_DO_SWAP) {
		/*
		 * TODO: this could be wrong 
		 */
		udi_endian_swap(data, swapped_buf, size, size, 1);
		addr = (ulong_t *) swapped_buf;
	}

	for (i = 0; i < (size / 4); i++) {
		outl(offset + pio->pio_osinfo._u.io.addr, *addr);
		*addr = d;
		offset += 4;
		addr++;
	}

	/*
	 * TODO:  
	 */
	if (pio->pace)
		_OSDEP_ASSERT(0);

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
	void *mmapped_addr ;

	switch (pio->regset_idx) {
	case 0:
		/* Not a "real" PIO mapping. */
		pio->pio_ops = NULL; 
		break;
	case 1:
		pio->pio_osinfo._u.io.addr = pio->offset;
		/* Special kludge for CMOS driver */	
		if ( 0 == udi_strcmp("udi_cmos", 
		  	dev->bind_channel->chan_region->reg_driver->drv_name)) {
			pio->pio_osinfo._u.io.addr = 0x70;
		}

		if (0 > ioperm(
			(unsigned long) pio->pio_osinfo._u.io.addr, 
			pio->length, TRUE)) {
			fprintf(stderr, 
			  "Unable to set ioperm for %x.  Are you root?\n", 
			  pio->pio_osinfo._u.io.addr);
			abort();
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
                pio->pio_osinfo._u.mem.mapped_addr = calloc(pio->length, 1);
                assert(pio->pio_osinfo._u.mem.mapped_addr);
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

	case 2:
		pio->pio_osinfo._u.mem.fd = open("/dev/mem", O_RDWR);
		if (pio->pio_osinfo._u.mem.fd < 0) {
			fprintf(stderr, "Can't open /dev/mem. Err %s\n", 
				strerror(errno));
			abort();
		}
		mmapped_addr = mmap( 0, pio->length, 
			(PROT_READ|PROT_WRITE), MAP_SHARED, 
			pio->pio_osinfo._u.mem.fd, 0xf0000);
		assert((void *) -1 != mmapped_addr);

		pio->pio_osinfo._u.mem.mapped_addr = mmapped_addr;

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

