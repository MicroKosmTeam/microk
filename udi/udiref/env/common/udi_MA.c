
/*
 * File: env/common/udi_MA.c
 *
 * Common code for UDI Management Agent.
 */

/*
 * Copyright udi_reference:
 * 
 * 
 *    Copyright (c) 2001 Unisys, Inc.  All rights reserved.
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
#include <udi_mgmt_MA.h>
#include <udi_MA_util.h>


/*
 * Change this at compile-time or in your favorite debugger to change
 * the default tracing events mask delivered to drivers inside a 
 * udi_usage_ind().
 */
const int _udi_default_trace_mask = 0;

/*
 * _udi_bind_channel_create returns the end of the channel
 * anchored onto the initiator of the pair.
 * but we need to set sec_region->reg_first_chan = the end that was
 * actually anchored onto the secondary region
 */
#define ASSIGN_REG_FIRST_CHAN(chan, reg) \
	reg->reg_first_chan = (chan->chan_region == reg) ? chan \
		: chan->other_end

typedef struct {
	udi_queue_t Q;
	char *meta_name;
	udi_mei_init_t *meta_init;
	udi_index_t num_templates;
	_OSDEP_DRIVER_T driver_info;
} MA_meta_elem_t;

/* 
 * MA's Region Data Area. 
 * This is not actually used by the MA, but must be here to create a region.
 */
typedef struct {
	udi_init_context_t init_context;
} MA_region_data_t;


/*
 * MA globals
 */
_udi_channel_t *_udi_MA_chan;
udi_mei_init_t *_udi_mgmt_meta;
udi_init_context_t *_udi_MA_initcontext;

/*
 * Miscellaneous private globals.
 */
_udi_driver_t *_udi_MA_driver;
STATIC _udi_region_t *_udi_MA_region;
STATIC MA_region_data_t *_udi_MA_rdata;
STATIC udi_queue_t _udi_MA_meta_Q;
STATIC udi_queue_t _udi_MA_unbound_node_list;
STATIC udi_queue_t driver_list;		/* Global driver list */
udi_MA_cb_t *_udi_op_abort_cb;		/* Single global cb for use with op_abort */


/*
 * Elements in the _udi_MA_unbound_node_list
 */
typedef struct {
	udi_queue_t link;
	_udi_dev_node_t *node;
} MA_unbound_node_elem_t;


STATIC udi_status_t _udi_MA_match_drivers(_udi_driver_t *driver);

STATIC void _udi_MA_bind(_udi_driver_t *child,
			 _udi_dev_node_t *child_node,
			 _udi_generic_call_t *callback,
			 void *arg,
			 udi_status_t *status);
STATIC void _udi_bind_to_parent1(_udi_bind_context_t *context);
STATIC void _udi_bind_to_parent2(_udi_bind_context_t *context);
STATIC void _udi_bind_to_parent3(_udi_bind_context_t *context);
STATIC void _udi_bind_to_parent4(_udi_bind_context_t *context);
STATIC void _udi_channel_bound(_udi_dev_node_t *node,
			       _udi_channel_t *chan,
		               udi_cb_t *orig_cb,
			       udi_index_t bind_cb_idx,
			       _udi_bind_context_t *context);

STATIC void
  _udi_channel_bound_with_path_handle(_udi_bind_context_t *context,
				      _udi_channel_t *bind_chan,
				      udi_index_t bind_cb_idx);
STATIC void _udi_bind2(_udi_bind_context_t *context);

void *_udi_MA_local_data(const char *,
			 udi_ubit32_t);

STATIC void _udi_MA_deinit1(_udi_reg_destroy_context_t *context);
STATIC void _udi_MA_deinit2(_udi_reg_destroy_context_t *context);
STATIC void _udi_MA_deinit3(_udi_reg_destroy_context_t *context);

STATIC void _udi_MA_enumerate(_udi_enum_context_t *context);
STATIC void _udi_MA_enumerate_release(_udi_dev_node_t *parent_node,
				      _udi_dev_node_t *child_node,
				      udi_ubit32_t child_id);

STATIC void _udi_MA_remove_unbound_node(_udi_dev_node_t *node);
STATIC void _udi_MA_add_unbound_node(_udi_dev_node_t *node);

void _udi_MA_unbind(_udi_dev_node_t *node,
		    _udi_generic_call_t *callback,
		    void *callbackarg);
STATIC void _udi_MA_usage_ind(_udi_bind_context_t *context);
STATIC void _udi_unbind1(_udi_unbind_context_t *context);
STATIC void _udi_unbind2(_udi_unbind_context_t *context);
STATIC void _udi_unbind3(_udi_devmgmt_context_t *context);
void _udi_unbind4(_udi_unbind_context_t *context);
void _udi_unbind5(_udi_unbind_context_t *context);
STATIC void _udi_unbind6(_udi_unbind_context_t *context);



/* debug functions */
void _udi_MA_print_dev_tree(char *drv);


/* Global functions from udi_MA_osdep.c. */
extern void _udi_MA_osdep_init(void);

/* MA channel ops are marked "LONG_EXEC" so they will be deferred. */
STATIC udi_ubit8_t MA_mgmt_op_flags[] = {
	UDI_OP_LONG_EXEC,
	UDI_OP_LONG_EXEC,
	UDI_OP_LONG_EXEC,
	UDI_OP_LONG_EXEC
};

/* Init info structure for MA "driver" */
STATIC udi_ops_init_t _udi_MA_ops_init_list[] = {
	{UDI_MA_MGMT_OPS_IDX, 0, UDI_MGMT_MA_OPS_NUM, 0,
	 (udi_ops_vector_t *)&_udi_MA_ops, MA_mgmt_op_flags},
	{0}
};
STATIC udi_cb_init_t _udi_MA_cb_init_list[] = {
	{UDI_MA_MGMT_CB_IDX, 0, UDI_MGMT_CB_NUM, sizeof (_udi_MA_scratch_t),
	 0, NULL}
	,
	{0}
};
udi_init_t udi_init_info = {
	NULL,				/* primary_init_info */
	NULL,				/* secondary_init_list */
	_udi_MA_ops_init_list,
	_udi_MA_cb_init_list,
	NULL,				/* gcb_init_list */
	NULL				/* cb_select_list */
};

/*
 * Entry point to init the MA.
 * Typically called by the osdep code.
 */

void
_udi_MA_init(void)
{

	_udi_MA_util_init();
	_udi_trlog_init();

	UDI_QUEUE_INIT(&_udi_MA_meta_Q);
	UDI_QUEUE_INIT(&driver_list);
	UDI_QUEUE_INIT(&_udi_MA_unbound_node_list);

	/*
	 * OS-specific initialization.
	 */
	_udi_MA_osdep_init();

}

/*
 * _udi_MA_start() -- Start up the MA "driver instance".
 */
STATIC void
_udi_MA_start(_OSDEP_DRIVER_T os_drv_init)
{
	/*
	 * "Load" the MA "driver".
	 */
	_udi_rprop_t *rpropp;

	_udi_MA_driver =
		_udi_MA_driver_load("udi", &udi_init_info, os_drv_init);

	/*
	 * Create a region for the MA.  We're not going to actually
	 * communicate over its first_channel because it's not a
	 * channel we can readily hook up to another region given 
	 * the way we create regions. 
	 */
	rpropp = _udi_region_prepare(0, sizeof (MA_region_data_t),
				     _udi_MA_driver);
	UDI_ENQUEUE_HEAD(&(_udi_MA_driver->drv_rprops), (udi_queue_t *)rpropp);
	_udi_MA_region = _udi_region_create(_udi_MA_driver, NULL, rpropp);
	UDI_QUEUE_INIT(&_udi_MA_region->reg_link);
	_udi_MA_rdata =
		(void *)&_udi_MA_region->reg_init_rdata->ird_init_context;
	_udi_MA_chan =
		_udi_channel_create(_udi_MA_region, UDI_MA_MGMT_OPS_IDX,
				    _udi_MA_rdata, TRUE)->other_end;
	UDI_QUEUE_REMOVE(&_udi_MA_chan->chan_queue);
	_udi_MA_region->reg_first_chan = _udi_MA_chan;
	_udi_MA_initcontext = &(_udi_MA_rdata->init_context);

	_udi_op_abort_cb = _udi_MA_cb_alloc();
	_OSDEP_SIMPLE_MUTEX_INIT(&_udi_op_abort_cb_mutex, "op_abort_cb_lock");

	/*
	 * Preinitialize several CBs to the pool.   The number doesn't
	 * matter as more may be added later.
	 */
	_udi_MA_add_cb_to_pool();
	_udi_MA_add_cb_to_pool();
	_udi_MA_add_cb_to_pool();
	_udi_MA_add_cb_to_pool();
	_udi_MA_add_cb_to_pool();
	_udi_MA_add_cb_to_pool();
}



/*
 * _udi_MA_deinit -- shutdown the MA... the environment is being unloaded.
 * typically called by the environment when UDI goes away.
 */
void
_udi_MA_deinit(void (*cbfn) (void))
{
	_udi_reg_destroy_context_t *context =
		_OSDEP_MEM_ALLOC(sizeof (_udi_reg_destroy_context_t),

				 0, UDI_WAITOK);

	context->final_callback = cbfn;
	_udi_MA_deinit1(context);
}

/*
 * Note: This function may recurse.
 */

STATIC void
_udi_MA_deinit1(_udi_reg_destroy_context_t *context)
{

	udi_queue_t *elem, *tmp;
	_udi_channel_t *ch;

	/*
	 * Only remove the MA when all of the region anchored to the
	 * other end of the channels have been destroyed.
	 */
	UDI_QUEUE_FOREACH(&_udi_MA_region->reg_chan_queue, elem, tmp) {
		ch = UDI_BASE_STRUCT(elem, _udi_channel_t,
				     chan_queue);

		if (ch->other_end->status == _UDI_CHANNEL_ANCHORED) {
			context->callback =
				(_udi_generic_call_t *)_udi_MA_deinit1;
			context->arg = context;
			context->region = ch->other_end->chan_region;
			DEBUG2(4030, 1,
			       (context->region->reg_driver->drv_name));
			_udi_region_destroy(context);
			return;
		}
	}
	_udi_MA_deinit2(context);
}

STATIC void
_udi_MA_deinit2(_udi_reg_destroy_context_t *context)
{
	/*
	 * _udi_MA_chan is channel that does not get used so is not torn down
	 * with the other channels 
	 */
	_OSDEP_MEM_FREE(_udi_MA_chan);

	/*
	 * need to flag the region queue not to run anymore
	 */
	context->callback = (_udi_generic_call_t *)_udi_MA_deinit3;
	context->arg = context;
	context->region = _udi_MA_region;
	_udi_MA_region = NULL;
	_udi_region_destroy(context);
}

STATIC void
_udi_MA_deinit3(_udi_reg_destroy_context_t *context)
{
	udi_queue_t *elem, *tmp;
	_udi_driver_t *driver;

	void (*final_callback) (void) = context->final_callback;

	driver = _udi_MA_driver;
	_udi_MA_driver = NULL;
	_udi_MA_destroy_driver(driver);

	/*
	 * lets unload all the meta's here maybe there is a better
	 * place to unload the one's that are not needed as drivers 
	 * unload.
	 */
	UDI_QUEUE_FOREACH(&_udi_MA_meta_Q, elem, tmp) {
		MA_meta_elem_t *meta_elem;
		meta_elem = UDI_BASE_STRUCT(elem, MA_meta_elem_t, Q);
		UDI_QUEUE_REMOVE(elem);
		_OSDEP_MEM_FREE((char *) meta_elem->meta_name);
		_OSDEP_MEM_FREE(elem);
	}

	_udi_MA_chan = NULL;
	_udi_MA_rdata = NULL;
	_udi_MA_region = NULL;
	_udi_MA_initcontext = NULL;

	/*
	 * Any further queues to the MA region, or leftovers on
	 * the region daemon will cause failures. Make sure you're
	 * deiniting your osdep environment properly.
	 */
	_udi_trlog_deinit();

	_OSDEP_SIMPLE_MUTEX_DEINIT(&_udi_op_abort_cb_mutex);
	udi_cb_free(UDI_GCB(&_udi_op_abort_cb->v.m));

	_OSDEP_MEM_FREE(context);

	_udi_MA_util_deinit();		/* cb pool is now dead. */

	(*final_callback) ();
}

/*
 * Allocate a management control block, this is really just a wrapper
 * to _udi_cb_alloc_waitok.  we want to protect _udi_MA_driver.
 */
udi_MA_cb_t *
_udi_MA_cb_alloc(void)
{
	udi_cb_t *gcb;
	_udi_cb_t orig_cb;
	udi_MA_cb_t *ma_cb;

	gcb = _udi_cb_alloc_waitok(&orig_cb,
				   _udi_MA_driver->
				   drv_cbtemplate[UDI_MA_MGMT_CB_IDX],
				   (udi_channel_t)_udi_MA_chan);
	ma_cb = UDI_MCB(gcb, udi_MA_cb_t);
	UDI_QUEUE_INIT(&ma_cb->cb_queue);
	return ma_cb;
}



/*
 * Scan meta list for an already existing entry. If the name matches check
 * that the meta_info also matches.
 *
 * Returns:
 *	NULL	entry not found
 *	non-NULL	address of queue element
 */
STATIC MA_meta_elem_t *
_udi_MA_meta_find(const char *meta_name,
		  udi_mei_init_t *meta_info)
{
	udi_queue_t *elem, *tmp;
	MA_meta_elem_t *meta_elem;

	/*
	 * Search _udi_MA_meta_Q for a matching entry. If found, check that 
	 * the meta_info is the same, if not barf mightily .
	 */
	UDI_QUEUE_FOREACH(&_udi_MA_meta_Q, elem, tmp) {
		meta_elem = UDI_BASE_STRUCT(elem, MA_meta_elem_t, Q);
		if (!udi_strcmp(meta_name, meta_elem->meta_name)) {
			if (meta_info) {
				_OSDEP_ASSERT(meta_elem->meta_init ==
					      meta_info);
			}

			return meta_elem;
		}
	}
	return NULL;
}

/*
 * Makes a metalanguage library available to the MA.
 * Typically called by the osdep code when it has a meta that it
 * wants us to know about.
 */
void *
_udi_MA_meta_load(const char *meta_name,
		  udi_mei_init_t *meta_info,
		  const _OSDEP_DRIVER_T os_drv_init)
{
	const char *provides_name;
	MA_meta_elem_t *meta_elem;
	udi_index_t n_templates;
	udi_ubit32_t cnt;

	/*
	 * See if this meta is already on the list. If not, add it.
	 */
	meta_elem = _udi_MA_meta_find(meta_name, meta_info);
	if (meta_elem != NULL) {
		DEBUG3(2012, 1, (meta_name));
		return NULL;
	}

	DEBUG2(2011, 1, (meta_name));

	/*
	 * Exhausted list, add new meta to end.
	 */

	meta_elem = _OSDEP_MEM_ALLOC(sizeof (MA_meta_elem_t), 0, UDI_WAITOK);

	meta_elem->meta_name = _OSDEP_MEM_ALLOC(udi_strlen(meta_name)+1, 
						0, UDI_WAITOK);
	udi_strcpy(meta_elem->meta_name, (char *) meta_name);
	meta_elem->meta_init = meta_info;
	meta_elem->driver_info = os_drv_init;

	UDI_ENQUEUE_TAIL(&_udi_MA_meta_Q, &meta_elem->Q);

	/*
	 * Count the number of ops vec templates for this meta.
	 */
	for (n_templates = 0;; n_templates++) {
		if (meta_info->
		    ops_vec_template_list[n_templates].meta_ops_num == 0)
			break;
	}
	meta_elem->num_templates = n_templates;

#ifdef NOTYET
	/*
	 * TODO: This should be used if/when the sprops works with meta libs
	 * as well as drivers/mappers. 
	 */
	cnt = _OSDEP_SPROPS_COUNT(meta_elem, UDI_SP_PROVIDE);
	provides_name = _OSDEP_SPROPS_GET_PROVIDES(meta_elem, 0);
#else
	cnt = 1;
	provides_name = meta_elem->meta_name;
#endif
	DEBUG3(3016, 4, (meta_elem->meta_name, n_templates, cnt, provides_name));

	if (udi_strcmp(meta_elem->meta_name, "udi_mgmt") == 0) {
		/*
		 * Mgmt meta; need special bootstrapping here; kickoff
		 * rest of MA initialization once mgmt meta is fully loaded.
		 */
		_udi_mgmt_meta = meta_info;
		_udi_MA_start(os_drv_init);
	}

	return meta_elem;
}

/*
 * Remove a previously loaded meta from the system. This routine updates the
 * _udi_MA_meta_Q list so that a subsequent reload of the meta can succeed.
 * This is typically called by the osdep code when it wants to unload
 * a meta.
 */
_OSDEP_DRIVER_T
_udi_MA_meta_unload(const char *meta_name)
{
	MA_meta_elem_t *meta_elem;
	_OSDEP_DRIVER_T driver_info;

	meta_elem = _udi_MA_meta_find(meta_name, NULL);

	/*
	 * Should we assert that the meta _must_ be in the registered list ? 
	 */
	if (meta_elem) {
		UDI_QUEUE_REMOVE(&meta_elem->Q);
		driver_info = meta_elem->driver_info;
		_OSDEP_MEM_FREE((char *) meta_elem->meta_name);
		_OSDEP_MEM_FREE(meta_elem);
		return driver_info;
	}
	udi_memset(&driver_info, 0, sizeof(driver_info));
	return driver_info;
}

/*
 * Make a driver module available to the environment.
 * Typically called by osdep code when it has found a driver (or mapper)
 * and wants to register it with us.
 */
_udi_driver_t *
_udi_MA_driver_load(const char *drv_name,
		    udi_init_t *init_info,
		    const _OSDEP_DRIVER_T os_drv_init)
{
	_udi_driver_t *drv;
	const char *meta_name = NULL;
	udi_ubit32_t parent_meta_max = 0, m_idx, child_meta_max = 0;
	udi_ubit32_t intern_meta_max = 0;
	udi_ubit32_t meta_idx = 0;
	MA_meta_elem_t *meta_elem;
	_udi_driver_meta_Q_t *new_elem = NULL;
	udi_ops_init_t *ops_init;
	udi_queue_t *elem, *tmp;
	udi_cb_init_t *cb_ilp;
	udi_gcb_init_t *gcb_ilp;

	UDIENV_CHECK(driver_name_required, drv_name != NULL);
	UDIENV_CHECK(driver_init_info_not_specified, init_info != NULL);

	DEBUG3(3010, 1, (drv_name));

	drv = _OSDEP_MEM_ALLOC(sizeof (_udi_driver_t), 0, UDI_WAITOK);

	drv->drv_name = drv_name;

	if (init_info->primary_init_info) {
		drv->enumeration_attr_list_length =
			init_info->primary_init_info->
			enumeration_attr_list_length;
		drv->child_data_size =
			init_info->primary_init_info->child_data_size;
		drv->per_parent_paths =
			init_info->primary_init_info->per_parent_paths;
	} else {
		drv->enumeration_attr_list_length = 0;
		drv->child_data_size = 0;
		drv->per_parent_paths = 0;
	}

	/*
	 * The caller may have had OS-dependent data to "smuggle" into the
	 * created _udi_driver_t's drv_osinfo structure.   Copy it in now. 
	 */
	drv->drv_osinfo = os_drv_init;

	UDI_QUEUE_INIT(&drv->drv_meta_Q);
	UDI_QUEUE_INIT(&drv->node_list);
	UDI_QUEUE_INIT(&drv->drv_rprops);
	_OSDEP_EVENT_INIT(&(drv->MA_event));

	_udi_osdep_sprops_init(drv);
	parent_meta_max = _udi_osdep_sprops_count(drv, UDI_SP_PARENT_BINDOPS);
	child_meta_max = _udi_osdep_sprops_count(drv, UDI_SP_CHILD_BINDOPS);
	intern_meta_max = _udi_osdep_sprops_count(drv, UDI_SP_INTERN_BINDOPS);

	if (init_info->primary_init_info != NULL)
		_udi_primary_region_init(init_info->primary_init_info, drv);

	DEBUG3(3011, 2, (drv_name, parent_meta_max));

	/*
	 * Obtain all parent metas 
	 */
	for (m_idx = 0; m_idx < parent_meta_max; m_idx++) {
		meta_idx = _udi_osdep_sprops_get_bindops(drv, m_idx,
				UDI_SP_PARENT_BINDOPS, UDI_SP_OP_METAIDX);

		meta_name = _udi_osdep_sprops_get_meta_if(drv, meta_idx);

		DEBUG3(3012, 3, (drv_name, meta_idx, meta_name));

		/*
		 * Check that we have the meta loaded -- fatal if we don't 
		 */

		meta_elem = _udi_MA_meta_find(meta_name, NULL);
		if (meta_elem == NULL) {
			_udi_env_log_write(_UDI_TREVENT_ENVERR,
					   UDI_LOG_ERROR, 0,
					   UDI_STAT_RESOURCE_UNAVAIL,
					   2013, meta_name);
			_udi_MA_destroy_driver(drv);
			return NULL;
		}

		new_elem = _OSDEP_MEM_ALLOC(sizeof (*new_elem), 0, UDI_WAITOK);
		new_elem->meta_name = meta_elem->meta_name;
		new_elem->meta_init = meta_elem->meta_init;
		new_elem->num_templates = meta_elem->num_templates;
		new_elem->meta_idx = meta_idx;

		UDI_ENQUEUE_TAIL(&drv->drv_meta_Q, &new_elem->Q);
	}

	DEBUG3(3013, 2, (drv_name, child_meta_max));

	/*
	 * Obtain all child metas 
	 */
	for (m_idx = 0; m_idx < child_meta_max; m_idx++) {
		meta_idx = _udi_osdep_sprops_get_bindops(drv, m_idx,
				UDI_SP_CHILD_BINDOPS, UDI_SP_OP_METAIDX);

		meta_name = _udi_osdep_sprops_get_meta_if(drv, meta_idx);

		DEBUG3(3014, 3, (drv_name, meta_idx, meta_name));

		/*
		 * Check that we have the meta loaded -- fatal if we don't 
		 */

		meta_elem = _udi_MA_meta_find(meta_name, NULL);
		if (meta_elem == NULL) {
			_udi_env_log_write(_UDI_TREVENT_ENVERR,
					   UDI_LOG_ERROR, 0,
					   UDI_STAT_RESOURCE_UNAVAIL,
					   2013, meta_name);
			_udi_MA_destroy_driver(drv);
			return NULL;
		}

		new_elem = _OSDEP_MEM_ALLOC(sizeof (*new_elem), 0, UDI_WAITOK);
		new_elem->meta_name = meta_elem->meta_name;
		new_elem->meta_init = meta_elem->meta_init;
		new_elem->num_templates = meta_elem->num_templates;
		new_elem->meta_idx = meta_idx;

		UDI_ENQUEUE_TAIL(&drv->drv_meta_Q, &new_elem->Q);
	}

	/*
	 * Obtain all internal metas 
	 */
	for (m_idx = 0; m_idx < intern_meta_max; m_idx++) {
		meta_idx = _udi_osdep_sprops_get_bindops(drv, m_idx,
				UDI_SP_INTERN_BINDOPS, UDI_SP_OP_METAIDX);

		meta_name = _udi_osdep_sprops_get_meta_if(drv, meta_idx);

		/*
		 * Check that we have the meta loaded -- fatal if we don't 
		 */

		meta_elem = _udi_MA_meta_find(meta_name, NULL);
		UDIENV_CHECK(have_intern_meta, meta_elem != NULL);

		new_elem = _OSDEP_MEM_ALLOC(sizeof (*new_elem), 0, UDI_WAITOK);
		new_elem->meta_name = meta_elem->meta_name;
		new_elem->meta_init = meta_elem->meta_init;
		new_elem->num_templates = meta_elem->num_templates;
		new_elem->meta_idx = meta_idx;

		UDI_ENQUEUE_TAIL(&drv->drv_meta_Q, &new_elem->Q);
	}

	/*
	 * Run over the init_info, calling primary_region_init, 
	 * mei_cb_init, and udi_mei_ops_init as appropriate.
	 */

	for (cb_ilp = init_info->cb_init_list; cb_ilp && cb_ilp->cb_idx;
	     cb_ilp++) {
		meta_idx = cb_ilp->meta_idx;

		meta_name = _udi_osdep_sprops_get_meta_if(drv, meta_idx);

		DEBUG3(3015, 5, (drv_name,
				 cb_ilp->cb_idx,
				 meta_idx, meta_name, cb_ilp->meta_cb_num));

		/*
		 * Check that we have the meta loaded -- fatal if we don't 
		 */

		meta_elem = _udi_MA_meta_find(meta_name, NULL);
		_OSDEP_ASSERT(meta_elem != NULL);

		drv->drv_cbtemplate[cb_ilp->cb_idx] =
			_udi_mei_cb_init(meta_idx,
				cb_ilp->meta_cb_num,	/* cb group */
				cb_ilp->cb_idx, cb_ilp->scratch_requirement,
				cb_ilp->inline_size, cb_ilp->inline_layout);
	}

	for (gcb_ilp = init_info->gcb_init_list; gcb_ilp && gcb_ilp->cb_idx;
	     gcb_ilp++) {
		drv->drv_cbtemplate[gcb_ilp->cb_idx] =
			_udi_mei_cb_init(meta_idx, 0,	/* cb_group */
				gcb_ilp->cb_idx, gcb_ilp->scratch_requirement,
				0, /* inline_size */
				0 /* inline layout */);
	}

	/*
	 * Loop through the ops_init list and initialise each op with its
	 * appropriate meta.
	 */

	UDIENV_CHECK(Driver_must_provide_an_ops_init_list_in_udi_init_info,
		     init_info->ops_init_list != NULL);

	{
		udi_mei_init_t *meta_init = NULL;
		udi_index_t num_templates = 0, meta_idx = 0;

		for (ops_init = init_info->ops_init_list;
		     ops_init->ops_idx != 0; ops_init++) {

			meta_init = NULL;
			if (ops_init->meta_idx == 0) {
				/*
				 * special: Management Metalanguage 
				 */
				meta_elem =
					_udi_MA_meta_find("udi_mgmt", NULL);
				UDIENV_CHECK(have_udi_mgmt_meta,
					     meta_elem != NULL);
				meta_init = meta_elem->meta_init;
				num_templates = meta_elem->num_templates;
				meta_idx = 0;
				meta_name = meta_elem->meta_name;
			} else {
				UDI_QUEUE_FOREACH(&drv->drv_meta_Q, elem, tmp) {
					new_elem =
						UDI_BASE_STRUCT(elem,
							_udi_driver_meta_Q_t,
							Q);
					DEBUG4(4002, 3,
					       (new_elem->meta_name,
						new_elem->meta_idx,
						ops_init->meta_idx));
					if (new_elem->meta_idx ==
					    ops_init->meta_idx) {

						meta_init = new_elem->meta_init;
						num_templates =
							new_elem->num_templates;
						meta_idx =
							new_elem->meta_idx;
						meta_name =
							new_elem->meta_name;
						break;
					}
				}
				/*
				 * this check ensures all vars are set
				 */
				UDIENV_CHECK(have_meta_info,
					     meta_init != NULL);
			}
			drv->drv_interface[ops_init->ops_idx] =
				_udi_mei_ops_init(meta_idx,
						  meta_name,
						  ops_init->meta_ops_num,
						  ops_init->ops_idx,
						  ops_init->ops_vector,
						  ops_init->op_flags,
						  ops_init->chan_context_size,
						  num_templates, meta_init);
		}
	}

	/*
	 * Get parent and/or child bind_ops from the static properties 
	 * For the management agent we 'know' that it only has a parent
	 * bind idx
	 */
	_udi_secondary_region_init(drv, init_info->secondary_init_list);
	drv->drv_parent_interfaces =
		_OSDEP_MEM_ALLOC(sizeof (_udi_bind_ops_list_t) *
				 (parent_meta_max + 1), 0, UDI_WAITOK);
	drv->drv_parent_interfaces[parent_meta_max].ops_idx = 0;
	if (parent_meta_max > 0) {
		_udi_bind_ops_list_t *this_entry;
		udi_index_t i, region_idx, ops_idx;
		udi_queue_t *listhead, *element, *tmp;

		/*
		 * Obtain parent_bind_ops 
		 */
		for (i = 0; i < parent_meta_max; i++) {
			this_entry = &(drv->drv_parent_interfaces[i]);
			ops_idx = this_entry->ops_idx =
				_udi_osdep_sprops_get_bindops(drv, i,
						UDI_SP_PARENT_BINDOPS,
						UDI_SP_OP_OPSIDX);
			region_idx =
				_udi_osdep_sprops_get_bindops(drv, i,
						UDI_SP_PARENT_BINDOPS,
						UDI_SP_OP_REGIONIDX);
			listhead = &(drv->drv_rprops);
			this_entry->rprop = NULL;
			UDI_QUEUE_FOREACH(listhead, element, tmp) {
				if (((_udi_rprop_t *)element)->region_idx ==
				    region_idx) {
					this_entry->rprop =
						(_udi_rprop_t *)element;
					break;
				}
			}
			UDIENV_CHECK(region_idx_mismatch,
				     this_entry->rprop != NULL);
			this_entry->rprop->parent_bind_cb_idx =
				_udi_osdep_sprops_get_bindops(drv, i,
					UDI_SP_PARENT_BINDOPS, UDI_SP_OP_CBIDX);
			if (drv->drv_interface[ops_idx] == NULL) {
				_udi_env_log_write(_UDI_TREVENT_ENVERR,
						   UDI_LOG_ERROR, 0,
						   UDI_STAT_NOT_UNDERSTOOD,
						   1008, ops_idx, "parent",
						   region_idx, drv->drv_name);
				_udi_MA_destroy_driver(drv);
				return NULL;
			}
			this_entry->meta_name =
				drv->drv_interface[ops_idx]->
				if_meta->meta_name;
			this_entry->meta_ops_num =
				drv->drv_interface[ops_idx]->
				if_ops_template->meta_ops_num;
		}
	}
	drv->drv_child_interfaces =
		_OSDEP_MEM_ALLOC(sizeof (_udi_bind_ops_list_t) *
				 (child_meta_max + 1), 0, UDI_WAITOK);
	drv->drv_child_interfaces[child_meta_max].ops_idx = 0;
	if (child_meta_max > 0) {
		_udi_bind_ops_list_t *this_entry;
		udi_index_t i, region_idx, ops_idx;
		udi_queue_t *listhead, *element, *tmp;

		/*
		 * Obtain child_bind_ops 
		 */
		for (i = 0; i < child_meta_max; i++) {
			this_entry = &(drv->drv_child_interfaces[i]);
			ops_idx = this_entry->ops_idx =
				_udi_osdep_sprops_get_bindops(drv, i,
					      UDI_SP_CHILD_BINDOPS,
					      UDI_SP_OP_OPSIDX);
			region_idx = _udi_osdep_sprops_get_bindops(drv, i,
					      UDI_SP_CHILD_BINDOPS,
					      UDI_SP_OP_REGIONIDX);
			listhead = &(drv->drv_rprops);
			UDI_QUEUE_FOREACH(listhead, element, tmp) {
				if (((_udi_rprop_t *)element)->region_idx ==
				    region_idx) {
					drv->drv_child_interfaces[i].rprop =
						(_udi_rprop_t *)element;
					break;
				}
			}
			if (drv->drv_interface[ops_idx] == NULL) {
				_udi_env_log_write(_UDI_TREVENT_ENVERR,
						   UDI_LOG_ERROR, 0,
						   UDI_STAT_NOT_UNDERSTOOD,
						   1008, ops_idx, "child",
						   region_idx, drv->drv_name);
				_udi_MA_destroy_driver(drv);
				return NULL;
			}
			this_entry->meta_name =
				drv->drv_interface[ops_idx]->
				if_meta->meta_name;
			this_entry->meta_ops_num =
				drv->drv_interface[ops_idx]->
				if_ops_template->meta_ops_num;
		}
	}

	DEBUG4(4001, 1, (drv_name));
	_udi_post_init(init_info, drv);

	DEBUG2(2001, 1, (drv_name));

	return drv;
}

/*
 * Create, initialize, and return a new child dev node for a 
 * given parent dev node.
 */
STATIC _udi_dev_node_t *
_udi_MA_new_node(_udi_dev_node_t *parent_node,
		 udi_ubit32_t child_ID)
{
	_udi_dev_node_t *child_node;

	/*
	 * Create the new device node. 
	 */
	child_node = _OSDEP_MEM_ALLOC(sizeof (_udi_dev_node_t), 0, UDI_WAITOK);

	/*
	 * Initialize various members of the newly created child dev node 
	 */
	_OSDEP_MUTEX_INIT(&child_node->lock, "_udi_node_lock");
	_OSDEP_EVENT_INIT(&child_node->event);
	UDI_QUEUE_INIT(&child_node->sibling);
	UDI_QUEUE_INIT(&child_node->child_list);
	UDI_QUEUE_INIT(&child_node->drv_node_list);
	UDI_QUEUE_INIT(&child_node->attribute_list);
	UDI_QUEUE_INIT(&child_node->region_list);
	UDI_QUEUE_INIT(&child_node->buf_path_list);

	/*
	 * Tell the child who its parent is. 
	 */
	child_node->parent_node = parent_node;

	child_node->parent_id = 0;	/* Set in udi_MA_bind_parent */
	child_node->bind_channel = NULL;	/* Set in udi_MA_bind */
	child_node->node_mgmt_channel = NULL;	/* Set in _udi_region_create */
	child_node->read_in_progress = 0;
	child_node->still_present = TRUE;

	/*
	 * Give the OS code a chance to grope for osinfo 
	 */
	_OSDEP_DEV_NODE_INIT(NULL, child_node);

	return child_node;
}


/*
 * return the region property that the child need to bind to.
 */
STATIC _udi_rprop_t *
_udi_match_parent_region(_udi_driver_t *parent,
			 const char **meta_name,
			 udi_index_t *ops_idx)
{
	_udi_bind_ops_list_t *cbind_ops;
	udi_index_t list_idx;

	/*
	 * special case for when child_bind_ops is declared and
	 * udi_enumerate_no_children is also used
	 */
	if (*ops_idx == 0) {
		cbind_ops = parent->drv_child_interfaces;
		*meta_name = cbind_ops->meta_name;
		*ops_idx = cbind_ops->ops_idx;
		return cbind_ops->rprop;
	}
	for (list_idx = 0, cbind_ops = parent->drv_child_interfaces;
	     cbind_ops->ops_idx != 0 && cbind_ops->ops_idx != *ops_idx;
	     cbind_ops = &(parent->drv_child_interfaces[++list_idx])) {
	}
	/*
	 *  the child was enumerated with op_idx but it did not match
	 *  one that is in the parent's child_bind_ops list
	 */
	UDIENV_CHECK(enumerated_opidx_not_found, cbind_ops->ops_idx != 0);
	*meta_name = cbind_ops->meta_name;
	return cbind_ops->rprop;
}


/*
 * create all the static regions associated with this driver
 */
STATIC void
_udi_MA_new_regions(_udi_bind_context_t *context)
{
	_udi_channel_t *mgmt_chan, *bind_chan;
	_udi_region_t *primary_region = context->primary_reg;
	_udi_region_t *sec_region;
	_udi_rprop_t *rprop;
	_udi_driver_t *child = context->driver;
	_udi_dev_node_t *child_node = context->node;

#if _UDI_PHYSIO_SUPPORTED
	int i;
#endif

	/*
	 *  all static regions created, now bind to parent if not orphan
	 */
	if (context->current_rprop == &child->drv_rprops) {
		if (context->parent_driver != NULL) {
			context->parent_prop =
				_udi_match_parent_region(
					context->parent_driver,
					&context->meta_name,
					&child_node->parent_ops_idx);
			context->list_idx = 0;
			_udi_bind_to_parent1(context);
		} else {
			_udi_bind2(context);
		}
		return;
	}
	/*
	 * dynamic regions 
	 */
	rprop = UDI_BASE_STRUCT(context->current_rprop, _udi_rprop_t, link);
	context->current_rprop = UDI_NEXT_ELEMENT(context->current_rprop);
	if (rprop->reg_attr & _UDI_RA_DYNAMIC) {
		_udi_MA_new_regions(context);
		return;
	}
	/*
	 * static regions 
	 */
	sec_region = _udi_region_create(child, child_node, rprop);
	if (rprop->region_idx == 0) {
		/*
		 * primary region
		 */
		primary_region = sec_region;
		mgmt_chan = _udi_channel_create(primary_region, 0,
				&(primary_region->reg_init_rdata->
					ird_init_context),
				TRUE);
		mgmt_chan->bind_chan = mgmt_chan->other_end->bind_chan = TRUE;
		_udi_channel_anchor(mgmt_chan, _udi_MA_region,
				    UDI_MA_MGMT_OPS_IDX,
				    &(_udi_MA_region->
				      reg_init_rdata->ird_init_context));
		primary_region->reg_first_chan = mgmt_chan->other_end;
		DEBUG2(2021, 4, (primary_region, mgmt_chan, child_node,
				 child->drv_name));
#if _UDI_PHYSIO_SUPPORTED
		/*
		 * This will come back as -1 if there was no limit specified.
		 * A valid (and common) value will be 0 to indicate a single
		 * serialization domain.
		 */
		i = _udi_osdep_sprops_get_pio_ser_lim(child) + 1;

		/*
		 * FIXME: this is a last-minute risk mitigation hack.
		 * It is legal for a driver to have *no* serialization 
		 * domains explicitly called out in udiprops.txt.  For this
		 * case, we explictly bump the number of domains again (to
		 * one).   This does mean we end up with a serialization
		 * domain allocated for ever region whether it uses PIO
		 * or not.
		 */
		if (i == 0) {
			i = 1;
		}

		if (i > 0) {
			child_node->n_pio_mutexen = i;
			child_node->pio_mutexen =
				_OSDEP_MEM_ALLOC(sizeof (_udi_pio_mutex_t) * i,
						 0, UDI_WAITOK);
			while (i--) {
				_OSDEP_MUTEX_INIT(&child_node->pio_mutexen[i].
						  lock, "regset_lock");
			}
		}
#endif
		/*
		 * node_mgmt_channel = MA side of the mgmt channel 
		 */
		child_node->node_mgmt_channel = mgmt_chan;
		context->primary_reg = primary_region;
		context->next_step = &_udi_MA_new_regions;
		_udi_MA_usage_ind(context);
	} else {
		/*
		 * secondary region
		 */
		bind_chan = _udi_bind_channel_create(primary_region,
						     rprop->primary_ops_idx,
						     sec_region,
						     rprop->secondary_ops_idx);
		ASSIGN_REG_FIRST_CHAN(bind_chan, sec_region);
		context->next_step = &_udi_MA_new_regions;
		_udi_channel_bound(child_node, bind_chan->other_end,
				   NULL, rprop->internal_bind_cb_idx, context);
	}
}


STATIC void
_udi_bind_to_parent1(_udi_bind_context_t *context)
{

	_udi_dev_node_t *child_node = context->node;
	const char *meta_name = context->meta_name;
	_udi_rprop_t *rprop;
	_udi_bind_ops_list_t *pbind_ops;
	_udi_driver_t *child = context->driver;
	_udi_region_t *new_child_region;
	_udi_channel_t *bind_chan;
	_udi_region_t *primary_region = context->primary_reg;
	udi_queue_t *listhead, *element, *tmp;


	/*
	 * gone through all the parent interfaces
	 */
	pbind_ops = &child->drv_parent_interfaces[context->list_idx];
	if (pbind_ops->ops_idx == 0) {
		_udi_bind2(context);
		return;
	}

	/*
	 * skip the ones that speak a different meta
	 */
	if (udi_strcmp(pbind_ops->meta_name, meta_name)) {
		context->list_idx++;
		_udi_bind_to_parent1(context);
		return;
	}
	/*
	 * create or get the child_region 
	 */
	new_child_region = NULL;
	if (pbind_ops->rprop->reg_attr & _UDI_RA_DYNAMIC) {
		rprop = pbind_ops->rprop;
		new_child_region =
			_udi_region_create(child, child_node, rprop);
		bind_chan =
			_udi_bind_channel_create(primary_region,
						 rprop->primary_ops_idx,
						 new_child_region,
						 rprop->secondary_ops_idx);
		ASSIGN_REG_FIRST_CHAN(bind_chan, new_child_region);
		context->next_step = &_udi_bind_to_parent2;
		context->sec_region = new_child_region;
		_udi_channel_bound(child_node, bind_chan->other_end,
				   NULL,
				   pbind_ops->rprop->internal_bind_cb_idx,
				   context);
	} else {
		listhead = &child_node->region_list;
		UDI_QUEUE_FOREACH(listhead, element, tmp) {
			new_child_region =
				UDI_BASE_STRUCT(element, _udi_region_t,
						reg_link);

			if (new_child_region->reg_prop == pbind_ops->rprop)
				break;
		}
		context->sec_region = new_child_region;
		UDIENV_CHECK(child_region_not_found, new_child_region != NULL);
		_udi_bind_to_parent2(context);
	}
}


STATIC void
_udi_bind_to_parent2(_udi_bind_context_t *context)
{

	_udi_region_t *ppr;		/* parent primary region */
	udi_queue_t *listhead, *element, *tmp;
	_udi_region_t *new_parent_region;
	_udi_rprop_t *match_rprop = context->parent_prop;
	_udi_channel_t *bind_chan;
	_udi_dev_node_t *node = context->node;
	_udi_dev_node_t *parent_node = node->parent_node;


	/*
	 * create or get the parent_region 
	 */
	new_parent_region = NULL;
	if (match_rprop->reg_attr & _UDI_RA_DYNAMIC) {
		ppr = NULL;
		UDI_QUEUE_FOREACH(&parent_node->region_list, element, tmp) {
			ppr = UDI_BASE_STRUCT(element, _udi_region_t,
					      reg_link);

			if (ppr->reg_prop->region_idx == 0)
				break;
		}
		UDIENV_CHECK(Primary_region_not_found,
			     ppr->reg_prop->region_idx == 0);
		context->parent_reg = ppr;
		new_parent_region =
			_udi_region_create(context->parent_driver,
					   parent_node, match_rprop);
		bind_chan =
			_udi_bind_channel_create(ppr,
						 match_rprop->primary_ops_idx,
						 new_parent_region,
						 match_rprop->
						 secondary_ops_idx);
		ASSIGN_REG_FIRST_CHAN(bind_chan, new_parent_region);
		context->next_step = &_udi_bind_to_parent3;
		context->psec_region = new_parent_region;
		_udi_channel_bound(node, bind_chan->other_end,
				   NULL, match_rprop->internal_bind_cb_idx,
				   context);
	} else {
		listhead = &parent_node->region_list;
		UDI_QUEUE_FOREACH(listhead, element, tmp) {
			new_parent_region =
				UDI_BASE_STRUCT(element, _udi_region_t,
						reg_link);

			if (new_parent_region->reg_prop == match_rprop)
				break;
		}
		UDIENV_CHECK(parent_region_not_found,
			     new_parent_region != NULL);
		context->psec_region = new_parent_region;
		_udi_bind_to_parent3(context);
	}
}

STATIC void
_udi_bind_to_parent3(_udi_bind_context_t *context)
{

	_udi_region_t *new_parent_region = context->psec_region;
	_udi_channel_t *bind_chan;
	_udi_dev_node_t *child_node = context->node;
	_udi_region_t *new_child_region = context->sec_region;
	_udi_bind_ops_list_t *pbind_ops =
		&context->driver->drv_parent_interfaces[context->list_idx];

	/*
	 * tell child to bind to parent 
	 */
	bind_chan = _udi_bind_channel_create(new_parent_region,
				child_node->parent_ops_idx, new_child_region,
				pbind_ops->ops_idx);
	child_node->bind_channel = bind_chan;
	context->next_step = &_udi_bind_to_parent4;
	if (!context->driver->per_parent_paths) {
		_udi_channel_bound(child_node, bind_chan->other_end,
				   NULL, pbind_ops->rprop->parent_bind_cb_idx,
				   context);
	} else {
		_udi_channel_bound_with_path_handle(context,
				bind_chan->other_end,
				pbind_ops->rprop->parent_bind_cb_idx);
	}
}

STATIC void
_udi_bind_to_parent4(_udi_bind_context_t *context)
{

	/*
	 * if the parent binding failed do not continue else move on to
	 * next ops_idx
	 */
	if (*context->status != UDI_OK) {
		_udi_bind2(context);
	} else {
		context->list_idx++;
		_udi_bind_to_parent1(context);
	}
}


STATIC void
_udi_enumerate_start(_udi_dev_node_t *node,
		     _udi_generic_call_t *callback,
		     void *arg,
		     udi_status_t *status,
		     _udi_driver_t *driver)
{

	_udi_enum_context_t *context;
	udi_MA_cb_t *cb;
	udi_size_t length;

	cb = _udi_MA_cb_alloc();
	context =
		_OSDEP_MEM_ALLOC(sizeof (_udi_enum_context_t), 0, UDI_WAITOK);
	context->node = node;
	context->enum_level = UDI_ENUMERATE_START;
	context->callback = callback;
	context->arg = arg;
	context->driver = driver;
	context->status = status;
	context->cb = &cb->v.en;

	UDI_GCB(context->cb)->channel = (udi_channel_t)node->node_mgmt_channel;
	UDI_GCB(context->cb)->initiator_context = context;

	length = context->driver->enumeration_attr_list_length;
	if (length) {
		cb->v.en.attr_list =
			_OSDEP_MEM_ALLOC(sizeof (udi_instance_attr_list_t) *
					 length, 0, UDI_WAITOK);
	} else {
		cb->v.en.attr_list = NULL;
	}
	_udi_MA_enumerate(context);
}

/*
 * bind did not succeed.  node has been unbound. lets move on
 */
STATIC void
_udi_bind3(void *arg)
{
	_udi_bind_context_t *context = (_udi_bind_context_t *)arg;
	_udi_generic_call_t *callback = context->callback;
	void *arg2 = context->arg;

	_OSDEP_MEM_FREE(context);
	(*callback) (arg2);
}

STATIC void
_udi_bind2(_udi_bind_context_t *context)
{

	_udi_driver_t *child = context->driver;
	_udi_dev_node_t *child_node = context->node;

	/*
	 * If the driver refused the bind, undo the bind then
	 * propagate that information back up.  for the async case
	 * there is no up, so free context here
	 */
	if (*context->status != UDI_OK) {
		_udi_MA_unbind(child_node, _udi_bind3, context);
		/* Bind didn't complete; put node on unbound queue */
		_udi_MA_add_unbound_node(child_node);
		return;
	}

	_OSDEP_MUTEX_LOCK(&child_node->lock);
	UDI_ENQUEUE_TAIL(&child->node_list, &child_node->drv_node_list);
	_OSDEP_MUTEX_UNLOCK(&child_node->lock);

	/*
	 * Start enumerating 
	 */
	_udi_enumerate_start(child_node, context->callback, context->arg,
			     context->status, context->driver);
	_OSDEP_MEM_FREE(context);

	DEBUG3(3020, 1, (child->drv_name));
}


/*
 * encapsulate the bind operation by creating a context and initing it 
 * for you
 * although the callback and arg is passed in here, they are actually
 * used at ENUMERATE_DONE, because that is when one "segment" of a 
 * bind operation is done.  a bind operation consist of many segment. 
 * a segment is defined to be the step between instantiating a driver
 * all the way to finished enumerating its children.  but the enumerated
 * children themselves bind so it is recursive.  in other words a driver
 * is finished binding when all of it's children finish binding.
 */
void
_udi_MA_bind(_udi_driver_t *driver,
	     _udi_dev_node_t *node,
	     _udi_generic_call_t *callback,
	     void *arg,
	     udi_status_t *status)
{

	_udi_bind_context_t *context;
	_udi_dev_node_t *parent_node = node->parent_node;

	/*
	 * make space for context
	 */
	context =
		_OSDEP_MEM_ALLOC(sizeof (_udi_bind_context_t), 0, UDI_WAITOK);
	context->driver = driver;
	context->node = node;
	context->count = 0;
	context->callback = callback;
	context->arg = arg;
	context->status = status;
	*status = UDI_OK;

	if (parent_node != NULL) {
		context->parent_driver = parent_node->node_mgmt_channel->
			other_end->chan_region->reg_driver;
	}
	/*
	 * Create regions for the new child instance.
	 */
	context->current_rprop = UDI_FIRST_ELEMENT(&driver->drv_rprops);
	_udi_MA_new_regions(context);
}

STATIC void
_udi_buf_path_list_release(_udi_dev_node_t *node)
{

	udi_queue_t *tmp, *elem;
	_udi_buf_path_t *buf_path;

	UDI_QUEUE_FOREACH(&node->buf_path_list, elem, tmp) {
		UDI_QUEUE_REMOVE(elem);
		buf_path = UDI_BASE_STRUCT(elem, _udi_buf_path_t, node_link);
		if ((buf_path->constraints_link.next) &&
		    !UDI_QUEUE_EMPTY(&buf_path->constraints_link)) {
			UDI_QUEUE_REMOVE(&buf_path->constraints_link);
		}
		_OSDEP_MEM_FREE(buf_path);
	}
}

/* free memory and dequeue node */
STATIC void
_udi_MA_remove_node(_udi_dev_node_t *node)
{
#if _UDI_PHYSIO_SUPPORTED
	int i;
#endif

	_udi_instance_attr_release(node);
	_udi_buf_path_list_release(node);
	UDI_QUEUE_REMOVE(&node->sibling);
	_OSDEP_MUTEX_DEINIT(&node->lock);
	if (node->attr_address_locator) {
		_OSDEP_MEM_FREE(node->attr_address_locator);
	}
	if (node->attr_physical_locator) {
		_OSDEP_MEM_FREE(node->attr_physical_locator);
	}

	_OSDEP_DEV_NODE_DEINIT(node);

#if _UDI_PHYSIO_SUPPORTED
	for (i = 0; i < node->n_pio_mutexen; i++) {
		_OSDEP_MUTEX_DEINIT(&node->pio_mutexen[i].lock);
	}
	if (node->pio_mutexen) {
		_OSDEP_MEM_FREE(node->pio_mutexen);
	}
#endif /* _UDI_PHYSIO_SUPPORTED */

	/*
	 * Free mem space when enum ack
	 */
	if (node->parent_node != NULL) {
		_udi_MA_enumerate_release(node->parent_node,
					  node, node->my_child_id);
	} else {
		_OSDEP_EVENT_DEINIT(&node->event);
		_OSDEP_MEM_FREE(node);
	}
}


/*
 * Make sure this node is no longer on the _udi_MA_unbound_node_list.
 */
STATIC void
_udi_MA_remove_unbound_node(_udi_dev_node_t *node)
{
	udi_queue_t *elem, *tmp;

	UDI_QUEUE_FOREACH(&_udi_MA_unbound_node_list, elem, tmp) {
		MA_unbound_node_elem_t *q_el;

		q_el = UDI_BASE_STRUCT(elem, MA_unbound_node_elem_t, link);

		if (q_el->node == node) {
			UDI_QUEUE_REMOVE(&q_el->link);
			_OSDEP_MEM_FREE(q_el);
			break;
		}
	}
}


STATIC void
_udi_MA_add_unbound_node(_udi_dev_node_t *new_node)
{
	MA_unbound_node_elem_t *q_el;

	/*
	 * We didn't find a matching driver.
	 * Stash the node away until we get more drivers
	 * we can try to match.
	 */
	q_el = _OSDEP_MEM_ALLOC(sizeof(MA_unbound_node_elem_t),
			UDI_MEM_NOZERO, UDI_WAITOK);
	q_el->node = new_node;

	UDI_ENQUEUE_TAIL(&_udi_MA_unbound_node_list, &q_el->link);
}



/*
 * kick off the unbind process.
 */
void
_udi_MA_unbind(_udi_dev_node_t *node,
	       _udi_generic_call_t *callback,
	       void *callbackarg)
{

	_udi_unbind_context_t *my_context;


	DEBUG3(3032, 2, (node, (node->node_mgmt_channel ?
				(node->node_mgmt_channel->other_end->
				 chan_region ? (node->node_mgmt_channel->
						other_end->chan_region->
						reg_driver ?
						node->node_mgmt_channel->
						other_end->
						chan_region->reg_driver->
						drv_name : "<unclaimed>") :
				 "<unclaimed>") : "<unclaimed>")));

	/*
	 * allocate context space for me
	 */
	my_context = _OSDEP_MEM_ALLOC(sizeof (_udi_unbind_context_t),
				      0, UDI_WAITOK);
	my_context->node = node;
	my_context->parent_context = NULL;
	my_context->reg_dest_context = NULL;
	my_context->callback = callback;
	my_context->callbackarg = callbackarg;
	_udi_unbind1(my_context);
}

STATIC void
_udi_unbind1(_udi_unbind_context_t *context)
{

	_udi_dev_node_t *node = context->node;
	_udi_dev_node_t *child_node;
	_udi_unbind_context_t *child;
	udi_queue_t *elem;

	/*
	 * Remove this node from the unbound node Q (if there)
	 * Caller will put it back on if necessary.
	 */
	_udi_MA_remove_unbound_node(node);

	/*
	 * no children to unbind 
	 */
	if (UDI_QUEUE_EMPTY(&node->child_list)) {
		_udi_unbind2(context);
		return;
	}
	/*
	 * unbind all our children. 
	 */
	elem = UDI_FIRST_ELEMENT(&node->child_list);
	child_node = UDI_BASE_STRUCT(elem, _udi_dev_node_t, sibling);

	child = _OSDEP_MEM_ALLOC(sizeof (_udi_unbind_context_t),
				 0, UDI_WAITOK);
	child->node = child_node;
	child->parent_context = context;
	_udi_unbind1(child);
}


STATIC void
_udi_unbind2(_udi_unbind_context_t *context)
{

	_udi_dev_node_t *node = context->node;
	_udi_devmgmt_context_t *mgmt_context;

	/*
	 * If this node has a driver (and regions, channels, etc.) then
	 * perform the MA activities to unbind that node.
	 */
	if (node->node_mgmt_channel != NULL) {
		mgmt_context =
			_OSDEP_MEM_ALLOC(sizeof (_udi_devmgmt_context_t), 0,
					 UDI_WAITOK);
		mgmt_context->callback = (_udi_devmgmt_call_t *) _udi_unbind3;
		mgmt_context->unbind_context = context;
		_udi_devmgmt_req(node, UDI_DMGMT_UNBIND, node->parent_id,
				 mgmt_context);
	} else {
		_udi_unbind6(context);
	}
}

STATIC void
_udi_unbind3(_udi_devmgmt_context_t *devmgmt_context)
{

	_udi_unbind_context_t *context = devmgmt_context->unbind_context;

	/*
	 * TODO: react to status != UDI_OK
	 */
	_OSDEP_MEM_FREE(devmgmt_context);
	_udi_MA_final_cleanup(context->node, context);
}

void
_udi_unbind4(_udi_unbind_context_t *unbind_context)
{
	_udi_dev_node_t *node = unbind_context->node;

#if _UDI_PHYSIO_SUPPORTED
	/*
	 * I'm not really comfortable with kicking the driver with an abort
	 * sequence while we're showing it to the door.   So this might be
	 * eventually be moved.
	 */
	if (!UDI_HANDLE_IS_NULL(node->pio_abort_handle, udi_pio_handle_t)) {
		_udi_MA_pio_trans(unbind_context, node->node_mgmt_channel,
				  node->pio_abort_handle, 0, NULL, NULL);
	} else
#endif
		_udi_unbind5(unbind_context);
}


void
_udi_unbind5(_udi_unbind_context_t *context)
{
	udi_queue_t *elem;
	_udi_region_t *region;
	_udi_reg_destroy_context_t *reg_context;

	/*
	 * Release region structures. 
	 */
	if (!UDI_QUEUE_EMPTY(&context->node->region_list)) {
		if (context->reg_dest_context == NULL)
			context->reg_dest_context =
				_OSDEP_MEM_ALLOC(sizeof
						 (_udi_reg_destroy_context_t),
						 0, UDI_WAITOK);
		reg_context = context->reg_dest_context;
		elem = UDI_FIRST_ELEMENT(&context->node->region_list);
		region = UDI_BASE_STRUCT(elem, _udi_region_t,
					 reg_link);

		reg_context->callback = (_udi_generic_call_t *)_udi_unbind5;
		reg_context->arg = (void *)context;
		reg_context->region = region;
		_udi_region_destroy(reg_context);
		return;
	}
	_OSDEP_MEM_FREE(context->reg_dest_context);
	context->reg_dest_context = NULL;
	_udi_unbind6(context);
}


STATIC void
_udi_unbind6(_udi_unbind_context_t *context)
{

	_udi_unbind_context_t *parent = context->parent_context;
	_udi_dev_node_t *node = context->node;

	/*
	 * region has been destroyed or no driver was bound to this node
	 * so this node is done.  if this is the last child
	 * then do work on parent
	 * even though the node may or may not be deallocated, the fields
	 * that pertain to having a region should be reset.
	 */
	UDI_QUEUE_INIT(&node->child_list);
	UDI_QUEUE_INIT(&node->region_list);
	node->node_mgmt_channel = NULL;
	node->bind_channel = NULL;
	UDI_QUEUE_REMOVE(&node->drv_node_list);
	UDI_QUEUE_INIT(&node->drv_node_list);

	if (parent == NULL) {
		/*
		 * remove node if this is root node 
		 */
		if (node->parent_node == NULL) {
			_udi_MA_remove_node(node);
		}
		if (context->callback != NULL) {
			(*context->callback) (context->callbackarg);
		}
		_OSDEP_MEM_FREE(context);
		return;
	}
	/*
	 * remove only when parent is being unbound 
	 */
	_udi_MA_remove_node(node);
	if (UDI_QUEUE_EMPTY(&parent->node->child_list)) {
		_udi_unbind2(parent);
		_OSDEP_MEM_FREE(context);
		return;
	}
	node = UDI_BASE_STRUCT(UDI_FIRST_ELEMENT(&parent->node->child_list),
			       _udi_dev_node_t, sibling);
	context->node = node;
	_udi_unbind1(context);
}

/*
 * Return the primary region-local data for the given instance of driver 
 * <drv_name> Typically called by mappers to find the region data.
 */
STATIC void *
_udi_MA_get_primary_rdata(_udi_dev_node_t *node)
{
	udi_queue_t *elem, *tmp;
	_udi_region_t *region;

	UDI_QUEUE_FOREACH(&node->region_list, elem, tmp) {
		region = UDI_BASE_STRUCT(elem, _udi_region_t,
					 reg_link);

		if (region->reg_prop->region_idx == 0) {
			return &region->reg_init_rdata->ird_init_context;
		}
	}
	/*
	 * getting to this function means that an instance of the driver has 
	 * been bound.  we are in big trouble if the primary region is not
	 * around.
	 */
	UDIENV_CHECK(Missing_Primary_Region, 0);
	return NULL;
}



/*
 * Return the instance number given the drv_name and the device node.
 * Similar to _udi_MA_local_data, which starts with the instance number.
 */
udi_sbit32_t
_udi_MA_dev_to_instance(const char *drv_name, _udi_dev_node_t *search_node)
{
	udi_ubit32_t n = 0;
	udi_queue_t *head, *elem, *tmp;
	_udi_driver_t *driver;
	_udi_dev_node_t *node;

	/* Search the for the driver */
	driver = _udi_driver_lookup_by_name(drv_name);
	if (driver == NULL)
		return -1;

	/* Now find the right instance */
	head = &driver->node_list;
	UDI_QUEUE_FOREACH(head, elem, tmp) {
		node = UDI_BASE_STRUCT(elem, _udi_dev_node_t, drv_node_list);
		/* compare the _udi_dev_node_t pointers */
		if (node == search_node) {
			return n;
		}
		n++;
	}
	return -1;
}


void *
_udi_MA_local_data(const char *drv_name,
		   udi_ubit32_t instance)
{
	udi_ubit32_t n = 0;
	udi_queue_t *head, *elem, *tmp;
	_udi_driver_t *driver = NULL;
	_udi_dev_node_t *node;

	driver = _udi_driver_lookup_by_name(drv_name);
	if (driver != NULL) {
		head = &driver->node_list;
		UDI_QUEUE_FOREACH(head, elem, tmp) {
			node = UDI_BASE_STRUCT(elem, _udi_dev_node_t,
					       drv_node_list);

			if (n == instance) {
				return _udi_MA_get_primary_rdata(node);
			}
			++n;
		}
	}
	return NULL;
}

/*
 * walk through all children nodes of the parent and match on child id
 * return pointer to node
 */

STATIC _udi_dev_node_t *
_udi_MA_find_node(_udi_dev_node_t *parent,
		  udi_ubit32_t child_id)
{

	udi_queue_t *elem, *tmp;
	_udi_dev_node_t *child;

	UDI_QUEUE_FOREACH(&parent->child_list, elem, tmp) {
		child = UDI_BASE_STRUCT(elem, _udi_dev_node_t,
					sibling);

		if (child->my_child_id == child_id)
			return child;
	}
	return NULL;
}

/*
 * walk through all children nodes of the parent and unbind all 
 * nodes that are !still_present
 */

STATIC void
_udi_MA_clean_node_branch(_udi_dev_node_t *parent)
{

	udi_queue_t *elem, *tmp;
	_udi_dev_node_t *child;

	UDI_QUEUE_FOREACH(&parent->child_list, elem, tmp) {
		child = UDI_BASE_STRUCT(elem, _udi_dev_node_t,
					sibling);

		if (!child->still_present) {
			_udi_MA_unbind(child, (_udi_generic_call_t *)
				       _udi_MA_remove_node, child);
		}
	}
	return;
}

void
_udi_MA_enumerate_ack(udi_enumerate_cb_t *cb,
		      udi_ubit8_t enum_result,
		      udi_index_t ops_idx)
{
	_udi_dev_node_t *new_node, *parent_node, *tmp_node;
	_udi_driver_t *child_driver;
	_udi_enum_context_t *context = UDI_GCB(cb)->initiator_context;
	udi_instance_attr_list_t *attr_ptr;
	udi_ubit8_t n, enumerate_type = context->enum_level;
	udi_boolean_t node_already_exist = TRUE;
	udi_queue_t *elem, *tmp;
	udi_ubit32_t tval;

	new_node = NULL;
	parent_node = context->node;

#define _6CONST2STR(Val,C1,C2,C3,C4,C5,C6,C7) (				\
	((Val) == C1 ? #C1 : ((Val) == C2 ? #C2 : ((Val) == C3 ? #C3 :	\
        ((Val) == C4 ? #C4 : ((Val) == C5 ? #C5 : ((Val) == C6 ? #C6 :	\
        ((Val) == C7 ? #C7 : "??"))))))))

	DEBUG3(3040, 2, (enum_result,
			 _6CONST2STR(enum_result,
				     UDI_ENUMERATE_OK, UDI_ENUMERATE_LEAF,
				     UDI_ENUMERATE_DONE, UDI_ENUMERATE_RESCAN,
				     UDI_ENUMERATE_REMOVED,
				     UDI_ENUMERATE_FAILED,
				     UDI_ENUMERATE_RELEASED)));

	/*
	 * free child_data if appropriate
	 */
	if (enum_result != UDI_ENUMERATE_OK && cb->child_data != NULL) {
		udi_mem_free(cb->child_data);
		cb->child_data = NULL;
	}
	context->cb = cb;

	switch (enum_result) {
	case UDI_ENUMERATE_OK:

		/*
		 * create new node only if node does not exist 
		 * but attributes may have changed 
		 */
		if ((new_node = _udi_MA_find_node(parent_node,
						  cb->child_ID)) == NULL) {
			new_node = _udi_MA_new_node(parent_node, cb->child_ID);
			node_already_exist = FALSE;
		}
		/* meta/ops_idx allowed to change? */

		new_node->still_present = TRUE;
		new_node->parent_ops_idx = ops_idx;

		DEBUG4(4040, 3, (new_node, cb->child_ID,
				 cb->attr_valid_length));

		attr_ptr = cb->attr_list;

		for (n = 0; n < cb->attr_valid_length; n++, attr_ptr++) {

			switch (attr_ptr->attr_type) {
			case UDI_ATTR_STRING:
				DEBUG4(4041, 2, (attr_ptr->attr_name,
						 attr_ptr->attr_value));
				break;
			case UDI_ATTR_UBIT32:
				/*
				 * Convert value to native endianness and
				 * avoid faults on non-aligned address 
				 * dereferencing
				 */
				tval = UDI_ATTR32_GET(attr_ptr->attr_value);
				udi_memcpy(attr_ptr->attr_value, &tval, 4);

				DEBUG4(4042, 2, (attr_ptr->attr_name, tval));
				break;
			case UDI_ATTR_BOOLEAN:
				DEBUG4(4043, 2, (attr_ptr->attr_name,
						 attr_ptr->attr_value));
				break;
			case UDI_ATTR_ARRAY8:
				DEBUG4(4044, 2, (attr_ptr->attr_name,
						 attr_ptr->attr_value));
				break;
			default:
				DEBUG1(1002, 2, (attr_ptr->attr_type,
						 attr_ptr->attr_name));
				break;
			}

			/*
			 * we should attach attributes to the node. 
			 */

			if (_udi_instance_attr_set(new_node,
						   attr_ptr->attr_name,
						   attr_ptr->attr_value,
						   attr_ptr->attr_length,
						   attr_ptr->attr_type,
						   FALSE) != UDI_OK) {
				DEBUG1(1003, 3, (attr_ptr->attr_name,
						 attr_ptr->attr_type,
						 new_node));
			}
		}
		context->enum_level = UDI_ENUMERATE_NEXT;
		/*
		 * the node may be bound to a driver already
		 */
		if (node_already_exist) {
			_udi_MA_enumerate(context);
			return;
		}
		new_node->my_child_id = cb->child_ID;
		UDI_ENQUEUE_TAIL(&parent_node->child_list, &new_node->sibling);

		child_driver = _udi_MA_match_children(new_node);
		if (child_driver == NULL) {
			/*
			 * Stash the node away until we get more drivers
			 * we can try to match.
			 */
			_udi_MA_add_unbound_node(new_node);

			_udi_MA_enumerate(context);
			return;
		}
		_OSDEP_DEV_NODE_INIT(child_driver, new_node);

		_udi_MA_bind(child_driver, new_node,
			     (_udi_generic_call_t *)_udi_MA_enumerate,
			     context, context->status);
		return;

	case UDI_ENUMERATE_LEAF:
		/*
		 * We must not create a child for a driver that returns
		 * UDI_ENUMERATE_LEAF: The driver must enumerate a child
		 * to bind to one, even if it is a mapper.
		 * If you have a broken driver that doesn't enumerate
		 * its children, FIX IT.
		 */

		/*
		 * If a leaf driver doesn't bind to a child, this
		 * should never occur:
		 */
		_OSDEP_ASSERT(enumerate_type != UDI_ENUMERATE_RELEASE);

		if (context->callback != NULL)
			(*context->callback) (context->arg);
		if (context->driver->enumeration_attr_list_length) {
			_OSDEP_MEM_FREE(cb->attr_list);
		}
		_OSDEP_MEM_FREE(context);
		udi_cb_free(UDI_GCB(cb));
		return;

	case UDI_ENUMERATE_RELEASED:
		_OSDEP_EVENT_DEINIT(&parent_node->event);
		_OSDEP_MEM_FREE(parent_node);
		_OSDEP_MEM_FREE(context);
		_udi_MA_return_cb(UDI_MCB(cb, udi_MA_cb_t));
		return;

	case UDI_ENUMERATE_DONE:
		_OSDEP_ENUMERATE_DONE(parent_node);
		/*
		 * clean the child list of !still_present nodes
		 */
		_udi_MA_clean_node_branch(parent_node);
		if (context->callback != NULL)
			(*context->callback) (context->arg);
		context->enum_level = UDI_ENUMERATE_NEW;
		context->status = &context->status2;
		context->callback = NULL;
		_udi_MA_enumerate(context);
		return;

	case UDI_ENUMERATE_RESCAN:
		UDIENV_CHECK(enumerat_result_mismatch,
			     enumerate_type == UDI_ENUMERATE_NEXT ||
			     enumerate_type == UDI_ENUMERATE_NEW);
		UDI_QUEUE_FOREACH(&parent_node->child_list, elem, tmp) {
			tmp_node = UDI_BASE_STRUCT(elem, _udi_dev_node_t,
						   sibling);

			tmp_node->still_present = FALSE;
		}
		context->enum_level = UDI_ENUMERATE_START;
		_udi_MA_enumerate(context);
		return;

	case UDI_ENUMERATE_REMOVED:
		UDIENV_CHECK(enumeration_result_mismatch,
			     enumerate_type == UDI_ENUMERATE_NEW);
		new_node = _udi_MA_find_node(parent_node, cb->child_ID);
		_OSDEP_ASSERT(new_node);
		_udi_MA_unbind(new_node,
			       (_udi_generic_call_t *)_udi_MA_remove_node,
			       new_node);
		context->enum_level = UDI_ENUMERATE_NEW;
		_udi_MA_enumerate(context);
		return;

	case UDI_ENUMERATE_REMOVED_SELF:
		UDIENV_CHECK(enumeration_result_mismatch,
			     enumerate_type == UDI_ENUMERATE_NEW);
		_udi_MA_unbind(parent_node,
			       (_udi_generic_call_t *)_udi_MA_remove_node,
			       parent_node);
		if (context->driver->enumeration_attr_list_length) {
			_OSDEP_MEM_FREE(cb->attr_list);
		}
		_OSDEP_MEM_FREE(context);
		udi_cb_free(UDI_GCB(cb));
		return;

	case UDI_ENUMERATE_FAILED:
		UDIENV_CHECK(enumeration_result_mismatch,
			     enumerate_type == UDI_ENUMERATE_NEW ||
			     enumerate_type == UDI_ENUMERATE_DIRECTED);
		if (enumerate_type == UDI_ENUMERATE_NEW) {
			if (context->driver->enumeration_attr_list_length) {
				_OSDEP_MEM_FREE(cb->attr_list);
			}
			_OSDEP_MEM_FREE(context);
			udi_cb_free(UDI_GCB(cb));
			return;
		}
		return;

	default:
		/* Driver error: invalid enum status */
		break;
	}
}

/*
 * Get the `devices' line from udiprops.txt for potential matching.
 * If this returns true, there is a potential match and enumeration
 * may be attepmted.
 */

/*
 * Return 0 if no match;
 * otherwise return the highest weight from the metalanguage rank function
 * for any of the driver's device lines that use the specified meta idx.
 */
STATIC udi_ubit8_t
_udi_MA_match_devline(_udi_driver_t *driver,
		_udi_dev_node_t *node,
		int meta_idx)
{
	int ret, dev_line_cnt, dev_line, dc, dev_cnt, max_dev_cnt;
	udi_ubit32_t val_32;
	udi_ubit8_t *val_array;
	char *str;
	udi_instance_attr_type_t attr_type;
	udi_boolean_t matched_driver;
	typedef union {
		char str[256];
		udi_boolean_t val_bool;
		udi_ubit32_t val_32;
	} value_t;
	char *name;
	char *sprops_value;
	value_t *value;
	udi_ubit32_t valsz;
	udi_ubit8_t max_rank = 0;
	udi_boolean_t val_bool;

	/*
	 * FIXME: I'm not convinced that these buffers will never be overwritten
	 * and I don't think they're adequately bound-checked.   Similarly, I 
	 * don't understand where the magic number came from.
	 * Since these aren't bounds checked, I'm leaving them as three separate
	 * allocations for now.   This will allow instrumented allocators to
	 * detect naughtiness if it happens.
	 */

#define BUFFER_SIZE 512

	name = _OSDEP_MEM_ALLOC(BUFFER_SIZE, 0, UDI_WAITOK);
	sprops_value = _OSDEP_MEM_ALLOC(BUFFER_SIZE, 0, UDI_WAITOK);
	value = _OSDEP_MEM_ALLOC(sizeof (value_t), 0, UDI_WAITOK);

	dev_line_cnt = _udi_osdep_sprops_count(driver, UDI_SP_DEVICE);
	DEBUG4(4060, 1, (dev_line_cnt));

	UDIENV_CHECK(No_Device_Line_With_Parent_Meta, dev_line_cnt != 0);

	matched_driver = FALSE;
	for (dev_line = 0; dev_line < dev_line_cnt; dev_line++) {


		/*
		 * If devline meta != meta_idx, skip this device line: even
		 * if the attributes match, it would be for the wrong meta.
		 */
		if (_udi_osdep_sprops_get_dev_meta_idx(driver, dev_line) !=
		    meta_idx)
			continue;

		max_dev_cnt = 0;
		dev_cnt = _udi_osdep_sprops_get_dev_nattr(driver, dev_line);

		/*
		 * Allocate storage for the attributes... need to pass
		 * the values to the metalanguage ranking function
		 */

		for (dc = 0; dc < dev_cnt; dc++) {
			/*
			 * handle all types of attributes
			 * since value can hold largest size 
			 */
			_udi_osdep_sprops_get_dev_attr_name(driver, dev_line,
					dc, name);
			ret = _udi_instance_attr_get(node, name, value,
						     sizeof (value_t),
						     &attr_type);
			if (ret == 0) {
				break;
			}
			valsz = _udi_osdep_sprops_get_dev_attr_value(driver,
						name, dev_line, attr_type,
						sprops_value);
			switch (attr_type) {
			case UDI_ATTR_STRING:
				str = sprops_value;
				if (str)
					if (udi_strcmp(str, value->str) == 0)
						continue;
				dc = dev_cnt;
				break;
			case UDI_ATTR_UBIT32:
				val_32 = *((udi_ubit32_t *)sprops_value);
				if (val_32 == value->val_32)
					continue;
				dc = dev_cnt;
				break;
			case UDI_ATTR_BOOLEAN:
				val_bool = (sprops_value[0] != 0);
				if (val_bool == value->val_bool)
					continue;
				dc = dev_cnt;
				break;
			case UDI_ATTR_ARRAY8:
				val_array = (udi_ubit8_t *)sprops_value;
				if (udi_memcmp(val_array, value->str, valsz)
				    == 0)
					continue;
				dc = dev_cnt;
				break;
			default:	/* error */
				break;
			}
		}
		/*
		 * Now check the loop exit variable.  If we had a mismatch,
		 * then the loop variable was forced too high, so it won't
		 * equal the exit value, therefore the following will only
		 * occur if everything match and the loop variable wasn't
		 * adjusted.
		 */
		if (dc == dev_cnt) {	/* potential match */
#if 1
			/*
			 * Old code that uses number of attributes
			 * instead of the metalanguate ranking function.
			 */
			if (dc >= max_dev_cnt) {
				max_dev_cnt = dc;
				matched_driver = TRUE;
				DEBUG2(201, 2, (node, driver->drv_name));
				max_rank = dc;
			}
#else
			/*
			 * Requires metas to have implemented ranking funcs.
			 */
			rank = meta->enum_rank_func(ubit_32 mask, void**array);
			if (rank >= max_rank) {
				max_rank = rank;
				matched_driver = TRUE;
				DEBUG2(201, 2, (node, driver->drv_name));
			}
#endif
		}
	}
#if 1
	/*
	 * Since we could have our only match with an empty devline (as
	 * with a mapper) we bump the count by one since 0 is failure.
	 */
	if (matched_driver)
		max_rank++;
#endif

	_OSDEP_MEM_FREE(name);
	_OSDEP_MEM_FREE(sprops_value);
	_OSDEP_MEM_FREE(value);

	return max_rank;
}


/*
 * Input: enumerated node that we don't have a driver for yet.
 *
 * Output: driver that is the best match for the node, or NULL
 * if no match can be found.
 *
 * The logic for matching a child node to a specific
 * driver is this:
 *
 * Walk the known (registered) driver list
 *
 * 1) Find a driver that speaks a parent meta that is the
 *    same as used to enumerate this node.
 * 2) Call _udi_MA_match_devline to match & rank the driver's device lines.
 * 3) Return the driver with the highest device line match.
 */

_udi_driver_t *
_udi_MA_match_children(_udi_dev_node_t *node)
{
	udi_queue_t *elem, *tmp;
	_udi_driver_t *drv, *match_driver;
	_udi_driver_t *best_drv = NULL;
	_udi_dev_node_t *p_node;
	int pm, pm_idx;
	char *pmname, *cmname;
	int parent_cnt, child_cnt;
	int best_match = 0;

	p_node = node->parent_node;

	/* Get the parent driver that enumerated the child */
	match_driver = p_node->node_mgmt_channel->other_end->
		chan_region->reg_driver;

	child_cnt = _udi_osdep_sprops_count(match_driver,
					UDI_SP_CHILD_BINDOPS);
	if (!child_cnt) {
		/* Driver enumerated a child but has no child metas */
		/* No node CAN be bound. */
		return NULL;
	}
	/*
	 * Note: we only need to check against the child
	 * meta that the parent driver enumerated.
	 */
	cmname = _udi_osdep_sprops_get_meta_if(match_driver,
			match_driver->drv_interface[node->parent_ops_idx]->
			if_meta->meta_idx);

	/* Search the driver list for the best child driver */
	UDI_QUEUE_FOREACH(&driver_list, elem, tmp) {
		drv = UDI_BASE_STRUCT(elem, _udi_driver_t, link);

		DEBUG4(4061, 1, (drv->drv_name));
		if (drv == match_driver) {
			/*
			 * Here we say a driver can't be it's own child.
			 * But I don't think there is anything in the spec
			 * that prohibits such an arrangement.
			 */
			continue;
		}

		/*
		 * The parent count is the # of metas present in the driver
		 * we are examining.  The goal is to match parent metas of
		 * the driver to the child meta enumerated.
		 */
		DEBUG4(4062, 2, (match_driver->drv_name, &drv->drv_osinfo));
		parent_cnt =
			_udi_osdep_sprops_count(drv, UDI_SP_PARENT_BINDOPS);
		for (pm = 0; pm < parent_cnt; pm++) {
			/*
			 * check driver parent metas for a match
			 */
			pm_idx = _udi_osdep_sprops_get_bindops(drv, pm,
						       UDI_SP_PARENT_BINDOPS,
						       UDI_SP_OP_METAIDX);
			pmname = _udi_osdep_sprops_get_meta_if(drv, pm_idx);

			if (udi_strcmp(pmname, cmname) == 0) {
				int weight;

				/*
				 * matched a meta, so walk the device
				 * lines for this meta and and match them
				 * against the enumeration info.
				 */
				weight = _udi_MA_match_devline(drv,
						node, pm_idx);
				if (weight > best_match) {
					best_match = weight;
					best_drv = drv;
				}
			}
		}
	}
	return best_drv;
}



/*
 * Input: newly loaded driver 
 *
 * Output: success/failure indication
 *
 * The logic for matching a child node to a specific
 * driver is this:
 *
 * Walk the known list of enumerated-but-not-bound drivers.
 * Attempt to find a child driver for each of those drivers
 * If a child driver is found, it *should* be us.  [If it is us,]
 * Remove the node from the linked list and bind it.
 */

STATIC udi_status_t
_udi_MA_match_drivers(_udi_driver_t *driver)
{
	udi_queue_t *elem, *tmp;
	_udi_driver_t *drv;
	udi_status_t status;
	_udi_dev_node_t *node;

	status = UDI_OK;

	UDI_QUEUE_FOREACH(&_udi_MA_unbound_node_list, elem, tmp) {
		MA_unbound_node_elem_t *q_el;

		q_el = UDI_BASE_STRUCT(elem, MA_unbound_node_elem_t, link);
		node = q_el->node;

		drv = _udi_MA_match_children(node);
#if 1
		/*
		 * Do we want to keep the old semantics of this call?
		 * This case *should* never occur... unless another
		 * driver is loaded before we are finished.
		 */
		if (drv != driver)
			continue;
#endif
		/* Remove the node from the unused list */
		UDI_QUEUE_REMOVE(&q_el->link);
		_OSDEP_MEM_FREE(q_el);

		/*
		 * Call _OSDEP_DEV_NODE_INIT again now that we have the
		 * driver for this node.  Should probably just be
		 * called from _udi_MA_bind.
		 */
		_OSDEP_DEV_NODE_INIT(driver, node);

		_udi_MA_bind(driver, node, _udi_operation_done,
				&node->event, &driver->MA_status);

		_OSDEP_EVENT_WAIT(&node->event);
		if (status == UDI_OK) {
			status = driver->MA_status;
		}
	}
	return status;
}


/*
 * Given a driver instance, make it known to the environment and 
 * potentially enumerate it.
 * 
 * This is typically called from the osdep code when a driver has
 * become known to the system, either via discovery or module load
 * or a similar process.
 */

udi_status_t
_udi_MA_install_driver(_udi_driver_t *drv)
{
	const char *name;
	_udi_dev_node_t *child_node;
	udi_status_t status;

	if (drv == NULL) {
		return UDI_STAT_CANNOT_BIND;
	}
	name = drv->drv_name;

	if (_udi_driver_lookup_by_name(name)) {
		/*
		 * Driver name already exists! destroy_driver_by_name
		 * will not work as intended if we don't enforce this.
		 */
		return UDI_STAT_CANNOT_BIND;
	}

	DEBUG3(3017, 1, (name));

	/*
	 * Handle the case of an orphan driver (like the bridge mapper)
	 * which have no parent metas.
	 */
	UDI_ENQUEUE_TAIL(&driver_list, &drv->link);
	if (_udi_osdep_sprops_count(drv, UDI_SP_PARENT_BINDOPS) == 0) {
		child_node = _udi_MA_new_node(NULL, 0);
		_udi_MA_bind(drv, child_node, _udi_operation_done,
			     &child_node->event, &drv->MA_status);
		_OSDEP_EVENT_WAIT(&child_node->event);
		if (drv->MA_status != UDI_OK) {
			_udi_MA_destroy_driver_by_name(drv->drv_name);
		}
		return drv->MA_status;
	}

	/*
	 * OK, time to see if the newly loaded and installed
	 * driver has any nodes waiting for it, so go
	 * call the matching function.  This can (and will likely)
	 * cause enumeration to occur.
	 */
	status = _udi_MA_match_drivers(drv);
	if (status != UDI_OK) {
		_udi_MA_destroy_driver_by_name(drv->drv_name);
	}
	return status;
}

/*
 *  Given a driver name, look for it in the driver_list and call 
 *  _udi_MA_destroy_driver on it and remove it from the queue.   
 *  This is useful for unloading drivers when only the name is known.
 *  Note that this destroys a driver, not a driver instance.
 */
void
_udi_MA_destroy_driver_by_name(const char *drv_name)
{
	udi_queue_t *elem, *tmp;
	_udi_driver_t *driver;
	_udi_dev_node_t *node;
	int keep_node;

	/*
	 * scan through all loaded drivers and unbind if necessary
	 */
	driver = _udi_driver_lookup_by_name(drv_name);
	if (driver) {
		UDI_QUEUE_FOREACH(&driver->node_list, elem, tmp) {
			node = UDI_BASE_STRUCT(elem, _udi_dev_node_t,
					       drv_node_list);

			/*
			 * We don't keep nodes for destroyed orphan drivers.
			 * Have to check here, because we might free the node
			 */
			keep_node = (node->parent_node != NULL);

			/* Removes node from unbound node queue... */
			_udi_MA_unbind(node,
				       _udi_operation_done, &driver->MA_event);
			_OSDEP_EVENT_WAIT(&driver->MA_event);

			/*
			 * Any enumerated nodes need to persist;
			 * put the node on the unbound node list
			 * so that it can be bound to a child later.
			 * XXX: OS should rescan the nodes after it
			 * has finished destroying driver(s) in case
			 * there is another match.
			 */
			if (keep_node) {
				_udi_MA_add_unbound_node(node);
			}
		}
		UDI_QUEUE_REMOVE(&driver->link);
		_udi_MA_destroy_driver(driver);
	}
}

STATIC void
_udi_MA_enumerate2(udi_cb_t *gcb,
		   void *new_mem)
{
	udi_enumerate_cb_t *enum_cb = UDI_MCB(gcb, udi_enumerate_cb_t);
	_udi_enum_context_t *context = gcb->initiator_context;

	enum_cb->child_data = new_mem;
	udi_enumerate_req(enum_cb, context->enum_level);
}

STATIC void
_udi_MA_enumerate(_udi_enum_context_t *context)
{

	udi_size_t child_data_size;
	_udi_driver_t *driver = context->driver;
	udi_enumerate_cb_t *cb = context->cb;


	cb->parent_ID = UDI_ANY_PARENT_ID;
	cb->child_data = NULL;
	cb->attr_valid_length = 0;
	if ((child_data_size = driver->child_data_size) == 0) {
		udi_enumerate_req(cb, context->enum_level);
		return;
	}
	/*
	 * Need to allocate child data
	 */
	udi_mem_alloc(&_udi_MA_enumerate2, UDI_GCB(cb),
		      driver->child_data_size,
		      UDI_MEM_NOZERO | UDI_MEM_MOVABLE);
}

STATIC void
_udi_MA_usage_ind(_udi_bind_context_t *context)
{

	_udi_MA_op_q_entry_t *op;
	_udi_dev_node_t *node = context->node;

	op = _OSDEP_MEM_ALLOC(sizeof (_udi_MA_op_q_entry_t), 0, UDI_WAITOK);
	op->op_cb.usage.gcb.channel = node->node_mgmt_channel;
	op->op_cb.usage.gcb.initiator_context = context;
	op->op_cb.usage.trace_mask = _udi_default_trace_mask;
	op->op_cb.usage.meta_idx = 0;
	op->op_type = _UDI_MA_USAGE_OP;
	op->op_param.usage.level = UDI_RESOURCES_NORMAL;
	_udi_MA_send_op(op);
}

STATIC void
_udi_channel_bound(_udi_dev_node_t *node,
		   _udi_channel_t *chan,
		   udi_cb_t *orig_cb,
		   udi_index_t bind_cb_idx,
		   _udi_bind_context_t *context)
{

	_udi_MA_op_q_entry_t *op;
	_udi_cb_t o_cb;
	_udi_driver_t *driver = context->driver;
	udi_cb_t *gcb;
	udi_channel_event_cb_t *ev_cb;

	op = _OSDEP_MEM_ALLOC(sizeof (_udi_MA_op_q_entry_t), 0, UDI_WAITOK);
	ev_cb = &op->op_cb.chan_ev;
	op->op_type = _UDI_MA_EV_OP;
	ev_cb->gcb.channel = (udi_channel_t)chan;
	ev_cb->gcb.initiator_context = context;
	ev_cb->event = UDI_CHANNEL_BOUND;
	ev_cb->params.parent_bound.parent_ID = node->parent_id;
	/*
	 * cbs with path_handles do not take this code path
	 */
	ev_cb->params.parent_bound.path_handles = NULL;
	if (bind_cb_idx) {
		gcb = _udi_cb_alloc_waitok(&o_cb,
					   driver->drv_cbtemplate[bind_cb_idx],
					   (udi_channel_t)chan->other_end);
		gcb->context = chan->other_end->chan_context;
		ev_cb->params.internal_bound.bind_cb = gcb;
	}
	_udi_MA_send_op(op);
}

/*
 * ENUMRATE_RELEASE is different from the other enumeratsion levels.
 * - the child_id is set by MA.
 * - the req is sent to the parent, but the node that the MA is concerned
 *   about is really the child.
 */
STATIC void
_udi_MA_enumerate_release(_udi_dev_node_t *parent_node,
			  _udi_dev_node_t *child_node,
			  udi_ubit32_t child_id)
{

	_udi_MA_op_q_entry_t *op;
	_udi_enum_context_t *context;
	udi_channel_t channel;

	op = _OSDEP_MEM_ALLOC(sizeof (_udi_MA_op_q_entry_t), 0, UDI_WAITOK);
	context =
		_OSDEP_MEM_ALLOC(sizeof (_udi_enum_context_t), 0, UDI_WAITOK);
	channel = op->op_cb.enu.gcb.channel =
		(udi_channel_t)parent_node->node_mgmt_channel;
	op->op_cb.enu.parent_ID = UDI_ANY_PARENT_ID;
	op->op_cb.enu.child_ID = child_id;
	op->op_cb.enu.gcb.initiator_context = context;
	context->node = child_node;
	context->enum_level = UDI_ENUMERATE_RELEASE;
	op->op_type = _UDI_MA_ENUM_OP;
	op->op_param.enu.enum_level = UDI_ENUMERATE_RELEASE;
	_udi_MA_send_op(op);
}

_udi_driver_t *
_udi_driver_lookup_by_name(const char *drvname)
{
	_udi_driver_t *drv;
	udi_queue_t *elem, *tmp;

	UDI_QUEUE_FOREACH(&driver_list, elem, tmp) {
		drv = UDI_BASE_STRUCT(elem, _udi_driver_t, link);

		if (udi_strcmp(drv->drv_name, drvname) == 0)
			return drv;
	}
	return NULL;
}

/*
 * Note:  inline_info needs to be alloc'd space because cb_realloc
 * needs to access it.  at which time this function would have returned.
 * but because we precalculate the target scratch size, it is really not
 * necessary in a running env.  although it is necessary because we
 * force realloc in a posix test
 */
STATIC void
_udi_channel_bound_with_path_handle(_udi_bind_context_t *context,
				    _udi_channel_t *bind_chan,
				    udi_index_t bind_cb_idx)
{

	_udi_driver_t *driver = context->driver;
	udi_cb_t *cb;
	udi_channel_event_cb_t *ev_cb;
	udi_index_t i;
	_udi_buf_path_t *buf_path;
	udi_buf_path_t *buf_path_handle_ptr;
	_udi_cb_t orig_cb;
	udi_ubit8_t inline_layout[4];
	_udi_inline_info_t *inline_info =
		_OSDEP_MEM_ALLOC(sizeof (_udi_inline_info_t), 0, UDI_WAITOK);
	_udi_mei_cb_template_t cb_template =
		*_udi_MA_driver->drv_cbtemplate[UDI_MA_MGMT_CB_IDX];

	inline_info->inline_size = driver->per_parent_paths *
		sizeof (udi_buf_path_t);
	cb_template.cb_alloc_size += inline_info->inline_size;
	inline_info->inline_offset = sizeof (_udi_cb_t) +
		sizeof (udi_channel_event_cb_t);
	inline_info->inline_ptr_offset = offsetof(udi_channel_event_cb_t,
						  params.parent_bound.
						  path_handles);
	if (driver->drv_mgmt_scratch_size > cb_template.cb_scratch_size)
		cb_template.cb_scratch_size = driver->drv_mgmt_scratch_size;
	inline_info->inline_layout = inline_layout;
	inline_layout[0] = UDI_DL_ARRAY;
	inline_layout[1] = driver->per_parent_paths;
	inline_layout[2] = UDI_DL_CHANNEL_T;
	inline_layout[3] = UDI_DL_END;
	cb_template.cb_inline_info = inline_info;
	cb = _udi_cb_alloc_waitok(&orig_cb,
				  &cb_template, (udi_channel_t)bind_chan);
	ev_cb = UDI_MCB(cb, udi_channel_event_cb_t);

	buf_path =
		_OSDEP_MEM_ALLOC(sizeof (_udi_buf_path_t) *
				 driver->per_parent_paths, 0, UDI_WAITOK);
	UDI_ENQUEUE_TAIL(&context->node->buf_path_list, &buf_path->node_link);
	buf_path_handle_ptr = ev_cb->params.parent_bound.path_handles;
	for (i = 0; i < driver->per_parent_paths; i++) {
		buf_path_handle_ptr[i] = (udi_buf_path_t)buf_path;
		buf_path++;
	}
	ev_cb->event = UDI_CHANNEL_BOUND;
	UDI_GCB(ev_cb)->initiator_context = context;
	if (bind_cb_idx) {
		cb = _udi_cb_alloc_waitok(&orig_cb,
					  driver->drv_cbtemplate[bind_cb_idx],
					  (udi_channel_t)bind_chan->other_end);
		cb->context = bind_chan->other_end->chan_context;
		ev_cb->params.parent_bound.bind_cb = cb;
	}
	udi_channel_event_ind(ev_cb);
}

void
_udi_MA_print_region_list(char *indent,
			  _udi_dev_node_t *node)
{

	_udi_region_t *region;
	udi_queue_t *elem, *tmp;

	UDI_QUEUE_FOREACH(&node->region_list, elem, tmp) {
		region = UDI_BASE_STRUCT(elem, _udi_region_t, reg_link);
		udi_debug_printf("%s+%p\n", indent, region);
	}
}

void
_udi_MA_print_node_tree(char *indent,
			_udi_dev_node_t *node)
{
	char new_indent[64];	/* XXX Still wrong */
	_udi_dev_node_t *child;
	udi_queue_t *elem, *tmp;

	udi_debug_printf("%s%p\n", indent, node);
	_udi_MA_print_region_list(indent, node);
	udi_strcpy(new_indent, indent);
	udi_strcat(new_indent, "\t");
	UDI_QUEUE_FOREACH(&node->child_list, elem, tmp) {
		child = UDI_BASE_STRUCT(elem, _udi_dev_node_t,
					sibling);
		_udi_MA_print_node_tree(new_indent, child);
	}
}

void
_udi_MA_print_dev_tree(char *drv)
{
	udi_queue_t *elem, *tmp;
	_udi_dev_node_t *node;
	_udi_driver_t *driver;

	driver = _udi_driver_lookup_by_name(drv);

	udi_debug_printf("Driver %s has the following node tree\n", drv);
	UDI_QUEUE_FOREACH(&driver->node_list, elem, tmp) {
		node = UDI_BASE_STRUCT(elem, _udi_dev_node_t,
				       drv_node_list);

		_udi_MA_print_node_tree("", node);
	}
}

_udi_dev_node_t *
_udi_MA_find_node_by_osinfo(_udi_match_func_t *match_func,
			    void *arg)
{

	udi_queue_t *elem, *tmp;
	udi_queue_t *elem2, *tmp2;
	_udi_driver_t *driver;
	_udi_dev_node_t *node;

	UDI_QUEUE_FOREACH(&driver_list, elem, tmp) {
		driver = UDI_BASE_STRUCT(elem, _udi_driver_t, link);

		UDI_QUEUE_FOREACH(&driver->node_list, elem2, tmp2) {
			node = UDI_BASE_STRUCT(elem2, _udi_dev_node_t,
					       drv_node_list);

			if ((*match_func) ((void *)&node->node_osinfo, arg))
				return node;
		}
	}
	return NULL;
}
