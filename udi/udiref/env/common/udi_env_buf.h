
/*
 * File: env/common/udi_env_buf.h
 *
 * UDI Buffer Common Environment definitions.
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

#ifndef _UDI_ENV_BUF_H
#define _UDI_ENV_BUF_H

#ifdef _UDI_PHYSIO_SUPPORTED
struct _udi_dma_handle;
#endif

/*
 * Buffer operations vector.
 *
 * These provide function pointers that are customized to handle a particular
 * buffer content layout. While this UDI environment implementation has its
 * own preferred layout, it is also prepared to deal with OS-specific native
 * layouts if requested to do so by OS-specific external mappers. This allows
 * the mappers to avoid the cost of converting layouts at the edges.
 */
typedef struct {
	void (*bop_xyzzy) (void);
	/*
	 * FIXME: Put real stuff here. 
	 */
} _udi_bops_t;

typedef struct _udi_buf_tag {
	udi_buf_tag_t bt_v;		/* Visible part of tag */
	struct _udi_dev_node *bt_drv_instance;
	/*
	 * For driver-instance-private tags 
	 */
	struct _udi_buf_tag *bt_next;
} _udi_buf_tag_t;

/*
 * Internal buffer handle structure.
 */
typedef struct _udi_buf {
	udi_buf_t buf;			/* The udi_buf_t */
	void *buf_contents;		/* Layout-specific contents */
	const _udi_bops_t *buf_ops;	/* Layout-specific functions */
	_udi_buf_path_t *buf_path;
	udi_size_t buf_total_size;	/* For quick reference */
	_udi_buf_tag_t *buf_tags;
	_OSDEP_ATOMIC_INT_T buf_refcnt;	/* Reference count for entire buf */
	struct _udi_alloc_hdr alloc_hdr;	/* Standard allocation header. */
	udi_boolean_t movable_warning_issued;	/* True to complain no more. */
#ifdef _UDI_PHYSIO_SUPPORTED
	struct _udi_dma_handle *buf_dmah;	/* DMA handle most recently mapped */
#endif
} _udi_buf_t;

/*
 * get the context of an external buffer
 */
#define _UDI_XBUF_CONTEXT(b) \
	((_udi_buf_container3x_t *)(b))->bc_xmem_context

/*
 * change the context of an external buffer
 */
#define _UDI_SET_XBUF_CONTEXT(b, x) \
	((_udi_buf_container3x_t *)(b))->bc_xmem_context = (x)

/*
 * UDI generic layout.
 */

/* External memory block operations. */

typedef struct {
	void (*xbop_free) (void *context,
			   udi_ubit8_t *space,
			   udi_size_t size);
#ifdef _UDI_PHYSIO_SUPPORTED
	void *(*xbop_dma_map) (void *context,
			       udi_ubit8_t *space,
			       udi_size_t size,
			       struct _udi_dma_handle * dmah);
	void (*xbop_dma_unmap) (void *context,
				udi_ubit8_t *space,
				udi_size_t size,
				struct _udi_dma_handle * dmah);
#endif
} _udi_xbops_t;

typedef struct _udi_buf_memblk {
	_OSDEP_ATOMIC_INT_T bm_refcnt;
	udi_ubit8_t *bm_space;		/* data storage space */
	udi_size_t bm_size;		/* # bytes of data space */
	const _udi_xbops_t *bm_external;	/* NULL if storage space
						 * is part of container;
						 * otherwise, ops for
						 * special handling of
						 * external memory */
} _udi_buf_memblk_t;

typedef struct _udi_buf_dataseg {
	udi_ubit8_t *bd_start;		/* first valid byte */
	udi_ubit8_t *bd_end;		/* last valid byte + 1 */
	_udi_buf_memblk_t *bd_memblk;
	void *bd_container;		/* pointer to containing
					 * allocated memory */
	struct _udi_buf_dataseg *bd_next;
} _udi_buf_dataseg_t;

/*
 * To streamline allocations for common cases, multiple components of the
 * buffer content may be allocated in one piece. Structures for these
 * combined allocations are defined below. They can be distinguished (e.g.
 * at free time) by examining the bm_container pointer, since bm_container
 * will point to the base of the allocated structure. If bm_container
 * points to the memblk itself, then the memblk was separately allocated.
 * If bm_container points to the dataseg, it must be a container2 type.
 * If bm_container points to the header, it must be a container3 type.
 *
 * In all cases, the actual data space is allocated in the same container
 * with the memblk, following it, unless bm_external is non-NULL. If
 * bm_external is non-NULL, a (void *) context pointer follows the memblk
 * instead, which is used as the context argument to the external functions;
 * the container1x, 2x, and 3x structures illustrate this case.
 */

typedef struct {
	_udi_buf_dataseg_t bc_dataseg;
} _udi_buf_container1_t;

typedef struct {
	_udi_buf_memblk_t bc_memblk;
	_udi_buf_dataseg_t bc_dataseg;
} _udi_buf_container2_t;

typedef struct {
	_udi_buf_memblk_t bc_memblk;
	_udi_buf_dataseg_t bc_dataseg;
	void *bc_xmem_context;
} _udi_buf_container2x_t;

typedef struct {
	_udi_buf_t bc_header;
	_udi_buf_memblk_t bc_memblk;
	_udi_buf_dataseg_t bc_dataseg;
} _udi_buf_container3_t;

typedef struct {
	_udi_buf_t bc_header;
	_udi_buf_memblk_t bc_memblk;
	_udi_buf_dataseg_t bc_dataseg;
	void *bc_xmem_context;
} _udi_buf_container3x_t;

void _udi_buf_extern_init(udi_buf_write_call_t *callback,
			  udi_cb_t *_gcb,
			  void *mem,
			  udi_size_t len,
			  _udi_xbops_t *xbops,
			  void *xbops_context,
			  udi_buf_path_t buf_path);
void _udi_buf_extern_add(udi_buf_t *_buf,
			 void *mem,
			 udi_size_t len,
			 _udi_xbops_t *xbops,
			 void *xbops_context);
void _udi_buf_write_sync(void *copy_mem,
			 udi_size_t maplen,
			 udi_size_t len,
			 _udi_buf_t *buf,
			 udi_size_t *base,
			 udi_size_t *offset,
			 _udi_xbops_t *xbops,
			 void *context);

/*
 * TODO: Add prototypes for some utility functions that allocate and
 * initialize various buf internals.
 */

#define _UDI_BUF_C3	1		/* Container has header/dataseg/memblk/xmem;
					 * i.e. _udi_buf_container3x_t */
#define _UDI_BUF_C2	2		/* Container has dataseg/memblk;
					 * i.e. _udi_buf_container2_t or 2x_t */
#define _UDI_BUF_C1	4		/* Container is only a dataseg;
					 * i.e. _udi_buf_container1_t */

#endif /* _UDI_ENV_BUF_H */
