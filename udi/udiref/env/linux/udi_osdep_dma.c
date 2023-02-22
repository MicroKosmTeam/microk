/*
 * File: env/linux/udi_osdep_DMA.c
 *
 * OS-dependent DMA routines for Linux (Kernel 2.2+)
 */

#if 0
/*
 * These flags control debug information output from the DMA code.
 * Enabling these flags may incur severe performance penalties.
 */
#define DMA_NOISY
#define _DMA_NOISY
#endif

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

#include "udi_osdep_opaquedefs.h"
#include <udi_env.h>
#include <asm/io.h>


#ifdef INSTRUMENT_MALLOC
extern unsigned long no_kmallocs, no_kfrees;
#endif

/* Which header does this belong in? */
extern void
 _udi_buf_write_sync(void *copy_mem, udi_size_t maplen, udi_size_t len,
		     _udi_buf_t *buf, udi_size_t *base, udi_size_t *offset,
		     _udi_xbops_t *xbops, void *context);

#if 0
#define pages_to_bytes(i) /*ptob*/ \
        ( (i) * PAGE_SIZE )
#endif

#define bytes_to_pages_round_up(i) \
        ( ( ( (i) & (PAGE_SIZE-1) ) != 0 ) + ( ( (i) >> PAGE_SHIFT ) ) )

#define min(i,j) \
        ( ((i) < (j)) ? (i) : (j))

/*
 * Get the order (bit size) for a given number of pages.
 * With Linux 2.2, you can only allocate MAX_ORDER=5 (32) pages at a time.
 * With Linux 2.4, you can allocate MAX_ORDER=10 pages at a time.
 * On PowerPC, try using 32-cntlzw(pages) instead.
 */
const udi_ubit8_t page_to_order[] =
    { 0, /* 1 page */
      1, /* 2 pages */
      2, 2, /* 3-4 pages */
      3, 3, 3, 3, /* 5-8 pages */
      4, 4, 4, 4, 4, 4, 4, 4, /* 9-16 pages */
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, /* MAX_ORDER=5 (2.2) */
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,

      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,

      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,

      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    };



typedef enum { MID_DATA = 0, LAST_DATA, SCGTH_MEM } _osdep_dma_mem_t;

STATIC kmem_cache_t *osdep_dma_alloc;
STATIC _OSDEP_SIMPLE_MUTEX_T osdep_dma_cache_lock;
STATIC udi_queue_t osdep_dma_cache;

/* Chop off all low-bits up to where we align. */
#define ALIGN_TO(addr, alignment) ((addr) & ~((alignment)-1))

/* How far away are we from being aligned? ALIGN_OFFSET==0 if aligned. */
#define ALIGN_OFFSET(addr, alignment) ((addr) & ((alignment)-1))

void
osdep_dma_init_cache()
{
	UDI_QUEUE_INIT(&osdep_dma_cache);
	_OSDEP_SIMPLE_MUTEX_INIT(&osdep_dma_cache_lock, "dma cache lock");
	osdep_dma_alloc = kmem_cache_create("udi_dma_cache",
					 sizeof(_osdep_dma_alloc_t), 0, 0, 
					 NULL, NULL);
}

/*
 * free all non-used parts of the cache
 */
void
osdep_dma_dispose_of_cache()
{
	_osdep_dma_alloc_t *alloc;
	while (!UDI_QUEUE_EMPTY(&osdep_dma_cache)) {
		alloc = UDI_BASE_STRUCT(UDI_DEQUEUE_HEAD(&osdep_dma_cache),
				_osdep_dma_alloc_t, link);
#ifdef __DMA_NOISY
		_OSDEP_PRINTF("udi: freeing a DMA allocation %p (mem %p) that"
				" was never unmapped or dma_free'd.\n",
				alloc, alloc->dma_mem);
#endif
		kfree(alloc->dma_mem);
		kmem_cache_free(osdep_dma_alloc, alloc);
	}
	kmem_cache_destroy(osdep_dma_alloc);
	osdep_dma_alloc = 0;
}

/*
 * Does the piece of memory need to be remapped because it doesn't satisfy 
 * all DMA constraints?
 */
udi_boolean_t
_osdep_dma_needs_remap(void *copy_mem, udi_size_t offset, udi_size_t len,
		       _udi_dma_handle_t *dmah, _osdep_dma_mem_t mem_type)
{
    udi_boolean_t no_remap;
    udi_ubit32_t v = _UDI_CONSTRAINT_VAL(dmah->dma_constraints->attributes,
					 UDI_DMA_ADDR_FIXED_TYPE);
 
#ifdef _DMA_NOISY
    _OSDEP_PRINTF
	("_osdep_dma_needs_remap: copy_mem=%p offset=0x%lx len=0x%lx dmah=%p mem_type=0x%x\n",
	 copy_mem, offset, len, dmah, mem_type);
    _OSDEP_PRINTF
	("constraints: phys_align=%lx dma_len=%lx dma_gran=%lx dmasize=%x phys_boundary=%lx\n",
	 dmah->dma_osinfo.phys_align, dmah->dma_osinfo.dma_len,
	 dmah->dma_osinfo.dma_gran, dmah->dma_osinfo.dmasize,
	 dmah->dma_osinfo.phys_boundary);

#endif
    if (mem_type != SCGTH_MEM) {
#ifdef _DMA_NOISY
	_OSDEP_PRINTF("mem_type != SCGTH_MEM\n");
#endif
	/*
	 * Check dma element constraints.
	 * What we're inspecting can be either a regular "middle"
	 * element or the last element in the scatter/gather list,
	 * the difference being that last elements don't have to satisfy the
	 * granularity constraint.
	 */

	/*
	 * Check
	 * a) if the start address of the memory chunk is aligned
	 *    according to  UDI_DMA_ELEMENT_ALIGNMENT_BITS 
	 * b) if the length of the memory chunk is less than that specified by
	 *    UDI_DMA_ELEMENT_LENGTH_BITS 
	 * c) if the length of the memory chunk is a multiple of that specified
	 *    by UDI_DMA_ELEMENT_GRNAULARITY_BITS 
	 * d) if the address range of the memory chunk to be mapped conforms to 
	 *    UDI_DMA_ADDR_FIXED_BITS of the given UDI_DMA_ADDR_FIXED_TYPE 
	 * e) if the address is within UDI_DMA_DATA_ADDRESSABLE_BITS 
	 * 
	 * If all of the above is true, no remapping is needed for now. 
	 * We have to remap if after all memory chunks have been mapped it 
	 * turns out that we use more scatter/gather elements/segments than 
	 * allowed. 
	 */

	no_remap =
	    /*
	     * a)
	     */
	    ( ( dmah->dma_osinfo.phys_align == 0 ) ||
	      ( ALIGN_OFFSET(virt_to_bus(copy_mem + offset),
	       		dmah->dma_osinfo.phys_align) == 0)
	    )
	    &&
	    /*
	     * b)
	     */
	    ( ( dmah->dma_osinfo.dma_len == 0 ) ||
	      ( len <= dmah->dma_osinfo.dma_len )
	    )
	    &&
	    /*
	     * c)
	     */
	    ( ( dmah->dma_osinfo.dma_gran == 0 ) || 
	      ( mem_type == LAST_DATA ) ||
	      ( ALIGN_OFFSET(len, dmah->dma_osinfo.dma_gran) == 0)
	    )
	    &&
	    /*
	     * e)
	     */
	    ( ( dmah->dma_osinfo.dmasize == 0 ) || 
	      ( dmah->dma_osinfo.dmasize >= 64) ||
	      /*
	       * I don't think the test below _*EVER*_ returns true
	       * and it seems to me to be a bug.
	       * Therefore, I comment it out for now.
	       */
#if 0
	      ( (sizeof(void *) * 8) <= dmah->dma_osinfo.dmasize ) ||
#endif
	      ( ( virt_to_bus(copy_mem + offset) <
	            (1ULL << (dmah->dma_osinfo.dmasize + 1)) ) &&
	        ( virt_to_bus(copy_mem + offset + len) <
	            (1ULL << (dmah->dma_osinfo.dmasize + 1)) )
	      )
	    );

	/*
	 * check d) last
	 */
	if (dmah->dma_osinfo.phys_boundary)
	    switch (v) {
		case UDI_DMA_FIXED_LIST:
		    /*
		     * The entire scatter/gather list must satisfy
		     * the fixed address bit constraint.  We have
		     * to check each element to see if it shares its fixed
		     * address bits with the first element.
		     * Then fall through to check the element itself.
		     * If there is no first element yet (i.e. this IS
		     * the first element), just fall through.
		     */
		    if (dmah->dma_osinfo.copy_mem)
			no_remap = no_remap &&
			    ( (ALIGN_TO(virt_to_bus(dmah->dma_osinfo.copy_mem),
					dmah->dma_osinfo.phys_boundary)
			      ) ==
			      (ALIGN_TO(virt_to_bus(copy_mem + offset),
					dmah->dma_osinfo.phys_boundary)
			      )
			    );

		    /*
		     * FALL THROUGH
		     */
		case UDI_DMA_FIXED_ELEMENT:
		    /*
		     * Only this memory chunk (= scatter/gather element)
		     * has to fulfill the fixed address constraint.
		     */
		    no_remap = no_remap &&
			( (ALIGN_TO(virt_to_bus(copy_mem + offset),
					dmah->dma_osinfo.phys_boundary)
			  ) ==
			  (ALIGN_TO(virt_to_bus(copy_mem + offset + len),
					dmah->dma_osinfo.phys_boundary)
			  )
			);
		    break;

		default:
		    /*
		     * Should not happen
		     */
		    _OSDEP_ASSERT(0);
	    }
    } else {

#ifdef _DMA_NOISY
	_OSDEP_PRINTF("mem_type == SCGTH_MEM\n");
	_OSDEP_PRINTF("constraints: scgth_phys_align=%lx scgth_dmasize=%x\n",
		      dmah->dma_osinfo.scgth_phys_align,
		      dmah->dma_osinfo.scgth_dmasize);
#endif
	/*
	 * Check scatter/gather list constaints.
	 */

	/*
	 * Check 
	 * a) if the start address of the memory chunk is aligned according to 
	 *    UDI_DMA_SCGTH_ALIGNMENT_BITS 
	 * b) if the address is within UDI_DMA_SCGTH_ADDRESSABLE_BITS 
	 * 
	 * If all of the above is true, no remapping is needed for now. 
	 * We have to remap if after all memory chunks have been mapped it 
	 * turns out that we use more scatter/gather elements/segments than 
	 * allowed. 
	 */

	no_remap =
	    /*
	     * a)
	     */
	    ( (dmah->dma_osinfo.scgth_phys_align == 0) ||
	      (ALIGN_OFFSET(virt_to_bus(copy_mem + offset),
	         dmah->dma_osinfo.scgth_phys_align) == 0)
	    ) &&
	    /*
	     * b)
	     */
	    ( (dmah->dma_osinfo.scgth_dmasize == 0) ||
	      (dmah->dma_osinfo.scgth_dmasize >= 64) ||
	      ( (virt_to_bus(copy_mem + offset)
	           < (1ULL << (dmah->dma_osinfo.scgth_dmasize + 1))
		) &&
		(virt_to_bus(copy_mem + offset + len)
	           < (1ULL << (dmah->dma_osinfo.scgth_dmasize + 1))
		)
	      )
	    );

    }
#ifdef _DMA_NOISY
    _OSDEP_PRINTF("_osdep_dma_needs_remap: %s\n", no_remap ? "FALSE" : "TRUE");
#endif

    return !no_remap;
}


udi_size_t
_osdep_dma_get_ofs(void* mem, _udi_dma_handle_t *dmah, 
		   _osdep_dma_mem_t mem_type)
{
    unsigned long start;
    
    if (mem_type == SCGTH_MEM) {
	if (dmah->dma_osinfo.scgth_phys_align == 0) {
	  return 0;
	}
	start = ALIGN_TO(virt_to_bus(mem), dmah->dma_osinfo.scgth_phys_align);
#ifdef _DMA_NOISY
	_OSDEP_PRINTF("start = %p\n", (void*)start);
#endif
	if (start < virt_to_bus(mem)) {
	    start += dmah->dma_osinfo.scgth_phys_align;
	}
    } else {
	if (dmah->dma_osinfo.phys_align == 0) {
	   return 0;
	}
	start = ALIGN_TO(virt_to_bus(mem), dmah->dma_osinfo.phys_align);
	if (start < virt_to_bus(mem)) {
	    start += dmah->dma_osinfo.phys_align;
    }
    }
    return (udi_size_t)(start - virt_to_bus(mem));
}
	
/*
 * Grab a piece of DMA memory
 */

#ifndef MAX_ORDER /* Allow Linux 2.4 kernel mmzone.h to override this. */
#define MAX_ORDER 5
#endif

_osdep_dma_alloc_t *
_osdep_dma_mem_alloc(udi_size_t len, _udi_dma_handle_t *dmah,
		     _osdep_dma_mem_t mem_type, int flags)
{
    void *dma_mem;
    _osdep_dma_alloc_t* alloc;
    udi_size_t offset;

#ifdef _DMA_NOISY
    _OSDEP_PRINTF("_osdep_dma_mem_alloc: len=0x%lx\n", len);
#endif

    alloc = kmem_cache_alloc(osdep_dma_alloc, 
			     (flags & UDI_WAITOK) ? GFP_KERNEL : GFP_ATOMIC);

    if (!alloc) { 
#ifdef _DMA_NOISY
	_OSDEP_PRINTF("could not get alloc.\n");
#endif
	return NULL;
    }	

    dma_mem = kmalloc(len, flags & UDI_WAITOK ? GFP_KERNEL : GFP_ATOMIC);
#ifdef INSTRUMENT_MALLOC
    no_kmallocs++;
#endif
    if (!dma_mem) {
#ifdef _DMA_NOISY
	_OSDEP_PRINTF("could not get mem.\n");
#endif
	kmem_cache_free(osdep_dma_alloc, alloc);
	return NULL;
    }

    if (_osdep_dma_needs_remap(dma_mem, 0, len, dmah, mem_type)) {
#ifdef _DMA_NOISY
	_OSDEP_PRINTF("mem didn't satisfy constraints, trying harder.\n");
#endif
#ifdef INSTRUMENT_MALLOC
	kfree(dma_mem);
	no_kfrees++;
#endif
	/*
	 * grab memory with the size of the original requested length plus
	 * the alignment of the memory
	 */
	dma_mem = kmalloc(len + (mem_type == SCGTH_MEM ? 
			   dmah->dma_osinfo.scgth_phys_align :
			   dmah->dma_osinfo.phys_align), 
			   flags & UDI_WAITOK ? GFP_KERNEL : GFP_ATOMIC);
#ifdef INSTRUMENT_MALLOC
	no_kmallocs++;
#endif
	if (!dma_mem) {
#ifdef _DMA_NOISY
	    _OSDEP_PRINTF("couldn't get mem this time.\n");
#endif
	    kmem_cache_free(osdep_dma_alloc, alloc);
	    return NULL;
	}
	offset = _osdep_dma_get_ofs(dma_mem, dmah, mem_type);
#ifdef _DMA_NOISY
	_OSDEP_PRINTF("better mem has offset %ld.\n", offset);
#endif
        _OSDEP_ASSERT(!_osdep_dma_needs_remap(dma_mem, offset, len, dmah,
		      mem_type));
    } else offset = 0;		
    alloc->dma_mem = dma_mem;
    alloc->ofs = offset;
    alloc->size = len;
    _OSDEP_SIMPLE_MUTEX_LOCK(&osdep_dma_cache_lock);
    UDI_ENQUEUE_TAIL(&osdep_dma_cache, &alloc->link);
    _OSDEP_SIMPLE_MUTEX_UNLOCK(&osdep_dma_cache_lock);
    return alloc;
}

/*
 * Free the DMA memory
 */
void
_osdep_dma_mem_free(_osdep_dma_alloc_t *alloc)
{
#ifdef _DMA_NOISY
    _OSDEP_PRINTF("_osdep_dma_mem_free: mem=%p size=0x%lx\n", alloc->dma_mem, 
							      alloc->size);
#endif
    kfree(alloc->dma_mem);
#ifdef INSTRUMENT_MALLOC
    no_kfrees++;
#endif
    _OSDEP_SIMPLE_MUTEX_LOCK(&osdep_dma_cache_lock);
    UDI_QUEUE_REMOVE(&alloc->link);
    _OSDEP_SIMPLE_MUTEX_UNLOCK(&osdep_dma_cache_lock);
    kmem_cache_free(osdep_dma_alloc, alloc);
}

/*
 * Start/Continue building the scatter gather list for the given memory
 * block
 */
udi_boolean_t
_osdep_dma_build_scgth(_udi_dma_handle_t *dmah, void *mem, udi_size_t size,
		       int flags)
{
    udi_scgth_t *scgth = &dmah->dma_scgth;
    udi_size_t scgth_size;
    void *scgth_mem = NULL;
    udi_scgth_element_32_t *el32p = NULL, *last32 = NULL;
    udi_scgth_element_64_t *el64p = NULL, *last64 = NULL;
    udi_boolean_t swap_scgth;
    udi_size_t seglen, thispage;
    unsigned long base, paddr, scgth_paddr;
    void *vaddr;
#ifdef _DMA_NOISY
    _OSDEP_PRINTF("_osdep_dma_build_scgth dmah=%p mem=%p size=0x%lx\n", dmah,
		  mem, size); 
#endif

    if (!scgth->scgth_num_elements) {
	if (scgth->scgth_format & UDI_SCGTH_32) {
	    scgth_size = sizeof(udi_scgth_element_32_t);
	}
	else {
	    scgth_size = sizeof(udi_scgth_element_64_t);
    	}

	/* Calculate the size rounded up to a page boundary. */
	scgth_size *= bytes_to_pages_round_up(dmah->dma_osinfo.dma_size);

	/*
	 * (TODO) Make physically incontiguous chunks of scatter/gather
	 * segments work.
	 * For now, we only allocate one chunk that's physically contiguous.
	 */
	dmah->dma_osinfo.scgth_alloc = _osdep_dma_mem_alloc(scgth_size, dmah, 
							     SCGTH_MEM, flags);
	if (!dmah->dma_osinfo.scgth_alloc) 
		return FALSE;
	scgth_mem = dmah->dma_osinfo.scgth_alloc->dma_mem + 
		    dmah->dma_osinfo.scgth_alloc->ofs;
#ifdef _DMA_NOISY
	_OSDEP_PRINTF("dma: allocated physical scgth mem at vaddr %p, size 0x%lx\n",
		      scgth_mem, scgth_size);
#endif
	if (!scgth_mem) {
	    return FALSE;
	}

	dmah->dma_scgth_size = scgth_size;

	dmah->dma_osinfo.els_in_seg = dmah->dma_osinfo.scgth_num_segments = 0;

	if (scgth->scgth_format & UDI_SCGTH_32) {
	    scgth->scgth_elements.el32p = el32p =
		(udi_scgth_element_32_t *) scgth_mem;
	} else {
	    scgth->scgth_elements.el64p = el64p =
		(udi_scgth_element_64_t *) scgth_mem;
	}

    } else {
#if 0 /*_OSDEP_HOST_IS_BIG_ENDIAN*/
#define ADDRSWAP32(x) (void*)_OSDEP_ENDIAN_SWAP_32(x)
#else
#define ADDRSWAP32(x) x
#endif
	if (scgth->scgth_format & UDI_SCGTH_32) {
	    scgth_mem = el32p =
		&scgth->scgth_elements.el32p[scgth->scgth_num_elements];
	} else {
	    scgth_mem = el64p =
		&scgth->scgth_elements.el64p[scgth->scgth_num_elements];
	}
#ifdef _DMA_NOISY
	_OSDEP_PRINTF("dma: using given scgth array at vaddr:%p\n", scgth_mem);
#endif
    }

    scgth_paddr = virt_to_bus(scgth_mem);
#ifdef _DMA_NOISY
    _OSDEP_PRINTF("dma: scgth_mem paddr:%lX\n", scgth_paddr);
#endif

    swap_scgth = scgth->scgth_must_swap;

    /*
     * Generate a scatter/gather list
     */

    vaddr = mem;
    while (size) {
	base = virt_to_bus(vaddr);	/* Phys addr of this segment */
	seglen = 0;
	do {
	    /* TBD: next line isn't 64-bit clean */
	    thispage =
		min(size, PAGE_SIZE - ((udi_ubit32_t) vaddr & (PAGE_SIZE - 1)));
	    /*
	     * bytes until the end of this page
	     */
	    seglen += thispage;	/* This many more contiguous bytes */
	    vaddr += thispage;	/* Bump up virtual address */
	    size -= thispage;	/* Recompute amount left */
	    if (size == 0)
		break;	/* End of request */
	    paddr = virt_to_bus(vaddr);
	    /*
	     * Get next page's address
	     */
	} while ((base + seglen) == paddr);
	/*
	 * Exiting this loop means we have a physically contig. block of size:
	 * seglen.
	 */

	/*
	 * Now set up the scatter/gather list element.
	 */

#ifdef _DMA_NOISY
	_OSDEP_PRINTF("dma: scgth el from %lX to %lX\n", base, base + seglen);
#endif

	if (scgth->scgth_format & UDI_SCGTH_32) {
	    if (swap_scgth) {
#ifdef _DMA_NOISY
		_OSDEP_PRINTF("dma: format is swapped 32 bit scgth. el32p:%p\n",
			      el32p);
#endif
		el32p->block_busaddr =
		    _OSDEP_ENDIAN_SWAP_32((udi_ubit32_t)base);
		el32p->block_length = _OSDEP_ENDIAN_SWAP_32(seglen);
	    } else {
#ifdef _DMA_NOISY
		_OSDEP_PRINTF("dma: format is no swap 32 bit scgth. el32p:%p\n",
			      el32p);
#endif
		el32p->block_busaddr = (udi_ubit32_t) base;
		el32p->block_length = seglen;
	    }
	    el32p++;
	} else {
	    if (swap_scgth) {
		el64p->block_busaddr.first_32 =
		    _OSDEP_ENDIAN_SWAP_32((udi_ubit32_t)(base >> 16 >> 16));
		el64p->block_busaddr.second_32 =
		    _OSDEP_ENDIAN_SWAP_32((udi_ubit32_t)base);
		el64p->block_length = _OSDEP_ENDIAN_SWAP_32(seglen);
	    } else {
#ifdef _DMA_NOISY
		_OSDEP_PRINTF("dma: format is no swap 64 bit scgth. el64p:%p\n",
			      el64p);
#endif
		el64p->block_busaddr.first_32 = (udi_ubit32_t)base;
		el64p->block_busaddr.second_32 =
		    (udi_ubit32_t) (base >> 16 >> 16);
		el64p->block_length = seglen;
	    }
	    el64p->el_reserved = 0;

	    el64p++;
	}
#ifdef _DMA_NOISY
	_OSDEP_PRINTF("dma: incrementing number of scatter/gather elements\n");
#endif
	scgth->scgth_num_elements++;
	if (dmah->dma_osinfo.scgth_max_el > 0 &&
	    (scgth->scgth_num_elements % dmah->dma_osinfo.scgth_max_el == 0)) {
	    /*
	     * We have scgth_max_el elements in current segment.
	     * Break off segment and start a new one that is
	     * physically adjacent to the previous one.
	     */

#ifdef _DMA_NOISY
	    _OSDEP_PRINTF("dma: new scgth seg at %p\n",
			  (scgth->scgth_format & UDI_SCGTH_32) ?
			 	 (void *)(el32p + 1)
			  	: (void *)(el64p + 1));
#endif

	    dmah->dma_osinfo.scgth_num_segments++;
	    if (scgth->scgth_format & UDI_SCGTH_32) {
		if (swap_scgth) {
		    el32p->block_busaddr =
			_OSDEP_ENDIAN_SWAP_32(virt_to_bus(el32p + 1));
		    el32p->block_length = 0;
		    if (last32)
			last32->block_length =
			    _OSDEP_ENDIAN_SWAP_32(
				   (dmah->dma_osinfo.els_in_seg + 1)
				   & UDI_SCGTH_EXT
				 );
		} else {
		    el32p->block_busaddr = virt_to_bus(el32p + 1);
		    el32p->block_length =
			UDI_SCGTH_EXT & (dmah->dma_osinfo.els_in_seg + 1);
		    if (last32) {
			last32->block_length =
			    (dmah->dma_osinfo.els_in_seg + 1) & UDI_SCGTH_EXT;
		}
		}
		last32 = el32p;
	    } else {
		if (swap_scgth) {
		    el64p->block_busaddr.first_32 =
			((sizeof(long) == 4) ? 0 :
			 _OSDEP_ENDIAN_SWAP_32(
				 (udi_ubit32_t) (virt_to_bus((el64p + 1)) >> 32)));

		    el64p->block_busaddr.second_32 = 
			_OSDEP_ENDIAN_SWAP_32(
				 (udi_ubit32_t)(virt_to_bus(el64p + 1) & 0x0FFFFFFFFULL));
		    el64p->block_length = 0;
		    if (last64)
			last64->block_length =
			    _OSDEP_ENDIAN_SWAP_32(
				 dmah->dma_osinfo.els_in_seg + 1);
		    el64p->el_reserved = _OSDEP_ENDIAN_SWAP_32(UDI_SCGTH_EXT);
		} else {
		    el64p->block_busaddr.first_32 =
			(udi_ubit32_t)(virt_to_bus(el64p + 1) & 0x0FFFFFFFFULL);
		    el64p->block_busaddr.second_32 =
			(udi_ubit32_t)(virt_to_bus(el64p + 1) >> 16 >> 16);
		    el64p->block_length = 0;
		    if (last64) {
			last64->block_length = dmah->dma_osinfo.els_in_seg + 1;
		    }
		    el64p->el_reserved = UDI_SCGTH_EXT;
		}
		last64 = el64p;

	    }
	    dmah->dma_osinfo.els_in_seg = 0;

	}
	/*
	 * new segment 
	 */
    }

    if (scgth->scgth_format & UDI_SCGTH_32) {
	if (last32) {
	    if (swap_scgth) {
		last32->block_length =
		    _OSDEP_ENDIAN_SWAP_32(
			(dmah->dma_osinfo.els_in_seg + 1) & UDI_SCGTH_EXT);
	    }
	    else {
		last32->block_length =
		    (dmah->dma_osinfo.els_in_seg + 1) & UDI_SCGTH_EXT;
	}
	}
    } else {
	if (last64) {
	    if (swap_scgth) {
		last64->block_length =
		    _OSDEP_ENDIAN_SWAP_32(dmah->dma_osinfo.els_in_seg + 1);
	    }
	    else {
		last64->block_length = dmah->dma_osinfo.els_in_seg + 1;
	}
    }
    }

    if (scgth->scgth_format & UDI_SCGTH_DMA_MAPPED) {
	if (scgth->scgth_format & UDI_SCGTH_32) {
	    el32p = &scgth->scgth_first_segment.el32;
  	    el32p->block_busaddr = scgth_paddr;
	    el32p->block_length = 
	       sizeof(udi_scgth_element_32_t) * scgth->scgth_num_elements;
	} else {
	    el64p = &scgth->scgth_first_segment.el64;
	    el64p->block_busaddr.first_32 = (udi_ubit32_t)(scgth_paddr);
	    el64p->block_busaddr.second_32 = (udi_ubit32_t)(scgth_paddr >> 16 >> 16);
	    el64p->block_length =
		sizeof(udi_scgth_element_64_t) * scgth->scgth_num_elements;
	    el64p->el_reserved = 0;
	}
    }

    return TRUE;
}

/*
 * Prepare the dma handle
 */
udi_status_t
_udi_dma_prepare(_udi_dma_handle_t *dmah)
{
    _udi_dma_constraints_t *cons = dmah->dma_constraints;
    udi_ubit32_t v, *attr = cons->attributes;

#ifdef _DMA_NOISY
    _OSDEP_PRINTF("entered _udi_dma_prepare\n");
#endif

    UDI_QUEUE_INIT(&dmah->dma_osinfo.remap_queue);
    dmah->dma_scgth.scgth_elements.el32p = NULL;
    dmah->dma_scgth.scgth_elements.el64p = NULL;

    /*
     * TBD: Handle endianness issues. 
     */

    /*
     * Get the number of addressable bits for the dma elements.
     */
    v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_DATA_ADDRESSABLE_BITS);
#ifdef _DMA_NOISY
    _OSDEP_PRINTF("dma_data_addr_bits: %d\n", v);
#endif
    dmah->dma_osinfo.dmasize = v;

    /*
     * Get the number of bits of the maximum length of a single
     * scatter/gather element.
     * If 0, there's no length limitation.
     */
    v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_ELEMENT_LENGTH_BITS);
#ifdef _DMA_NOISY
    _OSDEP_PRINTF("dma_elem_len_bits: %d\n", v);
#endif
    if (v) {
	dmah->dma_osinfo.dma_len = 1 << v;
    }
    else {
	dmah->dma_osinfo.dma_len = 0;
    }

    /*
     * Get the number of bits for the granularity of dma elements.
     * This means that the length of every dma element (except for the last
     * in a transfer) must be a multiple of this granularity.
     * If 0, no such restriction is enforced.
     */
    v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_ELEMENT_GRANULARITY_BITS);
#ifdef _DMA_NOISY
    _OSDEP_PRINTF("dma_elem_gran_bits: %d\n", v);
#endif
    if (v) {
	dmah->dma_osinfo.dma_gran = 1 << v;
    }
    else {
	dmah->dma_osinfo.dma_gran = 0;
    }

    /*
     * Get the alignment bits for a dma element.
     * If 0, no alignment is enforced.
     */
    v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_ELEMENT_ALIGNMENT_BITS);
#ifdef _DMA_NOISY
    _OSDEP_PRINTF("dma_elem_align_bits: %d\n", v);
#endif
    if (v) {
	dmah->dma_osinfo.phys_align = 1 << v;
    }
    else {
	dmah->dma_osinfo.phys_align = 0;
    }

    /*
     * Get the number of fixed bits in an address range
     * of a dma element/scatter-gather list.
     * If 0, no restriction is enforced.
     */
    v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_ADDR_FIXED_BITS);
#ifdef _DMA_NOISY
    _OSDEP_PRINTF("dma_addr_fixed_bits: %d\n", v);
#endif
    if (v) {
	dmah->dma_osinfo.phys_boundary = 1 << v;
    }
    else {
	dmah->dma_osinfo.phys_boundary = 0;
    }

    /*
     * Get the maximum number of scatter/gather elements.
     * If 0, no maximum number is enforced.
     */
    v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_MAX_ELEMENTS);
#ifdef _DMA_NOISY
    _OSDEP_PRINTF("dma: maximum number of scatter/gather elements is 0x%x\n",
   	          v);
#endif
    dmah->dma_osinfo.max_scgth = v;

    /*
     * Is this handle to be dma-mapped, i.e. hardware-visible?
     */
    v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_FORMAT);
    if ((v & UDI_SCGTH_DMA_MAPPED)) {
	/*
	 * Get the number of addressable bits for the scatter/gather list.
	 */
	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_ADDRESSABLE_BITS);
#ifdef _DMA_NOISY
        _OSDEP_PRINTF("dma_scgth_addressable_bits: %d\n", v);
#endif
	dmah->dma_osinfo.scgth_dmasize = v;

	/*
	 * Get the number of maximum scatter/gather segments
	 * (arrays of scatter/gather elements).
	 * If 0, no such restriction is enforced.
	 */
	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_MAX_SEGMENTS);
#ifdef _DMA_NOISY
        _OSDEP_PRINTF("dma_scgth_max_segments: %d\n", v);
#endif
	dmah->dma_osinfo.scgth_max_scgth = v;

	/*
	 * Get the number of maximum scatter/gather elements per
	 * scatter/gather segment.
	 * If 0, no such restriction is enforced.
	 */
	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_MAX_EL_PER_SEG);
#ifdef _DMA_NOISY
        _OSDEP_PRINTF("dma_scgth_max_el_per_seg: %d\n", v);
#endif
	dmah->dma_osinfo.scgth_max_el = v;

	/*
	 * Get the number of alignment bits for the scatter/gather list
	 * (the alignment must be met by every segment).
	 * If 0, no alignment is enforced.
	 */
	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_ALIGNMENT_BITS);
#ifdef _DMA_NOISY
        _OSDEP_PRINTF("dma_scgth_align_bits: %d\n", v);
#endif
	if (v) {
	    dmah->dma_osinfo.scgth_phys_align = 1 << v;
	}
	else {
	    dmah->dma_osinfo.scgth_phys_align = 0;
	}

	/*
	 * TBD: Figure out how much extra memory we need for each
	 * scatter/gather list element when we allocate the list.
	 * We currently don't support that functionality.
	 */

    } else {
	dmah->dma_osinfo.scgth_phys_align = 0;
    }
    

    return UDI_OK;
}

/*
 * Release a DMA handle.
 */
void
_udi_dma_release(_udi_dma_handle_t *dmah)
{
    
}

/* 
 * Allocate a chunk of DMA memory.
 */
void *
_udi_dma_mem_alloc(_udi_dma_handle_t *dmah, udi_size_t size, int flags)
{
    void *mem;

#ifdef _DMA_NOISY
    _OSDEP_PRINTF("entered _udi_dma_mem_alloc , size=0x%lx\n", size);
#endif

    dmah->dma_osinfo.dma_alloc = _osdep_dma_mem_alloc(size, dmah, LAST_DATA, 
						      flags);
	
    if (!dmah->dma_osinfo.dma_alloc)
	return NULL;

    mem = dmah->dma_osinfo.dma_alloc->dma_mem + dmah->dma_osinfo.dma_alloc->ofs;

#ifdef _DMA_NOISY
    _OSDEP_PRINTF("dma: allocated physical mem at %p\n", mem);
#endif

   /*
    * Initialize scgth structure size.
    *
    * If an old scgth list is around, remove it and tell 
    * _osdep_dma_build_scgth to start anew.
    */ 
    dmah->dma_osinfo.dma_size = size;
    if (dmah->dma_osinfo.scgth_alloc)
	_osdep_dma_mem_free(dmah->dma_osinfo.scgth_alloc);
    
    dmah->dma_scgth.scgth_elements.el32p = NULL;
    dmah->dma_scgth.scgth_elements.el64p = NULL;
    if (!_osdep_dma_build_scgth(dmah, mem, size, flags)) {
	_osdep_dma_mem_free(dmah->dma_osinfo.dma_alloc);
	mem = NULL;
    }
    dmah->dma_osinfo.copy_mem = mem;

    if (mem && !(flags & UDI_MEM_NOZERO)) udi_memset(mem, 0, size);
    return mem;
}

/*
 * Free the scatter/gather list for the dma handle.
 */
void
_udi_dma_scgth_free(_udi_dma_handle_t *dmah)
{
#ifdef _DMA_NOISY
    _OSDEP_PRINTF("entered _udi_dma_scgth_free\n");
#endif
#ifdef _DMA_NOISY
    _OSDEP_PRINTF("dma: freeing physical memory at %p\n",
		  dmah->dma_osinfo.scgth_alloc->dma_mem);
#endif
    _osdep_dma_mem_free(dmah->dma_osinfo.scgth_alloc);
    dmah->dma_scgth_size = 0;
}

/*
 * Free the dma memory for a dma handle.
 */
void
_udi_dma_mem_free(_udi_dma_handle_t *dmah)
{
#ifdef _DMA_NOISY
    _OSDEP_PRINTF("entered _udi_dma_mem_free\n");
#endif
#ifdef _DMA_NOISY
    _OSDEP_PRINTF("dma: freeing physical memory at %p\n",
		  dmah->dma_osinfo.dma_alloc->dma_mem);
#endif
    _osdep_dma_mem_free(dmah->dma_osinfo.dma_alloc);
}


/*
 * External buffer Ops for dma-able buffer memory
 */
STATIC void 
_osdep_dma_xfree(void *context, udi_ubit8_t *space, udi_size_t size) 
{
#ifdef __DMA_NOISY
	_OSDEP_PRINTF("Freeing x-dma buf at %p\n", space);
#endif
	_osdep_dma_mem_free((_osdep_dma_alloc_t *)context);
}

#ifdef _UDI_PHYSIO_SUPPORTED
STATIC void*
_osdep_dma_xmap(void *context, udi_ubit8_t* space, udi_size_t size,
		struct _udi_dma_handle *dmah)
{
#ifdef __DMA_NOISY
	_OSDEP_PRINTF("dma-mapping x-dma buf at %p\n", space);
#endif
	/*
         * Space points to dma-able memory already. Just build a scatter/gather
	 * list.
	 */
	return _osdep_dma_build_scgth(dmah, space, size, 
				      dmah->dma_osinfo.waitflag) ? space : NULL;
}

STATIC void
_osdep_dma_xunmap(void *context, udi_ubit8_t* space, udi_size_t size,
		  struct _udi_dma_handle *dmah)
{
#ifdef __DMA_NOISY
	_OSDEP_PRINTF("dma-unmapping x-dma buf at %p\n", space);
#endif
}
#endif


STATIC _udi_xbops_t _osdep_dma_xbops = {
	_osdep_dma_xfree 
#ifdef _UDI_PHYSIO_SUPPORTED
	, _osdep_dma_xmap, _osdep_dma_xunmap
#endif
};

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
_udi_dma_buf_map(struct _udi_dma_handle *dmah, _udi_alloc_marshal_t *allocm,
		 int waitflag)
{
    _osdep_dma_alloc_t *copy_alloc = NULL;
    _udi_buf_container2x_t *bc2;
    _udi_buf_container3x_t *bc3;
    _udi_buf_t bc_header;
    void* bc, *old_container;
    void *copy_mem = NULL;
    _udi_buf_t *buf = (_udi_buf_t *) dmah->dma_buf;
    _udi_buf_memblk_t *memblk;
    _udi_buf_dataseg_t *dataseg, *last = NULL;
    udi_boolean_t external_block_seen = FALSE, needs_remap;
    __osdep_dma_remap_t *remap_log;
    udi_size_t offset, len, xlen = 0;
    udi_ubit8_t *mem_space;
    udi_size_t mem_size;

#ifdef DMA_NOISY
    _OSDEP_PRINTF("entered _udi_dma_buf_map dmah=%p, buf=%p, ofs=%lx, " \
		  "len=%lx, current=%s, from=%p\n",
		  dmah, buf, dmah->dma_buf_offset, dmah->dma_maplen,
		  current->comm, __builtin_return_address(0));
#endif
    
    dmah->dma_osinfo.waitflag = waitflag;
    dmah->dma_osinfo.dma_size = dmah->dma_maplen;
    dmah->dma_scgth.scgth_num_elements = 0;
    dataseg = (_udi_buf_dataseg_t *) (buf->buf_contents);
    offset = dmah->dma_buf_offset;
    len = dmah->dma_maplen;
    /*
     * Find the dataseg that contains the part of the buffer that is to be 
     * mapped.
     */
    while (dataseg) {
	bc = dataseg->bd_container;
	if (&((_udi_buf_container1_t *) bc)->bc_dataseg == dataseg) {
#ifdef _DMA_NOISY
	    _OSDEP_PRINTF("_udi_dma_buf_map: no memblk.\n");
#endif
	    /*
	     * The data segment has no memory block. Therefore, it can
	     * not be external memory.
	     */
	    memblk = NULL;
	} else {
	    memblk = dataseg->bd_memblk;
	}
	/*
	 * Do not use the memblk here because a memblk may contain data that
	 * was deleted from the buffer.
	 * Only the datasegs know about that.
	 */
	mem_space = dataseg->bd_start;
	mem_size = dataseg->bd_end - dataseg->bd_start;

#ifdef _DMA_NOISY
	_OSDEP_PRINTF("_udi_dma_buf_map: dataseg=%p, memblk=%p, dir=%s\n",
		      dataseg, dataseg->bd_memblk,
		      dmah->dma_buf_flags & UDI_DMA_IN ? "IN" : "OUT");
	_OSDEP_PRINTF
	    ("bd_start=%p bd_end=%p bd_mem=%p bd_mem->size=0x%lx space=%p\n",
	     dataseg->bd_start, dataseg->bd_end, dataseg->bd_memblk,
	     dataseg->bd_memblk->bm_size, dataseg->bd_memblk->bm_space);
#endif
	if (mem_space && (mem_size <= offset)) {
	    offset -= mem_size;
	    last = dataseg;
	    dataseg = dataseg->bd_next;
	    continue;
	}

	if (mem_space == NULL) {
	    /*
	     * The buffer memblk has not been set up yet, i.e. does not have 
	     * memory associated with it.
	     * Since we are dma-mapping this buffer, it makes sense to allocate
	     * dma-able memory here right away. 
	     * If we are supposed to map at an offset into this dataseg, we
	     * allocate as many bytes of "normal" buffer memory as the offset 
	     * says, and then allocate a new container & segment with external
	     * (dma-able) memory. If no offset is set, we get rid of the empty
	     * container and replace it with the external one. 
	     * Note: This can only happen at the end of the buffer, so we don't
	     * have to worry about the empty dataseg's bd_next pointer.
	     */
#ifdef __DMA_NOISY
	    _OSDEP_PRINTF("memblk empty, allocating my own.\n");
#endif

	    if (offset) {
#ifdef __DMA_NOISY
		_OSDEP_PRINTF("offset = %ld\n", offset);
#endif
		mem_space = _OSDEP_MEM_ALLOC(offset, UDI_MEM_NOZERO, waitflag);
		if (!mem_space) return FALSE;
	    	
		memblk->bm_space = mem_space;
	 	memblk->bm_size = offset;
	    	memblk->bm_external = NULL;

	    	dataseg->bd_start = mem_space;
	    	dataseg->bd_end = mem_space + offset;
	    	buf->buf_total_size += offset;
		dataseg->bd_next = NULL;
		last = dataseg;
		offset = 0;
#ifdef __DMA_NOISY
		_OSDEP_PRINTF("memblk populated. space = %p, size = %ld, "
			      "dataseg = %p\n", mem_space, memblk->bm_size,
			      dataseg);
#endif
	   } 
  	   /*
	    * Now allocate the buffer space, using dma functions.
	    */
	   copy_alloc = _osdep_dma_mem_alloc(len, dmah, LAST_DATA, waitflag);
#ifdef __DMA_NOISY
	   _OSDEP_PRINTF("dma-alloced %ld bytes at %p (offset %ld)\n",
 		 	 len, copy_alloc->dma_mem, copy_alloc->ofs);
#endif	    
	   if (!copy_alloc) {
		/*
		 * This failed because waitflag was UDI_WAITOK, so defer.
		 */
		_OSDEP_MEM_FREE(bc);
		return FALSE;
    	   }

	   mem_space = copy_alloc->dma_mem + copy_alloc->ofs;

#ifdef __DMA_NOISY

	   _OSDEP_PRINTF("getting rid of unused container %p\n", 
	   	         dataseg->bd_container);
#endif
	   old_container = dataseg->bd_container;
	   /*
	    * If there is a previous dataseg, we need to allocate a
	    * _udi_buf_container2x_t and hang it after the previous dataseg.
	    * Otherwise, we need to allocate a _udi_buf_container3x_t and
	    * hang it in the beginning of the buffer. A container3x has all the
	    * header fields in it, so we need to make sure that we copy those
	    * also. Plus, since we effectively reallocate the buffer head, we
	    * have to update dmah->buf and buf with the new buffer head.
	    */
	  
	   if (!last) {
		udi_memcpy(&bc_header, 
			   (dataseg->bd_container), 
			   sizeof(bc_header));
	   }
	   
           if (last) {
		/*
		 * Allocate a _udi_buf_container2x_t to contain the dma-able
		 * external memory in the middle of a buffer.
		 */
	   	bc2 = _OSDEP_MEM_ALLOC(sizeof(*bc2), UDI_MEM_NOZERO, waitflag);
	      	if (!bc2) return FALSE;
		bc = bc2;
		bc3 = NULL;
	    	dataseg = &bc2->bc_dataseg;
	    	memblk = &bc2->bc_memblk;
	        bc2->bc_xmem_context = (void *)copy_alloc;
	    
#ifdef __DMA_NOISY
	      	_OSDEP_PRINTF("new container_2x_t allocated at %p\n", bc2);
#endif
	   } else {
		/*
		 * Allocate a _udi_buf_container3x_t to container the dma-able
		 * external memory at the beginning of the buffer.
		 */
		bc3 = _OSDEP_MEM_ALLOC(sizeof(*bc3), UDI_MEM_NOZERO, waitflag);
		if (!bc3) return FALSE;
		bc = bc3;
		bc2 = NULL;
	    	dataseg = &bc3->bc_dataseg;
	    	memblk = &bc3->bc_memblk;
	    	bc3->bc_xmem_context = (void *)copy_alloc;
		udi_memcpy(&bc3->bc_header, &bc_header, sizeof(bc_header));

		/*
		 * Update buffer handles.
		 */
		dmah->dma_buf = buf = (_udi_buf_t *)bc3;

#ifdef __DMA_NOISY
	      	_OSDEP_PRINTF("new container_3x_t allocated at %p\n", bc3);
#endif
	   }
#ifdef __DMA_NOISY	
	    _OSDEP_PRINTF("Filling in new dataseg.\n");
#endif
	    
      	    _OSDEP_MEM_FREE(old_container);
		
	    dataseg->bd_start = mem_space;    		
	    dataseg->bd_end = mem_space + len;
	    dataseg->bd_memblk = last ? &bc2->bc_memblk : &bc3->bc_memblk;
	    dataseg->bd_container = bc;
	    dataseg->bd_next = NULL;
	    _OSDEP_ATOMIC_INT_INIT(&memblk->bm_refcnt, 1);
	    memblk->bm_space = mem_space;
	    memblk->bm_size = len;
	    memblk->bm_external = &_osdep_dma_xbops;

	    if (last) {
#ifdef __DMA_NOISY
		_OSDEP_PRINTF("hanging new dataseg %p after %p\n", 
			      dataseg, last);
#endif
		last->bd_next = dataseg;
	    } else {
#ifdef __DMA_NOISY
		_OSDEP_PRINTF("hanging new dataseg %p as beginning of buf\n", 
			      dataseg);
#endif
		buf->buf_contents = dataseg;
	    }

	    buf->buf.buf_size += len;
	    buf->buf_total_size += len;
	    mem_size = len;
	}

	dataseg = dataseg->bd_next;

	/*
	 * xlen = the number of bytes to map in this memblk
	 */
	xlen = (len >= (mem_size - offset) ? (mem_size - offset) : len);

	/*
	 * If the external memory is dma-able memory that we just allocated
	 * above, treat it as regular memory. This is because if that buffer is
	 * ever used with different dma constraints than the memory was 
	 * allocated for, we simply remap it like we would regular buffer 
	 * memory. We can do that since we know what the memory looks like.
	 */
	if (memblk && memblk->bm_external && 
	    memblk->bm_external != &_osdep_dma_xbops) {
#ifdef _DMA_NOISY
	    _OSDEP_PRINTF("_udi_dma_buf_map: external block at %p.\n",
			  memblk->bm_space);
#endif
		
	    if (memblk->bm_external->xbop_dma_map) {
#ifdef _DMA_NOISY
		_OSDEP_PRINTF
		    ("_udi_dma_buf_map: invoking external dma_buf_map at %p.\n",
		     memblk->bm_external->xbop_dma_map);
#endif
		copy_mem =
		    (*memblk->
		     bm_external->xbop_dma_map) (*(void **) (memblk + 1),
						 mem_space + offset, xlen,
						 dmah);
		if (!copy_mem)
			goto Failure;
#ifdef DMA_NOISY
		_OSDEP_PRINTF
		    ("_udi_dma_buf_map: external buf_map returned %p.\n",
		     copy_mem);
#endif
		/*
		 * remember what we did 
		 */
		remap_log = _OSDEP_MEM_ALLOC(sizeof(*remap_log), 0, waitflag);
		if (!remap_log) {
			(*memblk->bm_external->xbop_dma_unmap)(
				*(void **)(memblk + 1), copy_mem, xlen, dmah);
			goto Failure;
		}
		remap_log->memblk = memblk;
		remap_log->mem_space = mem_space;
		remap_log->dma_mem = copy_mem;
		remap_log->dma_alloc = NULL;
		remap_log->size = xlen;
		remap_log->offset = offset;
		UDI_ENQUEUE_TAIL(&dmah->dma_osinfo.remap_queue,
				 &remap_log->link);
	    } else {
		/*
		 * TBD:
		 * Fancy a guess and just assume that the external
		 * memory is a contiguous block (and not some os-
		 * dependent scatter/gather structure or something
		 * like that) and just map the block as-is.
		 *
		 * Is that really the right thing to do though?
		 * For now we just don't support that.
		 */
#ifdef DMA_NOISY
	        _OSDEP_PRINTF("no dma map function for external mem block\n");
#endif
		goto Failure;
	    }
	    external_block_seen = TRUE;
	} else {
	    needs_remap =
		_osdep_dma_needs_remap(mem_space, offset, len, dmah,
				       dataseg ==
				       NULL ? LAST_DATA : MID_DATA);
	    if (needs_remap) {
		/*
		 * For now, if we have already dealt with
		 * external blocks, assert, because we won't
		 * interfere with the external memory's buf_map
		 * operation.
		 * TBD: Later, perhaps, we can try to consolidate
		 * mapping external blocks and internal blocks
		 * somehow if we really need to.
		 */
		if (external_block_seen) {
#ifdef DMA_NOISY
		    _OSDEP_PRINTF("WARNING! external buffer needs remap after"
				  " buf_map\n");
#endif
		    goto Failure;
		}

		/*
		 * Get a piece of dma-able memory.
		 */
		copy_alloc =
		    _osdep_dma_mem_alloc(xlen, dmah,
					 dataseg ==
					 NULL ? LAST_DATA : MID_DATA, 
					 waitflag);
		if (!copy_alloc) {
#ifdef DMA_NOISY
		   _OSDEP_PRINTF("dma memory for remap could not be grabbed.\n");
#endif
		    goto Failure;
		}

		copy_mem = copy_alloc->dma_mem + copy_alloc->ofs;

		/*
		 * Copy buffer content, if necessary.
		 */
		if (dmah->dma_buf_flags & UDI_DMA_OUT) {
		    udi_memcpy(copy_mem, mem_space + offset, xlen);
		}

		/*
		 * Remember what we mapped.
		 */
		remap_log = _OSDEP_MEM_ALLOC(sizeof(*remap_log), 0, 1);
		if (!remap_log) {
			_osdep_dma_mem_free(copy_alloc);
			goto Failure;
		}	
		remap_log->memblk = memblk;
		remap_log->mem_space = mem_space;
		remap_log->dma_alloc = copy_alloc;
		remap_log->dma_mem = copy_mem;
		remap_log->offset = offset;
		remap_log->size = xlen;
		UDI_ENQUEUE_TAIL(&dmah->dma_osinfo.remap_queue,
				 &remap_log->link);
	    } else {
		/*
		 * No mapping needed, copy_mem is the start
		 * of the buffer space.
		 */
		copy_mem = mem_space + offset;
	    }
	    /*
	     * Build the scatter gather list for the current copy_mem.
	     */
	    if (!_osdep_dma_build_scgth(dmah, copy_mem, xlen, waitflag)) {
#ifdef DMA_NOISY
		_OSDEP_PRINTF("scatter/gather list could not be built\n");
#endif
		goto Failure;
	    }

	}

	/*
	 * See if we have too many elements or too many segments.
	 * If so, we'll have to start over and grab a new chunk of
	 * dma-able memory for the entire buffer, uugh!
	 */
	if (
	       /*
	        * Do we have more elements than allowed?
	        */
	       ((dmah->dma_osinfo.max_scgth != 0) &&
		(dmah->dma_scgth.scgth_num_elements 
		  > dmah->dma_osinfo.max_scgth) )
	       ||
	       /*
	        * do we have more segments than allowed?
	        */
	       ((dmah->dma_osinfo.scgth_num_segments != 0) &&
		(dmah->dma_osinfo.scgth_num_segments >
		dmah->dma_osinfo.scgth_max_scgth))
	   ) {
	    /*
	     * If there is external memory involved, we are out of luck and have
	     * to fail.
	     * If this is internal memory, we'll just start over and copy the
	     * entire buffer.
	     */
	    if (external_block_seen) {
#ifdef DMA_NOISY
		_OSDEP_PRINTF("dma-able mem is too fragmented but external.\n"
			      "es=%d rs=%ld ee=%d re=%d\n",
			      dmah->dma_osinfo.scgth_max_scgth,
			      dmah->dma_osinfo.scgth_num_segments,
			      dmah->dma_osinfo.max_scgth,
			      dmah->dma_scgth.scgth_num_elements);
#endif
		goto Failure;
	    } else {
		copy_mem =
		    _udi_dma_mem_alloc(dmah, dmah->dma_maplen, UDI_MEM_NOZERO);

		if (!copy_mem)
#ifdef DMA_NOISY
	            _OSDEP_PRINTF("dma-able mem is too fragmented and no new"
				  " mem could be allocated.\n");	
#endif
		    goto Failure;

		if (dmah->dma_buf_flags & UDI_DMA_OUT) {
		    udi_buf_read(&dmah->dma_buf->buf, dmah->dma_buf_base,
				 dmah->dma_maplen, copy_mem); 
		}
	    }

	}
	if (offset < mem_size)
	    offset = 0;
	len -= xlen;
    }

    allocm->_u.dma_buf_map_result.complete = TRUE;
    allocm->_u.dma_buf_map_result.status = UDI_OK;
    allocm->_u.dma_buf_map_request.state = 255;
    /*
     * all done
     */

    return TRUE;

  Failure:
    /*
     * For some reason or other, we could not map the entire buffer portion.
     * Now walk the list of already mapped memblks and free the mappings.
     */
    {
	udi_queue_t *elem, *tmp;

	UDI_QUEUE_FOREACH(&dmah->dma_osinfo.remap_queue, elem, tmp) {
	    remap_log = UDI_BASE_STRUCT(elem, __osdep_dma_remap_t, link);

	    _OSDEP_MEM_FREE(remap_log);
	}

	/*
	 * Depending on whether we are just deferring further processing because
	 * we can't block but would need to or because there is some real
	 * problem, we return FALSE for deferral and TRUE with a failure status
	 * otherwise.
	 */ 
	if (waitflag & UDI_WAITOK) {
#ifdef _DMA_NOISY
		_OSDEP_PRINTF("Deferring buf mapping...\n");
#endif
		return FALSE;
	} else {
		allocm->_u.dma_buf_map_result.complete = TRUE;
		allocm->_u.dma_buf_map_result.status = 
			UDI_STAT_RESOURCE_UNAVAIL;
		allocm->_u.dma_buf_map_request.state = 255;
	}
#ifdef __DMA_NOISY
	_OSDEP_PRINTF("buffer %p has contents %p.\n", buf, buf->buf_contents);
#endif
	return TRUE;
    }
}

#if 0
void
_udi_dma_buf_map_discard(_udi_alloc_marshal_t *allocm)
{
    /*
     * XXX - Does this really need to be OSDEP? 
     */ }
#endif

void
_udi_dma_buf_unmap(struct _udi_dma_handle *dmah)
{
    udi_queue_t *elem;
    __osdep_dma_remap_t *remap_log;
#ifdef DMA_NOISY
    _OSDEP_PRINTF("entered _udi_dma_buf_unmap dmah=%p current=%s\n", dmah,
		  current->comm);
#endif
    
    /* 
     * We are undoing a dma-mapping for a udi buffer.
     * When we mapped the buffer, we remembered every re-allocation we did.
     * Now we go through these and, where needed, copy the data from the dma
     * memory back into the buffer.
     */
    while (!UDI_QUEUE_EMPTY(&dmah->dma_osinfo.remap_queue)) {
	elem = UDI_DEQUEUE_HEAD(&dmah->dma_osinfo.remap_queue);
	remap_log = UDI_BASE_STRUCT(elem, __osdep_dma_remap_t, link);

#ifdef _DMA_NOISY
	_OSDEP_PRINTF
	    ("remap_log=%p, remap_log->memblk_external=%p, remap_log->dma_mem=%p, remap_log->size=0x%lx\n",
	     remap_log, remap_log ? remap_log->memblk->bm_external : NULL,
	     remap_log ? remap_log->dma_mem : NULL,
	     remap_log ? remap_log->size : 0);
#endif

	if (remap_log->memblk && remap_log->memblk->bm_external &&
	    remap_log->memblk->bm_external->xbop_dma_unmap) {
	    /*
	     * unmap external block
	     */
	    (*remap_log->memblk->bm_external->xbop_dma_unmap)(
			*(void **) (remap_log->memblk + 1),
		        remap_log->dma_mem,
			remap_log->size,
			dmah);
	} else {
	    if (dmah->dma_buf_flags & UDI_DMA_IN) {
		udi_memcpy(remap_log->mem_space + remap_log->offset,
			   remap_log->dma_mem, remap_log->size);
	    }
	    _osdep_dma_mem_free(remap_log->dma_alloc);
	}
	_OSDEP_MEM_FREE(remap_log);
    }
#ifdef __DMA_NOISY
	_OSDEP_PRINTF("buffer %p has contents %p.\n", dmah->dma_buf, 
		      dmah->dma_buf->buf_contents);
#endif
}

