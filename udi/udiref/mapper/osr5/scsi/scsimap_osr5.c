/* #define SCSIMAP_DEBUG */
/*
 * ========================================================================
 * File	 : scsimap.c
 * Description  : A mapper which converts OSDI specific HBA requests to 
 *		UDI peripheral driver requests.
 *
 * ========================================================================
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
 * Written for SCO by: Rob Tarte
 * At: Pacific CodeWorks, Inc.
 * Contact: http://www.pacificcodeworks.com
 */

#define UDI_SCSI_VERSION 0x101

#include <sys/types.h>
#include <sys/immu.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/iobuf.h>
#include <sys/conf.h>
#include <sys/dir.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/seg.h>
#include <sys/page.h>
#include <sys/user.h>
#include <sys/dio.h>
#include <sys/disk.h>
#include <sys/devreg.h>
#include <sys/arch.h> 
#include <sys/select.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/mount.h>
#include <sys/cram.h>   
#include <sys/dma.h>

#include <sys/scsi.h> 
#include <sys/ci/cidriver.h>
#include <sys/ci/ciintr.h>
#include <sys/xdebug.h>
#include <udi_env.h>	 	/* for _udi_dev_node_t (TODO: better way) */
#include <udi_scsi.h>		/* SCSI Metalanguage */
#include "scsimap_osr5.h"

_OSDEP_MUTEX_T	scsimap_lock;

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

STATIC void scsimap_bind_to_parent(udi_cb_t *gcb);
STATIC void scsimap_instance_attribute_get_cbfn(udi_cb_t *,
		udi_instance_attr_type_t, udi_size_t);
STATIC void scsimap_scsi_bind_start(udi_scsi_bind_cb_t*);
STATIC void scsimap_next(scsimap_region_data_t *);
STATIC scsimap_sblk_t * scsimap_get_next_best_job(scsimap_region_data_t *);
STATIC udi_scsi_io_cb_t * scsimap_get_io_cb(scsimap_region_data_t *rdata);
STATIC void scsimap_send_io_req(udi_cb_t *, udi_buf_t *);
STATIC long scsimap_send_req(scsimap_region_data_t *, scsimap_sblk_t *);
STATIC void scsimap_cmd(scsimap_sblk_t *sp);
STATIC void scsimap_alloc_io_req_cb_cbfn(udi_cb_t *, udi_cb_t *);
STATIC void scsimap_sense(scsimap_sblk_t *sp);
STATIC void scsimap_done(scsimap_sblk_t *sp);
STATIC void scsimap_free_io_cb(scsimap_region_data_t *, udi_scsi_io_cb_t *);
STATIC void scsimap_putq(scsimap_sblk_t **, scsimap_sblk_t *, int);
STATIC void scsimap_alloc_bus_struct(scsimap_region_data_t *rdata);
STATIC void scsimap_ha_register(scsimap_region_data_t *, ulong_t local_cntl);
STATIC void scsimap_alloc_lun_struct(scsimap_region_data_t *rdata);
STATIC void scsimap_alloc_target_struct(scsimap_region_data_t *rdata);
STATIC void scsimap_assign_ha_struct(scsimap_region_data_t *);
STATIC int  scsimap_process_req(scsimap_region_data_t *, REQ_IO *);
STATIC void scsimap_get_instance_attr(udi_cb_t *, udi_instance_attr_type_t,
		udi_size_t);

typedef union {
	udi_ubit8_t		lun_attr[8];
} scsimap_bind_scratch_t;


/* FIXME - ??? */
#define SCSIMAP_SCSI_META	1

#define SCSI_OP_IDX	1

#define UDI_SCSI_MAX_CDB_LEN	12

#define SCSI_BIND_CB_IDX	1
#define SCSI_IO_CB_IDX		2
#define SCSI_CTL_CB_IDX		3
#define SCSI_EVENT_CB_IDX	4

#define SCSIMAP_MGMT_SCRATCH	0
#define SCSI_BIND_SCRATCH	sizeof(scsimap_bind_scratch_t)
#define SCSI_IO_SCRATCH		sizeof(scsimap_cb_res_t)
#define SCSI_CTL_SCRATCH	0
#define SCSI_EVENT_SCRATCH	0

STATIC char scsimap_name[] = "UDI SCSI Mapper vX.Y";

/* MA interface routines */
STATIC udi_devmgmt_req_op_t scsimap_devmgmt_req;
STATIC udi_final_cleanup_req_op_t scsimap_final_cleanup_req;
STATIC void scsimap_usage_ind(udi_usage_cb_t *, udi_ubit8_t);

/* SCSI Interface routines */
STATIC udi_channel_event_ind_op_t scsimap_scsi_channel_event_ind;
STATIC udi_scsi_bind_ack_op_t scsimap_scsi_bind_ack;
STATIC udi_scsi_unbind_ack_op_t scsimap_scsi_unbind_ack;
STATIC udi_scsi_io_ack_op_t scsimap_scsi_io_ack;
STATIC udi_scsi_io_nak_op_t scsimap_scsi_io_nak;
STATIC udi_scsi_ctl_ack_op_t scsimap_scsi_ctl_ack;
STATIC udi_scsi_event_ind_op_t scsimap_scsi_event_ind;

STATIC udi_mgmt_ops_t  scsimap_mgmt_ops = {
	scsimap_usage_ind,		/* usage_ind */
	udi_enumerate_no_children,	/* enumerate */
	scsimap_devmgmt_req,		/* devmgmt_req */
	scsimap_final_cleanup_req,	/* final_cleanup_req */
};


STATIC udi_scsi_pd_ops_t  scsimap_scsi_pd_ops = {
	scsimap_scsi_channel_event_ind,
	scsimap_scsi_bind_ack,
	scsimap_scsi_unbind_ack,
	scsimap_scsi_io_ack,
	scsimap_scsi_io_nak,
	scsimap_scsi_ctl_ack,
	scsimap_scsi_event_ind
};

/*
 * default set of op_flags
 */
static udi_ubit8_t scsimap_default_op_flags[] = {
	0, 0, 0, 0, 0, 0, 0
};

/*
 * -----------------------------------------------------------------------------
 * Management operations init section:
 * -----------------------------------------------------------------------------
 */
static udi_primary_init_t	scsimap_primary_init = {
	&scsimap_mgmt_ops,
	scsimap_default_op_flags,	/* op_flags */
	SCSIMAP_MGMT_SCRATCH,	/* mgmt_scratch_size */
	0, /* enumeration attr list length */
	sizeof( scsimap_region_data_t ),
	0,
	1	/* buf path */
};

/*
 * -----------------------------------------------------------------------------
 * Meta operations init section:
 * -----------------------------------------------------------------------------
 */
static udi_ops_init_t	scsimap_ops_init_list[] = {
	{
		SCSI_OP_IDX,
		SCSIMAP_SCSI_META,	/* meta index [from udiprops.txt] */
		UDI_SCSI_PD_OPS_NUM,
		0,			/* Channel context size */
		(udi_ops_vector_t *)&scsimap_scsi_pd_ops,
		scsimap_default_op_flags	/* op_flags */
	},
	{
		0	/* Terminator */
	}
};

/*
 * -----------------------------------------------------------------------------
 * Control Block init section:
 * -----------------------------------------------------------------------------
 */

static udi_layout_t	cdb_layout[] = { UDI_DL_UBIT8_T, UDI_DL_UBIT8_T,
					  UDI_DL_ARRAY, 
					  UDI_SCSI_MAX_CDB_LEN,
					  UDI_DL_UBIT8_T, UDI_DL_END,
					  UDI_DL_END };

static udi_cb_init_t	scsimap_cb_init_list[] = {
	{
		SCSI_BIND_CB_IDX,	/* SCSI Bind CB		*/
		SCSIMAP_SCSI_META,	/* from udiprops.txt 	*/
		UDI_SCSI_BIND_CB_NUM,	/* meta cb_num		*/
		SCSI_BIND_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
	{
		SCSI_IO_CB_IDX,		/* SCSI i/o CB		*/
		SCSIMAP_SCSI_META,	/* from udiprops.txt 	*/
		UDI_SCSI_IO_CB_NUM,	/* meta cb_num		*/
		SCSI_IO_SCRATCH,	/* scratch requirement	*/
		UDI_SCSI_MAX_CDB_LEN,	/* inline size		*/
		cdb_layout		/* inline layout	*/
	},
	{
		SCSI_CTL_CB_IDX,	/* SCSI ctl CB		*/
		SCSIMAP_SCSI_META,	/* from udiprops.txt 	*/
		UDI_SCSI_CTL_CB_NUM,	/* meta cb_num		*/
		SCSI_CTL_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
	{
		SCSI_EVENT_CB_IDX,	/* SCSI event CB		*/
		SCSIMAP_SCSI_META,	/* from udiprops.txt 	*/
		UDI_SCSI_EVENT_CB_NUM,	/* meta cb_num		*/
		SCSI_EVENT_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
	{
		0			/* Terminator */
	}
};

udi_init_t udi_init_info = {
	&scsimap_primary_init,
	NULL, /* Secondary init list */
	scsimap_ops_init_list, 
	scsimap_cb_init_list,
	NULL, /* gcb init list */
	NULL, /* cb select list */
};

STATIC ulong_t		scsimap_gtol[MAX_EXHAS];
STATIC ulong_t		scsimap_ltog[MAX_EXHAS];

#ifdef SCSIMAP_DEBUG
unsigned		scsimap_outstanding;
#endif

STATIC _OSDEP_ATOMIC_INT_T	scsimap_instance_counter;
STATIC udi_boolean_t		scsimap_init_done = FALSE;
udi_queue_t			scsimap_hba_list;

/*
 * UDI entry point for scsi_bind_ack
 */
STATIC void
scsimap_scsi_bind_ack(
	udi_scsi_bind_cb_t	*scsi_bind_cb,
	udi_ubit32_t		hd_timeout_increase,
	udi_status_t		status
)
{
	scsimap_region_data_t   *rdata; 

	rdata = UDI_GCB(scsi_bind_cb)->context; 

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("scsi_bind_ack\n");
#endif

	/*
	 * Sleaze alert: we keep track of the new <scsi_bind_cb> so that the
	 * OS-specific mapper can have a handle to use when it needs to
	 * allocate its own control blocks. The original bind_cb belongs to
	 * the MA and will disappear
	 */
	rdata->scsi_bind_cb = scsi_bind_cb;

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("scsimap_scsi_bind_ack: setting scsi_bind_cb = 0x%x\n",
		scsi_bind_cb);
#endif

	/* Allocate a pool of I/O CBs */

	UDI_QUEUE_INIT(&rdata->cb_res_pool);
	rdata->num_cbs = 0;

	scsimap_assign_ha_struct(rdata);

	/*
	 * Allocate a pool of I/O CBs
	 */
	udi_cb_alloc(scsimap_alloc_io_req_cb_cbfn,
			UDI_GCB(rdata->scsi_bind_cb),
			SCSI_IO_CB_IDX,
			UDI_GCB(rdata->scsi_bind_cb)->channel);

	/*
	 * Now allocate a scsimap_bus structure if needed.
	 */
	scsimap_alloc_bus_struct(rdata);
}


/* 
 * This routine is called by the resource allocation code after
 * resources (scsi control block and udi_buf_t) are acquired
 * for the i/o request specified by sp.
 */
STATIC void
scsimap_cmd(scsimap_sblk_t *sp)
{
	scsimap_region_data_t   *rdata;
	udi_scsi_io_cb_t    	*cb;

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("scsimap_cmd: sp=0x%x\n", sp);
#endif

	rdata = sp->rdata;

	/*
	 * If the scsi command is REQUEST SENSE and sense data is valid
	 * then call scsimap_sense()
	 */
	if (sp->reqp->scsi_cmd.six.opcode == SENSE_CMD &&
		(rdata->flags & SCSIMAP_SENSE_DATA_VALID))
	{
		rdata->flags &= ~SCSIMAP_SENSE_DATA_VALID;

		scsimap_sense(sp);

		return;
	}

	/*
	 * Initialize cb and call udi_scsi_io_req()
	 */
	cb		= (udi_scsi_io_cb_t *)sp->cb;
	cb->cdb_len	= sp->reqp->cmdlen;

	/*
	 * Copy command and bytes from the request structure to cdb
	 */ 
	bcopy((caddr_t)&sp->reqp->scsi_cmd, (caddr_t)cb->cdb_ptr,
		sp->reqp->cmdlen);
	
	/*
	 * tell the HBA to do all of the timeout work. 
	 */
	cb->timeout = 0;

	if (sp->reqp->data_len == 0)
	{
		cb->flags = 0;
	}
	else
	{
		cb->flags = (sp->reqp->dir == SCSI_IN) ?
			UDI_SCSI_DATA_IN : UDI_SCSI_DATA_OUT;
	}

	cb->attribute = UDI_SCSI_ORDERED_TASK;

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("udi_scsi_io_req: cb=0x%x\n", cb);
#endif

#ifdef SCSIMAP_DEBUG
	_OSDEP_MUTEX_LOCK(&scsimap_lock);

	scsimap_outstanding++;

	_OSDEP_MUTEX_UNLOCK(&scsimap_lock);
#endif

	udi_scsi_io_req(cb);
}


/*
 * This routine is called if the scsi command specified by sp is
 * REQUEST SENSE and rdata has valid sense data.
 */
STATIC void 
scsimap_sense(scsimap_sblk_t *sp)
{
	scsimap_region_data_t   *rdata = sp->rdata;
	int		     len;

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("scsimap_sense\n");
#endif

	len = MIN(sp->reqp->data_len, rdata->sense_len);

	bcopy((caddr_t)rdata->sense, (caddr_t)ptok(sp->reqp->data_ptr), len);

	scsimap_done(sp);
}


/*
 * Dequeue sblk(sp) from the given queue and return it.
 * NULL is returned if the queue does not contain any element.
 * If "where" is SCSIMAP_QHEAD then sp is dequeued from the head of the queue.
 * If "where" is SCSIMAP_QTAIL then sp is dequeued from the tail of the queue.
 * Queue is a doubly linked circular queue.
 */
STATIC scsimap_sblk_t *
scsimap_sblk_dequeue(scsimap_sblk_t **qp, int where)
{
	scsimap_sblk_t		*sp;

	sp = *qp;

	if (*qp == (scsimap_sblk_t *)NULL)
	{
		return (scsimap_sblk_t *) NULL;
	}

	if (where == SCSIMAP_QHEAD)
	{
		sp = *qp;
	}
	else
	{
		sp = (*qp)->prev;
	}

	if (sp->next == sp)
	{
		*qp = NULL;
	}
	else
	{
		if (*qp == sp)
		{
			*qp = sp->next;
		}

		sp->next->prev = sp->prev;

		sp->prev->next = sp->next;
	}

	sp->next = NULL;

	sp->prev = NULL;

	return sp;
}

/*
 * Enqueue sp on the queue and try acquiring resources.
 */
STATIC long
scsimap_send_req(scsimap_region_data_t *rdata, scsimap_sblk_t *sp)
{
	ulong_t c;

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("scsimap_send_req: sp = 0x%x reqp = 0x%x\n", sp,
		sp->reqp);
#endif

	sp->res_held = 0;

	c = scsimap_gtol[sp->reqp->ha_num];

	_OSDEP_MUTEX_LOCK(&rdata->r_lock);

	scsimap_putq(&rdata->sbq, sp, SCSIMAP_QTAIL);

	_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);

	/*
	 * Acquire resources.
	 */
	scsimap_next(rdata);

	return 0; /* success */
}

STATIC scsimap_sblk_t *
scsimap_get_next_best_job(
scsimap_region_data_t	*rdata
)
{
	scsimap_sblk_t	  *sp;

	_OSDEP_MUTEX_LOCK(&rdata->r_lock);

	if (rdata->sbq != (scsimap_sblk_t *) NULL)
	{
		sp = scsimap_sblk_dequeue(&rdata->sbq, SCSIMAP_QHEAD);
	}
	else
	{
		sp = (scsimap_sblk_t *) NULL;
	}

	_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);

	return sp;
}


/*
 * Call back function for scsi control block allocation.
 */
STATIC void
scsimap_alloc_io_req_cb_cbfn(udi_cb_t *gcb, udi_cb_t *new_cb)
{
	scsimap_region_data_t	*rdata;
	udi_scsi_io_cb_t	*scsi_io_cb;
	scsimap_cb_res_t	*cb_res;

	rdata = gcb->context;

	scsi_io_cb = UDI_MCB(new_cb, udi_scsi_io_cb_t);

	cb_res = UDI_GCB(scsi_io_cb)->scratch;

	cb_res->cb = scsi_io_cb;

	UDI_ENQUEUE_TAIL(&rdata->cb_res_pool, &cb_res->link);

	if (rdata->num_cbs++ < SCSIMAP_MAX_IO_CBS)
	{
		/* Allocate more CBs */
		udi_cb_alloc(scsimap_alloc_io_req_cb_cbfn,
				gcb,
				SCSI_IO_CB_IDX,
				gcb->channel);
		return;	
	}

	/* All CBs have been allocated */

#ifdef NOTYET
	if (scsimap_drv_attach(rdata)) {
		udi_channel_event_complete(rdata->channel_event_cb,
			UDI_STAT_CANNOT_BIND);
	}
	else {
		udi_channel_event_complete(rdata->channel_event_cb, UDI_OK);
	}
#endif /* NOTYET */
}


STATIC void
scsimap_done(scsimap_sblk_t *sp)
{
	ulong_t c;

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("scsimap_done: sp=0x%x  req_p=0x%x io_intr=0x%x\n",
		sp, sp->reqp, sp->reqp->io_intr);
#endif

	c = scsimap_gtol[sp->reqp->ha_num];

	(*sp->reqp->io_intr) (sp->reqp);

	_OSDEP_MEM_FREE(sp);
}

STATIC void
scsimap_devmgmt_req(udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, 
			udi_ubit8_t parent_id)

{
	scsimap_region_data_t *rdata = UDI_GCB(cb)->context;

	switch (mgmt_op) {

	case UDI_DMGMT_UNBIND:
		/*
		 * unbind from parent
		 */
		rdata->mgmt_cb = UDI_GCB(cb);

		if (rdata->multi_lun)
		{
			scsimap_scsi_unbind_ack(rdata->scsi_bind_cb);
		}
		else
		{
			udi_scsi_unbind_req(rdata->scsi_bind_cb);
		}

		/* unbind_ack will do the devmgmt_ack */
		return;

	default:
		break;
	}

	udi_devmgmt_ack(cb, 0, UDI_OK);
	
}

STATIC void
scsimap_final_cleanup_req(udi_mgmt_cb_t *cb)
{
	/* FIXME: inspect and see if any region-local or other data 
	 * needs to be released here.
	 */
	udi_final_cleanup_ack(cb);
}

/*
 * UDI entry point for scsi_channel_event
 */
STATIC void
scsimap_scsi_channel_event_ind(udi_channel_event_cb_t *channel_event_cb)
{
	switch (channel_event_cb->event)
	{
	case UDI_CHANNEL_CLOSED:
		break;

	case UDI_CHANNEL_BOUND:

		scsimap_bind_to_parent(UDI_GCB(channel_event_cb));
		return;

	case UDI_CHANNEL_OP_ABORTED:
		break;
	}

	udi_channel_event_complete(channel_event_cb, UDI_OK);
}

/*
 * The first per-instance driver code that executes.
 *
 * Tasks for this operation:
 *	+ Save info from the bind_to_parent_req control block for later use.
 *	+ Get attributes from device node and save them in region data.
 */

STATIC void
scsimap_bind_to_parent(
udi_cb_t	*gcb
)
{
	ulong_t			i;
	scsimap_region_data_t	*rdata;
	udi_channel_event_cb_t	*channel_event_cb;
	udi_scsi_bind_cb_t	*scsi_bind_cb;

	channel_event_cb = UDI_MCB(gcb, udi_channel_event_cb_t);

	scsi_bind_cb = UDI_MCB(channel_event_cb->params.internal_bound.bind_cb,
				udi_scsi_bind_cb_t);

	SCSIMAP_ASSERT(scsi_bind_cb != NULL, "scsi_bind_cb is NULL");

	rdata = gcb->context;

	/*
	 * save the udi_channel_event_cb_t
	 */
	rdata->mgmt_cb = gcb;

	rdata->buf_path = 
		channel_event_cb->params.parent_bound.path_handles[0];

	/*
	 * save the bind cb
	 */
	rdata->scsi_bind_cb = scsi_bind_cb;

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("scsimap_bind_to_parent: setting scsi_bind_cb = "
		"0x%x\n", scsi_bind_cb);

	udi_debug_printf("scsimap_bind_to_parent\n");
#endif

	rdata->channel_event_cb = UDI_MCB(gcb, udi_channel_event_cb_t);

	/*
	 * Perform OS-specific region-local data initialization (synchronous) 
	 */
	UDI_QUEUE_INIT(&rdata->alloc_q);
	UDI_QUEUE_INIT(&rdata->reqp_q);
	_OSDEP_MUTEX_INIT(&rdata->r_lock, "rdata lock");

	/* 
	* Get the attribute
	*/
	rdata->cbfn_type = SCSIMAP_START_GET_ATTR;

	scsimap_get_instance_attr(UDI_GCB(scsi_bind_cb), UDI_ATTR_NONE, 0);
}

/* 
 * Call back function for udi_instance_attr_get
 */
STATIC void
scsimap_get_instance_attr(udi_cb_t * gcb,
			  udi_instance_attr_type_t attr_type,
			  udi_size_t actual_length)
{
	scsimap_region_data_t *rdata = gcb->context;
	scsimap_bind_scratch_t *bind_scratch = gcb->scratch;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_get_instance_attr\n");
#endif

	switch (rdata->cbfn_type) {
	case SCSIMAP_START_GET_ATTR:
		/* check if this is the HBA device */
		rdata->cbfn_type = SCSIMAP_GET_MULTI_LUN_ATTR;

		udi_instance_attr_get(scsimap_get_instance_attr,
			gcb, "scsi_multi_lun", 0,
			(void *)&rdata->multi_lun,
			sizeof(rdata->multi_lun));

		break;

	case SCSIMAP_GET_MULTI_LUN_ATTR:
		/* check if this is the HBA device */
		if (rdata->multi_lun) {
			/* Get other attributes associated with multi-lun */
			rdata->cbfn_type = SCSIMAP_GET_MAX_BUSES_ATTR;

			udi_instance_attr_get(scsimap_get_instance_attr,
				gcb, "scsi_max_buses", 0,
				(void *)&rdata->max_buses,
				sizeof(rdata->max_buses));
		} else {
			/* Not a multi-lun device */
			rdata->cbfn_type = SCSIMAP_GET_BUS_ATTR;

			udi_instance_attr_get(scsimap_get_instance_attr,
				gcb, "scsi_bus", 0,
				(void *)&rdata->scsi_addr.b,
				sizeof(rdata->scsi_addr.b));
		}
		break;

	case SCSIMAP_GET_MAX_BUSES_ATTR:
		udi_assert(sizeof(rdata->max_buses) == actual_length);

		rdata->cbfn_type = SCSIMAP_GET_MAX_TGTS_ATTR;

		udi_instance_attr_get(scsimap_get_instance_attr,
			gcb, "scsi_max_tgts", 0,
			(void *)&rdata->max_targets,
			sizeof(rdata->max_targets));
		break;

	case SCSIMAP_GET_MAX_TGTS_ATTR:
		udi_assert(sizeof(rdata->max_targets) == actual_length);

		rdata->cbfn_type = SCSIMAP_GET_MAX_LUNS_ATTR;

		udi_instance_attr_get(scsimap_get_instance_attr,
			gcb, "scsi_max_luns", 0,
			(void *)&rdata->max_luns,
			sizeof(rdata->max_luns));
		break;

	case SCSIMAP_GET_MAX_LUNS_ATTR:
		udi_assert(sizeof(rdata->max_luns) == actual_length);

		udi_channel_event_complete(UDI_MCB(rdata->mgmt_cb,
			udi_channel_event_cb_t), UDI_OK);

		break;

	case SCSIMAP_GET_BUS_ATTR:
		udi_assert(sizeof(rdata->scsi_addr.b) == actual_length);

		rdata->cbfn_type = SCSIMAP_GET_TGT_ATTR;

		udi_instance_attr_get(scsimap_get_instance_attr,
			gcb, "scsi_target", 0,
			(void *)&rdata->scsi_addr.t,
			sizeof(rdata->scsi_addr.t));

		break;

	case SCSIMAP_GET_TGT_ATTR:
		udi_assert(sizeof(rdata->scsi_addr.t) == actual_length);

		rdata->cbfn_type = SCSIMAP_GET_LUN_ATTR;

		udi_instance_attr_get(scsimap_get_instance_attr,
			gcb, "scsi_lun", 0,
			(void *)bind_scratch->lun_attr,
			sizeof(rdata->scsi_addr.l));

		break;

	case SCSIMAP_GET_LUN_ATTR:
		udi_assert(sizeof(bind_scratch->lun_attr) == actual_length);

		/* 
		* Just use the second byte out of the SAM-2 style LUN struct
		*/
		rdata->scsi_addr.l = bind_scratch->lun_attr[1];

		rdata->cbfn_type = SCSIMAP_GET_INQ_ATTR;

		udi_instance_attr_get(scsimap_get_instance_attr,
			gcb, "scsi_inquiry", 0,
			(void *)&rdata->inquiry_data[0],
			sizeof(rdata->inquiry_data));

		break;

	case SCSIMAP_GET_INQ_ATTR:
		SCSIMAP_ASSERT(sizeof(rdata->inquiry_data) == actual_length,
			"[scsimap_instance_attribute_get_cbfn] Incorrect "
			"INQUIRY attribute length");

		/*
		* Allocate scsi control block for scsi bind operation.
		* XXX - Adaptec driver needs scratch space, but we
		*	 have to hard-code it for now.
		*/
		/*
		* Allocate control block for gio bind operation.
		*/
		scsimap_scsi_bind_start(UDI_MCB(gcb, udi_scsi_bind_cb_t));

		break;

	default:
		break;
	}
}


/*
 * Call back function for allocating a control block for scsi bind.
 * Initializes the control block and calls udi_scsi_bind_req().
 */
STATIC void
scsimap_scsi_bind_start(udi_scsi_bind_cb_t	*scsi_bind_cb)
{
#ifdef NOTYET /* aha294x bug workaround */
	scsimap_region_data_t *rdata = UDI_GCB(scsi_bind_cb)->context;
#endif

#ifdef NOTYET /* aha294x bug workaround */
	scsi_bind_cb->bind_req.tr_context = 0;
	scsi_bind_cb->bind_req.scratch_size = 0;
	scsi_bind_cb->bind_req.max_sense_len = SCSIMAP_MAX_SENSE_LEN;

	/* 
	 * If the scsi devices supports command queuing then set
	 * the queue_depth to SCSIMAP_MAX_QUEUE_DEPTH otherwise set
	 * the queue depth to 1.
	 */
#ifdef NOTYET /* aha294x bug workaround */
	scsi_bind_cb->bind_req.queue_depth = 
	  (SCSIMAP_IS_CMDQ(rdata->inquiry_data)) ? SCSIMAP_MAX_QUEUE_DEPTH : 1;
#else
	scsi_bind_cb->bind_req.queue_depth = 1;
#endif
	scsi_bind_cb->bind_req.timeout_granularity = SCSIMAP_TIMEOUT_GRANULARITY;

	scsi_bind_cb->bind_req.flags = 0;

	/*
	 * Turn on WDTR if the scsi device supports WDTR.
	 */
	if (SCSIMAP_IS_WDTR(rdata->inquiry_data))
		scsi_bind_cb->bind_req.flags |= SCSI_ENABLE_WDTR;

	/*
	 * Turn on SDTR if the scsi device supports SDTR.
	 */
	if (SCSIMAP_IS_SDTR(rdata->inquiry_data))
		scsi_bind_cb->bind_req.flags |= SCSI_ENABLE_SDTR;
#else
		
	/*
	 * TODO: Do we need to set some events here?
	 */
	scsi_bind_cb->events = 0;
#endif
	/*
	 * Send scsi metalanguage specific bind request to the HBA driver.
	 */

	udi_scsi_bind_req(scsi_bind_cb,
			UDI_SCSI_BIND_EXCLUSIVE,
			SCSIMAP_MAX_QUEUE_DEPTH,
			SCSIMAP_MAX_SENSE_LEN, 0);
}

/*
 * UDI entry point for scsi_io_ack
 */
STATIC void
scsimap_scsi_io_ack(
udi_scsi_io_cb_t	*scsi_io_cb
)
{
	scsimap_region_data_t	*rdata;
	scsimap_sblk_t		*sp;

	sp = (scsimap_sblk_t *)scsi_io_cb->gcb.initiator_context;

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("scsimap_scsi_io_ack: scsi_io_cb=0x%x reqp=0x%x "
		"sp=0x%x bp=0x%x\n", scsi_io_cb, sp->reqp, sp, sp->reqp->rbuf);
#endif

	rdata = UDI_GCB(scsi_io_cb)->context;

#ifdef SCSIMAP_DEBUG
	_OSDEP_MUTEX_LOCK(&scsimap_lock);

	scsimap_outstanding--;

	_OSDEP_MUTEX_UNLOCK(&scsimap_lock);
#endif

	/*
	 * Sense data is no longer valid.
	 */
	rdata->flags &= ~SCSIMAP_SENSE_DATA_VALID;

	if ((scsi_io_cb->data_buf != NULL) && (void *)ptok(sp->reqp->data_ptr))
	{
		ASSERT(scsi_io_cb->data_buf->buf_size == 
			sp->sbp->sb.SCB.sc_datasz);

		/*
		 * Free the resources allocated for the i/o request.
		 */
		udi_buf_free(scsi_io_cb->data_buf);
	}

	(*sp->reqp->io_intr)(sp->reqp);

	_OSDEP_MEM_FREE(sp);

	/*
	 * Return the CB resource back to the pool
	 */
	scsimap_free_io_cb(rdata, scsi_io_cb);

	scsimap_next(rdata);
}

/*
 * UDI entry point for scsi_io_nak
 */
STATIC void
scsimap_scsi_io_nak(
udi_scsi_io_cb_t *scsi_io_cb,
udi_status_t req_status,
udi_ubit8_t scsi_status,
udi_ubit8_t sense_status,
udi_buf_t *sense_buf
)
{
	scsimap_region_data_t	*rdata;
	scsimap_sblk_t		*sp;
	unsigned		len;

	sp = scsi_io_cb->gcb.initiator_context;

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("scsimap_scsi_io_ack: scsi_io_cb=0x%x reqp=0x%x "
		"sp=0x%x bp=0x%x\n", scsi_io_cb, sp->reqp, sp, sp->reqp->rbuf);
#endif

	rdata = UDI_GCB(scsi_io_cb)->context;

#ifdef SCSIMAP_DEBUG
	_OSDEP_MUTEX_LOCK(&scsimap_lock);

	scsimap_outstanding--;

	_OSDEP_MUTEX_UNLOCK(&scsimap_lock);
#endif

	/*
	 * Sense data is no longer valid.
	 */
	rdata->flags &= ~SCSIMAP_SENSE_DATA_VALID;

	if (scsi_io_cb->data_buf != NULL)
	{
		udi_buf_free(scsi_io_cb->data_buf);
	}

	switch (req_status)
	{
	case UDI_OK:
		break;

	case UDI_SCSI_STAT_NONZERO_STATUS_BYTE:
		/*
		 * Check if the scsi status is CHECK CONDITION.
		 */
		if (scsi_status == 2 /* S_CKCON */)
		{
			unsigned	i;
			paddr_t		scsi_sense;

			/*
			 * Copy sense data to rdata->sense, then free
			 * the sense buffer passed from the HBA.
			 */
			rdata->sense_len = MIN(sense_buf->buf_size,
				SCSIMAP_MAX_SENSE_LEN);

#ifdef SCSIMAP_DEBUG
			udi_debug_printf("reading sense data... ");
#endif

			udi_buf_read(sense_buf, 0, rdata->sense_len,
				rdata->sense);

			for (i = 0 ; i < rdata->sense_len ; i++)
			{
				udi_debug_printf("0x%x ", rdata->sense[i]);
			}

			udi_debug_printf("\n");

			if (sp->reqp->scsi_sense == NULL)
			{
				scsi_sense = (paddr_t)&sp->reqp->
					scsi_cmd.raw[sp->reqp->cmdlen];
			}
			else
			{
				scsi_sense = ptok(sp->reqp->scsi_sense);
			}

			bcopy((caddr_t)rdata->sense, (caddr_t)scsi_sense,
				rdata->sense_len);

			rdata->flags |= SCSIMAP_SENSE_DATA_VALID;

			udi_buf_free(sense_buf);

			sp->reqp->target_sts = scsi_status;

			sp->reqp->sense_len = rdata->sense_len;
		}
		else
		{
			sp->reqp->host_sts = 1;
		}

		break;

	case UDI_STAT_TIMEOUT:
		/*
		 * The command was timed out by the HBA and aborted.
		 * We will try it again.
		 */

	case UDI_SCSI_STAT_ABORTED_HD_BUS_RESET:
	case UDI_SCSI_STAT_ABORTED_RMT_BUS_RESET:
	case UDI_SCSI_STAT_ABORTED_REQ_BUS_RESET:
	case UDI_SCSI_STAT_ABORTED_REQ_TGT_RESET:
		/*
		 * The spec says that these I/O's "may or may not
		 * have been started".  We will retry them.
		 */

	case UDI_SCSI_STAT_ACA_PENDING:
	case UDI_SCSI_STAT_DEVICE_PHASE_ERROR:
	case UDI_SCSI_STAT_UNEXPECTED_BUS_FREE:
	case UDI_SCSI_STAT_DEVICE_PARITY_ERROR:
		if (sp->retry_count < 4)
		{
			_OSDEP_MUTEX_LOCK(&rdata->r_lock);

			scsimap_putq(&rdata->sbq, sp, SCSIMAP_QTAIL);

			_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);

			sp->retry_count++;

			/*
			 * Return the CB resource back to the pool
			 */
			scsimap_free_io_cb(rdata, scsi_io_cb);

			scsimap_next(rdata);

			return;
		}

		sp->reqp->host_sts = 1;

		break;

	case UDI_STAT_ABORTED:
		/*
		 * The spec sayss that this can only happen as the result
		 * of an explicit abort request by the PD.  Since we don't
		 * send aborts, this must be some sort of error.
		 */

	case UDI_SCSI_STAT_SELECTION_TIMEOUT:
	case UDI_SCSI_STAT_LINK_FAILURE:
	case UDI_STAT_NOT_UNDERSTOOD:
	case UDI_STAT_HW_PROBLEM:
	default:
		sp->reqp->host_sts = 1;

		break;
	}

	(*sp->reqp->io_intr)(sp->reqp);

	_OSDEP_MEM_FREE(sp);

	/*
	 * Return the CB resource back to the pool
	 */
	scsimap_free_io_cb(rdata, scsi_io_cb);

	scsimap_next(rdata);
}

void
scsimap_mem_free(void *context, udi_ubit8_t *space, udi_size_t size)
{
	/* DO NOTHING */
	/* This memory should not be free'd. This is external memory */
}

_udi_xbops_t scsimap_xbops = {&scsimap_mem_free, NULL, NULL};

/*
 * Try acquiring resources.
 *
 * For an SCB_TYPE or ISCB_TYPE requests the resources needed are
 * scsi control block and udi_buf_t.
 * For SFB_TYPE requests only scsi control block resource is needed.
 * Since we have only one alloc token per region and we may have 
 * number of i/o requests outstanding on one region we have to 
 * serialize resource allocation on alloc token.
 */ 
STATIC void
scsimap_next(
scsimap_region_data_t *rdata
)
{
	scsimap_sblk_t		*sp;
	udi_scsi_io_cb_t	*scsi_io_cb;
#ifdef SCSIMAP_DEBUG
	unsigned		i;

	udi_debug_printf("scsimap_next: rdata=0x%x\n", rdata);
#endif

	sp = scsimap_get_next_best_job(rdata);
	
	if (sp == (scsimap_sblk_t *) NULL)
	{
#ifdef SCSIMAP_DEBUG
		udi_debug_printf("scsimap_next: nothing to do\n");
#endif

		return;
	}

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("scsimap_next: processing sp=0x%x reqp=0x%x\n",
		sp, sp->reqp);
#endif

	_OSDEP_MUTEX_LOCK(&rdata->r_lock);

	if (sp->reqp->opcode == SENSE_CMD)
	{
		/*
		 * If the scsi command is REQUEST SENSE and sense data is valid
		 * then call scsimap_sense ()
		 */

		if (rdata->flags & SCSIMAP_SENSE_DATA_VALID)
		{
			rdata->flags &= ~SCSIMAP_SENSE_DATA_VALID;

			_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);

			scsimap_sense(sp);

			scsimap_next(rdata);

			return;
		}

		_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);
	}

	scsi_io_cb = scsimap_get_io_cb(rdata);

	if (scsi_io_cb == NULL)
	{
		/*
		 * put the job back on the alloc queue and return
		 */
		UDI_ENQUEUE_TAIL(&rdata->alloc_q, &sp->link);

		_OSDEP_MUTEX_LOCK(&rdata->r_lock);

		return;
	}

	_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);

	UDI_GCB(scsi_io_cb)->initiator_context = sp;

	sp->cb = scsi_io_cb;

	/*
	 * udi_buf_t resource is needed only for SCB_TYPE and ISCB_TYPE
	 * requests.
	 */
	if (sp->reqp->data_len)
	{
		_udi_buf_extern_init(scsimap_send_io_req,
			UDI_GCB(scsi_io_cb),
			(void *)ptok(sp->reqp->data_ptr),
			(udi_size_t)sp->reqp->data_len,
			&scsimap_xbops, NULL, rdata->buf_path);
	}
	else
	{
		scsimap_send_io_req(UDI_GCB(scsi_io_cb), NULL);
	}
}

/*
 * Call back function for udi_buf_t resource allocation.
 */
STATIC void
scsimap_send_io_req(udi_cb_t *gcb, udi_buf_t *new_buf)
{
	scsimap_sblk_t		*sp;
	udi_scsi_io_cb_t	*scsi_io_cb;

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("scsimap_send_io_req: new_buf=0x%x\n", new_buf);
#endif

	sp = gcb->initiator_context;

	scsi_io_cb = sp->cb;

	ASSERT(new_buf->buf_size == sp->sbp->sb.SCB.sc_datasz);	

	scsi_io_cb->data_buf = new_buf;

	scsimap_cmd(sp);

	scsimap_next(sp->rdata);
}

STATIC udi_scsi_io_cb_t *
scsimap_get_io_cb(
scsimap_region_data_t	*rdata
)
{
	udi_queue_t		*elem;
	scsimap_cb_res_t	*cb_res;

	if (!UDI_QUEUE_EMPTY(&rdata->cb_res_pool))
	{
		elem = UDI_DEQUEUE_HEAD(&rdata->cb_res_pool);

		cb_res = UDI_BASE_STRUCT(elem, scsimap_cb_res_t, link);

		((udi_scsi_io_cb_t *)cb_res->cb)->data_buf = NULL;

		return cb_res->cb;
	}

	return NULL;
}

/*
 * UDI entry point for scsi_ctl_ack
 */
STATIC void
scsimap_scsi_ctl_ack(
udi_scsi_ctl_cb_t	*scsi_ctl_cb,
udi_status_t		status
)
{
	scsimap_region_data_t	*rdata;
	scsimap_sblk_t		*sp;

	rdata = UDI_GCB(scsi_ctl_cb)->context; 

	sp = (scsimap_sblk_t *)UDI_GCB(scsi_ctl_cb)->initiator_context;

	switch (status)
	{
	case UDI_OK:
		sp->comp_code = 0;
		break;

	case UDI_STAT_HW_PROBLEM:
		sp->comp_code = 1;
		break;

	case UDI_STAT_NOT_UNDERSTOOD:
		sp->comp_code = 1;
		break;

	case UDI_STAT_NOT_SUPPORTED:
		sp->comp_code = 1;
		break;

	case UDI_SCSI_CTL_STAT_FAILED:
		sp->comp_code = 1;
		break;

	default:
		sp->comp_code = 1;
		break;
	}

	/* free the ctl cb */
	udi_cb_free(UDI_GCB(scsi_ctl_cb));

	scsimap_done(sp);
}

/*
 * UDI entry point for scsi_event_ind
 */
STATIC void
scsimap_scsi_event_ind(udi_scsi_event_cb_t *cb)
{
}

/*
 * UDI entry point for scsi_unbind_ack
 */
STATIC void
scsimap_scsi_unbind_ack(
udi_scsi_bind_cb_t	*scsi_bind_cb
)
{
	scsimap_region_data_t	*rdata = UDI_GCB(scsi_bind_cb)->context;

	/*
	 * Release any held CB and buffer which were used by the scsi_xfer_req
	 * support code
	 */
	udi_cb_free( UDI_GCB(scsi_bind_cb) );

	/* 
	 * Complete the devmgmt UNBIND operation.
	 */
	udi_devmgmt_ack( UDI_MCB(rdata->mgmt_cb, udi_mgmt_cb_t ), 0, UDI_OK );
}

STATIC void
scsimap_free_io_cb(scsimap_region_data_t *rdata, udi_scsi_io_cb_t *scsi_io_cb)
{
	scsimap_cb_res_t *cb_res = UDI_GCB(scsi_io_cb)->scratch;

	cb_res->cb = scsi_io_cb;

	_OSDEP_MUTEX_LOCK(&rdata->r_lock);

	UDI_ENQUEUE_TAIL(&rdata->cb_res_pool, &cb_res->link);

	_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);
}

/*
 * Enqueue sblk(sp) on the given queue.
 * If "where" is SCSIMAP_QHEAD then sp is enqueued at the head of the queue.
 * If "where" is SCSIMAP_QTAIL then sp is enqueued at the tail of the queue.
 * Queue is a doubly linked circular queue.
 */
STATIC void
scsimap_putq(scsimap_sblk_t **qp, scsimap_sblk_t *sp, int where)
{
	if (*qp != (scsimap_sblk_t *) NULL) {
		sp->next = *qp;
		sp->prev = (*qp)->prev;
		sp->next->prev = sp;
		sp->prev->next = sp;
		if (where == SCSIMAP_QHEAD)
			*qp = sp;
	} else 
		*qp = sp->next = sp->prev = sp;
}

/*
 * Call back function for udi_mem_alloc ()
 */
STATIC void
scsimap_mem_alloc_cbfn(
udi_cb_t			*gcb,
void				*mem
)
{
	scsimap_region_data_t	*rdata;
	int			c;
	int			ret_val;
	scsimap_scsi_addr_t      *sa;
	int			call_register;

	rdata = gcb->context;

	sa = &rdata->scsi_addr;

	switch (rdata->cbfn_type)
	{
	case SCSIMAP_ALLOC_BUS_STRUCT:
#ifdef SCSIMAP_DEBUG
		udi_debug_printf("SCSIMAP_ALLOC_BUS_STRUCT: c=0x%x b=0x%x\n",
			sa->c, sa->b);
#endif

		_OSDEP_MUTEX_LOCK(&scsimap_lock);

		/* 
		 * Since do_binds may come in parallel there is a chance
		 * for us to call udi_mem_alloc () to allocate a
		 * a scsimap_bus structure from more than one do_bind
		 * execution thread.
		 * So here check if we have already allocated a 
		 * scsimap_bus structure.
		 */
		if (rdata->hbap->scsimap_ha[sa->c].bus[sa->b] ==
			(scsimap_bus_t *)NULL)
		{
			/*
			 * scsimap_bus structure is not yet allocated.
			 */
			bzero((caddr_t) mem, sizeof(scsimap_bus_t));
			rdata->hbap->scsimap_ha[sa->c].bus[sa->b] = 
				(scsimap_bus_t *)mem;
			/* TODO:
			 * Initialize scsi_id to the right value.
			 * Currently this information is not given by 
			 * UDI HBA driver.
			 * So for the time being hard code it to 7.
			 */
			rdata->hbap->scsimap_ha[sa->c].bus[sa->b]->scsi_id = 7;

			_OSDEP_MUTEX_UNLOCK(&scsimap_lock);

		}
		else
		{
			_OSDEP_MUTEX_UNLOCK(&scsimap_lock);

			/*
			 * scsimap_bus structure is already allocated
			 * because of some other do_bind.
			 * Just free the memory which is allocated
			 * for scsimap_bus structure for this do_bind
			 */
			udi_mem_free(mem);
		}

		/*
		 * Allocate scsimap_target structure
		 */
		scsimap_alloc_target_struct(rdata);
		break;

	case SCSIMAP_ALLOC_TGT_STRUCT:
#ifdef SCSIMAP_DEBUG
		udi_debug_printf("SCSIMAP_ALLOC_TGT_STRUCT: c=0x%x b=0x%x "
			"t=0x%x\n", sa->c, sa->b, sa->t);
#endif

		_OSDEP_MUTEX_LOCK(&scsimap_lock);

		/* 
		 * Since do_binds may come in parallel there is a chance
		 * for us to call udi_mem_alloc() to allocate a
		 * a scsimap_target structure from more than one do_bind
		 * execution thread.
		 * So here check if we have already allocated a 
		 * scsimap_target structure.
		 */
		if (rdata->hbap->scsimap_ha[sa->c].bus[sa->b]
			->target[sa->t] == (scsimap_target_t *)NULL)
		{
			/*
			 * scsimap_target structure is not yet allocated.
			 */
			bzero((caddr_t) mem, sizeof(scsimap_target_t));
			rdata->hbap->scsimap_ha[sa->c].bus[sa->b]->
				target[sa->t]=
				(scsimap_target_t *)mem;

			_OSDEP_MUTEX_UNLOCK(&scsimap_lock);

		}
		else
		{
			_OSDEP_MUTEX_UNLOCK(&scsimap_lock);
			/*
			 * scsimap_target structure is already allocated
			 * because of some other do_bind.
			 * Just free the memory which is allocated
			 * for scsimap_bus structure for this do_bind
			 */
			udi_mem_free(mem);
		}

		/*
		 * Allocate scsimap_lun structure
		 */
		scsimap_alloc_lun_struct(rdata);
		break;

	case SCSIMAP_ALLOC_LUN_STRUCT:

#ifdef SCSIMAP_DEBUG
		udi_debug_printf("SCSIMAP_ALLOC_LUN_STRUCT: c=0x%x b=0x%x "
			"t=0x%x\n", sa->c, sa->b, sa->t, sa->l);
#endif

		bzero((caddr_t) mem, sizeof(scsimap_lun_t));

		rdata->hbap->scsimap_ha[sa->c].bus[sa->b]->target[sa->t]->
			lun[sa->l] = (scsimap_lun_t *)mem;

		rdata->hbap->scsimap_ha[sa->c].bus[sa->b]->target[sa->t]->
			lun[sa->l]->rdata = rdata;

		/*
		 * We have completed the bind processing.
		 * Acknowledge the bind.
		 */
		udi_channel_event_complete(
			UDI_MCB(rdata->mgmt_cb, udi_channel_event_cb_t),
			UDI_OK);

		/*
		 * Only call ha_register on the last child (target) for
		 * this HBA instance, since we don't get a second chance.
		 */
		_OSDEP_MUTEX_LOCK(&scsimap_lock);

		if (++rdata->hbap->scsimap_ha[sa->c].num_children_bound ==
				rdata->hbap->scsimap_ha[sa->c].num_children)
		{
			call_register = 1;
		}
		else
		{
			call_register = 0;
		}

		_OSDEP_MUTEX_UNLOCK(&scsimap_lock);

		if (call_register)
		{
			scsimap_ha_register(rdata, sa->c);
		}

		break;

	default:
#ifdef SCSIMAP_DEBUG
		udi_debug_printf("unkown alloc type\n");

		calldebug();
#endif

		break;
	}
}

/*
 * Allocate a scsimap_bus structure if needed.
 */
STATIC void
scsimap_alloc_bus_struct(
scsimap_region_data_t *rdata
)
{
	scsimap_scsi_addr_t      *sa;

	sa = &rdata->scsi_addr;

	_OSDEP_MUTEX_LOCK(&scsimap_lock);

	if (sa->b >= MAX_BUS)
	{
		/*
		 * Failed to allocate a controller number.
		 */
		SCSIMAP_ASSERT(0, "[scsimap_alloc_bus_struct] Number of "
			"buses exceeded MAX_BUS");
	}

	if (rdata->hbap->scsimap_ha[sa->c].bus[sa->b] == (scsimap_bus_t *)NULL)
	{
		rdata->cbfn_type = SCSIMAP_ALLOC_BUS_STRUCT;

		_OSDEP_MUTEX_UNLOCK(&scsimap_lock);

		udi_mem_alloc(scsimap_mem_alloc_cbfn,
			UDI_GCB(rdata->scsi_bind_cb),
			UDI_SIZEOF(scsimap_bus_t), 0);
	}
	else
	{
		/*
		 * scsimap_bus structure is already allocated.
		 * Now allocate scsimap_target structure if needed.
		 */
		_OSDEP_MUTEX_UNLOCK(&scsimap_lock);

		scsimap_alloc_target_struct(rdata);
	}
}

int	uM_sc_init_done;

scsimap_hba_t *
scsimap_find_hba(
char	*name
)
{
	scsimap_hba_t		*hbap;
	udi_queue_t		*temp_elem;
	udi_queue_t		*elem;

	UDI_QUEUE_FOREACH(&scsimap_hba_list, elem, temp_elem)
	{
		hbap = (scsimap_hba_t *)elem;

		if (udi_strcmp(name, hbap->name) == 0)
		{
			return hbap;
		}
	}

	return NULL;
}


scsimap_region_data_t   *
scsimap_get_rdata(
char		*name,
ulong_t		c,
ulong_t		b,
ulong_t		t,
ulong_t		l
)
{
	scsimap_region_data_t   *rdata;
	scsimap_hba_t		*hbap;

	hbap = scsimap_find_hba(name);

	if (hbap == NULL)
	{
		return NULL;
	}

	return SCSIMAP_GET_RDATA(c, b, t, l);
}


/*
 * OSDI entry point for all requests.
 */
int
uscsi_entry(
char	*name,
REQ_IO	*reqp
)
{
	scsimap_region_data_t   *rdata;
	scsi_disk_info		*sdiptr;
	ulong_t		  c,b,t,l;
	
	reqp->host_sts = reqp->target_sts = 0;

	if (uM_sc_init_done == 0)
	{
		_udi_driver_init();

		uM_sc_init_done = 1;
	}

	c = scsimap_gtol[reqp->ha_num];
	b = reqp->bus_num;
	t = reqp->id;
	l = reqp->lun;

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("uscsi_entry c=0x%x b=0x%x t=0x%x l=0x%x "
		"reqp=0x%x io_intr=0x%x bp=0x%x\n", c, b, t, l, reqp,
		reqp->io_intr, reqp->rbuf);
#endif

	rdata = scsimap_get_rdata(name, c, b, t, l);

	/*
	 * Return error if the sp is destined to a device that does not
	 * exist.
	 */
	if (rdata == (scsimap_region_data_t *)NULL)
	{
		if (reqp->req_type != SCSI_INIT || 
			reqp->hacmd == SCSI_DISK_INFO)
		{
			reqp->host_sts = 1;

			return -1;
		}
	}
	else
	{
		/*
		 * Skip the host adapter itself.
		 */
#define ID_PROCESOR	0X03
#define SCSIMAP_ID_TYPE(inq)		(inq[0] & 0x1F)

		if (SCSIMAP_ID_TYPE(rdata->inquiry_data) == ID_PROCESOR)
		{
			reqp->host_sts = 1;

			return -1;
		}
	}

	switch (reqp->req_type)
	{
	case SCSI_SEND_NC:
	case SCSI_SEND:
		return scsimap_process_req(rdata, reqp);

	case SCSI_INIT:
#ifdef SCSIMAP_DEBUG
		udi_debug_printf("SCSI_INIT ");
#endif

		if (reqp->ext_p)
		{
		    sdiptr = (scsi_disk_info *) reqp->ext_p->data;
		    sdiptr->magic = SCSI_V3_MAGIC;
		    sdiptr->v_maj = SCSI_V3_VERSION;
		    sdiptr->v_min = 0;
		}

		switch (reqp->hacmd)
		{
		case SCSI_DISK_INFO:
			reqp->ext_p->type = SCSI_DISK_INFO;
			reqp->ext_p->next = NULL; 
			/*
			 * Fake BIOS-compatability geometry.
			 */
			sdiptr->cylin = 0;
			sdiptr->heads = 255;
			sdiptr->sectors = 63;
			break;
		}

#ifdef SCSIMAP_DEBUG
		udi_debug_printf("done\n");
#endif

		break;

	case SCSI_INFO: /* Return the host adapter capabilities. */
#ifdef SCSIMAP_DEBUG
		udi_debug_printf("SCSI_INFO ");
#endif

		bcopy((caddr_t)rdata->hbap->scsimap_ha[c].hainfo,
			(caddr_t)ptok(reqp->data_ptr),
			reqp->data_len);

#ifdef SCSIMAP_DEBUG
		udi_debug_printf("done\n");
#endif

		break;
#ifdef NOTYET
	case SCSI_DEV_RESET: /* Reset a specific target ID. */
		break;
#endif
	default:	
		return -1;
	}

	return 0;
}

extern struct scsi_info	*scsi_info_head;

int uscsiintr()
{
}

STATIC void
scsimap_ha_register(
scsimap_region_data_t	*rdata,
ulong_t			local_cntl
)
{
	struct scsi_info	*sinfop;
	_udi_channel_t		*channelp;
	_udi_channel_t		*ochannelp;
	_udi_region_t		*regionp;
	_udi_driver_t		*drvp;

	channelp = _UDI_GCB_TO_CHAN(UDI_GCB(rdata->scsi_bind_cb));

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("channelp = 0x%x\n", channelp);
#endif

	ochannelp = channelp->other_end;

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("ochannelp = 0x%x\n", ochannelp);
#endif

	regionp = ochannelp->chan_region;

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("regionp = 0x%x\n", regionp);
#endif

	drvp = regionp->reg_driver;

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("drvp = 0x%x\n", drvp);
#endif

	/*
	 * In OSR5, we get local ha_num, so map one-to-one.
	 */
	scsimap_gtol[local_cntl] = local_cntl;
	scsimap_ltog[local_cntl] = local_cntl;

	for (sinfop = scsi_info_head ; sinfop != NULL ; sinfop = sinfop->next)
	{
#ifdef SCSIMAP_DEBUG
		udi_debug_printf("sinfop->drvp = 0x%x\n", sinfop->drvp);
#endif

		if (sinfop->drvp == drvp)
		{
			rdata->hbap->scsimap_ha[local_cntl].hareg =
				&sinfop->regp;

			rdata->hbap->scsimap_ha[local_cntl].hainfo =
				&sinfop->infop;
		}
	}
}

/*
 * Process a normal I/O request.
 */
STATIC int
scsimap_process_req(scsimap_region_data_t *rdata, REQ_IO *reqp)
{
	scsimap_sblk_t	*sp;
	unsigned	i;

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("scsimap_process_req: reqp = 0x%x CMD: [", reqp);
#endif
	
	for (i = 0 ; i < reqp->cmdlen ; i++)
	{
#ifdef SCSIMAP_DEBUG
		udi_debug_printf("%02x ", reqp->scsi_cmd.raw[i]);
#endif
	}

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("]\n");
#endif

	/*
	 * Allocate a per-request structure to hold the rest of the
	 * info we need.
	 */
	sp = _OSDEP_MEM_ALLOC(sizeof *sp, 0, 1);
	sp->reqp = reqp;
	sp->rdata = rdata;
	sp->retry_count = 0;

	/*
	 * Enqueue sp. And try allocating resources for the
	 * i/o request. The resources we need are scsi control
	 * block and udi_buf_t. After the resources are 
	 * acquired scsimap_cmd() will be called.
	 */
	scsimap_send_req(rdata, sp);

	return 0;
}


/*
 * Allocate scsimap_lun structure.
 */
STATIC void
scsimap_alloc_lun_struct(scsimap_region_data_t *rdata)
{
	scsimap_scsi_addr_t	*sa;

	sa = &rdata->scsi_addr;

	_OSDEP_MUTEX_LOCK(&scsimap_lock);

	if (sa->l >= MAX_EXLUS)
	{
		/*
		 * Failed to allocate a controller number.
		 */
		SCSIMAP_ASSERT(0, "[scsimap_alloc_lun_struct] Number of "
			"luns exceeded MAX_EXLUS");
	}

	rdata->cbfn_type = SCSIMAP_ALLOC_LUN_STRUCT;

	_OSDEP_MUTEX_UNLOCK(&scsimap_lock);

	udi_mem_alloc(scsimap_mem_alloc_cbfn,
		UDI_GCB(rdata->scsi_bind_cb),
		UDI_SIZEOF(scsimap_lun_t), 0);
}

/*
 * Allocate a scsimap_target structure if needed.
 */
STATIC void
scsimap_alloc_target_struct(scsimap_region_data_t *rdata)
{
	scsimap_scsi_addr_t      *sa;

	sa = &rdata->scsi_addr;

	_OSDEP_MUTEX_LOCK(&scsimap_lock);

	if (sa->t >= MAX_EXTCS)
	{
		/*
		 * Failed to allocate a controller number.
		 */
		SCSIMAP_ASSERT(0, "[scsimap_alloc_target_struct] Number of "
			"targets exceeded MAX_EXTCS");
	}

	if (rdata->hbap->scsimap_ha[sa->c].bus[sa->b]->target[sa->t] == 
		(scsimap_target_t *)NULL)
	{
		rdata->cbfn_type = SCSIMAP_ALLOC_TGT_STRUCT;

		_OSDEP_MUTEX_UNLOCK(&scsimap_lock);

		udi_mem_alloc(scsimap_mem_alloc_cbfn,
			UDI_GCB(rdata->scsi_bind_cb),
			UDI_SIZEOF(scsimap_target_t), 0);
	}
	else
	{
		/*
		 * scsimap_target structure is already allocated.
		 * Now allocate scsimap_lun structure.
		 */
		_OSDEP_MUTEX_UNLOCK(&scsimap_lock);

		scsimap_alloc_lun_struct(rdata);
	}
}

STATIC void
scsimap_hba_alloc_cbfn(
udi_cb_t		*gcb,
void			*mem
)
{
	udi_queue_t		*child_list;
	udi_queue_t		*temp_elem;
	udi_queue_t		*elem;
	_udi_dev_node_t		*parent_node;
	_udi_channel_t		*chan;
	scsimap_region_data_t	*rdata;
	scsimap_hba_t		*hbap;
	char			*name;

	_OSDEP_MUTEX_LOCK(&scsimap_lock);

	hbap = (scsimap_hba_t *)mem;

	UDI_ENQUEUE_TAIL(&scsimap_hba_list, (udi_queue_t *)hbap);

	chan = (_udi_channel_t *)gcb->channel;

	name = (char *)chan->other_end->chan_region->reg_driver->drv_name;

	parent_node = (_udi_dev_node_t *)
		chan->chan_region->reg_node->parent_node;

	rdata = gcb->context; 

	/*
	 * Got either an unused controller number or
	 * a controller number is already allocated for
	 * this HBA instance.
	 */
	rdata->scsi_addr.c = 0;

	rdata->hbap = hbap;

	hbap->name = name;

	/*
	 * Count the number of children on this node.
	 */
	hbap->scsimap_ha[0].num_children = 0;

	hbap->scsimap_ha[0].num_children_bound = 0;

	child_list = &((_udi_dev_node_t *)parent_node)->child_list;

	UDI_QUEUE_FOREACH(child_list, elem, temp_elem)
	{
		hbap->scsimap_ha[0].num_children++;
	}

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("scsimap_hba_alloc_cbfn: num_children = 0x%x\n",
		hbap->scsimap_ha[0].num_children);
#endif
	hbap->scsimap_ha[0].parent_node = parent_node;

	_OSDEP_MUTEX_UNLOCK(&scsimap_lock);
}

/*
 * Assign a scsimap_ha structure from the global array of scsimap_ha[] 
 * structures if scsimap_ha structure is not already allocated
 * for this HBA instance.
 * The index into the scsimap_ha[] array is our local controller
 * number.
 */
STATIC void
scsimap_assign_ha_struct(
scsimap_region_data_t	*rdata
)
{
	int			c;
	_udi_channel_t		*chan;
	_udi_dev_node_t		*parent_node;
	udi_cb_t		*gcb;
	scsimap_hba_t		*hbap;
	char			*name;

	_OSDEP_MUTEX_LOCK(&scsimap_lock);

	gcb = UDI_GCB(rdata->scsi_bind_cb);

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("scsimap_assign_ha_struct: gcb = 0x%x\n", gcb);
#endif

	chan = (_udi_channel_t *)gcb->channel;

	name = (char *)chan->other_end->chan_region->reg_driver->drv_name;

#ifdef SCSIMAP_DEBUG
	udi_debug_printf("scsimap_assign_ha_struct: chan = 0x%x\n", chan);
	udi_debug_printf("scsimap_assign_ha_struct: chan->chan_region = "
		"0x%x\n", chan->chan_region);
	udi_debug_printf("scsimap_assign_ha_struct: chan->chan_region->reg_node"
		"= 0x%x\n", chan->chan_region->reg_node);
	udi_debug_printf("scsimap_assign_ha_struct: chan->chan_region->"
		"reg_node->parent_node = 0x%x\n",
		chan->chan_region->reg_node->parent_node);
#endif

	parent_node = (_udi_dev_node_t *)
		chan->chan_region->reg_node->parent_node;

	hbap = scsimap_find_hba(name);

	if (hbap == NULL)
	{
#ifdef SCSIMAP_DEBUG
		udi_debug_printf("creating hba for %s\n", name);
#endif
		_OSDEP_MUTEX_UNLOCK(&scsimap_lock);

		udi_mem_alloc(scsimap_hba_alloc_cbfn,
			UDI_GCB(rdata->scsi_bind_cb),
			UDI_SIZEOF(scsimap_hba_t), 0);

		return;
	}

	/*
	 * If we have already allocated a controller number for this
	 * HBA instance then find it.
	 * If we have not already allocated a controller number for
	 * this HBA instance then allocate an unused controller
	 * number for this HBA instance.
	 */
	for (c = 0; c < MAX_EXHAS && hbap->scsimap_ha[c].parent_node != NULL
		&& hbap->scsimap_ha[c].parent_node != parent_node; c++)
	{
		continue;
	}

	if (c < MAX_EXHAS)
	{
		rdata->scsi_addr.c = c;

		rdata->hbap = hbap;
	}
	else 
	{
		/*
		 * Failed to allocate a controller number.
		 */
		SCSIMAP_ASSERT(0, "[scsimap_assign_ha_struct] Number of "
			"controllers exceeded MAX_EXHAS");
	}

	_OSDEP_MUTEX_UNLOCK(&scsimap_lock);
}


STATIC void
scsimap_init(
)
{
#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_init\n");
#endif
	UDI_QUEUE_INIT(&scsimap_hba_list);
}

/*
 * allocate static data
 */
STATIC void
scsimap_usage_ind(
udi_usage_cb_t	*cb,
udi_ubit8_t	resource_level
)
{
#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_init\n");
#endif

	if (!scsimap_init_done)
	{
		_OSDEP_ATOMIC_INT_INIT(&scsimap_instance_counter, 0);

		scsimap_init_done = TRUE;
	}

	_OSDEP_ATOMIC_INT_INCR((caddr_t)&scsimap_instance_counter);

	if (_OSDEP_ATOMIC_INT_READ(&scsimap_instance_counter) == 1)
	{
		scsimap_init();
	}

	udi_static_usage(cb, resource_level);
}
