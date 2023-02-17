
/*
 * File: env/common/udi_constraints.c
 *
 * UDI Environment -- DMA Constraints Functions
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

/*
 * Constraints combination types.
 */
#define UDI_ATTR_MOST_RESTRICTIVE	0
#define UDI_ATTR_LEAST_RESTRICTIVE	1

/* The restrictions are invalid if _UDI_CONSTR_RESTR_UNK is set */
#define _UDI_CONSTR_RESTR_UNK		0

/* Restrictions are not based on value if _UDI_CONSTR_RESTR_NONE is set */
#define _UDI_CONSTR_RESTR_NONE		1

/* Greater values are more restrictive if _UDI_CONSTR_RESTR_ASCEND is set */
#define _UDI_CONSTR_RESTR_ASCEND	2

/*
 * Greater values are more restrictive, except that zero is most restrictive,
 * if _UDI_CONSTR_RESTR_ASCEND is set
 */
#define _UDI_CONSTR_RESTR_ASCEND_ZMOST	3

/*
 * Greater values are less restrictive, but zero--if legal--is least
 * restrictive of all, if _UDI_CONSTR_RESTR_DESCEND is set
 */
#define _UDI_CONSTR_RESTR_DESCEND	4

/* Setting new values adds to the restr if _UDI_CONSTR_RESTR_ADD is set */
#define _UDI_CONSTR_RESTR_ADD		5

/*
 * Here's where all the min/max, least+most restrictive and default
 * values are kept
 */
_udi_constraints_range_types _udi_constraints_range_attributes
	[_UDI_CONSTRAINTS_INDEX_MAX] = {
	/*
	 * Fields are:
	 *   attr_id
	 *   min_value
	 *   max_value
	 *   least_restr
	 *   most_restr
	 *   default_value
	 *   restr_type
	 */
	{UDI_DMA_DATA_ADDRESSABLE_BITS, 16, 255, 255, 16, 255,
	 _UDI_CONSTR_RESTR_DESCEND}
	,
	{UDI_DMA_NO_PARTIAL, 0, 1, 0, 1, 0,
	 _UDI_CONSTR_RESTR_ASCEND}
	,
	{UDI_DMA_SCGTH_MAX_ELEMENTS, 0, 0xffff, 0, 1, 0,
	 _UDI_CONSTR_RESTR_DESCEND}
	,
	{UDI_DMA_SCGTH_FORMAT, 0, UDI_SCGTH_32 | UDI_SCGTH_64 |
	 UDI_SCGTH_DMA_MAPPED | UDI_SCGTH_DRIVER_MAPPED,
	 0, 0, UDI_SCGTH_32 | UDI_SCGTH_DMA_MAPPED, _UDI_CONSTR_RESTR_NONE}
	,
	{UDI_DMA_SCGTH_ENDIANNESS, UDI_DMA_ANY_ENDIAN, UDI_DMA_LITTLE_ENDIAN,
	 0, 0, UDI_DMA_ANY_ENDIAN, _UDI_CONSTR_RESTR_NONE}
	,
	{UDI_DMA_SCGTH_ADDRESSABLE_BITS, 16, 255, 255, 16, 255,
	 _UDI_CONSTR_RESTR_DESCEND}
	,
	{UDI_DMA_SCGTH_MAX_SEGMENTS, 0, 255, 0, 1, 0,
	 _UDI_CONSTR_RESTR_DESCEND}
	,
	{UDI_DMA_SCGTH_ALIGNMENT_BITS, 0, 255, 0, 255, 0,
	 _UDI_CONSTR_RESTR_ASCEND}
	,
	{UDI_DMA_SCGTH_MAX_EL_PER_SEG, 0, 0xffff, 0, 1, 0,
	 _UDI_CONSTR_RESTR_DESCEND}
	,
	{UDI_DMA_SCGTH_PREFIX_BYTES, 0, 0xffff, 0, 0xffff, 0,
	 _UDI_CONSTR_RESTR_ASCEND}
	,
	{UDI_DMA_ELEMENT_ALIGNMENT_BITS, 0, 255, 0, 255, 0,
	 _UDI_CONSTR_RESTR_ASCEND}
	,
	{UDI_DMA_ELEMENT_LENGTH_BITS, 0, 32, 0, 1, 0,
	 _UDI_CONSTR_RESTR_DESCEND}
	,
	{UDI_DMA_ELEMENT_GRANULARITY_BITS, 0, 32, 0, 1, 0,
	 _UDI_CONSTR_RESTR_DESCEND}
	,
	{UDI_DMA_ADDR_FIXED_BITS, 0, 255, 0, 1, 0,
	 _UDI_CONSTR_RESTR_DESCEND}
	,
	{UDI_DMA_ADDR_FIXED_TYPE, 1, 3, 1, 3, 1,
	 _UDI_CONSTR_RESTR_ASCEND}
	,
	{UDI_DMA_ADDR_FIXED_VALUE_LO, 0, 0xffffffff, 0, 0, 0,
	 _UDI_CONSTR_RESTR_NONE}
	,
	{UDI_DMA_ADDR_FIXED_VALUE_HI, 0, 0xffffffff, 0, 0, 0,
	 _UDI_CONSTR_RESTR_NONE}
	,
	{UDI_DMA_SEQUENTIAL, 0, 1, 0, 1, 0,
	 _UDI_CONSTR_RESTR_ASCEND}
	,
	{UDI_DMA_SLOP_IN_BITS, 0, 8, 0, 8, 0,
	 _UDI_CONSTR_RESTR_ASCEND}
	,
	{UDI_DMA_SLOP_OUT_BITS, 0, 8, 0, 8, 0,
	 _UDI_CONSTR_RESTR_ASCEND}
	,
	{UDI_DMA_SLOP_OUT_EXTRA, 0, 0xffff, 0, 0xffff, 0,
	 _UDI_CONSTR_RESTR_ASCEND}
	,
	{UDI_DMA_SLOP_BARRIER_BITS, 0, 255, 1, 0, 1,
	 _UDI_CONSTR_RESTR_ASCEND_ZMOST}
};

/*
 * Mapping table to convert sparse attribute type code to internal
 * dense index. Subtract one from type code before indexing into table.
 */
udi_dma_constraints_attr_t _udi_constraints_index[_UDI_CONSTRAINTS_TYPES_MAX] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	1,				/* UDI_DMA_DATA_ADDRESSABLE_BITS */
	2,				/* UDI_DMA_NO_PARTIAL */
	0, 0, 0, 0, 0, 0, 0, 0,
	3,				/* UDI_DMA_SCGTH_MAX_ELEMENTS */
	4,				/* UDI_DMA_SCGTH_FORMAT */
	5,				/* UDI_DMA_SCGTH_ENDIANNESS */
	6,				/* UDI_DMA_SCGTH_ADDRESSABLE_BITS */
	7,				/* UDI_DMA_SCGTH_MAX_SEGMENTS */
	0, 0, 0, 0, 0,
	8,				/* UDI_DMA_SCGTH_ALIGNMENT_BITS */
	9,				/* UDI_DMA_SCGTH_MAX_EL_PER_SEG */
	10,				/* UDI_DMA_SCGTH_PREFIX_BYTES */
	0, 0, 0, 0, 0, 0, 0,
	11,				/* UDI_DMA_ELEMENT_ALIGNMENT_BITS */
	12,				/* UDI_DMA_ELEMENT_LENGTH_BITS */
	13,				/* UDI_DMA_ELEMENT_GRANULARITY_BITS */
	0, 0, 0, 0, 0, 0, 0,
	14,				/* UDI_DMA_ADDR_FIXED_BITS */
	15,				/* UDI_DMA_ADDR_FIXED_TYPE */
	16,				/* UDI_DMA_ADDR_FIXED_VALUE_LO */
	17,				/* UDI_DMA_ADDR_FIXED_VALUE_HI */
	0, 0, 0, 0, 0, 0,
	18,				/* UDI_DMA_SEQUENTIAL */
	19,				/* UDI_DMA_SLOP_IN_BITS */
	20,				/* UDI_DMA_SLOP_OUT_BITS */
	21,				/* UDI_DMA_SLOP_OUT_EXTRA */
	22				/* UDI_DMA_SLOP_BARRIER_BITS */
};

/* These are the default values for constraints, derived from above */
STATIC _udi_dma_constraints_t _udi_default_dma_constr = {
	{{0, 0}, 0, 0},
	{
	 255,				/* UDI_DMA_DATA_ADDRESSABLE_BITS */
	 0,				/* UDI_DMA_NO_PARTIAL */
	 0,				/* UDI_DMA_SCGTH_MAX_ELEMENTS */
	 UDI_SCGTH_DMA_MAPPED,		/* UDI_DMA_SCGTH_FORMAT */
	 UDI_DMA_ANY_ENDIAN,		/* UDI_DMA_SCGTH_ENDIANNESS */
	 255,				/* UDI_DMA_SCGTH_ADDRESSABLE_BITS */
	 0,				/* UDI_DMA_SCGTH_MAX_SEGMENTS */
	 0,				/* UDI_DMA_SCGTH_ALIGNMENT_BITS */
	 0,				/* UDI_DMA_SCGTH_MAX_EL_PER_SEG */
	 0,				/* UDI_DMA_SCGTH_PREFIX_BYTES */
	 0,				/* UDI_DMA_ELEMENT_ALIGNMENT_BITS */
	 0,				/* UDI_DMA_ELEMENT_LENGTH_BITS */
	 0,				/* UDI_DMA_ELEMENT_GRANULARITY_BITS */
	 0,				/* UDI_DMA_ADDR_FIXED_BITS */
	 1,				/* UDI_DMA_ADDR_FIXED_TYPE */
	 0,				/* UDI_DMA_ADDR_FIXED_VALUE_LO */
	 0,				/* UDI_DMA_ADDR_FIXED_VALUE_HI */
	 0,				/* UDI_DMA_SEQUENTIAL */
	 0,				/* UDI_DMA_SLOP_IN_BITS */
	 0,				/* UDI_DMA_SLOP_OUT_BITS */
	 0,				/* UDI_DMA_SLOP_OUT_EXTRA */
	 1				/* UDI_DMA_SLOP_BARRIER_BITS */
	 }
};
_udi_dma_constraints_t *_udi_default_dma_constraints =
	&_udi_default_dma_constr;

STATIC udi_boolean_t
_udi_merge_one_constraints_attribute(_udi_dma_constraints_t *constraints,
				     udi_ubit32_t attr_index,
				     udi_ubit32_t new_attr_value,
				     udi_ubit32_t restriction)
{
	udi_ubit32_t min_value;
	udi_ubit32_t max_value;
	udi_ubit32_t least_restr;
	udi_ubit32_t most_restr;
	udi_ubit8_t restr_type;
	udi_ubit32_t *attr = constraints->attributes;

	_OSDEP_ASSERT(attr_index < _UDI_CONSTRAINTS_INDEX_MAX);

	min_value = _udi_constraints_range_attributes[attr_index].min_value;
	max_value = _udi_constraints_range_attributes[attr_index].max_value;
	least_restr =
		_udi_constraints_range_attributes[attr_index].least_restr;
	most_restr = _udi_constraints_range_attributes[attr_index].most_restr;
	restr_type = _udi_constraints_range_attributes[attr_index].restr_type;

	/*
	 * Check for invalid constraint restriction type 
	 */
	_OSDEP_ASSERT(restr_type == _UDI_CONSTR_RESTR_NONE ||
		      restr_type == _UDI_CONSTR_RESTR_ASCEND ||
		      restr_type == _UDI_CONSTR_RESTR_ASCEND_ZMOST ||
		      restr_type == _UDI_CONSTR_RESTR_DESCEND ||
		      restr_type == _UDI_CONSTR_RESTR_ADD);
	_OSDEP_ASSERT(restriction == UDI_ATTR_MOST_RESTRICTIVE ||
		      restriction == UDI_ATTR_LEAST_RESTRICTIVE);

	/*
	 * Check for out of range values
	 */
	_OSDEP_ASSERT(new_attr_value >= min_value &&
		      new_attr_value <= max_value);

	/*
	 * Check for overflow
	 */
	_OSDEP_ASSERT(restr_type != _UDI_CONSTR_RESTR_ADD ||
		      (new_attr_value + attr[attr_index] >= min_value &&
		       new_attr_value + attr[attr_index] <= max_value));

	/*
	 * Then, set it appropriately 
	 */
	switch (restr_type) {
	case _UDI_CONSTR_RESTR_NONE:
		attr[attr_index] = new_attr_value;
		break;
	case _UDI_CONSTR_RESTR_ADD:
		attr[attr_index] += new_attr_value;
		break;
	case _UDI_CONSTR_RESTR_ASCEND:
		_OSDEP_ASSERT(most_restr != 0);
		if (restriction == UDI_ATTR_MOST_RESTRICTIVE) {
			if (attr[attr_index] < new_attr_value) {
				attr[attr_index] = new_attr_value;
			}
		} else {		/* restriction == UDI_ATTR_LEAST_RESTRICTIVE */

			if (attr[attr_index] > new_attr_value) {
				attr[attr_index] = new_attr_value;
			}
		}
		break;
	case _UDI_CONSTR_RESTR_ASCEND_ZMOST:
		_OSDEP_ASSERT(most_restr == 0);
		if (restriction == UDI_ATTR_MOST_RESTRICTIVE) {
			if (attr[attr_index] < new_attr_value ||
			    new_attr_value == 0) {
				attr[attr_index] = new_attr_value;
			}
		} else {		/* restriction == UDI_ATTR_LEAST_RESTRICTIVE */

			if (attr[attr_index] > new_attr_value &&
			    new_attr_value != 0) {
				attr[attr_index] = new_attr_value;
			}
		}
		break;
	case _UDI_CONSTR_RESTR_DESCEND:
		if (restriction == UDI_ATTR_MOST_RESTRICTIVE) {
			if (attr[attr_index] == 0) {
				attr[attr_index] = new_attr_value;
			} else if (new_attr_value != 0) {
				if (attr[attr_index] > new_attr_value) {
					attr[attr_index] = new_attr_value;
				}
			}
		} else {		/* restriction == UDI_ATTR_LEAST_RESTRICTIVE */

			if (new_attr_value == 0) {
				attr[attr_index] = 0;
			} else if (attr[attr_index] != 0) {
				if (attr[attr_index] < new_attr_value) {
					attr[attr_index] = new_attr_value;
				}
			}
		}
		break;
	}
	return (TRUE);
}

STATIC void
_udi_constraints_attr_set_complete(_udi_alloc_marshal_t *allocm,
				   void *new_mem)
{
	_udi_cb_t *cb = UDI_BASE_STRUCT(allocm, _udi_cb_t, cb_allocm);
	_udi_dma_constraints_t *src_constr =
		allocm->_u.constraints_attr_set_request.src_constr;
	const udi_dma_constraints_attr_spec_t *attr_list =
		allocm->_u.constraints_attr_set_request.attr_list;
	udi_ubit16_t list_length =
		allocm->_u.constraints_attr_set_request.list_length;
	_udi_dma_constraints_t *new_constr = new_mem;
	_udi_dma_constraints_t orig_constraints;
	const udi_dma_constraints_attr_spec_t *as;
	udi_dma_constraints_attr_t attr_type;
	udi_ubit32_t attr_value;
	udi_dma_constraints_attr_t attr_index;
	udi_boolean_t compound_attr;
	udi_status_t status = UDI_OK;

	if (src_constr == NULL) {
		_OSDEP_ASSERT(new_mem != NULL);

		/*
		 * Starting fresh; set all constraints to defaults 
		 */
		for (attr_index = 0; attr_index < _UDI_CONSTRAINTS_INDEX_MAX;
		     attr_index++) {
			*((udi_ubit32_t *)new_constr->attributes +
			  attr_index) =
_udi_constraints_range_attributes[attr_index].default_value;
		}
	} else {
		/*
		 * First, copy the dst constraints 
		 */
		if (new_mem == NULL) {
			new_constr = src_constr;
			/*
			 * Save original constraints, so we can back out
			 * in case of failure.
			 */
			orig_constraints = *src_constr;
		} else {
			*new_constr = *src_constr;
		}
	}

	compound_attr = FALSE;

	/*
	 * Set each of the passed-in constraints.
	 */
	for (as = attr_list; (udi_ubit32_t)(as - attr_list) < list_length;
	     as++) {
		attr_type = as->attr_type;
		attr_value = as->attr_value;

		_OSDEP_ASSERT(attr_type &&
			      attr_type <= _UDI_CONSTRAINTS_TYPES_MAX);

		/*
		 * Check if the value for the constraint attribute is
		 * supported by the OS. If not, print out a warning message
		 * and set the status.
		 */
		if (!_OSDEP_CONSTRAINTS_ATTR_CHECK(attr_type, attr_value)) {
			_OSDEP_PRINTF("WARNING! Constraints attribute %d has"
				      " an unsupported value: %d.\n",
				      attr_type, attr_value);
			status = UDI_STAT_NOT_SUPPORTED;
			continue;
		}

		switch (attr_type) {
		case UDI_DMA_ADDRESSABLE_BITS:
			if (!compound_attr) {
				attr_type = UDI_DMA_DATA_ADDRESSABLE_BITS;
				compound_attr = TRUE;
				as--;
			} else {
				attr_type = UDI_DMA_SCGTH_ADDRESSABLE_BITS;
				compound_attr = FALSE;
			}
			break;
		case UDI_DMA_ALIGNMENT_BITS:
			if (!compound_attr) {
				attr_type = UDI_DMA_ELEMENT_ALIGNMENT_BITS;
				compound_attr = TRUE;
				as--;
			} else {
				attr_type = UDI_DMA_SCGTH_ALIGNMENT_BITS;
				compound_attr = FALSE;
			}
			break;
		}

		attr_index = _udi_constraints_index[attr_type - 1];

		_OSDEP_ASSERT(attr_index != 0);

		if (!_udi_merge_one_constraints_attribute(new_constr,
							  attr_index - 1,
							  attr_value,
							  UDI_ATTR_MOST_RESTRICTIVE))
		{
			status = UDI_STAT_NOT_SUPPORTED;
			break;
		}
	}

	allocm->_u.constraints_attr_set_result.new_memory = FALSE;

	if (status == UDI_OK) {
		SET_RESULT_UDI_CT_CONSTRAINTS_ATTR_SET(allocm,
						       (udi_dma_constraints_t)
						       new_constr, UDI_OK);
		if (new_mem != NULL) {
			new_constr->alloc_hdr.ah_type =
				_UDI_ALLOC_HDR_CONSTRAINT;
			_udi_add_alloc_to_tracker(_UDI_CB_TO_CHAN(cb)->
						  chan_region,
						  &new_constr->alloc_hdr);
			UDI_QUEUE_INIT(&new_constr->buf_path_q);
			allocm->_u.constraints_attr_set_result.new_memory =
				TRUE;
		}
	} else {
		SET_RESULT_UDI_CT_CONSTRAINTS_ATTR_SET(allocm,
						       (udi_dma_constraints_t)
						       src_constr, status);
		/*
		 * Revert to original constraints or free new memory. 
		 */
		if (new_mem == NULL)
			*src_constr = orig_constraints;
		else {
			_OSDEP_MEM_FREE(new_mem);
		}

		_OSDEP_PRINTF("udi_dma_constraints_attr_set failed\n");
	}
}

STATIC void
_udi_constraints_attr_set_discard(_udi_alloc_marshal_t *allocm)
{
	if (allocm->_u.constraints_attr_set_result.new_memory) {
		_OSDEP_MEM_FREE((_udi_dma_constraints_t *)
				allocm->_u.constraints_attr_set_result.
				new_constraints);
	}
}

STATIC void
_udi_constraints_attr_set_discard_incoming(_udi_alloc_marshal_t *allocm)
{
	_OSDEP_MEM_FREE((_udi_dma_constraints_t *)
			allocm->_u.constraints_attr_set_request.src_constr);
}

STATIC _udi_alloc_ops_t _udi_dma_constraints_attr_set_ops = {
	_udi_constraints_attr_set_complete,
	_udi_constraints_attr_set_discard,
	_udi_constraints_attr_set_discard_incoming,
	_UDI_ALLOC_COMPLETE_WONT_BLOCK
};

void
udi_dma_constraints_attr_set(udi_dma_constraints_attr_set_call_t *callback,
			     udi_cb_t *gcb,
			     udi_dma_constraints_t _src_constraints,
			     const udi_dma_constraints_attr_spec_t *attr_list,
			     udi_ubit16_t list_length,
			     udi_ubit8_t flags)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	_udi_dma_constraints_t *src_constraints = _src_constraints;
	udi_boolean_t immediate;
	void *new_mem;

	allocm->_u.constraints_attr_set_request.src_constr = src_constraints;
	allocm->_u.constraints_attr_set_request.attr_list = attr_list;
	allocm->_u.constraints_attr_set_request.list_length = list_length;
	allocm->_u.constraints_attr_set_request.flags = flags;

	if (!(flags & UDI_DMA_CONSTRAINTS_COPY) && src_constraints != NULL) {
		/*
		 * Since we're only changing values in the existing object,
		 * we don't need to do any allocations, and can complete
		 * the operation here.
		 */
		new_mem = NULL;
		immediate = TRUE;
	} else {
		new_mem = _OSDEP_MEM_ALLOC(sizeof (_udi_dma_constraints_t), 0,
					   UDI_NOWAIT);
		immediate = (new_mem != NULL);
	}

	if (immediate) {
		_udi_constraints_attr_set_complete(allocm, new_mem);

		_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_CONSTRAINTS_ATTR_SET,
				      &_udi_dma_constraints_attr_set_ops,
				      callback, 2,
				      (allocm->_u.constraints_attr_set_result.
				       new_constraints,
				       allocm->_u.constraints_attr_set_result.
				       status));
	} else {
		/*
		 * We need to wait for the memory, so queue it up 
		 */
		_UDI_ALLOC_QUEUE(cb, _UDI_CT_CONSTRAINTS_ATTR_SET,
				 callback, sizeof (_udi_dma_constraints_t),
				 &_udi_dma_constraints_attr_set_ops);
	}
}

void
udi_dma_constraints_attr_reset(udi_dma_constraints_t constraints,
			       udi_dma_constraints_attr_t attr_type)
{
	udi_dma_constraints_attr_t attr_index;
	_udi_dma_constraints_t *constr = constraints;

	_OSDEP_ASSERT(attr_type && attr_type <= _UDI_CONSTRAINTS_TYPES_MAX);
	attr_index = _udi_constraints_index[attr_type - 1];
	_OSDEP_ASSERT(attr_index && attr_index <= _UDI_CONSTRAINTS_INDEX_MAX);
	constr->attributes[attr_index - 1] =
		_udi_constraints_range_attributes[attr_index -
						  1].default_value;
}

void
udi_dma_constraints_free(udi_dma_constraints_t _constraints)
{
	_udi_dma_constraints_t *constraints = _constraints;
	udi_queue_t *tmp, *elem;
	_udi_buf_path_t *buf_path;

	if (constraints != NULL) {
		UDI_QUEUE_FOREACH(&constraints->buf_path_q, elem, tmp) {
			buf_path = UDI_BASE_STRUCT(elem, _udi_buf_path_t,
						   constraints_link);
			buf_path->constraints = UDI_NULL_DMA_CONSTRAINTS;
			UDI_QUEUE_REMOVE(elem);
			UDI_QUEUE_INIT(elem);
		}
		_udi_rm_alloc_from_tracker(&constraints->alloc_hdr);
		_OSDEP_MEM_FREE(constraints);
	}
}

#ifdef NOTYET				/* Removed from spec; may eventually add back a variant. */

STATIC void
_udi_constraints_attr_combine_complete(_udi_alloc_marshal_t *allocm,
				       void *new_mem)
{
	_udi_dma_constraints_t *src_constr =
		allocm->_u.constraints_attr_combine_request.src_constr;
	_udi_dma_constraints_t *new_constr = new_mem;
	_udi_cb_t *cb = UDI_BASE_STRUCT(allocm, _udi_cb_t, cb_allocm);
	udi_ubit16_t index;

	if (src_constr == NULL) {
		/*
		 * Starting fresh; set all constraints to defaults 
		 */
		for (index = 0; index < _UDI_CONSTRAINTS_INDEX_MAX; index++) {
			*(((udi_ubit32_t *)new_constr->attributes) + index) =
				_udi_constraints_range_attributes[index].
				default_value;
		}
	} else {
		/*
		 * First, copy the dst constraints 
		 */
		*new_constr = *src_constr;
	}

	new_constr->alloc_hdr.ah_type = _UDI_ALLOC_HDR_CONSTRAINT;
	_udi_add_alloc_to_tracker(_UDI_CB_TO_CHAN(cb)->chan_region,
				  &new_constr->alloc_hdr);
	UDI_QUEUE_INIT(&new_constr->buf_path_q);
	SET_RESULT_UDI_CT_CONSTRAINTS_ATTR_COMBINE(allocm,
						   (udi_dma_constraints_t)
						   new_constr);
}

void
OLD_udi_dma_constraints_attr_combine(udi_dma_constraints_attr_combine_call_t *
				     callback,
				     udi_cb_t *gcb,
				     udi_dma_constraints_t _src_constraints,
				     udi_dma_constraints_t _dst_constraints,
				     udi_ubit8_t attr_combine_method)
{
	_udi_dma_constraints_t *src_constr = _src_constraints;
	_udi_dma_constraints_t *dst_constr = _dst_constraints;
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	udi_ubit16_t index;

	void *new_mem;

	_OSDEP_ASSERT(attr_combine_method == UDI_ATTR_MOST_RESTRICTIVE ||
		      attr_combine_method == UDI_ATTR_LEAST_RESTRICTIVE);

	/*
	 * If there's already a destination constraints object, we don't need
	 * to do any allocations, and can complete the operation here.
	 */
	if (dst_constr != NULL) {
		_OSDEP_ASSERT(src_constr != NULL);

		/*
		 * Now, go through and combine the constraints 
		 */
		for (index = 0; index < _UDI_CONSTRAINTS_INDEX_MAX; index++) {
			/*
			 * some constraints attributes can not be combined
			 */
			if (_udi_constraints_range_attributes[index].restr_type
			    == _UDI_CONSTR_RESTR_NONE)
				continue;
			(void)_udi_merge_one_constraints_attribute(dst_constr,
								   index,
								   src_constr->
								   attributes
								   [index],
								   attr_combine_method);
		}

		_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_CONSTRAINTS_ATTR_COMBINE,
				      &_udi_constraints_attr_combine_ops,
				      callback, 1,
				      ((udi_dma_constraints_t)dst_constr));
		return;
	}

	if (UDI_HANDLE_IS_NULL(src_constr, udi_dma_constraints_t))
		  _OSDEP_ASSERT(UDI_HANDLE_IS_NULL(dst_constr,
						   udi_dma_constraints_t));

	allocm->_u.constraints_attr_combine_request.src_constr = src_constr;

	new_mem = _OSDEP_MEM_ALLOC(sizeof (_udi_dma_constraints_t), 0,
				   UDI_NOWAIT);

	if (new_mem != NULL) {
		/*
		 * We got the memory immediately! 
		 */
		_udi_constraints_attr_combine_complete(allocm, new_mem);
		_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_CONSTRAINTS_ATTR_COMBINE,
				      &_udi_constraints_attr_combine_ops,
				      callback, 1,
				      ((udi_dma_constraints_t)new_mem));
	} else {
		/*
		 * We need to wait for the memory, so queue it up 
		 */
		_UDI_ALLOC_QUEUE(cb, _UDI_CT_CONSTRAINTS_ATTR_COMBINE,
				 callback, sizeof (_udi_dma_constraints_t),
				 &_udi_constraints_attr_combine_ops);
	}
}

#endif /* NOTYET */

#else  /* !_UDI_PHYSIO_SUPPORTED */

char ___unused;				/* Keep the compiler happy. */

#endif /* _UDI_PHYSIO_SUPPORTED */
