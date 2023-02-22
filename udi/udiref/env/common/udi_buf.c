
/*
 * File: env/common/udi_buf.c
 *
 * UDI Buffer Management Routines
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

/*
 * This is the backfill character, used when 'extra' data is needed
 * when accessing virtual buffer space that hasn't been initialized.
 */
#define _UDI_BF_CHAR	0xca

#if _UDI_PHYSIO_SUPPORTED
extern _udi_dma_constraints_t *_udi_default_dma_constraints;
#endif /* UDI_PHYSIO_SUPPORTED */

/*
 * _udi_buf_discard
 */
STATIC void
_udi_buf_discard(_udi_alloc_marshal_t *allocm)
{
	;
}

STATIC void _udi_buf_wrcp_complete(_udi_alloc_marshal_t *allocm,
				   void *new_mem);
STATIC void _udi_buf_xinit_complete(_udi_alloc_marshal_t *allocm,
				    void *new_mem);

static _udi_alloc_ops_t _udi_buf_wrcp_ops = {
	_udi_buf_wrcp_complete,
	_udi_buf_discard,
	_udi_alloc_no_incoming,
	_UDI_ALLOC_COMPLETE_MIGHT_BLOCK
};

static _udi_alloc_ops_t _udi_buf_xinit_ops = {
	_udi_buf_xinit_complete,
	_udi_buf_discard,
	_udi_alloc_no_incoming,
	_UDI_ALLOC_COMPLETE_WONT_BLOCK
};

/*
 * Removes any invalid buffer tags, due to shrinking of buffer space
 */
STATIC void
_udi_buf_rem_inv_tags(_udi_buf_t *buf)
{
	udi_buf_tag_t *tag;
	udi_ubit8_t dodelete;

	_udi_buf_tag_t *buf_tag = buf->buf_tags;
	udi_size_t bsize = ((udi_buf_t *)buf)->buf_size;
	_udi_buf_tag_t *pbuf_tag = NULL;

	while (buf_tag != NULL) {
		dodelete = 0;
		tag = &buf_tag->bt_v;

		switch (tag->tag_type) {
		case UDI_BUFTAG_SET_iBE16_CHECKSUM:
			/*
			 * First, make sure the destination is valid 
			 * checksum is 16-bit, so add 2 bytes for comparison 
			 */
			if (tag->tag_value + 2 > bsize)
				dodelete = 1;
			/*
			 * Fall through to check offset/length 
			 */
		case UDI_BUFTAG_BE16_CHECKSUM:
		case UDI_BUFTAG_SET_TCP_CHECKSUM:
		case UDI_BUFTAG_SET_UDP_CHECKSUM:
		case UDI_BUFTAG_TCP_CKSUM_GOOD:
		case UDI_BUFTAG_UDP_CKSUM_GOOD:
		case UDI_BUFTAG_IP_CKSUM_GOOD:
		case UDI_BUFTAG_TCP_CKSUM_BAD:
		case UDI_BUFTAG_UDP_CKSUM_BAD:
		case UDI_BUFTAG_IP_CKSUM_BAD:
			/*
			 * Make sure the offset and length are still valid 
			 */
			if (tag->tag_off + tag->tag_len > bsize)
				dodelete = 1;
			break;
		default:
			break;
		}

		if (!dodelete) {
			pbuf_tag = buf_tag;
		} else {
			/*
			 * Found one to delete, so remove it from the chain 
			 */
			pbuf_tag->bt_next = buf_tag->bt_next;
			/*
			 * Do not forget to free the memory 
			 */
			_OSDEP_MEM_FREE(buf_tag);
		}

		/*
		 * Next! 
		 */
		if (!dodelete)
			buf_tag = buf_tag->bt_next;
		else
			buf_tag = pbuf_tag->bt_next;
	}
}

#define _UDI_BUF_COPY	1
#define _UDI_BUF_WRITE	2

STATIC void
_udi_buf_calc_mem(struct _buf_wrcp_request *request,
		  udi_size_t buf_size,
		  udi_size_t tot_size,
		  udi_size_t *xtr_size)
{
#ifdef NOTYET
	extern udi_dma_constraints_attr_t _udi_constraints_index[];
#endif
	_udi_buf_dataseg_t *dataseg;
	udi_size_t offset;

#if _UDI_PHYSIO_SUPPORTED
	udi_ubit32_t *attr = request->constraints->attributes;
#else
	udi_ubit32_t *attr = NULL;
#endif

	*xtr_size = 0;

	/*
	 * If this is an initial buffer allocation, take into account
	 * UDI_BUF_HEADER_SIZE and UDI_BUF_TRAILER_SIZE.
	 */
	if (request->dst_buf != NULL || attr == NULL) {
		request->bh_alloc = request->bt_alloc = 0;
	} else {

/*
 * Note: When the buffer code can do proper resizing without allocation,
 *      enable this.
 * Otherwise, enabling this just allocates extra space which we never use.
 */
#ifdef NOTYET
		use the new mechanism to get buf header and trailer size
			request->bh_alloc =
			attr[_udi_constraints_index[(UDI_BUF_HEADER_SIZE) - 1]
			     - 1];

		request->bt_alloc =
			attr[_udi_constraints_index[(UDI_BUF_TRAILER_SIZE) - 1]
			     - 1];
#else
		request->bh_alloc = request->bt_alloc = 0;
#endif
	}

	if (request->dst_buf == NULL) {
		/*
		 * Ensure space or Allocate / initialize 
		 * dst_buf == NULL, dst_len == 0, dst_off == 0 
		 */

		/*
		 * Whether we are doing the Ensure space or Allocate/initialize
		 * cases, we still do the same type of allocation
		 */
		request->cont_type = _UDI_BUF_C3;	/* Alloc hdr/ds/memblk */
		request->hdr_alloc = sizeof (_udi_buf_container3x_t);
		request->vir_mem = request->src_len;

		/*
		 * However, the amount of real memory allocation differs 
		 */
		if (request->src == NULL) {
			/*
			 * Allocate (reserve) only 
			 */
			request->mem_alloc = 0;
			request->phy_mem = 0;
		} else {
			/*
			 * Allocate and initialize 
			 */
			request->mem_alloc =
				request->src_len + request->bh_alloc +
				request->bt_alloc;
			request->phy_mem = request->src_len;
		}
	} else if (request->src == NULL) {
		/*
		 * Delete 
		 * dst_buf != NULL, src == NULL, src_len == 0 
		 * Only allocate if we are deleting the middle of a segment 
		 * Go to the first segment where the delete needs to occur 
		 */
		dataseg = request->dst_buf->buf_contents;
		offset = 0;
		while (dataseg &&
		       (dataseg->bd_end - dataseg->bd_start) + offset <
		       request->dst_off)
			dataseg = dataseg->bd_next;
		if (dataseg && offset != request->dst_off &&
		    (dataseg->bd_end - dataseg->bd_start) + offset <
		    request->dst_off + request->dst_len) {
			request->cont_type = _UDI_BUF_C1;
			request->hdr_alloc = sizeof (_udi_buf_dataseg_t);
		} else {
			request->cont_type = 0;
			request->hdr_alloc = 0;
		}
		request->mem_alloc = 0;
		request->vir_mem = buf_size - request->dst_len;
		if (buf_size == tot_size) {
			/*
			 * Easy case, where phys and virt buf space are equal 
			 */
			request->phy_mem = request->vir_mem;
		} else {
			/*
			 * phys and virt buf space are not equal 
			 */
			if (tot_size >= request->dst_off + request->dst_len) {
				/*
				 * Deleted physical memory 
				 */
				request->phy_mem = tot_size - request->dst_len;
			} else if (tot_size > request->dst_off) {
				/*
				 * Delete part of physical memory 
				 */
				request->phy_mem = tot_size - request->dst_off;
			} else {
				/*
				 * No physical memory deleted 
				 */
				request->phy_mem = tot_size;
			}
		}
	} else if (request->dst_len) {
		/*
		 * Overwrite and/or Replace
		 * dst_buf != NULL, src != NULL, dst_len > 0, src_len != 0 
		 * Identify how much new physical memory we need and if we
		 * Also find out if we need to insert some memory in
		 * the middle of a segment, and we hence need another new
		 * segment 
		 * Finally, check if we have to delete some part of the 
		 * buffer. 
		 */
		/*
		 * if dst_off + dst_len is larger than the buffer size,
		 * set it to buffer size, because in this case we may end up
		 * having to actually allocate some more memory
		 */
		if (request->dst_off + request->dst_len > tot_size) {
			if (request->dst_off > tot_size)
				request->dst_len = 0;
			else
				request->dst_len = tot_size - request->dst_off;
		}
		if (request->src_len > request->dst_len) {
			/*
			 * we're replacing dst_len bytes with src_len bytes
			 */
			request->cont_type = _UDI_BUF_C2;
			request->hdr_alloc = sizeof (_udi_buf_container2_t);
			request->mem_alloc = request->src_len -
				request->dst_len;

			/*
			 * See if we are inserting on a segment boundary 
			 */
			dataseg = request->dst_buf->buf_contents;
			offset = 0;
			while (dataseg && offset < request->dst_off) {
				offset += dataseg->bd_end - dataseg->bd_start;
				dataseg = dataseg->bd_next;
			}
			if (offset == request->dst_off) {
				/*
				 * We're at a segment boundary 
				 */
				if (!dataseg)
					request->mem_alloc = request->src_len;
			} else if (offset < request->dst_off ||
				   (offset == 0 && buf_size == 0)) {
				/*
				 * We are adding in the virtual memory 
				 * section i.e., dst_off is beyond the current 
				 * buffer, so we extend the buffer to accomodate 
				 * for both dst_len and the distance between 
				 * dst_off and the current end of the buffer     
				 */
				request->mem_alloc =
					(request->dst_off + request->src_len) -
					offset;
			} else {
				/*
				 * We are in the middle of a segment, so we 
				 * need to alloc another segment 
				 */
				request->cont_type |= _UDI_BUF_C1;
				*xtr_size = sizeof (_udi_buf_dataseg_t);
			}
			if (buf_size < request->dst_off + request->src_len) {
				request->vir_mem =
					request->dst_off + request->src_len;
			} else {
				request->vir_mem = buf_size;
			}
		} else if (request->src_len < request->dst_len) {
			/*
			 * We have to delete parts of the buffer
			 */
			dataseg = request->dst_buf->buf_contents;
			offset = 0;
			while (dataseg &&
			       (dataseg->bd_end - dataseg->bd_start) + offset <
			       request->dst_off)
				dataseg = dataseg->bd_next;
			if (dataseg && offset != request->dst_off &&
			    (dataseg->bd_end - dataseg->bd_start) + offset <
			    request->dst_off + request->dst_len -
			    request->src_len) {
				request->cont_type = _UDI_BUF_C1;
				request->hdr_alloc =
					sizeof (_udi_buf_dataseg_t);
			} else {
				request->cont_type = 0;
				request->hdr_alloc = 0;
			}
			request->mem_alloc = 0;
			request->vir_mem = buf_size - request->dst_len +
				request->src_len;
			if (buf_size == tot_size) {
				/*
				 * Easy case, where phys and virt buf space are
				 * equal 
				 */
				request->phy_mem = request->vir_mem;
			} else {
				/*
				 * phys and virt buf space are not equal 
				 */
				request->phy_mem = tot_size -
					request->dst_len + request->src_len;
			}
		} else {
			request->cont_type = 0;
			request->hdr_alloc = 0;
			request->mem_alloc = 0;
			request->vir_mem = buf_size;
		}

		request->phy_mem = tot_size + request->mem_alloc;
	} else {
		/*
		 * Insert 
		 * dst_buf != NULL, src != NULL, dst_len == 0 
		 */
		request->cont_type = _UDI_BUF_C2;	/* Alloc /ds/memblk */
		request->hdr_alloc = sizeof (_udi_buf_container2_t);
		request->mem_alloc = request->src_len;
		/*
		 * See if we are inserting on a segment boundary 
		 */
		dataseg = request->dst_buf->buf_contents;
		offset = 0;
		while (dataseg && offset < request->dst_off) {
			offset += dataseg->bd_end - dataseg->bd_start;
			dataseg = dataseg->bd_next;
		}
		if (offset == request->dst_off) {
			/*
			 * we're at a segment boundary
			 */
			request->mem_alloc = request->src_len;
		} else if (offset < request->dst_off ||
			   (offset == 0 && buf_size == 0)) {
			/*
			 * Or if we are adding in the virtual memory section 
			 */
			request->mem_alloc =
				(request->dst_off + request->src_len) - offset;
		} else {
			/*
			 * We are not, so we need to alloc another segment 
			 */
			request->cont_type |= _UDI_BUF_C1;
			*xtr_size = sizeof (_udi_buf_dataseg_t);
		}
		request->phy_mem = tot_size + request->mem_alloc;
		request->vir_mem = buf_size + request->src_len;
	}
	if (request->phy_mem > request->vir_mem)
		request->phy_mem = request->vir_mem;
}

STATIC void
_udi_buf_copy_buf(_udi_buf_t *dst_buf,
		  udi_size_t dst_off,
		  udi_size_t dst_len,
		  _udi_buf_t *src_buf,
		  udi_size_t src_off)
{
	_udi_buf_dataseg_t *dst_seg = dst_buf->buf_contents;
	_udi_buf_dataseg_t *src_seg = src_buf->buf_contents;
	udi_size_t bytestocopy = dst_len;
	udi_size_t src_offset, dst_offset;
	udi_size_t src_segsize, dst_segsize;
	udi_size_t src_rem, dst_rem;

	src_offset = dst_offset = 0;

	/*
	 * First, find the beginning positions 
	 */

	src_segsize = src_seg->bd_end - src_seg->bd_start;
	while (src_segsize == 0 || src_offset + src_segsize <= src_off) {
		src_offset += src_segsize;
		src_seg = src_seg->bd_next;
		src_segsize = src_seg->bd_end - src_seg->bd_start;
	}
	src_offset = (src_off - src_offset);

	dst_segsize = dst_seg->bd_end - dst_seg->bd_start;
	while (dst_segsize == 0 || dst_offset + dst_segsize <= dst_off) {
		dst_offset += dst_segsize;
		dst_seg = dst_seg->bd_next;
		dst_segsize = dst_seg->bd_end - dst_seg->bd_start;
	}
	dst_offset = (dst_off - dst_offset);

	/*
	 * Now, do the copies, until we have copied everything 
	 */
	while (src_seg != NULL && bytestocopy) {
		if (dst_segsize == 0) {
			dst_offset += dst_segsize;
			dst_seg = dst_seg->bd_next;
			dst_segsize = dst_seg->bd_end - dst_seg->bd_start;
			continue;
		}
		if (src_segsize == 0) {
			src_offset += src_segsize;
			src_seg = src_seg->bd_next;
			if (src_seg)
				src_segsize =
					src_seg->bd_end - src_seg->bd_start;
			else
				src_segsize = 0;
			continue;
		}
		src_rem = src_segsize - src_offset;
		if (src_rem > bytestocopy)
			src_rem = bytestocopy;
		dst_rem = dst_segsize - dst_offset;
		if (dst_rem > bytestocopy)
			dst_rem = bytestocopy;
		if (src_rem > dst_rem) {
			/*
			 * Copy in the rest of the dst segment 
			 */
			udi_memcpy(((udi_ubit8_t *)dst_seg->bd_start) +
				   dst_offset,
				   ((udi_ubit8_t *)src_seg->bd_start) +
				   src_offset, dst_rem);
			dst_seg = dst_seg->bd_next;
			dst_segsize = dst_seg->bd_end - dst_seg->bd_start;
			dst_offset = 0;
			src_offset += dst_rem;
			bytestocopy -= dst_rem;
		} else if (src_rem < dst_rem) {
			/*
			 * Copy to the rest of the src segment 
			 */
			udi_memcpy(((udi_ubit8_t *)dst_seg->bd_start) +
				   dst_offset,
				   ((udi_ubit8_t *)src_seg->bd_start) +
				   src_offset, src_rem);
			src_seg = src_seg->bd_next;
			if (src_seg != NULL) {
				src_segsize =
					src_seg->bd_end - src_seg->bd_start;
			}
			src_offset = 0;
			dst_offset += src_rem;
			bytestocopy -= src_rem;
		} else {
			/*
			 * Segments of equal size 
			 */
			udi_memcpy(((udi_ubit8_t *)dst_seg->bd_start) +
				   dst_offset,
				   ((udi_ubit8_t *)src_seg->bd_start) +
				   src_offset, dst_rem);
			dst_seg = dst_seg->bd_next;
			if (dst_seg != NULL) {
				dst_segsize =
					dst_seg->bd_end - dst_seg->bd_start;
				dst_offset = 0;
			}
			bytestocopy -= src_rem;
		}
	}
	/*
	 * If we still needed more, it must be a backfill, due to an
	 * expansion of the buf_size.
	 */
	while (bytestocopy) {
		dst_rem = dst_segsize - dst_offset;
		if (dst_rem > bytestocopy)
			dst_rem = bytestocopy;
		/*
		 * Copy to the rest of the src bogus data 
		 */
		udi_memset(((udi_ubit8_t *)dst_seg->bd_start) + dst_offset,
			   _UDI_BF_CHAR, dst_rem);
		if (dst_seg->bd_next) {
			dst_seg = dst_seg->bd_next;
			dst_segsize = dst_seg->bd_end - dst_seg->bd_start;
			dst_offset = 0;
		}
		bytestocopy -= dst_rem;
	}
}

void
_udi_mem_overwrite_buf(const void *src_mem,
		       udi_size_t src_len,
		       _udi_buf_t *dst_buf,
		       udi_size_t dst_off,
		       udi_size_t dst_len)
{
	_udi_buf_dataseg_t *segment;
	udi_size_t seglen;
	udi_size_t tocopy;
	udi_size_t copied;
	udi_size_t offset;
	udi_size_t copysize;

	tocopy = src_len;
	segment = (_udi_buf_dataseg_t *)(dst_buf->buf_contents);
	seglen = (segment->bd_end - segment->bd_start);
	offset = copied = 0;

	/*
	 * Go to the first segment with data space 
	 */
	while (offset + seglen < dst_off || !seglen) {
		offset += seglen;
		segment = segment->bd_next;
		seglen = segment->bd_end - segment->bd_start;
	}

	/*
	 * The first copy is a special case, since it may not start at
	 * the beginning of the segment.
	 */
	copysize = offset + seglen - dst_off;
	if (copysize > tocopy)
		copysize = tocopy;
	if (copysize) {
		udi_memcpy(segment->bd_start + (dst_off - offset), src_mem,
			   copysize);
		copied += copysize;
		tocopy -= copysize;
	}
	/*
	 * Copy into each segment, until all data is copied 
	 */
	while (tocopy) {
		segment = segment->bd_next;
		copysize = segment->bd_end - segment->bd_start;
		if (copysize > tocopy)
			copysize = tocopy;
		udi_memcpy(segment->bd_start,
			   ((udi_ubit8_t *)src_mem) + copied, copysize);
		copied += copysize;
		tocopy -= copysize;
	}
}

#define	_UDI_BUF_FREE_DATASEG_ON_WRITE	0

/*
 * _udi_buf_wrcp_complete
 */
STATIC void
_udi_buf_wrcp_complete(_udi_alloc_marshal_t *allocm,
		       void *new_mem)
{
	STATIC void _udi_buf_segment_free(_udi_buf_t *new_mem,
					  _udi_buf_dataseg_t *freeds,
					  udi_boolean_t free_hdr);
	_udi_buf_t *bc_header;
	_udi_buf_dataseg_t *bc_dataseg, *bc_dataseg2, *dataseg,
		*ldataseg, *freeds;
	_udi_buf_memblk_t *bc_memblk;
	udi_ubit8_t *mem;
	udi_size_t offset, count;
	_udi_cb_t *cb = UDI_BASE_STRUCT(allocm, _udi_cb_t, cb_allocm);
	struct _buf_wrcp_request *request = &allocm->_u.buf_wrcp_request;
	_udi_buf_t *src_buf = request->src;
	udi_size_t orig_virt =
		(request->dst_buf == NULL) ?
		0 : ((udi_buf_t *)request->dst_buf)->buf_size;

	if (request->dst_buf == NULL) {
		/*
		 * Ensure space or Allocate / initialize 
		 * dst_buf == NULL, dst_len == 0, dst_off == 0 
		 * First, set up bc_header 
		 */
		_udi_buf_container3x_t *bc = (_udi_buf_container3x_t *)new_mem;

		bc_header = &bc->bc_header;
		bc_header->alloc_hdr.ah_type = _UDI_ALLOC_HDR_BUF;
		_udi_add_alloc_to_tracker(_UDI_CB_TO_CHAN(cb)->chan_region,
					  &bc_header->alloc_hdr);
		/*
		 * Then, set up bc_dataseg 
		 */
		bc_dataseg = &bc->bc_dataseg;
		/*
		 * Also set up bc_memblk 
		 */
		bc_memblk = &bc->bc_memblk;
		/*
		 * And the actual data space 
		 */
		mem = request->mem_alloc ? (udi_ubit8_t *)(bc + 1) : 0;
		/*
		 * Then, put in data 
		 */
		bc_header->buf_contents = bc_dataseg;
		bc_header->buf_ops = NULL;
		bc_header->buf_path = request->buf_path;
		bc_header->buf_tags = NULL;
#ifdef _UDI_PHYSIO_SUPPORTED
		bc_header->buf_dmah = NULL;
#endif
		_OSDEP_ATOMIC_INT_INIT(&bc_header->buf_refcnt, 1);
		bc_dataseg->bd_start =
			(request->mem_alloc ? mem + request->bh_alloc : 0);
		bc_dataseg->bd_end =
			(request->mem_alloc ? mem + request->mem_alloc -
			 request->bt_alloc : 0);
		bc_dataseg->bd_memblk = bc_memblk;
		bc_dataseg->bd_container = bc;
		bc_dataseg->bd_next = NULL;
		_OSDEP_ATOMIC_INT_INIT(&bc_memblk->bm_refcnt, 1);
		bc_memblk->bm_space = mem;
		bc_memblk->bm_size = request->mem_alloc;
		bc_memblk->bm_external = NULL;
		if (request->src != NULL && request->src_len != 0) {
			/*
			 * We have the request to initialize, so do it 
			 */
			if (request->req_type == _UDI_BUF_WRITE) {
				udi_memcpy(bc_dataseg->bd_start,
					   (char *)request->src,
					   request->src_len);
			} else {
				_udi_buf_copy_buf(new_mem, request->dst_off,
						  request->src_len, src_buf,
						  request->src_off);
			}
		}
	} else if (request->src == NULL) {
		/*
		 * Delete 
		 * dst_buf != NULL, src == NULL, src_len == 0 
		 * Go to the first segment where the delete needs to occur 
		 */
		dataseg = request->dst_buf->buf_contents;
		offset = 0;
		while (dataseg &&
		       (dataseg->bd_end - dataseg->bd_start) + offset <
		       request->dst_off) {
			offset += dataseg->bd_end - dataseg->bd_start;
			dataseg = dataseg->bd_next;
		}
		if (request->cont_type == _UDI_BUF_C1) {
			/*
			 * Delete a middle portion of a segment 
			 */
			bc_dataseg = new_mem;
			bc_dataseg->bd_start =
				dataseg->bd_start + (request->dst_off +
						     request->dst_len) -
				offset;
			bc_dataseg->bd_end = dataseg->bd_end;
			bc_dataseg->bd_memblk = dataseg->bd_memblk;
			bc_dataseg->bd_container = new_mem;
			bc_dataseg->bd_next = dataseg->bd_next;
			dataseg->bd_next = bc_dataseg;
			dataseg->bd_end = bc_dataseg->bd_start - 1;
			_OSDEP_ATOMIC_INT_INCR(&dataseg->bd_memblk->bm_refcnt);
			dataseg = NULL;
		}
		if (dataseg) {
			/*
			 * Go through the segs, and adjust 
			 */
			count = request->dst_len;
			offset = request->dst_off - offset;
			while (dataseg && count) {
				if (!offset) {
					if (dataseg->bd_end -
					    dataseg->bd_start < count) {
						/*
						 * Remove whole segment 
						 */
						count -= dataseg->bd_end -
							dataseg->bd_start;
						dataseg->bd_end = NULL;
						dataseg->bd_start = NULL;
					} else {
						/*
						 * Adjust the front of seg 
						 */
						dataseg->bd_start += count;
						count = 0;
					}
				} else {
					/*
					 * Clip end of first segment 
					 */
					count -= (dataseg->bd_end -
						  dataseg->bd_start) - offset;
					dataseg->bd_end =
						dataseg->bd_start + offset;
				}
				dataseg = dataseg->bd_next;
				offset = 0;
			}
		}
		new_mem = request->dst_buf;
	} else if (request->dst_len) {
		/*
		 * Overwrite / Replace
		 * dst_buf != NULL,src != NULL, dst_len > 0, src_len != 0 
		 * First, initialize the dataseg, if we allocated memory 
		 */
		_udi_buf_container2_t *bc;

		if (request->mem_alloc) {
			if (request->fmem_alloc)
				bc = (_udi_buf_container2_t *)
					request->fmem_alloc;
			else
				bc = (_udi_buf_container2_t *) new_mem;
			bc_dataseg = &bc->bc_dataseg;

			if (request->cont_type & _UDI_BUF_C1) {
				if (request->fmem_alloc) {
					bc_dataseg2 = new_mem;
				} else {
					/*
					 * If we requested extra memory which we
					 * have not gotten yet, we are in the 
					 * alloc daemon thread, and can block 
					 * waiting for the additional memory.
					 */
					bc_dataseg2 =
						_OSDEP_MEM_ALLOC(sizeof
								 (_udi_buf_dataseg_t),
								 UDI_MEM_NOZERO,
								 UDI_WAITOK);
				}
			} else {
				bc_dataseg2 = NULL;
			}

			ldataseg = NULL;
			dataseg = request->dst_buf->buf_contents;
			offset = 0;
			while (dataseg &&
			       offset + (dataseg->bd_end - dataseg->bd_start)
			       <= request->dst_off) {
				offset +=
					(dataseg->bd_end - dataseg->bd_start);
				ldataseg = dataseg;
				dataseg = dataseg->bd_next;
			}
			if (!bc_dataseg2) {
				if (!dataseg) {
					bc_dataseg->bd_next = NULL;
					ldataseg->bd_next = bc_dataseg;
				} else if (offset == request->dst_off) {
					/*
					 * The easy beginning of offset case 
					 */
					/*
					 * Point the prev segment to the new 
					 * segment 
					 */
					if (ldataseg) {
						bc_dataseg->bd_next = dataseg;
						ldataseg->bd_next = bc_dataseg;
					} else {
						/*
						 * dataseg points to first 
						 * segment of buffer, 
						 * dst_off == 0
						 */
						bc_dataseg->bd_next = dataseg;
						request->dst_buf->
							buf_contents =
							bc_dataseg;
					}
				}
			} else {
				/*
				 * We have a bc_dataseg2 
				 */
				/*
				 * Populate the new dataseg 
				 */
				bc_dataseg2->bd_start =
					dataseg->bd_start + (request->dst_off -
							     offset);
				bc_dataseg2->bd_end = dataseg->bd_end;
				bc_dataseg2->bd_memblk = dataseg->bd_memblk;
				bc_dataseg2->bd_container = bc_dataseg2;
				bc_dataseg2->bd_next = dataseg->bd_next;
				bc_dataseg->bd_next = bc_dataseg2;
				/*
				 * And modify the endpoint and ref cnt 
				 */
				dataseg->bd_end =
					dataseg->bd_start + (request->dst_off -
							     offset);
				_OSDEP_ATOMIC_INT_INCR(&dataseg->bd_memblk->
						       bm_refcnt);
				dataseg->bd_next = bc_dataseg;
			}

			/*
			 * Populate the new dataseg/memblk 
			 */
			bc_memblk = &bc->bc_memblk;
			mem = (udi_ubit8_t *)(bc + 1);

			bc_dataseg->bd_start = mem;
			/*
			 * bc_dataseg->bd_end = mem + request->src_len - 
			 *                            request->dst_len; 
			 */
			bc_dataseg->bd_end = mem + request->mem_alloc;
			bc_dataseg->bd_memblk = bc_memblk;
			bc_dataseg->bd_container = bc;

			_OSDEP_ATOMIC_INT_INIT(&bc_memblk->bm_refcnt, 1);
			bc_memblk->bm_space = mem;
			/*
			 * bc_memblk->bm_size = request->src_len - 
			 *                      request->dst_len; 
			 */
			bc_memblk->bm_size = request->mem_alloc;
			bc_memblk->bm_external = NULL;
		} else if (request->src_len < request->dst_len) {
			/*
			 * Delete part of a segment
			 */
			dataseg = request->dst_buf->buf_contents;
			offset = 0;
			while (dataseg &&
			       (dataseg->bd_end - dataseg->bd_start) + offset <
			       request->dst_off) {
				offset += dataseg->bd_end - dataseg->bd_start;
				dataseg = dataseg->bd_next;
			}
			if (request->cont_type == _UDI_BUF_C1) {
				/*
				 * Delete a middle portion of a segment 
				 */
				bc_dataseg = new_mem;
				bc_dataseg->bd_start =
					dataseg->bd_start + (request->dst_off +
							     request->dst_len -
							     request->
							     src_len) - offset;
				bc_dataseg->bd_end = dataseg->bd_end;
				bc_dataseg->bd_memblk = dataseg->bd_memblk;
				bc_dataseg->bd_container = new_mem;
				bc_dataseg->bd_next = dataseg->bd_next;
				dataseg->bd_next = bc_dataseg;
				dataseg->bd_end = bc_dataseg->bd_start - 1;
				_OSDEP_ATOMIC_INT_INCR(&dataseg->bd_memblk->
						       bm_refcnt);
				dataseg = NULL;
			}
			if (dataseg) {
				/*
				 * Go through the segs, and adjust 
				 */
				count = request->dst_len;
				offset = request->dst_off - offset;
				while (dataseg && count) {
					if (!offset) {
						if (dataseg->bd_end -
						    dataseg->bd_start <
						    count) {
							/*
							 * Remove whole segment 
							 */
							count -= dataseg->
								bd_end -
								dataseg->
								bd_start;
							dataseg->bd_end = NULL;
							dataseg->bd_start =
								NULL;
						} else {
							/*
							 * Adjust the front of
							 * segment
							 */
							dataseg->bd_start +=
								count;
							count = 0;
						}
					} else {
						/*
						 * Clip end of first segment 
						 */
						count -= (dataseg->bd_end -
							  dataseg->bd_start) -
							offset;
						dataseg->bd_end =
							dataseg->bd_start +
							offset;
					}
					dataseg = dataseg->bd_next;
					offset = 0;
				}
			}
		}

		/*
		 * Now, do the copy 
		 */
		if (request->req_type == _UDI_BUF_WRITE) {
			_udi_mem_overwrite_buf(request->src, request->src_len,
					       request->dst_buf,
					       request->dst_off,
					       request->dst_len);
		} else {
			_udi_buf_copy_buf(request->dst_buf, request->dst_off,
					  request->src_len, src_buf,
					  request->src_off);
		}
		new_mem = request->dst_buf;
	} else {
		/*
		 * Insert 
		 *
		 * (Note: bc_dataseg should be in a container2, and
		 * bc_dataseg2, if present, should be in a container1
		 * [dataseg only].)
		 */
		_udi_buf_container2_t *bc;

		if (request->fmem_alloc)
			bc = (_udi_buf_container2_t *) request->fmem_alloc;
		else
			bc = (_udi_buf_container2_t *) new_mem;
		bc_dataseg = &bc->bc_dataseg;

		/*
		 * dst_buf != NULL, src != NULL, dst_len == 0 
		 */
		if (request->cont_type & _UDI_BUF_C1) {
			if (request->fmem_alloc) {
				bc_dataseg2 = new_mem;
			} else {
				/*
				 * If we requested extra memory which we have
				 * not gotten yet, we are in the alloc daemon
				 * thread, and can block waiting for the
				 * additional memory.
				 */
				bc_dataseg2 =
					_OSDEP_MEM_ALLOC(sizeof
							 (_udi_buf_dataseg_t),
							 UDI_MEM_NOZERO,
							 UDI_WAITOK);
			}
		} else {
			bc_dataseg2 = NULL;
		}

		ldataseg = NULL;
		dataseg = request->dst_buf->buf_contents;
		offset = 0;
		while (dataseg &&
		       offset + (dataseg->bd_end - dataseg->bd_start) <=
		       request->dst_off) {
			offset += (dataseg->bd_end - dataseg->bd_start);
			ldataseg = dataseg;
			dataseg = dataseg->bd_next;
		}
		if (!bc_dataseg2) {
			if (!dataseg) {
				bc_dataseg->bd_next = NULL;
				ldataseg->bd_next = bc_dataseg;
			} else if (offset == request->dst_off) {
				/*
				 * The easy beginning of offset case 
				 */
				/*
				 * Point the prev segment to the new segment 
				 */
				if (ldataseg) {
					bc_dataseg->bd_next = dataseg;
					ldataseg->bd_next = bc_dataseg;
				} else {
					bc_dataseg->bd_next = dataseg;
					request->dst_buf->buf_contents =
						bc_dataseg;
				}
			}
		} else {
			/*
			 * We have a bc_dataseg2 
			 */
			/*
			 * Populate the new dataseg 
			 */
			bc_dataseg2->bd_start =
				dataseg->bd_start + (request->dst_off -
						     offset);
			bc_dataseg2->bd_end = dataseg->bd_end;
			bc_dataseg2->bd_memblk = dataseg->bd_memblk;
			bc_dataseg2->bd_container = bc_dataseg2;
			bc_dataseg2->bd_next = dataseg->bd_next;
			bc_dataseg->bd_next = bc_dataseg2;
			/*
			 * And modify the endpoint and ref cnt 
			 */
			dataseg->bd_end =
				dataseg->bd_start + (request->dst_off -
						     offset);
			_OSDEP_ATOMIC_INT_INCR(&dataseg->bd_memblk->bm_refcnt);
			dataseg->bd_next = bc_dataseg;
		}

		/*
		 * Populate the new dataseg/memblk 
		 */
		bc_memblk = &bc->bc_memblk;
		mem = (udi_ubit8_t *)(bc + 1);

		bc_dataseg->bd_start = mem;
		/*
		 * bc_dataseg->bd_end = mem + request->src_len; 
		 */
		bc_dataseg->bd_end = mem + request->mem_alloc;
		bc_dataseg->bd_memblk = bc_memblk;
		bc_dataseg->bd_container = bc;

		_OSDEP_ATOMIC_INT_INIT(&bc_memblk->bm_refcnt, 1);
		bc_memblk->bm_space = mem;
		/*
		 * bc_memblk->bm_size = request->src_len; 
		 */
		bc_memblk->bm_size = request->mem_alloc;
		bc_memblk->bm_external = NULL;

		/*
		 * Copy the new data into the new buffer 
		 */
		if (request->req_type == _UDI_BUF_WRITE) {
			if (dataseg) {
				udi_memcpy(mem, request->src,
					   request->src_len);
			} else {
				udi_memcpy(mem + (request->dst_off - offset),
					   request->src, request->src_len);
			}
		} else {
			_udi_buf_copy_buf(request->dst_buf, request->dst_off,
					  request->src_len, src_buf,
					  request->src_off);
		}
		new_mem = request->dst_buf;
	}

	/*
	 * And adjust the real + virt buffer sizes 
	 */
	((_udi_buf_t *)new_mem)->buf_total_size = request->phy_mem;
	((_udi_buf_t *)new_mem)->buf.buf_size = request->vir_mem;

	/*
	 * If the buffer size got smaller, remove any invalid tags.
	 * Also, modify any segment size beyond the new virtual, to be 0
	 * and remove and free unnecessary dataseg/mblks.
	 */
	if (orig_virt > ((udi_buf_t *)new_mem)->buf_size) {
		if (((_udi_buf_t *)new_mem)->buf_tags != NULL)
			_udi_buf_rem_inv_tags(new_mem);

		freeds = NULL;		/* If below #if removed, remove this too */
#if _UDI_BUF_FREE_DATASEG_ON_WRITE
		ldataseg = NULL;
		dataseg = ((_udi_buf_t *)new_mem)->buf_contents;
		offset = 0;

		while (dataseg != NULL) {
			freeds = NULL;
			if (dataseg->bd_end - dataseg->bd_start == 0 ||
			    ((udi_buf_t *)new_mem)->buf_size < offset) {
				freeds = dataseg;
				if (ldataseg)
					ldataseg->bd_next = dataseg->bd_next;
				else
					((_udi_buf_dataseg_t *)(((_udi_buf_t *)
								 new_mem)->
								buf_contents))->
						bd_next = dataseg->bd_next;
			} else {
				ldataseg = dataseg;
				offset += dataseg->bd_end - dataseg->bd_start;
			}
			dataseg = dataseg->bd_next;
			if (freeds) {
				_udi_buf_segment_free(new_mem, freeds, FALSE);
			}
		}
#endif /* _UDI_BUF_FREE_DATASEG_ON_WRITE */
	}

	/*
	 * All cases fall through here to set the result 
	 */
	if (request->req_type == _UDI_BUF_WRITE) {
		SET_RESULT_UDI_CT_BUF_WRITE(allocm, new_mem);
	} else {
		SET_RESULT_UDI_CT_BUF_COPY(allocm, new_mem);
	}
}

void
udi_buf_write(udi_buf_write_call_t *callback,
	      udi_cb_t *_gcb,
	      const void *src_mem,
	      udi_size_t src_len,
	      udi_buf_t *_dst_buf,
	      udi_size_t dst_off,
	      udi_size_t dst_len,
	      udi_buf_path_t _buf_path)
{
	void *new_mem;
	udi_size_t xtr_size;

	_udi_buf_t *dst_buf = (_udi_buf_t *)_dst_buf;
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(_gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	_udi_buf_path_t *buf_path = _UDI_HANDLE_TO_BUF_PATH(_buf_path);

#if _UDI_PHYSIO_SUPPORTED
	_udi_dma_constraints_t *constraints;
#endif /* _UDI_PHYSIO_SUPPORTED */
	struct _buf_wrcp_request *request = &allocm->_u.buf_wrcp_request;

#if TRACKER
	_udi_alloc_hdr_t *ah;
#endif

	/*
	 * Obtain the logical (buf_size) and total (buf_total_size) sizes 
	 */
	udi_size_t buf_size = (_dst_buf == NULL) ? 0 : _dst_buf->buf_size;
	udi_size_t tot_size = (_dst_buf == NULL) ? 0 : dst_buf->buf_total_size;

	/*
	 * Make sure the request is valid 
	 */
	_OSDEP_ASSERT(dst_off >= 0 && dst_off <= buf_size);
	_OSDEP_ASSERT(dst_len >= 0 && dst_len <= buf_size - dst_off);
	_OSDEP_ASSERT(dst_buf != NULL || (dst_off == 0 && dst_len == 0));
	_OSDEP_ASSERT(buf_path != NULL || dst_buf != NULL);

	/*
	 * if this path is not associated with constratints yet then set it
	 * to a default set
	 */
	if (buf_path != NULL) {
#if _UDI_PHYSIO_SUPPORTED
		constraints = buf_path->constraints;
#endif /* _UDI_PHYSIO_SUPPORTED */
	} else {
		buf_path = dst_buf->buf_path;
#if _UDI_PHYSIO_SUPPORTED
		constraints = buf_path->constraints;
#endif /* _UDI_PHYSIO_SUPPORTED */
	}
#if _UDI_PHYSIO_SUPPORTED
	if (constraints == NULL) {
		constraints = _udi_default_dma_constraints;
	}
#endif /* _UDI_PHYSIO_SUPPORTED */

#if TRACKER
	ah = _udi_find_alloc_in_tracker(_UDI_CB_TO_CHAN(cb)->chan_region,
					(void *)src_mem);

/*
 * I think that something like this really is a good idea, but right 
 * now the precise definition of movable memory is under debate and this
 * has the unfortunate effect of exposing a deadlock condition in the 
 * env_log_write code if the allocation of the cb is deferred...
 * 06/09/00 robertl
 */
	if (src_mem &&
	    ((ah == NULL) || (0 == (ah->ah_flags & UDI_MEM_MOVABLE)))) {
		/*
		 * We were called with a src_mem that wasn't movable.
		 * Unfortunately, we may not have a dst_buf yet (we may
		 * be allocating one) so it's hard to track this.
		 */
		if (dst_buf && !dst_buf->movable_warning_issued) {
			dst_buf->movable_warning_issued = TRUE;
		}
		_udi_env_log_write(_UDI_TREVENT_ENVERR,
				   UDI_LOG_ERROR, 0,
				   UDI_STAT_NOT_UNDERSTOOD,
				   926, dst_buf, src_mem);
	}
#endif

	/*
	 * Cover the "does nothing" case first 
	 */
	if (dst_buf != NULL && src_len == 0 && dst_len == 0) {
		SET_RESULT_UDI_CT_BUF_WRITE(allocm, _dst_buf);
		_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_BUF_WRITE,
				      &_udi_buf_wrcp_ops, callback, 1,
				      (allocm->_u.buf_result.new_buf));
		return;
	}

	/*
	 * Set up various fields for use in _udi_buf_calc_mem 
	 */
#if _UDI_PHYSIO_SUPPORTED
	request->constraints = constraints;
#endif /* _UDI_PHYSIO_SUPPORTED */
	request->buf_path = buf_path;
	request->src = (void *)src_mem;
	request->src_len = src_len;
	request->dst_buf = dst_buf;
	request->dst_off = dst_off;
	request->dst_len = dst_len;
	request->cont_type = 0;
	request->req_type = _UDI_BUF_WRITE;

	/*
	 * Determine the amount/kind of memory which needs to be allocated
	 */
	_udi_buf_calc_mem(request, buf_size, tot_size, &xtr_size);

	/*
	 * Try to allocate the required memory 
	 */
	request->fmem_alloc = NULL;
	if (request->hdr_alloc || request->mem_alloc) {
		/*
		 * Allocate the needed memory 
		 */
		new_mem =
			_OSDEP_MEM_ALLOC(request->hdr_alloc +
					 request->mem_alloc, UDI_MEM_NOZERO,
					 UDI_NOWAIT);
		if (new_mem == NULL) {
			/*
			 * Queue up the memory request 
			 */
			_UDI_ALLOC_QUEUE(cb, _UDI_CT_BUF_WRITE, callback,
					 request->hdr_alloc +
					 request->mem_alloc,
					 &_udi_buf_wrcp_ops);
			return;
		}
	} else {
		new_mem = NULL;
	}
	/*
	 * Try to allocate additional required memory 
	 */
	if (xtr_size) {
		request->fmem_alloc = new_mem;
		new_mem =
			_OSDEP_MEM_ALLOC(xtr_size, UDI_MEM_NOZERO, UDI_NOWAIT);
		if (new_mem == NULL) {
			/*
			 * Queue up the memory request 
			 */
			_UDI_ALLOC_QUEUE(cb, _UDI_CT_BUF_WRITE, callback,
					 xtr_size, &_udi_buf_wrcp_ops);
			return;
		}
	}

	/*
	 * Do the appropriate thing to complete the request, non-blocking.
	 * If we reached here, we already have all the memory we need.
	 */
	_udi_buf_wrcp_complete(allocm, new_mem);
	_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_BUF_WRITE, &_udi_buf_wrcp_ops,
			      callback, 1, (allocm->_u.buf_result.new_buf));
}

void
udi_buf_copy(udi_buf_write_call_t *callback,
	     udi_cb_t *_gcb,
	     udi_buf_t *_src_buf,
	     udi_size_t src_off,
	     udi_size_t src_len,
	     udi_buf_t *_dst_buf,
	     udi_size_t dst_off,
	     udi_size_t dst_len,
	     udi_buf_path_t _buf_path)
{
	void *new_mem;
	udi_size_t xtr_size;

	_udi_buf_t *dst_buf = (_udi_buf_t *)_dst_buf;
	_udi_buf_t *src_buf = (_udi_buf_t *)_src_buf;
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(_gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	_udi_buf_path_t *buf_path = _UDI_HANDLE_TO_BUF_PATH(_buf_path);

#if _UDI_PHYSIO_SUPPORTED
	_udi_dma_constraints_t *constraints;
#endif /* _UDI_PHYSIO_SUPPORTED */
	struct _buf_wrcp_request *request = &allocm->_u.buf_wrcp_request;

	/*
	 * Obtain the logical (buf_size) and total (buf_total_size) sizes 
	 */
	udi_size_t buf_size = (_dst_buf == NULL) ? 0 : _dst_buf->buf_size;
	udi_size_t tot_size = (_dst_buf == NULL) ? 0 : dst_buf->buf_total_size;
	udi_size_t sbuf_size = (_src_buf == NULL) ? 0 : _src_buf->buf_size;

	/*
	 * Make sure the request is valid 
	 */
	_OSDEP_ASSERT(src_off >= 0 && src_off < sbuf_size);
	_OSDEP_ASSERT(src_len > 0 && src_len <= sbuf_size - src_off);
	_OSDEP_ASSERT(dst_off >= 0 && dst_off <= buf_size);
	_OSDEP_ASSERT(dst_len >= 0 && dst_len <= buf_size - dst_off);
	_OSDEP_ASSERT(dst_buf != NULL || (dst_off == 0 && dst_len == 0));

	if (buf_path != NULL) {
#if _UDI_PHYSIO_SUPPORTED
		constraints = buf_path->constraints;
#endif /* _UDI_PHYSIO_SUPPORTED */
	} else {
		buf_path = dst_buf->buf_path;
#if _UDI_PHYSIO_SUPPORTED
		constraints = buf_path->constraints;
#endif /* _UDI_PHYSIO_SUPPORTED */
	}
#if _UDI_PHYSIO_SUPPORTED
	if (constraints == NULL) {
		constraints = _udi_default_dma_constraints;
	}
	_OSDEP_ASSERT(buf_path != NULL || constraints != NULL);
#endif /* _UDI_PHYSIO_SUPPORTED */
	/*
	 * Cover the "does nothing" case first 
	 */
	if (dst_buf != NULL && src_len == 0 && dst_len == 0) {
		SET_RESULT_UDI_CT_BUF_WRITE(allocm, _dst_buf);
		_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_BUF_COPY, &_udi_buf_wrcp_ops,
				      callback, 1,
				      (allocm->_u.buf_result.new_buf));
		return;
	}

	/*
	 * Set up various fields for use in _udi_buf_calc_mem 
	 */
#if _UDI_PHYSIO_SUPPORTED
	request->constraints = constraints;
#endif /* _UDI_PHYSIO_SUPPORTED */
	request->buf_path = buf_path;
	request->src = (void *)src_buf;
	request->src_off = src_off;
	request->src_len = src_len;
	request->dst_buf = dst_buf;
	request->dst_off = dst_off;
	request->dst_len = dst_len;
	request->cont_type = 0;
	request->req_type = _UDI_BUF_COPY;

	/*
	 * Determine the amount/kind of memory which needs to be allocated
	 */
	_udi_buf_calc_mem(request, buf_size, tot_size, &xtr_size);

	/*
	 * Try to allocate the required memory 
	 */
	request->fmem_alloc = NULL;
	if (request->hdr_alloc || request->mem_alloc) {
		/*
		 * Allocate the needed memory 
		 */
		new_mem =
			_OSDEP_MEM_ALLOC(request->hdr_alloc +
					 request->mem_alloc, UDI_MEM_NOZERO,
					 UDI_NOWAIT);
		if (new_mem == NULL) {
			/*
			 * Queue up the memory request 
			 */
			_UDI_ALLOC_QUEUE(cb, _UDI_CT_BUF_COPY, callback,
					 request->hdr_alloc +
					 request->mem_alloc,
					 &_udi_buf_wrcp_ops);
			return;
		}
	} else {
		new_mem = NULL;
	}
	/*
	 * Try to allocate additional required memory 
	 */
	if (xtr_size) {
		request->fmem_alloc = new_mem;
		new_mem =
			_OSDEP_MEM_ALLOC(xtr_size, UDI_MEM_NOZERO, UDI_NOWAIT);
		if (new_mem == NULL) {
			/*
			 * Queue up the memory request 
			 */
			_UDI_ALLOC_QUEUE(cb, _UDI_CT_BUF_COPY, callback,
					 xtr_size, &_udi_buf_wrcp_ops);
			return;
		}
	}

	/*
	 * Do the appropriate thing to complete the request, non-blocking.
	 * If we reached here, we already have all the memory we need.
	 */
	_udi_buf_wrcp_complete(allocm, new_mem);
	_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_BUF_WRITE, &_udi_buf_wrcp_ops,
			      callback, 1, (allocm->_u.buf_result.new_buf));
}

STATIC udi_boolean_t
_udi_buf_is_external(_udi_buf_t *buf)
{
	_udi_buf_memblk_t *memblk;
	_udi_buf_dataseg_t *dataseg = (_udi_buf_dataseg_t *)
		(buf->buf_contents);

	while (dataseg != NULL) {
		if (dataseg->bd_start != dataseg->bd_end) {
			/*
			 * It's a non-zero size segment, so check it 
			 */
			memblk = dataseg->bd_memblk;
			if (memblk->bm_external)
				return (TRUE);
			else
				return (FALSE);
		}
		dataseg = dataseg->bd_next;
	}
	return (FALSE);
}

STATIC void
_udi_buf_segment_free(_udi_buf_t *buf,
		      _udi_buf_dataseg_t *curseg,
		      udi_boolean_t free_hdr)
{
	udi_ubit32_t refcnt;
	udi_boolean_t valid_segment_type = FALSE;
	_udi_buf_memblk_t *memblk = curseg->bd_memblk;
	void *bc = curseg->bd_container;

	if (free_hdr || &((_udi_buf_container3_t *) bc)->bc_dataseg != curseg) {
		/*
		 * Decrement the memory reference count 
		 */
		if (_OSDEP_ATOMIC_INT_READ(&memblk->bm_refcnt))
			_OSDEP_ATOMIC_INT_DECR(&memblk->bm_refcnt);
	}
	refcnt = _OSDEP_ATOMIC_INT_READ(&memblk->bm_refcnt);

	/*
	 * If the memblk has a zero reference count, remove refcnt
	 * and deallocate any external memory.
	 */
	if (refcnt == 0) {
		_OSDEP_ATOMIC_INT_DEINIT(&memblk->bm_refcnt);
		if (memblk->bm_external) {
			((*(memblk->bm_external->xbop_free))) (*(void **)
							       (curseg + 1),
							       memblk->
							       bm_space,
							       memblk->
							       bm_size);
		}
	}

	if (&((_udi_buf_container1_t *) bc)->bc_dataseg == curseg) {
		/*
		 * container is _udi_buf_container1_t or 1x_t 
		 */
		if (refcnt == 0) {
			/*
			 * memblk pointed to is finished 
			 */
			_udi_buf_dataseg_t *ds_container =
				(void *)(memblk + 1);
			_OSDEP_MEM_FREE(ds_container->bd_container);
		}
		_OSDEP_MEM_FREE(bc);
	} else if (&((_udi_buf_container2_t *) bc)->bc_dataseg == curseg) {
		/*
		 * container is _udi_buf_container2_t or 2x_t 
		 */
		if (refcnt == 0) {
			_OSDEP_MEM_FREE(bc);
		}
	} else if (&((_udi_buf_container3_t *) bc)->bc_dataseg == curseg) {
		/*
		 * container is _udi_buf_container3_t or 3x_t 
		 */
		if (free_hdr) {
			if (refcnt == 0) {
				_OSDEP_MEM_FREE(bc);
			}
		}
	} else {
		_OSDEP_ASSERT(valid_segment_type);
	}
}

void
udi_buf_free(udi_buf_t *_buf)
{
	_udi_buf_dataseg_t *curseg;
	_udi_buf_dataseg_t *nextseg;
	_udi_buf_t *buf = (_udi_buf_t *)_buf;
	_udi_buf_tag_t *tag, *oldtag;

	if ((udi_buf_t *)buf == NULL)
		return;

	if (_OSDEP_ATOMIC_INT_READ(&buf->buf_refcnt))
		_OSDEP_ATOMIC_INT_DECR(&buf->buf_refcnt);

	/*
	 * Check the reference count 
	 */
	if (!_OSDEP_ATOMIC_INT_READ(&buf->buf_refcnt)) {
		/*
		 * Free up the buffer reference count for the mem 
		 */
		_OSDEP_ATOMIC_INT_DEINIT(&buf->buf_refcnt);
	} else {
		/*
		 * If there is still a reference count, just return 
		 */
		return;
	}

	/*
	 * Free up any buffer tags.  They were allocated either via
	 * _OSDEP_MEM_ALLOC or via the callback allocation mechanism.
	 */

	tag = buf->buf_tags;
	while (tag != NULL) {
		oldtag = tag;
		tag = tag->bt_next;
		_OSDEP_MEM_FREE(oldtag);
	}

	/*
	 * Because of the number of places that buffer containers are 
	 * allocated (udi_dma_mem_to_buf, external mappers, etc.) it's 
	 * possible that we'll try to release one that isn't specifically 
	 * marked with an alloc header.   So only release what we have.
	 */
	if (!_udi_buf_is_external(buf)) {
		_udi_rm_alloc_from_tracker(&buf->alloc_hdr);
	}

	/*
	 * Travel the linked list of bc_dataseg structures, freeing
	 * stuff as we go.
	 */
	nextseg = (_udi_buf_dataseg_t *)(buf->buf_contents);
	while (nextseg) {
		curseg = nextseg;
		nextseg = nextseg->bd_next;
		_udi_buf_segment_free(buf, curseg, TRUE);
	}
}

void
udi_buf_read(udi_buf_t *_src_buf,
	     udi_size_t src_off,
	     udi_size_t src_len,
	     void *dst_mem)
{
	udi_size_t offset;
	udi_size_t copied;
	udi_size_t tocopy;
	udi_size_t copysize;
	_udi_buf_dataseg_t *segment;
	udi_size_t len, ext, shr;
	_udi_buf_t *src_buf = (_udi_buf_t *)_src_buf;
	udi_size_t buf_size = _src_buf->buf_size;

	ext = shr = 0;

	/*
	 * For empty buffers, or buf_size of 0, just return 
	 */
	if (_src_buf == NULL || buf_size == 0)
		return;

	_OSDEP_ASSERT(src_off + src_len <= buf_size);

	/*
	 * Check if we have a modified buf_size 
	 */
	if (buf_size != src_buf->buf_total_size) {
		if (buf_size > src_buf->buf_total_size) {
			/*
			 * How much did we increase it by? 
			 */
			ext = buf_size - src_buf->buf_total_size;
		} else {
			/*
			 * How much did we shrink it by? 
			 */
			shr = src_buf->buf_total_size - buf_size;
		}
	}

	if (src_buf->buf_total_size > src_off) {
		/*
		 * Find the first data segment 
		 */

		segment = (_udi_buf_dataseg_t *)(src_buf->buf_contents);
		offset = 0;

		len = (segment->bd_end - segment->bd_start);
		/*
		 * Go to the first segment to be read from 
		 */

		while (offset + len < src_off || !len) {
			offset += len;
			segment = segment->bd_next;
			len = (segment->bd_end - segment->bd_start);
		}
		_OSDEP_ASSERT(offset <= buf_size);

		/*
		 * The first copy is a special case, since it may not start at
		 * the beginning of the segment.
		 */
		copied = 0;
		tocopy = src_len;
		if (src_off + src_len > buf_size)
			tocopy = buf_size;
		if (offset + len - src_off <= buf_size)
			copysize = offset + len - src_off;
		else
			copysize = buf_size - offset;

		if (copysize > src_len)
			copysize = src_len;
		udi_memcpy(dst_mem, segment->bd_start + (src_off - offset),
			   copysize);
		copied += copysize;
		tocopy -= copysize;

		/*
		 * Copy from each segment to memory, until all data is copied
		 * or we have no more segments to read.
		 */
		while (tocopy && segment->bd_next) {
			segment = segment->bd_next;
			copysize = (segment->bd_end - segment->bd_start);
			if (copysize > tocopy)
				copysize = tocopy;
			udi_memcpy(((char *)dst_mem) + copied,
				   segment->bd_start, copysize);
			copied += copysize;
			tocopy -= copysize;
		}
	} else {
		copied = 0;
		tocopy = src_len;
	}

	/*
	 * If the buf_size was increased, and we still need more data,
	 * ext will have the amount of the increase.
	 */

	if (tocopy && ext) {
		if (tocopy < ext)
			ext = tocopy;
		/*
		 * Copy extra data, since we are returning data for an as-yet
		 * unallocated portion of the buffer.
		 */
		udi_memset(((char *)dst_mem) + copied, _UDI_BF_CHAR, ext);
		copied += ext;
	}
}

STATIC void
_udi_buf_xinit_complete(_udi_alloc_marshal_t *allocm,
			void *new_mem)
{
	void *xmem = allocm->_u.buf_xinit_request.src_mem;
	udi_size_t len = allocm->_u.buf_xinit_request.src_len;
	_udi_xbops_t *xbops = allocm->_u.buf_xinit_request.xbops;
	void *xbops_context = allocm->_u.buf_xinit_request.xbops_context;
	_udi_buf_container3x_t *bc = new_mem;
	_udi_buf_t *buf = &bc->bc_header;
	_udi_buf_dataseg_t *dataseg = &bc->bc_dataseg;
	_udi_buf_memblk_t *memblk = &bc->bc_memblk;

	/*
	 * Fill in the header 
	 */
	buf->buf.buf_size = len;
	buf->buf_contents = dataseg;
	buf->buf_ops = NULL;
	buf->buf_path = allocm->_u.buf_xinit_request.buf_path;
	buf->buf_total_size = len;
	buf->buf_tags = NULL;
#ifdef _UDI_PHYSIO_SUPPORTED
	buf->buf_dmah = NULL;
#endif
	_OSDEP_ATOMIC_INT_INIT(&buf->buf_refcnt, 1);

	dataseg->bd_start = xmem;
	dataseg->bd_end = ((udi_ubit8_t *)xmem) + len;
	dataseg->bd_memblk = memblk;
	dataseg->bd_container = bc;
	dataseg->bd_next = NULL;

	_OSDEP_ATOMIC_INT_INIT(&memblk->bm_refcnt, 1);
	memblk->bm_space = xmem;
	memblk->bm_size = len;
	memblk->bm_external = xbops;

	bc->bc_xmem_context = xbops_context;

	SET_RESULT_UDI_CT_BUF_XINIT(allocm, new_mem);
}

/*
 * Passed a memory pointer, a length, and a ptr to the set of _udi_xbops_t,
 * Set up a buffer which points to the memory.
 */
void
_udi_buf_extern_init(udi_buf_write_call_t *callback,
		     udi_cb_t *_gcb,
		     void *mem,
		     udi_size_t len,
		     _udi_xbops_t *xbops,
		     void *xbops_context,
		     udi_buf_path_t buf_path)
{
	void *new_mem;

	_udi_cb_t *cb = _UDI_CB_TO_HEADER(_gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;

	/*
	 * Set up marshalled parameters 
	 */
	allocm->_u.buf_xinit_request.src_mem = mem;
	allocm->_u.buf_xinit_request.src_len = len;
	allocm->_u.buf_xinit_request.xbops = xbops;
	allocm->_u.buf_xinit_request.xbops_context = xbops_context;
	allocm->_u.buf_xinit_request.buf_path = buf_path;

	/*
	 * First, allocate a _udi_buf_container3x_t 
	 */
	new_mem = _OSDEP_MEM_ALLOC(sizeof (_udi_buf_container3x_t),
				   UDI_MEM_NOZERO, UDI_NOWAIT);

	if (new_mem == NULL) {
		/*
		 * Queue up the memory request 
		 */
		_UDI_ALLOC_QUEUE(cb, _UDI_CT_BUF_XINIT, callback,
				 sizeof (_udi_buf_container3x_t),
				 &_udi_buf_xinit_ops);

		return;
	}

	/*
	 * We got the memory, so complete the operation, and do the callback 
	 */
	_udi_buf_xinit_complete(allocm, new_mem);
	_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_BUF_XINIT, &_udi_buf_xinit_ops,
			      callback, 1, (allocm->_u.buf_result.new_buf));
}

/*
 * Passed a memory pointer, a length, and a ptr to the set of _udi_xbops_t,
 * Add the memory to the passed in buffer.  This routine may block!
 */
void
_udi_buf_extern_add(udi_buf_t *_buf,
		    void *mem,
		    udi_size_t len,
		    _udi_xbops_t *xbops,
		    void *xbops_context)
{
	_udi_buf_container2x_t *bc;
	_udi_buf_dataseg_t *dataseg, *new_ds;
	_udi_buf_memblk_t *memblk;

	_udi_buf_t *buf = (_udi_buf_t *)_buf;

	/*
	 * First, get to the end of the buffer 
	 */
	if (buf->buf_contents != NULL) {
		dataseg = (_udi_buf_dataseg_t *)buf->buf_contents;
		while (dataseg->bd_next != NULL)
			dataseg = dataseg->bd_next;
	} else {
		dataseg = NULL;
	}

	/*
	 * Then, allocate a _udi_buf_container2x_t 
	 */
	bc = _OSDEP_MEM_ALLOC(sizeof (_udi_buf_container2x_t),
			      UDI_MEM_NOZERO, UDI_WAITOK);
	new_ds = &bc->bc_dataseg;
	memblk = &bc->bc_memblk;

	/*
	 * We waited for the memory, so fill in the new segment info
	 */
	new_ds->bd_start = mem;
	new_ds->bd_end = ((udi_ubit8_t *)mem) + len;
	new_ds->bd_memblk = memblk;
	new_ds->bd_container = bc;
	new_ds->bd_next = NULL;

	_OSDEP_ATOMIC_INT_INIT(&memblk->bm_refcnt, 1);
	memblk->bm_space = mem;
	memblk->bm_size = len;
	memblk->bm_external = xbops;

	bc->bc_xmem_context = xbops_context;

	/*
	 * Set old ending segment to point to this new one 
	 */
	if (dataseg) {
		dataseg->bd_next = new_ds;
	} else {
		buf->buf_contents = new_ds;
	}

	/*
	 * Update buffer size to include newly attached data 
	 */
	buf->buf.buf_size += len;
	buf->buf_total_size += len;
}

/*
 * Given a buffer, creates a single memory segment buffer.
 * This is called by the PIO code because it needs to operate
 * on virtually contiguous buffers. Think of it as 'pullupmsg'
 * (from STREAMS) for UDI buffers.
 */
udi_buf_t *
_udi_buf_compact(udi_buf_t *_src_buf)
{
	udi_size_t alloc_size, hdr_size, tocopy, offset;
	_udi_buf_container3x_t *bc;
	_udi_buf_dataseg_t *dataseg, *segment;
	_udi_buf_memblk_t *memblk;
	udi_size_t seglen;
	_udi_buf_t *dst_buf;
	udi_ubit8_t *mem;
	_udi_buf_t *src_buf = (_udi_buf_t *)_src_buf;

	/*
	 * Do we really need to compact anything? 
	 */
	if (_src_buf == NULL || _src_buf->buf_size == 0)
		return (_src_buf);
	if (_src_buf->buf_size == src_buf->buf_total_size &&
	    ((_udi_buf_dataseg_t *)(src_buf->buf_contents))->bd_next == 0)
		return (_src_buf);

	/*
	 * Yes, we do, so do it 
	 */
	hdr_size = sizeof (_udi_buf_container3x_t);
	alloc_size = _src_buf->buf_size;

	/*
	 * Allocate space for the new _udi_buf_container3x_t, and initialize it.
	 */
	bc = _OSDEP_MEM_ALLOC(hdr_size + alloc_size, UDI_MEM_NOZERO,
			      UDI_WAITOK);
	mem = (udi_ubit8_t *)(bc + 1);

	dst_buf = &bc->bc_header;
	dataseg = &bc->bc_dataseg;
	memblk = &bc->bc_memblk;

	dst_buf->buf.buf_size = alloc_size;
	dst_buf->buf_contents = dataseg;
	dst_buf->buf_ops = src_buf->buf_ops;
	dst_buf->buf_path = src_buf->buf_path;
	dst_buf->buf_total_size = alloc_size;
	dst_buf->buf_tags = src_buf->buf_tags;
#ifdef _UDI_PHYSIO_SUPPORTED
	dst_buf->buf_dmah = NULL;
#endif
	_OSDEP_ATOMIC_INT_INIT(&dst_buf->buf_refcnt, 1);

	dataseg->bd_start = mem;
	dataseg->bd_end = mem + alloc_size;
	dataseg->bd_memblk = memblk;
	dataseg->bd_container = bc;
	dataseg->bd_next = NULL;

	_OSDEP_ATOMIC_INT_INIT(&memblk->bm_refcnt, 1);
	memblk->bm_space = mem;
	memblk->bm_size = alloc_size;
	memblk->bm_external = NULL;

	/*
	 * Now, do the buffer copy 
	 */
	tocopy = alloc_size;
	offset = 0;
	segment = (_udi_buf_dataseg_t *)(src_buf->buf_contents);
	while (tocopy && segment != NULL) {
		seglen = (segment->bd_end - segment->bd_start);
		if (seglen != 0) {
			if (tocopy < seglen)
				seglen = tocopy;
			udi_memcpy(dataseg->bd_start + offset,
				   segment->bd_start, seglen);
			tocopy -= seglen;
			offset += seglen;
		}
		segment = segment->bd_next;
	}

	/*
	 * Fill in any extra, due to an extended buf_size 
	 */
	if (tocopy)
		udi_memset(dataseg->bd_start + offset, _UDI_BF_CHAR, tocopy);

	/*
	 * We are finished, so free the old buffer, and return the new buffer 
	 */
	udi_buf_free(_src_buf);
	return ((udi_buf_t *)dst_buf);
}

/*
 * This, like _udi_buf_compact, is for the PIO code.
 * It returns the beginning of the buffer contents so that PIO can
 * operate on it like a "normal" chunk of memory.
 */
void *
_udi_buf_getmemaddr(udi_buf_t *_src_buf)
{
	_udi_buf_t *src_buf = (_udi_buf_t *)_src_buf;

	if (src_buf->buf_total_size == 0)
		return NULL;
	_OSDEP_ASSERT(src_buf->buf_contents != 0);
	_OSDEP_ASSERT(((_udi_buf_dataseg_t
			*)(src_buf->buf_contents))->bd_start != 0);

	return (((_udi_buf_dataseg_t *)(src_buf->buf_contents))->bd_start);
}

#define _UDI_BUF_TAG_SET	1
#define _UDI_BUF_TAG_APPLY	2

void
_udi_buf_tag_discard(_udi_alloc_marshal_t *allocm)
{
	;
}

void _udi_buf_tag_complete(_udi_alloc_marshal_t *allocm,
			   void *new_mem);

static _udi_alloc_ops_t _udi_buf_tag_ops = {
	_udi_buf_tag_complete,
	_udi_buf_tag_discard,
	_udi_alloc_no_incoming,
	_UDI_ALLOC_COMPLETE_WONT_BLOCK
};

#define LEFTOVER_BYTE	1		/* Use a leftover byte from the previous calc */
#define ONE_BYTE	2		/* Do the calculation for 1 byte */
#define EXTRA_BYTES	4		/* Add extra filler bytes */

void
_udi_tag_compute(udi_tagtype_t tag_type,
		 udi_ubit8_t *mem,
		 udi_size_t len,
		 udi_ubit32_t *value,
		 udi_ubit8_t *flag,
		 udi_ubit8_t *leftover)
{
	udi_ubit16_t val;
	udi_ubit8_t *ptr = mem;
	udi_ubit8_t is_extra = ((*flag & EXTRA_BYTES) ? 1 : 0);

	switch (tag_type) {
	case UDI_BUFTAG_BE16_CHECKSUM:
	case UDI_BUFTAG_SET_iBE16_CHECKSUM:
	case UDI_BUFTAG_SET_TCP_CHECKSUM:
	case UDI_BUFTAG_SET_UDP_CHECKSUM:
		if ((*flag) & ONE_BYTE) {
			if ((*flag) & LEFTOVER_BYTE) {
				val = (udi_ubit16_t)0x100 *(udi_ubit16_t)
				 *leftover;

				if (ptr != NULL) {
					val += (udi_ubit16_t)(!is_extra ? *ptr
							      : _UDI_BF_CHAR);
				}
				*flag = (~LEFTOVER_BYTE & *flag);
			} else {
				val = (udi_ubit16_t)0x100 *(udi_ubit16_t)
				  (!is_extra ? *ptr : _UDI_BF_CHAR);
			}
			*value += val;
			if (*value > 0xffff)
				*value -= 0xffff;
			return;
		}
		while (ptr < mem + len - 1) {
			if (((*flag) & LEFTOVER_BYTE) == 0) {
				val = (udi_ubit16_t)0x100 *(udi_ubit16_t)
				  (!is_extra ? *ptr : _UDI_BF_CHAR);

				ptr++;
				val += (udi_ubit16_t)(udi_ubit16_t)(!is_extra
								    ? *ptr :
								    _UDI_BF_CHAR);
				ptr++;
			} else {
				val = (udi_ubit16_t)0x100 *(udi_ubit16_t)
				 *leftover;

				val += (udi_ubit16_t)(udi_ubit16_t)(!is_extra
								    ? *ptr :
								    _UDI_BF_CHAR);
				*flag = (~LEFTOVER_BYTE & *flag);
				ptr++;
			}
			*value += val;
			if (*value > 0xffff)
				*value -= 0xffff;
		}
		/*
		 * we still have 1 byte left over 
		 */
		if (ptr < mem + len) {
			if ((*flag) & LEFTOVER_BYTE) {
				/*
				 * Two leftovers make one complete 
				 */
				val = (udi_ubit16_t)0x100 *(udi_ubit16_t)
				 *leftover;

				val += (udi_ubit16_t)(udi_ubit16_t)(!is_extra
								    ? *ptr :
								    _UDI_BF_CHAR);
				*value += val;
				if (*value > 0xffff)
					*value -= 0xffff;
			} else {
				/*
				 * Just mark the leftover 
				 */
				*leftover =
					(udi_ubit16_t)(!is_extra ? *ptr :
						       _UDI_BF_CHAR);
				*flag |= LEFTOVER_BYTE;
			}
		}
		break;
	}
}

#define _UDI_IP_HDR_SIZE	20
#define _UDI_TCP_CKSUMOFF	_UDI_IP_HDR_SIZE+16
#define _UDI_UDP_CKSUMOFF	_UDI_IP_HDR_SIZE+6
#define MYMAX(a,b) (a > b) ? (a) : (b)

void
_udi_buf_tag_add_bufmem(udi_buf_t *_buf,
			void *new_mem,
			udi_size_t needmem)
{
	_udi_buf_dataseg_t *segment;
	_udi_buf_container2_t *bc = new_mem;
	_udi_buf_dataseg_t *dataseg = &bc->bc_dataseg;
	_udi_buf_memblk_t *memblk = &bc->bc_memblk;
	udi_ubit8_t *mem = (udi_ubit8_t *)(bc + 1);
	_udi_buf_t *buf = (_udi_buf_t *)_buf;

	/*
	 * Find the end 
	 */
	segment = (_udi_buf_dataseg_t *)(buf->buf_contents);
	if (segment == NULL) {
		buf->buf_contents = dataseg;
	} else {
		while (segment->bd_next != NULL) {
			segment = segment->bd_next;
		}
		segment->bd_next = dataseg;
	}

	dataseg->bd_start = mem;
	dataseg->bd_end = mem + needmem;
	dataseg->bd_memblk = memblk;
	dataseg->bd_container = bc;
	dataseg->bd_next = NULL;

	_OSDEP_ATOMIC_INT_INIT(&memblk->bm_refcnt, 1);
	memblk->bm_space = mem;
	memblk->bm_size = needmem;
	memblk->bm_external = NULL;

	/*
	 * Increment the buffer size - note buf_size does not need changing 
	 */
	buf->buf_total_size += needmem;
}

void
_udi_buf_tag_compute(_udi_buf_t *buf,
		     udi_size_t buf_size,
		     udi_size_t buf_off,
		     udi_size_t buf_len,
		     udi_tagtype_t tag_type,
		     udi_ubit16_t *value)
{
	udi_ubit32_t chksum;
	udi_size_t offset;
	udi_size_t copied;
	udi_size_t tocopy;
	udi_size_t copysize;
	_udi_buf_dataseg_t *segment;
	udi_size_t len, ext, shr;
	udi_ubit8_t flag, leftover;

	ext = shr = 0;
	chksum = *value;

	/*
	 * Check if we have a modified buf_size 
	 */
	if (buf_size != buf->buf_total_size) {
		if (buf_size > buf->buf_total_size) {
			/*
			 * How much did we increase it by? 
			 */
			ext = buf_size - buf->buf_total_size;
		} else {
			/*
			 * How much did we shrink it by? 
			 */
			shr = buf->buf_total_size - buf_size;
		}
	}

	/*
	 * Get to the data in segments, first 
	 */
	if (buf->buf_contents != NULL) {

		segment = (_udi_buf_dataseg_t *)(buf->buf_contents);
		offset = 0;

		len = (segment->bd_end - segment->bd_start);
		/*
		 * Go to the first segment to be read from 
		 */

		while (offset + len < buf_off && len == 0) {
			offset += len;
			segment =
				segment->bd_next ? segment->bd_next : segment;
			len = (segment->bd_end - segment->bd_start);
		}
		_OSDEP_ASSERT(offset <= buf_size);

		/*
		 * The first copy is a special case, since it may not start at
		 * the beginning of the segment.
		 */
		copied = 0;
		tocopy = buf_len;
		if (buf_off + buf_len > buf_size)
			tocopy = buf_size;
		if (offset + len - buf_off <= buf_size)
			copysize = offset + len - buf_off;
		else
			copysize = buf_size - offset;

		flag = 0;
		if (copysize > buf_len)
			copysize = buf_len;
		_udi_tag_compute(tag_type,
				 (udi_ubit8_t *)segment->bd_start + (buf_off -
								     offset),
				 copysize, &chksum, &flag, &leftover);
		copied += copysize;
		tocopy -= copysize;

		/*
		 * Copy from each segment to memory, until all data is copied
		 * or we have no more segments to read.
		 */
		while (tocopy && segment->bd_next) {
			segment = segment->bd_next;
			copysize = (segment->bd_end - segment->bd_start);
			if (copysize > tocopy)
				copysize = tocopy;
			_udi_tag_compute(tag_type,
					 (udi_ubit8_t *)segment->bd_start,
					 copysize, &chksum, &flag, &leftover);
			copied += copysize;
			tocopy -= copysize;
		}
	} else {
		/*
		 * We don't have any additional bytes to copy, but still
		 * need to account for previous leftovers.
		 */
		tocopy = 0;
	}

	/*
	 * If the buf_size was increased, and we still need more data,
	 * ext will have the amount of the increase.
	 */

	if (tocopy && ext) {
		if (tocopy < ext)
			ext = tocopy;
		/*
		 * Copy extra data, since we are returning data for an as-yet
		 * unallocated portion of the buffer.
		 */
		flag |= EXTRA_BYTES;
		_udi_tag_compute(tag_type, NULL, ext, &chksum, &flag,
				 &leftover);
	}

	if (flag & LEFTOVER_BYTE) {
		flag |= ONE_BYTE;
		_udi_tag_compute(tag_type, NULL, ext, &chksum, &flag,
				 &leftover);
	}

	/*
	 * 1's complement + truncate the checksum to a 16-bit number 
	 */
	chksum = (~chksum & 0x0000ffff);
	*value = (udi_ubit16_t)chksum;
}

/* This routine assumes that the destination buffer space has been allocated */
void
_udi_buf_tag_cksum_set(udi_ubit16_t checksum,
		       udi_buf_t *_buf,
		       udi_size_t buf_off)
{
	udi_ubit8_t chksum[2];		/* The chksum, in big-endian representation */
	_udi_buf_dataseg_t *segment;
	udi_size_t len, offset;
	_udi_buf_t *buf = (_udi_buf_t *)_buf;

	chksum[0] = (udi_ubit16_t)checksum % 256;
	chksum[1] = (udi_ubit16_t)(checksum / 256) % 256;

	/*
	 * Get to the right buffer segment offset 
	 */
	segment = (_udi_buf_dataseg_t *)(buf->buf_contents);
	offset = 0;
	len = (segment->bd_end - segment->bd_start);
	while (offset + len < buf_off) {
		offset += len;
		segment = segment->bd_next ? segment->bd_next : segment;
		len = (segment->bd_end - segment->bd_start);
	}
	*((char *)(segment->bd_start) + (buf_off - offset)) = chksum[0];
	*((char *)(segment->bd_start) + (buf_off - offset) + 1) = chksum[1];
}

void
_udi_buf_tag_complete(_udi_alloc_marshal_t *allocm,
		      void *new_mem)
{
	udi_ubit16_t ntags, i;
	_udi_buf_tag_t *buf_tag;
	udi_buf_tag_t *_buf_tag;
	udi_ubit16_t value;
	udi_ubit8_t chksum[2];
	udi_boolean_t match;
	udi_ubit8_t type = allocm->_u.buf_tag_request.req_type;
	_udi_buf_t *buf = allocm->_u.buf_tag_request.buf;
	udi_buf_t *_buf = (udi_buf_t *)buf;
	udi_buf_tag_t *_tag = allocm->_u.buf_tag_request.tag;
	udi_tagtype_t tag_type = allocm->_u.buf_tag_request.tag_type;

	_OSDEP_ASSERT(type == _UDI_BUF_TAG_SET || type == _UDI_BUF_TAG_APPLY);
	if (type == _UDI_BUF_TAG_SET) {
		ntags = allocm->_u.buf_tag_request.remain;

		/*
		 * Copy the new tag, and set the link pointers 
		 */
		udi_memcpy(new_mem, _tag, sizeof (udi_buf_tag_t));
		((_udi_buf_tag_t *) new_mem)->bt_next = buf->buf_tags;
		buf->buf_tags = new_mem;

		/*
		 * Now, loop through, doing the rest of the tags 
		 */
		_tag++;

		for (i = 1; i < ntags; i++) {
			/*
			 * buffer tag tag lengths cannot be 0 
			 */
			_OSDEP_ASSERT(_tag->tag_len != 0);
			/*
			 * Make sure we are not going over the size of the
			 * buffer 
			 */
			_OSDEP_ASSERT(_buf->buf_size >=
				      _tag->tag_off + _tag->tag_len);
			_OSDEP_ASSERT(_tag->tag_type !=
				      UDI_BUFTAG_SET_iBE16_CHECKSUM ||
				      _tag->tag_value < _buf->buf_size);

			/*
			 * If we have tags, check them for a match 
			 */
			match = FALSE;
			buf_tag = buf->buf_tags;
			while (buf_tag != NULL) {
				_buf_tag = &buf_tag->bt_v;
				if (_buf_tag->tag_type == _tag->tag_type &&
				    _buf_tag->tag_off == _tag->tag_off &&
				    _buf_tag->tag_len == _tag->tag_len) {
					_buf_tag->tag_value = _tag->tag_value;
					match = TRUE;
					break;
				}
				buf_tag = buf_tag->bt_next;
			}

			/*
			 * No existing tag match, so allocate a new one
			 */
			if (!match) {
				/*
				 * We need to alloc a new buffer tag
				 * structure. 
				 */
				new_mem = _OSDEP_MEM_ALLOC(sizeof
							   (_udi_buf_tag_t),
							   UDI_MEM_NOZERO,
							   UDI_WAITOK);
				/*
				 * Initialize the tag
				 */
				udi_memcpy(new_mem, _tag,
					   sizeof (udi_buf_tag_t));
				((_udi_buf_tag_t *) new_mem)->bt_next =
					buf->buf_tags;
				buf->buf_tags = new_mem;
			}
			_tag++;
		}
	} else {
		/*
		 * This is the harder case of a udi_buf_tag_apply completion 
		 */

		/*
		 * If no tag_type was specified, there is nothing to do 
		 */
		if (tag_type == 0) {
			SET_RESULT_UDI_CT_BUF_TAG(allocm, (udi_buf_t *)buf);
			return;
		}

		/*
		 * Add the new memory to the end of the buffer 
		 */
		_udi_buf_tag_add_bufmem((udi_buf_t *)buf, new_mem,
					allocm->_u.req_size -
					sizeof (_udi_buf_container2_t));

		/*
		 * We've got all the memory, so just do the calculations and
		 * store the value in the appropriate place.
		 */
		buf_tag = buf->buf_tags;
		while (buf_tag != NULL) {
			_buf_tag = &buf_tag->bt_v;
			if (_buf_tag->tag_type & tag_type) {
				value = 0;
				/*
				 * Found match 
				 */
				switch (_buf_tag->tag_type) {
				case UDI_BUFTAG_SET_iBE16_CHECKSUM:
					_udi_buf_tag_compute(buf,
							     _buf->buf_size,
							     _buf_tag->tag_off,
							     _buf_tag->tag_len,
							     _buf_tag->
							     tag_type, &value);
					_udi_buf_tag_cksum_set((udi_ubit16_t)
							       value, _buf,
							       _buf_tag->
							       tag_off);
					break;
				case UDI_BUFTAG_SET_TCP_CHECKSUM:
					_udi_buf_tag_compute(buf,
							     _buf->buf_size,
							     _buf_tag->tag_off,
							     _UDI_TCP_CKSUMOFF,
							     _buf_tag->
							     tag_type, &value);
					_udi_buf_tag_compute(buf,
							     _buf->buf_size,
							     _UDI_TCP_CKSUMOFF
							     + 2,
							     _buf_tag->tag_len,
							     _buf_tag->
							     tag_type, &value);
					_udi_buf_tag_cksum_set((udi_ubit16_t)
							       value, _buf,
							       _buf_tag->
							       tag_off +
							       _UDI_TCP_CKSUMOFF);
					break;
				case UDI_BUFTAG_SET_UDP_CHECKSUM:
					_udi_buf_tag_compute(buf,
							     _buf->buf_size,
							     _buf_tag->tag_off,
							     _UDI_UDP_CKSUMOFF,
							     _buf_tag->
							     tag_type, &value);
					_udi_buf_tag_compute(buf,
							     _buf->buf_size,
							     _UDI_UDP_CKSUMOFF
							     + 2,
							     _buf_tag->tag_len,
							     _buf_tag->
							     tag_type, &value);
					chksum[0] =
						(udi_ubit16_t)(value % 256);
					chksum[1] =
						(udi_ubit16_t)((udi_ubit16_t)
							       (value / 256) %
							       256);
					_udi_buf_tag_cksum_set((udi_ubit16_t)
							       value, _buf,
							       _buf_tag->
							       tag_off +
							       _UDI_UDP_CKSUMOFF);
					break;
				}
			}
			buf_tag = buf_tag->bt_next;
		}
	}

	/*
	 * Set the result before exiting 
	 */
	SET_RESULT_UDI_CT_BUF_TAG(allocm, (udi_buf_t *)buf);
}

void
udi_buf_tag_set(udi_buf_tag_set_call_t *callback,
		udi_cb_t *gcb,
		udi_buf_t *_buf,
		udi_buf_tag_t *tag_array,
		udi_ubit16_t tag_array_length)
{
	udi_ubit16_t i;
	udi_boolean_t match;
	udi_buf_tag_t *_buf_tag;
	void *new_mem;

	_udi_buf_t *buf = (_udi_buf_t *)_buf;
	_udi_buf_tag_t *buf_tag = buf->buf_tags;
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	udi_buf_tag_t *tag = tag_array;

	/*
	 * Cycle through the array 
	 */
	for (i = 0; i < tag_array_length; i++) {
		/*
		 * buffer tag tag lengths cannot be 0 
		 */
		_OSDEP_ASSERT(tag->tag_len != 0);
		/*
		 * Make sure we are not going over the size of the buffer 
		 */
		_OSDEP_ASSERT(_buf->buf_size >= tag->tag_off + tag->tag_len);
		_OSDEP_ASSERT(tag->tag_type != UDI_BUFTAG_SET_iBE16_CHECKSUM ||
			      tag->tag_value < _buf->buf_size);

		/*
		 * If we have tags, check them for a match 
		 */
		match = FALSE;
		while (buf_tag != NULL) {
			_buf_tag = &buf_tag->bt_v;
			if (_buf_tag->tag_type == tag->tag_type &&
			    _buf_tag->tag_off == tag->tag_off &&
			    _buf_tag->tag_len == tag->tag_len) {
				_buf_tag->tag_value = tag->tag_value;
				match = TRUE;
				break;
			}
			buf_tag = buf_tag->bt_next;
		}

		/*
		 * No existing tag match, so allocate a new one
		 */
		if (!match) {
			/*
			 * We need to allocate a new buffer tag structure. 
			 */
			new_mem =
				_OSDEP_MEM_ALLOC(sizeof (_udi_buf_tag_t),
						 UDI_MEM_NOZERO, UDI_NOWAIT);
			if (new_mem != NULL) {
				/*
				 * We got the memory immediately!
				 * Initialize the tag
				 */
				allocm->_u.buf_tag_request.req_type =
					_UDI_BUF_TAG_SET;
				allocm->_u.buf_tag_request.buf = buf;
				allocm->_u.buf_tag_request.tag = tag;
				allocm->_u.buf_tag_request.remain = 0;
				_udi_buf_tag_complete(allocm, new_mem);
			} else {
				break;
			}
		}
		tag++;
	}
	/*
	 * Did we get them all? 
	 */
	if (i < tag_array_length) {
		/*
		 * We did not, so queue a request 
		 */
		allocm->_u.buf_tag_request.req_type = _UDI_BUF_TAG_SET;
		allocm->_u.buf_tag_request.buf = buf;
		allocm->_u.buf_tag_request.tag = tag;
		allocm->_u.buf_tag_request.remain = tag_array_length - i;
		_UDI_ALLOC_QUEUE(cb, _UDI_CT_BUF_TAG, callback,
				 sizeof (_udi_buf_tag_t),
				 (_udi_alloc_ops_t *) & _udi_buf_tag_ops);
	} else {
		/*
		 * We did, so set the result and do the callback 
		 */
		SET_RESULT_UDI_CT_BUF_TAG(allocm, (udi_buf_t *)_buf);
		_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_BUF_TAG, &_udi_buf_tag_ops,
				      callback, 1,
				      (allocm->_u.buf_tag_result.new_buf));
	}
}

udi_ubit16_t
udi_buf_tag_get(udi_buf_t *_buf,
		udi_tagtype_t tag_type,
		udi_buf_tag_t *tag_array,
		udi_ubit16_t tag_array_length,
		udi_ubit16_t tag_start_idx)
{
	udi_buf_tag_t *tag;
	udi_buf_tag_t *ctag = tag_array + tag_start_idx;
	_udi_buf_t *buf = (_udi_buf_t *)_buf;
	_udi_buf_tag_t *buf_tag = buf->buf_tags;
	udi_ubit16_t count = 0;

	while (buf_tag != NULL) {
		tag = &buf_tag->bt_v;
		if (tag->tag_type & tag_type) {
			/*
			 * Found match 
			 */
			if ((udi_ubit32_t)count >=
			    (udi_ubit32_t)tag_start_idx &&
			    (udi_ubit32_t)(count - tag_start_idx) <
			    (udi_ubit32_t)tag_array_length) {
				/*
				 * Store it 
				 */
				udi_memcpy(ctag, tag, sizeof (udi_buf_tag_t));
				ctag++;
			}
			count++;
		}
		buf_tag = buf_tag->bt_next;
	}

	return (count);
}

udi_ubit32_t
udi_buf_tag_compute(udi_buf_t *_buf,
		    udi_size_t buf_off,
		    udi_size_t buf_len,
		    udi_tagtype_t tag_type)
{
	udi_ubit16_t value = 0;
	_udi_buf_t *buf = (_udi_buf_t *)_buf;
	udi_size_t buf_size = _buf->buf_size;

	/*
	 * Must be one thing in the UDI_BUFTAG_VALUES category 
	 * Right now, only UDI_BUFTAG_BE16_CHECKSUM 
	 */
	_OSDEP_ASSERT(tag_type & UDI_BUFTAG_VALUES);
	_OSDEP_ASSERT(tag_type == UDI_BUFTAG_BE16_CHECKSUM);

	/*
	 * For empty buffers, or buf_size of 0, just return 0 
	 */
	if (_buf == NULL || buf_size == 0)
		return (0);

	_udi_buf_tag_compute(buf, buf_size, buf_off, buf_len, tag_type,
			     &value);
	return ((udi_ubit32_t)value);
}

void
udi_buf_tag_apply(udi_buf_tag_apply_call_t *callback,
		  udi_cb_t *gcb,
		  udi_buf_t *_buf,
		  udi_tagtype_t tag_type)
{
	udi_size_t needmem;
	void *new_mem;
	udi_buf_tag_t *tag;
	_udi_buf_t *buf = (_udi_buf_t *)_buf;
	_udi_buf_tag_t *buf_tag = buf->buf_tags;
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;

	/*
	 * Must be one thing in the UDI_BUFTAG_UPDATES category 
	 */
	_OSDEP_ASSERT((tag_type & UDI_BUFTAG_UPDATES) || tag_type == 0);

	/*
	 * Search the buffer tags for a match, checking for unallocated space 
	 */
	needmem = 0;
	tag = NULL;
	while (buf_tag != NULL) {
		tag = &buf_tag->bt_v;
		if (tag->tag_type & tag_type) {
			/*
			 * Found match 
			 */
			switch (tag->tag_type) {
			case UDI_BUFTAG_SET_iBE16_CHECKSUM:
				if (buf->buf_total_size < tag->tag_value + 2)
					needmem =
						MYMAX(needmem,
						      2 + tag->tag_value -
						      buf->buf_total_size);
				break;
			case UDI_BUFTAG_SET_TCP_CHECKSUM:
				if (buf->buf_total_size <
				    tag->tag_off + _UDI_TCP_CKSUMOFF + 2)
					needmem =
						MYMAX(needmem,
						      2 + _UDI_TCP_CKSUMOFF +
						      tag->tag_off -
						      buf->buf_total_size);
				break;
			case UDI_BUFTAG_SET_UDP_CHECKSUM:
				if (buf->buf_total_size <
				    tag->tag_off + _UDI_UDP_CKSUMOFF + 2)
					needmem =
						MYMAX(needmem,
						      2 + _UDI_UDP_CKSUMOFF +
						      tag->tag_off -
						      buf->buf_total_size);
				break;
			}
		}
		buf_tag = buf_tag->bt_next;
	}

	/*
	 * Set up marshalled parameter 
	 */
	allocm->_u.buf_tag_request.buf = buf;
	allocm->_u.buf_tag_request.tag = tag;
	allocm->_u.buf_tag_request.tag_type = tag_type;
	allocm->_u.buf_tag_request.req_type = _UDI_BUF_TAG_APPLY;

	if (needmem) {
		new_mem =
			_OSDEP_MEM_ALLOC(needmem +
					 sizeof (_udi_buf_container2_t),
					 UDI_MEM_NOZERO, UDI_NOWAIT);
		if (new_mem == NULL) {
			/*
			 * We need to wait for the memory, so queue it up 
			 */
			_UDI_ALLOC_QUEUE(_UDI_CB_TO_HEADER(gcb),
					 _UDI_CT_BUF_TAG, callback,
					 needmem +
					 sizeof (_udi_buf_container2_t),
					 (_udi_alloc_ops_t *) &
					 _udi_buf_tag_ops);
			return;
		}

		/*
		 * Add the new memory to the end of the buffer 
		 */
		_udi_buf_tag_add_bufmem(_buf, new_mem, needmem);
	} else {
		/*
		 * No allocation needed, so set new_mem to NULL 
		 */
		new_mem = NULL;
	}

	/*
	 * We got the memory immediately! 
	 */
	_udi_buf_tag_complete(allocm, new_mem);
	_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_BUF_TAG, &_udi_buf_tag_ops, callback,
			      1, (allocm->_u.buf_tag_result.new_buf));
}

/*
 * Assume that buf is non-null, so the header is already preallocated,
 * and that we want to free up any of the pre-existing buffer.
 * This must by done synchonously, without waiting, so no buffer space
 * allocations can be done.
 */
void
_udi_buf_write_sync(void *copy_mem,
		    udi_size_t maplen,
		    udi_size_t len,
		    _udi_buf_t *buf,
		    udi_size_t *base,
		    udi_size_t *offset,
		    _udi_xbops_t *xbops,
		    void *context)
{
	/*
	 * This assumes the buffer header was allocated as part
	 * of a type 3x container:
	 */
	_udi_buf_container3x_t *bc = (_udi_buf_container3x_t *)buf;
	_udi_buf_dataseg_t *dataseg = &bc->bc_dataseg;
	_udi_buf_memblk_t *memblk = &bc->bc_memblk;

	_OSDEP_ASSERT(buf != NULL && buf->buf_contents != NULL);

	/*
	 * If this is an external buffer, copy the data, else attach the
	 * memory to the buffer.
	 *
	 * Do this whenever the external memory is large enough to hold
	 * the requested portion of the dma-mapped mem. 
	 */

	if (len <= buf->buf_total_size && _udi_buf_is_external(buf)) {
		_udi_mem_overwrite_buf((udi_ubit8_t *)copy_mem + *offset, len,
				       buf, 0, buf->buf_total_size);
		/*
		 * free the memory 
		 */
		(*(xbops->xbop_free)) (context, copy_mem, maplen);

		/*
		 * per the spec, a udi_dma_mem_to_buf'ed buffer as well as a
		 * udi_dma_buf_unmap'ed buffer will have the explicitly
		 * specified length as its buffer size. In the case we're 
		 * looking at here, we have external memory of a possibly
		 * larger size, so we just set buf->buf.buf_size to the new
		 * value.
		 */
		buf->buf.buf_size = len;
		return;
	}

	/*
	 * Make sure we don't free the header 
	 */
	_OSDEP_ATOMIC_INT_INCR(&((_udi_buf_dataseg_t *)buf->buf_contents)->
			       bd_memblk->bm_refcnt);

	/*
	 * Free the old buffer, if it existed 
	 */
	udi_buf_free((udi_buf_t *)buf);

	/*
	 * Check if the header already points to external memory
	 * If so, then free it.
	 */
	if (memblk->bm_external)
		((*(memblk->bm_external->xbop_free))) (bc->bc_xmem_context,
						       memblk->bm_space,
						       memblk->bm_size);

	/*
	 * Basically, just point to the new memory.
	 */
	buf->buf.buf_size = len;
	buf->buf_contents = dataseg;
	buf->buf_ops = NULL;
	buf->buf_total_size = len;
	buf->buf_tags = NULL;
#ifdef _UDI_PHYSIO_SUPPORTED
	buf->buf_dmah = NULL;
#endif
	_OSDEP_ATOMIC_INT_INCR(&buf->buf_refcnt);

	dataseg->bd_start = ((udi_ubit8_t *)copy_mem) + *offset;
	dataseg->bd_end = ((udi_ubit8_t *)copy_mem) + *offset + len;
	dataseg->bd_memblk = memblk;
	dataseg->bd_next = NULL;

	memblk->bm_space = copy_mem;
	memblk->bm_size = maplen;
	memblk->bm_external = xbops;

	/*
	 * Set the context, if any 
	 */
	if (xbops != NULL)
		bc->bc_xmem_context = context;

	/*
	 * Reset the offset and base to the new one 
	 */
	/*
	 * *offset = *base = 0; 
	 */
}

/*
 * Given a pointer to a data segment, return the size of that segment.
 * Also, set the data start point, and the pointer to the next segment.
 */
udi_size_t
_udi_buf_get_dataseg_data(_udi_buf_dataseg_t *seg,
			  udi_boolean_t inc_zero_len,
			  udi_ubit8_t *start,
			  _udi_buf_dataseg_t *next)
{
	udi_size_t segsize = seg->bd_end - seg->bd_start;

	/*
	 * For the end of the segment list, or if not including 0 len segs 
	 */
	if (seg == NULL || inc_zero_len != TRUE) {
		while (seg != NULL && segsize == 0)
			seg = seg->bd_next;
		if (seg == NULL) {
			start = NULL;
			next = NULL;
			return 0;
		}
	}

	start = seg->bd_start;
	next = seg->bd_next;
	return (segsize);
}

STATIC void
_udi_buf_path_alloc_complete(_udi_alloc_marshal_t *allocm,
			     void *new_mem)
{

	_udi_cb_t *cb = UDI_BASE_STRUCT(allocm, _udi_cb_t, cb_allocm);
	_udi_alloc_hdr_t *ah = new_mem;
	_udi_buf_path_t *new_buf_path;

	new_buf_path = (_udi_buf_path_t *) (ah + 1);
	ah->ah_type = _UDI_ALLOC_HDR_BUF_PATH;
	ah->ah_flags = 0;
	_udi_add_alloc_to_tracker(_UDI_CB_TO_CHAN(cb)->chan_region, ah);
#if _UDI_PHYSIO_SUPPORTED
	new_buf_path->constraints = NULL;
#endif /* _UDI_PHYSIO_SUPPORTED */
	UDI_QUEUE_INIT(&new_buf_path->constraints_link);
	SET_RESULT_UDI_CT_BUF_PATH_ALLOC(allocm, new_buf_path);
}

STATIC void
_udi_buf_path_alloc_discard(_udi_alloc_marshal_t *allocm)
{

	_OSDEP_MEM_FREE(allocm->_u.mem_alloc_result.new_mem);
}

STATIC _udi_alloc_ops_t _udi_buf_path_alloc_ops = {
	_udi_buf_path_alloc_complete,
	_udi_buf_path_alloc_discard,
	_udi_alloc_no_incoming,
	_UDI_ALLOC_COMPLETE_WONT_BLOCK
};


/*
 * allocate a buf_path for driver use
 * we will not track this allocation via alloc_hdr becuase
 * the environment will free these
 */
void
udi_buf_path_alloc(udi_buf_path_alloc_call_t *callback,
		   udi_cb_t *gcb)
{

	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_hdr_t *ah;
	_udi_buf_path_t *new_buf_path;
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	udi_size_t size;

	size = sizeof (_udi_buf_path_t) + sizeof (_udi_alloc_hdr_t);
	ah = _OSDEP_MEM_ALLOC(size, 0, UDI_NOWAIT);
	if (ah != NULL) {
		new_buf_path = (_udi_buf_path_t *) (ah + 1);
		_udi_buf_path_alloc_complete(allocm, ah);
		_UDI_IMMED_CALLBACK(cb, _UDI_CT_BUF_PATH_ALLOC,
				    &_udi_buf_path_alloc_ops,
				    callback, 1, (new_buf_path));
		return;
	}
	_UDI_ALLOC_QUEUE(cb, _UDI_CT_BUF_PATH_ALLOC,
			 callback, size, &_udi_buf_path_alloc_ops);
}

void
udi_buf_path_free(udi_buf_path_t buf_path_handle)
{

	_udi_buf_path_t *buf_path = buf_path_handle;
	_udi_alloc_hdr_t *ah = ((_udi_alloc_hdr_t *)buf_path_handle) - 1;

	UDI_QUEUE_REMOVE(&buf_path->constraints_link);
	_udi_rm_alloc_from_tracker(ah);
	_OSDEP_MEM_FREE(ah);
}

void
udi_buf_best_path(udi_buf_t *buf,
		  udi_buf_path_t *path_handles,
		  udi_ubit8_t npaths,
		  udi_ubit8_t last_fit,
		  udi_ubit8_t *best_fit_array)
{
	udi_ubit8_t n;

	/*
	 * All paths are considered equal.   This is generally the
	 * case for systems all sharing a local bus.  NUMA-like 
	 * environments will want to do something more ambitious.
	 */
	for (n = 0; n < npaths; n++) {
		best_fit_array[n] = n;
	}

	/*
	 * This looks like a bounds violation, but the caller really 
	 * must provide space for 'npaths + 1' members in 
	 * best_fit_array.
	 */
	best_fit_array[n++] = UDI_BUF_PATH_END;
}
