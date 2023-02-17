
/*
 * File:  udi_bridgeP.h
 *
 * UDI Bus Bridge Mapper private header file
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
#ifndef __UDI_BRIDGEP_H__
#define __UDI_BRIDGEP_H__

#define BRIDGE_MAX_INTERRUPT_ATTACHMENT_COUNT 4

/*
 * If the OSDEP part of the bridge mapper doesn't define this, make it a 
 * no-op.
 * Note: If the OSDEP part defines it, the function must be synchronous.
 */
#ifndef _OSDEP_BUS_BRIDGE_BIND_REQ
#define _OSDEP_BUS_BRIDGE_BIND_REQ
#endif

/* 
 * Region data area.
 */
typedef struct bridge_mapper_region_data {
	udi_init_context_t init_context;	/* Std region init_context */
	udi_queue_t child_list;
	udi_ubit32_t r_child_id;
	_OSDEP_BRIDGE_RDATA_T osinfo;
} bridge_mapper_region_data_t;

/* 
 * Child Device Data.
 */
typedef struct bridge_mapper_child_data {
	udi_queue_t link;
	udi_ubit32_t child_ID;

	/*
	 * This is an array of attachments... The max attach    
	 * count should be 255 due to the size of the data      
	 * type for the interrupt_idx.                          
	 * HOWEVER, Kurt says a count of four should be enough  
	 */

	struct intr_attach_context
	 *interrupt_attachments[BRIDGE_MAX_INTERRUPT_ATTACHMENT_COUNT];

	/*
	 * _OSDEP_ENUMERATE_PCI_DEVICE() will fill in these     
	 * values so we can return attributes.                  
	 * Element is described in PHYSICAL I/O SPEC Table 6-1  
	 */

	udi_ubit32_t pci_vendor_id;
	udi_ubit32_t pci_device_id;
	udi_ubit32_t pci_revision_id;
	udi_ubit32_t pci_base_class;
	udi_ubit32_t pci_sub_class;
	udi_ubit32_t pci_prog_if;
	udi_ubit32_t pci_subsystem_vendor_id;
	udi_ubit32_t pci_subsystem_id;
	udi_ubit32_t pci_unit_address;
	udi_ubit32_t pci_slot;

	/*
	 * This is for the OS to provide information needed for 
	 * interrupt registration.                              
	 * example usage is device identificaton and irq level  
	 */

	_OSDEP_BRIDGE_CHILD_DATA_T osinfo;
} bridge_mapper_child_data_t;

/* 
 * Child Context 
 */
typedef struct bridge_mapper_context {
	udi_child_chan_context_t chan_context;
	bridge_mapper_child_data_t *child_data;
} bridge_mapper_child_context_t;

/*
 * Dispatcher state for any given attachment.
 */
typedef enum {
	ds_invalid = 0,
	ds_disarmed,
	ds_armed,
	ds_flow_controlled,
	ds_overrunning,
	ds_interrupt_region
} dispatcher_state_t;

/* 
 * Info specific to a given interrupt attachment.
 *
 * The mutex here is needed for updates to the free_list since it can be
 * updated in the ISR when we're not executing in a region.
 */
typedef struct intr_attach_context {
	_OSDEP_SIMPLE_MUTEX_T mutex;
	struct bridge_mapper_child_data *child_data;
	udi_channel_t intr_event_channel;
	udi_ubit32_t num_event_cbs_on_q;
	udi_intr_event_cb_t *free_list;
	udi_index_t interrupt_idx;
	udi_index_t min_event_pend;
	dispatcher_state_t dispatcher_state;
	/*
	 * if the interrupt attachment needs preprocessing, use the 
	 * pio_handle for it.
	 */
	udi_pio_handle_t pio_handle;
	udi_cb_t *orun_cb;		/* cb for interrupt overrunn state */
	udi_status_t intr_attach_result;
	udi_ubit16_t intr_result;

	_OSDEP_INTR_T osinfo;
} intr_attach_context_t;

/*
 * Scratch area in enumerate control blocks.
 */
typedef struct {
	/*
	 * pass this through allocation
	 */
	udi_ubit8_t enumeration_level;
} enumerate_cb_scratch_t;

/*
 * Scratch area in intr_attach control blocks.
 */
typedef struct {

/*
 * This is the interrupt level context before it is completely set up
 * and linked with the device region.
 */
	intr_attach_context_t *intr_attach_context;

/*
 * This is a cb that we are holding separate until it gets a buf allocated.
 */
	udi_intr_event_cb_t *new_cb;
} intr_attach_cb_scratch_t;

/*
 * Scratch area in intr_event control blocks.
 */
typedef struct {
	udi_ubit8_t intr_result;
	udi_intr_event_cb_t *next;
} intr_event_cb_scratch_t;

/* Macros to access fields in the intr attach context. */
#define _INTR_EVENT_CHANNEL	(intr_attach_context->intr_event_channel)

#define INTR_EVENT_NEXT(event_cb)			\
	(((intr_event_cb_scratch_t *)((UDI_MCB(event_cb,udi_cb_t))->scratch))->next)

#if DEBUG_BRIDGE
static void
dumplist(intr_attach_context_t * context)
{
	int i;
	udi_intr_event_cb_t *iecb;

	iecb = context->free_list;
	_OSDEP_PRINTF("Free list:");
	while (1) {
		_OSDEP_PRINTF("%p ", iecb);
		if (iecb != NULL) {
			_OSDEP_PRINTF("(%p ",
				      ((UDI_MCB(iecb, udi_cb_t))->scratch));
			_OSDEP_PRINTF("%x) ",
				      *(char *)((UDI_MCB(iecb, udi_cb_t))->
						scratch));
			iecb = INTR_EVENT_NEXT(iecb);
		}
		if (iecb == NULL)
			break;
	}
	_OSDEP_PRINTF("\n");

}
#endif /* DEBUG_BRIDGE */


/* Must lock around updates to event_cb free_list since it can
   be updated in the ISR when we're not executing in a region. */
#define INTR_EVENT_LIST_APPEND(context, event_cb)	\
	_OSDEP_SIMPLE_MUTEX_LOCK(&context->mutex);	\
        INTR_EVENT_NEXT(event_cb) = context->free_list;	\
	context->free_list = event_cb;			\
	context->num_event_cbs_on_q++; 			\
	_OSDEP_SIMPLE_MUTEX_UNLOCK(&context->mutex)

#define INTR_EVENT_LIST_REMOVE(context, event_cb)	\
	_OSDEP_SIMPLE_MUTEX_LOCK(&context->mutex);	\
	event_cb = context->free_list;			\
	if (event_cb != NULL)				\
	    context->free_list = INTR_EVENT_NEXT(context->free_list); \
	context->num_event_cbs_on_q--;			 \
	_OSDEP_SIMPLE_MUTEX_UNLOCK(&context->mutex)


#define BRIDGE_SET_ATTR_UBIT32(attr, name, val)	\
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_UBIT32; \
		(attr)->attr_length = sizeof(udi_ubit32_t); \
		UDI_ATTR32_SET((attr)->attr_value, (val))

#define  BRIDGE_SET_ATTR_STRING(attr, name, val) \
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_STRING; \
		(attr)->attr_length = udi_strlen(val) + 1; \
		udi_strcpy((char *)(attr)->attr_value, (val))

#ifndef _OSDEP_BUS_BRIDGE_BIND_DONE
#define _OSDEP_BUS_BRIDGE_BIND_DONE(x)
#endif

#ifndef _OSDEP_BUS_BRIDGE_UNBIND_DONE
#define _OSDEP_BUS_BRIDGE_UNBIND_DONE(x)
#endif

#ifndef _OSDEP_BUS_BRIDGE_DEINIT
#define _OSDEP_BUS_BRIDGE_DEINIT(x)
#endif

/*
#define _OSDEP_BUS_BRIDGE_ALLOC_CONSTRAINTS
and privide a udi_dma_constraints_attr_spec_t array named
_osdep_bridge_attr_list if you want to use your constraints instead
of the default attributes.
 */

#endif /* __UDI_BRIDGEP_H__ */
