
/*
 * File: giomap.c
 *
 * A mapper which converts OS specific driver i/o requests to 
 * udi gio driver requests.
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




/*
 * ========================================================================
 *
 *	NOTE: This code is #include'd by the OS-specific mapper code and
 *	      provides the UDI-specific implementation for the GIO interface.
 *
 *	Required OS-specific routines:
 *		giomap_OS_init	
 *			initialise OS-specific members of the region data area.
 *			This is a non-blocking synchronous routine.
 *		giomap_OS_deinit
 *			release any OS-specific members of the region data area
 *			which were allocated by giomap_OS_init (e.g. MUTEXs).
 *			This is a non-blocking routine.
 *		giomap_OS_bind_done
 *			Called when the GIO bind has completed. The OS code
 *			should perform any initialisation required and then
 *			call udi_channel_event_complete with the passed
 *			parameters
 *		giomap_OS_unbind_done
 *			Called when the UDI_DMGMT_UNBIND operation has removed
 *			the parent-bind channel. The OS-specific code should
 *			release any bindings instantiated by giomap_OS_bind_done
 *		giomap_OS_io_done
 *			called on completion of a udi_gio_xfer_req for both
 *			successful and unsuccessful completion cases
 *		giomap_OS_abort_ack
 *			called on completion of a udi_gio_abort_req
 *		giomap_OS_channel_event
 *			called on receipt of a udi_channel_event_ind
 *		giomap_OS_event
 *			called on receipt of a udi_gio_event_ind
 *		giomap_OS_Alloc_resources
 *			allocate all internal resources required by the mapper.
 *			This will be called once the initial MA binding is
 *			established but before the meta-specific binding has
 *			been done. This is an asynchronous routine which must
 *			call giomap_Resources_Alloced on completion.
 *		giomap_OS_Free_resources
 *			free-up all internal resources allocated by 
 *			giomap_OS_Alloc_resources. This will be called prior
 *			to the MA-specific unbind completing. 
 *			This routine is synchronous.
 *
 *	Provided OS-specific interfaces:
 *		giomap_Resources_Alloced
 *			Called when all OS-specific resources (control blocks,
 *			buffers etc.) have been allocated.
 * ========================================================================
 */

#include "giomap.h"

#ifdef DEBUG
#define	GIOMAP_DEBUG
#endif /* DEBUG */


/*
 * Queue'able structure for keeping track of the available control blocks for
 * use by the data transfer operations
 */
typedef enum { Biostart, Ioctl } giomap_elem_t;

typedef enum { Diag, Xfer } giomap_cb_t;

typedef struct giomap_queue_elem {
	udi_queue_t Q;
	udi_cb_t *cbp;			/* CB attached to this element */
	_OSDEP_EVENT_T event;		/* per element event */
	udi_gio_xfer_cb_t *rw_cb;	/* CB for Read/Write ops */
	udi_gio_xfer_cb_t *diag_cb;	/* CB for Diagnostic ops */
	giomap_cb_t cb_type;		/* Type of CB referenced by cbp */
	giomap_buf_t *buf_p;
	giomap_uio_t *uio_p;
	udi_status_t status;		/* Request status */
	giomap_elem_t type;		/* Type of queue element */
	union {
		giomap_buf_t buf;	/* originating biostart request */
		giomap_uio_t uio;	/* originating ioctl request */
	} u;
	void *kernbuf;			/* ioctl() user mapping buffer */
	udi_ubit32_t prev_count;	/* Previous u_count value */
	udi_boolean_t single_xfer;	/* Set if xfer fits within kernbuf */
	udi_size_t xfer_len;		/* Length of transfer for this CB */
} giomap_queue_t;

/*
 * Queue sentinel definitions
 */
typedef struct giomap_queue_head {
	udi_queue_t Q;
	udi_ubit32_t numelem;
} giomap_q_hd_t;

/*
 * Region-local data
 */
typedef struct giomap_region_data {
	udi_init_context_t init_context;
	udi_gio_bind_cb_t *my_bind_cb;

	/*
	 * General purpose allocation request queue 
	 */
	giomap_q_hd_t alloc_q;		/* Allocation requests */

	/*
	 * udi_gio_xfer_req queue 
	 */
	_OSDEP_MUTEX_T xfer_lock;	/* xfer queue mutex */
	_OSDEP_EVENT_T xfer_q_event;	/* Queue access conditional */
	giomap_q_hd_t xfer_q;		/* Queue for xfer_cb requests */
	giomap_q_hd_t xfer_inuse_q;	/* transfer reqs outstanding */

	/*
	 * Resource request bitmasks 
	 */
	udi_ubit32_t resources;		/* Allocated resources bitmap */
	udi_ubit32_t resource_rqst;	/* Requested resources bitmap */

	/*
	 * Maximum buffer size [OS specific] 
	 */
	udi_size_t giomap_bufsize;

	/*
	 * Parent driver-specific values 
	 */

	udi_buf_path_t buf_path;
	udi_ubit32_t dev_size_hi;	/* Device size [bytes]        */
	udi_ubit32_t dev_size_lo;	/* Device size [bytes]        */

	/*
	 * Reverse Link for OS-driver use 
	 */
	udi_ubit32_t channel;		/* Device instance            */
} giomap_region_data_t;

/*
 * Maximum number of control blocks to allocate for both biostart() and
 * ioctl() originated requests
 */
#define	GIOMAP_MAX_CBS	10		/* Arbitrary value */

/*
 * Resource bitmask definitions
 */
#define	GiomapR_Req_Mem		0x0001	/* Elements for Transfer Request use */
#define	GiomapR_Abort_Mem	0x0002	/* Elements for Abort Request use */

/*
 * -------------------- End of Data Structure Definitions ---------------------
 */
#ifndef STATIC
#define STATIC static
#endif

#define UDI_GIO_INTERFACE	1

#define UDI_GIO_BIND_CB_IDX		1
#define UDI_GIO_XFER_CB_RW_IDX		2	/* udi_gio_rw_params_t */
#define	UDI_GIO_XFER_CB_DIAG_IDX	3	/* udi_gio_diag_params_t */
#define UDI_GIO_ABORT_CB_IDX		4
#define UDI_GIO_EVENT_CB_IDX		5

/*
 * Management Meta declarations:
 */
STATIC udi_devmgmt_req_op_t giomap_devmgmt_req;
STATIC udi_final_cleanup_req_op_t giomap_final_cleanup_req;

/*
 * GIO Meta declarations:
 */
STATIC udi_channel_event_ind_op_t giomap_gio_channel_event;
STATIC void giomap_gio_bind_ack(udi_gio_bind_cb_t *,
				udi_ubit32_t,
				udi_ubit32_t,
				udi_status_t);
STATIC void giomap_gio_unbind_ack(udi_gio_bind_cb_t *);
STATIC void giomap_gio_xfer_ack(udi_gio_xfer_cb_t *);
STATIC void giomap_gio_xfer_nak(udi_gio_xfer_cb_t *,
				udi_status_t);
STATIC void giomap_gio_event(udi_gio_event_cb_t *);

/* STATIC */ udi_mgmt_ops_t giomap_mgmt_ops = {
	udi_static_usage,		/* usage_ind */
	udi_enumerate_no_children,	/* enumerate */
	giomap_devmgmt_req,		/* devmgmt_req */
	giomap_final_cleanup_req,	/* final_cleanup_req */
};

STATIC udi_gio_client_ops_t giomap_gio_ops = {
	giomap_gio_channel_event,
	giomap_gio_bind_ack,
	giomap_gio_unbind_ack,
	giomap_gio_xfer_ack,
	giomap_gio_xfer_nak,
	giomap_gio_event
};

/*
 * default set of flags
 */
STATIC const udi_ubit8_t giomap_default_op_flags[] = {
	0, 0, 0, 0, 0, 0
};

/*
 * Driver-specific module initialization.
 *
 * Tasks for this operation:
 *      + Register the interface ops vectors for management and gio
 *        metalanguage
 *      + Register info for the creation of a region per driver instance.
 *      + Set up for the creation of bind channels with parents.
 *
 * Sleaze Alert:
 *	The user can specify arbitrary tr_params structures, and we need to
 *	support these cleanly. At the moment we only support rw_params_t and
 *	diag_params_t. This allows the user to specific diagnostic specific
 *	parameters, but not to specify a layout for them [yet].
 *	This needs to be fixed.
 */
#if 0
void
init_module(void)
{
	udi_layout_t xfer_rw_layout[] = { UDI_DL_UBIT32_T, UDI_DL_UBIT32_T,
		UDI_DL_UBIT16_T, UDI_DL_END
	};
	udi_layout_t xfer_diag_layout[] = { UDI_DL_UBIT8_T, UDI_DL_UBIT8_T,
		UDI_DL_ARRAY,
		UDI_GIO_MAX_PARAMS_SIZE - 2,
		UDI_DL_UBIT8_T, UDI_DL_END,
		UDI_DL_END
	};

	/*
	 * Register interface ops for gio metalanguage.
	 */
	udi_gio_client_ops_init(UDI_GIO_INTERFACE, &giomap_gio_ops);

	/*
	 * Register GIO control block properties 
	 */
	udi_gio_bind_cb_init(UDI_GIO_BIND_CB_IDX, sizeof (udi_gio_bind_cb_t));
	udi_gio_xfer_cb_init(UDI_GIO_XFER_CB_RW_IDX,
			     sizeof (udi_gio_xfer_cb_t),
			     sizeof (udi_gio_rw_params_t), xfer_rw_layout);

	udi_gio_xfer_cb_init(UDI_GIO_XFER_CB_DIAG_IDX,
			     sizeof (udi_gio_xfer_cb_t),
			     sizeof (udi_gio_diag_params_t) +
			     sizeof (udi_ubit8_t) * UDI_GIO_MAX_PARAMS_SIZE -
			     2, xfer_diag_layout);


	udi_gio_abort_cb_init(UDI_GIO_ABORT_CB_IDX,
			      sizeof (udi_gio_abort_cb_t));
	udi_gio_event_cb_init(UDI_GIO_EVENT_CB_IDX,
			      sizeof (udi_gio_event_cb_t));

	/*
	 * Initialize primary region properties
	 */
	udi_primary_region_init(&giomap_mgmt_ops,
				NULL,
				0,
				UDI_GIO_INTERFACE,
				0, sizeof (giomap_region_data_t), NULL);
}
#else

static udi_layout_t xfer_rw_layout[] = { UDI_DL_UBIT32_T, UDI_DL_UBIT32_T,
	UDI_DL_UBIT16_T, UDI_DL_END
};
static udi_layout_t xfer_diag_layout[] = { UDI_DL_UBIT8_T, UDI_DL_UBIT8_T,
	UDI_DL_ARRAY,
	UDI_GIO_MAX_PARAMS_SIZE - 2,
	UDI_DL_UBIT8_T, UDI_DL_END,
	UDI_DL_END
};


static udi_primary_init_t gio_primary_init = {
	&giomap_mgmt_ops,
	giomap_default_op_flags,	/* op_flags */
	0,				/* mgmt scratch */
	0,				/* enum list length */
	sizeof (giomap_region_data_t),	/* rdata size */
	0,				/* child data size */
	1,				/* buf path */
};

static udi_ops_init_t gio_ops_init_list[] = {
	{
	 UDI_GIO_INTERFACE,
	 1,				/* FIXME: meta_idx */
	 UDI_GIO_CLIENT_OPS_NUM,
	 0,				/* chan context size */
	 (udi_ops_vector_t *)&giomap_gio_ops,
	 giomap_default_op_flags	/* op_flags */
	 },
	{
	 0				/* List terminator */
	 }
};
static udi_cb_init_t gio_cb_init_list[] = {
	{
	 UDI_GIO_BIND_CB_IDX,
	 1,				/* FIXME: meta_idx */
	 UDI_GIO_BIND_CB_NUM,		/* meta cb num */
	 sizeof (udi_gio_bind_cb_t)	/* scratch req */
	 },
	{
	 UDI_GIO_XFER_CB_RW_IDX,
	 1,				/* FIXME: meta_idx */
	 UDI_GIO_XFER_CB_NUM,		/* meta cb num */
	 sizeof (udi_gio_xfer_cb_t),	/* scratch req */
	 sizeof (udi_gio_rw_params_t),	/* inline size */
	 xfer_rw_layout			/* inline layout */
	 },
	{
	 UDI_GIO_XFER_CB_DIAG_IDX,
	 1,				/* FIXME: meta_idx */
	 UDI_GIO_XFER_CB_NUM,		/* meta cb num */
	 sizeof (udi_gio_xfer_cb_t),	/* scratch req */
	 sizeof (udi_gio_diag_params_t) + sizeof (udi_ubit8_t) + UDI_GIO_MAX_PARAMS_SIZE - 2,	/* inline size */
	 xfer_diag_layout		/* inline layout */
	 },
	{
	 UDI_GIO_EVENT_CB_IDX,
	 1,				/* FIXME: meta_idx */
	 UDI_GIO_EVENT_CB_NUM,		/* meta cb num */
	 sizeof (udi_gio_event_cb_t),	/* scratch req */
	 },
	{
	 0				/* Terminator */
	 }
};


udi_gcb_init_t gio_gcb_init_list[] = { {0, 0} };
udi_cb_select_t gio_cb_select_list[] = { {0, 0} };

udi_init_t udi_init_info = {
	&gio_primary_init,
	NULL,				/* secondary init list */
	gio_ops_init_list,
	gio_cb_init_list,
	gio_gcb_init_list,
	gio_cb_select_list
};

#endif



STATIC void
giomap_Resources_Alloced(giomap_region_data_t * rdata)
{
	/*
	 * Send gio metalanguage specific bind request to the driver.
	 */
	/*
	 * TODO: Handle allocation failure 
	 */
	udi_gio_bind_req(rdata->my_bind_cb);
}


/*
 * UDI entry point for udi_devmgmt_req
 */
STATIC void
giomap_devmgmt_req(udi_mgmt_cb_t *cb,
		   udi_ubit8_t mgmt_op,
		   udi_ubit8_t parent_id)
{
	giomap_region_data_t *rdata = UDI_GCB(cb)->context;
	udi_ubit8_t flags = 0;

	switch (mgmt_op) {
	case UDI_DMGMT_PREPARE_TO_SUSPEND:
	case UDI_DMGMT_SUSPEND:
		flags = UDI_DMGMT_NONTRANSPARENT;
		break;
	case UDI_DMGMT_UNBIND:
		/*
		 * Unbind from parent 
		 */
		/*
		 * Completion handled in udi_gio_unbind_ack... 
		 */
		UDI_GCB(rdata->my_bind_cb)->initiator_context = cb;
		udi_gio_unbind_req(rdata->my_bind_cb);
		return;
	default:
		break;
	}

	udi_devmgmt_ack(cb, flags, UDI_OK);
}

/*
 * UDI entry point for udi_final_cleanup_req
 */
STATIC void
giomap_final_cleanup_req(udi_mgmt_cb_t *cb)
{
	giomap_region_data_t *rdata = UDI_GCB(cb)->context;

	/*
	 * We are about to disappear, the MA should release the region-local
	 * data (after all it allocated it), but we need to release any
	 * data which we allocated and is not covered by the tear-down
	 * process. 
	 * This is typically OS-specific data allocated (or instantiated)
	 * by giomap_OS_init(). We just call giomap_OS_deinit to free this
	 * data up.
	 */

	giomap_OS_deinit(rdata);
	udi_final_cleanup_ack(cb);
}

/*
 * UDI entry point for gio_bind_ack
 */
STATIC void
giomap_gio_bind_ack(udi_gio_bind_cb_t *gio_bind_cb,
		    udi_ubit32_t device_size_lo,
		    udi_ubit32_t device_size_hi,
		    udi_status_t status)
{
	giomap_region_data_t *rdata =
		(giomap_region_data_t *) UDI_GCB(gio_bind_cb)->context;
	udi_channel_event_cb_t *ev_cb =
		UDI_GCB(gio_bind_cb)->initiator_context;

	rdata->my_bind_cb = gio_bind_cb;
	if (status != UDI_OK) {
		udi_channel_event_complete(ev_cb, UDI_STAT_CANNOT_BIND);
		return;
	}

	/*
	 * The OS code might have a use for these. 
	 */
	rdata->dev_size_hi = device_size_hi;
	rdata->dev_size_lo = device_size_lo;

	giomap_OS_bind_done(ev_cb, status);

}

/*
 * UDI entry point for gio_xfer_ack
 */
STATIC void
giomap_gio_xfer_ack(udi_gio_xfer_cb_t *gio_xfer_cb)
{
	giomap_OS_io_done(gio_xfer_cb, UDI_OK);
}

/*
 * UDI entry point for gio_xfer_nak
 */
STATIC void
giomap_gio_xfer_nak(udi_gio_xfer_cb_t *gio_xfer_cb,
		    udi_status_t status)
{
	giomap_OS_io_done(gio_xfer_cb, status);
}

/*
 * UDI entry point for gio_unbind_ack
 */
STATIC void
giomap_gio_unbind_ack(udi_gio_bind_cb_t *gio_bind_cb)
{
	giomap_region_data_t *rdata = UDI_GCB(gio_bind_cb)->context;
	udi_cb_t *cb = UDI_GCB(gio_bind_cb)->initiator_context;

	/*
	 * Release any OS-specific bindings that are still present 
	 */
	giomap_OS_unbind_done(gio_bind_cb);

	udi_cb_free(UDI_GCB(gio_bind_cb));

	/*
	 * Release this region's local OS-specific data 
	 */
	giomap_OS_Free_Resources(rdata);

	/*
	 * Complete the devmgmt UNBIND operation 
	 */
	udi_devmgmt_ack(UDI_MCB(cb, udi_mgmt_cb_t),
			0,
			UDI_OK);
}


/*
 * UDI entry point for gio_event
 */
STATIC void
giomap_gio_event(udi_gio_event_cb_t *cb)
{
	/*
	 * Call out to OS specific event handling code. When it has completed
	 * the event handling it will call udi_gio_event_res() to return the
	 * CB to the provider.
	 */
	giomap_OS_event(cb);
}

/*
 * UDI entry point for gio_channel_closed, constraints changes etc.
 * Handle initial allocation for UDI_CHANNEL_BOUND case, all other events
 * are handled by the OS-specific channel_event code.
 * NOTE: Abort requests are handled by the OS-specific mapper code.
 */
STATIC void
giomap_gio_channel_event(udi_channel_event_cb_t *cb)
{
	giomap_region_data_t *rdata = UDI_GCB(cb)->context;

	switch (cb->event) {
	case UDI_CHANNEL_BOUND:
		/*
		 * Handle bind_to_parent functionality 
		 */

		rdata->my_bind_cb = UDI_MCB(cb->params.parent_bound.bind_cb,
					    udi_gio_bind_cb_t);
		UDI_GCB(rdata->my_bind_cb)->initiator_context = cb;
		rdata->buf_path = *cb->params.parent_bound.path_handles;
		/*
		 * Perform OS-specific region-local data initialisation 
		 * (synchronous) 
		 */
		giomap_OS_init(rdata);

		/*
		 * Allocate os resources
		 */
		giomap_OS_Alloc_Resources(rdata);

		break;
	default:
		giomap_OS_channel_event(cb);
		break;
	}
}
