/*
 * File: env/posix/posix.c
 *
 * FreeBSD Programmed I/O (PIO) code.
 * Very much like the UnixWare code except in the build of the mappings.
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

#define UDI_PCI_VERSION 0x101

#include <udi_env.h>
#include <udi_pci.h>
#include <sys/bus.h>
#include <machine/bus.h>
#include <sys/rman.h>
#include <machine/resource.h>

/* 
 * FIXME: This is morally wrong.   It's another place where we are too 
 * cozy with the internals of the FreeBSD PCI system.
 */
#include "/usr/src/sys/pci/pcivar.h"

#define PIO_DEBUG 0

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
	u_int d;
	u_int *addr = (u_int *) data;

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
	u_int d;
	u_int *addr = (u_int *) data;
	char swapped_buf[32];

	if (pio->flags & _UDI_PIO_DO_SWAP) {
		/*
		 * TODO: this could be wrong 
		 */
		udi_endian_swap(data, swapped_buf, size, size, 1);
		addr = (u_int *) swapped_buf;
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

/* 
 * Handle memory reads larger than we can atomically access.
 */
static void
mem_rd_many(udi_ubit32_t offset,
	    void *data,
	    udi_size_t size,
	    _udi_pio_handle_t *pio)
{
	if (pio->flags & _UDI_PIO_DO_SWAP) {
		/*
		 * TODO: this could be wrong 
		 */
		udi_endian_swap(&pio->pio_osinfo._u.mem.mapped_addr[offset],
				data, size, size, 1);
	} else {
		bcopy(data, &pio->pio_osinfo._u.mem.mapped_addr[offset], size);
	}

	/*
	 * TODO:  
	 */
	if (pio->pace)
		_OSDEP_ASSERT(0);

}

/* 
 * Handle memory writes larger than we can atomically access.
 */
static void
mem_wr_many(udi_ubit32_t offset,
	    void *data,
	    udi_size_t size,
	    _udi_pio_handle_t *pio)
{
	if (pio->flags & _UDI_PIO_DO_SWAP) {
		/*
		 * TODO: this could be wrong 
		 */
		udi_endian_swap(data,
				&pio->pio_osinfo._u.mem.mapped_addr[offset],
				size, size, 1);
	} else {
		bcopy(&pio->pio_osinfo._u.mem.mapped_addr[offset], data, size);
	}
	/*
	 * TODO:  
	 */
	if (pio->pace)
		_OSDEP_ASSERT(0);
}

static void
pci_cfg_rd_many(udi_ubit32_t offset,
		void *data,
		udi_size_t size,
		_udi_pio_handle_t *pio)
{
	udi_size_t loop = size / 4;
	udi_ubit32_t d;
	/*
	 * TODO: attribute handling 
	 */
	while (loop--) {
#if 0
		d = pci_read_config(pio->pio_osinfo.device, 
				   pio->pio_osinfo._u.cfg.offset + offset, 4);
#endif
		((udi_ubit32_t) data) = d;
		offset += 4;
	}
}

static void
pci_cfg_wr_many(udi_ubit32_t offset,
		void *data,
		udi_size_t size,
		_udi_pio_handle_t *pio)
{
	/*
	 * TODO: attribute handling 
	 */
#if 0
	cm_write_devconfig(pio->pio_osinfo.cfg_key,
			   (size_t) offset, data, size);
#endif
	/* TODO: pci_write_config(...); */
	_OSDEP_ASSERT(0);
}

#include "osdep_pops.h"

/*
 * Internal helper function to populate a PIO handle with information 
 * appropriate for an I/O mapping.
 */
static void
_udi_set_pio_io_addr(_udi_pio_handle_t *pio,
		     unsigned long base,
		     unsigned long offset,
		      struct resource *res)
{

	pio->pio_osinfo._u.io.addr = base + offset;
	pio->address_type = _UDI_PIO_IO_SPACE;
	pio->pio_ops = &_udi_pio_io_access;

	if (pio->flags & _UDI_PIO_DO_SWAP) {
		pio->pio_ops = &_udi_pio_io_swapped_access;
	}

	if (pio->pace) {
		pio->pio_ops = &_udi_pio_io_paced_access;
		if (pio->flags & _UDI_PIO_DO_SWAP) {
			pio->pio_ops = &_udi_pio_io_paced_swapped_access;
		}
	}
#if PIO_DEBUG
	printf("  I/O 0x%x\n", pio->pio_osinfo._u.io.addr);
#endif
}

/*
 * Internal helper function to populate a PIO handle with information 
 * appropriate for an I/O mapping.
 */
static udi_boolean_t
_udi_set_pio_mem_addr(_udi_pio_handle_t *pio,
		      unsigned long base,
		      unsigned long offset, 
		      struct resource *res)
{
	pio->pio_osinfo._u.mem.tag = rman_get_bustag(res);
	pio->pio_osinfo._u.mem.handle = rman_get_bushandle(res);
	pio->pio_osinfo._u.mem.mapped_addr = pio->pio_osinfo._u.mem.handle + offset;

	pio->address_type = _UDI_PIO_MEM_SPACE;
	pio->pio_ops = &_udi_pio_mem_access;

	if (pio->flags & _UDI_PIO_DO_SWAP) {
		pio->pio_ops = &_udi_pio_mem_swapped_access;
	}

	if (pio->pace) {
		pio->pio_ops = &_udi_pio_mem_paced_access;
		if (pio->flags & _UDI_PIO_DO_SWAP) {
			pio->pio_ops = &_udi_pio_mem_paced_swapped_access;
		}
	}
#if PIO_DEBUG
	printf("  mem phys 0x%x/virt 0x%x\n", (udi_ubit32_t) (base + offset),
	       (udi_ubit32_t)pio->pio_osinfo._u.mem.mapped_addr);
#endif

	return UDI_OK;
}

void
_udi_get_pio_mapping(_udi_pio_handle_t *pio,
		     _udi_dev_node_t *dev)
{
	_OSDEP_DEV_NODE_T *osinfo = &dev->node_osinfo;
	int pci_cfg_off;
	udi_index_t regset;
	udi_ubit32_t length;
	udi_ubit32_t offset;            
	udi_ubit32_t physaddr;            
	udi_ubit32_t ff_mask = 0xffffffff;
	udi_ubit32_t bar_length;
	
	length = pio->length;
	offset = pio->offset;
	regset = pio->regset_idx;

	if (regset == UDI_PCI_CONFIG_SPACE) {
                pio->pio_osinfo.device = osinfo->device;
                pio->pio_osinfo._u.cfg.offset = offset;
                pio->pio_ops = &_udi_pio_pci_cfg_access;
                pio->address_type = _UDI_PIO_CFG_SPACE;

                return;
        }

	_OSDEP_ASSERT(regset < sizeof(osinfo->bus_resource) / 
			sizeof(osinfo->bus_resource[0]));

	/* Find the BAR */
	pci_cfg_off = 0x10 + regset * 4;

	/* FreeBSD uses the config space for the resource ID.   Odd. */
	pio->pio_osinfo._u.mem.rid = pci_cfg_off;

	/*
	 * I think FreeBSD could free us of all this if we just called
	 * bus_space_{read,write}_{1,2,4} in our POPS functions and collapsed
	 * mem and i/o POPs vectors.   But that's a bit foreign to me, so I"ll
	 * go tiwh what I know and we'll optimize/nativize later.
	 * But we may _still_ have to grope PCI config space in order to
	 * determine if it needs to be bus_resource_alloced with _MEM or _IO
	 * anyway. :-(
	 */	
	physaddr = pci_read_config(osinfo->device, pci_cfg_off, 4);
	pci_write_config(osinfo->device, pci_cfg_off, ff_mask, 4);
	bar_length = pci_read_config(osinfo->device, pci_cfg_off, 4);
	pci_write_config(osinfo->device, pci_cfg_off, physaddr, 4);

	if (physaddr & 0x01) {
		physaddr &= ~0x03;
		bar_length = (~(bar_length & ~0x0F) + 1);
		if (osinfo->bus_resource[regset] == NULL) {
			osinfo->bus_resource[regset] = 
				bus_alloc_resource(osinfo->device, 
				SYS_RES_IOPORT, &pci_cfg_off, 0, ~0, 
				pio->length, RF_ACTIVE);
		}
		_udi_set_pio_io_addr(pio, physaddr, offset, osinfo->bus_resource[regset]);
	} else {
		physaddr &= ~0x0F;
		bar_length = (~(bar_length & ~0x0F) + 1);
		if (osinfo->bus_resource[regset] == NULL) {
			osinfo->bus_resource[regset] = 
				bus_alloc_resource(osinfo->device, 
				SYS_RES_MEMORY, &pci_cfg_off, 0, ~0, 
				pio->length, RF_ACTIVE);
		}
		_udi_set_pio_mem_addr(pio, physaddr, offset, osinfo->bus_resource[regset]);
	}
}

void
_udi_release_pio_mapping(_udi_pio_handle_t *pio)
{
	/*
	 * Since the bus resources are shared across all MAPs, we dont
	 * nuke the mapping here.   That's done in node_deinit when this
	 * driver instance is blasted.
	 */
}

/* FIXME: TODO */
#if LATER
void
udi_pio_probe(udi_pio_probe_call_t * callback,
	      udi_cb_t *gcb,
	      udi_pio_handle_t pio_handle,
	      void *mem_ptr,
	      udi_ubit32_t pio_offset,
	      udi_ubit8_t tran_size_idx,
	      udi_ubit8_t direction)
{
	udi_ubit32_t tran_size = 1 << tran_size_idx;
	_udi_pio_handle_t *pio = pio_handle;
	udi_status_t status;
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);

	/*
	 * Let the OS VM system trap any bad accesses for us.   If it
	 * detects that anything naughty happened, END_CATCH will return
	 * non-zero.
	 */

	CATCH_FAULTS(CATCH_ALL_FAULTS) {
		/*
		 * Call the POPS functions for htis mapping.   They'll do 
		 * any needed pacing, swapping, and so on for us.
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
	}

	status = END_CATCH()? UDI_STAT_HW_PROBLEM : UDI_OK;
	_UDI_IMMED_CALLBACK(cb, _UDI_CT_PIO_PROBE,
			    &_udi_alloc_ops_nop, callback, 1, (status));

}
#endif
