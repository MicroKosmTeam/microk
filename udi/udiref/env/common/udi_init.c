
/*
 * File: env/common/udi_init.c
 *
 * UDI environment initialization.
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

#define NEW_INIT 0

#include <udi_env.h>
#include "udi_mgmt_MA.h"		/*? */

#define MYMAX(a,b) (a > b) ? (a) : (b)

/*
 * Create an populate the first region (a.k.a. "primary region") of
 * a new driver instance.   Binds region properties, cb templates, and
 * other such data appropriately.
 */
void
_udi_primary_region_init(udi_primary_init_t *primary_init_info,
			 _udi_driver_t *drv)
{
	_udi_rprop_t *rpropp;
	_udi_mei_cb_template_t *tplate;
	_udi_interface_t *iface;

	_OSDEP_ASSERT(drv != NULL);
	_OSDEP_ASSERT(primary_init_info != NULL);
	_OSDEP_ASSERT(_udi_mgmt_meta != NULL);
	_OSDEP_ASSERT(primary_init_info->rdata_size <= UDI_MIN_ALLOC_LIMIT);
	_OSDEP_ASSERT(primary_init_info->mgmt_op_flags != NULL);
	_OSDEP_ASSERT(primary_init_info->child_data_size <=
		      UDI_MIN_ALLOC_LIMIT);
	UDIENV_CHECK(primary_region_data_size_greater_than_udi_init_context,
		     primary_init_info->rdata_size >=
		     sizeof (udi_init_context_t));


	rpropp = _udi_region_prepare(0, primary_init_info->rdata_size, drv);
	UDI_ENQUEUE_HEAD(&(drv->drv_rprops), (udi_queue_t *)rpropp);

	tplate = _udi_mei_cb_init(0, UDI_MGMT_CB_NUM, 0,	/* cb_idx */
				  primary_init_info->mgmt_scratch_requirement, 0,	/* inline_size */
				  NULL);	/* inline_layout */
	drv->drv_cbtemplate[0] = tplate;

	iface = _udi_mei_ops_init(0, "udi_mgmt", UDI_MGMT_DRV_OPS_NUM, 0,	/* ops_idx */
				  (udi_ops_vector_t *)primary_init_info->
				  mgmt_ops, primary_init_info->mgmt_op_flags,
				  0, 2, _udi_mgmt_meta);
	drv->drv_interface[0] = iface;

	/*
	 * FIXME: Is this really necessary, or does above cover it? 
	 */
	drv->drv_mgmt_scratch_size =
		primary_init_info->mgmt_scratch_requirement;


}

/*
 * Regions other than the primary region are much simpler to create and
 * bind.  Do those here.
 */
void
_udi_secondary_region_init(_udi_driver_t *drv,
			   udi_secondary_init_t *sec_init_list)
{
	_udi_rprop_t *rpropp;

	if (sec_init_list == NULL)
		return;
	_OSDEP_ASSERT(drv != NULL);

	while (sec_init_list->region_idx) {
		rpropp = _udi_region_prepare(sec_init_list->region_idx,
					     sec_init_list->rdata_size, drv);
		UDI_ENQUEUE_TAIL(&(drv->drv_rprops), (udi_queue_t *)rpropp);
		sec_init_list++;
	}
}

/*
 * The heart of udi_secondary_region_prepare(), but bypasses the
 * debugging checks that it's called for a non-primary region.
 */
_udi_rprop_t *
_udi_region_prepare(udi_index_t region_idx,
		    udi_size_t rdata_size,
		    _udi_driver_t *drv)
{
	_udi_rprop_t *rpropp;
	udi_ubit32_t region_cnt, r_cnt, int_bind_cnt, i_cnt, timeout;

	_OSDEP_ASSERT(drv != NULL);
	_OSDEP_ASSERT(rdata_size >= sizeof (udi_init_context_t));
	_OSDEP_ASSERT(rdata_size <= UDI_MIN_ALLOC_LIMIT);

	rpropp = _OSDEP_MEM_ALLOC(sizeof (_udi_rprop_t), 0, UDI_WAITOK);
	rpropp->region_idx = region_idx;
	rpropp->rdata_size = rdata_size;
	region_cnt = _udi_osdep_sprops_count(drv, UDI_SP_REGION);
	int_bind_cnt = _udi_osdep_sprops_count(drv, UDI_SP_INTERN_BINDOPS);
	for (r_cnt = 0; r_cnt < region_cnt; r_cnt++) {
		if (rpropp->region_idx !=
		    _udi_osdep_sprops_get_reg_idx(drv, r_cnt))
			continue;
		rpropp->reg_attr = _udi_osdep_sprops_get_reg_attr(drv, r_cnt);
		timeout = _udi_osdep_sprops_get_reg_overrun(drv, r_cnt);
		rpropp->overrun_time.seconds = timeout / NSEC_PER_SEC;
		rpropp->overrun_time.nanoseconds = timeout % NSEC_PER_SEC;
		if (rpropp->region_idx == 0) {
			rpropp->primary_ops_idx = UDI_MA_MGMT_OPS_IDX;
			rpropp->secondary_ops_idx = 0;
			continue;
		}
		for (i_cnt = 0; i_cnt < int_bind_cnt; i_cnt++) {
			if (_udi_osdep_sprops_get_bindops
			    (drv, i_cnt, UDI_SP_INTERN_BINDOPS,
			     UDI_SP_OP_REGIONIDX) != rpropp->region_idx)
				continue;
			rpropp->primary_ops_idx =
				_udi_osdep_sprops_get_bindops(drv, i_cnt,
							      UDI_SP_INTERN_BINDOPS,
							      UDI_SP_OP_PRIMOPSIDX);
			rpropp->secondary_ops_idx =
				_udi_osdep_sprops_get_bindops(drv, i_cnt,
							      UDI_SP_INTERN_BINDOPS,
							      UDI_SP_OP_SECOPSIDX);
			rpropp->internal_bind_cb_idx =
				_udi_osdep_sprops_get_bindops(drv, i_cnt,
							      UDI_SP_INTERN_BINDOPS,
							      UDI_SP_OP_CBIDX);
			break;
		}
	}
	return rpropp;
}

#if NEW_INIT

/*
 * _udi_post_init_processing is called from _udi_post_init's
 * innermost loop to do the following:
 *   - Maximize the cb properties (marshall, vis_size) across the
 *     op_templates that are passed in the various calls to this routine.
 *   - Set inline_ptr_offset (from this op_template's visible layout)
 *     into the cb_template.
 *   - Save chain_offset and inline_ptr_offset in the corresponding
 *     drv_interface->if_op_data;
 */
STATIC void
_udi_post_init_processing(_udi_mei_cb_template_t *cb_template,
			  _udi_interface_t *drv_interface,
			  udi_mei_ops_vec_template_t *ops_template,
			  udi_mei_op_template_t *op_template,
			  udi_index_t op_idx,
			  udi_boolean_t meta_already_processed)
{
	udi_size_t this_marshal_size;
	udi_layout_t *marshal_layout;
	udi_size_t this_vis_size;
	udi_layout_t *visible_layout;
	udi_ubit16_t inline_ptr_offset = 0;
	udi_ubit16_t chain_offset = 0;
	_udi_interface_op_data_t *opdatap;

	if (!meta_already_processed) {
		/*
		 * Accumulate marshal sizes. 
		 */
		marshal_layout = op_template->marshal_layout;
		this_marshal_size =
			_udi_get_layout_size(marshal_layout, NULL, NULL);
		cb_template->cb_marshal_size =
			MYMAX(this_marshal_size, cb_template->cb_marshal_size);

		/*
		 * Accumulate visible sizes. 
		 */
		visible_layout = op_template->visible_layout;
		this_vis_size = _udi_get_layout_size(visible_layout,
						     &inline_ptr_offset,
						     &chain_offset);
		cb_template->cb_visible_size = MYMAX(this_vis_size,
						     cb_template->
						     cb_visible_size);
		/* 
		 * Keep the total cb size naturally aligned so we can 
		 * compute the scratch from the end of it and remain 
		 * assured it's aligned right.
		 */
		cb_template->cb_visible_size = _udi_alignto(cb_template->
			cb_visible_size, sizeof(_udi_naturally_aligned));
		cb_template->cb_visible_layout = visible_layout;

		/*
		 * Set inline_ptr_offset in the cb_template. 
		 */
		if (inline_ptr_offset && cb_template->cb_inline_info) {
			cb_template->cb_inline_info->inline_ptr_offset =
				inline_ptr_offset;
		}
		/*
		 * Update scratch size in cb_template. 
		 */
		cb_template->cb_scratch_size =
			MYMAX(cb_template->cb_scratch_size,
			      cb_template->cb_marshal_size);

		_OSDEP_ASSERT(cb_template->cb_visible_size <=
			      UDI_MEI_MAX_VISIBLE_SIZE);
		_OSDEP_ASSERT(cb_template->cb_marshal_size <=
			      UDI_MEI_MAX_MARSHAL_SIZE);
	}

	/*
	 * Stash the inline info pointer and the chain offset into the
	 * per-op data of the interface.  References to if_op_data are
	 * bumped by one to skip over the implicit event_ind member of
	 * the op_template.
	 */
	opdatap = &drv_interface->if_op_data[op_idx + 1];
	if (ops_template == drv_interface->if_ops_template) {
		opdatap->inline_info = cb_template->cb_inline_info;
		if (chain_offset)
			opdatap->opdata_chain_offset = chain_offset;
	}
#if 0
	printf("CB visible size %d/scratch %d/inline_ptr_offset %d\n",
	       cb_template->cb_visible_size, cb_template->cb_marshal_size,
	       inline_ptr_offset);
#endif
}
#endif /* NEW_INIT */

/*
 *  This is called by the MA to compute control block storage
 *  requirements and preserve that info in the cb_template for this 
 *  driver.  It also computes inline information and other data
 *  gleaned from the driver or module init_info.
 */
void
_udi_post_init(udi_init_t *init_info,
	       _udi_driver_t *drv)
{
	_udi_mei_cb_template_t *cb_template;
	udi_ubit32_t i, j, k;
	_udi_inline_info_t *iip;
	udi_layout_t *visible_layout;
	udi_size_t offset;
	_udi_interface_t *interface;
	udi_mei_op_template_t *op_template;
	udi_size_t *scratch_sizep;
	udi_index_t cb_select_pair[256] = { 0 };
	udi_boolean_t meta_processed;

	/*
	 * For every control block template 
	 */
	for (i = 0; i < 256; i++) {
		cb_template = drv->drv_cbtemplate[i];

		if (cb_template == NULL)
			continue;
		meta_processed = FALSE;

/* 
 * FIXME: This seems wrong.   Should the gcb be represented in the layout
 * template for this control block (no) or should we just add it in like
 * this?   If this is right, then we need to subtract the size of the gcb
 * back out of the _udi_MA_cb_t pseudo-template.
 */
		/*
		 * We always have at least a gcb visible 
		 */
		cb_template->cb_visible_size = MYMAX(sizeof (udi_cb_t),
						     cb_template->
						     cb_visible_size);

		visible_layout = NULL;

		/*
		 * For each ops_idx (w/associated interface) for this driver 
		 */
		for (j = 0; j < 256; j++) {
			_udi_meta_t *this_meta;

			interface = drv->drv_interface[j];
			if (interface == NULL) {
				continue;
			}
			/*
			 * Find the metalanguage for this interface.
			 * FIXME: We may have multiple interfaces for the
			 * same meta, so we'll needlessly process the same
			 * meta multiple times.
			 */
			this_meta = interface->if_meta;

			/*
			 * For each set of ops in the ops_vec_template list
			 * of this meta (even ones we aren't directly using).
			 */
			for (k = 0; k < this_meta->meta_n_ops_templates; k++) {
				udi_mei_ops_vec_template_t *ops_template;
				int l;

				ops_template =
					&this_meta->
					meta_info->ops_vec_template_list[k];
				op_template = ops_template->op_template_list;

				/*
				 * For each op template in this ops template 
				 */
				for (l = 0; op_template &&
				     op_template->op_name != NULL;
				     l++, op_template++) {
					udi_ubit16_t inline_ptr_offset = 0;
					udi_ubit16_t chain_offset = 0;
					udi_size_t vis_size;

					if (op_template->meta_cb_num !=
					    cb_template->cb_group)
						/*
						 * No match 
						 */
						continue;
#if NEW_INIT
					_udi_post_init_processing(cb_template,
								  interface,
								  ops_template,
								  op_template,
								  l
								  /*
								   * op_idx 
								   */
								  ,
								  meta_processed);
					meta_processed = TRUE;
#else
					/*
					 * Match 
					 */
					visible_layout =
						op_template->visible_layout;

					/*
					 * Accumulate marshal and visible sizes.
					 * Each is no smaller than itself or the
					 * layout size from the op_template.
					 */
					cb_template->cb_marshal_size =
						MYMAX(_udi_get_layout_size
						      (op_template->
						       marshal_layout, NULL,
						       NULL),
						      cb_template->
						      cb_marshal_size);

					vis_size =
						_udi_get_layout_size
						(visible_layout,
						 &inline_ptr_offset,
						 &chain_offset);

					cb_template->cb_visible_size =
						MYMAX(vis_size,
						      cb_template->
						      cb_visible_size);
					/* 
					 * Keep the total cb size naturally 
					 * aligned so we can compute the
					 * scratch from the end of it and
					 * remain assured it's aligned right.
					 */
					cb_template->cb_visible_size = 
						_udi_alignto(cb_template->
						cb_visible_size, 
						sizeof(_udi_naturally_aligned));
					cb_template->cb_visible_layout =
						visible_layout;

					if (inline_ptr_offset &&
					    cb_template->cb_inline_info) {
						cb_template->
							cb_inline_info->
							inline_ptr_offset =
							inline_ptr_offset;
					}
					/*
					 * Stash the inline info pointer into
					 * the per-op data of the interface.
					 * References to if_op_data are bumped 
					 * by one to skip over the implicit 
					 * event_ind member of the op_template.
					 */
					if (ops_template ==
					    interface->if_ops_template) {
						interface->if_op_data[l +
								      1].
							inline_info =
							cb_template->
							cb_inline_info;
					}
					if (chain_offset &&
					    ops_template ==
					    interface->if_ops_template) {
						interface->if_op_data[l +
								      1].
							opdata_chain_offset =
							chain_offset;
					}

					cb_template->cb_scratch_size =
						MYMAX(cb_template->
						      cb_scratch_size,
						      cb_template->
						      cb_marshal_size);

					_OSDEP_ASSERT(cb_template->
						      cb_visible_size <=
						      UDI_MEI_MAX_VISIBLE_SIZE);
					_OSDEP_ASSERT(cb_template->
						      cb_marshal_size <=
						      UDI_MEI_MAX_MARSHAL_SIZE);
#if 0
					printf("CB  visible size %d/scratch %d/inline_ptr_offset %d\n", cb_template->cb_visible_size, cb_template->cb_marshal_size, inline_ptr_offset);
#endif
#endif /* NEW_INIT */
				}
			}
		}

		/*
		 * Calculate complete block sizes.
		 * Scratch and marshal spaces are a union.
		 */
		/*
		 * Make sure we include the udi_cb_t in the offset
		 * calculation. Otherwise, inline data would overlap
		 * the visible data area.
		 */
		offset = sizeof (udi_cb_t) + sizeof (_udi_cb_t) +
			cb_template->cb_visible_size;
		cb_template->cb_alloc_size = offset +
			cb_template->cb_scratch_size;

		/*
		 * Add in inline size, if any, from _udi_mei_cb_init.
		 */
		iip = cb_template->cb_inline_info;
		if (iip && iip->inline_size != 0) {
			iip->inline_offset = offset;
			offset += iip->inline_size;
			cb_template->cb_alloc_size += iip->inline_size;
		}
	}

	/*
	 * put the cb_select_list in an array so that access to 
	 * it will be faster later
	 */

	if (init_info->cb_select_list != NULL)

/* fixme: cb_select_pair probably not valid here. */
		for (j = 0;
		     j < 256 && init_info->cb_select_list[j].cb_idx != 0; j++)
			cb_select_pair[init_info->cb_select_list[j].cb_idx] =
				init_info->cb_select_list[j].ops_idx;
	/*
	 * For each op template in the interface 
	 */
	for (j = 1; j < 256; j++) {
		interface = drv->drv_interface[j];
		if (interface == NULL) {
			continue;
		}
		/*
		 * For each op in the op_template array 
		 */
		for (k = 0;
		     interface->if_ops_template->op_template_list[k].op_name !=
		     NULL; k++) {

			op_template =
				&interface->if_ops_template->
				op_template_list[k];
			/*
			 * References to if_op_data are bumped by one to skip
			 * over the implicit event_ind member of the 
			 * op_template.
			 */
			scratch_sizep =
				&interface->if_op_data[k +
						       1].
				if_op_data_scratch_size;
			*scratch_sizep = 0;
			/*
			 * for each cb_template in the interface 
			 */
			for (i = 1; i < 256; i++) {
				cb_template = drv->drv_cbtemplate[i];
				if (cb_template == NULL)
					continue;
				if (cb_template->meta_idx !=
				    interface->if_meta->meta_idx)
					continue;
				if (cb_template->cb_group !=
				    op_template->meta_cb_num)
					continue;
				/*
				 * override if cb_select_list specifies so 
				 */
				if (cb_select_pair[i] == j) {
					DEBUG3(3050, 6,
					       (drv->drv_name, j, k,
						*scratch_sizep,
						cb_template->cb_idx,
						cb_template->cb_scratch_size));
					*scratch_sizep =
						cb_template->cb_scratch_size;

					break;
				}
				/*
				 * get max out of all candidate cb templates 
				 */
				if (cb_template->cb_scratch_size >
				    *scratch_sizep) {
					DEBUG3(3051, 6,
					       (drv->drv_name, j, k,
						*scratch_sizep,
						cb_template->cb_idx,
						cb_template->cb_scratch_size));
					*scratch_sizep =
						cb_template->cb_scratch_size;
				}
			}
		}
	}
}


/* 
 * Locale management for the UDI environment.
 *
 * This is used internally in the UDI environment implementation to get/set
 * the locale; the default is "C".  If the argument is non-NULL the new 
 * locale is set and returned; if the argument is NULL the current locale is
 * unchanged.
 */

STATIC char _udi_curr_locale[80] = "C";

char *
_udi_current_locale(char *new_locale)
{
	if (new_locale) {
		udi_strncpy(_udi_curr_locale, new_locale,
			    sizeof (_udi_curr_locale));
	}
	return _udi_curr_locale;
}


/* 
 * Debug Management for the UDI environment
 *
 * This is used to determine the level of debug output from the UDI
 * environment implementation.
 *
 * There are 5 levels defined:
 *     0 = no debug output
 *     1 = error debug output
 *     2 = major event debug output
 *     3 = detail debug output
 *     4 = unit-debugging output (should be removed after unit is functional)
 */

udi_ubit32_t _udi_debug_level = 0;
