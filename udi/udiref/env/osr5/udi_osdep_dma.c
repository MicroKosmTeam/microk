/* #define DEBUG_ALLOC */
#define DEFER_SG_FREE
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

/*
 * UDI Environment -- Direct Memory Access.
 */


#include <udi_env.h>
#include <sys/sysmacros.h>
#include <sys/immu.h>
#include <sys/stream.h>
#include <sys/cmn_err.h>
#include <sys/xdebug.h>

extern _OSDEP_EVENT_T		_udi_alloc_event;
struct _osdep_udi_freemem	*_osdep_freelist;
extern _OSDEP_MUTEX_T		_udi_free_lock;

int			check_active;


udi_status_t
_udi_dma_prepare(
_udi_dma_handle_t	*dmah
)
{
	udi_scgth_t		*scgth;
	_udi_dma_constraints_t	*cons;
	udi_ubit32_t		v;
	udi_ubit32_t		*attr;

	if (dmah->dma_osinfo != NULL)
	{
		calldebug();
	}

	dmah->dma_osinfo = (__osdep_dma_t *)
		_udi_kmem_alloc(sizeof(__osdep_dma_t), 0, 1);

	if (dmah->dma_osinfo == NULL)
	{
#ifdef DEBUG_ALLOC
		udi_debug_printf("Alloc of dma_osinfo failed\n");
#endif

		return UDI_STAT_RESOURCE_UNAVAIL;
	}

#ifdef DEBUG_ALLOC
	udi_debug_printf("_udi_dma_prepare: dmah->dma_osinfo = 0x%x\n",
		dmah->dma_osinfo);
#endif

	scgth = &dmah->dma_scgth;

	cons = dmah->dma_constraints;

	attr = cons->attributes;

	/* TODO: Handle endianness issues. */

	/*
	 * Remember the scatter/gather visibility requirements.
	 */
	dmah->dma_osinfo->scgth_vis =
		_UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_FORMAT);

	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_DATA_ADDRESSABLE_BITS);

	if (v > 64)
	{
		v = 64;
	}

	dmah->dma_osinfo->phys_dmasize = v;

	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_MAX_ELEMENTS);

	dmah->dma_osinfo->phys_max_scgth = v;

	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_ELEMENT_ALIGNMENT_BITS);

	if (v > 31)
	{
#ifdef DEBUG_ALLOC
		udi_debug_printf("alignment bits > 31 not supported \n");
#endif
		_udi_kmem_free(dmah->dma_osinfo);

		return UDI_STAT_NOT_SUPPORTED;
	}

	dmah->dma_osinfo->phys_align = 1 << v;

	v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_ADDR_FIXED_BITS);

	if (v != 0)
	{
		if (v > 31)
		{
#ifdef DEBUG_ALLOC
			udi_debug_printf("UDI_DMA_ADDR_FIXED_BITS > 31 not "
				"supported\n");
#endif
			_udi_kmem_free(dmah->dma_osinfo);

			return UDI_STAT_NOT_SUPPORTED;
		}

		dmah->dma_osinfo->phys_boundary = 1 << v;

		v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_ADDR_FIXED_TYPE);

		if (v == UDI_DMA_FIXED_VALUE)
		{
#ifdef DEBUG_ALLOC
			udi_debug_printf(
				"UDI_DMA_FIXED_VALUE not supported\n");
#endif
			_udi_kmem_free(dmah->dma_osinfo);

			return UDI_STAT_NOT_SUPPORTED;
		}
	}


	if (_UDI_CONSTRAINT_VAL(attr, UDI_DMA_SLOP_IN_BITS) != 0 ||
	    _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SLOP_OUT_BITS) != 0 ||
	    _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SLOP_OUT_EXTRA) != 0)
	{
		/* TODO: Add support for these. */
#ifdef DEBUG_ALLOC
		udi_debug_printf("non zero SLOP constraints not supported\n");
#endif
		_udi_kmem_free(dmah->dma_osinfo);

		return UDI_STAT_NOT_SUPPORTED;
	}

#ifdef DEBUG_ALLOC
	udi_debug_printf("_udi_dma_prepare: total_segment_size 0x%x, "
		"scgth_vis 0x%x, phys_dmasize 0x%x\n",
		dmah->dma_osinfo->total_segment_size,
		dmah->dma_osinfo->scgth_vis,
		dmah->dma_osinfo->phys_dmasize);

	udi_debug_printf("_udi_dma_prepare: phys_max_scgth 0x%x, "
		"phys_align 0x%x, phys_boundary 0x%x\n",
		dmah->dma_osinfo->phys_max_scgth,
		dmah->dma_osinfo->phys_align,
		dmah->dma_osinfo->phys_boundary);
#endif

	return UDI_OK;
}

void
_udi_dma_release(
_udi_dma_handle_t *dmah
)
{
#ifdef DEFER_SG_FREE
	/*
	 * Free cached scgth memory, if any.
	 */
	_udi_dma_scgth_really_free(dmah);
#endif

#ifdef DEBUG_ALLOC
	//udi_debug_printf("_udi_dma_release 0x%x\n", dmah->dma_osinfo);
#endif

	_udi_kmem_free(dmah->dma_osinfo);
}

void *
_udi_dma_mem_alloc(
_udi_dma_handle_t	*dmah,
udi_size_t		size,
int			flags
)
{
	udi_ubit32_t			paddr[2];
	udi_size_t			scgth_size;
	void				*mem;
	void				*scgth_mem;
	udi_scgth_element_32_t		*el32p;
	udi_scgth_element_64_t		*el64p;
	udi_scgth_t			*scgth;
	udi_boolean_t			swap_scgth;
	udi_ubit32_t			i;
	udi_ubit32_t			element_size;
	udi_ubit32_t			num_pages;
	udi_ubit32_t			breakup_size;
	udi_ubit32_t			sleep_flag;
	udi_ubit32_t			pacur;

#ifdef DEBUG_ALLOC
	udi_debug_printf(
		"_udi_dma_mem_alloc: dmah = 0x%x, size 0x%x, flags 0x%x\n",
		dmah, size, flags);
#endif

	scgth = &dmah->dma_scgth;

	num_pages = btoc(size);

#ifdef DEBUG_ALLOC
	udi_debug_printf(
		"_udi_dma_mem_alloc: num_pages = 0x%x, phys_max_scgth = 0x%x\n",
			num_pages, dmah->dma_osinfo->phys_max_scgth);
#endif

	if (activeintr && check_active)
	{
		calldebug();
	}

	/*
	 * Allocate the memory that we will be copying to/from.
	 */
	if (dmah->dma_osinfo->phys_max_scgth == 0)
	{
		mem = (void *)sptalloc(num_pages, PG_P|PG_RW, 0, 0);

		dmah->dma_osinfo->mem_type = OSDEP_MEM_TYPE_SPTALLOC;

		breakup_size = NBPP;

		scgth->scgth_num_elements = btoc(size);
	}
	else if (num_pages > dmah->dma_osinfo->phys_max_scgth)
	{
#ifdef DEBUG_ALLOC
		udi_debug_printf(
			"_udi_dma_mem_alloc: OSDEP_MEM_TYPE_GETCPAGES\n");
#endif

		mem = (void *)getcpages(num_pages, KM_SLEEP);
#ifdef DEBUG_ALLOC
		udi_memset(mem, 0xac, ctob(num_pages));
#endif
		dmah->dma_osinfo->mem_type = OSDEP_MEM_TYPE_GETCPAGES;

		breakup_size = size;

		scgth->scgth_num_elements = 1;
	}
	else
	{
#ifdef DEBUG_ALLOC
		udi_debug_printf(
			"_udi_dma_mem_alloc: OSDEP_MEM_TYPE_SPTALLOC:2\n");
#endif

		mem = (void *)sptalloc(num_pages, PG_P|PG_RW, 0, 0);

#ifdef DEBUG_ALLOC
		udi_memset(mem, 0xad, ctob(num_pages));
#endif
		dmah->dma_osinfo->mem_type = OSDEP_MEM_TYPE_SPTALLOC;

		breakup_size = NBPP;

		scgth->scgth_num_elements = num_pages;
	}

	_OSDEP_ASSERT(dmah->dma_osinfo->mem_type != 0);

#ifdef DEBUG_ALLOC
	qincl((caddr_t)&_udi_mem_stats[dmah->dma_osinfo->mem_type].num_alloc);

	udi_debug_printf("_udi_dma_mem_alloc: mem = 0x%x : ", mem);

	udi_debug_printf("data : ");

	for (i = 0 ; i < 8 ; i++)
	{
		udi_debug_printf("%02x ", *((uchar_t *)mem + i));
	}

	udi_debug_printf("\n");

	osdep_register_memory(mem);
#endif

	if (!(flags & UDI_MEM_NOZERO))
	{
		udi_memset(mem, 0, size);
	}

	/*
	 * Allocate the scgth map that we will use for the DMA transfer.
	 */

	if (scgth->scgth_format & UDI_SCGTH_32)
	{
		scgth_size = sizeof(udi_scgth_element_32_t);
	}
	else
	{
		scgth_size = sizeof(udi_scgth_element_64_t);
	}

	scgth_size = scgth_size * scgth->scgth_num_elements;

	swap_scgth = scgth->scgth_must_swap &&
		!(scgth->scgth_format & UDI_SCGTH_DRIVER_MAPPED);

	scgth_size = KMEM_SIZE(scgth_size);

#ifdef DEFER_SG_FREE
	if (scgth_size <= dmah->dma_scgth_allocsize)
	{
		scgth_mem = scgth->scgth_elements.el32p;
	}
	else
	{
		/*
		 * Free cached scgth memory, if any.
		 */
		if (dmah->dma_scgth_allocsize > 0)
		{
			_udi_dma_scgth_really_free(dmah);
		}
#endif
		if ((dmah->dma_osinfo->scgth_vis & UDI_SCGTH_DMA_MAPPED) == 0)
		{
			scgth_mem = (void *)
				sptalloc(btoc(scgth_size), PG_P|PG_RW, 0, 0);
#ifdef DEBUG_ALLOC
			udi_memset(scgth_mem, 0xae, scgth_size);
#endif
			dmah->dma_scgth_allocsize = ctob(btoc(scgth_size));
		}
		else
		{
			if (activeintr || mp_qrunning() || _udi_init_time)
			{
				sleep_flag = KM_NOSLEEP;
			}
			else
			{
				sleep_flag = KM_SLEEP;
			}

			scgth_mem = kmem_zalloc(scgth_size, sleep_flag);
#ifdef DEBUG_ALLOC
			udi_memset(scgth_mem, 0xaf, scgth_size);
#endif
			dmah->dma_scgth_allocsize = scgth_size;
		}
#ifdef DEFER_SG_FREE
	}
#endif

#ifdef DEBUG_ALLOC
	udi_debug_printf("_udi_dma_mem_alloc: scgth = 0x%x scgth_mem = 0x%x\n",
		scgth, scgth_mem);

	osdep_register_memory(scgth_mem);
#endif

	/*
	 * Generate a scatter/gather list
	 */
	if (scgth->scgth_format & UDI_SCGTH_32)
	{
		el32p = (udi_scgth_element_32_t *)scgth_mem;

		scgth->scgth_elements.el32p = el32p;
	}
	else
	{
		el64p = (udi_scgth_element_64_t *)scgth_mem;

		scgth->scgth_elements.el64p = el64p;
	}

	for (i = 0 ; i < scgth->scgth_num_elements ; i++)
	{
		paddr[0] = svirtophys((char *)mem + (i * breakup_size));

		element_size = (size - (breakup_size * i) > breakup_size) ?
			breakup_size : (size - (breakup_size * i));

#ifdef DEBUG_ALLOC
		udi_debug_printf("_udi_dma_mem_alloc: 0x%x: virt 0x%x "
			"phys 0x%x\n", i, (char *)mem + (i * breakup_size),
			paddr[0]);
#endif

		if (scgth->scgth_format & UDI_SCGTH_32)
		{
			if (swap_scgth)
			{
				el32p->block_busaddr =
					_OSDEP_ENDIAN_SWAP_32(paddr[0]);

				el32p->block_length =
					_OSDEP_ENDIAN_SWAP_32(element_size);
			}
			else
			{
				el32p->block_busaddr = paddr[0];

				el32p->block_length = element_size;
			}

			el32p++;
		}
		else
		{
			if (swap_scgth)
			{
				el64p->block_busaddr.first_32 = 0;

				el64p->block_busaddr.second_32 =
					_OSDEP_ENDIAN_SWAP_32(paddr[0]);

				el64p->block_length =
					_OSDEP_ENDIAN_SWAP_32(element_size);
			}
			else
			{
				el64p->block_busaddr.first_32 = paddr[0];

				el64p->block_busaddr.second_32 = 0;

				el64p->block_length = element_size;
			}

			el64p->el_reserved = 0;

			el64p++;
		}
	}

	if (scgth->scgth_format & UDI_SCGTH_DMA_MAPPED)
	{
		pacur = svirtophys(scgth_mem);

		if (scgth->scgth_format & UDI_SCGTH_32)
		{
#ifdef TERMINATE_SG
			/*
			 * Zero out the last sg list entry.
			 */
			el32p->block_busaddr = 0;
			el32p->block_length = 0;
#endif

			el32p = &scgth->scgth_first_segment.el32;

			el32p->block_busaddr = pacur;

			el32p->block_length = scgth_size;
		}
		else
		{
#ifdef TERMINATE_SG
			/*
			 * Zero out the last sg list entry.
			 */
			el64p->block_busaddr.first_32 = 0;
			el64p->block_busaddr.second_32 = 0;
			el64p->block_length = 0;
			el64p->el_reserved = 0;
#endif

			el64p = &scgth->scgth_first_segment.el64;

			el64p->block_busaddr.first_32 = pacur;

			el64p->block_busaddr.second_32 = 0;

			el64p->block_length = scgth_size;

			el64p->el_reserved = 0;
		}
	}

	return mem;
}

void
_udi_dma_scgth_free(
_udi_dma_handle_t	*dmah
)
{
#ifdef DEFER_SG_FREE
	/* Keep it around for reuse... */
}

void
_udi_dma_scgth_really_free(_udi_dma_handle_t * dmah)
{
#endif
	udi_scgth_t			*scgth;
	udi_size_t			scgth_size;
	void				*mem;
	struct _osdep_udi_freemem	*freep;

	scgth = &dmah->dma_scgth;

	scgth_size = dmah->dma_scgth_allocsize;

	if (scgth_size == 0)
	{
		return;
	}

#ifdef DEFER_SG_FREE
	dmah->dma_scgth_allocsize = 0;
#endif

#ifdef DEBUG_ALLOC
	udi_debug_printf("_udi_dma_scgth_really_free: dmah = 0x%x\n", dmah);
#endif

	mem = scgth->scgth_elements.el32p;

#if 0
	if (scgth->scgth_format & UDI_SCGTH_32)
	{
		scgth_size = sizeof(udi_scgth_element_32_t);
	}
	else
	{
		scgth_size = sizeof(udi_scgth_element_64_t);
		mem = scgth->scgth_elements.el64p;
	}

	scgth_size = scgth_size * scgth->scgth_num_elements;

	scgth_size = KMEM_SIZE(scgth_size);
#endif

	_OSDEP_MUTEX_LOCK(&_udi_free_lock);

	freep = (struct _osdep_udi_freemem *)mem;

	freep->next = _osdep_freelist;

	_osdep_freelist = freep;

	if ((dmah->dma_osinfo->scgth_vis & UDI_SCGTH_DMA_MAPPED) == 0)
	{
#ifdef DEBUG_ALLOC
		udi_debug_printf("sptfree: mem = 0x%x size = 0x%x\n",
			mem, ctob(btoc(scgth_size)));
#endif

		freep->mem_type = OSDEP_MEM_TYPE_SPTALLOC;
	}
	else
	{
#ifdef DEBUG_ALLOC
		udi_debug_printf("kmem_free: mem = 0x%x size = 0x%x\n",
			mem, scgth_size);
#endif

		freep->mem_type = OSDEP_MEM_TYPE_KMEM;
	}

	_OSDEP_ASSERT(freep->mem_type != 0);

#ifdef DEBUG_ALLOC
	qincl((caddr_t)&_udi_mem_stats[freep->mem_type].num_defer_free);
#endif

	freep->length = scgth_size;

#ifdef DEBUG_ALLOC
	osdep_unregister_memory(mem);
#endif

	_OSDEP_MUTEX_UNLOCK(&_udi_free_lock);

#ifdef DEBUG_ALLOC
	udi_debug_printf("_udi_dma_scgth_really_free: mem = 0x%x\n", mem);
#endif

	wakeup((caddr_t)&_udi_alloc_event);
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
_udi_dma_buf_map(
struct _udi_dma_handle	*dmah,
_udi_alloc_marshal_t	*allocm,
int			waitflag
)
{
	void *copy_mem;

	if (dmah->dma_osinfo == NULL)
	{
		calldebug();
	}

#if HAVE_USE_INPLACE
	switch (_udi_dma_use_inplace(dmah, waitflag))
	{
	case _UDI_DMA_CANT_WAIT:
		return FALSE;

	case _UDI_DMA_CANT_USE_INPLACE:
#endif
		if (waitflag != UDI_WAITOK)
		{
			return FALSE;
		}

		copy_mem = _udi_dma_mem_alloc(dmah, dmah->dma_maplen, UDI_MEM_NOZERO);

		dmah->dma_osinfo->copy_mem = copy_mem;

		if (dmah->dma_buf_flags & UDI_DMA_OUT)
		{
			udi_buf_read(&dmah->dma_buf->buf, dmah->dma_buf_base,
				     dmah->dma_maplen, copy_mem);
		}
#if HAVE_USE_INPLACE
	}
#endif

	allocm->_u.dma_buf_map_result.complete = TRUE;
	allocm->_u.dma_buf_map_result.status = UDI_OK;
	allocm->_u.dma_buf_map_request.state = 255; /* all done */

#ifdef NOTYET
	switch (allocm->_u.dma_buf_map_request.state) {
	case 0:	/* Get the ball rolling... */
	default:
		_OSDEP_ASSERT(0);
	}
#endif /* NOTYET */

	return TRUE;
}

STATIC
void
_udi_defer_free(
void		*context,
udi_ubit8_t	*copy_mem,
udi_size_t	size
)
{
	struct _osdep_udi_freemem	*freep;
	__osdep_dma_t			*dma_osinfop;

	dma_osinfop = (__osdep_dma_t *)context;

#ifdef DEBUG_ALLOC
	udi_debug_printf("_udi_defer_free: copy_mem = 0x%x size = 0x%x "
		"dma_osinfo = 0x%x\n",
		copy_mem, size, dma_osinfop);
#endif

	_OSDEP_MUTEX_LOCK(&_udi_free_lock);

	freep = (struct _osdep_udi_freemem *)copy_mem;

	_OSDEP_ASSERT(dma_osinfop->mem_type != 0);

#ifdef DEBUG_ALLOC
	qincl((caddr_t)&_udi_mem_stats[dma_osinfop->mem_type].num_defer_free);
#endif

	freep->mem_type = dma_osinfop->mem_type;

	freep->length = size;

	freep->next = _osdep_freelist;

	_osdep_freelist = freep;

#ifdef DEBUG_ALLOC
	osdep_unregister_memory(copy_mem);
#endif

	_OSDEP_MUTEX_UNLOCK(&_udi_free_lock);

	wakeup((caddr_t)&_udi_alloc_event);
}

STATIC _udi_xbops_t dma_xbops = { &_udi_defer_free, NULL, NULL };

void
_udi_dma_buf_unmap(
struct _udi_dma_handle	*dmah
)
{
	void				*copy_mem;
	int				s;
#ifdef DEBUG_ALLOC
	int				i;
#endif
#ifdef DEBUG_ALLOC
	udi_debug_printf("_udi_dma_buf_unmap: dmah 0x%x - ", dmah);
#endif

	/* (See PERF TODO comment in _udi_dma_buf_map.) */

	copy_mem = dmah->dma_osinfo->copy_mem;

#if HAVE_USE_INPLACE
	if (copy_mem == NULL)
	{
		return;
	}
#endif

#ifdef DEBUG_ALLOC
	udi_debug_printf("copy_mem 0x%x\n", copy_mem);
#endif

	if (dmah->dma_buf_flags & UDI_DMA_IN)
	{
#ifdef DEBUG_ALLOC
		udi_debug_printf("_udi_buf_write_sync: copy_mem 0x%x - "
				"dmah 0x%x\n", copy_mem, dmah);

		udi_debug_printf("_udi_dma_buf_unmap - data: ");

		for (i = 0 ; i < 8 ; i++)
		{
			udi_debug_printf("%02x ", *((uchar_t *)copy_mem + i));
		}

		udi_debug_printf("\n");
#endif
		_udi_buf_write_sync(copy_mem, dmah->dma_maplen,
			dmah->dma_maplen,
			dmah->dma_buf, &(dmah->dma_buf_base),
			&(dmah->dma_buf_offset), &dma_xbops,
			(void *)dmah->dma_osinfo);

		return;
	}

	_udi_defer_free((void *)dmah->dma_osinfo, copy_mem, dmah->dma_maplen);
}

int
udi_get_ret(
int	*ret
)
{
	return;
}

