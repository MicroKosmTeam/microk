
/*
 * File: env/common/udi_mem.c
 *
 * UDI Environment -- public memory management.
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


STATIC void
_udi_mem_alloc_complete(_udi_alloc_marshal_t *allocm,
			void *new_mem)
{
	_udi_alloc_hdr_t *ah = new_mem;
	_udi_cb_t *cb = UDI_BASE_STRUCT(allocm, _udi_cb_t, cb_allocm);

	ah->ah_type = _UDI_ALLOC_HDR_MEM;
	ah->ah_flags = 0;
	if (allocm->_u.mem_alloc_request.flags & UDI_MEM_MOVABLE) {
		ah->ah_flags |= UDI_MEM_MOVABLE;
	}

	new_mem = (char *)new_mem + sizeof (_udi_mem_handle_t);
	_udi_add_alloc_to_tracker(_UDI_CB_TO_CHAN(cb)->chan_region, ah);
	_OSDEP_ASSERT(_UDI_IS_NATURALLY_ALIGNED(new_mem));
	SET_RESULT_UDI_CT_MEM_ALLOC(allocm, new_mem);
}


STATIC void
_udi_mem_alloc_discard(_udi_alloc_marshal_t *allocm)
{
	_OSDEP_MEM_FREE(allocm->_u.mem_alloc_result.new_mem);
}

STATIC _udi_alloc_ops_t _udi_mem_alloc_ops = {
	_udi_mem_alloc_complete,
	_udi_mem_alloc_discard,
	_udi_alloc_no_incoming,
	_UDI_ALLOC_COMPLETE_WONT_BLOCK
};

void
udi_mem_alloc(udi_mem_alloc_call_t *callback,
	      udi_cb_t *gcb,
	      udi_size_t size,
	      udi_ubit8_t flags)
{
	void *new_mem;
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;

	_OSDEP_ASSERT(size <=
		      _UDI_GCB_TO_CHAN(gcb)->chan_region->reg_init_rdata->
		      ird_init_context.limits.max_legal_alloc);

	size += sizeof (_udi_mem_handle_t);

	new_mem = _OSDEP_MEM_ALLOC(size, flags, UDI_NOWAIT);

	allocm->_u.mem_alloc_request.flags = flags;

	if (new_mem != NULL) {
		_udi_mem_alloc_complete(allocm, new_mem);
		new_mem = (char *)new_mem + sizeof (_udi_mem_handle_t);
		_UDI_IMMED_CALLBACK(cb, _UDI_CT_MEM_ALLOC,
				    &_udi_mem_alloc_ops,
				    callback, 1, (new_mem));
		return;
	}
	_UDI_ALLOC_QUEUE(cb, _UDI_CT_MEM_ALLOC,
			 callback, size, &_udi_mem_alloc_ops);
}

void
udi_mem_free(void *target_mem)
{
	if (target_mem != NULL) {
		target_mem = (char *)target_mem - sizeof (_udi_mem_handle_t);
		_udi_rm_alloc_from_tracker(target_mem);
		_OSDEP_MEM_FREE(target_mem);
	}
}
