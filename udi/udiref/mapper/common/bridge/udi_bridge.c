
/*
 * File:  udi_bridge.c
 *
 * Common UDI Bridge Metalanguage Mapper.
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
#include "udi_bridgeP.h"

/*
 * Flip this non-zero in your debugger or at compile-time if you want
 * to see the multitude of debugging.   You must also add -DBRIDGEMAP_DEBUG
 * to your udiprops.txt to get this compiled in.
 */
#if BRIDGEMAP_DEBUG
STATIC udi_boolean_t _udi_bridge_debug_level = 0;
#else
#define _udi_bridge_debug_level 0
#endif

#define _BRIDGE_DEBUG if (_udi_bridge_debug_level) udi_debug_printf

/*
 * This is the value to be used by the OSDEP enumeration code in
 * the event that no value is available for an attribute.
 */
#define BOGUS 0xFFFFFFFF

/*
 * This is the number of attributes we know about - and by the way,
 * the minimum amount we demand in doing an enumeration request.
 */
#define BRIDGE_MAPPER_ATTRIBUTE_COUNT 16

/* 
 * Function Prototypes
 */

/* Mgmt Operations */
STATIC udi_usage_ind_op_t bridge_mapper_usage_ind;
STATIC udi_enumerate_req_op_t bridge_mapper_enumerate_req;
STATIC udi_devmgmt_req_op_t bridge_mapper_devmgmt_req;
STATIC udi_final_cleanup_req_op_t bridge_mapper_final_cleanup_req;

/* Bus-Bridge Operations */
STATIC udi_channel_event_ind_op_t bus_bridge_channel_event_ind;
STATIC udi_bus_bind_req_op_t bus_bridge_bus_bind_req;
STATIC udi_bus_unbind_req_op_t bus_bridge_bus_unbind_req;
STATIC udi_intr_attach_req_op_t bus_bridge_intr_attach_req;
STATIC udi_intr_detach_req_op_t bus_bridge_intr_detach_req;

/* Interrupt Dispatcher Operations */
STATIC udi_channel_event_ind_op_t intr_dispatcher_channel_event_ind;
STATIC udi_intr_event_rdy_op_t intr_dispatcher_intr_event_rdy;

/* Callback functions */
STATIC void intr_attach_req_1(udi_cb_t *gcb,
			      void *new_mem);
STATIC void intr_attach_req_2(udi_cb_t *gcb,
			      udi_channel_t new_channel);
STATIC void intr_attach_req_3(udi_cb_t *gcb,
			      udi_cb_t *new_cb);
STATIC void _udi_bridge_mapper_isr_1(udi_cb_t *gcb,
				     udi_buf_t *buf,
				     udi_status_t status,
				     udi_ubit16_t intr_status);
STATIC udi_dma_constraints_attr_set_call_t bus_bridge_alloc_constraints;


/* Other Forward References */

STATIC void intr_detach_cleanup(void *_context);

STATIC _OSDEP_ISR_RETURN_T _udi_bridge_mapper_isr(void *_context);

udi_boolean_t _OSDEP_ENUMERATE_PCI_DEVICE(bridge_mapper_child_data_t *,
					  udi_enumerate_cb_t *,
					  udi_ubit8_t);

/*
 * MGMT META Ops Vectors
 */
STATIC udi_mgmt_ops_t bridge_mapper_mgmt_ops = {
	bridge_mapper_usage_ind,	/* usage_ind */
	bridge_mapper_enumerate_req,	/* enumerate_req */
	bridge_mapper_devmgmt_req,	/* devmgmt_req */
	bridge_mapper_final_cleanup_req	/* final_cleanup_req */
};

/*
 * BUS-BRIDGE META Ops Vectors
 */
STATIC udi_bus_bridge_ops_t bridge_mapper_bus_bridge_ops = {
	bus_bridge_channel_event_ind,
	bus_bridge_bus_bind_req,
	bus_bridge_bus_unbind_req,
	bus_bridge_intr_attach_req,
	bus_bridge_intr_detach_req
};

/*
 * INTR-DISPATCHER META Ops Vectors
 */
STATIC udi_intr_dispatcher_ops_t bridge_mapper_intr_dispatcher_ops = {
	intr_dispatcher_channel_event_ind,
	intr_dispatcher_intr_event_rdy
};

#define BUS_BRIDGE_OPS_IDX 1
#define INTR_DISPATCHER_OPS_IDX 2

#define INTR_ATTACH_CB_INDEX 1
#define INTR_EVENT_CB_INDEX 2

/*
 * ========================================================================
 * 
 * Bridge Mapper Initialization structure
 * 
 * ------------------------------------------------------------------------
 */

STATIC udi_ubit8_t bridge_mapper_default_op_flags[] = {
	0, 0, 0, 0, 0
};

/*
 * -----------------------------------------------------------------------------
 * Management operations init section:
 * -----------------------------------------------------------------------------
 */
STATIC udi_primary_init_t bridge_mapper_primary_init = {
	&bridge_mapper_mgmt_ops,
	bridge_mapper_default_op_flags,	/* mgmt op flags */
	sizeof (enumerate_cb_scratch_t),	/* mgmt_scratch_size */
	BRIDGE_MAPPER_ATTRIBUTE_COUNT,	/* enumeration attr list length */
	sizeof (bridge_mapper_region_data_t),
	sizeof (bridge_mapper_child_data_t),	/* Child data size */
	0,				/* buf path */
};

/*
 * -----------------------------------------------------------------------------
 * Meta operations init section:
 * -----------------------------------------------------------------------------
 */
STATIC udi_ops_init_t bridge_mapper_ops_init_list[] = {
	{
	 BUS_BRIDGE_OPS_IDX,
	 BRIDGE_MAPPER_BRIDGE_META,	/* meta index [from udiprops.txt] */
	 UDI_BUS_BRIDGE_OPS_NUM,
	 sizeof (bridge_mapper_child_context_t),	/* Child context size */
	 (udi_ops_vector_t *)&bridge_mapper_bus_bridge_ops,
	 bridge_mapper_default_op_flags,	/* op flags */
	 }
	,
	{
	 INTR_DISPATCHER_OPS_IDX,
	 BRIDGE_MAPPER_BRIDGE_META,	/* meta index [from udiprops.txt] */
	 UDI_BUS_INTR_DISPATCH_OPS_NUM,
	 0,				/* channel context size */
	 (udi_ops_vector_t *)&bridge_mapper_intr_dispatcher_ops,
	 bridge_mapper_default_op_flags,	/* op flags */
	 }
	,
	{
	 0				/* Terminator */
	 }
};

/*
 * -----------------------------------------------------------------------------
 * Control Block init section:
 * -----------------------------------------------------------------------------
 */
STATIC udi_cb_init_t bridge_mapper_cb_init_list[] = {
	{
	 INTR_ATTACH_CB_INDEX,		/* intr attach CB */
	 BRIDGE_MAPPER_BRIDGE_META,	/* from udiprops.txt    */
	 UDI_BUS_INTR_ATTACH_CB_NUM,	/* meta cb_num          */
	 sizeof (intr_attach_cb_scratch_t),	/* scratch requirement */
	 0,				/* inline size          */
	 NULL				/* inline layout        */
	 }
	,
	{
	 INTR_EVENT_CB_INDEX,		/* intr event CB */
	 BRIDGE_MAPPER_BRIDGE_META,	/* from udiprops.txt    */
	 UDI_BUS_INTR_EVENT_CB_NUM,	/* meta cb_num          */
	 sizeof (intr_event_cb_scratch_t),	/* scratch requirement */
	 0,				/* inline size          */
	 NULL				/* inline layout        */
	 }
	,
	{
	 0				/* Terminator */
	 }
};

udi_init_t udi_init_info = {
	&bridge_mapper_primary_init,
	NULL,				/* Secondary init list */
	bridge_mapper_ops_init_list,
	bridge_mapper_cb_init_list,
	NULL,				/* gcb init list */
	NULL,				/* cb select list */
};


/* TODO -
 * This routine can go away once the channel context is set correctly to
 * child data during bind operation
 */
bridge_mapper_child_data_t *
bridge_mapper_find_child_data(bridge_mapper_region_data_t * rdata,
			      udi_ubit32_t child_id)
{
	bridge_mapper_child_data_t *child_data;
	udi_queue_t *elem, *tmp;

	UDI_QUEUE_FOREACH(&rdata->child_list, elem, tmp) {
		child_data = UDI_BASE_STRUCT(elem,
					     bridge_mapper_child_data_t, link);
		_BRIDGE_DEBUG
			("find_child_data:child_data = %p, child_ID = %d, "
			 "child_data->child_ID = %d\n", child_data, child_id,
			 child_data->child_ID);
		if (child_data->child_ID == child_id)
			return child_data;
	}
	return (bridge_mapper_child_data_t *) NULL;
}

/*
 * This is supposed to be the first call ...
 */

STATIC void
bridge_mapper_usage_ind(udi_usage_cb_t *usage_cb,
			udi_ubit8_t resource_level)
{
	bridge_mapper_region_data_t *rdata = UDI_GCB(usage_cb)->context;

	_BRIDGE_DEBUG("bridge_mapper_usage_ind\n");

	UDI_QUEUE_INIT(&rdata->child_list);
	rdata->r_child_id = 0;

	/*
	 * Tracing not supported 
	 */
	usage_cb->trace_mask = 0;
	udi_usage_res(usage_cb);
}

/*
 * Quarrantine the PCI-specific enumeration attributes into a
 * function of their own.   It's still uncomfortable that we are
 * this cozy with the "portable" part of the bridge...
 */
STATIC int
bridge_mapper_create_pci_enum_attrs(int n,
	udi_instance_attr_list_t *attr_list,
	bridge_mapper_child_data_t *child_data
	)
{
	char identifier_buf[19], address_locator_buf[6],
		physical_locator_buf[3];
	BRIDGE_SET_ATTR_STRING(&attr_list[n], "bus_type", "pci");
	n++;

	if (child_data->pci_vendor_id != BOGUS) {
		BRIDGE_SET_ATTR_UBIT32(&attr_list[n], "pci_vendor_id",
				       child_data->pci_vendor_id);
		n++;
	}

	if (child_data->pci_device_id != BOGUS) {
		BRIDGE_SET_ATTR_UBIT32(&attr_list[n], "pci_device_id",
				       child_data->pci_device_id);
		n++;
	}

	if (child_data->pci_revision_id != BOGUS) {
		BRIDGE_SET_ATTR_UBIT32(&attr_list[n], "pci_revision_id",
				       child_data->pci_revision_id);
		n++;
	}

	if (child_data->pci_base_class != BOGUS) {
		BRIDGE_SET_ATTR_UBIT32(&attr_list[n], "pci_base_class",
				       child_data->pci_base_class);
		n++;
	}

	if (child_data->pci_sub_class != BOGUS) {
		BRIDGE_SET_ATTR_UBIT32(&attr_list[n], "pci_sub_class",
				       child_data->pci_sub_class);
		n++;
	}

	if (child_data->pci_prog_if != BOGUS) {
		BRIDGE_SET_ATTR_UBIT32(&attr_list[n], "pci_prog_if",
				       child_data->pci_prog_if);
		n++;
	}

	if (child_data->pci_subsystem_vendor_id != BOGUS) {
		BRIDGE_SET_ATTR_UBIT32(&attr_list[n],
				       "pci_subsystem_vendor_id",
				       child_data->pci_subsystem_vendor_id);
		n++;
	}

	if (child_data->pci_subsystem_id != BOGUS) {
		BRIDGE_SET_ATTR_UBIT32(&attr_list[n], "pci_subsystem_id",
				       child_data->pci_subsystem_id);
		n++;
	}

	/*
	 * Construct the identifier attribute 
	 */
	udi_snprintf(identifier_buf, sizeof (identifier_buf),
		     "%04X%04X%02X%04X%04X",
		     child_data->pci_vendor_id,
		     child_data->pci_device_id,
		     child_data->pci_revision_id,
		     child_data->pci_subsystem_vendor_id,
		     child_data->pci_subsystem_id);
	BRIDGE_SET_ATTR_STRING(&attr_list[n], "identifier", identifier_buf);
	n++;

	if (child_data->pci_unit_address != BOGUS) {
		BRIDGE_SET_ATTR_UBIT32(&attr_list[n], "pci_unit_address",
				       child_data->pci_unit_address);
		n++;

		/*
		 * construct the address locator attribute 
		 */
		udi_snprintf(address_locator_buf, sizeof (address_locator_buf),
			     "%02X%02X%X",
			     (child_data->pci_unit_address >> 8) & 0xFF,
			     (child_data->pci_unit_address >> 3) & 0x1F,
			     (child_data->pci_unit_address) & 0x07);
		BRIDGE_SET_ATTR_STRING(&attr_list[n], "address_locator",
				       address_locator_buf);
		n++;
	}

	if (child_data->pci_slot != BOGUS) {
		BRIDGE_SET_ATTR_UBIT32(&attr_list[n], "pci_slot",
				       child_data->pci_slot);
		n++;

		/*
		 * construct the physical_locator attribute 
		 */
		udi_snprintf(physical_locator_buf,
			     sizeof (physical_locator_buf),
			     "%02X", child_data->pci_slot & 0xFF);
		BRIDGE_SET_ATTR_STRING(&attr_list[n], "physical_locator",
				       physical_locator_buf);
		n++;
	}
	return n;
}
/*
 *	MA calls this to find the set of devices....
 *	Enumerate PCI bus
 *	We are called repeatedly until flag set to say it is last device..
 *	Enumeration result is usually ok or done.
 */

STATIC void
bridge_mapper_enumerate_req(udi_enumerate_cb_t *cb,
			    udi_ubit8_t enumeration_level)
{
	bridge_mapper_region_data_t *rdata = cb->gcb.context;
	bridge_mapper_child_data_t *child_data;
	udi_instance_attr_list_t *attr_list;
	udi_index_t n;

/* 
 *	The child data is preallocated for us.  
 *
 *	Pass this memory to the OSDEP routine to fill out the
 *	OSINFO section of the device context.  This and the enumeration 
 *	level should be the arguments.
 *	If there is no device found, then free the memory and return a 
 *	NULL context and zero attr_valid_length.
 *
 *	Otherwise, fill in the attribute list, and
 *	return the device to the management agent...
 */

	_BRIDGE_DEBUG("bridge_mapper_enumerate_req enumeration_level = %d\n",
		      enumeration_level);
	_BRIDGE_DEBUG("bridge_mapper_enumerate_req child_data = %p\n",
		      cb->child_data);
	/*
	 * just look for valid input 
	 */

	switch (enumeration_level) {
	case UDI_ENUMERATE_START:
	case UDI_ENUMERATE_START_RESCAN:

		/*
		 * start from beginning 
		 */
		break;
	case UDI_ENUMERATE_NEXT:

		/*
		 * get next             
		 */

		break;
	case UDI_ENUMERATE_NEW:
		/*
		 * Hot plug devices.  just fail for now. 
		 */
		udi_enumerate_ack(cb, UDI_ENUMERATE_FAILED, 0);
		return;
	case UDI_ENUMERATE_DIRECTED:
		/*
		 * In this call, the MA wants us to make believe
		 * that we found a specific device.  To do this
		 * we will go ahead and create a device node struct.
		 */

#if 0

		/*
		 * Respond FAILED for now.      
		 */

		udi_enumerate_ack(cb, UDI_ENUMERATE_FAILED, NULL, 0, 0);

		return;
#else
		break;
#endif

	case UDI_ENUMERATE_RELEASE:
		/*
		 * OK to free the child context here - there will be
		 * no more accesses to it from the MA.
		 */
		child_data = bridge_mapper_find_child_data(rdata,
							   cb->child_ID);

		_BRIDGE_DEBUG("bridge_mapper ENUMERATE_RELEASE -"
			      " rdata %p child_data %p [%u]\n",
			      rdata, child_data, cb->child_ID);

		/*
		 * TRY TO THINK OF STRUCTURES THAT NEED TO BE FREED  
		 */

		if (child_data) {
			UDI_QUEUE_REMOVE(&child_data->link);
			udi_mem_free(child_data);
		}

		udi_enumerate_ack(cb, UDI_ENUMERATE_RELEASED, 0);
		return;
	default:
		/*
		 * Unknown option assert for now. 
		 */
		_OSDEP_ASSERT(0);

	}

	child_data = cb->child_data;
	attr_list = cb->attr_list;

	for (n = 0; n < BRIDGE_MAX_INTERRUPT_ATTACHMENT_COUNT; n++)
		child_data->interrupt_attachments[n] = 0;

	/*
	 * We now have a context structure...  Ask OSDEP
	 * code to fill in osinfo (and other fields?) for us...
	 * We need the enumeration level to pass on to the osdep code.
	 */
	if (!_OSDEP_ENUMERATE_PCI_DEVICE(child_data, cb, enumeration_level)) {
		/*
		 * the last device has already been returned or an error
		 * occurred in the os-dependent portion.
		 */
		cb->attr_valid_length = 0;
		udi_enumerate_ack(cb,
				  (enumeration_level ==
				   UDI_ENUMERATE_DIRECTED ?
				   UDI_ENUMERATE_FAILED : UDI_ENUMERATE_DONE),
				  BUS_BRIDGE_OPS_IDX);

		return;
	}

	/*
	 * Now set the attributes ...
	 *
	 *      NOTE! - "pci_slot" is the only optional attribute
	 *
	 *      "address_locator" is a combination...
	 *
	 *      For now the convention for handling directed enumeration is
	 *           that the osdep handling cleared the attr_names that it
	 *           used.  Therefore any non-zero attr_name entries here
	 *           are attrs that the bridge is unaware of and should be
	 *           left untouched.  It makes walking the list a bit
	 *           awkward, but c'est la vie.
	 */
	n = 0;
	if (enumeration_level == UDI_ENUMERATE_DIRECTED) {
		while (n < cb->attr_valid_length &&
		       attr_list[n].attr_name[0] != 0)
			n++;
	}

	n = bridge_mapper_create_pci_enum_attrs(n, attr_list, child_data);

	cb->attr_valid_length = n;
#if !_OSDEP_BRIDGE_MAPPER_ASSIGN_CHILD_ID
	child_data->child_ID = cb->child_ID = rdata->r_child_id++;
#endif

	UDI_ENQUEUE_HEAD(&rdata->child_list, &child_data->link);

	udi_enumerate_ack(cb, UDI_ENUMERATE_OK, BUS_BRIDGE_OPS_IDX);
}

STATIC void
bridge_mapper_devmgmt_req(udi_mgmt_cb_t *cb,
			  udi_ubit8_t mgmt_op,
			  udi_ubit8_t parent_id)
{
	udi_ubit8_t flags = 0;
	udi_status_t status = UDI_OK;

	/*
	 * TODO: Do something useful for the bulk of the cases 
	 */
	switch (mgmt_op) {
	case UDI_DMGMT_PREPARE_TO_SUSPEND:
	case UDI_DMGMT_SUSPEND:
	case UDI_DMGMT_SHUTDOWN:
	case UDI_DMGMT_PARENT_SUSPENDED:
	case UDI_DMGMT_RESUME:
	case UDI_DMGMT_UNBIND:
		break;
	default:
		_OSDEP_ASSERT(0);	/* unknown value */
	}

	udi_devmgmt_ack(cb, flags, status);
}


STATIC void
bridge_mapper_final_cleanup_req(udi_mgmt_cb_t *cb)
{
	/*
	 *      We are going away.  If anything got allocated that is
	 *      not done on a per-child basis, it is time to free it.
	 */
	_OSDEP_BUS_BRIDGE_DEINIT(cb);

	/*
	 * - SO FAR, NOTHING TO DO HERE 
	 */

	udi_final_cleanup_ack(cb);
}

/*
 * ========================================================================
 * 
 * Bus Bridge Operations
 * 
 * ------------------------------------------------------------------------
 */
STATIC void
bus_bridge_channel_event_ind(udi_channel_event_cb_t *channel_event_cb)
{
	_BRIDGE_DEBUG("bus_bridge_channel_event_ind\n");

	switch (channel_event_cb->event) {

	case UDI_CHANNEL_CLOSED:
		_BRIDGE_DEBUG
			("bus_bridge_channel_event_ind - UDI_CHANNEL_CLOSED\n");
		/*
		 *      channel closed could be due to abort
		 *      - would need to check cleanups.
		 *      don't release child context, but
		 *      unbind and detach should be synthesized
		 *      if necessary. 
		 */
		udi_channel_close(channel_event_cb->gcb.channel);

/* STILL TODO - DO THE NECESSARY CLEANUPS */

		break;

	case UDI_CHANNEL_BOUND:
		_OSDEP_ASSERT(0);
		break;

		/*
		 * We shouldn't get any of these: 
		 */
	case UDI_CHANNEL_OP_ABORTED:
	default:
		_OSDEP_ASSERT(0);
	}

	udi_channel_event_complete(channel_event_cb, UDI_OK);
}

#ifndef _OSDEP_BUS_BRIDGE_ALLOC_CONSTRAINTS

udi_dma_constraints_attr_spec_t _osdep_bridge_attr_list[] = {
	{ UDI_DMA_ADDRESSABLE_BITS, 64 },
	{ UDI_DMA_SCGTH_MAX_ELEMENTS, 256 },
};

#endif

STATIC void
bus_bridge_bus_bind_req(udi_bus_bind_cb_t *bus_bind_cb)
{
	udi_ubit16_t list_length;

	(void)_OSDEP_BUS_BRIDGE_BIND_REQ(bus_bind_cb);

	list_length = sizeof(_osdep_bridge_attr_list) /
		sizeof(udi_dma_constraints_attr_spec_t);

	/*
	 * allocate a set of dma constraints for my child
	 */
	udi_dma_constraints_attr_set(bus_bridge_alloc_constraints,
				UDI_GCB(bus_bind_cb),
				UDI_NULL_DMA_CONSTRAINTS,
				_osdep_bridge_attr_list,
				list_length, 0);
}


STATIC void
bus_bridge_alloc_constraints(udi_cb_t *gcb,
			     udi_dma_constraints_t new_constraints,
			     udi_status_t status)
{

	bridge_mapper_child_context_t *child_context = gcb->context;
	udi_ubit8_t preferred_endianness = 0;

	if (status == UDI_OK) {

		child_context->child_data =
			bridge_mapper_find_child_data(child_context->
						      chan_context.rdata,
						      child_context->
						      chan_context.child_ID);

		_OSDEP_ASSERT(child_context->child_data);

		/*
		 * OS should be able to tell us the endianness based 
		 * on the osinfo that identifies the device.... 
		 * usually little-endian because PCI is little endian, 
		 * but we want to support things beyond PCI in time.
		 */

		preferred_endianness =
			_OSDEP_ENDIANNESS_QUERY(child_context->child_data->
						osinfo);

		/*
		 * osdep hook into bind_req
		 */
		_OSDEP_BUS_BRIDGE_BIND_DONE(UDI_MCB(gcb, udi_bus_bind_cb_t));
	} else {
		new_constraints = UDI_NULL_DMA_HANDLE;
	}

	/*
	 * Complete the binding
	 */
	udi_bus_bind_ack(UDI_MCB(gcb, udi_bus_bind_cb_t),
			 new_constraints,
			 preferred_endianness,
			 status);
}

STATIC void
bus_bridge_bus_unbind_req(udi_bus_bind_cb_t *bus_bind_cb)
{
	bridge_mapper_child_context_t *child_context =
		UDI_GCB(bus_bind_cb)->context;
	bridge_mapper_child_data_t *child_data = child_context->child_data;

	_BRIDGE_DEBUG("bus_bridge_bus_unbind_req - child_data %p\n",
		      child_data);

	/*
	 *      What happened?  We are getting unbound from the MA! 
	 *
	 *      Check if there are still interrupt attachments....
	 *
	 *      If so, we could close the channels and disconnect the driver.
	 *
	 */

	/*
	 * osdep hook into unbind_req
	 */
	_OSDEP_BUS_BRIDGE_UNBIND_DONE(bus_bind_cb);

	/*
	 * Not much useful to do here - basically a nop.
	 */
	udi_bus_unbind_ack(bus_bind_cb);
}


/*
 * bus_bridge_intr_attach_req - Request an interrupt attachment
 */
STATIC void
bus_bridge_intr_attach_req(udi_intr_attach_cb_t *cb)
{
	bridge_mapper_child_context_t *child_context = UDI_GCB(cb)->context;
	bridge_mapper_child_data_t *child_data = child_context->child_data;

	/*
	 * don't get confused, we need to allocate this yet 
	 */
	intr_attach_context_t *intr_attach_context;

	_BRIDGE_DEBUG("bus_bridge_intr_attach_req - child_data %p\n",
		      child_data);

	/*
	 * see if the attachment index exceeds what we thought
	 * was reasonable.
	 */
	if (cb->interrupt_idx >= BRIDGE_MAX_INTERRUPT_ATTACHMENT_COUNT) {
		/*
		 * just in case somebody thinks we can continue like this 
		 */
		_udi_env_log_write(_UDI_TREVENT_ENVERR, UDI_LOG_ERROR, 0,
				   UDI_STAT_RESOURCE_UNAVAIL, 100);
		udi_intr_attach_ack(cb, UDI_STAT_RESOURCE_UNAVAIL);
		return;
	}

	/*
	 * we could already have an attachment context for this guy.
	 * check the list of attaches for a duplicate index before allocating
	 * a new context.
	 */
	intr_attach_context =
		child_data->interrupt_attachments[cb->interrupt_idx];

	if (intr_attach_context != 0) {
		/*
		 * we already have an attachment.  Only the pio_handle 
		 * is allowed to change.  We can change to using 
		 * preprocessing, but not the other way around.
		 */
		if (!UDI_HANDLE_IS_NULL(cb->preprocessing_handle,
					udi_pio_handle_t)) {
			/*
			 * free the old one, per the spec 
			 */
			udi_pio_unmap(intr_attach_context->pio_handle);
			intr_attach_context->dispatcher_state = ds_disarmed;
			intr_attach_context->pio_handle =
				cb->preprocessing_handle;
		} else {
			/*
			 * no handle was provided.  We will reject the 
			 * attempt to change to non-preprocessed, or the
			 * duplicate registrations of a non-preprocessed
			 * handler for this interrupt index
			 */
			_udi_env_log_write(_UDI_TREVENT_ENVERR,
					   UDI_LOG_ERROR, 0,
					   UDI_STAT_INVALID_STATE, 101);

			intr_attach_context->dispatcher_state = ds_disarmed;
			udi_intr_attach_ack(cb, UDI_STAT_INVALID_STATE);
			return;
		}
	}

	/*
	 * Allocate a context area for this intr attachment.
	 */

	{
		/*
		 * THis used to be a udi_mem_alloc.    That would be wrong.
		 */
		void *new_mem;

		new_mem = _OSDEP_MEM_ALLOC(sizeof (intr_attach_context_t),
					   0, UDI_WAITOK);
		intr_attach_req_1(UDI_GCB(cb), new_mem);
	}
}

STATIC void
_udi_bus_bridge_validate_pio_handle(udi_pio_handle_t pio)
{
	_udi_pio_handle_t *ph = (_udi_pio_handle_t *)pio;

	_OSDEP_ASSERT((ph->trans_entry_pt[0] != NULL) &&
		      (ph->trans_entry_pt[1] != NULL) &&
		      (ph->trans_entry_pt[2] != NULL) &&
		      (ph->trans_entry_pt[3] != NULL));
	return;
}

/*
 *	We got the interrupt attach context...
 */
STATIC void
intr_attach_req_1(udi_cb_t *gcb,
		  void *new_mem)
{
	udi_intr_attach_cb_t *cb = UDI_MCB(gcb, udi_intr_attach_cb_t);
	intr_attach_context_t *intr_attach_context = new_mem;

	intr_attach_cb_scratch_t *scratch = UDI_GCB(cb)->scratch;
	bridge_mapper_child_context_t *child_context = UDI_GCB(cb)->context;
	bridge_mapper_child_data_t *child_data = child_context->child_data;

	_BRIDGE_DEBUG
		("intr_attach_req_1 - child_data %p intr_attach_context %p\n",
		 child_data, intr_attach_context);

	/*
	 * Initialize the context area for this intr attachment. 
	 */
	intr_attach_context->child_data = child_data;
	intr_attach_context->interrupt_idx = cb->interrupt_idx;
	intr_attach_context->min_event_pend = cb->min_event_pend;
	intr_attach_context->free_list = NULL;
	intr_attach_context->num_event_cbs_on_q = 0;
	intr_attach_context->dispatcher_state = ds_disarmed;

	/*
	 * CD1 says thou shalt have at least a minimum of two.
	 */
	_OSDEP_ASSERT(cb->min_event_pend >= 2);

	/*
	 * if we are to use preprocessing, then there will be a handle
	 * provided.
	 */
	if (UDI_HANDLE_IS_NULL(cb->preprocessing_handle, udi_pio_handle_t)) {
		intr_attach_context->dispatcher_state = ds_interrupt_region;
	} else {
		_udi_bus_bridge_validate_pio_handle(cb->preprocessing_handle);
		intr_attach_context->pio_handle = cb->preprocessing_handle;
	}

	_OSDEP_SIMPLE_MUTEX_INIT(&intr_attach_context->mutex,
				 "_udi_bridge_mapper_lock");
	/*
	 * Keep a pointer to the interrupt attach context we allocated.
	 */
	scratch->intr_attach_context = intr_attach_context;

	/*
	 * Now we want to spawn an intr_event  channel connection to the     
	 * caller.  This channel is necessary in order to       
	 * properly direct the interrupt event CBs              
	 */
	udi_channel_spawn(intr_attach_req_2, gcb,
			  gcb->channel, intr_attach_context->interrupt_idx,
			  INTR_DISPATCHER_OPS_IDX, intr_attach_context);
}

/*
 *	Interrupt event channel is created.
 */
STATIC void
intr_attach_req_2(udi_cb_t *gcb,
		  udi_channel_t new_channel)
{
	udi_intr_attach_cb_t *cb = UDI_MCB(gcb, udi_intr_attach_cb_t);
	intr_attach_cb_scratch_t *scratch = gcb->scratch;
	intr_attach_context_t *intr_attach_context =
		scratch->intr_attach_context;
	bridge_mapper_child_data_t *child_data =
		intr_attach_context->child_data;
	udi_status_t status;

	_BRIDGE_DEBUG("intr_attach_req_2 - child_data %p new_channel %p\n",
		      child_data, new_channel);

	intr_attach_context->intr_event_channel = new_channel;

	/*
	 *      Register interrupts with the operating system.
	 */
	status = _OSDEP_REGISTER_ISR(_udi_bridge_mapper_isr,
				     gcb,
				     intr_attach_context,
				     &(child_data->osinfo),
				     &(intr_attach_context->osinfo));

	/*
	 * Before doing the ack, set the context of all cb's on this 
	 * channel to point back to our intr_attach_context.  This is
	 * particularly useful on freshly allocated unbind cb's.
	 */
	udi_channel_set_context(cb->gcb.channel, intr_attach_context);

	if (status != UDI_OK) {
		_BRIDGE_DEBUG("OSDEP_REGISTER_ISR failed with: 0x%x\n",
			      status);
		_udi_env_log_write(_UDI_TREVENT_ENVERR, UDI_LOG_ERROR, 0,
				   UDI_STAT_RESOURCE_UNAVAIL, 102, status);
                /*
		 * Unclaim ownership of this handle.   This will result
		 * in the intr_detach doing a pio_unmap of a null handle
		 * (which will be ignored, as the spec says) and it is
		 * up to the calling driver to release this handle.
		 */
		intr_attach_context->pio_handle = UDI_NULL_PIO_HANDLE;

		/*
		 * Free resources associated with this attachment 
		 */
		intr_detach_cleanup(intr_attach_context);
	} else {
		/*
		 *      We want the interrupt attach context to be 
		 *      returned on all ops with this channel.
		 */
		udi_channel_set_context(intr_attach_context->
					intr_event_channel,
					intr_attach_context);
		/*
		 * Set it manually now becuase we'll need the context
		 * member of this cb set before we use it for a channel op.
		 */
		gcb->context = intr_attach_context;

		child_data->interrupt_attachments[cb->interrupt_idx] =
			intr_attach_context;
	}

	intr_attach_context->intr_attach_result = status;
	/* 
	 * The cb will never be used for a channel op so the channel 
	 * and cb_idx are inconsequential.
	 */
	udi_cb_alloc(intr_attach_req_3, gcb, 0, UDI_NULL_CHANNEL);
}

/*
 * Called upon successful allocation of the control block that will 
 * be used for interrupt overrunning condition.
 */
STATIC void
intr_attach_req_3(udi_cb_t *gcb,
		  udi_cb_t *new_cb)
{
	udi_intr_attach_cb_t *cb = UDI_MCB(gcb, udi_intr_attach_cb_t);
	intr_attach_context_t *intr_attach_context = gcb->context;

	intr_attach_context->orun_cb = new_cb;
	new_cb->initiator_context = intr_attach_context;
	udi_intr_attach_ack(cb, intr_attach_context->intr_attach_result);
}


/*
 * bus_bridge_intr_detach_req - Request an interrupt detachment
 *
 *	Steps to take:
 *	1.	Disconnect from OSDEP ISR
 *	2.	Channel close
 *	3.	Free Event CB's and Bufs
 *	4.	Free Interrupt context.
 *
 *	The last three steps are done by intr_detach_cleanup()
 */
STATIC void
bus_bridge_intr_detach_req(udi_intr_detach_cb_t *cb)
{
	intr_attach_context_t *intr_attach_context = UDI_GCB(cb)->context;
	bridge_mapper_child_data_t *child_data =
		intr_attach_context->child_data;
	udi_status_t status;

	_OSDEP_ASSERT(intr_attach_context ==
		      child_data->interrupt_attachments[cb->interrupt_idx]);

	_BRIDGE_DEBUG("bus_bridge_intr_detach_req. intr idx = %d\n",
		      cb->interrupt_idx);

	/*
	 * intr_attach_context->cb = cb; 
	 */
	status = _OSDEP_UNREGISTER_ISR(_udi_bridge_mapper_isr,
				       UDI_GCB(cb),
				       intr_attach_context,
				       &(child_data->osinfo),
				       &(intr_attach_context->osinfo));

	/*
	 * detach from our enumerated device region 
	 */
	child_data->interrupt_attachments[cb->interrupt_idx] = 0;

	/*
	 * Free resources that we allocated for this intr attachment. 
	 */
	intr_detach_cleanup(intr_attach_context);

	udi_intr_detach_ack(cb);
}

STATIC void
intr_detach_cleanup(void *mumble)
{
	intr_attach_context_t *intr_attach_context = mumble;
	udi_intr_event_cb_t *event_cb;

	_BRIDGE_DEBUG("intr_detach_cleanup\n");
	/*
	 * Destroy the intr_event_channel. 
	 */

	udi_channel_close(intr_attach_context->intr_event_channel);
	udi_cb_free(intr_attach_context->orun_cb);

	do {
		/*
		 * Dequeue a free list element
		 */
		INTR_EVENT_LIST_REMOVE(intr_attach_context, event_cb);
		if (event_cb != NULL) {
			/*
			 * Free any attached buf
			 */
			udi_buf_free(event_cb->event_buf);

			/*
			 * Free the cb
			 */
			udi_cb_free(UDI_GCB(event_cb));
		}
	} while (event_cb != NULL);

	/*
	 * Some OS's may need to deallocate a mutex here 
	 */

	_OSDEP_SIMPLE_MUTEX_DEINIT(&intr_attach_context->mutex);

	/*
	 * Free this intr attach context. 
	 */
	udi_pio_unmap(intr_attach_context->pio_handle);
}

/*
 * ========================================================================
 * 
 * Interrupt Dispatcher Operations
 * 
 * ------------------------------------------------------------------------
 */
STATIC void
intr_dispatcher_channel_event_ind(udi_channel_event_cb_t *channel_event_cb)
{
	/*
	 * could be channel closed from the interupt channel
	 */
	_BRIDGE_DEBUG("intr_dispatcher_channel_event_ind\n");
	udi_cb_free(UDI_GCB(channel_event_cb));
}

STATIC void
intr_dispatcher_intr_event_rdy(udi_intr_event_cb_t *intr_event_cb)
{
	intr_attach_context_t *attach_context =
		UDI_GCB(intr_event_cb)->context;

	_BRIDGE_DEBUG("intr_dispatcher_intr_event_rdy attach_context"
		      " %p  cb %p state %d\n",
		      attach_context, intr_event_cb,
		      attach_context->dispatcher_state);
	_OSDEP_ASSERT(attach_context != NULL);

	/*
	 * Preserve the interrupt attachment context in the gcb so that
	 * the PIO callback can find the context from the gcb.
	 */
	UDI_GCB(intr_event_cb)->initiator_context = attach_context;

	/*
	 * Link the cb onto the intr_event_cb free list. 
	 */

	switch (attach_context->dispatcher_state) {
	case ds_disarmed:
	case ds_overrunning:
	case ds_flow_controlled:
		/*
		 * Enter ds_armed if we have enough cb's.
		 */
		if (attach_context->num_event_cbs_on_q + 1 >=
		    attach_context->min_event_pend) {
			intr_event_cb_scratch_t *iecbs;

			_BRIDGE_DEBUG("Entering ARMED\n");
			iecbs = UDI_GCB(intr_event_cb)->scratch;
			iecbs->intr_result = 0;
			attach_context->dispatcher_state = ds_armed;
			_BRIDGE_DEBUG("Calling PIO 0\n");
			udi_pio_trans(_udi_bridge_mapper_isr_1,
				      UDI_GCB(intr_event_cb),
				      attach_context->pio_handle,
				      0, intr_event_cb->event_buf, NULL);
			/*
			 * Keep the cb by bypassing the LIST_APPEND.
			 */
			return;
		}
	case ds_armed:
	case ds_interrupt_region:
		/*
		 * Nothing to do 
		 */
		break;
	default:
		_OSDEP_ASSERT(0);
	}

	INTR_EVENT_LIST_APPEND(attach_context, intr_event_cb);
}

/*
 * ========================================================================
 * 
 * Bridge Mapper ISR -- called for all interrupts generated for UDI 
 *   interrupt handlers.  
 *
 *   Parameters:
 *     The context parameter is an OS-specific parameter which can point to
 *     an OS-specific structure if that's useful in a given OS.  Typically
 *     this will point directly to the intr_attach_context_t associated
 *     with a UDI interrupt attachment.
 *
 *   MP Locking: 
 *     This routine does not execute in the bridge mapper's region.  
 *     Therefore any common structures updated by this routine must be 
 *     protected by the intr_attach_context's mutex, both here and 
 *     elsewhere in the bridge mapper.  The macros INTR_EVENT_LIST_REMOVE
 *     and INTR_EVENT_LIST_APPEND provide this for the intr_event_cb free
 *     list.
 *     Also, this implementation assumes that the ISR is non-reentrant
 *     for a given interrupt attachment (a typical property of ISRs)
 * ------------------------------------------------------------------------
 */

STATIC _OSDEP_ISR_RETURN_T
_udi_bridge_mapper_isr(void *_context)
{
	udi_intr_event_cb_t *intr_event_cb;
	intr_attach_context_t *intr_attach_context =
		(intr_attach_context_t *) _OSDEP_GET_ISR_CONTEXT(_context);

	void *event_info = (void *)_OSDEP_GET_EVENT_INFO(_context);
	intr_event_cb_scratch_t *iecbs;
	int pio_label;

	_BRIDGE_DEBUG("_udi_bridge_mapper_isr attach_context %p  state %d\n",
		      intr_attach_context,
		      intr_attach_context->dispatcher_state);

	switch (intr_attach_context->dispatcher_state) {
	case ds_armed:
		INTR_EVENT_LIST_REMOVE(intr_attach_context, intr_event_cb);
		iecbs = UDI_GCB(intr_event_cb)->scratch;
		iecbs->intr_result = 0;
		if (intr_attach_context->num_event_cbs_on_q == 0) {
			intr_attach_context->dispatcher_state =
				ds_flow_controlled;
			pio_label = 2;
		} else {
			pio_label = 1;
		}
		_BRIDGE_DEBUG("(ds_armed) Calling PIO %d\n", pio_label);
		udi_pio_trans(_udi_bridge_mapper_isr_1,
			      UDI_GCB(intr_event_cb),
			      intr_attach_context->pio_handle,
			      pio_label, intr_event_cb->event_buf, event_info);

		return _OSDEP_ISR_RETURN_VAL(intr_attach_context->intr_result);

	case ds_disarmed:
		/*
		 * We must be sharing an interrupt with something
		 * but we haven't completed our attachement yet.
		 */
		/*
		 * FALLTHRU 
		 */
	case ds_flow_controlled:
		intr_attach_context->dispatcher_state = ds_overrunning;
		/*
		 * FALLTHRU 
		 */
	case ds_overrunning:
		_BRIDGE_DEBUG("(ds_orun) Calling PIO 3\n");
		udi_pio_trans(_udi_bridge_mapper_isr_1,
			      intr_attach_context->orun_cb,    
			      intr_attach_context->pio_handle,
			      3, intr_event_cb->event_buf, event_info);
		return _OSDEP_ISR_RETURN_VAL(intr_attach_context->intr_result);
	case ds_interrupt_region:
		/*
		 * TODO: be sure INTR_PREPROCESSED bit is clear
		 * and call (or schedule to be called) the intr 
		 * region.
		 */
		return _OSDEP_ISR_RETURN_VAL(intr_attach_context->intr_result);
	default:
		_OSDEP_ASSERT(0);
	}
}

/*
 * this is the callback for the pio_trans function
 * Status may contain two bits of interest.
 * INTR_UNCLAIMED means that this was not the device causing the interrupt
 *    (Hint: consider shared interrupts; this means "call the next ISR".
 * INTR_NO_EVENT means that no udi_intr_event_ind operation should be sent
 *    to the handler driver.
 */
STATIC void
_udi_bridge_mapper_isr_1(udi_cb_t *gcb,
			 udi_buf_t *buf,
			 udi_status_t status,
			 udi_ubit16_t intr_result)
{
	udi_intr_event_cb_t *intr_event_cb = UDI_MCB(gcb, udi_intr_event_cb_t);
	intr_attach_context_t *intr_attach_context =
		UDI_GCB(intr_event_cb)->initiator_context;
	int indicator_return = UDI_INTR_PREPROCESSED;
	intr_event_cb_scratch_t *intr_scratch = gcb->scratch;

	_BRIDGE_DEBUG("_udi_bridge_mapper_isr_1.  State %d. "
		      "PIO Scratch 0x%x.  PIO Return 0x%x.\n",
		      intr_attach_context->dispatcher_state,
		      intr_scratch->intr_result, intr_result);
	/*
	 * if we got an immediate callback, this will be useful 
	 */
	intr_attach_context->intr_result = intr_scratch->intr_result;
	intr_event_cb->intr_result = intr_result;

	/*
	 * Keep track of the event buffer in case udi_pio_trans had to
	 * reallocate the buffer becuase it was fragged.
	 */
	intr_event_cb->event_buf = buf;

	/*
	 * ack the interrupt to the OS 
	 */

	_OSDEP_ISR_ACKNOWLEDGE(intr_result);

	switch (intr_attach_context->dispatcher_state) {
	case ds_armed:
		if (intr_scratch->
		    intr_result & (UDI_INTR_NO_EVENT | UDI_INTR_UNCLAIMED)) {
			INTR_EVENT_LIST_APPEND(intr_attach_context,
					       intr_event_cb);
			return;
		}
		break;

	case ds_flow_controlled:
		if (intr_scratch->intr_result & UDI_INTR_NO_EVENT) {
			INTR_EVENT_LIST_APPEND(intr_attach_context,
					       intr_event_cb);
			return;
		}
		if (intr_scratch->intr_result & UDI_INTR_UNCLAIMED) {
			intr_attach_context->dispatcher_state = ds_armed;
			INTR_EVENT_LIST_APPEND(intr_attach_context,
					       intr_event_cb);
			return;
		}
		intr_attach_context->dispatcher_state = ds_overrunning;
		/*
		 * FALLTHRU 
		 */
	case ds_overrunning:
		if (!(intr_scratch->intr_result & UDI_INTR_UNCLAIMED)) {
			indicator_return |= UDI_INTR_OVERRUN_OCCURRED;
		}
		return;

		/*
		 * These should never happen.
		 */
	case ds_interrupt_region:
	case ds_disarmed:
	default:
		_OSDEP_ASSERT(0);
	}

	udi_intr_event_ind(intr_event_cb, indicator_return);
}
