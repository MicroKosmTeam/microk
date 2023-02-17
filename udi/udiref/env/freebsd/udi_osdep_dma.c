/*
 * File: env/uw/udi_osdep_DMA.c
 *
 * FreeBSD DMA code.
 * 
 * This is flagrantly ripped off from an ancient version of the UW code
 * to serve as a simple tool for bringup.   The use of contigmalloc is 
 * known to be evil.   Basically this whole file needs replaced by someone
 * that understands FreeBSD DMA.
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

#include <vm/vm.h>
#include <vm/pmap.h>
#include <machine/pmap.h>
/* 
 * FIXME: Surely BSD provides something like these, but I can't find them.
 */
#define btopr(i) \
        ( ( (i) & (PAGE_SIZE-1) ) ? ( ( (i) >> PAGE_SHIFT ) + 1 ) : ( (i) >> PAGE_SHIFT) )
#define ptob(i) \
	((i)*PAGE_SIZE)


udi_status_t _udi_dma_prepare(_udi_dma_handle_t * dmah)
{
#if 0
	udi_scgth_t *scgth = &dmah->dma_scgth;
#endif
	_udi_dma_constraints_t *cons = dmah->dma_constraints;
	udi_ubit32_t v, *attr = cons->attributes;

	/*
	 * TODO: Handle endianness issues. 
	 */

	/*
	 * Remember the scatter/gather visibility requirements.
	 */
#if 0
	dmah->dma_osinfo.scgth_vis =
		_UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_VISIBILITY);
#endif

	/*
	 * Get the number of addressable bits for the dma elements.
	 */
	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_DATA_ADDRESSABLE_BITS);
	if (v >= (8 * sizeof(void *))) {
		/* 
		 * No boundary; can be addressed anywhere.
	 	 */
		dmah->dma_osinfo.boundary = 0;
	} else {
		printf("Warning: only %d addressable bits\n", v);
		/* XXX: I think this is wrong */
		dmah->dma_osinfo.boundary = 1 << v;
	}

	/*
 	 * Get required alignment.
	 */
	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_ALIGNMENT_BITS);
	if (v > (8 * sizeof(void *)))  {
		return UDI_STAT_NOT_SUPPORTED;
	}
	dmah->dma_osinfo.alignment = 1 << v;

	/* 
	 * FIXME: Should decode DME_ADDR_FIXED_BITS for these.
	 */
	dmah->dma_osinfo.low = 0;
	dmah->dma_osinfo.high = -1;

#if 0
	/*
	 * Convert UDI DMA constraints to DDI 8 physreq_t (for data).
	 */
	preqp = physreq_alloc(KM_SLEEP);
	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_DATA_ADDRESSABLE_BITS);
	if (v > 64)
		v = 64;
	preqp->phys_dmasize = v;
	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_MAX_ELEMENTS);
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
	 * * we have to do it here
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
	if (dmah->dma_osinfo.scgth_vis != UDI_DMA_SCGTH_DRIVER_VISIBLE) {
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

	/*
	 * TODO: Move this scgth setup to common code. 
	 */

	if (dmah->dma_osinfo.physreq->phys_dmasize <= 32)
		scgth->scgth_format = UDI_SCGTH_32;
	else
		scgth->scgth_format = UDI_SCGTH_64;
	scgth->scgth_format |= dmah->dma_osinfo.scgth_vis << 6;

	if (scgth->scgth_format & UDI_SCGTH_DMA_MAPPED) {
		v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_ENDIANNESS);
		_OSDEP_ASSERT(v == UDI_DMA_LITTLE_ENDIAN ||
			      v == UDI_DMA_BIG_ENDIAN);
		/*
		 * TODO: Get bridge driver's input on endianness swapping. 
		 */
#if _OSDEP_BIG_ENDIAN
#error freebsd is little endian
		scgth->scgth_must_swap = (v == UDI_DMA_LITTLE_ENDIAN);
#else
		scgth->scgth_must_swap = (v == UDI_DMA_BIG_ENDIAN);
#endif
	} else
		scgth->scgth_must_swap = FALSE;
#else


#if 0
	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_DATA_ADDRESSABLE_BITS);
	dmah->dma_osinfo.dmasize = v;

	dmah->dma_scgth.scgth_elements.el32p = NULL;
	dmah->dma_scgth.scgth_elements.el64p = NULL;
#endif

	return UDI_OK;
#endif
}

void
_udi_dma_release(_udi_dma_handle_t * dmah)
{
#if 0
	printf("_udi_dma_release");
	_OSDEP_DEBUG_BREAK;
#endif
#if 0
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
#endif
}

void *
_udi_dma_mem_alloc(_udi_dma_handle_t * dmah,
		   udi_size_t size,
		   int flags)
{
	unsigned long scgth_paddr;
	udi_size_t scgth_size;
	void *mem, *scgth_mem;
	udi_scgth_element_32_t *el32p;
	udi_scgth_element_64_t *el64p;
	udi_scgth_t *scgth = &dmah->dma_scgth;
	udi_boolean_t swap_scgth;
	udi_size_t seglen, thispage;
	unsigned long base, paddr;
	caddr_t vaddr;
	udi_size_t pagesize, pageoffset;
	_osdep_dma_t *osinfo = &dmah->dma_osinfo;

	/*
	 * FIXME - I don't see a way to get a sglist in FreeBSD nor do I see
	 * an interface for 36-bit (PAE) mode on 32-bit hardware.  So for now
	 * (and to keep things simple) we just allocate things as one physical
	 * chunk.
	 */
	dmah->dma_osinfo.copy_mem_size = size;

	/*
	 * FIXME FIXME FIXME
	 * So much for simplicity.   contigmalloc is broken.  Actually, it
	 * seems to be contigfree that's broken.   Consensus on freebsd-
	 * hackers is so strong against its use that it doesn't look like
	 * it's going to be fixed.   So this code should get scrapped,
	 * replaced with something approximately like the caching UnixWare
	 * dma code, and contig*'s use replaced by bus_dma_tag_create and 
	 * bus_dmamem_alloc. 
 	 */
	mem = contigmalloc(size, M_DEVBUF, M_WAITOK,
			dmah->dma_osinfo.low, dmah->dma_osinfo.high,
			osinfo->alignment, osinfo->boundary);

#if 0
	_OSDEP_ASSERT(mem);
#else
	if (!mem) {
		printf("failed contigmalloc");
		_OSDEP_DEBUG_BREAK;
	}
#endif
	if (!(flags & UDI_MEM_NOZERO))
		udi_memset(mem, 0, size);

	if (scgth->scgth_format & UDI_SCGTH_32)
		scgth_size = sizeof (udi_scgth_element_32_t);
	else
		scgth_size = sizeof (udi_scgth_element_64_t);
	scgth_size *= btopr(size);	/* worst case scenerio */

#if 0
	if (dmah->dma_osinfo.scgth_physreq != NULL) {
		scgth_mem = kmem_alloc_phys(scgth_size,
					    dmah->dma_osinfo.scgth_physreq,
					    scgth_paddr, 0);
	} else {
		scgth_mem = kmem_alloc(scgth_size, KM_SLEEP);
	}
#endif
	scgth_mem = _OSDEP_MEM_ALLOC(scgth_size, 0, UDI_WAITOK);
	scgth_paddr = vtophys(scgth_mem);

	dmah->dma_scgth_size = scgth_size;

	swap_scgth = scgth->scgth_must_swap &&
		!(scgth->scgth_format & UDI_SCGTH_DRIVER_MAPPED);

	/*
	 * Generate a scatter/gather list 
	 */

	if (scgth->scgth_format & UDI_SCGTH_32) {
		scgth->scgth_elements.el32p = el32p =
			(udi_scgth_element_32_t *) scgth_mem;
	} else {
		scgth->scgth_elements.el64p = el64p =
			(udi_scgth_element_64_t *) scgth_mem;
	}
	vaddr = mem;
	scgth->scgth_num_elements = 0;
	pagesize = ptob(1);
	pageoffset = pagesize - 1;
	while (size) {
		base = vtophys(vaddr);	/* Phys addr of this segment */
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
			paddr = vtophys(vaddr);
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
					_OSDEP_ENDIAN_SWAP_32((udi_ubit32_t)
							      base);
				el32p->block_length =
					_OSDEP_ENDIAN_SWAP_32(seglen);
			} else {
				el32p->block_busaddr = (udi_ubit32_t) base;
				el32p->block_length = seglen;
			}
			el32p++;
		} else {
#if 0
			printf("64 bit lists not supported\n");
			_OSDEP_DEBUG_BREAK;
			_OSDEP_ASSERT(0);
#else
			if (swap_scgth) {
#if 0
				el64p->block_busaddr.first_32 =
					_OSDEP_ENDIAN_SWAP_32(
					      (udi_ubit32_t) (base >> 32));
#else
				el64p->block_busaddr.first_32 = 0;
#endif
				el64p->block_busaddr.second_32 =
					_OSDEP_ENDIAN_SWAP_32((udi_ubit32_t)
							      base);
				el64p->block_length =
					_OSDEP_ENDIAN_SWAP_32(seglen);
			} else {
				el64p->block_busaddr.first_32 =
					(udi_ubit32_t) base;
#if 0
				el64p->block_busaddr.second_32 =
					(paddr32_t) (base >> 32);
#else
				/* XXX: 32 bit physical addresses on x86 */
				udi_assert(sizeof(long) == 4);
				el64p->block_busaddr.second_32 = 0;
#endif
				el64p->block_length = seglen;
			}
			el64p->el_reserved = 0;

			el64p++;
		}
		scgth->scgth_num_elements++;
#endif
	}

	if (scgth->scgth_format & UDI_SCGTH_DMA_MAPPED) {
		if (scgth->scgth_format & UDI_SCGTH_32) {
			el32p = &scgth->scgth_first_segment.el32;
			el32p->block_busaddr = scgth_paddr;
			el32p->block_length = sizeof (udi_scgth_element_32_t) *
				scgth->scgth_num_elements;
#if 0
	printf("DMA_MAPPED: block_busaddr %x\n", el32p->block_busaddr);
	printf("DMA_MAPPED: block_length %x\n", el32p->block_busaddr);
#endif
		} else {
			el64p = &scgth->scgth_first_segment.el64;
			el64p->block_busaddr.first_32 = scgth_paddr;
			el64p->block_busaddr.second_32 = 0;
			el64p->block_length = sizeof (udi_scgth_element_64_t) *
				scgth->scgth_num_elements;
			el64p->el_reserved = 0;
		}
	}

	return mem;
}

void
_udi_dma_scgth_free(_udi_dma_handle_t * dmah)
{
	udi_scgth_t *scgth = &dmah->dma_scgth;
#if 0
	udi_size_t scgth_size = dmah->dma_scgth_size;
	printf("dma_scgth_free");
	_OSDEP_DEBUG_BREAK;

	kmem_free(scgth->scgth_elements.el32p, scgth_size);
#endif
	_OSDEP_MEM_FREE(scgth->scgth_elements.el32p);
	dmah->dma_scgth_size = 0;
}

/*
 * _OSDEP_DMA_BUF_MAP is responsible for:
 *	1) Ensuring that the buffer memory meets DMA constraints.
 *		- segment or allocate/copy as needed
 *	2) Mapping the resulting data memory into DMA space.
 *	3) Building a scatter/gather list.
 *	4) Mapping the scatter/gather list as required by
 *		UDI_DMA_SCGTH_VISIBILITY.
 *	5) Filling in dmah->dma_scgth.
 */
udi_boolean_t
_udi_dma_buf_map(struct _udi_dma_handle *dmah,
		 _udi_alloc_marshal_t * allocm,
		 int waitflag)
{
	void *copy_mem;

	/*
	 * PERF TODO: Should check if some or all of the buffer memory
	 * is usable as is, and should also try to do memory allocations
	 * without blocking if waitflag is UDI_NOWAIT.
	 *
	 * For now, though, take the easy way out, and always copy to/from
	 * newly allocated memory, using a blocking allocation.
	 */
	if (waitflag != UDI_WAITOK)
		return FALSE;

	copy_mem = _udi_dma_mem_alloc(dmah, dmah->dma_maplen, UDI_MEM_NOZERO);
	dmah->dma_osinfo.copy_mem = copy_mem;

	if (dmah->dma_buf_flags & UDI_DMA_OUT) {
		udi_buf_read(&dmah->dma_buf->buf, dmah->dma_buf_base,
			     dmah->dma_maplen, copy_mem);
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
_udi_kmem_free(void *context,
	       udi_ubit8_t *space,
	       udi_size_t size)
{
	printf("_udi_kmem_free");
	_OSDEP_DEBUG_BREAK;
#if 0
	kmem_free(space, size);
#endif
}

_udi_xbops_t dma_xbops = { &_udi_kmem_free, NULL, NULL };

void
_udi_dma_buf_unmap(struct _udi_dma_handle *dmah)
{
	void *copy_mem;
	udi_size_t orig_size = dmah->dma_buf->buf.buf_size;
	udi_boolean_t did_sync = 0;
#if 0
	printf("_udi_dma_buf_unmap %p", dmah);
	_OSDEP_DEBUG_BREAK;
#endif

	/*
	 * (See PERF TODO comment in _udi_dma_buf_map.) 
	 */

	copy_mem = dmah->dma_osinfo.copy_mem;

	if (dmah->dma_buf_flags & UDI_DMA_IN) {
		_udi_buf_write_sync(copy_mem, dmah->dma_maplen, dmah->dma_maplen,
				    dmah->dma_buf, &(dmah->dma_buf_base),
				    &(dmah->dma_buf_offset), &dma_xbops, NULL);
		did_sync = 1;
	}
	if (dmah->dma_buf_flags & UDI_DMA_OUT && !did_sync) {
		contigfree(copy_mem, dmah->dma_osinfo.copy_mem_size,M_DEVBUF);
	}
	dmah->dma_buf->buf.buf_size = orig_size;

	dmah->dma_osinfo.copy_mem = NULL;
}
