
/*
 * UnixWare 7 PIO handlers.
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
#define _DDI 8

#include <udi_env.h>
#include <udi_pci.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#include <sys/hpci_bios.h>

#include <vm/faultcatch.h>
#include <sys/user.h>

#include <sys/ddi.h>

#define PCI_COMMAND_DISABLE	0xfffc

#if  _UDI_SUPPRESS_PIO_DEBUGGING
#define VERIFY_ALIGNED(P, size)
#else 
#define IS_ALIGNED(P, size) (0 == ((long)(P) & (size) - 1))
#define VERIFY_ALIGNED(P, size) \
  if ((!(IS_ALIGNED(P, size)) && ((pio->flags & _UDI_PIO_UNALIGNED) == 0)))  \
      _OSDEP_PRINTF("Alignment problem at %d: %p not %d byte aligned\n", \
                      __LINE__, P, (size));
#endif


#if __USLC__
# include <sys/inline.h>
#endif


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

/* 
 * Handle memory reads larger than we can atomically access.
 */
static void
mem_rd_many(udi_ubit32_t offset,
	    void *data,
	    udi_size_t size,
	    _udi_pio_handle_t *pio)
{
	if (!(pio->flags & _UDI_PIO_UNALIGNED)) {
		VERIFY_ALIGNED(offset, size);
	}
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
	if (!(pio->flags & _UDI_PIO_UNALIGNED)) {
		VERIFY_ALIGNED(offset, size);
	}

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
	/*
	 * TODO: attribute handling 
	 */
	cm_read_devconfig(pio->pio_osinfo.cfg_key,
			  (size_t) offset, data, size);
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
	cm_write_devconfig(pio->pio_osinfo.cfg_key,
			   (size_t) offset, data, size);
}

#include "osdep_pops.h"

/*
 * Internal helper function to populate a PIO handle with information 
 * appropriate for an I/O mapping.
 */
STATIC void
_udi_set_pio_io_addr(_udi_pio_handle_t *pio,
		     unsigned long base,
		     unsigned long offset)
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
STATIC void
_udi_set_pio_mem_addr(_udi_pio_handle_t *pio,
		      unsigned long base,
		      unsigned long offset)
{

	/*
	 * Though one might be tempted to try to use devemem_mapin()
	 * from DDI8, it as has an interface (specifically the 
	 * definition of 'memory block number') that is ill-suited
	 * to the UDI needs of accessing PCI BARs...
	 */
	pio->pio_osinfo._u.mem.mapped_addr =
		physmap64((paddr64_t) (base + offset), pio->length, TRUE);
	_OSDEP_ASSERT(pio->pio_osinfo._u.mem.mapped_addr);

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
	printf("  mem phys 0x%x/virt 0x%x\n", (base + offset),
	       (udi_ubit32_t)pio->pio_osinfo._u.mem.mapped_addr);
#endif

}

void
_udi_get_pio_mapping(_udi_pio_handle_t *pio,
		     _udi_dev_node_t *dev)
{
	udi_ubit32_t length = pio->length;
	udi_ubit32_t offset = pio->offset;
	udi_index_t regset = pio->regset_idx;
	_udi_dev_node_t *node = dev;
	rm_key_t key = node->node_osinfo.rm_key;
	cm_args_t cm;
	cm_num_t bustype;
	cm_range_t range;
	ushort_t command, dis_command;
	uint_t ff_mask = 0xFFFFFFFF;
	size_t roffset;

	/*
	 * Might need to be reconsidered for PCI 2.1 
	 */
	uint_t base, bar_length;
	int err;


	/*
	 * Find out what bus type the resmgr says this rmkey holds.
	 */
	cm.cm_key = key;
	cm_begin_trans(cm.cm_key, RM_READ);
	cm.cm_param = CM_BRDBUSTYPE;
	cm.cm_val = &bustype;
	cm.cm_vallen = sizeof bustype;
	cm.cm_n = 0;
	err = cm_getval(&cm);
	_OSDEP_ASSERT(err == 0);
	cm_end_trans(key);

	switch (bustype) {
	case CM_BUS_PCI:
		if (regset == UDI_PCI_CONFIG_SPACE) {
			pio->pio_osinfo.cfg_key = key;
			pio->pio_osinfo._u.cfg.offset = offset;
			pio->pio_ops = &_udi_pio_pci_cfg_access;
			pio->address_type = _UDI_PIO_CFG_SPACE;

			return;
		}

		_OSDEP_ASSERT(regset >= 0 && regset < 6);

		/*
		 * We would prefer to get the BAR info from resmgr 
		 * CM_IOADDR/CM_MEMADDR but there's insufficient 
		 * information to figure out which of these values
		 * corresponds to the desired BAR #, so we need to 
		 * go read the registers ourselves. Yuck!
		 */

		cm_read_devconfig16(key, PCI_COMMAND_OFFSET, &command);
		roffset = PCI_BASE_REGISTER_OFFSET + regset * 4;
		cm_read_devconfig32(key, roffset, &base);
		dis_command = command & PCI_COMMAND_DISABLE;
		cm_write_devconfig16(key, PCI_COMMAND_OFFSET, &dis_command);
		if (base == 0)
			bar_length = 0;
		else {
			cm_write_devconfig32(key, roffset, &ff_mask);
			cm_read_devconfig32(key, roffset, &bar_length);
			cm_write_devconfig32(key, roffset, &base);
		}
		cm_write_devconfig16(key, PCI_COMMAND_OFFSET, &command);

		/*
		 * KLUDGE ALERT:
		 * The udi_cmos driver will request a mapping at base 0x70.  
		 * Later, this will be treated as a system bus binding.   
		 * Until that interface is available, we simply flip a 
		 * few bits to trigger the code below to let this be 
		 * treated as a "normal" PCI I/O device.
		 */
		if (base == 0x00) {
			base = 0x71;
			bar_length = ~15;
		}

		if (base & 0x01) {
			/*
			 * I/O space 
			 */
			base &= ~0x03;
			/*
			 * XXX CA_MAKE_EXT_IO_ADDR for ccNUMA 
			 */
			bar_length = (~(bar_length & ~0x03) + 1);

			_OSDEP_ASSERT(offset <= bar_length &&
			       offset + length <= bar_length);
			_udi_set_pio_io_addr(pio, base, offset);
		} else {
			base &= ~0x0F;
			bar_length = (~(bar_length & ~0x0F) + 1);

			_OSDEP_ASSERT(offset <= bar_length &&
			       offset + length <= bar_length);
			_udi_set_pio_mem_addr(pio, base, offset);
		}
		break;
	case CM_BUS_ISA:
		_OSDEP_ASSERT(regset == 0 || regset == 1);
		/*
		 * Find out the IOADDR
		 */
		cm.cm_key = key;
		cm.cm_val = &range;
		cm.cm_vallen = sizeof range;
		cm.cm_n = 0;

		switch (regset) {

		case 0:		/* read CM_IOADDR, call _udi_set_pio_io_addr */
			cm.cm_param = CM_IOADDR;
			break;
		case 1:
			cm.cm_param = CM_MEMADDR;
			break;
		default:
			udi_assert(0);
		}

		cm_begin_trans(cm.cm_key, RM_READ);
		err = cm_getval(&cm);
		cm_end_trans(key);
		_OSDEP_ASSERT(err == 0);

		base = range.startaddr;
		bar_length = range.endaddr - range.startaddr;

		_OSDEP_ASSERT(offset <= bar_length && offset + length <= bar_length);
		if (regset == 0) {
			/*
			 * I/O space 
			 */
			_udi_set_pio_io_addr(pio, base, offset);
		} else {
			/*
			 * memory space 
			 */
			_udi_set_pio_mem_addr(pio, base, offset);
		}
		break;
	default:
		_OSDEP_ASSERT((void *)bustype == "Unknown bustype returned.");
	}
}

void
_udi_release_pio_mapping(_udi_pio_handle_t *pio)
{
	switch (pio->address_type) {
	case _UDI_PIO_MEM_SPACE:

		/*
		 * Though it does not sound logical that physmap_free is
		 * the opposite of physmap64, I'm assured that it is.
		 * So we use this to release the kernel virtual mappings
		 * for this range of addresses.
		 */
		physmap_free(pio->pio_osinfo._u.mem.mapped_addr,
			     pio->length, 0);
		break;
	default:
		break;
	}
}

void
udi_pio_probe(udi_pio_probe_call_t *callback,
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
