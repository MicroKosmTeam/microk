
/*
 * File:  bridgetest_driver.c
 *
 * UDI Bridge Test Sample Driver
 * This driver tests the functionality of the common bridge mapper 
 * by registering for interrupt attachment and driving interrupts....
 * 
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

#define UDI_VERSION 0x101
#define UDI_PHYSIO_VERSION 0x101
#include <udi.h>			/* Standard UDI Definitions */
#include <udi_physio.h>			/* Physical I/O Extensions */

static void xyz_bus_bind_ack_1(udi_cb_t *,
			       udi_pio_handle_t);
static void xyz_bus_bind_ack_2(udi_cb_t *,
			       udi_pio_handle_t);
static void xyz_bus_bind_ack_3(udi_cb_t *gcb,
			       udi_channel_t new_channel);
static void xyz_bus_bind_ack_4(udi_cb_t *gcb,
			       udi_cb_t *new_cb);
static void got_intr_buf(udi_cb_t *gcb,
			 udi_buf_t *new_buf);
static void got_intr_cb(udi_cb_t *gcb,
			udi_cb_t *new_cb);
static void xyz_udi_instance_attr_get_1(udi_cb_t *,
					udi_instance_attr_type_t,
					udi_size_t);


/*
 * Region data structure for this driver.
 */
typedef struct {
	udi_init_context_t init_context;	/* provided by environment */
	udi_channel_t bridge_channel;	/* channel to parent bridge */
	udi_channel_t intr_channel;	/* interrupt event channel */
	udi_mgmt_cb_t *mgmt_cb;		/* unbind control block */
	udi_bus_bind_cb_t *bus_bind_cb;	/* bus bind control block */
	udi_ubit32_t event_cb_count;	/* number of intr event cb's we have */
	udi_intr_attach_cb_t *intr_attach_cb;	/* interrupt attach control block */
	udi_channel_event_cb_t *chan_ev_cb;	/* interrupt attach control block */
	/*
	 * PIO trans handle for interrupt preprocessor.
	 */
	udi_pio_handle_t intr_handle;
	udi_buf_path_t buf_path;
} xyz_region_data_t;

typedef struct {
	udi_ubit32_t attr_value;	/* attribute value */
} intr_attach_cb_scratch_t;

udi_index_t xyz_driver_intr_spawn_idx = 0;

/*
 * CB indexes for each of the types of control blocks used.
 */
#define BUS_BIND_CB_IDX		4
#define INTR_ATTACH_CB_IDX	5
#define INTR_EVENT_CB_IDX	6
#define INTR_DETACH_CB_IDX	7

/*
 * Scratch sizes for each control block type.
 */
#define BUS_BIND_SCRATCH	0
#define INTR_ATTACH_SCRATCH	sizeof(intr_attach_cb_scratch_t)
#define INTR_EVENT_SCRATCH	0
#define INTR_DETACH_SCRATCH	0

/*
 * Ops indexes for each of the entry point ops vectors used.
 */
#define BUS_DEVICE_OPS_IDX	2
#define INTR_HANDLER_OPS_IDX	3

#define MIN_INTR_EVENT_CBS 	4
#define NUM_INTR_EVENT_CBS 	8

/*
 * Driver entry points for the Management Metalanguage.
 */
static udi_devmgmt_req_op_t xyz_devmgmt_req;
static udi_final_cleanup_req_op_t xyz_final_cleanup_req;

static udi_mgmt_ops_t xyz_mgmt_ops = {
	udi_static_usage,
	udi_enumerate_no_children,
	xyz_devmgmt_req,
	xyz_final_cleanup_req
};

/*
 * Driver "bottom-side" entry points for the Bus Bridge Metalanguage.
 */

static udi_channel_event_ind_op_t xyz_parent_channel_event;
static udi_bus_bind_ack_op_t xyz_bus_bind_ack;
static udi_bus_unbind_ack_op_t xyz_bus_unbind_ack;
static udi_intr_attach_ack_op_t xyz_intr_attach_ack;
static udi_intr_detach_ack_op_t xyz_intr_detach_ack;

static udi_bus_device_ops_t xyz_bus_device_ops = {
	xyz_parent_channel_event,
	xyz_bus_bind_ack,
	xyz_bus_unbind_ack,
	xyz_intr_attach_ack,
	xyz_intr_detach_ack
};

static udi_channel_event_ind_op_t xyz_intr_channel_event;
static udi_intr_event_ind_op_t xyz_intr_event_ind;

static udi_intr_handler_ops_t xyz_intr_handler_ops = {
	xyz_intr_channel_event,
	xyz_intr_event_ind
};

/*
 * none of my channel ops need any special flags so use this one for all
 */
static const udi_ubit8_t xyz_default_op_flags[] = {
	0, 0, 0, 0, 0};

/*
 * PIO properties of the physical I/O registers we'll use on the trans list.
 */
#define CMOS_REGSET	0		/* first (and only) register set */
#define CMOS_BASE	0		/* base address within this register set */
#define CMOS_LENGTH	0		/* No device registers. */
#define CMOS_PACE	0		/* wait 5 microseconds between accesses */

#define ABORT_SCRATCH	100		/* Scratch size for abort sequence */

/*
 *	A simple trans list to increment the first word (4 bytes)
 *	of the buffer argument.
 */
static udi_pio_trans_t intr_preprocess_list[] = {
	/* 
	 *  Entry point zero is to enable interrupts for the first time.
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, UDI_INTR_UNCLAIMED},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0},
	{UDI_PIO_STORE + UDI_PIO_SCRATCH + UDI_PIO_R1, UDI_PIO_1BYTE, 
		UDI_PIO_R0},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0xffff},

	{UDI_PIO_LABEL, 0, 1},
	{UDI_PIO_LABEL, 0, 2},
	{UDI_PIO_LABEL, 0, 3},

	/*
	 * R0 <- offset into BUF 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0x0000},
	/*
	 * R1 <- buf[0] 
	 */
	{UDI_PIO_LOAD + UDI_PIO_BUF + UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1},
	/*
	 * R1 ++ 
	 */
	{UDI_PIO_ADD_IMM + UDI_PIO_R1, UDI_PIO_4BYTE, 1},
	/*
	 * buf[0] <- R1 
	 */
	{UDI_PIO_STORE + UDI_PIO_BUF + UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1},
	/* 
	 * "Forget" to claim our interruption every once in a while just
	 * to exercise the UDI_INTR_UNCLAIMED paths.
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, 0x0000},
	{UDI_PIO_ADD + UDI_PIO_R7, UDI_PIO_2BYTE, UDI_PIO_R1},
	{UDI_PIO_AND_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, 0x03},

	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0x0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_2BYTE, UDI_PIO_NZ},
		{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, UDI_INTR_UNCLAIMED},

	{UDI_PIO_STORE + UDI_PIO_SCRATCH + UDI_PIO_R1, UDI_PIO_1BYTE, 
		UDI_PIO_R0},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0xffff},
};

static udi_pio_trans_t abort_sequence[] = {
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0x4321}, 
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, ABORT_SCRATCH-4},
	{ UDI_PIO_STORE + UDI_PIO_SCRATCH + UDI_PIO_R0, 
		UDI_PIO_4BYTE, UDI_PIO_R1},
	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 }
};

static udi_primary_init_t bd_primary_init = {
	&xyz_mgmt_ops,
	xyz_default_op_flags,
	0,				/* mgmt scratch */

	/*
	 * SINCE OUR MANAGEMENT META SAYS udi_enumerate_no_children     
	 * THERE ARE NO CHILDREN - THEREFORE NO ENUMERATION CALLS       
	 * AND NO ATTRIBUTES TO RETURN IN THE ATTR_LIST                 
	 */
	0,				/* enum attr list length */
	sizeof (xyz_region_data_t),
	0, /* child data */
	1,		/* buf path */
};

static udi_ops_init_t bd_ops_init_list[] = {
	{
	 BUS_DEVICE_OPS_IDX,
	 MY_BRIDGE_META,		/* meta index */
	 UDI_BUS_DEVICE_OPS_NUM,	/* meta ops num */
	 0,				/* chan context size */
	 (udi_ops_vector_t *) & xyz_bus_device_ops,
	 xyz_default_op_flags
	 },
	{
	 INTR_HANDLER_OPS_IDX,
	 MY_BRIDGE_META,		/* meta index */
	 UDI_BUS_INTR_HANDLER_OPS_NUM,	/* meta ops num */
	 0,				/* chan context size */
	 (udi_ops_vector_t *) & xyz_intr_handler_ops,
	 xyz_default_op_flags
	 },
	{
	 0}

};

static udi_cb_init_t bd_cb_init_list[] = {
	{
	 BUS_BIND_CB_IDX,
	 MY_BRIDGE_META,		/* meta idx */
	 UDI_BUS_BIND_CB_NUM,
	 BUS_BIND_SCRATCH
	 /*
	  * No inline, no layout 
	  */
	 },
	{
	 INTR_ATTACH_CB_IDX,
	 MY_BRIDGE_META,		/* meta idx */
	 UDI_BUS_INTR_ATTACH_CB_NUM,
	 INTR_ATTACH_SCRATCH,
	 /*
	  * No inline, no layout 
	  */
	 },
	{
	 INTR_EVENT_CB_IDX,
	 MY_BRIDGE_META,		/* meta idx */
	 UDI_BUS_INTR_EVENT_CB_NUM,
	 INTR_EVENT_SCRATCH
	 /*
	  * No inline, no layout 
	  */
	 },
	{
	 INTR_DETACH_CB_IDX,
	 MY_BRIDGE_META,		/* meta idx */
	 UDI_BUS_INTR_DETACH_CB_NUM,
	 INTR_DETACH_SCRATCH,
	 /*
	  * No inline, no layout 
	  */
	 },
	{
	 NULL}
};

static udi_secondary_init_t secondary_rgn_init_list[] = {
	{
	 1,
	 sizeof (udi_init_context_t)	/* rgn data */
	 }
	,
	{
	 0}
};

udi_init_t udi_init_info = {
	&bd_primary_init,
	NULL, /*secondary_rgn_init_list,*/
	bd_ops_init_list,
	bd_cb_init_list,
	NULL,				/* gcb init list */
	NULL,				/* cb select list */
};



static void
xyz_bus_bind_ack(udi_bus_bind_cb_t * bus_bind_cb,
		 udi_dma_constraints_t constraints,
		 udi_ubit8_t preferred_endianness,
		 udi_status_t status)
{
	xyz_region_data_t *rdata = bus_bind_cb->gcb.context;

	/*
	 * Don't need to do anything with preferred_endianness, since our
	 * device doesn't do DMA. Even if it did, preferred_endianness is
	 * only used with bi-endianness devices that can change endianness.
	 */

	/* 
	 * i do not use constraints thank you
	 */
	udi_dma_constraints_free(constraints);
	/*
	 * Save the bus bind control block for unbinding later.
	 */
	rdata->bus_bind_cb = bus_bind_cb;

	rdata->bridge_channel = bus_bind_cb->gcb.channel;

	udi_pio_map(xyz_bus_bind_ack_1, UDI_GCB(bus_bind_cb),
		    CMOS_REGSET, CMOS_BASE, CMOS_LENGTH,
		    intr_preprocess_list,
		    sizeof intr_preprocess_list / sizeof (udi_pio_trans_t),
		    UDI_PIO_STRICTORDER, CMOS_PACE, 0);
}


static void
xyz_bus_bind_ack_1(udi_cb_t *gcb,
		   udi_pio_handle_t new_pio_handle)
{
	xyz_region_data_t *rdata = gcb->context;

	/*
	 * Save the PIO handle for later use. 
	 */
	rdata->intr_handle = new_pio_handle;

	udi_pio_map(xyz_bus_bind_ack_2, gcb,
		    CMOS_REGSET, CMOS_BASE, 0,
		    abort_sequence,
		    sizeof abort_sequence / sizeof (udi_pio_trans_t),
		    UDI_PIO_STRICTORDER, CMOS_PACE, 0);
}

static void
xyz_bus_bind_ack_2(udi_cb_t *gcb,
		   udi_pio_handle_t abort_handle)

{
	xyz_region_data_t *rdata = gcb->context;

	udi_pio_abort_sequence(abort_handle, ABORT_SCRATCH);

	/*
	 * we spawn our end of the channel 
	 * the bridge will spawn its end. 
	 * because the ops idx is non-zero, this is an anchored channel 
	 */

	udi_channel_spawn(xyz_bus_bind_ack_3,
			  gcb, rdata->bridge_channel,
			  xyz_driver_intr_spawn_idx,
			  INTR_HANDLER_OPS_IDX, rdata);
}

/*	This is the channel spawn callback		*/

/*	The channel is used for interrupt events	*/
static void
xyz_bus_bind_ack_3(udi_cb_t *gcb,
		   udi_channel_t new_channel)
{
	xyz_region_data_t *rdata = gcb->context;

	rdata->intr_channel = new_channel;


/*
 *	Interrupt Attachment...
 *	1. Allocate an interrupt_attach_cb_t
 *	2. Fill it out...
 *	3. call udi_intr_attach_re
 */

	udi_cb_alloc(xyz_bus_bind_ack_4, gcb, INTR_ATTACH_CB_IDX, new_channel);
}

static void
xyz_bus_bind_ack_4(udi_cb_t *gcb,
		   udi_cb_t *new_cb)
{
	xyz_region_data_t *rdata = gcb->context;
	udi_intr_attach_cb_t *cb = UDI_MCB(new_cb, udi_intr_attach_cb_t);

	/*
	 * this may not be the final one we get to save 
	 */
	rdata->intr_attach_cb = cb;

	/*
	 * ok to move on to the next index 
	 */
	cb->interrupt_idx = xyz_driver_intr_spawn_idx++;
	cb->min_event_pend = MIN_INTR_EVENT_CBS;

	/*
	 * This is where we could attach a PIO HANDLE 
	 */

#if 1

	/*
	 * Take a look at the device attributes in the node 
	 */
	udi_instance_attr_get(xyz_udi_instance_attr_get_1,
			      gcb,
			      "pci_vendor_id",
			      NULL,
			      &(
				((intr_attach_cb_scratch_t
				  *) (UDI_GCB(cb)->scratch))->attr_value), 4);
#else
	 /*TODO*/ if (xyz_driver_intr_spawn_idx++) {
		cb->preprocessing_handle = rdata->intr_handle;
	}

	udi_intr_attach_req(rdata->bridge_channel, cb);

	/*
	 * we pick up processing in the xyz_intr_attach_ack() 
	 */
#endif
}

static void
xyz_udi_instance_attr_get_1(udi_cb_t *gcb,
			    udi_instance_attr_type_t attr_type,
			    udi_size_t actual_length)
{
	xyz_region_data_t *rdata = gcb->context;

	udi_intr_attach_cb_t *cb = rdata->intr_attach_cb;

	if (((intr_attach_cb_scratch_t *) (UDI_GCB(cb)->scratch))->attr_value
	    != 0x1111) {
		cb->preprocessing_handle = rdata->intr_handle;
	} else {
		/*
		 * I have no idea why Richard mapped these if he wasn't
		 * going to tell the interrupt handler about them.
		 * Since we aren't going to use this PIO handle, unmap
		 * it at this time.
		 */
		udi_pio_unmap(rdata->intr_handle);
		cb->preprocessing_handle = NULL;
	}


	/*
	 * set the channel for our attach request
	 */
	cb->gcb.channel = rdata->bridge_channel;

	/*
	 * we pick up processing in the xyz_intr_attach_ack() 
	 */
	udi_intr_attach_req(cb);

}

static void
xyz_parent_channel_event(udi_channel_event_cb_t * channel_event_cb)
{
	xyz_region_data_t *rdata = UDI_GCB(channel_event_cb)->context;
	udi_bus_bind_cb_t *bcb = UDI_MCB(channel_event_cb->
		params.parent_bound.bind_cb, udi_bus_bind_cb_t);

	switch (channel_event_cb->event) {
	case UDI_CHANNEL_BOUND:

		rdata->chan_ev_cb = channel_event_cb;
		rdata->buf_path = channel_event_cb->
			params.parent_bound.path_handles[0];
		UDI_GCB(bcb)->initiator_context = channel_event_cb;
		udi_bus_bind_req(bcb);
		break;
	default:
		udi_channel_event_complete(channel_event_cb, UDI_OK);
	}
}

static void
xyz_bus_unbind_ack(udi_bus_bind_cb_t * bus_bind_cb)
{
	xyz_region_data_t *rdata = bus_bind_cb->gcb.context;

	udi_cb_free(UDI_GCB(bus_bind_cb));
	udi_devmgmt_ack(rdata->mgmt_cb, 0, UDI_OK);
}

static void
xyz_intr_detach_callback(udi_cb_t *gcb,
			 udi_cb_t *new_cb)
{
	udi_intr_detach_cb_t *dcb;
	xyz_region_data_t *rdata = gcb->context;

	/*
	 * We were attached multiple times with different interrupt_idx
	 * values, so we have to simulate the same on detachment. 
	 */
	static int detach_count;

	dcb = UDI_MCB(new_cb, udi_intr_detach_cb_t);
	dcb->interrupt_idx = detach_count++;

	udi_cb_free(UDI_GCB(rdata->intr_attach_cb));
	udi_intr_detach_req(dcb);
}

static void
xyz_devmgmt_req(udi_mgmt_cb_t * cb,
		udi_ubit8_t mgmt_op,
		udi_ubit8_t parent_id)
{
	xyz_region_data_t *rdata = UDI_GCB(cb)->context;

	switch (mgmt_op) {
	case UDI_DMGMT_UNBIND:
		rdata->mgmt_cb = cb;
		udi_cb_alloc(xyz_intr_detach_callback,
			     UDI_GCB(cb),
			     INTR_DETACH_CB_IDX, rdata->bridge_channel);
		break;
	default:
		udi_devmgmt_ack(cb, 0, UDI_OK);
	}

}

static void
xyz_final_cleanup_req(udi_mgmt_cb_t * cb)
{
	/*
	 * We have nothing to free that wasn't already freed by
	 * unbinding children and parents.
	 */

	udi_final_cleanup_ack(cb);
}

static void
xyz_intr_attach_ack(udi_intr_attach_cb_t * intr_attach_cb,
		    udi_status_t status)
{
	udi_cb_t *gcb;
	xyz_region_data_t *rdata = UDI_GCB(intr_attach_cb)->context;

	/*
	 * NOTE: Our original CB could have been transformed. 
	 */
	rdata->intr_attach_cb = intr_attach_cb;	/* save for later detach */

	gcb = rdata->bus_bind_cb->gcb.initiator_context;

	udi_cb_alloc(got_intr_cb, UDI_GCB(intr_attach_cb), INTR_EVENT_CB_IDX,
                             rdata->intr_channel);
}

static void
xyz_intr_detach_ack(udi_intr_detach_cb_t * intr_attach_cb)
{
	xyz_region_data_t *rdata = UDI_GCB(intr_attach_cb)->context;

	/*
	 * Complete interrupt detachment here... 
	 * free the intr_attach_cb 
	 */
	udi_cb_free(UDI_GCB(intr_attach_cb));

	udi_bus_unbind_req(rdata->bus_bind_cb);

}

static
void
got_intr_buf(udi_cb_t *gcb,
             udi_buf_t *new_buf)
{                               
        udi_intr_event_cb_t *intr_event_cb = UDI_MCB(gcb, udi_intr_event_cb_t);

	/* Notify the bridge dispatcher that it can have this cb */
	intr_event_cb->event_buf = new_buf;
	udi_intr_event_rdy(intr_event_cb);
}
static 
void
got_intr_cb(udi_cb_t *gcb,
		udi_cb_t *new_cb)
{
	xyz_region_data_t *rdata = gcb->context;                               

	/* 
	 * We sort of do a "fork" here.  We take the new cb and use it
	 * to lloc our buf which, once we have it, we attach to the cb and
	 * hand to the dispatcher.   For the original cb we're holding, we
	 * use that to alloc more cb's.
	 * 
	 * TODO: use udi_cb_alloc_batch.
	 */
	UDI_BUF_ALLOC(got_intr_buf, new_cb, NULL, 1024, rdata->buf_path);
        if (++rdata->event_cb_count < NUM_INTR_EVENT_CBS) {
		udi_cb_alloc(got_intr_cb, gcb, INTR_EVENT_CB_IDX,
				     rdata->intr_channel);
	} else {
		udi_channel_event_complete(rdata->chan_ev_cb, UDI_OK);
	}
}

static void
xyz_intr_event_ind(udi_intr_event_cb_t * intr_event_cb,
		   udi_ubit8_t flags)
{
	/*
	 * Nothing really to do.   Just hand the CB back to the dispatcher.
	 */
	udi_assert(intr_event_cb->intr_result == 0xffff);
	udi_intr_event_rdy(intr_event_cb);
}

static void
xyz_intr_channel_event(udi_channel_event_cb_t * channel_event_cb)
{
	udi_channel_event_complete(channel_event_cb, UDI_OK);
}
