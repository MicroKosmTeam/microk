
/*
 * File:  pseudo.c
 *
 * UDI Prototype Pseudo Driver.   Generates and consumes data streams of
 * test patterns to allow testing with no hardware and no physio.
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

#include <udi.h>			/* Standard UDI definitions */
#include "pseudo.h"

#define PSEUDO_GIO_INTERFACE	1	/* Ops index for GIO entry points */
#define PSEUDO_GIO_XFER_CB_IDX	1	/* Generic xfer  blk for udi_gcb_init */

#define	GIO_BUF_BUG

/*
 * Data Logging flag.  Default: off.   Can be turned on in udiprops.txt.
 * Controls display (ascii hex dump) and sizes of data packets read/written.
 */
#ifndef PSEUDO_VERBOSE
#  define PSEUDO_VERBOSE  0
#endif

/*
 * Management Metalanguage/Pseudo Driver entry points
 */

static udi_enumerate_req_op_t pseudo_enumerate_req;
static udi_devmgmt_req_op_t pseudo_devmgmt_req;
static udi_final_cleanup_req_op_t pseudo_final_cleanup_req;

static udi_mgmt_ops_t pseudo_mgmt_ops = {
	udi_static_usage,
	pseudo_enumerate_req,
	pseudo_devmgmt_req,
	pseudo_final_cleanup_req
};

/*
 * GIO/Pseudo Driver entry points
 */

static udi_channel_event_ind_op_t pseudo_gio_channel_event;
static udi_gio_bind_req_op_t pseudo_gio_bind_req;
static udi_gio_unbind_req_op_t pseudo_gio_unbind_req;
static udi_gio_xfer_req_op_t pseudo_gio_xfer_req;

static udi_gio_provider_ops_t pseudo_gio_provider_ops = {
	pseudo_gio_channel_event,
	pseudo_gio_bind_req,
	pseudo_gio_unbind_req,
	pseudo_gio_xfer_req,
	udi_gio_event_res_unused
};


/*
 * none of my channel ops need any special flags so use this one for all
 */
static const udi_ubit8_t pseudo_default_op_flags[] = {
	0, 0, 0, 0, 0 };
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

static udi_primary_init_t pseudo_primary_init = {
	&pseudo_mgmt_ops,
	pseudo_default_op_flags,
	0,				/* mgmt scratch */
	0,				/* enumerate no attributes */
	sizeof (pseudo_region_data_t),
	0,				/*child_data_size */
	0,		/*buf path */
};

static udi_ops_init_t pseudo_ops_init_list[] = {
	{
	 PSEUDO_GIO_INTERFACE,
	 PSEUDO_GIO_META,		/* meta index */
	 UDI_GIO_PROVIDER_OPS_NUM,	/* meta ops num */
	 0,				/* chan context size */
	 (udi_ops_vector_t *) & pseudo_gio_provider_ops,	/* ops vector */
	 pseudo_default_op_flags
	 },
	{
	 0}
};

static udi_layout_t xfer_layout[] = { UDI_DL_UBIT32_T, UDI_DL_UBIT32_T,
	UDI_DL_UBIT16_T, UDI_DL_END
};

static udi_cb_init_t pseudo_cb_init_list[] = {
	{
	 PSEUDO_GIO_XFER_CB_IDX,
	 PSEUDO_GIO_META,		/* GIO meta idx [from udiprops.txt] */
	 UDI_GIO_XFER_CB_NUM,		/* meta cb_num */
	 0,				/* scratch req */
	 sizeof (udi_gio_rw_params_t),	/* inline size */
	 xfer_layout}
	,
	{
	 0}
};

udi_init_t udi_init_info = {
	&pseudo_primary_init,
	NULL,				/* Secondary init list */
	pseudo_ops_init_list,
	pseudo_cb_init_list,
	NULL,				/* gcb init list */
	NULL				/* cb select list */
};

static void
pseudo_devmgmt_req(udi_mgmt_cb_t * cb,
		   udi_ubit8_t mgmt_op,
		   udi_ubit8_t parent_id)
{
	switch (mgmt_op) {
	default:
		udi_debug_printf("pseudo_devmgmt_req (%d)\n", mgmt_op);
		udi_devmgmt_ack(cb, 0, UDI_OK);
	}
}

static void
pseudo_enumerate_req(
	udi_enumerate_cb_t *cb,
	udi_ubit8_t enum_level)
{
	udi_ubit8_t result;

	/*
	 * We don't enumerate any attributes; the meta alone should be
	 * enough to bind to the mapper.
	 */
	cb->attr_valid_length = 0;

	switch (enum_level) {
	case UDI_ENUMERATE_START:
	case UDI_ENUMERATE_START_RESCAN:
		result = UDI_ENUMERATE_OK;
		break;

	case UDI_ENUMERATE_NEXT:
		result = UDI_ENUMERATE_DONE;
		break;

	case UDI_ENUMERATE_RELEASE:
		/* release resources for the specified child */
		result = UDI_ENUMERATE_RELEASED;
		break;

	case UDI_ENUMERATE_NEW:
	case UDI_ENUMERATE_DIRECTED:
	default:
		result = UDI_ENUMERATE_FAILED;
	}
	udi_enumerate_ack(cb, result, PSEUDO_GIO_INTERFACE);
}




static udi_log_write_call_t cleanup_req_logged_cb;

static void
pseudo_final_cleanup_req(udi_mgmt_cb_t * cb)
{
	udi_log_write(cleanup_req_logged_cb, UDI_GCB(cb),
		      UDI_TREVENT_LOCAL_PROC_ENTRY, UDI_LOG_INFORMATION, 0,
		      UDI_OK, 1500);
}

static void
cleanup_req_logged_cb(udi_cb_t *gcb,
		      udi_status_t correlated_status)
{
	udi_mgmt_cb_t *cb = UDI_MCB(gcb, udi_mgmt_cb_t);

	/*
	 * TODO: Release any data which was allocated and not freed by other
	 * tear-down processing
	 */
	udi_final_cleanup_ack(cb);
}

/*
 *------------------------------------------------------------------------------
 * GIO operations
 *------------------------------------------------------------------------------
 */
static void
pseudo_gio_channel_event(udi_channel_event_cb_t * channel_event_cb)
{
	udi_channel_event_complete(channel_event_cb, UDI_OK);
}

static udi_mem_alloc_call_t pseudo_gio_bind_req_2;

static void
pseudo_gio_bind_req(udi_gio_bind_cb_t * gio_bind_cb)
{
	/*
	 * Fake up some constraints so that the GIO mapper can call 
	 * UDI_BUF_ALLOC safely
	 */
	gio_bind_cb->xfer_constraints.udi_xfer_max = 0;
	gio_bind_cb->xfer_constraints.udi_xfer_typical = 0;
	gio_bind_cb->xfer_constraints.udi_xfer_granularity = 1;
	gio_bind_cb->xfer_constraints.udi_xfer_one_piece = FALSE;
	gio_bind_cb->xfer_constraints.udi_xfer_exact_size = FALSE;
	gio_bind_cb->xfer_constraints.udi_xfer_no_reorder = TRUE;

	/* Get our tx buffers.  */
	udi_mem_alloc(pseudo_gio_bind_req_2, UDI_GCB(gio_bind_cb),
		PSEUDO_BUF_SZ, UDI_MEM_MOVABLE);
}

static void
pseudo_gio_bind_req_2(udi_cb_t *gcb, void *new_mem)
{
	pseudo_region_data_t *rdata = gcb->context;
	rdata->tx_queue = new_mem;

	udi_gio_bind_ack(UDI_MCB(gcb, udi_gio_bind_cb_t), 0, 0, UDI_OK);
}


static void
pseudo_gio_unbind_req(udi_gio_bind_cb_t * gio_bind_cb)
{
	pseudo_region_data_t *rdata = UDI_GCB(gio_bind_cb)->context;
	if (rdata->tx_queue)
		udi_mem_free(rdata->tx_queue);

	udi_gio_unbind_ack(gio_bind_cb);
}

static void
pseudo_buf_written(udi_cb_t *gcb,
		   udi_buf_t *new_buf)
{
	udi_gio_xfer_cb_t *xfer_cb = UDI_MCB(gcb, udi_gio_xfer_cb_t);
	udi_size_t bufsize;

	xfer_cb->data_buf = new_buf;
	/*
	 * TODO: I'm worried that the actual amount of data written to the
	 * <new_buf> isn't easily accessibly. new_buf->buf_size will
	 * return the maximum _valid_ data size which is what was
	 * initially provided by the application. Do we have to double
	 * check this amount against our internal tx_queue[] size ??
	 */
	bufsize = new_buf->buf_size;
#if 0
	if (bufsize < xfer_cb->data_len)
		xfer_cb->data_len = bufsize;
#endif

#if PSEUDO_VERBOSE
	udi_debug_printf("pseudo_buf_written: data_len = %d\n", bufsize);
#endif
	udi_gio_xfer_ack(xfer_cb);
}

/*
 * This is executed via a callback on the cb_alloc in the gio_xfer case
 * where we need to pass data up to the mapper.
 */
static void
pseudo_gio_do_read(udi_gio_xfer_cb_t * gio_xfer_cb)
{
	udi_cb_t *gcb = UDI_GCB(gio_xfer_cb);
	pseudo_region_data_t *rdata = gcb->context;
	udi_size_t len =
		(PSEUDO_BUF_SZ - 1 ) <
		gio_xfer_cb->data_buf->buf_size ? (PSEUDO_BUF_SZ - 
						   1) : gio_xfer_cb->data_buf->

		buf_size;
	udi_size_t i = 0, n;

	/*
	 * Write a simple test pattern into the tx_queue. 
	 */
	while (i < len) {
		n =
			udi_snprintf((char *)&rdata->tx_queue[i], (len - i),
				     "%d ", rdata->testcounter);

		/*
		 * If the entire sprintf buffer wouldn't fit, lop it off 
		 * and issue a partial read.
		 * If the string ends in a space, we know that a complete
		 * number was generated, and thus should not be lopped off.
		 * In other words, the driver will only work correctly all
		 * the time if you read NUMBYTES each time where
		 * NUMBYTES=sprintf(buf,"%d ",-1).
		 * If the driver returns 0 bytes for some read, it is likely
		 * that you are not requesting enough data. Why return 0
		 * instead of a partial buffer? Because it would get
		 * really messy if we stored the state of the
		 * partially written string in the driver for subsequent
		 * reads.
		 */
		i += n;
		if ((n == 0) || (i == len)) {
			if (rdata->tx_queue[i - (i == 0 ? 0 : 1)] != ' ')
				i -= n;
			else
				rdata->testcounter++;
			break;
		}
		rdata->testcounter++;
	}

	/*
	 * Tell the mapper how much data is present. 
	 */
	gio_xfer_cb->data_buf->buf_size = i;

	/*
	 * Let the mapper have the data.   
	 */
#ifdef	GIO_BUF_BUG
	udi_buf_write(pseudo_buf_written, gcb, rdata->tx_queue, i,
		      gio_xfer_cb->data_buf, 0, i, UDI_NULL_BUF_PATH);
#else
	udi_buf_write(pseudo_buf_written, gcb, rdata->tx_queue, i,
		      gio_xfer_cb->data_buf, 0, 0, UDI_NULL_BUF_PATH);
#endif /* GIO_BUF_BUG */
}

static void
pseudo_gio_xfer_req(udi_gio_xfer_cb_t * gio_xfer_cb)
{
	pseudo_region_data_t *rdata = UDI_GCB(gio_xfer_cb)->context;

	switch (gio_xfer_cb->op) {
	case UDI_GIO_DIR_WRITE:{
			/*
			 * From client (application) to provider (this driver) 
			 */
			udi_size_t nread = 0;
			udi_buf_t *buf = (udi_buf_t *)gio_xfer_cb->data_buf;
			udi_ubit8_t *cp;
			udi_ubit32_t src_offset = 0;

#if PSEUDO_VERBOSE
			udi_size_t n;
			udi_ubit32_t iter = 0;

			udi_debug_printf("Size of complete write buffer 0x%x\n",
				   buf->buf_size);
#endif
			while (src_offset < buf->buf_size) {
				nread = sizeof (rdata->rx_queue);	/* Sz of src buffer */
				if (buf->buf_size - src_offset < nread)
					nread = buf->buf_size - src_offset;
				udi_buf_read(buf, src_offset,	/* src offset */
					     nread,	/* Size data to read */
					     &rdata->rx_queue);	/* Destination */
				cp = &rdata->rx_queue[0];
#if PSEUDO_VERBOSE
				udi_debug_printf("\nIteration %d\n"
					   "Size of partial write buffer 0x%x\n",
					   iter++, nread);
#endif
				src_offset += nread;

				/*
				 * At this point, 
				 * rdata->rx_queue[0] - rdata->rx_queue[nread]
				 * holds 'nread' bytes of data passed down from the
				 * user to us.   This would be where we could pass
				 * it to hardware or something.   We'll just print
				 * it in hexdump-ish format.  FIXME: assumes an ASCII
				 * bytestream and encoding.
				 */
#if PSEUDO_VERBOSE
				for (n = 0; n < nread; n++, cp++) {
					udi_ubit8_t c = *cp;

					if (!(n & 15)) {
						udi_debug_printf("\n %04x:  ", n);
					}
					if (((c >= ' ') && (c <= '~')) ||
					    (c == '\t') || (c == '\r') ||
					    (c == '\n')) {
						udi_debug_printf("%c", c);
					} else {
						udi_debug_printf("{%2x}", c);
					}
				}
#endif
			}
			/*
			 * Free the buffer locally so the GIO client (mapper)  
			 * doesn't have to. 
			 */
			udi_buf_free(gio_xfer_cb->data_buf);
			gio_xfer_cb->data_buf = NULL;
			udi_gio_xfer_ack(gio_xfer_cb);
		}
		break;

		/*
		 * From provider (us) to client (application) 
		 */
	case UDI_GIO_DIR_READ:
		pseudo_gio_do_read(gio_xfer_cb);
		break;

		/*
		 * If the user specified anything else, (including a 
		 * UDI_GIO_DIR_READ|UDI_GIO_DIR_WRITE), surrender 
		 */
	default:
		udi_gio_xfer_nak(gio_xfer_cb, UDI_STAT_NOT_UNDERSTOOD);
		break;
	}
}





