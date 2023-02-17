
/*
 * File: env/udi_imc.c
 *
 * UDI IMC (Inter-Module Communication) Services and Region Support
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
#include "udi_mgmt_MA.h"
#include "udi_MA_util.h"

/*
 * CHAN_FIRST_END(chan) returns either the specified channel endpoint
 * or the other end of the same channel, depending on which one is the
 * "first" of the two. This is used in some cases where the whole
 * channel needs to be referenced, rather than just a single endpoint.
 */
#define CHAN_FIRST_END(chan)	\
		((chan)->other_end < (chan) ? (chan)->other_end : (chan))

STATIC void _udi_channel_detach(_udi_channel_t *chan);

void
udi_channel_set_context(udi_channel_t target_channel,
			void *context)
{
	_udi_channel_t *_target_channel = target_channel;

	_target_channel->chan_context = context;
}

/*
 * _udi_channel_anchor() anchors an existing channel in the specified 
 * region using the specified channel ops index and channel context.
 */
void
_udi_channel_anchor(_udi_channel_t *channel,
		    _udi_region_t *region,
		    udi_index_t ops_idx,
		    void *channel_context)
{
	_OSDEP_ASSERT(channel != NULL);
	_OSDEP_ASSERT(region != NULL);

	/*
	 * Anchor channel in the region. 
	 */
	channel->chan_region = region;
	channel->chan_context = channel_context;
	channel->chan_interface = region->reg_driver->drv_interface[ops_idx];
	channel->status = _UDI_CHANNEL_ANCHORED;
	UDI_ENQUEUE_TAIL(&region->reg_chan_queue, &channel->chan_queue);
}

void
udi_channel_anchor(udi_channel_anchor_call_t *callback,
		   udi_cb_t *gcb,
		   udi_channel_t channel,
		   udi_index_t ops_idx,
		   void *channel_context)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_channel_t *chan = channel;

	_OSDEP_ASSERT(ops_idx != 0);

	/*
	 * This implementation of udi_channel_anchor() can always call
	 * _udi_channel_anchor() synchronously, since it doesn't support
	 * multiple domains.
	 */

	_udi_channel_anchor(channel, _UDI_GCB_TO_CHAN(gcb)->chan_region,
			    ops_idx, channel_context);
	/*
	 * Nothing to allocate in this implementation; call callback directly.
	 */
	_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_CHANNEL_ANCHOR,
			      &_udi_alloc_ops_nop, callback, 1, (channel));
	/*
	 * the other end initiated a channel_close before this end is
	 * anchored, so when this end is anchored a UDI_CHANNEL_CLOSED should
	 * be delivered to this end.  hence the op initiates from the other end
	 */
	if (chan->other_end->status == _UDI_CHANNEL_CLOSED) {
		_udi_channel_close((udi_channel_t)chan->other_end);
	}
}

/*
 * _udi_channel_create() creates a new channel and anchors it in the 
 * specified region using the specified channel ops index and channel
 * context.  The other end is loose, and is returned to the caller.
 * Note that this is a synchronous allocation routine which must be 
 * called from a blocking context.
 *
 * If anchor is not set, neither end will be anchored.
 */
_udi_channel_t *
_udi_channel_create(_udi_region_t *region,
		    udi_index_t ops_idx,
		    void *channel_context,
		    udi_boolean_t anchor)
{
	_udi_channel_t *new_channel;
	_udi_channel_t *other_end;

	new_channel = _OSDEP_MEM_ALLOC(2 * sizeof (_udi_channel_t),
				       UDI_MEM_NOZERO, UDI_WAITOK);

	other_end = new_channel + 1;

	/*
	 * Link the two ends together. 
	 */
	new_channel->other_end = other_end;
	other_end->other_end = new_channel;
	new_channel->bind_chan = new_channel->other_end->bind_chan = FALSE;
	/*
	 * Initialise the channel queue (used to keep track of anchored chans 
	 */
	UDI_QUEUE_INIT(&new_channel->chan_queue);
	UDI_QUEUE_INIT(&other_end->chan_queue);

	/*
	 * Set chan_spawns to NULL for the first end of this channel.
	 * The other end's chan_spawns is not used.
	 */
	new_channel->chan_spawns = NULL;

	/*
	 * intially both ends are loose 
	 */
	new_channel->status = _UDI_CHANNEL_LOOSE;
	new_channel->other_end->status = _UDI_CHANNEL_LOOSE;

	/*
	 * Anchor one end of the new channel in the region, if requested. 
	 */
	if (anchor) {
		_udi_channel_anchor(new_channel, region, ops_idx,
				    channel_context);
	}

	/*
	 * The other end is returned loose (unanchored); its remaining fields
	 * will get set when that end is anchored.
	 */

	return (other_end);
}

/*
 * _udi_create_bind_channel() creates a bind channel between two regions.  and 
 * anchors each end of the channel to the two regions.  returns the end of the
 * channel that was bound to initiator 
 */
_udi_channel_t *
_udi_bind_channel_create(_udi_region_t *region1,
			 udi_index_t ops_idx1,
			 _udi_region_t *region2,
			 udi_index_t ops_idx2)
{
	_udi_region_t *region[2];
	_udi_interface_t *chan_interface[2];
	udi_index_t ops_idx[2];
	_udi_channel_t *bind_channel, *initiator_chan = NULL;
	udi_child_chan_context_t *child_chan_context;
	void *channel_context;
	udi_ubit8_t i, relationship;


	/*
	 * this function should not be called for mangement channels 
	 */
	_OSDEP_ASSERT(ops_idx1 != 0 && ops_idx2 != 0);
	/*
	 * fill arguments into the array 
	 */
	region[0] = region1;
	region[1] = region2;
	ops_idx[0] = ops_idx1;
	ops_idx[1] = ops_idx2;

	bind_channel = _udi_channel_create(NULL, 0, NULL, FALSE);
	bind_channel->bind_chan = bind_channel->other_end->bind_chan = TRUE;
	/*
	 * take care of the chan_context_size alloc if specified and 
	 * appropriate 
	 */
	for (i = 0; i < 2; i++) {
		chan_interface[i] =
			region[i]->reg_driver->drv_interface[ops_idx[i]];

		/*
		 * default channel context 
		 */
		channel_context =
			&(region[i]->reg_init_rdata->ird_init_context);
		relationship =
			chan_interface[i]->if_ops_template->relationship;
		if (chan_interface[i]->chan_context_size > 0 &&
		    relationship & UDI_MEI_REL_BIND) {
			child_chan_context =
				_OSDEP_MEM_ALLOC(chan_interface[i]->
						 chan_context_size, 0,
						 UDI_WAITOK);
			if (relationship & UDI_MEI_REL_EXTERNAL &&
			    !(relationship & UDI_MEI_REL_INITIATOR)) {
				_OSDEP_ASSERT(chan_interface[i]->
					      chan_context_size >=
					      sizeof
					      (udi_child_chan_context_t));
				DEBUG3(3053, 1, ("Using child_chan_context"));
				/*
				 * this is the parent, so get child_id from other region 
				 */
				child_chan_context->child_ID =
					region[i ? 0 : 1]->reg_node->
					my_child_id;
				child_chan_context->rdata = channel_context;
			} else {
				_OSDEP_ASSERT(chan_interface[i]->
					      chan_context_size >=
					      sizeof (udi_chan_context_t));
				DEBUG3(3053, 1, ("Using chan_context"));
				child_chan_context->rdata = channel_context;
			}
			channel_context = child_chan_context;
		}
		if (relationship & UDI_MEI_REL_INITIATOR) {
			initiator_chan = bind_channel;
		}

		/*
		 * anchor this end of the channel 
		 */
		_udi_channel_anchor(bind_channel, region[i],
				    ops_idx[i], channel_context);
		bind_channel = bind_channel->other_end;
	}
	return initiator_chan;
}


/*
 * _udi_region_create() creates a new region. One end of the
 * channel is anchored in the new region, using its first_interface and
 * init context. The other end is loose, and is returned to the caller.
 * Note that this is a synchronous allocation routine which must be called
 * from a blocking context.
 */
_udi_region_t *
_udi_region_create(_udi_driver_t *driver,
		   _udi_dev_node_t *node,
		   _udi_rprop_t *rgn_prop)
{
	_udi_region_t *new_region;
	_udi_init_rdata_t *rdata;
	udi_init_context_t *rgn_data_area;
	int ii;

	new_region = _OSDEP_MEM_ALLOC(sizeof (_udi_region_t), 0, UDI_WAITOK);
	rdata = _OSDEP_MEM_ALLOC(rgn_prop->rdata_size +
				 sizeof (_udi_region_t *), 0, UDI_WAITOK);

	rdata->ird_region = new_region;
	REGION_INIT(new_region, driver);

#ifdef _UDI_PHYSIO_SUPPORTED
	UDI_QUEUE_INIT(&new_region->dma_mem_handle_q);
#endif
	new_region->reg_driver = driver;
	new_region->reg_node = node;
	new_region->reg_init_rdata = rdata;
	new_region->reg_prop = rgn_prop;
	if (node != NULL)
		UDI_ENQUEUE_TAIL(&node->region_list, &new_region->reg_link);

	rgn_data_area = &new_region->reg_init_rdata->ird_init_context;

	rgn_data_area->region_idx = rgn_prop->region_idx;

	/*
	 * Selectively cache high-running region properties right into the
	 * region structure for fast access.
	 */
	new_region->reg_is_interrupt = (rgn_prop->reg_attr &
					_UDI_RA_TYPE_MASK) ==
		_UDI_RA_INTERRUPT;

	/*
	 * Initialize the init_context at the front of the region data area 
	 */
	_OSDEP_GET_LIMITS_T(&rgn_data_area->limits);

	/*
	 * Ensure that the OSDEP code doesn't violate the spec guarantees. 
	 */
	_OSDEP_ASSERT(rgn_data_area->limits.max_legal_alloc >=
		      rgn_data_area->limits.max_safe_alloc);
	_OSDEP_ASSERT(rgn_data_area->limits.max_safe_alloc >=
		      UDI_MIN_ALLOC_LIMIT);
	_OSDEP_ASSERT(rgn_data_area->limits.max_trace_log_formatted_len >=
		      UDI_MIN_TRACE_LOG_LIMIT);
	_OSDEP_ASSERT(rgn_data_area->limits.max_instance_attr_len >=
		      UDI_MIN_INSTANCE_ATTR_LIMIT);

	new_region->reg_tracemask = _udi_trlog_initial;
	for (ii = 0; ii < 256; ii++)
		new_region->reg_ml_tracemask[ii] = _udi_trlog_ml_initial[ii];

	/*
	 * Initialize our list of hash buckets now.
	 */
	for (ii = 0; ii < RGN_HASH_SIZE; ii++) {
		UDI_QUEUE_INIT(&new_region->reg_alloc_hash[ii]);
	}

	return (new_region);
}

#if TRACKER
/*
 * when destroying a region, environments that do per-region resource
 * tracking can use this function to reclaim any resources that are left
 * over but "owned" by that region.
 */
STATIC void
_udi_reap_region_resources(_udi_region_t *region)
{
	int i;
	udi_queue_t *elem, *tmp;

	for (i = 0; i < RGN_HASH_SIZE; i++) {
		UDI_QUEUE_FOREACH(&region->reg_alloc_hash[i], elem, tmp) {
			_udi_alloc_hdr_t *ah;

			if (elem == NULL) {
				continue;
			}
			ah = UDI_BASE_STRUCT(elem, struct _udi_alloc_hdr,
					     ah_link);

			UDI_QUEUE_REMOVE(&ah->ah_link);
			_udi_dealloc_orphan(ah, region->reg_driver->drv_name);
		}
	}
}
#endif /* TRACKER */

/*
 * This helper function is called when we really, really are ready for
 * the region to be nuked.   Elsewhere it is unhooked from everything, but
 * since the region q might be holding a final_cleanup_ack (which may incur
 * locks in the region itself) we don't actually pull the trigger until
 * we get here.   This is called right after most occurrences of RUN_REGION_Q.
 */
void
_udi_do_region_destroy(_udi_region_t *region)
{
	/*
	 * Deallocate region data area. 
	 */
	_OSDEP_MEM_FREE(region->reg_init_rdata);

	/*
	 * De-initialize region. 
	 */
	REGION_DEINIT(region);

	/*
	 * Deallocate region. 
	 */
	_OSDEP_MEM_FREE(region);
}


/*
 * This function should not be confused with the future function region_kill.
 * region_destroy takes care of channels differently; it does
 * not send UDI_CHANNEL_CLOSED.  region_kill should send 
 * UDI_CHANNEL_CLOSED to the child region over that channel.
 * Additionally, the way to clear the execution queue may need to be 
 * different.
 */

void
_udi_region_destroy(_udi_reg_destroy_context_t *context)
{

	_udi_region_t *region = context->region;
	udi_queue_t *head, *elem, *tmp;
	_udi_channel_t *chan;
	udi_boolean_t defer = FALSE;

	/*
	 * Prevent any other cb's from being queued onto this 
	 * region while we're blowing the region away.
	 */
	REGION_LOCK(region);

	if (region->reg_tearing_down) {
		REGION_UNLOCK(region);
		return;
	}

	/*
	 * The specification says that once the final_cleanup_ack has been
	 * delivered to us by the driver that the driver shall have no 
	 * further channel activity.   The driver is responsible for not
	 * delivering this while there are any outstanding acks.   Enforce
	 * that now.
	 */
	_OSDEP_ASSERT(UDI_QUEUE_EMPTY(&region->reg_queue));

	/*
	 * This is to handle the case where a final_cleanup_req (which will
	 * probably trigger a final_cleanup_ack to the MA which knows to 
	 * destroy the region) is queued onto the region.  We simply set a 
	 * flag so that we don't deallocate the current region -which may
	 * be holding a rgn lock- in this function.   The caller of this fn
	 * knows to look at the "self_destruct" flag and call _udi_do_region
	 * destroy() if necessary.
	 *
	 */
	if (region->reg_active) {
		region->reg_self_destruct = 1;
		defer = TRUE;
	}

	/*
	 * Flag the region as being torn down.  Mark it as busy so
	 * any other callers trying to grab the region lock will 
	 * queue the work onto this region.   Since we (unlike 
	 * normal region runners) will destroy the region we just
	 * ignore the queued work before exiting.   This prevents
	 * us from having to hold the region lock (a spin lock) during
	 * potentially expensive operations.
	 */
	region->reg_tearing_down = TRUE;

	region->reg_active = TRUE;

	UDI_QUEUE_REMOVE(&region->reg_link);
	REGION_UNLOCK(region);

	/*
	 * We are now unlocked, but marked busy.  Ordinarily, this would
	 * be a horrible thing.  Since we're nuking the region, we just
	 * collect any work queued for us and take it out with the garbage.
	 * NOTE: We _CANNOT_ make any asynchronous calls to this region as it
	 * is disappearing even as we look at it. Hence, we have no debugging
	 * in this routine
	 */

	/*
	 * Destroy the region's MA allocated channels.
	 * don't send UDI_CHANNEL_CLOSE because UDI_DEVMGMT_UNBIND has taken
	 * care of all the resource cleanup from the driver side 
	 */
	head = &region->reg_chan_queue;
	UDI_QUEUE_FOREACH(head, elem, tmp) {
		chan = UDI_BASE_STRUCT(elem, _udi_channel_t,
				       chan_queue);

		if (chan->bind_chan) {
			_udi_channel_detach(chan);
			_udi_channel_destroy(chan);
		}
	}

	if (!defer) {
		_udi_do_region_destroy(region);
	}

	(*context->callback) (context->arg);
}

/*
 * detach a channel from a region. basically the reverse of channel anchor
 */
STATIC void
_udi_channel_detach(_udi_channel_t *chan)
{

	chan->status = _UDI_CHANNEL_CLOSED;
	UDI_QUEUE_REMOVE(&chan->chan_queue);
	if (chan->chan_interface->chan_context_size > 0)
		_OSDEP_MEM_FREE(chan->chan_context);
	chan->chan_context = NULL;
	chan->chan_region = NULL;
	chan->chan_interface = NULL;
}

/*
 * XXX - check to make sure we've already closed the channel (here or before
 * we get here).  Also, any chan_spawns in progress should be cleaned up via
 * channel closes, which must be done prior to destroying the channel.
 */
void
_udi_channel_destroy(_udi_channel_t *chan)
{
	_udi_spawn_t *sp, *sp_next;


	/*
	 * if other region is still anchored, do not free resource yet. 
	 * resources will be freed when the other end is closed
	 */
	if (chan->other_end->status == _UDI_CHANNEL_ANCHORED) {
		return;
	}
	/*
	 * spawn list only exist in the first channel 
	 */
	sp = CHAN_FIRST_END(chan)->chan_spawns;
	while (sp != NULL) {
		sp_next = sp->spawn_next;
		_OSDEP_MEM_FREE(sp);
		sp = sp_next;
	}
	/*
	 * Deallocate both ends of the channel (in one block). 
	 */
	_OSDEP_MEM_FREE(CHAN_FIRST_END(chan));
}

/*
 * _udi_spawn_rendezvous attempts to rendezvous with a udi_channel_spawn
 * on the other end of a channel.  If the new channel has been spawned on 
 * the other end, the new channel is returned; otherwise NULL is returned.
 *
 * When NULL is returned, the "spawn" lock for the source channel is still
 * held, and must be released by calling _udi_spawn_unlock().
 */
STATIC _udi_channel_t *
_udi_spawn_rendezvous(_udi_channel_t *channel,	/* spawn (source) channel */

		      udi_index_t spawn_idx)
{
	_udi_channel_t *first_end = CHAN_FIRST_END(channel);
	_udi_spawn_t *spawn_info;
	_udi_spawn_t **prev_spawn;
	_udi_channel_t *new_channel;

	/*
	 * See if this spawn_idx is pending on the channel already.
	 * Since either end could get here first, we need to make sure
	 * we use the same lock and spawn list in either case. To do this
	 * we make use of the concept of the "first" end of the channel.
	 */
	REGION_LOCK(first_end->chan_region);
	spawn_info = first_end->chan_spawns;
	prev_spawn = &first_end->chan_spawns;
	for (; spawn_info != NULL; spawn_info = spawn_info->spawn_next) {
		if (spawn_info->spawn_idx == spawn_idx)
			break;
		prev_spawn = &spawn_info->spawn_next;
	}

	if (spawn_info != NULL) {
		/*
		 * We found this spawn_idx already pending on the channel.
		 * It must have been created by the other end, since this end
		 * hasn't put anything on the list yet.
		 *
		 * Remove spawn_info from the spawn list now that we've
		 * rendezvous'd with the other end.
		 */
		*prev_spawn = spawn_info->spawn_next;
		REGION_UNLOCK(first_end->chan_region);

		/*
		 * Get the new channel out of spawn_info. 
		 */
		new_channel = spawn_info->spawn_channel->other_end;

		_OSDEP_MEM_FREE(spawn_info);
	} else
		new_channel = NULL;

	return (new_channel);
}

STATIC void
_udi_spawn_unlock(_udi_channel_t *channel)
{
	channel = CHAN_FIRST_END(channel);
	REGION_UNLOCK(channel->chan_region);
}

void
_udi_channel_spawn_complete(_udi_alloc_marshal_t *allocm,
			    void *new_mem)
{
	_udi_channel_t *channel = allocm->_u.channel_spawn_request.channel;
	_udi_region_t *region = channel->chan_region;
	udi_index_t spawn_idx = allocm->_u.channel_spawn_request.spawn_idx;
	udi_index_t ops_idx = allocm->_u.channel_spawn_request.ops_idx;
	void *chan_context = allocm->_u.channel_spawn_request.channel_context;
	_udi_spawn_t *spawn_info = new_mem;
	_udi_channel_t *new_channel, *spawned_channel;

	/*
	 * The first allocation got us a spawn_info struct;
	 * now allocate a new channel, anchoring one end accordingly.
	 */
	new_channel = _udi_channel_create(region,
					  ops_idx,
					  chan_context,
					  (ops_idx != 0))->other_end;
	/*
	 * Hang this channel off of the spawn_info and prepare to add it
	 * to the list. Before we can add it, we need to atomically check
	 * if the other end got there first and already put one on the list.
	 */
	spawn_info->spawn_idx = spawn_idx;
	spawn_info->spawn_channel = new_channel;

	spawned_channel = _udi_spawn_rendezvous(channel, spawn_idx);
	if (spawned_channel == NULL) {
		/*
		 * We're first, so add ours to the list. 
		 */
		spawn_info->spawn_next = CHAN_FIRST_END(channel)->chan_spawns;
		CHAN_FIRST_END(channel)->chan_spawns = spawn_info;
		_udi_spawn_unlock(channel);
		spawned_channel = new_channel;
		new_channel->other_end->status = _UDI_CHANNEL_SPAWNING;
	} else {
		/*
		 * Discard our allocations and get the info from other end. 
		 */
		_udi_channel_detach(new_channel);
		_udi_channel_destroy(new_channel);
		_OSDEP_MEM_FREE(spawn_info);
		if (ops_idx != 0) {
			_udi_channel_anchor(spawned_channel,
					    region, ops_idx, chan_context);
		}
	}
	SET_RESULT_UDI_CT_CHANNEL_SPAWN(allocm,
					(udi_channel_t)spawned_channel);
}

void
_udi_channel_spawn_discard(_udi_alloc_marshal_t *allocm)
{
	/*
	 * FIXME: this isn't right 
	 */
	UDIENV_CHECK(FIXME_THIS_IS_NOT_RIGHT, 0);
	_udi_channel_destroy(allocm->_u.channel_result.new_channel);
}

STATIC _udi_alloc_ops_t _udi_channel_spawn_alloc_ops = {
	_udi_channel_spawn_complete,
	_udi_channel_spawn_discard,
	_udi_alloc_no_incoming,
	_UDI_ALLOC_COMPLETE_MIGHT_BLOCK
};

void
udi_channel_spawn(udi_channel_spawn_call_t *callback,
		  udi_cb_t *gcb,
		  udi_channel_t _channel,
		  udi_index_t spawn_idx,
		  udi_index_t ops_idx,
		  void *channel_context)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;

	_udi_channel_t *channel = _channel;
	_udi_channel_t *new_channel;

	new_channel = _udi_spawn_rendezvous(channel, spawn_idx);
	if (new_channel != NULL) {
		if (ops_idx != 0) {
			_udi_channel_anchor(new_channel,
					    _UDI_GCB_TO_CHAN(gcb)->chan_region,
					    ops_idx, channel_context);
		}

		/*
		 * Return the newly spawned channel 
		 */
		_OSDEP_IMMED_CALLBACK(cb, _UDI_CT_CHANNEL_SPAWN,
				      &_udi_channel_spawn_alloc_ops,
				      callback, 1, (new_channel));
	} else {
		_udi_spawn_unlock(channel);

		/*
		 * We have multiple allocations to do.  To make it simple just
		 * marshal up the arguments and do the allocations in the daemon.
		 */
		allocm->_u.channel_spawn_request.channel = channel;
		allocm->_u.channel_spawn_request.spawn_idx = spawn_idx;
		allocm->_u.channel_spawn_request.ops_idx = ops_idx;
		allocm->_u.channel_spawn_request.channel_context =
			channel_context;

		_UDI_ALLOC_QUEUE(cb, _UDI_CT_CHANNEL_SPAWN, callback,
				 sizeof (_udi_spawn_t),
				 &_udi_channel_spawn_alloc_ops);
	}
}

void
udi_channel_op_abort(udi_channel_t target_channel,
		     udi_cb_t *orig_cb)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(orig_cb);
	udi_channel_event_cb_t *ev_cb = &_udi_op_abort_cb->v.ce;

	/*
	 * in case the cb has been realloced 
	 */
	cb = cb->new_cb;

	if (!(cb->cb_flags & _UDI_CB_ABORTABLE)) {
		/*
		 * if cb is not abortable do nothing.  Either operation 
		 * completed or operation is not abortable
		 */
		return;
	}
	cb->abort_chan = (_udi_channel_t *)target_channel;
	_OSDEP_SIMPLE_MUTEX_LOCK(&_udi_op_abort_cb_mutex);
	if (!UDI_QUEUE_EMPTY(&_udi_op_abort_cb->cb_queue)) {
		UDI_ENQUEUE_TAIL(&_udi_op_abort_cb->cb_queue, &cb->abort_link);
		_OSDEP_SIMPLE_MUTEX_UNLOCK(&_udi_op_abort_cb_mutex);
		return;
	}
	UDI_ENQUEUE_TAIL(&_udi_op_abort_cb->cb_queue, &cb->abort_link);
	_OSDEP_SIMPLE_MUTEX_UNLOCK(&_udi_op_abort_cb_mutex);
	UDI_GCB(ev_cb)->channel = target_channel;
	ev_cb->event = UDI_CHANNEL_OP_ABORTED;
	ev_cb->params.orig_cb = _UDI_HEADER_TO_CB(cb);
	udi_channel_event_ind(ev_cb);
}

void
udi_channel_close(udi_channel_t channel)
{
	_udi_channel_t *chan = channel;

	if (channel == UDI_NULL_CHANNEL)
		return;
	UDIENV_CHECK(channel_already_closed,
		     chan->status != _UDI_CHANNEL_CLOSED);
	UDIENV_CHECK(driver_should_not_close_bind_channels, !chan->bind_chan);
	chan->status = _UDI_CHANNEL_CLOSED;
	if (chan->other_end->status == _UDI_CHANNEL_ANCHORED) {
		_udi_channel_close(channel);
	} else if (chan->other_end->status != _UDI_CHANNEL_LOOSE) {
		_udi_channel_detach(chan);
		_udi_channel_detach(chan->other_end);
		_udi_channel_destroy(chan);
	}
}

/*
 * ------------------------------------------------------------------
 * Callback queuing and executing
 */
void
_udi_queue_callback_l(_udi_cb_t *cb)
{
	_udi_region_t *rgn = _UDI_CB_TO_CHAN(cb)->chan_region;

	/*
	 * If this isn't a callback, it must be a channel op.
	 * If it's a channel op, we shouldn't be here.
	 */
	_OSDEP_ASSERT(cb->cb_flags & _UDI_CB_CALLBACK);

	/*
	 * Queue the callback to the region in a control block. 
	 */
	_UDI_REGION_Q_APPEND(rgn, &cb->cb_qlink);
	/*
	 * Do the callback if the region isn't currently active 
	 */
	if (_udi_set_region_active(rgn)) {
		UDI_RUN_REGION_QUEUE(rgn);
	}
}

void
_udi_queue_callback(_udi_cb_t *cb)
{
	_udi_region_t *rgn = _UDI_CB_TO_CHAN(cb)->chan_region;

	REGION_LOCK(rgn);
	_udi_queue_callback_l(cb);
	REGION_UNLOCK(rgn);
}

STATIC void
_udi_do_callback(_udi_cb_t *cb)
{
	_udi_callback_func_t *callback = (_udi_callback_func_t *)cb->cb_func;
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	udi_cb_t *gcb = _UDI_HEADER_TO_CB(cb);

	allocm->alloc_state = _UDI_ALLOC_IDLE;

	cb->cb_flags &= ~_UDI_CB_CALLBACK;

	/*
	 * XXX - Depending on what the callers are doing, there
	 * may or may not be the need for a memory barrier here.
	 */

	/* *INDENT-OFF* */
	switch (cb->op_idx) {
	case _UDI_CT_CANCEL:
		(*(udi_cancel_call_t *) callback) (gcb);
		break;
	case _UDI_CT_CB_ALLOC:
		(*(udi_cb_alloc_call_t *) callback) (gcb,
				     allocm->_u.cb_alloc_result.new_cb);
		break;
	case _UDI_CT_MEM_ALLOC:
		(*(udi_mem_alloc_call_t *) callback) (gcb,
				      allocm->_u.mem_alloc_result.new_mem);
		break;

	case _UDI_CT_BUF_PATH_ALLOC:
		(*(udi_buf_path_alloc_call_t *) callback) (gcb,
			allocm->_u.buf_path_alloc_result.new_buf_path);
		break;

	case _UDI_CT_BUF_COPY:
	case _UDI_CT_BUF_WRITE:
	case _UDI_CT_BUF_TAG:
	case _UDI_CT_BUF_XINIT:
		/*
		 * SLIGHT HACK: All of them use same params so just pick one. 
		 */
		(*(udi_buf_copy_call_t *) callback) (gcb,
						     allocm->_u.buf_result.
						     new_buf);
		break;

	case _UDI_CT_INSTANCE_ATTR_GET:
		(*(udi_instance_attr_get_call_t *) callback) (gcb,
		      allocm->_u.instance_attr_get_result.attr_type,
		      allocm->_u.instance_attr_get_result.actual_length);
		break;
	case _UDI_CT_INSTANCE_ATTR_SET:
		(*(udi_instance_attr_set_call_t *) callback) (gcb,
			      allocm->_u.instance_attr_set_result.status);
		break;
	case _UDI_CT_CHANNEL_ANCHOR:
	case _UDI_CT_CHANNEL_SPAWN:
		/*
		 * SLIGHT HACK: These use the same params so just pick one. 
		 */
		(*(udi_channel_anchor_call_t *) callback) (gcb,
			        allocm->_u.channel_result.new_channel);
		break;

#ifdef _UDI_PHYSIO_SUPPORTED
	case _UDI_CT_CONSTRAINTS_ATTR_SET:
		(*(udi_dma_constraints_attr_set_call_t *) callback) (gcb,
			 allocm->_u.constraints_attr_set_result.new_constraints,
			 allocm->_u.constraints_attr_set_result.status);
		break;
	case _UDI_CT_DMA_PREPARE:
		(*(udi_dma_prepare_call_t *) callback) (gcb,
				allocm->_u.dma_prepare_result.new_dma_handle);
		break;
	case _UDI_CT_DMA_BUF_MAP:
		(*(udi_dma_buf_map_call_t *) callback) (gcb,
				allocm->_u.dma_buf_map_result.scgth,
				allocm->_u.dma_buf_map_result.complete,
				allocm->_u.dma_buf_map_result.status);
		break;
	case _UDI_CT_DMA_MEM_ALLOC:
		(*(udi_dma_mem_alloc_call_t *) callback) (gcb,
			  allocm->_u.dma_mem_alloc_result.new_dma_handle,
			  allocm->_u.dma_mem_alloc_result.mem_ptr,
			  allocm->_u.dma_mem_alloc_result.actual_gap,
			  allocm->_u.dma_mem_alloc_result.single_element,
			  allocm->_u.dma_mem_alloc_result.scgth,
			  allocm->_u.dma_mem_alloc_result.must_swap);
		break;
	case _UDI_CT_DMA_SYNC:
		(*(udi_dma_sync_call_t *) callback) (gcb);
		break;
	case _UDI_CT_DMA_SCGTH_SYNC:
		(*(udi_dma_scgth_sync_call_t *) callback) (gcb);
		break;
	case _UDI_CT_DMA_MEM_TO_BUF:
		(*(udi_dma_mem_to_buf_call_t *) callback) (gcb,
			   allocm->_u.dma_mem_to_buf_result.new_buf);
		break;

	case _UDI_CT_PIO_MAP:
		(*(udi_pio_map_call_t *)callback) (gcb,
			   allocm->_u.pio_map_result.new_handle);
		break;
	case _UDI_CT_PIO_TRANS:
		(*(udi_pio_trans_call_t *)callback) (gcb,
				     allocm->_u.pio_trans_result.new_buf,
				     allocm->_u.pio_trans_result.status,
				     allocm->_u.pio_trans_result.result);
		break;
#endif /* _UDI_PHYSIO_SUPPORTED */

	case _UDI_CT_TIMER_START:
		(*(udi_timer_expired_call_t *) callback) (gcb);
		break;

	case _UDI_CT_TIMER_START_REPEATING:
	{
		udi_ubit32_t nmissed;
		/*
		* Move the cb back onto the appropriate timeout list
		* (by calling _udi_timer_start) BEFORE calling the 
		* driver's callback.  
		*
		* At this point, before calling into the driver, we know
		* that the driver region is not currently "active" (we're
		* about to enter it and make it active).  And in order to
		* get here the control block was either on a timeout list
		* or the region queue, so we know that the control block
		* hasn't been cancelled yet (and can't be cancelled until
		* we reenter the region -- because only this region can
		* call udi_timer_cancel on this cb).  (Ie. if the driver
		* hasn't cancelled it by now it'll have to wait until
		* it gets invoked again).
		*
		* Therefore, we know that we can move the cb back to the
		* timeout list if we do it right here before calling the
		* driver's callback.  After calling the callback all bets
		* are off:  the driver may have called udi_cancel on the
		* cb or even freed it.
		*/
		nmissed = _udi_timer_start(
                                (udi_timer_expired_call_t *) cb->cb_func, cb, 
				allocm->_u.timer_request.ticks_left,
				_UDI_TIMER_RESTART_REPEATING);

		(*(udi_timer_tick_call_t *) callback) (gcb->context, nmissed);
		break;
        }
	case _UDI_CT_LOG_WRITE:
		(*(udi_log_write_call_t *) callback)
			(gcb, cb->cb_callargs.logm.logm_status);
		break;
 	case _UDI_CT_QUEUE_TO_REGION:
                (*(udi_queue_to_region_call_t *) callback)
                        (gcb, allocm->_u.generic_param.param);
                break;   
	default:
		_OSDEP_ASSERT(0);
		break;
	}
	/* *INDENT-ON* */
}

/*
 * ------------------------------------------------------------------
 * Run the next operation/callback in the region queue.
 *
 * The region lock must be held before calling this routine.
 */
void
_udi_run_next_on_region_queue(_udi_region_t *region)
{
	_udi_cb_t *cb;
	udi_cb_t *gcb;
	_udi_channel_t *channel;
	_udi_interface_t *ifp;
	udi_mei_op_template_t *op_template;
	udi_index_t op_idx;
	udi_boolean_t cb_was_pending;

	_OSDEP_ASSERT(!UDI_QUEUE_EMPTY(&region->reg_queue));
	_OSDEP_ASSERT(REGION_IS_ACTIVE(region));

	cb = (_udi_cb_t *) _UDI_REGION_Q_REMOVE_FIRST(region);

	REGION_UNLOCK(region);

	/*
	 * If this control block holds a callback, do it and return.
	 * Otherwise, it's a channel operation.
	 */
	if (cb->cb_flags & _UDI_CB_CALLBACK) {
		cb_was_pending = cb->cb_flags & _UDI_CB_ALLOC_PENDING;
		cb->cb_flags &= ~_UDI_CB_ALLOC_PENDING;
		_udi_do_callback(cb);
		return;
	}
	/*
	 * just free the memory, all info in cb_old is invalid 
	 */
	if (cb->old_cb != NULL && !(cb->cb_flags & _UDI_CB_ABORTABLE)) {
		_OSDEP_MEM_FREE(cb->old_cb);
		cb->old_cb = NULL;
	}

	op_idx = cb->op_idx;

	/*
	 * _OSDEP_DEBUG(run_next_on_region_queue); 
	 */

	gcb = _UDI_HEADER_TO_CB(cb);
	channel = gcb->channel;
	ifp = channel->chan_interface;
	gcb->context = channel->chan_context;

	op_template = &ifp->if_op_template_list[op_idx];

	/*
	 * We are preparing to leave this region and do a channel op to 
	 * to a new region.  If we are now an interrupt region, release 
	 * the IPL.
	 */
	if (region->reg_is_interrupt) {
		_OSDEP_UNMASK_INTR();
	}
	(*op_template->backend_stub) (ifp->if_ops[op_idx], gcb, gcb->scratch);
}


/*
 * Sometimes it's useful to force a callback into a region even if that
 * region is potentially not active.   Of course, someone eventually has 
 * to make the region active and run it.   That would be us.
 */
void
_udi_run_inactive_region(_udi_region_t *region)
{
	/*
	 * Potentially, the region _is_ busy.   The caller cannot (and should
	 * not have to) know this.  If so, we return quickly
	 * and rely on the current region execution to drain its queue 
	 * before returning.  Otherwise, we run the current region queue.
	 */
	REGION_LOCK(region);
	if (_udi_set_region_active(region)) {
		UDI_RUN_REGION_QUEUE(region);
	}
	if (region->reg_self_destruct) {
		REGION_UNLOCK(region);
		_udi_do_region_destroy(region);
	} else {
		REGION_UNLOCK(region);
	}

}

udi_boolean_t
_udi_set_region_active(_udi_region_t *region)
{
	if (REGION_IS_ACTIVE(region))
		return FALSE;
	if (region->reg_is_interrupt)
		_OSDEP_MASK_INTR();
	(region)->reg_active = TRUE;
	return TRUE;
}
