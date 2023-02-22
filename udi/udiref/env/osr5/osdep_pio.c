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

/*
 * Written for SCO by: Rob Tarte
 * At: Pacific CodeWorks, Inc.
 * Contact: http://www.pacificcodeworks.com
 */

#define UDI_PCI_VERSION 0x101

#include <sys/pci.h>
#include <sys/types.h>
#include <sys/immu.h>
#include "udi_env.h"
#include "udi_pci.h"

/*
 * PIO routines
 * ------------------------------------------
 */

#define PCI_VENDOR_ID_OFFSET		0x00
#define PCI_DEVICE_ID_OFFSET		0x02
#define PCI_COMMAND_OFFSET		0x04
#define PCI_BASE_REGISTER_OFFSET	0x10
#define PCI_COMMAND_DISABLE		0xFFFC

/* 
 * Handle I/O reads larger than we can atomically access.
 */
static 
void
io_rd_many(udi_ubit32_t  offset, 
	void *data, 
	udi_size_t size, 
	_udi_pio_handle_t *pio)
{
	int i;
	ulong_t d;
	ulong_t *addr = (ulong_t *) data;

	for (i = 0; i < (size/4); i++) {
		d = inl(offset + pio->pio_osinfo._u.io_addr);
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
static
void
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

	for (i = 0; i < (size/4); i++) {
		outl(offset + pio->pio_osinfo._u.io_addr, *addr);
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
static 
void
mem_rd_many(
	udi_ubit32_t  offset, 
	void *data, 
	udi_size_t size, 
	_udi_pio_handle_t *pio)
{
	if (pio->flags & _UDI_PIO_DO_SWAP) {
		/*
		 * TODO: this could be wrong 
		 */
		udi_endian_swap(&pio->pio_osinfo._u.mapped_addr[offset],
			 data, size, size, 1);
	} else {
		bcopy((caddr_t)data, 
		      (caddr_t)&pio->pio_osinfo._u.mapped_addr[offset], size);
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
static
void
mem_wr_many(
	udi_ubit32_t offset, 
	void *data, 
	udi_size_t size, 
	_udi_pio_handle_t *pio)
{
	if (pio->flags & _UDI_PIO_DO_SWAP) {
		/*
		 * TODO: this could be wrong 
		 */
		udi_endian_swap(data,
			&pio->pio_osinfo._u.mapped_addr[offset],
			 size, size, 1);
	} else {
		bcopy((caddr_t)&pio->pio_osinfo._u.mapped_addr[offset], 
			(caddr_t)data, size);
	}
	/*
	 * TODO:
	 */
	if (pio->pace)
		_OSDEP_ASSERT(0);
}

static 
void
pci_cfg_rd_many(
	udi_ubit32_t  offset, 
	void *value, 
	udi_size_t size, 
	_udi_pio_handle_t *pio)
{
	while (size >= sizeof(unsigned long))
	{
		pci_readdword(&(pio->pio_osinfo.devinfo),
			(unsigned short)offset, value);

		value = (udi_ubit32_t *)((uchar_t *)value + 
			sizeof(unsigned long));

		offset += sizeof(unsigned long);

		size -= sizeof(unsigned long);
	}

	while (size >= sizeof(unsigned short))
	{
		pci_readword(&pio->pio_osinfo.devinfo,
			(unsigned short)offset, value);

		value = (udi_ubit32_t *)((uchar_t *)value + 
			sizeof(unsigned short));

		offset += sizeof(unsigned short);

		size -= sizeof(unsigned short);
	}

	while (size >= sizeof(unsigned char))
	{
		pci_readbyte(&pio->pio_osinfo.devinfo,
			(unsigned char)offset, value);

		value = (udi_ubit32_t *)((uchar_t *)value +
			sizeof(unsigned char));

		offset += sizeof(unsigned char);

		size -= sizeof(unsigned char);
	}
}

static
void
pci_cfg_wr_many(
	udi_ubit32_t offset, 
	void *value, 
	udi_size_t size, 
	_udi_pio_handle_t *pio)
{
	while (size >= sizeof(unsigned long))
	{
		pci_writedword(&(pio->pio_osinfo.devinfo),
			(unsigned short)offset, *(unsigned long *)value);

		value = (udi_ubit32_t *)((uchar_t *)value + 
			sizeof(unsigned long));

		offset += sizeof(unsigned long);

		size -= sizeof(unsigned long);
	}

	while (size >= sizeof(unsigned short))
	{
		pci_writedword(&pio->pio_osinfo.devinfo,
			(unsigned short)offset, *(unsigned short *)value);

		value = (udi_ubit32_t *)((uchar_t *)value + 
			sizeof(unsigned short));

		offset += sizeof(unsigned short);

		size -= sizeof(unsigned short);
	}

	while (size >= sizeof(unsigned char))
	{
		pci_writedword(&pio->pio_osinfo.devinfo,
			(unsigned char)offset, *(unsigned char *)value);

		value = (udi_ubit32_t *)((uchar_t *)value +
			sizeof(unsigned char));

		offset += sizeof(unsigned char);

		size -= sizeof(unsigned char);
	}
}
static
void 
pci_cfg_wr_swapped_many(
	udi_ubit32_t offset, 
	void *value, 
	udi_size_t size, 
	_udi_pio_handle_t *pio)
{
	/* TODO */
	_OSDEP_ASSERT(0);
}
static
void 
pci_cfg_rd_swapped_many(
	udi_ubit32_t offset, 
	void *value, 
	udi_size_t size, 
	_udi_pio_handle_t *pio)
{
	/* TODO */
	_OSDEP_ASSERT(0);
}

static
void 
pci_cfg_wr_paced_many(
	udi_ubit32_t offset, 
	void *value, 
	udi_size_t size, 
	_udi_pio_handle_t *pio)
{
	/* TODO */
	_OSDEP_ASSERT(0);
}
static
void 
pci_cfg_rd_paced_many(
	udi_ubit32_t offset, 
	void *value, 
	udi_size_t size, 
	_udi_pio_handle_t *pio)
{
	/* TODO */
	_OSDEP_ASSERT(0);
}

#include "osdep_pops.h"

void
_udi_get_pio_mapping(
	_udi_pio_handle_t	*pio,
	_udi_dev_node_t		*dev
)
{
	udi_ubit32_t		length;
	udi_ubit32_t		offset;
	udi_index_t		regset;
	_udi_dev_node_t		*node;
	ushort_t		command;
	ushort_t		dis_command;
	uint_t			ff_mask;
	size_t			roffset;
	/* Might need to be reconsidered for PCI 2.1 */
	uint_t			base;
	uint_t			bar_length;
	int			err;
	struct pci_devinfo	*dp;
	int			npages;

	length = pio->length;
	offset = pio->offset;
	regset = pio->regset_idx;

	ff_mask = 0xFFFFFFFF;

	node = dev;

	dp = &node->node_osinfo.devinfo;

	if (regset == UDI_PCI_CONFIG_SPACE) {
		pio->pio_osinfo.devinfo = node->node_osinfo.devinfo;
		pio->pio_osinfo._u.offset = offset;
		pio->pio_ops = &_udi_pio_pci_cfg_access;
		pio->address_type = _UDI_PIO_CFG_SPACE;
		
		return;
	}

	ASSERT(regset >= 0 && regset < 6);

	/*
	 * We would prefer to get the BAR info from resmgr CM_IOADDR/CM_MEMADDR
	 * but there's insufficient information to figure out which of these
	 * values corresponds to the desired BAR #, so we need to go read the
	 * registers ourselves. Yuck!
	 */
	/*
	 * Read the selected BAR to find the address information.
	 */
	pci_readword(dp, PCI_COMMAND_OFFSET, &command);

	roffset = PCI_BASE_REGISTER_OFFSET + regset * 4;

	pci_readdword(dp, roffset, (unsigned long *)&base);

	dis_command = command & PCI_COMMAND_DISABLE;

	pci_writeword(dp, PCI_COMMAND_OFFSET, dis_command);

	if (base == 0)
	{
		bar_length = 0;
	}
	else
	{
		pci_writedword(dp, roffset, ff_mask);

		pci_readdword(dp, roffset, (unsigned long *)&bar_length);

		pci_writedword(dp, roffset, base);
	}

	pci_writeword(dp, PCI_COMMAND_OFFSET, command);

	/* 
	 * KLUDGE ALERT:
	 * The udi_cmos driver will request a mapping at base 0x70.  Later,
	 * this will be treated as a system bus binding.   Until that 
	 * interface is available, we simply flip a few bits to trigger the
	 * code below to let this be treated as a "normal" PCI I/O device.
 	 */
	if (base == 0x00)
	{
		base = 0x71;
		bar_length = ~15;
	}

	if (base & 0x01)
	{
		/* I/O space */
		base &= ~0x03;

		/* XXX CA_MAKE_EXT_IO_ADDR for ccNUMA */
		bar_length = (~(bar_length & ~0x03) + 1);

		ASSERT(offset <= bar_length && offset + length <= bar_length);

		pio->pio_osinfo._u.io_addr = base + offset;

		pio->pio_ops = &_udi_pio_io_access;

		pio->address_type = _UDI_PIO_IO_SPACE;

		if (pio->pace)
		{
			pio->pio_ops = &_udi_pio_io_paced_access;
		}

		if (pio->flags & _UDI_PIO_DO_SWAP)
		{
			pio->pio_ops = &_udi_pio_io_swapped_access;
		}

		if (pio->pace)
		{
			pio->pio_ops = &_udi_pio_io_paced_access;

			if (pio->flags & _UDI_PIO_DO_SWAP)
			{
				pio->pio_ops =
					&_udi_pio_io_paced_swapped_access;
			}
		}
#if PIO_DEBUG
		udi_debug_printf("  I/O 0x%x\n", pio->pio_osinfo._u.io_addr);
#endif
		return;
	}

	/*
	 * Memory mapped
	 */
	base &= ~0x0F;

	bar_length = (~(bar_length & ~0x0F) + 1);

	ASSERT(offset <= bar_length && offset + length <= bar_length);

	npages = btoc(base + offset - ctob(btoct(base + offset)) +
		length);

	pio->pio_osinfo.npages = npages;

	/* 
	 * Though one might be tempted to try to use devemem_mapin()
	 * from DDI8, it as has an interface (specifically the 
	 * definition of 'memory block number') that is ill-suited
	 * to the UDI needs of accessing PCI BARs...
	 */
	pio->pio_osinfo._u.mapped_addr = (udi_ubit8_t *)
		sptalloc(npages, PG_RW|PG_PCD|PG_P, btoct(base + offset), 0) +
			  ((base + offset) & POFFMASK);

	_OSDEP_ASSERT(pio->pio_osinfo._u.mapped_addr);		

	pio->pio_ops = &_udi_pio_mem_access;

	pio->address_type = _UDI_PIO_MEM_SPACE;

	if (pio->flags & _UDI_PIO_DO_SWAP)
	{
		pio->pio_ops = &_udi_pio_mem_swapped_access;
	}

	if (pio->pace)
	{
		pio->pio_ops = &_udi_pio_mem_paced_access;

		if (pio->flags & _UDI_PIO_DO_SWAP)
		{
			pio->pio_ops = &_udi_pio_mem_paced_swapped_access;
		}
	}

#if PIO_DEBUG
	udi_debug_printf("  mem phys 0x%x/virt 0x%x\n",
		(base + offset), 
		(udi_ubit32_t)pio->pio_osinfo._u.mapped_addr);
#endif

	return;
}

void
_udi_release_pio_mapping( 
	_udi_pio_handle_t *pio
)
{
	switch (pio->address_type)
	{
	case  _UDI_PIO_MEM_SPACE:
		sptfree((caddr_t)ctob(btoct(pio->pio_osinfo._u.mapped_addr)),
			pio->pio_osinfo.npages, 0);
		break;
	default:
		break;
	}
}

void 
udi_pio_probe(
        udi_pio_probe_call_t *callback,
        udi_cb_t *gcb,
	udi_pio_handle_t pio_handle,
	void *mem_ptr,
	udi_ubit32_t pio_offset,
	udi_ubit8_t tran_size_idx,
	udi_ubit8_t direction)
{
	udi_ubit32_t		tran_size;
	_udi_pio_handle_t	*pio;
	udi_status_t		status;
	_udi_cb_t		*cb;

	tran_size = 1 << tran_size_idx;

	pio = pio_handle;

	cb = _UDI_CB_TO_HEADER(gcb);

#if 0
	/* 
	 * Let the OS VM system trap any bad accesses for us.   If it
	 * detects that anything naughty happened, END_CATCH will return
	 * non-zero.
	 */

	CATCH_FAULTS(CATCH_ALL_FAULTS) {
#endif
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
			(*pio->pio_ops->std_ops.pio_read_ops[tran_size_idx])
				(pio_offset, mem_ptr, tran_size, pio);
		break;
		default: _OSDEP_ASSERT(0);
		}
#if 0
	}

	status = END_CATCH() ? UDI_STAT_HW_PROBLEM : UDI_OK;
#else
	status = UDI_OK;
#endif

	_UDI_IMMED_CALLBACK(cb, _UDI_CT_PIO_PROBE, &_udi_alloc_ops_nop,
		callback, 1, (status));
}
