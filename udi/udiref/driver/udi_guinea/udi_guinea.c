/*
 * ========================================================================
 * File:  udi_guinea.c
 * Descr: UDI Guinea Pig Driver
 *
 * Related files:
 *      udi.h            - UDI services public definitions
 * ------------------------------------------------------------------------
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
 *
 */

#define UDI_VERSION 0x101
#define	UDI_PHYSIO_VERSION 0x101

#include <udi.h>         	/* Standard UDI definitions */
#include <udi_physio.h>		/* Physical I/O Extensions */
#include "udi_guinea.h"

#define GUINEA_GIO_INTERFACE	1	/* Ops index for GIO entry points */
#define	GUINEA_BUS_INTERFACE	2	/* Ops index for bus entry points */
#define GUINEA_GIO_RX_CLIENT_INTERFACE	3
#define GUINEA_GIO_TX_CLIENT_INTERFACE	4	
#define GUINEA_GIO_RX_PROVIDER_INTERFACE	5	
#define GUINEA_GIO_TX_PROVIDER_INTERFACE	6	

#define GUINEA_GIO_XFER_CB_IDX	1	/* Generic xfer  blk for udi_gcb_init */
#define	GUINEA_BUS_BIND_CB_IDX	3
#define GUINEA_GIO_BIND_CB_IDX  10

#define GUINEA_GCB_IDX_1 	127	/* Not really used. just for cb tests */
#define	GUINEA_GCB_SIZE_1	432
#define GUINEA_GCB_IDX_2 	255
#define	GUINEA_GCB_SIZE_2	789

#define GUINEA_CHAN_CONTEXT_SIZE sizeof(guinea_chan_context_t)
#define GUINEA_CHILD_CHAN_CONTEXT_SIZE sizeof(guinea_child_chan_context_t)
#define GUINEA_CHAN_CONTEXT_PRIVATE_SIZE 5
#define GUINEA_SCRATCH_SIZE 500
#define GUINEA_SCRATCH_OVERRIDE_SIZE 800

#define GUINEA_XFER_RX 1
#define GUINEA_XFER_TX 2

typedef struct {
	udi_chan_context_t channel_context;
	udi_ubit8_t	private[GUINEA_CHAN_CONTEXT_PRIVATE_SIZE];
} guinea_chan_context_t;

typedef struct {
	udi_child_chan_context_t channel_context;
	udi_ubit8_t private[GUINEA_CHAN_CONTEXT_PRIVATE_SIZE];
} guinea_child_chan_context_t;

typedef struct {
	udi_ubit8_t xfer_type;
} guinea_scratch_t;

#define GUINEA_CHILD_DATA_SIZE sizeof(guinea_child_data_t)

#define	GIO_BUF_BUG

/*
 * Management Metalanguage/Pseudo Driver entry points
 */

static udi_devmgmt_req_op_t guinea_devmgmt_req;
static udi_final_cleanup_req_op_t guinea_final_cleanup_req;
static udi_enumerate_req_op_t guinea_enumerate_req;

static udi_mgmt_ops_t  guinea_mgmt_ops = {
	udi_static_usage,
	guinea_enumerate_req,
	guinea_devmgmt_req,
	guinea_final_cleanup_req
};

static void guinea_gcb_cb_1( udi_cb_t *gcb, udi_cb_t *new_cb );

/*
 * Driver "bottom-side" entry points for the Bus Bridge Metalanguage
 */
static udi_channel_event_ind_op_t	guinea_bus_channel_event;
static udi_bus_bind_ack_op_t		guinea_bus_bind_ack;
static udi_bus_unbind_ack_op_t		guinea_bus_unbind_ack;

static udi_bus_device_ops_t guinea_bus_device_ops = {
	guinea_bus_channel_event,
	guinea_bus_bind_ack,
	guinea_bus_unbind_ack,
	udi_intr_attach_ack_unused,
	udi_intr_detach_ack_unused
};

/*
 * GIO/Pseudo Driver entry points
 */

static udi_channel_event_ind_op_t guinea_gio_channel_event;
static udi_channel_event_ind_op_t guinea_gio_int_channel_event;
static udi_channel_event_ind_op_t guinea_gio_rxtx_channel_event;
static udi_gio_bind_req_op_t guinea_gio_bind_req;
static udi_gio_bind_req_op_t guinea_gio_rxtx_bind_req;
static udi_gio_unbind_req_op_t guinea_gio_unbind_req;
static udi_gio_unbind_req_op_t guinea_gio_rxtx_unbind_req;
static udi_gio_xfer_req_op_t guinea_gio_xfer_req;
static udi_gio_xfer_req_op_t guinea_gio_rx_xfer_req;
static udi_gio_xfer_req_op_t guinea_gio_tx_xfer_req;

static udi_gio_bind_ack_op_t guinea_gio_rx_bind_ack;
static udi_gio_bind_ack_op_t guinea_gio_tx_bind_ack;
static udi_gio_unbind_ack_op_t guinea_gio_unbind_ack;
static udi_gio_xfer_ack_op_t guinea_gio_rx_xfer_ack;
static udi_gio_xfer_ack_op_t guinea_gio_tx_xfer_ack;
static udi_gio_xfer_nak_op_t guinea_gio_rx_xfer_nak;
static udi_gio_xfer_nak_op_t guinea_gio_tx_xfer_nak;

/* interfaces with giomap */
static udi_gio_provider_ops_t  guinea_gio_provider_ops = {
	guinea_gio_channel_event,
	guinea_gio_bind_req,
	guinea_gio_unbind_req,
	guinea_gio_xfer_req,
	udi_gio_event_res_unused
};

/* rx_secondary provider -> primary client */
static udi_gio_client_ops_t  guinea_gio_rx_client_ops = {
	guinea_gio_int_channel_event,
	guinea_gio_rx_bind_ack,
	guinea_gio_unbind_ack,
	guinea_gio_rx_xfer_ack,
	guinea_gio_rx_xfer_nak,
	udi_gio_event_res_unused
};

/* tx_secondary provider -> primary client */
static udi_gio_client_ops_t  guinea_gio_tx_client_ops = {
	guinea_gio_int_channel_event,
	guinea_gio_tx_bind_ack,
	guinea_gio_unbind_ack,
	guinea_gio_tx_xfer_ack,
	guinea_gio_tx_xfer_nak,
	udi_gio_event_res_unused
};

/* primary client -> rxtx_secondary provider  */
static udi_gio_provider_ops_t  guinea_gio_rx_provider_ops = {
	guinea_gio_rxtx_channel_event,
	guinea_gio_rxtx_bind_req,
	guinea_gio_rxtx_unbind_req,
	guinea_gio_rx_xfer_req,
	udi_gio_event_res_unused
};
static udi_gio_provider_ops_t  guinea_gio_tx_provider_ops = {
	guinea_gio_rxtx_channel_event,
	guinea_gio_rxtx_bind_req,
	guinea_gio_rxtx_unbind_req,
	guinea_gio_tx_xfer_req,
	udi_gio_event_res_unused
};

static const udi_ubit8_t guinea_default_op_flags[] = {
	0, 0, 0, 0, 0, 0
};

/*
 * ========================================================================
 * BEGIN: main entry point.
 *
 * Driver-specific module initialization.
 *
 * Tasks for this operation:
 *	+ Initialize primary region properties
 *      + Register the interface ops vectors for each metalanguage interface
 *        used in this driver (module).
 */

static udi_primary_init_t guinea_primary_init = {
	&guinea_mgmt_ops,
	guinea_default_op_flags,	/* op_flags */
	0, /* mgmt scratch */
	0, /* enumerate no children */
	sizeof(guinea_region_data_t),
	GUINEA_CHILD_DATA_SIZE, /*child_data_size*/
	0,	/* buf path */
};

static udi_secondary_init_t guinea_secondary_init_list[] = {
	{
		1,
		sizeof(guinea_region_data_t)
	},
	{
		2,
		sizeof(guinea_region_data_t)
	},
	{
		0
	}
};
	

static udi_ops_init_t guinea_ops_init_list[] = {
	{
		GUINEA_GIO_INTERFACE,
		GUINEA_GIO_META, /* meta index */
		UDI_GIO_PROVIDER_OPS_NUM, /* meta ops num */
		GUINEA_CHILD_CHAN_CONTEXT_SIZE, /* chan context size */
		(udi_ops_vector_t*)&guinea_gio_provider_ops, /* ops vector */
		guinea_default_op_flags	/* op_flags */
	},
	{
		GUINEA_BUS_INTERFACE,
		GUINEA_BUS_META,			/* Bus meta index [from udiprops.txt] */
		UDI_BUS_DEVICE_OPS_NUM,
		GUINEA_CHAN_CONTEXT_SIZE,	/* channel context size */
		(udi_ops_vector_t *)&guinea_bus_device_ops,
		guinea_default_op_flags	/* op_flags */
	},
	{
		GUINEA_GIO_RX_PROVIDER_INTERFACE,
		GUINEA_GIO_META, /* meta index */
		UDI_GIO_PROVIDER_OPS_NUM, /* meta ops num */
		GUINEA_CHILD_CHAN_CONTEXT_SIZE, /* chan context size */
		(udi_ops_vector_t*)&guinea_gio_rx_provider_ops, /* ops vector */
		guinea_default_op_flags	/* op_flags */
	},
	{
		GUINEA_GIO_TX_PROVIDER_INTERFACE,
		GUINEA_GIO_META, /* meta index */
		UDI_GIO_PROVIDER_OPS_NUM, /* meta ops num */
		GUINEA_CHILD_CHAN_CONTEXT_SIZE, /* chan context size */
		(udi_ops_vector_t*)&guinea_gio_tx_provider_ops, /* ops vector */
		guinea_default_op_flags	/* op_flags */
	},
	{
		GUINEA_GIO_RX_CLIENT_INTERFACE,
		GUINEA_GIO_META, /* meta index */
		UDI_GIO_CLIENT_OPS_NUM, /* meta ops num */
		GUINEA_CHILD_CHAN_CONTEXT_SIZE, /* chan context size */
		(udi_ops_vector_t*)&guinea_gio_rx_client_ops, /* ops vector */
		guinea_default_op_flags	/* op_flags */
	},
	{
		GUINEA_GIO_TX_CLIENT_INTERFACE,
		GUINEA_GIO_META, /* meta index */
		UDI_GIO_CLIENT_OPS_NUM, /* meta ops num */
		GUINEA_CHILD_CHAN_CONTEXT_SIZE, /* chan context size */
		(udi_ops_vector_t*)&guinea_gio_tx_client_ops, /* ops vector */
		guinea_default_op_flags	/* op_flags */
	},
	{
		0
	}
};

static udi_layout_t	xfer_layout[] = { UDI_DL_UBIT32_T, UDI_DL_UBIT32_T,
					 UDI_DL_UBIT16_T, UDI_DL_END };

/* A layout with the sole purpose of exercising inline. */
static udi_layout_t	dummy_layout[] = { UDI_DL_UBIT32_T, 
						UDI_DL_INLINE_UNTYPED,
						UDI_DL_UBIT32_T, 
					UDI_DL_END };

static udi_cb_init_t guinea_cb_init_list[] = { 
	{
		GUINEA_GIO_XFER_CB_IDX,
		GUINEA_GIO_META,	/* GIO meta idx [from udiprops.txt] */
		UDI_GIO_XFER_CB_NUM, 	/* meta cb_num */
		GUINEA_SCRATCH_SIZE, 			/* scratch req */
		sizeof(udi_gio_rw_params_t), /* inline size */
		xfer_layout
	},
	{	/* this value should be taken over the first */
		GUINEA_GIO_XFER_CB_IDX+1,
		GUINEA_GIO_META,	/* GIO meta idx [from udiprops.txt] */
		UDI_GIO_XFER_CB_NUM, 	/* meta cb_num */
		GUINEA_SCRATCH_SIZE+1, 			/* scratch req */
		sizeof(udi_gio_rw_params_t), /* inline size */
		xfer_layout
	},
	{
		GUINEA_BUS_BIND_CB_IDX,
		GUINEA_BUS_META,	/* Bus meta idx [from udiprops.txt] */
		UDI_BUS_BIND_CB_NUM,
		GUINEA_SCRATCH_SIZE,			/* scratch */
		0,			/* inline size */
		NULL			/* inline layout */
	},
	{
		GUINEA_BUS_BIND_CB_IDX+1,
		GUINEA_BUS_META,	/* Bus meta idx [from udiprops.txt] */
		UDI_BUS_BIND_CB_NUM,
		GUINEA_SCRATCH_SIZE + 5,			/* scratch */
		0,			/* inline size */
		NULL			/* inline layout */
	},
	{	/*this is the over value */
		GUINEA_BUS_BIND_CB_IDX+2,
		GUINEA_BUS_META,	/* Bus meta idx [from udiprops.txt] */
		UDI_BUS_BIND_CB_NUM,
		GUINEA_SCRATCH_OVERRIDE_SIZE,			/* scratch */
		10240,			/* inline size */
		dummy_layout		/* inline layout */
	},
	{
		GUINEA_BUS_BIND_CB_IDX+3,
		GUINEA_BUS_META,	/* Bus meta idx [from udiprops.txt] */
		UDI_BUS_BIND_CB_NUM,
		GUINEA_SCRATCH_SIZE + 10,			/* scratch */
		0,			/* inline size */
		NULL			/* inline layout */
	},
	{
		GUINEA_GIO_BIND_CB_IDX,
		GUINEA_GIO_META,	/* Bus meta idx [from udiprops.txt] */
		UDI_GIO_BIND_CB_NUM,
		GUINEA_SCRATCH_SIZE + 10,			/* scratch */
		0,			/* inline size */
		NULL			/* inline layout */
	},
	{
		0
	}
};

static udi_cb_select_t guinea_cb_select_list[] ={
	{
		GUINEA_BUS_INTERFACE,
		GUINEA_BUS_BIND_CB_IDX+2
	},
	{
		0
	}
};

static udi_gcb_init_t guinea_gcb_init_list[] = {
	{
		GUINEA_GCB_IDX_1,
		GUINEA_GCB_SIZE_1
	},
	{
		GUINEA_GCB_IDX_2,
		GUINEA_GCB_SIZE_2
	},
	{
		0
	}
};

udi_init_t udi_init_info = {
	&guinea_primary_init,
	guinea_secondary_init_list, /* Secondary init list */
	guinea_ops_init_list,
	guinea_cb_init_list,
	guinea_gcb_init_list,
	guinea_cb_select_list /* cb select list */
};

/* Miscellaneous prototypes. */
guinea_child_data_t * guinea_find_child_data( guinea_region_data_t *,
						udi_ubit32_t );

static void
guinea_devmgmt_req(
	udi_mgmt_cb_t *cb, 
	udi_ubit8_t mgmt_op,
	udi_ubit8_t parent_id)
{
	guinea_region_data_t	*rdata = UDI_GCB(cb)->context;
	

	switch( mgmt_op ) {
	case UDI_DMGMT_UNBIND:
		/* Keep a link back to this CB for use in the ack. */
		rdata->bus_bind_cb->gcb.initiator_context = cb;

		/* Do the metalanguage-specific unbind */
		udi_bus_unbind_req( rdata->bus_bind_cb );
		break;
	default:
		udi_devmgmt_ack(cb, 0, UDI_OK);
	}
}

static void
guinea_final_cleanup_req(udi_mgmt_cb_t *cb)
{
	
	/* TODO: Release any data which was allocated and not freed by other
	 * 	 tear-down processing
	 */
	udi_final_cleanup_ack(cb);
}

/*
 *------------------------------------------------------------------------------
 * GIO operations
 *------------------------------------------------------------------------------
 */
static void
guinea_gio_channel_event(udi_channel_event_cb_t *channel_event_cb)
{
	udi_channel_event_complete(channel_event_cb, UDI_OK);
}

static void
guinea_gio_rxtx_channel_event(udi_channel_event_cb_t *channel_event_cb)
{
	udi_channel_event_complete(channel_event_cb, UDI_OK);
}

static void
guinea_gio_int_channel_event(
	udi_channel_event_cb_t *ev_cb){

	udi_cb_t *gio_cb = ev_cb->params.internal_bound.bind_cb;

	switch(ev_cb->event){
	case UDI_CHANNEL_BOUND:
		/* Take a little diversion to test generic cb allocation */
		gio_cb->initiator_context = ev_cb;
		udi_cb_alloc(guinea_gcb_cb_1, 
				gio_cb, GUINEA_GCB_IDX_1, gio_cb->channel);
		break;
	default:
		udi_channel_event_complete(ev_cb, UDI_OK);
	}
}


static void
guinea_gio_bind_req(udi_gio_bind_cb_t *gio_bind_cb)
{
	udi_gio_bind_ack(gio_bind_cb, 0, 0, UDI_OK);
}

static void
guinea_gio_rxtx_bind_req(udi_gio_bind_cb_t *gio_bind_cb)
{
	guinea_chan_context_t *chan_context = UDI_GCB(gio_bind_cb)->context;
	guinea_region_data_t *rdata = chan_context->channel_context.rdata;

	udi_memset(rdata->buf, 0xda, sizeof(rdata->buf));
	udi_gio_bind_ack(gio_bind_cb, 0, 0, UDI_OK);
}

static void 
guinea_gio_unbind_req(udi_gio_bind_cb_t *gio_bind_cb) 
{
	udi_gio_unbind_ack(gio_bind_cb);
}

static void 
guinea_gio_rxtx_unbind_req(udi_gio_bind_cb_t *gio_bind_cb) 
{
	udi_gio_unbind_ack(gio_bind_cb);
}

static void 
guinea_gio_xfer_req(
	udi_gio_xfer_cb_t *gio_xfer_cb) {

	guinea_chan_context_t *chan_context = UDI_GCB(gio_xfer_cb)->context;
	guinea_region_data_t *rdata = chan_context->channel_context.rdata;
	udi_gio_xfer_cb_t *xfer_cb;
	switch(gio_xfer_cb->op){
	case UDI_GIO_OP_READ:
		xfer_cb = rdata->tx_cb;
		break;
	case UDI_GIO_OP_WRITE:
		xfer_cb = rdata->rx_cb;
		break;
	default:
		udi_gio_xfer_nak(gio_xfer_cb, UDI_STAT_NOT_UNDERSTOOD);
		return;
	}
	UDI_GCB(xfer_cb)->initiator_context = gio_xfer_cb;
	xfer_cb->op = gio_xfer_cb->op;
	xfer_cb->tr_params = gio_xfer_cb->tr_params;
	xfer_cb->data_buf = gio_xfer_cb->data_buf;
	udi_gio_xfer_req(xfer_cb);
}
static void
guinea_buf_written(
	udi_cb_t	*gcb,
	udi_buf_t	*new_buf )
{
	udi_gio_xfer_cb_t	*xfer_cb = UDI_MCB(gcb, udi_gio_xfer_cb_t);
	udi_size_t		bufsize;

	xfer_cb->data_buf = new_buf;
	/* TODO: I'm worried that the actual amount of data written to the
	 *	 <new_buf> isn't easily accessibly. new_buf->buf_size will
	 *	 return the maximum _valid_ data size which is what was
	 *	 initially provided by the application. Do we have to double
	 *	 check this amount against our internal buf[] size ??
	 */
	bufsize = new_buf->buf_size;
#if 0
	if ( bufsize < xfer_cb->data_len )
		xfer_cb->data_len = bufsize;
#endif

#ifdef DEBUG
/*TODO: log */ udi_debug_printf("guinea_buf_written: data_len = %d\n", bufsize);
#endif
	udi_gio_xfer_ack( xfer_cb );
}

/*
 * This is executed via a callback on the cb_alloc in the gio_xfer case
 * where we need to pass data up to the mapper.
 */
static void
guinea_gio_do_read(udi_gio_xfer_cb_t *gio_xfer_cb)
{
	udi_cb_t *gcb = UDI_GCB(gio_xfer_cb);
	guinea_child_chan_context_t *child_context = gcb->context;

	guinea_region_data_t *rdata = child_context->channel_context.rdata;
	udi_size_t len = 
	    (sizeof(rdata->buf) - 1) < gio_xfer_cb->data_buf->buf_size
		? (sizeof(rdata->buf) - 1)
		: gio_xfer_cb->data_buf->buf_size;
	udi_size_t i=0, n;
	udi_memset(child_context->private, GUINEA_CHAN_CONTEXT_PRIVATE_SIZE, GUINEA_CHAN_CONTEXT_PRIVATE_SIZE);
	/* Write a simple test pattern into the buf. */
	while (i<len) {
		n = udi_snprintf((char *) &rdata->buf[i], 
			(len-i), "%d ",  rdata->testcounter);

		/* If the entire sprintf buffer wouldn't fit, lop it off 
		 * and issue a partial read.
		 */
		i += n;
#if 0
		if ( i + n == len )	/* No more space */
#else
		if ( (n == 0) || (i == len) )
#endif
			break;
		rdata->testcounter++;
	}

#if 0
	/* Tell the mapper how much data is present. */
	gio_xfer_cb->data_len = i;
#endif

	/* Let the mapper have the data.   
	 */
#ifdef	GIO_BUF_BUG
	udi_buf_write( guinea_buf_written, gcb,
		       rdata->buf, i, gio_xfer_cb->data_buf,
		       0, i, UDI_NULL_BUF_PATH );
#else
	udi_buf_write( guinea_buf_written, gcb,
		       rdata->buf, i, gio_xfer_cb->data_buf,
		       0, 0, UDI_NULL_BUF_PATH );
#endif	/* GIO_BUF_BUG */
}

static void 
guinea_gio_tx_xfer_req(
	udi_gio_xfer_cb_t *gio_xfer_cb) {
	guinea_gio_do_read(gio_xfer_cb);
}

static void 
guinea_gio_rx_xfer_req(udi_gio_xfer_cb_t *gio_xfer_cb)
{
	guinea_child_chan_context_t *child_context = UDI_GCB(gio_xfer_cb)->context;
	guinea_region_data_t *rdata = child_context->channel_context.rdata;
		/* From client (application) to provider (this driver) */
		udi_size_t nread = 0;
		udi_buf_t *buf = (udi_buf_t *)gio_xfer_cb->data_buf; 
		udi_ubit8_t * cp;
		udi_ubit32_t src_offset = 0;
#ifdef DEBUG
		udi_size_t n;
		udi_ubit32_t iter = 0;
#endif

	udi_memset(UDI_GCB(gio_xfer_cb)->scratch, 0xda, GUINEA_SCRATCH_SIZE+1);
	udi_memset(child_context->private, GUINEA_CHAN_CONTEXT_PRIVATE_SIZE, GUINEA_CHAN_CONTEXT_PRIVATE_SIZE);

#ifdef DEBUG
/*TODO: log */	udi_debug_printf("Size of complete write buffer 0x%x\n", 
			buf->buf_size);
#endif
		while	(buf->buf_size > src_offset) {
			nread = sizeof(rdata->buf);
			if (buf->buf_size-src_offset < nread)
				nread = buf->buf_size-src_offset;
			udi_buf_read(buf, 
				src_offset,	/* src offset */
				nread,		/* Size of incoming data */
				&rdata->buf);	/* Destination */
			cp = &rdata->buf[0];
#ifdef DEBUG
/*TODO: log */	udi_debug_printf("\nIteration %d\nSize of partial write buffer 0x%x\n", 
			iter++, nread);
#endif
			src_offset += nread;
			/* At this point, 
			 *	rdata->buf[0] - rdata->buf[nread]
			 * holds 'nread' bytes of data passed down from the
			 * user to us.   This would be where we could pass
			 * it to hardware or something.   We'll just print
			 * it in hexdump-ish format.  FIXME: assumes an ASCII
			 * bytestream and encoding.
			 */
#ifdef DEBUG
			for( n = 0; n < nread; n++,cp++) {
				udi_ubit8_t c = *cp;
				if (!(n & 15 )) {
					udi_debug_printf("\n %04x:  ", n);
				}
				if (((c >= ' ') && (c <= '~')) || (c == '\t')
					|| (c == '\r') || (c == '\n')) {
					udi_debug_printf("%c", c);
				} else {
					udi_debug_printf("{%2x}", c);
				}
			}
#endif
			nread = buf->buf_size;
		}
		/* Free the buffer locally so the GIO client (mapper)  
		 * doesn't have to. 
		 */
		udi_buf_free(gio_xfer_cb->data_buf); 
		gio_xfer_cb->data_buf = NULL;
		udi_gio_xfer_ack(gio_xfer_cb);
}

static void
guinea_xfer_cb_alloc_cbfn(
	udi_cb_t *gcb,
	udi_cb_t *new_cb){

	guinea_chan_context_t *chan_context = gcb->context;
	guinea_region_data_t *rdata = chan_context->channel_context.rdata;
	guinea_scratch_t *scratch = gcb->scratch;

	switch(scratch->xfer_type){
	case GUINEA_XFER_RX:
		rdata->rx_cb = UDI_MCB(new_cb, udi_gio_xfer_cb_t);
		break;
	case GUINEA_XFER_TX:
		rdata->tx_cb = UDI_MCB(new_cb, udi_gio_xfer_cb_t);
		break;
	default:
		udi_assert(0);
	}
	udi_channel_event_complete(UDI_MCB(gcb->initiator_context, udi_channel_event_cb_t), UDI_OK);
}

static void
guinea_gio_rx_bind_ack(
	udi_gio_bind_cb_t *cb,
	udi_ubit32_t device_size_lo,
	udi_ubit32_t device_size_hi,
	udi_status_t status){

	udi_cb_t *gcb = UDI_GCB(cb);
	guinea_chan_context_t *chan_context = gcb->context;
	guinea_region_data_t *rdata = chan_context->channel_context.rdata;
	guinea_scratch_t *scratch = gcb->scratch;

	rdata->rx_bind_cb = cb;
	scratch->xfer_type = GUINEA_XFER_RX;
	udi_cb_alloc(&guinea_xfer_cb_alloc_cbfn, gcb, GUINEA_GIO_XFER_CB_IDX, gcb->channel);
}
static void
guinea_gio_tx_bind_ack(
	udi_gio_bind_cb_t *cb,
	udi_ubit32_t device_size_lo,
	udi_ubit32_t device_size_hi,
	udi_status_t status){

	udi_cb_t *gcb = UDI_GCB(cb);
	guinea_chan_context_t *chan_context = gcb->context;
	guinea_region_data_t *rdata = chan_context->channel_context.rdata;
	guinea_scratch_t *scratch = gcb->scratch;

	rdata->rx_bind_cb = cb;
	scratch->xfer_type = GUINEA_XFER_TX;
	udi_cb_alloc(&guinea_xfer_cb_alloc_cbfn, gcb, GUINEA_GIO_XFER_CB_IDX, gcb->channel);
}


static void
guinea_gio_unbind_ack(
	udi_gio_bind_cb_t *cb){

	udi_channel_event_cb_t *ce_cb = cb->gcb.initiator_context;
	udi_cb_free(UDI_GCB(cb));
	udi_channel_event_complete(ce_cb, UDI_OK);
}

static void
guinea_gio_rx_xfer_ack(
	udi_gio_xfer_cb_t *cb){

	udi_gio_xfer_cb_t *orig_cb = UDI_GCB(cb)->initiator_context;
	guinea_chan_context_t *chan_context = UDI_GCB(cb)->context;
	guinea_region_data_t *rdata = chan_context->channel_context.rdata;

	orig_cb->op = cb->op;
	orig_cb->tr_params = cb->tr_params;
	orig_cb->data_buf = cb->data_buf;
	rdata->rx_cb = cb;
	udi_gio_xfer_ack(UDI_GCB(cb)->initiator_context);
}

static void
guinea_gio_tx_xfer_ack(
	udi_gio_xfer_cb_t *cb){

	udi_gio_xfer_cb_t *orig_cb = UDI_GCB(cb)->initiator_context;
	guinea_chan_context_t *chan_context = UDI_GCB(cb)->context;
	guinea_region_data_t *rdata = chan_context->channel_context.rdata;

	orig_cb->op = cb->op;
	orig_cb->tr_params = cb->tr_params;
	orig_cb->data_buf = cb->data_buf;
	rdata->tx_cb = cb;
	udi_gio_xfer_ack(UDI_GCB(cb)->initiator_context);
}

static void
guinea_gio_rx_xfer_nak(
	udi_gio_xfer_cb_t *cb,
	udi_status_t status){

	udi_gio_xfer_cb_t *orig_cb = UDI_GCB(cb)->initiator_context;
	guinea_chan_context_t *chan_context = UDI_GCB(cb)->context;
	guinea_region_data_t *rdata = chan_context->channel_context.rdata;

	orig_cb->op = cb->op;
	orig_cb->tr_params = cb->tr_params;
	orig_cb->data_buf = cb->data_buf;
	rdata->rx_cb = cb;
	udi_gio_xfer_nak(UDI_GCB(cb)->initiator_context, status);
}

static void
guinea_gio_tx_xfer_nak(
	udi_gio_xfer_cb_t *cb,
	udi_status_t status){

	udi_gio_xfer_cb_t *orig_cb = UDI_GCB(cb)->initiator_context;
	guinea_chan_context_t *chan_context = UDI_GCB(cb)->context;
	guinea_region_data_t *rdata = chan_context->channel_context.rdata;

	orig_cb->op = cb->op;
	orig_cb->tr_params = cb->tr_params;
	orig_cb->data_buf = cb->data_buf;
	rdata->tx_cb = cb;
	udi_gio_xfer_nak(UDI_GCB(cb)->initiator_context, status);
}

/*
 *------------------------------------------------------------------------------
 * Bus Bridge Interface Ops
 *------------------------------------------------------------------------------
 */

static void
guinea_bus_channel_event(
	udi_channel_event_cb_t	*cb )
{
	udi_bus_bind_cb_t *bus_bind_cb = 
		UDI_MCB( cb->params.parent_bound.bind_cb, udi_bus_bind_cb_t );
	switch (cb->event) {
	case UDI_CHANNEL_BOUND:
		UDI_GCB(bus_bind_cb)->initiator_context = cb;
		UDI_GCB(bus_bind_cb)->channel = UDI_GCB(cb)->channel;
		udi_bus_bind_req(bus_bind_cb);
		break;
	default:
		udi_channel_event_complete(cb, UDI_OK);
	}
}

/*
 * guinea_gcb_cb_[12] simply allocate control blocks via some
 * meta_idx's that were registered via udi_gcb_init.  We allocate two
 * cb's, memset the scratch (to let the bounds checkers prove the right 
 * scratch was allocated) free the cb's, then resume the bind_to_secondary
 * sequence from where we hijacked it.
 */
static void
guinea_cb_alloc_batch(
	udi_cb_t *gcb,
	udi_cb_t *first_cb){

	udi_cb_t *cb;
	udi_gio_bind_cb_t *bind_cb = UDI_MCB(gcb, udi_gio_bind_cb_t);

	cb = (udi_cb_t*)first_cb->initiator_context;
	udi_cb_free(first_cb);
	first_cb = (udi_cb_t*)cb->initiator_context;
	udi_cb_free(cb);
	/*
	 * spec 101 says initiator_context of last cb shall be NULL
	 */
	udi_assert(first_cb->initiator_context == NULL);
	udi_cb_free(first_cb);
	udi_gio_bind_req(bind_cb);
}
static void
guinea_gcb_cb_2(
	udi_cb_t *gcb,
	udi_cb_t *new_cb )
{
	guinea_chan_context_t *chan_context = gcb->context;
	guinea_region_data_t	*rdata = chan_context->channel_context.rdata;

	rdata->gcb2 = new_cb;
	udi_memset(rdata->gcb2->scratch, 0x43, GUINEA_GCB_SIZE_2);
	udi_memset(rdata->gcb1->scratch, 0x89, GUINEA_GCB_SIZE_1);
	udi_cb_free(rdata->gcb1);
	udi_cb_free(rdata->gcb2);
	udi_cb_alloc_batch(guinea_cb_alloc_batch, gcb, GUINEA_GCB_IDX_2,
		3, FALSE, 0, NULL);
}

static void
guinea_gcb_cb_1(
	udi_cb_t *gcb,
	udi_cb_t *new_cb )
{
	guinea_chan_context_t *chan_context = gcb->context;
	guinea_region_data_t	*rdata = chan_context->channel_context.rdata;
	rdata->gcb1 = new_cb;
	udi_cb_alloc(guinea_gcb_cb_2, gcb, GUINEA_GCB_IDX_2, gcb->channel);
}

static void
guinea_bus_bind_ack(
	udi_bus_bind_cb_t	*bus_bind_cb,
	udi_dma_constraints_t	constraints,
	udi_ubit8_t		preferred_endianness,
	udi_status_t		status )
{
	udi_channel_event_cb_t	*channel_event_cb = 
					UDI_GCB(bus_bind_cb)->initiator_context;
	guinea_chan_context_t *chan_context = UDI_GCB(bus_bind_cb)->context;

	guinea_region_data_t	*rdata = chan_context->channel_context.rdata;

	udi_memset(chan_context->private, GUINEA_CHAN_CONTEXT_PRIVATE_SIZE, GUINEA_CHAN_CONTEXT_PRIVATE_SIZE);

	/* As we don't have any hardware we can simply complete the 
	 * UDI_CHANNEL_BOUND event after stashing the bus_bind_cb for the
	 * subsequent unbind()
	 */
	udi_dma_constraints_free(constraints);
	rdata->bus_bind_cb = bus_bind_cb;
	udi_channel_event_complete(channel_event_cb, status);
}

static void
guinea_bus_unbind_ack(
	udi_bus_bind_cb_t	*bus_bind_cb )
{
	udi_mgmt_cb_t *cb = bus_bind_cb->gcb.initiator_context;
	guinea_region_data_t *rdata = UDI_GCB(cb)->context;

	udi_cb_free(UDI_GCB(bus_bind_cb));
	if(rdata->enum_new_cb != NULL){
		udi_enumerate_ack(rdata->enum_new_cb, UDI_ENUMERATE_FAILED, 0);
		rdata->enum_new_cb = NULL;
	}
	udi_devmgmt_ack(cb, 0, UDI_OK);
}

guinea_child_data_t *
guinea_find_child_data(
	guinea_region_data_t *rdata,
	udi_ubit32_t child_id){

	udi_queue_t *elem, *tmp;
	guinea_child_data_t *cdata;

	UDI_QUEUE_FOREACH(&rdata->child_data_list, elem, tmp){
		cdata = UDI_BASE_STRUCT(elem, guinea_child_data_t, Q);
		if(cdata->child_id == child_id){
			UDI_QUEUE_REMOVE(elem);
			return cdata;
		}
	}
	return NULL;
}

/* enumerate 4 children ask MA to restart then only enumerate 3.  this should
test implicite remove and ack of UDI_ENUMERATE_RESCAN */
static void
guinea_enumerate_req(udi_enumerate_cb_t *enum_cb,
	udi_ubit8_t enum_level)
{

	/* if child_data was allocated with the incorrect size
	 * then MALLOC_CHECK should complain
	 */
	guinea_region_data_t	*rdata = UDI_GCB(enum_cb)->context;
	guinea_child_data_t	*cdata = enum_cb->child_data;
	udi_ubit8_t		enum_result;
	udi_index_t		ops_idx = 0;

	switch(enum_level){
	case UDI_ENUMERATE_NEXT:
		if(rdata->hot_added){
			enum_result = UDI_ENUMERATE_DONE;
			break;
		}
		if(rdata->enum_counter == 4){
			udi_queue_t *elem, *tmp;
			rdata->enum_counter++;
			UDI_QUEUE_FOREACH(&rdata->child_data_list, elem, tmp){
				UDI_QUEUE_REMOVE(elem);
				cdata = UDI_BASE_STRUCT(elem, 
					guinea_child_data_t, Q);
				udi_mem_free(cdata);
			}
			UDI_QUEUE_INIT(&rdata->child_data_list);
			enum_result = UDI_ENUMERATE_RESCAN;
			break;
		}
		else if( rdata->enum_counter == 8){
			rdata->enum_counter++;
			enum_result = UDI_ENUMERATE_DONE;
			break;
		}
	case UDI_ENUMERATE_START_RESCAN:
	case UDI_ENUMERATE_START:
		if(rdata->enum_new_cb != NULL){
			udi_enumerate_ack(rdata->enum_new_cb, 
				UDI_ENUMERATE_FAILED, 0);
			rdata->enum_new_cb = NULL;
		}
		if(rdata->enum_counter == 0)
			UDI_QUEUE_INIT(&rdata->child_data_list);
		cdata->child_id = enum_cb->child_ID = rdata->enum_counter++ % 4;
		enum_cb->attr_valid_length = 0;
		ops_idx =1;
		enum_result = UDI_ENUMERATE_OK;
		UDI_ENQUEUE_TAIL(&rdata->child_data_list, &cdata->Q);
		break;
	case UDI_ENUMERATE_RELEASE:
		udi_mem_free(guinea_find_child_data(rdata, enum_cb->child_ID));
		enum_result = UDI_ENUMERATE_RELEASED;
		break;
	case UDI_ENUMERATE_NEW:
		if(rdata->enum_new_cb != NULL)
			udi_enumerate_ack(rdata->enum_new_cb, 
				UDI_ENUMERATE_FAILED, 0);
		rdata->enum_new_cb = enum_cb;
		if(!rdata->hot_added){
			cdata->child_id = enum_cb->child_ID = 5;
			enum_cb->attr_valid_length = 0;
			ops_idx =1;
			enum_result = UDI_ENUMERATE_OK;
			UDI_ENQUEUE_TAIL(&rdata->child_data_list, &cdata->Q);
			rdata->enum_new_cb = NULL;
			rdata->hot_added = TRUE;
			break;
		}
		return;
	default:
		ops_idx = 0;
		enum_result = UDI_ENUMERATE_FAILED;
		break;
	}
	udi_enumerate_ack(enum_cb, enum_result, ops_idx); 
}

/*
 * The sole purpose of this section is to be obnoxious about the clause
 * in UDI Core Specification section 8.3 of the specification reading 
 * "UDI drivers ... must be * strictly conforming freestanding programs in 
 * conformance with ISO/IEC9899:1990.    
 * If the following fails to compile, generates undue warnings, or actually
 * lands in any of the udi_asserts, udibuild is not providing you with a 
 * conforming environment.
 */
void memcpy(void);
void memcpy(void) {udi_assert(0);}

void memzero(void);
void memzero(void) {udi_assert(0);}

void strcpy(void);
void strcpy(void) {udi_assert(0);}

void strcat(void);
void strcat(void) {udi_assert(0);}

void strlen(void);
void strlen(void) {udi_assert(0);}

void bcopy(void);
void bcopy(void) {udi_assert(0);}

void bzero(void);
void bzero(void) {udi_assert(0);}

void errno(void);
void errno(void) {udi_assert(0);}

void asm(void);
void asm(void) {udi_assert(0);}

void alloca(void);
void alloca(void) {udi_assert(0);}

void lbolt(void);
void lbolt(void) {udi_assert(0);}

void main(void) {udi_assert(0);} 

/* Throw in some C99 keywords. */
void inline(void);
void inline(void) {udi_assert(0);}

void restrict(void);
void restrict(void) {udi_assert(0);}

