
/*
 * File: env/uw/udi_osdep_DMA.c
 *
 * OS-dependent DMA routines for UnixWare 7.
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

#define _DDI 8
#include <udi_env.h>
#include <sys/ddi.h>

/* Warning: proto for non DDI ($interface base) functions follow. */
paddr64_t vtop64(caddr_t vaddr,
		 void *procp);

udi_status_t
_udi_dma_prepare(_udi_dma_handle_t *dmah)
{
	udi_scgth_t *scgth = &dmah->dma_scgth;
	_udi_dma_constraints_t *cons = dmah->dma_constraints;
	udi_ubit32_t v, *attr = cons->attributes;
	physreq_t *preqp;

	/*
	 * TODO: Handle endianness issues. 
	 */

	/*
	 * Convert UDI DMA constraints to DDI 8 physreq_t (for data).
	 */
	preqp = physreq_alloc(KM_SLEEP);
	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_DATA_ADDRESSABLE_BITS);
	if (v > 64)
		v = 64;
	preqp->phys_dmasize = v;
	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_MAX_ELEMENTS);
	if (v > 255)
		v = 255;
	preqp->phys_max_scgth = v;
	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_ELEMENT_ALIGNMENT_BITS);
	if (v > 31)
		return UDI_STAT_NOT_SUPPORTED;
	preqp->phys_align = 1 << v;
	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_ADDR_FIXED_BITS);
	if (v != 0) {
		if (v > 31)
			return UDI_STAT_NOT_SUPPORTED;
		preqp->phys_boundary = 1 << v;
		v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_ADDR_FIXED_TYPE);
		if (v == UDI_DMA_FIXED_VALUE)
			return UDI_STAT_NOT_SUPPORTED;
	}
	if (_UDI_CONSTRAINT_VAL(attr, UDI_DMA_SLOP_IN_BITS) != 0 ||
	    _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SLOP_OUT_BITS) != 0 ||
	    _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SLOP_OUT_EXTRA) != 0) {
		/*
		 * TODO: Add support for these. 
		 */
		return UDI_STAT_NOT_SUPPORTED;
	}

	/*
	 * OS should be doing that but since it doesn't
	 * we have to do it here
	 */
	if (preqp->phys_dmasize == 1) {
		/*
		 * A physically contiguous chunk is needed 
		 */
		preqp->phys_flags |= PREQ_PHYSCONTIG;
	}

	if (!physreq_prep(preqp, KM_SLEEP)) {
		physreq_free(preqp);
		return UDI_STAT_NOT_SUPPORTED;
	}
	dmah->dma_osinfo.physreq = preqp;

	/*
	 * Convert UDI DMA constraints to DDI 8 physreq_t (for scgth list).
	 */
	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_FORMAT);
	if (v & UDI_SCGTH_DMA_MAPPED) {
		preqp = dmah->dma_osinfo.scgth_physreq =
			physreq_alloc(KM_SLEEP);
		v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_ADDRESSABLE_BITS);
		if (v > 64)
			v = 64;
		preqp->phys_dmasize = v;
		v = 1 /* _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_MAX_SEGMENTS) */ ;
		preqp->phys_max_scgth = v;
		/*
		 * FIXME: what about UDI_DMA_SCGTH_MAX_EL_PER_SEG? 
		 */
		v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_ALIGNMENT_BITS);
		if (v > 31)
			return UDI_STAT_NOT_SUPPORTED;
		preqp->phys_align = 1 << v;
		if (!physreq_prep(preqp, KM_SLEEP)) {
			physreq_free(dmah->dma_osinfo.physreq);
			physreq_free(preqp);
			return UDI_STAT_NOT_SUPPORTED;
		}
		/*
		 * The following is only used if visibility is DRIVER_VISIBLE,
		 * so do it here.
		 */
		v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_PREFIX_BYTES);
		dmah->dma_osinfo.scgth_prefix_bytes = v;
	} else
		preqp = NULL;
	dmah->dma_osinfo.scgth_physreq = preqp;

	return UDI_OK;
}

void
_udi_dma_release(_udi_dma_handle_t *dmah)
{
	/*
	 * Free cached scgth memory, if any.
	 */
	_udi_dma_scgth_really_free(dmah);

	/*
	 * If we were cancelled, it looks like this could be NULL 
	 */
	if (dmah->dma_osinfo.physreq != NULL) {
		physreq_free(dmah->dma_osinfo.physreq);
	}

	/*
	 * If we were DMA_SCGTH_DRIVER_VISIBLE, this could be NULL 
	 */
	if (dmah->dma_osinfo.scgth_physreq != NULL) {
		physreq_free(dmah->dma_osinfo.scgth_physreq);
	}
}

void *
_udi_dma_mem_alloc(_udi_dma_handle_t *dmah,
		   udi_size_t size,
		   int flags)
{
	paddr32_t scgth_paddr[2];
	udi_size_t scgth_size;
	void *mem, *scgth_mem;
	udi_scgth_element_32_t *el32p;
	udi_scgth_element_64_t *el64p;
	udi_scgth_t *scgth = &dmah->dma_scgth;
	udi_boolean_t swap_scgth;
	udi_size_t seglen, thispage;
	paddr64_t base, paddr;
	caddr_t vaddr;
	udi_size_t pagesize, pageoffset;

	/*
	 * FIXME - check for the maximum number of elements supported by the
	 * caller. Use kmem_alloc_phys if the caller wants all the memory
	 * as one physical contiguous chunk.
	 */
	mem = kmem_alloc_physreq(size, dmah->dma_osinfo.physreq, KM_SLEEP);
	if (!(flags & UDI_MEM_NOZERO))
		udi_memset(mem, 0, size);

	if (scgth->scgth_format & UDI_SCGTH_32)
		scgth_size = sizeof (udi_scgth_element_32_t);
	else
		scgth_size = sizeof (udi_scgth_element_64_t);
	scgth_size *= btopr(size);	/* worst case scenerio */

	if (scgth_size <= dmah->dma_scgth_allocsize)
		scgth_mem = scgth->scgth_elements.el32p;
	else {
		/*
		 * Free cached scgth memory, if any.
		 */
		if (dmah->dma_scgth_allocsize > 0)
			_udi_dma_scgth_really_free(dmah);

		/*
		 * Allocate new memory, since we need more than we had.
		 */
		if (dmah->dma_osinfo.scgth_physreq != NULL) {
			scgth_mem = kmem_alloc_phys(scgth_size,
						    dmah->dma_osinfo.
						    scgth_physreq, scgth_paddr,
						    0);
		} else
			scgth_mem = kmem_alloc(scgth_size, KM_SLEEP);
		dmah->dma_scgth_allocsize = scgth_size;
	}

	dmah->dma_scgth_size = scgth_size;

	swap_scgth = scgth->scgth_must_swap &&
		!(scgth->scgth_format & UDI_SCGTH_DRIVER_MAPPED);

	/*
	 * Generate a scatter/gather list 
	 */

	if (scgth->scgth_format & UDI_SCGTH_32) {
		scgth->scgth_elements.el32p = el32p =
			(udi_scgth_element_32_t *)scgth_mem;
	} else {
		scgth->scgth_elements.el64p = el64p =
			(udi_scgth_element_64_t *)scgth_mem;
	}

	vaddr = mem;
	scgth->scgth_num_elements = 0;
	pagesize = ptob(1);
	pageoffset = pagesize - 1;
	while (size) {
		base = vtop64(vaddr, NULL);	/* Phys addr of this segment */
		seglen = 0;
		do {
			thispage = min(size, pagesize -
				       ((udi_ubit32_t)vaddr & pageoffset));
			/*
			 * bytes until the end of this page 
			 */
			seglen += thispage;	/* This many more contiguous */
			vaddr += thispage;	/* Bump virtual address */
			size -= thispage;	/* Recompute amount left */
			if (!size)
				break;	/* End of request */
			paddr = vtop64(vaddr, NULL);
			/*
			 * Get next page's address 
			 */
		} while (base + seglen == paddr);

		/*
		 * Now set up the scatter/gather list element 
		 */

		if (scgth->scgth_format & UDI_SCGTH_32) {
			if (swap_scgth) {
				el32p->block_busaddr =
					_OSDEP_ENDIAN_SWAP_32((paddr32_t)
							      base);
				el32p->block_length =
					_OSDEP_ENDIAN_SWAP_32(seglen);
			} else {
				el32p->block_busaddr = (paddr32_t) base;
				el32p->block_length = seglen;
			}
			el32p++;
		} else {
			if (swap_scgth) {
				el64p->block_busaddr.first_32 =
					_OSDEP_ENDIAN_SWAP_32((paddr32_t) (base
									   >>
									   32));
				el64p->block_busaddr.second_32 =
					_OSDEP_ENDIAN_SWAP_32((paddr32_t)
							      base);
				el64p->block_length =
					_OSDEP_ENDIAN_SWAP_32(seglen);
			} else {
				el64p->block_busaddr.first_32 =
					(paddr32_t) base;
				el64p->block_busaddr.second_32 =
					(paddr32_t) (base >> 32);
				el64p->block_length = seglen;
			}
			el64p->el_reserved = 0;

			el64p++;
		}
		scgth->scgth_num_elements++;
	}

	if (scgth->scgth_format & UDI_SCGTH_DMA_MAPPED) {
		if (scgth->scgth_format & UDI_SCGTH_32) {
			el32p = &scgth->scgth_first_segment.el32;
			el32p->block_busaddr = scgth_paddr[0];
			el32p->block_length = sizeof (udi_scgth_element_32_t) *
				scgth->scgth_num_elements;
		} else {
			el64p = &scgth->scgth_first_segment.el64;
			el64p->block_busaddr.first_32 = scgth_paddr[0];
			el64p->block_busaddr.second_32 = scgth_paddr[1];
			el64p->block_length = sizeof (udi_scgth_element_64_t) *
				scgth->scgth_num_elements;
			el64p->el_reserved = 0;
		}
	}

	return mem;
}

void
_udi_dma_scgth_free(_udi_dma_handle_t *dmah)
{
	/*
	 * Keep it around for reuse... 
	 */
}

void
_udi_dma_scgth_really_free(_udi_dma_handle_t *dmah)
{
	udi_scgth_t *scgth = &dmah->dma_scgth;
	udi_size_t scgth_size = dmah->dma_scgth_allocsize;

	if (scgth_size > 0) {
		kmem_free(scgth->scgth_elements.el32p, scgth_size);
		dmah->dma_scgth_size = 0;
		dmah->dma_scgth_allocsize = 0;
	}
}

/*
 * _OSDEP_DMA_BUF_MAP is responsible for:
 *	1) Ensuring that the buffer memory meets DMA constraints.
 *		- segment or allocate/copy as needed
 *	2) Mapping the resulting data memory into DMA space.
 *	3) Building a scatter/gather list.
 *	4) Mapping the scatter/gather list as required by
 *		UDI_DMA_SCGTH_FORMAT.
 *	5) Filling in dmah->dma_scgth.
 */
udi_boolean_t
_udi_dma_buf_map(struct _udi_dma_handle *dmah,
		 _udi_alloc_marshal_t *allocm,
		 int waitflag)
{
	void *copy_mem;

	switch (_udi_dma_use_inplace(dmah, waitflag)) {
	case _UDI_DMA_CANT_WAIT:
		return FALSE;
	case _UDI_DMA_CANT_USE_INPLACE:
		if (waitflag != UDI_WAITOK)
			return FALSE;
		copy_mem = _udi_dma_mem_alloc(dmah, dmah->dma_maplen,
					      UDI_MEM_NOZERO);
		dmah->dma_osinfo.copy_mem = copy_mem;

		if (dmah->dma_buf_flags & UDI_DMA_OUT) {
			udi_buf_read(&dmah->dma_buf->buf, dmah->dma_buf_base,
				     dmah->dma_maplen, copy_mem);
		}
	}

	allocm->_u.dma_buf_map_result.complete = TRUE;
	allocm->_u.dma_buf_map_result.status = UDI_OK;
	allocm->_u.dma_buf_map_request.state = 255;	/* all done */

#ifdef NOTYET
	switch (allocm->_u.dma_buf_map_request.state) {
	case 0:			/* Get the ball rolling... */
	default:
		_OSDEP_ASSERT(0);
	}
#endif /* NOTYET */

	return TRUE;
}

void
_udi_dma_buf_map_discard(_udi_alloc_marshal_t *allocm)
{
	/*
	 * XXX - Does this really need to be OSDEP? 
	 */
}

void
_udi_kmem_free(void *context,
	       udi_ubit8_t *space,
	       udi_size_t size)
{
	kmem_free(space, size);
}

STATIC _udi_xbops_t dma_xbops = { &_udi_kmem_free, NULL, NULL };

void
_udi_dma_buf_unmap(struct _udi_dma_handle *dmah)
{
	void *copy_mem;
	udi_size_t orig_size;
	udi_boolean_t did_sync;

	if (!dmah->dma_osinfo.copy_mem)
		return;

	orig_size = dmah->dma_buf->buf.buf_size;
	did_sync = 0;
	copy_mem = dmah->dma_osinfo.copy_mem;

	if (dmah->dma_buf_flags & UDI_DMA_IN) {
		_udi_buf_write_sync(copy_mem, dmah->dma_maplen,
				    dmah->dma_maplen, dmah->dma_buf,
				    &(dmah->dma_buf_base),
				    &(dmah->dma_buf_offset), &dma_xbops, NULL);
		did_sync = 1;
	}
	if (dmah->dma_buf_flags & UDI_DMA_OUT && !did_sync) {
		kmem_free(copy_mem, dmah->dma_maplen);
	}
	dmah->dma_buf->buf.buf_size = orig_size;

	dmah->dma_osinfo.copy_mem = NULL;
}
