
/*
 * File: env/uw/udi_osdep_dma2.c
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


/*
 * Entire file conditionally compiled away.
 */
#if HAVE_USE_INPLACE
#define _DDI 8
#include <udi_env.h>
#include <sys/ddi.h>

#ifdef DEBUG
#include <sys/vmparam.h>
#else
#define KADDR(v) 1
#endif

/* Warning: proto for non DDI ($interface base) functions follow. */
paddr64_t vtop64(caddr_t vaddr,
		 void *procp);

#ifndef INPLACE_STATS
#define INPLACE_STATS	1
#endif

struct __udi_dma_buf_map_stats {
	_OSDEP_ATOMIC_INT_T buf_map_called_ntimes;
	_OSDEP_ATOMIC_INT_T data_duplicated;
	_OSDEP_ATOMIC_INT_T zerobytes_buffer;
	_OSDEP_ATOMIC_INT_T unk_data_addressable_bits;
	_OSDEP_ATOMIC_INT_T nonzero_prefix;
	_OSDEP_ATOMIC_INT_T eltlen_lt_pgsz;
	_OSDEP_ATOMIC_INT_T nonzero_fixed_bits;
	_OSDEP_ATOMIC_INT_T data_alignment;
	_OSDEP_ATOMIC_INT_T too_many_elts_per_seg;
} _udi_dma_buf_map_stats;

#if INPLACE_STATS
#define CANT_USE_INPLACE(why) ( \
	_OSDEP_ATOMIC_INT_INCR(&_udi_dma_buf_map_stats.data_duplicated), \
	_OSDEP_ATOMIC_INT_INCR(&_udi_dma_buf_map_stats.why), \
	_UDI_DMA_CANT_USE_INPLACE \
)
#else  /* INPLACE_STATS */
#define CANT_USE_INPLACE(why) _UDI_DMA_CANT_USE_INPLACE
#endif /* INPLACE_STATS */

STATIC void
_udi_dma_scgth_add_element(udi_boolean_t scgth_is32,
			   udi_boolean_t scgth_swap,
			   udi_scgth_element_32_t **el32pp,
			   udi_scgth_element_64_t **el64pp,
			   udi_ubit32_t *scgth_nelts,
			   udi_ubit32_t scgth_maxnelts,
			   paddr64_t pabase,
			   udi_ubit32_t pc_nbytes)
{
	_OSDEP_ASSERT(*scgth_nelts < scgth_maxnelts);
	(*scgth_nelts)++;

	if (scgth_is32) {
		if (scgth_swap) {
			(*el32pp)->block_busaddr =
				_OSDEP_ENDIAN_SWAP_32((paddr32_t) pabase);
			(*el32pp)->block_length =
				_OSDEP_ENDIAN_SWAP_32(pc_nbytes);
		} else {
			(*el32pp)->block_busaddr = (paddr32_t) pabase;
			(*el32pp)->block_length = pc_nbytes;
		}
		(*el32pp)++;
		return;
	}

	/*
	 * 64 bit format for the scatter/gather list 
	 */
	if (scgth_swap) {
		(*el64pp)->block_busaddr.first_32 =
			_OSDEP_ENDIAN_SWAP_32((paddr32_t) (pabase >> 32));
		(*el64pp)->block_busaddr.second_32 =
			_OSDEP_ENDIAN_SWAP_32((paddr32_t) pabase);
		(*el64pp)->block_length = _OSDEP_ENDIAN_SWAP_32(pc_nbytes);
	} else {
		(*el64pp)->block_busaddr.first_32 = (paddr32_t) pabase;
		(*el64pp)->block_busaddr.second_32 =
			(paddr32_t) (pabase >> 32);
		(*el64pp)->block_length = pc_nbytes;
	}
	(*el64pp)->el_reserved = 0;

	(*el64pp)++;

}

int
_udi_dma_use_inplace(struct _udi_dma_handle *dmah,
		     int waitflag)
{
	_udi_dma_constraints_t *cons = dmah->dma_constraints;
	udi_ubit32_t *attr = cons->attributes;

#define data_addressable_bits	_UDI_CONSTRAINT_VAL(attr, UDI_DMA_DATA_ADDRESSABLE_BITS)
#define scgth_max_elements	_UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_MAX_ELEMENTS)
#define scgth_addressable_bits	_UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_ADDRESSABLE_BITS)
#define scgth_max_segments	_UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_MAX_SEGMENTS)
#define scgth_alignment_bits	_UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_ALIGNMENT_BITS)
#define scgth_max_el_per_seg	_UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_MAX_EL_PER_SEG)
#define scgth_prefix_bytes	_UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_PREFIX_BYTES)
#define element_alignment_bits	_UDI_CONSTRAINT_VAL(attr, UDI_DMA_ELEMENT_ALIGNMENT_BITS)
#define element_length_bits	_UDI_CONSTRAINT_VAL(attr, UDI_DMA_ELEMENT_LENGTH_BITS)
#define element_granularity_bits _UDI_CONSTRAINT_VAL(attr, UDI_DMA_ELEMENT_GRANULARITY_BITS)
#define addr_fixed_bits		_UDI_CONSTRAINT_VAL(attr, UDI_DMA_ADDR_FIXED_BITS)
#define addr_fixed_type		_UDI_CONSTRAINT_VAL(attr, UDI_DMA_ADDR_FIXED_TYPE)
#define addr_fixed_value_lo	_UDI_CONSTRAINT_VAL(attr, UDI_DMA_ADDR_FIXED_VALUE_LO)
#define addr_fixed_value_hi	_UDI_CONSTRAINT_VAL(attr, UDI_DMA_ADDR_FIXED_VALUE_HI)

	physreq_t *prp;
	udi_scgth_t *scgth;
	_udi_buf_t *bufp;
	_udi_buf_dataseg_t *segp;
	udi_ubit32_t nsegs, scgth_nelts, scgth_maxnelts;
	udi_size_t scgth_nbytes, elt_max_nbytes;
	udi_scgth_element_32_t *el32p;
	udi_scgth_element_64_t *el64p;
	void *scgth_mem;
	udi_boolean_t scgth_swap, scgth_is32;
	udi_ubit32_t curseg_nbytes, togo_nbytes, bytes_this_iter, pc_nbytes;
	const udi_ubit32_t pagesize = ptob(1);
	const udi_ubit32_t pageoffset = pagesize - 1;
	const udi_ubit32_t pagenumber = ~pageoffset;
	udi_ubit32_t alignment_mask;

	caddr_t vacur;
	udi_ubit32_t prev_vpage;
	paddr64_t pacur, pa_elt_start, prev_ppage;

#if INPLACE_STATS
	_OSDEP_ATOMIC_INT_INCR(&_udi_dma_buf_map_stats.buf_map_called_ntimes);
#endif

	bufp = (_udi_buf_t *)&dmah->dma_buf->buf;
	if (bufp->buf_total_size == 0)
		return CANT_USE_INPLACE(zerobytes_buffer);
	if (data_addressable_bits != 32 && data_addressable_bits != 64)
		return CANT_USE_INPLACE(unk_data_addressable_bits);

	/*
	 * UDI_DMA_NO_PARTIAL: IGNORED 
	 */

	/*
	 * IGNORED. see UDI_DMA_MAX_ELPER_SEG below. 
	 */

	/*
	 * UDI_DMA_SCGTH_VISIBILITY: IGNORED 
	 */
	/*
	 * UDI_DMA_SCGTH_ENDIANNESS: IGNORED 
	 */

	/*
	 * UDI_DMA_SCGTH_ADDRESSABLE_BITS: IGNORED.
	 * Assumed met because of dmah->dma_osinfo.scgth_physreq 
	 */

	/*
	 * UDI_DMA_SCGTH_MAX_SEGMENTS: IGNORED. We will not generate more than
	 * one segment 
	 */

	/*
	 * UDI_DMA_SCGTH_ALIGNMENT_BITS: IGNORED.
	 * Assumed to be met because of the memory for the scgth list is
	 * kmem_alloc_phys with a physreq_t of dmah->dma_osinfo.scgth_physreq 
	 */

	/*
	 * UDI_DMA_MAX_EL_PER_SEG: CHECKED.
	 * In our case (just one seg. supported), max elts per seg should be
	 * the same as UDI_DMA_SCGTH_MAX_ELEMENTS. Enforced below through
	 * prp->phys_max_scgth. 
	 */

	/*
	 * If UDI_DMA_SCGTH_PREFIX_BYTES is NZ, don't try to use the UDI
	 * buffer in place. 
	 */
	if (scgth_prefix_bytes)
		return CANT_USE_INPLACE(nonzero_prefix);

	/*
	 * UDI_DMA_ELEMENT_ALIGNMENT_BITS is enforced below through
	 * prp->phys_align. 
	 */

	/*
	 * UDI_DMA_ELEMENT_LENGTH_BITS: maximum length of a scatter/gather
	 * element. How come I don't see an equivalent of this in the physreq
	 * structure? 0 means no restriction. Otherwise we should cap DMA
	 * elements' size to (1 << element_length_bits). 
	 */
	if (element_length_bits) {
		elt_max_nbytes = (1 << element_length_bits);
		if (elt_max_nbytes < pagesize)
			return CANT_USE_INPLACE(eltlen_lt_pgsz);
	} else
		elt_max_nbytes = 0;	/* No limit */

	/*
	 * UDI_DMA_ELEMENT_GRANULARITY_BITS: IGNORED. Later? 
	 */

	if (addr_fixed_bits)		/* See also assertion on phys_boundary below */
		return CANT_USE_INPLACE(nonzero_fixed_bits);

	/*
	 * Now to take care of physreq object related to the data buffer 
	 */
	prp = dmah->dma_osinfo.physreq;

	_OSDEP_ASSERT(dmah->dma_buf_base == 0);	/* XXX legitimate ? */
	_OSDEP_ASSERT(prp->phys_align == (1 << element_alignment_bits));
	_OSDEP_ASSERT(prp->phys_boundary == 0);
	_OSDEP_ASSERT(prp->phys_dmasize == data_addressable_bits);
	_OSDEP_ASSERT((prp->phys_flags & PREQ_PHYSCONTIG) == 0);

	/*
	 * If the following is false, a per page breakup is not possible 
	 */
	_OSDEP_ASSERT(prp->phys_align <= pagesize);
	alignment_mask = prp->phys_align - 1;

	/*
	 * First pass: evaluate the number of scatter/gather
	 * elements: nelts <= nsegs + sum(btopr(size(dataseg(i)))).
	 * This does not even assume that the beginning of each segment
	 * is page aligned. 
	 */
	segp = (_udi_buf_dataseg_t *)bufp->buf_contents;
	for (nsegs = scgth_maxnelts = 0; segp; nsegs++, segp = segp->bd_next)
		scgth_maxnelts +=
			((((udi_ubit32_t)segp->bd_start) & pageoffset) != 0)
			+ btopr(segp->bd_end - segp->bd_start);

	scgth = &dmah->dma_scgth;
	scgth_swap = scgth->scgth_must_swap &&
		!(scgth->scgth_format & UDI_SCGTH_DRIVER_MAPPED);
	if (scgth->scgth_format & UDI_SCGTH_32) {
		scgth_nbytes =
			scgth_maxnelts * sizeof (udi_scgth_element_32_t);
		scgth_is32 = TRUE;
	} else {
		scgth_nbytes =
			scgth_maxnelts * sizeof (udi_scgth_element_64_t);
		scgth_is32 = FALSE;
	}

	/*
	 * Allocate memory for the scatter/gather list itself 
	 */
	if (scgth_nbytes <= dmah->dma_scgth_allocsize)
		scgth_mem = scgth->scgth_elements.el32p;
	else {
		if (waitflag != UDI_WAITOK)
			return _UDI_DMA_CANT_WAIT;
		if (dmah->dma_scgth_allocsize > 0)
			_udi_dma_scgth_really_free(dmah);
		if (dmah->dma_osinfo.scgth_physreq != NULL) {
			scgth_mem = kmem_alloc_phys(scgth_nbytes,
						    dmah->dma_osinfo.
						    scgth_physreq,
						    (paddr32_t *) & scgth->
						    scgth_first_segment.el32.
						    block_busaddr, 0);
		} else
			scgth_mem = kmem_alloc(scgth_nbytes, KM_SLEEP);
		dmah->dma_scgth_allocsize = scgth_nbytes;
	}
	dmah->dma_scgth_size = scgth_nbytes;

	/*
	 * Initialize the pointers to the beginning of the scgth list 
	 */
	if (scgth_is32)
		scgth->scgth_elements.el32p = el32p =
			(udi_scgth_element_32_t *)scgth_mem;
	else
		scgth->scgth_elements.el64p = el64p =
			(udi_scgth_element_64_t *)scgth_mem;

	/*
	 * Actually build the scatter/gather list 
	 */
	scgth_nelts = 0;
	segp = (_udi_buf_dataseg_t *)bufp->buf_contents;
	prev_vpage = (udi_ubit32_t)~0;
	pc_nbytes = 0;
	togo_nbytes = bufp->buf_total_size;
	while (togo_nbytes) {
		curseg_nbytes = segp->bd_end - segp->bd_start;
		togo_nbytes -= curseg_nbytes;

		vacur = (caddr_t) segp->bd_start;
		while (curseg_nbytes) {
			_OSDEP_ASSERT(KADDR(vacur));
			if ((((udi_ubit32_t)vacur) & pagenumber) != prev_vpage) {
				pacur = vtop64(vacur, NULL);
				prev_ppage = pacur & pagenumber;
				prev_vpage =
					((udi_ubit32_t)vacur) & pagenumber;
			} else
				pacur = prev_ppage |
					(((udi_ubit32_t)vacur) & pageoffset);

			if (!pc_nbytes) {
				/*
				 * Physically contiguous area starts here 
				 */
				if (((udi_ubit32_t)vacur) & alignment_mask)
					return CANT_USE_INPLACE
						(data_alignment);
				pa_elt_start = pacur;
			}

			bytes_this_iter = min(curseg_nbytes,
					      pagesize -
					      (((udi_ubit32_t)vacur) &
					       pageoffset));

			/*
			 * If needed, cap the DMA element's size. 
			 */
			if (elt_max_nbytes)
				bytes_this_iter = min(bytes_this_iter,
						      elt_max_nbytes);

			if (pc_nbytes && (pacur != pa_elt_start + pc_nbytes)) {
				_udi_dma_scgth_add_element(scgth_is32,
							   scgth_swap, &el32p,
							   &el64p,
							   &scgth_nelts,
							   scgth_maxnelts,
							   pa_elt_start,
							   pc_nbytes);
				if (((udi_ubit32_t)vacur) & alignment_mask)
					return CANT_USE_INPLACE
						(data_alignment);
				pa_elt_start = pacur;
				pc_nbytes = bytes_this_iter;
			} else
				pc_nbytes += bytes_this_iter;

			vacur += bytes_this_iter;
			curseg_nbytes -= bytes_this_iter;
		}

		segp = segp->bd_next;
	}

	if (pc_nbytes)
		_udi_dma_scgth_add_element(scgth_is32, scgth_swap, &el32p,
					   &el64p, &scgth_nelts,
					   scgth_maxnelts, pa_elt_start,
					   pc_nbytes);

	if (prp->phys_max_scgth && (scgth_nelts > prp->phys_max_scgth))
		return CANT_USE_INPLACE(too_many_elts_per_seg);

	scgth->scgth_num_elements = scgth_nelts;
	dmah->dma_osinfo.copy_mem = NULL;

	if (scgth->scgth_format & UDI_SCGTH_DMA_MAPPED) {
		if (scgth_is32) {
			el32p = &scgth->scgth_first_segment.el32;
			el32p->block_length = sizeof (udi_scgth_element_32_t) *
				scgth->scgth_num_elements;
		} else {
			el64p = &scgth->scgth_first_segment.el64;
			el64p->block_length = sizeof (udi_scgth_element_64_t) *
				scgth->scgth_num_elements;
			el64p->el_reserved = 0;
		}
	}

	return 0;
}

#endif /* HAVE_USE_INPLACE */
