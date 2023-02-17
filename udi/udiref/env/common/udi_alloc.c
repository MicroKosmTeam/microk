
/*
 * File: src/env/common/udi_alloc.c
 *
 * UDI Environment -- alloc daemon and related infrastructure.
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

_OSDEP_MUTEX_T _udi_alloc_lock;
udi_queue_t _udi_alloc_list;

void
_udi_alloc_init(void)
{
	UDI_QUEUE_INIT(&_udi_alloc_list);
	_OSDEP_MUTEX_INIT(&_udi_alloc_lock, "_udi_alloc_lock");
	_udi_timer_init();
}

void
_udi_alloc_deinit(void)
{
	_udi_timer_deinit();
	_OSDEP_MUTEX_DEINIT(&_udi_alloc_lock);
}

/*
 * _udi_alloc_no_incoming -- NOP alloc_discard_incoming function
 *
 * Use this as the DISCARD_INCOMING operation for allocation services that
 * have no incoming consumed objects (ie. before entering alloc_daemon_work).
 */

/* ARGSUSED */
void
_udi_alloc_no_incoming(_udi_alloc_marshal_t *allocm)
{
	;
}

_udi_alloc_ops_t _udi_alloc_ops_nop = {
	NULL,				/* complete */
	_udi_alloc_no_incoming,		/* discard */
	_udi_alloc_no_incoming,		/* discard_incoming */
	_UDI_ALLOC_COMPLETE_WONT_BLOCK
};

/*
 * _udi_alloc_queue is called when an allocation is not able to be
 * satisfied immediately.  The cb that is holding the alloc request
 * will be placed on a linked list of requests so a separate daemon
 * can later satisfy the request and deliver the emminent callback.
 * 
 */

void
_udi_alloc_queue(_udi_cb_t *cb)
{
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;

	cb->cb_flags |= _UDI_CB_CALLBACK;
	cb->cb_flags |= _UDI_CB_ALLOC_PENDING;

	/*
	 * Should be idle when we receive it from the driver. 
	 */
	_OSDEP_ASSERT(allocm->alloc_state == _UDI_ALLOC_IDLE);

	allocm->alloc_state = _UDI_ALLOC_PENDING;

	/*
	 * Add cb to tail of allocation list 
	 */
	_OSDEP_MUTEX_LOCK(&_udi_alloc_lock);
	UDI_ENQUEUE_TAIL(&_udi_alloc_list, &cb->cb_qlink);

	_OSDEP_MUTEX_UNLOCK(&_udi_alloc_lock);

	/*
	 * Wake up allocation daemon, if not already running 
	 */
	_OSDEP_NEW_ALLOC();
}


/*
 * A helper function to be called from the allocation daemon (which is
 * so os-dependent as to be  unrepresentable here) to do one piece of
 * work that may be queued on _udi_alloc_list.
 */
udi_boolean_t
_udi_alloc_daemon_work(void)
{
	void *new_mem;
	_udi_cb_t *cb;
	_udi_alloc_marshal_t *allocm;
	_udi_region_t *region;

	/*
	 * Pull first control block token off list; return if none yet 
	 */

	_OSDEP_MUTEX_LOCK(&_udi_alloc_lock);

	if (UDI_QUEUE_EMPTY(&_udi_alloc_list)) {
		_OSDEP_MUTEX_UNLOCK(&_udi_alloc_lock);
		return FALSE;
	}

	/*
	 * Dequeue this control block from the alloc list 
	 */
	cb = (_udi_cb_t *)UDI_DEQUEUE_HEAD(&_udi_alloc_list);
	allocm = &cb->cb_allocm;
	region = _UDI_CB_TO_CHAN(cb)->chan_region;

	/*
	 * Mark this buffer as busy, but release the lock and   
	 * retest as needed.  
	 */
	allocm->alloc_state = _UDI_ALLOC_BUSY;
	_OSDEP_MUTEX_UNLOCK(&_udi_alloc_lock);

	/*
	 * Attempt to allocate the memory.  Block if necessary. 
	 */

	/*
	 * FIXME: This allocation may not be required, (see udi_channel_spawn
	 *        for one case that doesn't ask for memory)
	 */
	if (allocm->_u.req_size > 0) {
		new_mem = _OSDEP_MEM_ALLOC(allocm->_u.req_size,
					   allocm->_u.mem_alloc_request.flags,
					   UDI_WAITOK);
	} else {
		new_mem = NULL;
	}

	REGION_LOCK(region);

	switch (allocm->alloc_state) {
	case _UDI_ALLOC_BUSY:
#ifndef _OSDEP_SIMPLE_MUTEX_BLOCKABLE
		if (allocm->alloc_ops->alloc_may_block ==
		    _UDI_ALLOC_COMPLETE_MIGHT_BLOCK) {
			REGION_UNLOCK(region);
			_UDI_ALLOC_COMPLETE(allocm, new_mem);
			REGION_LOCK(region);

			if (allocm->alloc_state == _UDI_ALLOC_CANCEL) {
				REGION_UNLOCK(region);
				_UDI_ALLOC_DISCARD(allocm);
				/*
				 * FIXME: Do we need to do other cleanup? 
				 */
			}
		} else
#endif /* !_OSDEP_SIMPLE_MUTEX_BLOCKABLE */
			_UDI_ALLOC_COMPLETE(allocm, new_mem);
		break;
	case _UDI_ALLOC_CANCEL:
		REGION_UNLOCK(region);
		_OSDEP_MEM_FREE(new_mem);
		_UDI_ALLOC_DISCARD_INCOMING(allocm);
		REGION_LOCK(region);
		break;
	default:
		_OSDEP_ASSERT(0);
	}
	allocm->alloc_state = _UDI_ALLOC_DONE;

	/*
	 * Queue the callback to the region and run the region queue
	 * if the region is not currently active. 
	 */
	_udi_queue_callback_l(cb);

	REGION_UNLOCK(region);

	return TRUE;

}

#if TRACKER

/*
 * A resource has been allocated and we need to "bill" it to the
 * region.  We implement this via a simple hash to keep the subsequent
 * search times down.
 */

void
_udi_add_alloc_to_tracker(_udi_region_t *region,
			  _udi_alloc_hdr_t *alloc_hdr)
{
	/*
	 * Throw away the bottom few bits of the alloc_hdr ptr becuase
	 * we are probably aligned to some word multiple.   This gives 
	 * us more unique bits to look at.
	 */
	unsigned long key = ((unsigned long)(alloc_hdr) >> 3) % RGN_HASH_SIZE;

	/*
	 * Simply add this onto the appropriate hash bucket. 
	 */
	UDI_ENQUEUE_TAIL(&region->reg_alloc_hash[key], &alloc_hdr->ah_link);
}

/*
 * Given a udi driver-visible object such as a udi_cb, a udi_mem_alloc
 * chunk, or a PIO handle, locate and return the alloc_hdr associated
 * with that object.   This can be used to enforce the movable memory
 * requirement.
 */

_udi_alloc_hdr_t *
_udi_find_alloc_in_tracker(_udi_region_t *region,
			   void *allocation)
{
	udi_queue_t *elem, *tmp;
	unsigned long key;

	allocation = (char *)allocation - sizeof (_udi_alloc_hdr_t);
	key = ((unsigned long)(allocation) >> 3) % RGN_HASH_SIZE;

	UDI_QUEUE_FOREACH(&region->reg_alloc_hash[key], elem, tmp) {
		_udi_alloc_hdr_t *ah = (_udi_alloc_hdr_t *)elem;

		if (elem == allocation) {
			return ah;
		}
	}
	return NULL;
}

void
_udi_rm_alloc_from_tracker(_udi_alloc_hdr_t *alloc_hdr)
{
	UDI_QUEUE_REMOVE(&alloc_hdr->ah_link);
}

void
_udi_dealloc_orphan(_udi_alloc_hdr_t *alloc_hdr,
		    const char *name)
{
	if (alloc_hdr == NULL) {
		return;
	}

	switch (alloc_hdr->ah_type) {
	case _UDI_ALLOC_HDR_PIO_MAP:{
			_udi_pio_handle_t *ph;
			ph = UDI_BASE_STRUCT(alloc_hdr, _udi_pio_handle_t,
					     alloc_hdr);

#if LATER

/* 
 * The use of env_log_write here can trigger what I consider a bug
 * in the tracing/logging code.   If you attempt a env_log_write after
 * tearing down the primary region, you're hosed.   So I'm just deferring
 * this for now.
 */
			_udi_env_log_write(_UDI_TREVENT_ENVINFO,
					   UDI_LOG_INFORMATION, 0,
					   UDI_OK, 940, ph);
#else
			_OSDEP_PRINTF
				("%s: Reclaiming lost pio handle %p with udi_pio_unmap.\n",
				 name, ph);
#endif
			udi_pio_unmap((udi_pio_handle_t)ph);
			break;
		}
	case _UDI_ALLOC_HDR_MEM:{
			_udi_mem_handle_t *mem_handle;

			mem_handle = (_udi_mem_handle_t *)
				((char *)alloc_hdr +
				 sizeof (_udi_alloc_hdr_t));
#if LATER
			_udi_env_log_write(_UDI_TREVENT_ENVINFO,
					   UDI_LOG_INFORMATION, 0,
					   UDI_OK, 941, mem_handle);
#else
			_OSDEP_PRINTF
				("%s: Reclaiming lost mem buffer %p with udi_mem_free.\n",
				 name, mem_handle);
#endif
			udi_mem_free(mem_handle);
			break;
		}
	case _UDI_ALLOC_HDR_CONSTRAINT:{
			_udi_dma_constraints_t *constraints;

			constraints = UDI_BASE_STRUCT(alloc_hdr,
						      _udi_dma_constraints_t,
						      alloc_hdr);

#if LATER
			_udi_env_log_write(_UDI_TREVENT_ENVINFO,
					   UDI_LOG_INFORMATION, 0,
					   UDI_OK, 941, constraints);
#else
			_OSDEP_PRINTF
				("%s: Reclaiming lost constraint %p with udi_dma_constraints_free.\n",
				 name, constraints);
#endif
			udi_dma_constraints_free((_udi_dma_constraints_t *)
						 constraints);
			break;
		}
	case _UDI_ALLOC_HDR_BUF:{
			_udi_buf_t *buf;

			buf = UDI_BASE_STRUCT(alloc_hdr, _udi_buf_t,
					      alloc_hdr);

			_OSDEP_PRINTF
				("%s: Reclaiming lost udi_buf %p with udi_buf_free.\n",
				 name, buf);
			udi_buf_free(&buf->buf);
			break;
		}
	case _UDI_ALLOC_HDR_DMA:{
			_udi_dma_handle_t *dmah;

			dmah = UDI_BASE_STRUCT(alloc_hdr, _udi_dma_handle_t,
					       alloc_hdr);

			_OSDEP_PRINTF
				("%s: Reclaiming lost udi_dma_handle_t %p with udi_dma_free.\n",
				 name, dmah);
			udi_dma_free((udi_dma_handle_t)dmah);
			break;
		}
	case _UDI_ALLOC_HDR_BUF_PATH:{
			_udi_buf_path_t *buf_path;

			buf_path = (_udi_buf_path_t *)
				((char *)alloc_hdr +
				 sizeof (_udi_alloc_hdr_t));
			_OSDEP_PRINTF
				("%s: Reclaiming lost udi_buf_path_t %p with udi_buf_path_free.\n",
				 name, buf_path);
			udi_buf_path_free((udi_buf_path_t)buf_path);
			break;
		}
	default:
		_OSDEP_PRINTF("Uh Oh.   This can't be good.\n");
		break;
	}
}
#else

void
_udi_add_alloc_to_tracker(_udi_region_t *region,
			  _udi_alloc_hdr_t *alloc_hdr)
{
	return;
}

void
_udi_rm_alloc_from_tracker(_udi_alloc_hdr_t *alloc_hdr)
{
	return;
}

void
_udi_dealloc_orphan(_udi_alloc_hdr_t *alloc_hdr,
		    const char *name)
{
	return;
}

#endif /* TRACKER */

void
udi_cancel(udi_cancel_call_t *callback,
	   udi_cb_t *gcb)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	_udi_region_t *region = _UDI_GCB_TO_CHAN(gcb)->chan_region;

	_OSDEP_MUTEX_LOCK(&_udi_alloc_lock);
	REGION_LOCK(region);

	_OSDEP_ASSERT(allocm->alloc_state != _UDI_ALLOC_IDLE);

	switch (allocm->alloc_state) {
	case _UDI_ALLOC_BUSY:
	case _UDI_ALLOC_CANCEL:
		/*
		 * Currently blocked in daemon 
		 */
		allocm->alloc_state = _UDI_ALLOC_CANCEL;
		REGION_UNLOCK(region);
		_OSDEP_MUTEX_UNLOCK(&_udi_alloc_lock);
		break;
	default:
		switch (allocm->alloc_state) {
		case _UDI_ALLOC_PENDING:
			/*
			 * Pending allocation (queued to alloc daemon) 
			 */
			UDI_QUEUE_REMOVE(&cb->cb_qlink);
			REGION_UNLOCK(region);
			_OSDEP_MUTEX_UNLOCK(&_udi_alloc_lock);
			_UDI_ALLOC_DISCARD_INCOMING(allocm);
			break;
		case _UDI_ALLOC_DONE:
		case _UDI_ALLOC_IMMED_DONE:
			/*
			 * Pending callback (queued to region) 
			 */
			_UDI_REGION_Q_REMOVE(region, &region->reg_queue);
			REGION_UNLOCK(region);
			_OSDEP_MUTEX_UNLOCK(&_udi_alloc_lock);
			_UDI_ALLOC_DISCARD(allocm);
			break;
		default:
			_OSDEP_ASSERT(0);
		}
		allocm->alloc_state = _UDI_ALLOC_IDLE;
		cb->cb_flags &= ~_UDI_CB_CALLBACK;

		_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_CANCEL, allocm->alloc_ops,
				      callback, 0, (0));
		break;
	}
}
