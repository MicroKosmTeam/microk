
/*
 * File: env/common/udi_dma.c
 *
 * UDI Environment -- direct memory access management
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

#ifdef _UDI_PHYSIO_SUPPORTED

void
udi_dma_limits(udi_dma_limits_t *dma_limits)
{
	/*
	 * TODO: Get real values 
	 */
	dma_limits->max_legal_contig_alloc = UDI_DMA_MIN_ALLOC_LIMIT;
	dma_limits->max_safe_contig_alloc = UDI_DMA_MIN_ALLOC_LIMIT;
	dma_limits->cache_line_size = 64;
}

STATIC void
_udi_dma_prepare_complete(_udi_alloc_marshal_t *allocm,
			  void *new_mem)
{
	udi_cb_t *gcb;
	_udi_dma_handle_t *dmah = new_mem;
	udi_ubit32_t v, *attr;
	udi_scgth_t *scgth = &dmah->dma_scgth;

	dmah->dma_constraints = allocm->_u.dma_prepare_request.constraints;
	dmah->dma_flags = allocm->_u.dma_prepare_request.flags;
	gcb = allocm->_u.dma_prepare_request.orig_cb;
	attr = dmah->dma_constraints->attributes;

	(void)_OSDEP_DMA_PREPARE(dmah, gcb);

	scgth->scgth_format = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_FORMAT);

	if (scgth->scgth_format & UDI_SCGTH_DMA_MAPPED) {
		v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_ENDIANNESS);
		_OSDEP_ASSERT(v == UDI_DMA_LITTLE_ENDIAN ||
			      v == UDI_DMA_BIG_ENDIAN);
		/*
		 * TODO: Get bridge driver's input on endianness swapping. 
		 */
#if _OSDEP_BIG_ENDIAN
		scgth->scgth_must_swap = (v == UDI_DMA_LITTLE_ENDIAN);
#else
		scgth->scgth_must_swap = (v == UDI_DMA_BIG_ENDIAN);
#endif
	} else {
		scgth->scgth_must_swap = FALSE;
	}

	SET_RESULT_UDI_CT_DMA_PREPARE(allocm, (udi_dma_handle_t)dmah);
}

STATIC void
_udi_dma_prepare_discard(_udi_alloc_marshal_t *allocm)
{
	_OSDEP_DMA_RELEASE(allocm->_u.dma_prepare_result.new_dma_handle);
	_OSDEP_MEM_FREE(allocm->_u.dma_prepare_result.new_dma_handle);
}

STATIC _udi_alloc_ops_t _udi_dma_prepare_ops = {
	_udi_dma_prepare_complete,
	_udi_dma_prepare_discard,
	_udi_alloc_no_incoming,
#if _OSDEP_DMA_PREPARE_MIGHT_BLOCK
	_UDI_ALLOC_COMPLETE_MIGHT_BLOCK
#else
	_UDI_ALLOC_COMPLETE_WONT_BLOCK
#endif
};

void
udi_dma_prepare(udi_dma_prepare_call_t *callback,
		udi_cb_t *gcb,
		udi_dma_constraints_t _constraints,
		udi_ubit8_t flags)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	_udi_dma_constraints_t *constraints =
		_UDI_HANDLE_TO_DMA_CONSTRAINTS(_constraints);
#if !_OSDEP_DMA_PREPARE_MIGHT_BLOCK
	void *new_mem;
#endif

	/*
	 * Make sure the driver called udi_bus_bind_req before preparing a dma
	 * handle.
	 * If the driver has been bound to the bridge driver, the bind channel
	 * in its dev_node will be set.
	 */
	_OSDEP_ASSERT((_UDI_GCB_TO_CHAN(gcb)->chan_region->reg_node->
		       bind_channel != NULL) &&
		      "You must call udi_bus_bind_req before udi_dma_prepare!");

	constraints =
		/*
		 * Store the remaining args where the complete routine can get them. 
		 */
		allocm->_u.dma_prepare_request.constraints = constraints;
	allocm->_u.dma_prepare_request.flags = flags;
	allocm->_u.dma_prepare_request.orig_cb = gcb;

	/*
	 * Allocate a DMA handle.
	 *
	 * If we'll be able to complete the request without blocking,
	 * try to get the memory without queueing.
	 */
#if !_OSDEP_DMA_PREPARE_MIGHT_BLOCK
	new_mem = _OSDEP_MEM_ALLOC(sizeof (_udi_dma_handle_t), 0, UDI_NOWAIT);

	if (new_mem != NULL) {
		_udi_dma_prepare_complete(allocm, new_mem);
		_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_DMA_PREPARE,
				      &_udi_dma_prepare_ops,
				      callback, 1,
				      ((udi_dma_handle_t)new_mem));
		return;
	}
#endif

	_UDI_ALLOC_QUEUE(cb, _UDI_CT_DMA_PREPARE,
			 callback, sizeof (_udi_dma_handle_t),
			 &_udi_dma_prepare_ops);
}


STATIC void
_udi_dma_buf_map_complete(_udi_alloc_marshal_t *allocm,
			  void *new_mem)
{
	_udi_dma_handle_t *dmah;
	udi_status_t status;

	dmah = allocm->_u.dma_buf_map_request.dmah;

	/*
	 * The OS-dependent code will likely have to go through a
	 * series of potentially-blocking steps, some of which may
	 * have been completed without needing to block, in the main
	 * udi_dma_buf_map(). Loop through the rest of them here, now
	 * that we can block. The OSDEP code keeps track of where it
	 * left off, using allocm->_u.dma_buf_map_request.state.
	 *
	 * Each call to _OSDEP_DMA_BUF_MAP advances to the next state.
	 * With the UDI_WAITOK flag, _OSDEP_DMA_BUF_MAP is required to
	 * make progress (or fail), so we don't need to check the return
	 * value. When all the steps have been completed (successfully or
	 * not), _OSDEP_DMA_BUF_MAP sets the state to 255. In the latter
	 * case, _OSDEP_DMA_BUF_MAP must set the complete flag and status
	 * code in allocm->_u.dma_buf_map_result.
	 *
	 * _OSDEP_DMA_BUF_MAP is responsible for:
	 *      1) Ensuring that the buffer memory meets DMA constraints.
	 *              - segment or allocate/copy as needed
	 *      2) Mapping the resulting data memory into DMA space.
	 *      3) Building a scatter/gather list.
	 *      4) Mapping the scatter/gather list as required by
	 *              UDI_DMA_SCGTH_FORMAT.
	 *      5) Filling in dmah->dma_scgth.
	 */
	do {
		(void)_OSDEP_DMA_BUF_MAP(dmah, allocm, UDI_WAITOK);
	} while (allocm->_u.dma_buf_map_request.state != 255);

	allocm->_u.dma_buf_map_result.scgth = &dmah->dma_scgth;
	status = allocm->_u.dma_buf_map_result.status;

	if (status == UDI_OK) {
		_OSDEP_DMA_SYNC(dmah, dmah->dma_buf_base,
				dmah->dma_maplen, dmah->dma_buf_flags);
	}
}

STATIC void
_udi_dma_buf_map_discard(_udi_alloc_marshal_t *allocm)
{
	_OSDEP_DMA_BUF_UNMAP(allocm->_u.dma_buf_map_result.dmah);
	_OSDEP_DMA_SCGTH_FREE(allocm->_u.dma_buf_map_result.dmah);
}

STATIC void
_udi_dma_buf_map_discard_incoming(_udi_alloc_marshal_t *allocm)
{
	_OSDEP_DMA_BUF_MAP_DISCARD(allocm);
}

STATIC _udi_alloc_ops_t _udi_dma_buf_map_ops = {
	_udi_dma_buf_map_complete,
	_udi_dma_buf_map_discard,
	_udi_dma_buf_map_discard_incoming,
	_UDI_ALLOC_COMPLETE_MIGHT_BLOCK
};

void
udi_dma_buf_map(udi_dma_buf_map_call_t *callback,
		udi_cb_t *gcb,
		udi_dma_handle_t dma_handle,
		udi_buf_t *buf,
		udi_size_t offset,
		udi_size_t len,
		udi_ubit8_t flags)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	_udi_dma_handle_t *dmah = dma_handle;
	_udi_dma_constraints_t *constraint = dmah->dma_constraints;
	_udi_buf_t *_buf = (_udi_buf_t *)buf;
	_udi_buf_path_t *buf_path = _UDI_HANDLE_TO_BUF_PATH(_buf->buf_path);
	udi_boolean_t please_use_a_different_buf_path = FALSE;

	/*
	 * Make sure requested direction(s) are a subset of those set
	 * when the DMA handle was allocated.
	 */
	_OSDEP_ASSERT((flags & ~dmah->dma_flags) == 0);

	_OSDEP_ASSERT((flags & (UDI_DMA_IN | UDI_DMA_OUT)) != 0);
	_OSDEP_ASSERT(dmah->dma_mem == NULL);

	/*
	 * Associate the buf_path of the buffer with this dma constratint
	 * and reverse reference it by putting the buf_path in a queue
	 */
	if (buf_path->constraints == NULL) {
		buf_path->constraints = dmah->dma_constraints;
		UDI_ENQUEUE_TAIL(&constraint->buf_path_q,
				 &buf_path->constraints_link);
	} else if (buf_path->constraints != dmah->dma_constraints) {
		_OSDEP_ASSERT(please_use_a_different_buf_path);
	}
	/*
	 * Rewind to beginning of buffer if this is a new buffer or
	 * the UDI_DMA_REWIND flag is set.
	 */
	if ((flags & UDI_DMA_REWIND) || (_udi_buf_t *)buf != dmah->dma_buf)
		dmah->dma_buf_offset = offset;

	/*
	 * Keep track of mapping properties in the DMA handle.
	 */
	dmah->dma_buf_base = offset;
	dmah->dma_maplen = len;
	dmah->dma_buf_flags = flags & ~UDI_DMA_REWIND;
	dmah->dma_buf = (_udi_buf_t *)buf;

	/*
	 * Store the remaining args where the complete routine can get them. 
	 */
	allocm->_u.dma_buf_map_request.dmah = dmah;

	/*
	 * The spec probably should have required offset+len to not exceed
	 * buf_size, to be consistent with udi_buf_read et al. It didn't,
	 * so we have to extend buf_size here.
	 */
	if (offset + len > buf->buf_size)
		buf->buf_size = offset + len;

	/*
	 * If we're going to be modifying buffer data, we need to make sure
	 * that the memory is all instantiated and non-shared, so that
	 * udi_dma_buf_unmap can operate synchronously.
	 */
#ifdef NOTYET
	_udi_buf_write_prep(callback ?, gcb, buf, offset, len);
#endif

	/*
	 * The OS-dependent code will likely have to go through a
	 * series of potentially-blocking steps. Loop through them
	 * here, with non-blocking optimistic allocations. Keep track
	 * of how far we get using a udi_ubit8_t state variable in
	 * allocm->_u.dma_buf_map_request.state.
	 *
	 * _OSDEP_DMA_BUF_MAP returns TRUE if it was able to make progress,
	 * and advances the state. When all the steps have been completed
	 * (successfully or not), _OSDEP_DMA_BUF_MAP sets the state to 255.
	 * In the latter case, _OSDEP_DMA_BUF_MAP must set the complete flag
	 * and status code in allocm->_u.dma_buf_map_result.
	 */
	allocm->_u.dma_buf_map_request.state = 0;
	while (_OSDEP_DMA_BUF_MAP(dmah, allocm, UDI_NOWAIT)) {
		if (allocm->_u.dma_buf_map_request.state == 255) {
			/*
			 * Completed the whole request without blocking. 
			 */
			_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_DMA_BUF_MAP,
					      &_udi_dma_buf_map_ops,
					      callback, 3,
					      (&dmah->dma_scgth,
					       allocm->_u.dma_buf_map_result.
					       complete,
					       allocm->_u.dma_buf_map_result.
					       status));
			return;
		}
	}

	/*
	 * _OSDEP_DMA_BUF_MAP must now block in order to make further progress,
	 * so continue in the complete routine on an alloc daemon thread.
	 */
	_UDI_ALLOC_QUEUE(cb, _UDI_CT_DMA_BUF_MAP,
			 callback, 0, &_udi_dma_buf_map_ops);
}

udi_buf_t *
udi_dma_buf_unmap(udi_dma_handle_t dma_handle,
		  udi_size_t new_buf_size)
{
	_udi_dma_handle_t *dmah = dma_handle;
	_udi_dma_constraints_t *constraint;
	_udi_buf_path_t *buf_path;
	_udi_buf_t *dma_buf;
	udi_queue_t *tmp, *elem;

	if (UDI_HANDLE_IS_NULL(dma_handle, udi_dma_handle_t)) {
		return NULL;
	}

	_OSDEP_ASSERT(dmah->dma_buf != NULL);

	constraint = dmah->dma_constraints;

	/*
	 * set buf size to the new value
	 */
	dmah->dma_buf->buf.buf_size = new_buf_size;

	/*
	 * unassociate this constraint from all bufs 
	 */
	UDI_QUEUE_FOREACH(&constraint->buf_path_q, elem, tmp) {
		buf_path = UDI_BASE_STRUCT(elem,
					   _udi_buf_path_t, constraints_link);
		buf_path->constraints = UDI_NULL_DMA_CONSTRAINTS;
		UDI_QUEUE_REMOVE(elem);
		UDI_QUEUE_INIT(elem);
	}
	if (dmah->dma_buf_flags & UDI_DMA_IN) {
		_OSDEP_DMA_SYNC(dmah, dmah->dma_buf_base,
				dmah->dma_maplen, UDI_DMA_IN);
	}

	_OSDEP_DMA_BUF_UNMAP(dmah);
	_OSDEP_DMA_SCGTH_FREE(dmah);

	dma_buf = dmah->dma_buf;
	dmah->dma_buf = NULL;

	return (udi_buf_t *)dma_buf;
}

STATIC void
_udi_dma_mem_alloc_complete(_udi_alloc_marshal_t *allocm,
			    void *new_mem)
{
	_udi_dma_handle_t *dmah = new_mem;
	udi_scgth_t *scgth = &dmah->dma_scgth;
	udi_ubit8_t flags;
	udi_size_t size;
	udi_status_t status;
	udi_boolean_t must_swap;
	udi_ubit32_t v, *attr;
	udi_cb_t *gcb;
	_udi_cb_t *cb = UDI_BASE_STRUCT(allocm, _udi_cb_t, cb_allocm);


	dmah->alloc_hdr.ah_type = _UDI_ALLOC_HDR_DMA;
	_udi_add_alloc_to_tracker(_UDI_CB_TO_CHAN(cb)->chan_region,
				  &dmah->alloc_hdr);

	dmah->dma_constraints = allocm->_u.dma_mem_alloc_request.constraints;
	dmah->dmah_region = _UDI_CB_TO_CHAN(cb)->chan_region;
	flags = allocm->_u.dma_mem_alloc_request.flags;
	dmah->dma_flags = flags & (UDI_DMA_IN | UDI_DMA_OUT);
	gcb = allocm->_u.dma_mem_alloc_request.gcb;
	attr = dmah->dma_constraints->attributes;

	/*
	 * NOTE: architectures that impose DMA/cache interactions will
	 * need to do something more ambitious here.
	 */
	size = allocm->_u.dma_mem_alloc_request.nelements *
		allocm->_u.dma_mem_alloc_request.element_size;;

	status = _OSDEP_DMA_PREPARE(dmah, gcb);
	if (status != UDI_OK) {
		/*
		 * TODO: something 
		 */
		_OSDEP_ASSERT(0);
	}

	scgth->scgth_format = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_FORMAT);
	if (scgth->scgth_format & UDI_SCGTH_DMA_MAPPED) {
		v = _UDI_CONSTRAINT_VAL(attr, UDI_DMA_SCGTH_ENDIANNESS);
		_OSDEP_ASSERT(v == UDI_DMA_LITTLE_ENDIAN ||
			      v == UDI_DMA_BIG_ENDIAN);
		/*
		 * TODO: Get bridge driver's input on endianness swapping. 
		 */
#if _OSDEP_BIG_ENDIAN
		scgth->scgth_must_swap = (v == UDI_DMA_LITTLE_ENDIAN);
#else
		scgth->scgth_must_swap = (v == UDI_DMA_BIG_ENDIAN);
#endif
	} else {
		scgth->scgth_must_swap = FALSE;
	}

	dmah->dma_mem = _OSDEP_DMA_MEM_ALLOC(dmah, size, flags);
	dmah->dma_maplen = size;

	/*
	 * TODO: Get bridge driver's input on endianness swapping. 
	 */
#if _OSDEP_BIG_ENDIAN
	must_swap = !!(flags & UDI_DMA_LITTLE_ENDIAN);
#else
	must_swap = !!(flags & UDI_DMA_BIG_ENDIAN);
#endif

	SET_RESULT_UDI_CT_DMA_MEM_ALLOC(allocm,
					(udi_dma_handle_t)dmah,
					dmah->dma_mem,
					0, TRUE, &dmah->dma_scgth, must_swap);
}

STATIC void
_udi_dma_mem_alloc_discard(_udi_alloc_marshal_t *allocm)
{
	_udi_dma_handle_t *dmah = (_udi_dma_handle_t *)
		allocm->_u.dma_mem_alloc_result.new_dma_handle;

	_OSDEP_DMA_SCGTH_FREE(dmah);
	_OSDEP_DMA_MEM_FREE(dmah);
	_OSDEP_DMA_RELEASE(dmah);
	_OSDEP_MEM_FREE(dmah);
}

STATIC _udi_alloc_ops_t _udi_dma_mem_alloc_ops = {
	_udi_dma_mem_alloc_complete,
	_udi_dma_mem_alloc_discard,
	_udi_alloc_no_incoming,
	_UDI_ALLOC_COMPLETE_MIGHT_BLOCK
};

void
udi_dma_mem_alloc(udi_dma_mem_alloc_call_t *callback,
		  udi_cb_t *gcb,
		  udi_dma_constraints_t constraints,
		  udi_ubit8_t flags,
		  udi_ubit16_t nelements,
		  udi_size_t element_size,
		  udi_size_t max_gap)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	_udi_region_t *rgn = _UDI_CB_TO_CHAN(cb)->chan_region;

	/*
	 * NOTE:   This implementation assumes the host architecture
	 * can simply allocate memory contiguously, disregarding the
	 * nelements and max_gap stuff that was introduced in CD1 
	 * for PHYSIO A20000210.1.  For architectures with restrictive
	 * dma and cache interactions, this will have to be revisited.
	 */
	udi_ubit32_t size = nelements * element_size;

	/*
	 * Make sure the driver called udi_bus_bind_req before allocating 
	 * dma memory.
	 * If the driver has been bound to the bridge driver, the bind channel
	 * in its dev_node will be set.
	 */
	_OSDEP_ASSERT((_UDI_GCB_TO_CHAN(gcb)->chan_region->reg_node->
		       bind_channel != NULL) &&
	      "You must call udi_bus_bridge_req before udi_dma_mem_alloc!");
	
	if (!UDI_QUEUE_EMPTY(&rgn->dma_mem_handle_q)) {
		udi_queue_t *Qd_dmah, *tmpQ;
		_udi_dma_handle_t *dmah = NULL;
		udi_boolean_t must_swap;

		UDI_QUEUE_FOREACH(&rgn->dma_mem_handle_q, Qd_dmah, tmpQ) {
			dmah = UDI_BASE_STRUCT(Qd_dmah, _udi_dma_handle_t,
					       dma_rgn_q);

			if ((dmah->dma_constraints ==
			     (_udi_dma_constraints_t *)constraints) &&
			    dmah->dma_maplen >= size) {
				UDI_QUEUE_REMOVE(Qd_dmah);
				tmpQ = NULL;
				break;
			}
		}
		if (tmpQ == NULL) {
#if _OSDEP_BIG_ENDIAN
			must_swap = !!(flags & UDI_DMA_LITTLE_ENDIAN);
#else
			must_swap = !!(flags & UDI_DMA_BIG_ENDIAN);
#endif
			_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_DMA_MEM_ALLOC, &_udi_dma_mem_alloc_ops, callback, 6, (dmah, dmah->dma_mem, 0,	/*actual gap */
														TRUE,	/* single element */
														&dmah->
														dma_scgth,
														must_swap));
			return;
		}
	}


	/*
	 * Since we need to do several complicated allocations, just go right
	 * to the daemon rather than trying non-blocking allocations. Start
	 * off by allocating a new DMA handle.
	 */
	allocm->_u.dma_mem_alloc_request.constraints =
		(_udi_dma_constraints_t *)constraints;
	allocm->_u.dma_mem_alloc_request.flags = flags;
	allocm->_u.dma_mem_alloc_request.nelements = nelements;
	allocm->_u.dma_mem_alloc_request.element_size = element_size;
	allocm->_u.dma_mem_alloc_request.gcb = gcb;
	_UDI_ALLOC_QUEUE(cb, _UDI_CT_DMA_MEM_ALLOC,
			 callback, sizeof (_udi_dma_handle_t),
			 &_udi_dma_mem_alloc_ops);
}

void
udi_dma_sync(udi_dma_sync_call_t *callback,
	     udi_cb_t *gcb,
	     udi_dma_handle_t dma_handle,
	     udi_size_t offset,
	     udi_size_t length,
	     udi_ubit8_t flags)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);

	_OSDEP_DMA_SYNC((_udi_dma_handle_t *)dma_handle,
			offset, length, flags);

	_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_DMA_SYNC,
			      &_udi_alloc_ops_nop, callback, 0, (0));
}

void
udi_dma_scgth_sync(udi_dma_scgth_sync_call_t *callback,
		   udi_cb_t *gcb,
		   udi_dma_handle_t dma_handle)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);

	_OSDEP_DMA_SCGTH_SYNC((_udi_dma_handle_t *)dma_handle);

	_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_DMA_SCGTH_SYNC,
			      &_udi_alloc_ops_nop, callback, 0, (0));
}

void
udi_dma_mem_barrier(udi_dma_handle_t dma_handle)
{
	_OSDEP_DMA_MEM_BARRIER((_udi_dma_handle_t *)dma_handle);
}

void
udi_dma_free(udi_dma_handle_t dma_handle)
{
	_udi_dma_handle_t *dmah = dma_handle;

	if (UDI_HANDLE_IS_NULL(dma_handle, udi_dma_handle_t)) {
		return;
	}

	_OSDEP_ASSERT(dmah->dma_buf == NULL);

	if (dmah->dma_mem) {
		_udi_rm_alloc_from_tracker(&dmah->alloc_hdr);
		_OSDEP_DMA_SCGTH_FREE(dmah);
		_OSDEP_DMA_MEM_FREE(dmah);
	}

	_OSDEP_DMA_RELEASE(dmah);
	_OSDEP_MEM_FREE(dmah);
}

STATIC void
_udi_dma_mem_to_buf_discard(_udi_alloc_marshal_t *allocm)
{
	udi_buf_free(allocm->_u.dma_mem_to_buf_result.new_buf);
}

STATIC void _udi_dma_mem_to_buf_complete(_udi_alloc_marshal_t *allocm,
					 void *new_mem);

STATIC _udi_alloc_ops_t _udi_dma_mem_to_buf_ops = {
	_udi_dma_mem_to_buf_complete,
	_udi_dma_mem_to_buf_discard,
	_udi_alloc_no_incoming,
	_UDI_ALLOC_COMPLETE_WONT_BLOCK
};

STATIC void
_udi_dma_mem_to_buf_complete(_udi_alloc_marshal_t *allocm,
			     void *new_mem)
{
	_udi_buf_container3x_t *bc;
	_udi_buf_t *buf;
	_udi_buf_dataseg_t *dataseg;
	_udi_buf_memblk_t *memblk;
	_udi_dma_handle_t *dmah = allocm->_u.dma_mem_to_buf_request.dmah;
	udi_size_t base = dmah->dma_buf_base;
	udi_size_t offset = allocm->_u.dma_mem_to_buf_request.src_off;

	bc = (_udi_buf_container3x_t *)new_mem;
	buf = &bc->bc_header;

	if (allocm->_u.dma_mem_to_buf_request.type == 1) {
		dataseg = &bc->bc_dataseg;
		memblk = &bc->bc_memblk;

		buf->buf.buf_size = 0;
		buf->buf_contents = dataseg;
		buf->buf_ops = NULL;
		/*
		 * buf->buf_dma_constraints = NULL; mlau
		 */
		buf->buf_total_size = 0;
		buf->buf_tags = NULL;
		buf->buf_dmah = NULL;
		_OSDEP_ATOMIC_INT_INIT(&buf->buf_refcnt, 1);

		dataseg->bd_start = 0;
		dataseg->bd_end = 0;
		dataseg->bd_memblk = memblk;
		dataseg->bd_container = bc;
		dataseg->bd_next = NULL;

		_OSDEP_ATOMIC_INT_INIT(&memblk->bm_refcnt, 0);
		memblk->bm_space = NULL;
		memblk->bm_size = 0;
		memblk->bm_external = NULL;
	}

	/*
	 * OK, now we have a destination buffer, so do the mem_to_buf part 
	 */
	_udi_buf_write_sync(dmah->dma_mem, dmah->dma_maplen,
			    allocm->_u.dma_mem_to_buf_request.src_len,
			    buf, &base, &offset, &_udi_dma_m2buf_xbops, dmah);

	/*
	 * Set the result 
	 */
	SET_RESULT_UDI_CT_DMA_MEM_TO_BUF(allocm, (udi_buf_t *)buf);
}

void
udi_dma_mem_to_buf(udi_dma_mem_to_buf_call_t *callback,
		   udi_cb_t *gcb,
		   udi_dma_handle_t dma_handle,
		   udi_size_t src_off,
		   udi_size_t src_len,
		   udi_buf_t *dst_buf)
{
	_udi_buf_dataseg_t *dataseg;
	_udi_buf_memblk_t *memblk;

	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	_udi_buf_t *buf = (_udi_buf_t *)dst_buf;

	allocm->_u.dma_mem_to_buf_request.dmah =
		(struct _udi_dma_handle *)dma_handle;
	allocm->_u.dma_mem_to_buf_request.src_off = src_off;
	allocm->_u.dma_mem_to_buf_request.src_len = src_len;
	allocm->_u.dma_mem_to_buf_request.type = 0;

	if (buf == NULL) {
		_udi_buf_container3x_t *bc =
			_OSDEP_MEM_ALLOC(sizeof (_udi_buf_container3x_t),
					 UDI_MEM_NOZERO, UDI_NOWAIT);

		if (bc == NULL) {
			/*
			 * Queue up the memory request 
			 */
			allocm->_u.dma_mem_to_buf_request.type = 1;
			_UDI_ALLOC_QUEUE(cb, _UDI_CT_DMA_MEM_TO_BUF, callback,
					 sizeof (_udi_buf_container3x_t),
					 &_udi_dma_mem_to_buf_ops);
			return;
		}
		/*
		 * We got the memory for the buffer, so populate it 
		 */
		buf = &bc->bc_header;
		dataseg = &bc->bc_dataseg;
		memblk = &bc->bc_memblk;

		buf->buf.buf_size = 0;
		buf->buf_contents = dataseg;
		buf->buf_ops = NULL;
		/*
		 * buf->buf_dma_constraints = NULL; mlau
		 */
		buf->buf_total_size = 0;
		buf->buf_tags = NULL;
		buf->buf_dmah = NULL;
		_OSDEP_ATOMIC_INT_INIT(&buf->buf_refcnt, 1);

		dataseg->bd_start = 0;
		dataseg->bd_end = 0;
		dataseg->bd_memblk = memblk;
		dataseg->bd_container = bc;
		dataseg->bd_next = NULL;

		_OSDEP_ATOMIC_INT_INIT(&memblk->bm_refcnt, 0);
		memblk->bm_space = NULL;
		memblk->bm_size = 0;
		memblk->bm_external = NULL;
	}

	/*
	 * We have a buffer, so complete the operation and do the callback 
	 */
	_udi_dma_mem_to_buf_complete(allocm, (void *)buf);
	_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_DMA_MEM_TO_BUF,
			      &_udi_dma_mem_to_buf_ops, callback, 1,
			      (allocm->_u.dma_mem_to_buf_result.new_buf));
}

void
_udi_dma_m2buf_free(void *context,
		    udi_ubit8_t *space,
		    udi_size_t size)
{
	_udi_dma_handle_t *dmah = context;

	/*
	 * This buffer segment was created by attaching memory from a
	 * udi_dma_mem_to_buf operation, so we move that (preserved)
	 * DMA handle and memory back into the region's pool for 
	 * subsequent allocations. 
	 */

/*XXX: probably want a high-watermark for the dma_mem_handle_q pool. */
	UDI_ENQUEUE_TAIL(&dmah->dmah_region->dma_mem_handle_q,
			 &dmah->dma_rgn_q);
}

_udi_xbops_t _udi_dma_m2buf_xbops = { &_udi_dma_m2buf_free, NULL, NULL };

#else  /* !_UDI_PHYSIO_SUPPORTED */

char ___unused;				/* Keep the compiler happy. */

#endif /* _UDI_PHYSIO_SUPPORTED */
