/*
 * File: mapper/posix/gio/giomap_posix.c
 *
 * Posix test-frame harness for a GIO mapper. Common code is held in
 * ../../mapper/common/gio/giomap.c
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
 * ============================================================================
 *
 * Provided interfaces:
 *
 * Mapper use:
 *	giomap_OS_init
 *	giomap_OS_io_done
 *	giomap_OS_abort_ack
 *	giomap_OS_event
 *
 * System use:
 *	giomap_open
 *	giomap_close
 *	giomap_ioctl
 *	giomap_biostart
 * ============================================================================
 */

#include <udi_env.h>
#include <giomap_posix.h>

#ifndef STATIC
#define	STATIC static
#endif

#define	MY_MIN(a, b)	((a) < (b) ? (a) : (b))

/*
 * Constants
 */
const char *mapper_name = "giomap";		/* Test-frame specific */


/*
 * Forward declarations
 */
struct giomap_region_data;

/*
 *------------------------------------------------------------------------
 * Mapper Interface routines [called by giomap.c]
 *------------------------------------------------------------------------
 */
STATIC void giomap_OS_init( struct giomap_region_data * );
STATIC void giomap_OS_deinit( struct giomap_region_data * );
STATIC void giomap_OS_io_done( udi_gio_xfer_cb_t *, udi_status_t );
STATIC void giomap_OS_Alloc_Resources( struct giomap_region_data * );
STATIC void giomap_OS_Free_Resources( struct giomap_region_data * );
STATIC void giomap_OS_channel_event( udi_channel_event_cb_t * );
STATIC void giomap_OS_bind_done( udi_channel_event_cb_t *, udi_status_t );
STATIC void giomap_OS_unbind_done( udi_gio_bind_cb_t * );
STATIC void giomap_OS_event( udi_gio_event_cb_t * );

/*
 * Code Section
 */

#include <giomap.c>		/* Common mapper code */

/*
 *------------------------------------------------------------------------
 * Local OS-specific mapper routines [not called from giomap.c]
 *------------------------------------------------------------------------
 */
STATIC void giomap_req_enqueue( giomap_queue_t * );
STATIC void giomap_req_release( giomap_queue_t * );
STATIC void giomap_req_buf_cbfn( udi_cb_t *, udi_buf_t * );

STATIC giomap_queue_t *giomap_getQ( giomap_region_data_t *, void *, 
				    giomap_elem_t );
STATIC void giomap_send_req( giomap_queue_t * );
STATIC giomap_queue_t *giomap_buf_enqueue( giomap_region_data_t *,
					giomap_buf_t * );
STATIC void _giomap_kernbuf_cbfn( udi_cb_t *, void * );
STATIC void _giomap_got_alloc_cb( udi_cb_t *, void * );
STATIC void _giomap_got_req_RW_cb( udi_cb_t *, udi_cb_t * );
STATIC void _giomap_got_req_DIAG_cb( udi_cb_t *, udi_cb_t * );
STATIC void _giomap_got_reqmem( udi_cb_t *, void * );
STATIC void _giomap_getnext_rsrc( giomap_region_data_t * );

STATIC void giomap_calc_offsets( udi_ubit32_t , udi_ubit32_t , udi_ubit32_t *,
				 udi_ubit32_t * );


/*
 * ---------------------------------------------------------------------------
 * User interface
 * ---------------------------------------------------------------------------
 */

int 
giomap_open( udi_ubit32_t channel )
{
	giomap_region_data_t	*rdata;

	rdata = _udi_MA_local_data(mapper_name, channel);

	if ( rdata == NULL ) {
		/* Driver hasn't been bound by MA */
		return -1;
	}
	
	return 0;
}

int 
giomap_close( udi_ubit32_t channel )
{
	giomap_region_data_t	*rdata;

	rdata = _udi_MA_local_data(mapper_name, channel);

	if ( rdata == NULL ) {
		return -1;
	}

	return 0;
}

int 
giomap_ioctl( udi_ubit32_t channel, int cmd, void *arg )
{
	giomap_region_data_t	*rdata;
	giomap_uio_t		uio_req, *uio_p;
	udi_gio_xfer_cb_t	*ioc_cb;
	giomap_queue_t		*qelem;
	udi_status_t		status;
	udi_size_t		xfer_len;

	rdata = _udi_MA_local_data(mapper_name, channel);

	if ( rdata == NULL ) {
		return -1;
	}
	
	switch ( cmd ) {
	case UDI_GIO_DATA_XFER:
		/*
		 * Issue udi_gio_data_xfer_req();
		 * wait for udi_gio_data_xfer_ack/nak
		 */
		udi_memcpy( &uio_req, arg, sizeof( giomap_uio_t ) );
		uio_p = &uio_req;

		uio_p->u_resid = uio_p->u_count;
		uio_p->u_count = 0;

		/*
		 * Ensure that tr_param size does not exceed what we've
		 * previously allocated. If it does, we fail the request.
		 * TODO: Need to (maybe) handle dynamic reallocation of
		 *	 xfer_cb size
		 */
		if ( uio_p->tr_param_len > UDI_GIO_MAX_PARAMS_SIZE ) {
			uio_p->u_error = UDI_STAT_NOT_SUPPORTED;
			udi_memcpy( arg, uio_p, sizeof( giomap_uio_t ) );
			return -1;
		}
		
		/*
		 * Validate the operation request for asynchronous i/o
		 * We can only run asynchronously if there is no associated
		 * data transfer to the user application.
		 * Async write()s are supported, as are operations which have
		 * no associated data transfer.
		 * Async read()s will fail with UDI_STAT_NOT_SUPPORTED
		 */
		
		if ( (uio_p->u_async) && (uio_p->u_op & UDI_GIO_DIR_READ) ) {
			uio_p->u_error = UDI_STAT_NOT_SUPPORTED;
			udi_memcpy( arg, uio_p, sizeof( giomap_uio_t ) );
			return -1;
		}

		/* Get a queue element for the request -- may block */
		qelem = giomap_getQ( rdata, uio_p, Ioctl );

		while ( (qelem->status == UDI_OK) &&
			(qelem->uio_p->u_resid > 0 ) ) {

			/* Process request  -- may block */
			giomap_req_enqueue( qelem );

			/*
			 * Bail out if we encountered an error
			 */
			if ( qelem->status != UDI_OK ) {
				break;
			}

			/*
			 * Check to see if we can return an asynchronous
			 * request. This will work if the amount of data will
			 * fit into the qelem's kernel buffer. Otherwise we
			 * have to issue multiple requests and can only
			 * return the async handle when we have sent the last
			 * block to the driver
			 */
			if ( (uio_p->u_async) && (qelem->single_xfer) ) {
				qelem->uio_p->u_handle = qelem;

				/* copy updated request back to user */
				udi_memcpy( arg, qelem->uio_p, 
					    sizeof( giomap_uio_t ));
				return 0;
			}

			/* Copy data out to user space if OP_DIR_READ set */

			ioc_cb = UDI_MCB( qelem->cbp, udi_gio_xfer_cb_t );

			if ( uio_p->u_op & UDI_GIO_DIR_READ ) {
				xfer_len = ioc_cb->data_buf->buf_size;
				udi_buf_read(ioc_cb->data_buf, 0, 
					     xfer_len, 
					     qelem->kernbuf );
				
				udi_memcpy( qelem->uio_p->u_addr, 
					    qelem->kernbuf, 
					    xfer_len );
				
				qelem->uio_p->u_resid -= xfer_len;
				qelem->uio_p->u_count += xfer_len;
				if ( xfer_len == 0 ) {
					/* No more data available from device */
					break;
				}
			} else if ( uio_p->u_op & UDI_GIO_DIR_WRITE ) {
				xfer_len = qelem->xfer_len;
				qelem->uio_p->u_resid -= xfer_len;
				qelem->uio_p->u_count += xfer_len;
			}
		}

		/*
		 * Completed transfer
		 */
		qelem->uio_p->u_error = qelem->status;

		status = qelem->status;

		udi_memcpy( arg, qelem->uio_p, sizeof( giomap_uio_t ) );
		giomap_req_release( qelem );

		return -(status);

#ifdef LATER
	case UDI_GIO_INQUIRY:
		/*
		 * Issue udi_gio_data_xfer_req()
		 * wait for udi_gio_data_xfer_ack/nak
		 */
		_OSDEP_EVENT_WAIT( &rdata->giomap_data_xfer );
		return rdata->last_xfer_status;
#endif
	default:
		return -1;	/* Unsupported command */
	}
}

/*
 * biostart() interface:
 *	Issue a READ or WRITE request.
 *	Writes are expected to complete in one shot, so any outstanding data
 *	will be passed back to the application.
 *
 * NOTE: This routine is called with the user-supplied data already mapped and
 *	 locked into the system address space. This allows us to use the
 *	 supplied buffer addresses as source/destination for the udi_buf_xxxx
 *	 routines without having to perform a mapping operation first.
 */
int 
giomap_biostart( udi_ubit32_t channel, giomap_buf_t *buf_p )
{
	giomap_region_data_t	*rdata;
	udi_gio_xfer_cb_t	*xfer_cb;
	int			amount;
	giomap_queue_t		*qelem;
	udi_size_t		xfer_len;

	rdata = _udi_MA_local_data(mapper_name, channel);

	buf_p->b_resid = buf_p->b_bcount;
	if ( rdata == NULL ) {
		return -1;
	}

	/* Get handle for request */
	qelem = giomap_buf_enqueue(rdata, buf_p);

	/* Wait on the request to complete */
	_OSDEP_EVENT_WAIT( &qelem->event );
	
	/*
	 * Determine how much data (if any) has been transferred
	 */
	xfer_cb = UDI_MCB( qelem->cbp, udi_gio_xfer_cb_t );

	if ( buf_p->b_flags & GIOMAP_B_READ ) {
		/*
		 * Copy new data to application.
		 */
		xfer_len = xfer_cb->data_buf->buf_size;
		udi_buf_read(xfer_cb->data_buf, 0, xfer_len, buf_p->b_addr);
#if 0 || OLDWAY
		buf_p->b_resid = 0;
#endif
	} else {
		xfer_len = qelem->xfer_len;
#if 0 || OLDWAY
		buf_p->b_resid -= xfer_len;
#endif
	}
	buf_p->b_resid -= xfer_len;

	amount = (int)xfer_len;

	/*
	 * Update the residual count and error flags so that the calling
	 * application will get the correct return from the pseudo read/write
	 * call
	 */
	buf_p->b_error = qelem->status;

	/*
	 * Make queue element available for subsequent use
	 */
	giomap_req_release( qelem );

	if ( buf_p->b_error != UDI_OK ) {
		return -(buf_p->b_error);
	} else {
		return amount;
	}
}

/*
 * Support Routines
 */

/*
 * giomap_getQ:
 * -----------
 *	Return a queue element suitable for the given <type> of operation.
 *	The originating request <req_p> will be copied into the per-element
 *	data structure to allow for subsequent asynchronous UDI context
 *	processing to occur.
 *	This routine will block until a queue element is made available
 *
 * Return Value:
 *	Request element which is suitable for use in a udi_gio_xfer_req op.
 */
STATIC giomap_queue_t *
giomap_getQ(
	giomap_region_data_t	*rdata,
	void			*req_p,
	giomap_elem_t		type )
{
	giomap_queue_t	*qelem;
	udi_gio_op_t	opcode;

	_OSDEP_MUTEX_LOCK( &rdata->xfer_lock );
	while ( UDI_QUEUE_EMPTY( &rdata->xfer_q.Q ) ) {
		_OSDEP_EVENT_WAIT( &rdata->xfer_q_event );
	}
	qelem = (giomap_queue_t *)UDI_DEQUEUE_HEAD( &rdata->xfer_q.Q );
	rdata->xfer_q.numelem--;
	_OSDEP_MUTEX_UNLOCK( &rdata->xfer_lock );

	/*
	 * Copy originating request into per-element data structure
	 */
	qelem->type = type;
	qelem->status = UDI_OK;

	switch ( type ) {
	case Biostart:
		/* Kernel-based request */
		udi_memcpy( qelem->buf_p, req_p, sizeof( giomap_buf_t ) );
		break;
	case Ioctl:
		/* User-based request */
		udi_memcpy( qelem->uio_p, req_p, sizeof( giomap_uio_t ) );
		qelem->prev_count = 0;
		/*
		 * Set up the correct CB to use for the request. For ordinary
		 * GIO_OP_READ/GIO_OP_WRITE we use the <rw_cb>;
		 * For GIO_OP_DIAG_RUN_TEST and other ops we use <diag_cb> 
		 * which contains up to UDI_GIO_MAX_PARAMS_SIZE bytes
		 */
		opcode = qelem->uio_p->u_op;
		if ( opcode == UDI_GIO_OP_READ || opcode == UDI_GIO_OP_WRITE ) {
			qelem->cbp = UDI_GCB(qelem->rw_cb);
			qelem->cb_type = Xfer;
		} else {
			qelem->cbp = UDI_GCB(qelem->diag_cb);
			qelem->cb_type = Diag;
		} 
		break;
	}

	return qelem;

}

/*
 * giomap_buf_enqueue:
 * ------------------
 *	Enqueue a udi_gio_xfer_req request based on the passed in user-supplied
 *	parameters. The data has been previously mapped into system space which
 *	allows us to use the addresses as source/destination for the udi_buf_
 *	operations.
 *	We take a queue element from our list of available elements [xfer_q]
 *	and potentially block if the list is empty. This is allowable as we
 *	are running in user context (not UDI context).
 *	Once we have a request block we save the user-supplied data and start
 *	performing UDI channel operations. This puts us firmly into the UDI
 *	context which means we can no longer use _OSDEP_EVENT_WAIT or
 *	_OSDEP_MUTEX_LOCK from this execution thread.
 *
 * Return value:
 *	request element which is being used for the udi_gio_xfer_req operation.
 *	The caller should wait for the per-element event to be signalled before
 *	continuing with the user application thread. This signalling is done by
 *	giomap_OS_io_done()
 */
STATIC giomap_queue_t *
giomap_buf_enqueue(
	giomap_region_data_t	*rdata,		/* Region-local data */
	giomap_buf_t		*buf_p )	/* User request */
{
	udi_gio_xfer_cb_t *xfer_cb;
	udi_boolean_t	   big_enough;
	giomap_queue_t	  *qelem;
	udi_ubit32_t	   amount = buf_p->b_bcount;

	qelem = giomap_getQ( rdata, buf_p, Biostart );

	qelem->cbp = UDI_GCB(qelem->rw_cb);
	qelem->cb_type = Xfer;

	xfer_cb = qelem->rw_cb;

	/*
	 * We've now got a queue element and its associated CB.
	 * Update it to reference the user-supplied request and start the
	 * allocation chain off.
	 *
	 * NOTE: once we start UDI_BUF_ALLOC'ing we have no user state
	 *	 available. This requires us to stash the queue element address
	 *	 in the xfer_cb's initiator context.
	 */
	udi_memcpy( qelem->buf_p, buf_p, sizeof( giomap_buf_t ) );

	if ( buf_p->b_flags & GIOMAP_B_READ ) {
		xfer_cb->op = UDI_GIO_OP_READ;
	} else {
		xfer_cb->op = UDI_GIO_OP_WRITE;
	}

	/*
	 * Determine size of buffer associated with <xfer_cb>
	 * If its large enough to hold the data (buf_p->b_bcount) we don't
	 * need to allocate a new one.
	 */
	if ( xfer_cb->data_buf != NULL ) {
		big_enough = xfer_cb->data_buf->buf_size >= amount;
	} else {
		big_enough = 0;
	}

	qelem->xfer_len = amount;
	xfer_cb->gcb.initiator_context = qelem;	/* Reverse link */

	/*
	 * Obtain a UDI buffer to hold the outgoing / incoming data. The
	 * allocation will instantiate the data to the passed in buffer
	 * contents.
	 */
	
	if ( (xfer_cb->op & (UDI_GIO_DIR_READ|UDI_GIO_DIR_WRITE)) &&
	     (amount > 0) ) {
		if ( xfer_cb->op & UDI_GIO_DIR_READ ) {
			/* Read into buffer */
			if ( !big_enough ) {
				udi_buf_free( xfer_cb->data_buf );
				/* Allocate new buffer */
				UDI_BUF_ALLOC( giomap_req_buf_cbfn, qelem->cbp, 
						NULL, amount, 
						rdata->buf_path);
			} else {
				/* Re-use the existing buffer */
				/*
				 * We have to delete any extraneous bytes from
				 * the buffer so that the buf_size is correctly
				 * updated. As we cannot delete 0 bytes (ahem)
				 * we need to special case this
				 */
				if ( xfer_cb->data_buf->buf_size > amount ) {
				    UDI_BUF_DELETE( giomap_req_buf_cbfn, 
				    	qelem->cbp,
					xfer_cb->data_buf->buf_size - amount,
					xfer_cb->data_buf, 0);
				} else {
				    giomap_req_buf_cbfn( qelem->cbp, 
				    			 xfer_cb->data_buf );
				}
			}
	    	} else {
			xfer_cb->op = UDI_GIO_OP_WRITE;
			/* Write from buffer */
			if ( !big_enough ) {
				udi_buf_free( xfer_cb->data_buf );
				/* Allocate and fill new buffer */
				UDI_BUF_ALLOC( giomap_req_buf_cbfn, qelem->cbp, 
						buf_p->b_addr, buf_p->b_bcount, 
						rdata->buf_path);
			} else {
				/* Shrink buffer size to <amount> */
				xfer_cb->data_buf->buf_size = amount;
				/* Re-use the existing buffer */
				udi_buf_write( giomap_req_buf_cbfn, qelem->cbp,
						buf_p->b_addr, buf_p->b_bcount,
						xfer_cb->data_buf, 0, 
						buf_p->b_bcount,
						UDI_NULL_BUF_PATH);
			}
	    	}
	} else {
		udi_buf_free( xfer_cb->data_buf );
		xfer_cb->data_buf = NULL;
		/* Call parent driver directly... */
		UDI_GCB(xfer_cb)->channel= UDI_GCB(rdata->my_bind_cb)->channel;
		udi_gio_xfer_req( xfer_cb );
	}
	return qelem;
}

/*
 * giomap_req_enqueue:
 * ------------------
 *	Enqueue a udi_gio_xfer_req request based on the passed in user-supplied
 *	parameters. This will be an ioctl()-based request and will require
 *	mapping the data buffers into kernel space [maximum of giomap_bufsize
 *	per transfer]
 *	This routine may be called multiple times to satisfy one user request.
 *	If the u_count field is non-zero, it means we're on a subsequent
 *	iteration.
 *
 *	TODO: handle arbitrary udi_layout_t specifications in a proper manner
 */
STATIC void
giomap_req_enqueue(
	giomap_queue_t	*qelem )		/* Request element */
{
	udi_gio_xfer_cb_t *xfer_cb = UDI_MCB( qelem->cbp, udi_gio_xfer_cb_t );
	udi_ubit32_t	   amount = qelem->uio_p->u_resid;
	udi_gio_rw_params_t *rwparams;
	udi_ubit32_t	   user_addr;
	giomap_region_data_t *rdata = UDI_GCB(xfer_cb)->context;

	GIOMAP_ASSERT( xfer_cb->tr_params != NULL );

	rwparams = xfer_cb->tr_params;
	xfer_cb->op = qelem->uio_p->u_op;
	xfer_cb->gcb.initiator_context = qelem;

	/*
	 * Validate the command bitfields. It is not permissible to have both
	 * UDI_GIO_DIR_READ and UDI_GIO_DIR_WRITE set in the same <op> field
	 */
	if ( (xfer_cb->op ^ 
		( udi_gio_op_t )(UDI_GIO_DIR_READ|UDI_GIO_DIR_WRITE)) ==
		( udi_gio_op_t )~(UDI_GIO_DIR_READ|UDI_GIO_DIR_WRITE)) {
#ifdef DEBUG
		_OSDEP_PRINTF("giomap_req_enqueue: Illegal u_op setting [%x]\n",
			qelem->uio_p->u_op);
#endif
		qelem->status = UDI_STAT_NOT_UNDERSTOOD;
		return;
	}

	/*
	 * Check to see if this request will fit into one transaction. If so,
	 * we don't have to special-case the data buffer mapping
	 */
	qelem->single_xfer = amount <= rdata->giomap_bufsize;

	/*
	 * Copy next block of data into kernel space for UDI_GIO_OP_WRITE
	 * Any other command (with the DIR_WRITE bit set) is unsupported as we
	 * do not know what the tr_params contents are. This makes it impossible
	 * to adjust a device offset to split the request into smaller chunks.
	 */
	if ( !qelem->single_xfer && (xfer_cb->op & UDI_GIO_DIR_WRITE) &&
		(xfer_cb->op != UDI_GIO_OP_WRITE) ) {
#ifdef DEBUG
		_OSDEP_PRINTF("giomap_req_enqueue: Cannot split request op = 0x%02x\n",
			xfer_cb->op);
#endif
		qelem->status = UDI_STAT_NOT_SUPPORTED;
		return;
	}

	/*
	 * Allocate ourselves a buffer of giomap_bufsize bytes to use as a
	 * staging area between user and kernel space. We do this the first
	 * time through if there is data to transfer
	 */
	
	if ( (qelem->uio_p->u_count == 0) && 
	     (xfer_cb->op & (UDI_GIO_DIR_WRITE|UDI_GIO_DIR_READ) ) ) {

		if ( qelem->kernbuf == NULL ) {
			/*
		 	 * Allocate a buffer, the callback will handle the 
			 * initial copy in the GIO_DIR_READ case
		 	 */
			udi_mem_alloc( _giomap_kernbuf_cbfn, qelem->cbp,
					rdata->giomap_bufsize, UDI_MEM_MOVABLE|UDI_MEM_NOZERO );
		} else {
			/*
			 * Re-use the pre-allocated kernbuf
			 */
			_giomap_kernbuf_cbfn( qelem->cbp, qelem->kernbuf );
		}
			
		if ( !qelem->uio_p->u_async || !qelem->single_xfer ) {
			_OSDEP_EVENT_WAIT( &qelem->event );
		} 
		return;
	}

	/*
	 * Now we have to copy in user data for the next [amount] bytes. If
	 * we still have more data than will fit into qelem->kernbuf, we go
	 * through this code again [from ioctl()]
	 * We need to update the tr_params field to make sure we transfer the
	 * data to/from the correct part of the device.
	 */
	if (xfer_cb->op == UDI_GIO_OP_WRITE || xfer_cb->op == UDI_GIO_OP_READ) {
		/* Our offset is giomap_bufsize bytes further on */
		if ( rwparams->offset_lo >= GIOMAP_MAX_OFFSET ) {
			rwparams->offset_lo += qelem->uio_p->u_count -
					       qelem->prev_count;
			rwparams->offset_hi++;
		} else {
			rwparams->offset_lo += qelem->uio_p->u_count -
					       qelem->prev_count;
		}
	}
	/*
	 * Update user source/destination address 
	 */
	user_addr = (udi_ubit32_t )qelem->uio_p->u_addr;
	user_addr += qelem->uio_p->u_count - qelem->prev_count;
	qelem->uio_p->u_addr = (void *)user_addr;

	qelem->prev_count = qelem->uio_p->u_count;

	/*
	 * Update amount to reflect maximum transfer we're going to perform
	 */
	if ( !qelem->single_xfer )
		amount = rdata->giomap_bufsize;

	/*
	 * Copy data into kernel buffer -- does not use UDI buffer scheme as
	 * we're emulating copyin(). The READ data will be copied into the
	 * kernbuf and then copied out to user-space by the ioctl() routine.
	 */
	if ( xfer_cb->op & UDI_GIO_DIR_WRITE ) {
		udi_memcpy( qelem->kernbuf, qelem->uio_p->u_addr, amount );
	}

	/* Now we can stage the request */

	giomap_send_req( qelem );

	if ( !qelem->uio_p->u_async || !qelem->single_xfer ) {
		_OSDEP_EVENT_WAIT( &qelem->event );
	}
	return;
}

/*
 * _giomap_kernbuf_cbfn:
 * --------------------
 *	Called on completion of kernel buffer allocation to hold data which is
 *	destined / sourced from user space. If we are initially writing to the
 *	device we copy the first giomap_bufsize bytes into the newly allocated
 *	buffer. This emulates copyin()
 */
STATIC void
_giomap_kernbuf_cbfn(
	udi_cb_t	*gcb,
	void		*new_mem )
{
	udi_gio_xfer_cb_t	*xfer_cb = UDI_MCB( gcb, udi_gio_xfer_cb_t );
	giomap_queue_t		*qelem = xfer_cb->gcb.initiator_context;
	udi_ubit32_t		amount;
	giomap_region_data_t	*rdata = gcb->context;

	amount = qelem->uio_p->u_resid;
	if ( amount > rdata->giomap_bufsize )
		amount = rdata->giomap_bufsize;

	qelem->kernbuf = new_mem;
	if ( qelem->uio_p->u_op & UDI_GIO_DIR_WRITE ) {
		udi_memcpy( qelem->kernbuf, qelem->uio_p->u_addr, amount );
	}

	/*
	 * Copy the user-specified tr_params over our xfer_cb ones.
	 * The size of the params must be less than the maximum size we
	 * preallocated in udi_gio_xfer_cb_init()
	 */
	if ( qelem->uio_p->tr_param_len ) {
		udi_memcpy( xfer_cb->tr_params, qelem->uio_p->tr_params,
			    qelem->uio_p->tr_param_len );
	}

	/* Issue request to driver */
	giomap_send_req( qelem );
}

/*
 * giomap_send_req:
 * ---------------
 *	Issue a user mapped request to the underlying driver. Write requests
 *	will have their mapped data in qelem->kernbuf. Read requests will
 *	be mapped out to user space by the originating ioctl() call.
 *
 *	The size of the transfer will be a maximum of giomap_bufsize 
 *
 *	NOTE: We must remove the element from its queue to avoid adding the
 *	      same element multiple times to the xfer_inuse_q.  This only
 *	      happens when we split a transfer request into multiple chunks for
 *	      the Ioctl case.
 */
STATIC void
giomap_send_req(
	giomap_queue_t	*qelem )
{
	udi_gio_xfer_cb_t	*xfer_cb;
	udi_boolean_t		big_enough;
	udi_ubit32_t		amount;
	void			*user_addr = qelem->kernbuf;
	giomap_region_data_t	*rdata;
	
	xfer_cb = UDI_MCB( qelem->cbp, udi_gio_xfer_cb_t );
	rdata   = UDI_GCB(xfer_cb)->context;
	
	amount = qelem->uio_p->u_resid;

	if ( amount > rdata->giomap_bufsize )
		amount = rdata->giomap_bufsize;

	/*
	 * Determine size of buffer associated with <xfer_cb>
	 * If its large enough to hold the data (buf_p->b_bcount) we don't
	 * need to allocate a new one.
	 */
	if ( xfer_cb->data_buf != NULL ) {
		big_enough = xfer_cb->data_buf->buf_size >= amount;
	} else {
		big_enough = 0;
	}

	qelem->xfer_len = amount;

	/*
	 * Remove queue element from any active queue _before_ we start any
	 * asynchronous processing
	 */
	UDI_QUEUE_REMOVE( &qelem->Q );

	if ( (xfer_cb->op & (UDI_GIO_DIR_READ|UDI_GIO_DIR_WRITE)) &&
	     (amount > 0) ) {
		if ( xfer_cb->op & UDI_GIO_DIR_READ ) {
			/* Read into buffer */
			if ( !big_enough ) {
				udi_buf_free( xfer_cb->data_buf );
				/* Allocate new buffer */
				UDI_BUF_ALLOC( giomap_req_buf_cbfn, qelem->cbp, 
						NULL, amount, 
						rdata->buf_path);
			} else {
				/* Re-use the existing buffer */
				/*
				 * We have to delete any extraneous bytes from
				 * the buffer so that the buf_size is correctly
				 * updated. As we cannot delete 0 bytes (ahem)
				 * we need to special case this
				 */
				if ( xfer_cb->data_buf->buf_size > amount ) {
				    UDI_BUF_DELETE( giomap_req_buf_cbfn, 
				    	qelem->cbp,
					xfer_cb->data_buf->buf_size - amount,
					xfer_cb->data_buf, 0 );
				} else {
				    giomap_req_buf_cbfn( qelem->cbp, 
				    	xfer_cb->data_buf );
				}
			}
	    	} else {
			/* Write from buffer */
			if ( !big_enough ) {
				udi_buf_free( xfer_cb->data_buf );
				/* Allocate and fill new buffer */
				UDI_BUF_ALLOC( giomap_req_buf_cbfn, qelem->cbp, 
						user_addr, amount, 
						rdata->buf_path );
			} else {
				/* Shrink buffer size to <amount> */
				xfer_cb->data_buf->buf_size = amount;
				/* Re-use the existing buffer */
				udi_buf_write( giomap_req_buf_cbfn, qelem->cbp,
						user_addr, amount,
						xfer_cb->data_buf, 0, amount,
						UDI_NULL_BUF_PATH );
			}
	    	}
	} else {
		udi_buf_free( xfer_cb->data_buf );
		xfer_cb->data_buf = NULL;
		/* Call parent driver directly... */
		UDI_GCB(xfer_cb)->channel = UDI_GCB(rdata->my_bind_cb)->channel;
		udi_gio_xfer_req( xfer_cb );
	}
}

/*
 * giomap_req_buf_cbfn:
 * -------------------
 * 	Callback function for buffer allocation. We have a reference to the
 * 	(unattached) queue element which corresponds to the user originated
 * 	biostart() request.
 * 	We simply place this request on the xfer_inuse_q and pass it down to the
 * 	underlying driver.
 */
STATIC void
giomap_req_buf_cbfn(
	udi_cb_t	*gcb,
	udi_buf_t	*new_buf )
{
	giomap_region_data_t	*rdata = gcb->context;
	udi_gio_xfer_cb_t	*xfer_cb = UDI_MCB( gcb, udi_gio_xfer_cb_t );
	udi_gio_rw_params_t	*rwparams;
	giomap_queue_t		*qelem = xfer_cb->gcb.initiator_context;

	/*
	 * Fill in the remaining fields of the xfer_cb
	 */
	
	xfer_cb->data_buf = new_buf;

	switch( qelem->type ) {
	case Biostart:
		rwparams = xfer_cb->tr_params;
		/* Convert block offset to byte offset */
		giomap_calc_offsets( qelem->buf_p->b_blkno, 
				     qelem->buf_p->b_blkoff,
				     &(rwparams->offset_hi),
				     &(rwparams->offset_lo) );
		GIOMAP_ASSERT( rwparams == xfer_cb->tr_params );
		break;
	case Ioctl:
		/*
		 * The rw_params field has been filled in by the enqueue
		 * function. The user supplies the tr_params for the request
		 * and we only have to update it if the request needs to be
		 * split into multiple chunks
		 */
		break;
	default:
		GIOMAP_ASSERT( (qelem->type == Biostart) || 
				(qelem->type == Ioctl) );
		break;
	}

	/*
	 * Put request on head of inuse elements. This provides a mechanism
	 * for determining what requests need to be aborted
	 */
	UDI_ENQUEUE_HEAD( &rdata->xfer_inuse_q.Q, (udi_queue_t *)qelem );
	rdata->xfer_inuse_q.numelem++;

	UDI_GCB(xfer_cb)->channel = UDI_GCB(rdata->my_bind_cb)->channel;
	udi_gio_xfer_req( xfer_cb );
}

/*
 * giomap_req_release:
 * ------------------
 * 	Release passed in xfer_q element back to the available list for future
 * 	user requests.
 * 	Called from system context and uses no locks. Awakens any blocked user
 * 	request by signalling xfer_event
 */
STATIC void
giomap_req_release(
	giomap_queue_t	*qelem )
{
	giomap_region_data_t	*rdata = (qelem->cbp)->context;

	qelem->status = UDI_OK;

	UDI_QUEUE_REMOVE( &qelem->Q );
	rdata->xfer_inuse_q.numelem--;
	UDI_ENQUEUE_TAIL( &rdata->xfer_q.Q, &qelem->Q );
	rdata->xfer_q.numelem++;
	/* Signal any blocked process that there's a new queue element */
	_OSDEP_EVENT_SIGNAL( &rdata->xfer_q_event );	
}

/*
 * ---------------------------------------------------------------------------
 * Interface to common giomap code
 * ---------------------------------------------------------------------------
 */

/*
 * giomap_OS_init:
 * --------------
 * Called on first per-instance driver instantiation. We need to initialise
 * the OS-dependent structures in the region-local data. This routine is 
 * synchronous.
 */
STATIC void
giomap_OS_init( 
	giomap_region_data_t *rdata )
{
	/*
	 * Allocate the control blocks that we're going to use for our
	 * internal xfer_cb and ioc_cb source. We need to allocate queue
	 * elements to hold the CB references and these will get moved from
	 * the available queue (xfer_q, ioc_q) to the in-use queue when the
	 * request is submitted.
	 *
	 * On completion of a request, we can awaken the correct process by
	 * ensuring that the transaction context references the queue element
	 * of the originating request.
	 */
	UDI_QUEUE_INIT( &rdata->xfer_q.Q );
	UDI_QUEUE_INIT( &rdata->xfer_inuse_q.Q );

	/* General purpose allocation token queue */
	UDI_QUEUE_INIT( &rdata->alloc_q.Q );

	/* Initialise the queue-specific mutex's -- needed for MP systems */
	_OSDEP_MUTEX_INIT( &rdata->xfer_lock, "giomap_posix: Transfer Q lock" );

	_OSDEP_EVENT_INIT( &rdata->xfer_q_event );

	/*
	 * Initialise the maximum allocation size that we'll use for our
	 * break-up buffer. This is dependent on the environment imposed
	 * limits
	 */
	if ( rdata->init_context.limits.max_safe_alloc > 0 ) {
		rdata->giomap_bufsize =
			MY_MIN( rdata->init_context.limits.max_safe_alloc,
				GIOMAP_BUFSIZE );
	} else {
		rdata->giomap_bufsize = GIOMAP_BUFSIZE;
	}
	return;
}

/*
 * giomap_OS_deinit:
 * ----------------
 * Called just before the region is torn down (from udi_final_cleanup_req).
 * This needs to release any resources which were allocated by giomap_OS_init
 * Currently this is just the OSDEP_MUTEX_T members
 */
STATIC void
giomap_OS_deinit(
	giomap_region_data_t	*rdata )
{
	_OSDEP_MUTEX_DEINIT( &rdata->xfer_lock );
	_OSDEP_EVENT_DEINIT( &rdata->xfer_q_event );
}

/*
 * giomap_OS_io_done:
 * -----------------
 * Called on completion of an i/o request. Either from a user-buffer [ioctl]
 * or a kernel buffer [biostart]
 */
STATIC void
giomap_OS_io_done( 
	udi_gio_xfer_cb_t *gio_xfer_cb, 
	udi_status_t	   status )
{
	giomap_queue_t		*qelem;

	qelem = (giomap_queue_t *)gio_xfer_cb->gcb.initiator_context;
	qelem->cbp = UDI_GCB(gio_xfer_cb);
	qelem->rw_cb = gio_xfer_cb;

#ifdef DEBUG
	_OSDEP_PRINTF("giomap_OS_io_done( cb = %p, status = %d )\n",
		gio_xfer_cb, status);
#endif /* DEBUG */

	qelem->status = status;

	/*
	 * Handle asynchronous ioctl() requests by only waking up synchronous
	 * ones
	 */
	switch ( qelem->type ) {
	case Ioctl:
		if ( !qelem->uio_p->u_async || !qelem->single_xfer ) {
			_OSDEP_EVENT_SIGNAL( &qelem->event );
		} else {
			/*
			 * Release queue element. The originating ioctl has
			 * long gone....
			 */
			giomap_req_release( qelem );
		}
		break;
	case Biostart:
		_OSDEP_EVENT_SIGNAL( &qelem->event );
		break;
	default:
		_OSDEP_PRINTF("giomap_OS_io_done: Illegal queue element type %d\n",
			qelem->type);
		GIOMAP_ASSERT( (qelem->type == Ioctl) || 
				(qelem->type == Biostart) );
		break;
	}
	return;
}

/*
 * giomap_OS_channel_event:
 * -----------------------
 * Called on receipt of a channel event indication from the environment. This
 * is the place to update the OS-specific structures which correspond to the
 * event that occurred. E.g., for a CONSTRAINTS_CHANGED event, we would need
 * to update any OS information which related to the existing device w.r.t
 * DMA restrictions, buffer data layout etc. etc.
 */
STATIC void
giomap_OS_channel_event(
	udi_channel_event_cb_t *cb )
{
	/* TODO: */
	udi_channel_event_complete( cb, UDI_OK );
}

/*
 * giomap_OS_bind_done:
 * -------------------
 * Called at end of meta bind sequence to perform OS specific initialisation
 * and to complete the originatl channel_event_ind
 */
STATIC void
giomap_OS_bind_done(
	udi_channel_event_cb_t *cb,
	udi_status_t		status)
{
	udi_channel_event_complete( cb, status );
}

/*
 * giomap_OS_unbind_done:
 * ---------------------
 * Called at end of meta unbind sequence. Needs to release any OS-specific
 * bindings that were created by giomap_OS_bind_done. On return from this
 * routine the MA will be informed that the pending UDI_DMGMT_UNBIND operation
 * is complete.
 */
STATIC void
giomap_OS_unbind_done(
	udi_gio_bind_cb_t	*cb)
{
	return;		/* Nothing to do */
}

/*
 * giomap_OS_event:
 * ---------------
 * Called on receipt of an asynchronous event notification from the GIO
 * provider. This routine could initiate an async handler for the user 
 * application to inform it of the event. The contents of the event params
 * field is defined by the provider. We know nothing about it. Hopefully the
 * user application also understands the layout.
 * Once the user has been notified of the event, the event_cb needs to be
 * returned to the provider [via udi_gio_event_res]
 */
STATIC void
giomap_OS_event(
	udi_gio_event_cb_t	*cb)
{
	udi_gio_event_res(cb);		/* Do nothing */
}

/*
 * ===========================================================================
 * Internal resource allocation routines
 */

/*
 * Allocate queue elements and control blocks for use by biostart() and
 * ioctl() interfaces
 */
STATIC void
giomap_OS_Alloc_Resources(
	giomap_region_data_t	*rdata )
{
	udi_mem_alloc( _giomap_got_alloc_cb, UDI_GCB(rdata->my_bind_cb),
		      sizeof( udi_queue_t ), UDI_MEM_NOZERO );
}

/*
 * Free all previously allocated queue elements and control blocks
 */
STATIC void
giomap_OS_Free_Resources(
	giomap_region_data_t	*rdata )
{

	udi_ubit32_t	ii;
	giomap_queue_t	*qelem;
	udi_queue_t	*head, *elem, *tmp;

	for ( ii = 0x80000000; ii > 0; ii >>= 1 ) {
		if ( !(rdata->resources & ii) ) continue;
		switch ( ii ) {
		case GiomapR_Req_Mem:
			head = &rdata->xfer_q.Q;
			GIOMAP_ASSERT( !UDI_QUEUE_EMPTY( &rdata->xfer_q.Q ) );
			UDI_QUEUE_FOREACH(head, elem, tmp) {
				qelem = (giomap_queue_t *)elem;
				_OSDEP_EVENT_DEINIT( &qelem->event );
				udi_buf_free(qelem->rw_cb->data_buf);
				udi_cb_free(UDI_GCB(qelem->rw_cb));
				qelem->rw_cb = NULL;
				udi_buf_free(qelem->diag_cb->data_buf);
				udi_cb_free(UDI_GCB(qelem->diag_cb));
				qelem->diag_cb = NULL;
				UDI_QUEUE_REMOVE( elem );
				rdata->xfer_q.numelem--;
				udi_mem_free( qelem->kernbuf );
				qelem->kernbuf = NULL;
				udi_mem_free( qelem );
			}
			GIOMAP_ASSERT(UDI_QUEUE_EMPTY(&rdata->xfer_inuse_q.Q));
			break;
		default:
			break;
		}
		rdata->resources &= ~ii;
	}
}

STATIC void
_giomap_getnext_rsrc(
	giomap_region_data_t	*rdata )
{
	udi_ubit32_t	ii;
	udi_queue_t	*rq, *tmp;

	for ( ii = 1; ii < 0x80000000; ii <<= 1 ) {
		if ( rdata->resource_rqst & ii ) continue;
		if ( rdata->resources & ii ) continue;

		rq = UDI_DEQUEUE_HEAD(&rdata->alloc_q.Q);
		if ( rq == NULL ) return;
		rdata->alloc_q.numelem--;
		rdata->resource_rqst |= ii;
		switch ( ii ) {
		case GiomapR_Req_Mem:
			udi_mem_alloc( _giomap_got_reqmem,
					UDI_GCB(rdata->my_bind_cb),
					sizeof( giomap_queue_t ), 0);
			udi_mem_free( rq );
			rq = NULL;
			break;
		default:
			UDI_ENQUEUE_TAIL( &rdata->alloc_q.Q, rq );
			rdata->alloc_q.numelem++;
			rdata->resources |= ii;
			break;
		}
	}

	if ( rdata->resources == 0x7fffffff ) {
		UDI_QUEUE_FOREACH( &rdata->alloc_q.Q, rq, tmp ) {
			UDI_QUEUE_REMOVE( rq );
			udi_mem_free( rq );
			rdata->alloc_q.numelem--;
		}
		rdata->resources |= 0x80000000;
		giomap_Resources_Alloced( rdata );
	}
}


STATIC void
_giomap_got_reqmem(
	udi_cb_t	*gcb,
	void		*new_mem )
{
	giomap_region_data_t	*rdata = gcb->context;

	UDI_ENQUEUE_TAIL( &rdata->xfer_q.Q, (udi_queue_t *)new_mem );
	rdata->xfer_q.numelem++;

	/*
	 * Allocate CBs for:
	 *	Read/Write 	[udi_gio_rw_params_t], 
	 *	Diagnostics	[udi_gio_diag_params_t] + User-specific,
	 */
	udi_cb_alloc( _giomap_got_req_RW_cb, gcb, 
		UDI_GIO_XFER_CB_RW_IDX, gcb->channel );
}

STATIC void
_giomap_got_req_RW_cb(
	udi_cb_t	*gcb,
	udi_cb_t	*new_cb )
{
	giomap_region_data_t	*rdata = gcb->context;
	giomap_queue_t		*qelem;

	qelem = (giomap_queue_t *)UDI_LAST_ELEMENT( &rdata->xfer_q.Q );
	qelem->rw_cb = UDI_MCB( new_cb, udi_gio_xfer_cb_t );

	udi_cb_alloc( _giomap_got_req_DIAG_cb, gcb, 
		UDI_GIO_XFER_CB_DIAG_IDX, gcb->channel );
}

STATIC void
_giomap_got_req_DIAG_cb(
	udi_cb_t	*gcb,
	udi_cb_t	*new_cb )
{
	giomap_region_data_t	*rdata = gcb->context;
	giomap_queue_t		*qelem;

	qelem = (giomap_queue_t *)UDI_LAST_ELEMENT( &rdata->xfer_q.Q );
	qelem->diag_cb = UDI_MCB( new_cb, udi_gio_xfer_cb_t );

	_OSDEP_EVENT_INIT( &qelem->event );
	qelem->diag_cb = UDI_MCB( new_cb, udi_gio_xfer_cb_t );

	/* Initialise cbp to the diag_cb */
	qelem->cbp = new_cb;
	qelem->cb_type = Diag;

	qelem->buf_p = &qelem->u.buf;
	qelem->uio_p = &qelem->u.uio;

	if ( rdata->xfer_q.numelem < GIOMAP_MAX_CBS ) {
		udi_mem_alloc( _giomap_got_reqmem, gcb, 
				sizeof( giomap_queue_t ), 0 );
	} else {
		rdata->resources |= GiomapR_Req_Mem;
		udi_mem_alloc( _giomap_got_alloc_cb, gcb, sizeof( udi_queue_t ),
				UDI_MEM_NOZERO );
	}
}

/*
 * Get the next resource - common to both xfer_q and abort_q
 */
STATIC void
_giomap_got_alloc_cb(
	udi_cb_t	*gcb,
	void		*new_mem )
{
	giomap_region_data_t	*rdata = gcb->context;

	UDI_ENQUEUE_TAIL(&rdata->alloc_q.Q, (udi_queue_t *)new_mem);
	rdata->alloc_q.numelem++;
	_giomap_getnext_rsrc( rdata );
}


/*
 * giomap_calc_offsets:
 * -------------------
 *	Convert a block-offset to its corresponding byte offset representation.
 *	This may exceed 32bits, so the hi and lo arguments are updated
 *	appropriately.
 */
STATIC void
giomap_calc_offsets(
	udi_ubit32_t	block,
	udi_ubit32_t	blkoff,
	udi_ubit32_t	*hi,
	udi_ubit32_t	*lo )
{
	if ( block >= GIOMAP_MAX_BLOCK ) {
		*lo = (block - GIOMAP_MAX_BLOCK) << GIOMAP_SEC_SHFT;
		*hi = (block / GIOMAP_MAX_BLOCK);
	} else {
		*lo = block << GIOMAP_SEC_SHFT;
		*hi = 0;
	}
	*lo += blkoff;
}
