
/* 
 * File: mapper/common/scsi/scsimap.c
 *
 * Description: SCSI mapper, acts as a UDI PD driver and provides HBA 
 * functionality to the OS.
 *
 * Contact: udi-tech@zk3.dec.com 
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

#define UDI_SCSI_VERSION 0x101
#include <udi_env.h>
#include <udi_scsi.h>

#include <scsimap.h>

/* 
 * this is the common code for the scsi mapper
 * osdep code needs to implement the following functions:
 *
 *      void osdep_scsimap_device_add(scsimap_region_data_t*)
 *                      register the device with the OS
 *      void osdep_scsimap_device_rm(scsimap_region_data_t*)
 *                      de-register the device with the OS
 *      udi_boolean_t osdep_scsimap_hba_detach(scsimap_hba_t*)
 *                      de-register the host adapter with the OS
 *      void osdep_scsimap_region_cleanup(scsimap_region_data_t*)
 *                      perform osdep per_region cleanup
 *	void osdep_scsimap_init()
 *	  	        perform static initialization
 *	void osdep_scsimap_deinit()
 *	                perform static de-initialization
 *      void osdep_scsimap_io_ack(scsimap_req_t*)
 *                      communicate successful completion of a SCSI I/O request 
 *	                to the OS
 *      void osdep_scsimap_io_nak(scsimap_req_t*)
 *                      communicate error during SCSI I/O request to the OS
 *      void osdep_scsimap_ctl_ack(scsimap_req_t*)
 *                      communicate completion of a SCSI control request to the 
 *	                OS
 *      void osdep_scsimap_sense(scsimap_req_t*, void* sense_data)
 *                      transfer sense data to the OS
 *      void osdep_scsimap_inquiry(scsimap_re_t*, void* inquiry_data)
 *                      transfer inquiry data to the OS
 *      _udi_xbops_t osdep_scsimap_xbops
 *                      an operations vector used to free, dma-map and
 *                      dma-unmap mapper memory
 */

STATIC void osdep_scsimap_device_add(scsimap_region_data_t * rdata);
STATIC void osdep_scsimap_device_rm(scsimap_region_data_t * rdata);
STATIC udi_boolean_t osdep_scsimap_hba_detach(scsimap_hba_t * hba);
STATIC void osdep_scsimap_io_ack(scsimap_req_t * req);
STATIC void osdep_scsimap_io_nak(scsimap_req_t * req,
				 udi_status_t req_status,
				 udi_ubit8_t scsi_status,
				 udi_ubit8_t sense_status);
STATIC void osdep_scsimap_sense(scsimap_req_t * req,
				udi_ubit8_t *sense);
STATIC void osdep_scsimap_inquiry(scsimap_req_t * req,
				  udi_ubit8_t *inq);
STATIC void osdep_scsimap_ctl_ack(scsimap_req_t * req,
				  udi_status_t status);
STATIC void osdep_scsimap_init(void);
STATIC void osdep_scsimap_deinit(void);

STATIC void osdep_scsimap_dma_free(void *,
				   udi_ubit8_t *,
				   udi_size_t);
STATIC void *osdep_scsimap_dma_map(void *,
				   udi_ubit8_t *,
				   udi_size_t,
				   _udi_dma_handle_t *);
STATIC void osdep_scsimap_dma_unmap(void *,
				    udi_ubit8_t *,
				    udi_size_t,
				    _udi_dma_handle_t *);

_udi_xbops_t osdep_scsimap_xbops = {
	osdep_scsimap_dma_free,
	osdep_scsimap_dma_map,
	osdep_scsimap_dma_unmap
};

 /*
  * this mapper "exports" the following functions that should be used from the 
  * osdep mapper code:
  *
  *     STATIC int scsimap_send_req(scsimap_req_t* req)
  *             queues the request <req> with the HBA driver and returns 
  *             immediatley
  */

#define SCSIMAP_SCSI_META	1

#define UDI_SCSI_MAX_CDB_LEN	12

#define SCSI_OP_IDX		1

#define SCSI_BIND_CB_IDX	1
#define SCSI_IO_CB_IDX		2
#define SCSI_CTL_CB_IDX		3
#define SCSI_EVENT_CB_IDX	4

#define SCSIMAP_MGMT_SCRATCH	0
#define SCSI_BIND_SCRATCH	sizeof(scsimap_bind_scratch_t)
#define SCSI_IO_SCRATCH		sizeof(scsimap_cb_res_t)+64
#define SCSI_CTL_SCRATCH	0
#define SCSI_EVENT_SCRATCH	0

/* MA interface routines */
STATIC udi_usage_ind_op_t _scsimap_usage_ind;
STATIC udi_enumerate_req_op_t _scsimap_enumerate_req;
STATIC udi_devmgmt_req_op_t _scsimap_devmgmt_req;
STATIC udi_final_cleanup_req_op_t _scsimap_final_cleanup_req;

/* SCSI Interface routines */
STATIC udi_channel_event_ind_op_t _scsimap_scsi_channel_event_ind;
STATIC udi_scsi_bind_ack_op_t _scsimap_scsi_bind_ack;
STATIC udi_scsi_unbind_ack_op_t _scsimap_scsi_unbind_ack;
STATIC udi_scsi_io_ack_op_t _scsimap_scsi_io_ack;
STATIC udi_scsi_io_nak_op_t _scsimap_scsi_io_nak;
STATIC udi_scsi_ctl_ack_op_t _scsimap_scsi_ctl_ack;
STATIC udi_scsi_event_ind_op_t _scsimap_scsi_event_ind;
STATIC udi_usage_ind_op_t _scsimap_usage_ind;

STATIC udi_mgmt_ops_t scsimap_mgmt_ops = {
	_scsimap_usage_ind,		/* usage_ind */
	_scsimap_enumerate_req,		/* enumerate_req */
	_scsimap_devmgmt_req,		/* devmgmt_req */
	_scsimap_final_cleanup_req,	/* final_cleanup_req */
};

/* 
 * "Bottom-side" entry points for the SCSI Metalanguage.
 */
STATIC udi_scsi_pd_ops_t scsimap_scsi_pd_ops = {
	_scsimap_scsi_channel_event_ind,
	_scsimap_scsi_bind_ack,
	_scsimap_scsi_unbind_ack,
	_scsimap_scsi_io_ack,
	_scsimap_scsi_io_nak,
	_scsimap_scsi_ctl_ack,
	_scsimap_scsi_event_ind
};

/*
 * default set of op_flags
 */
STATIC udi_ubit8_t scsimap_default_op_flags[] = {
	0, 0, 0, 0, 0, 0, 0
};

/* 
 * ========================================================================
 * main initialization structure
 *
 * Tasks for this operation:
 *      + Initialize primary region properties
 *      + Register the channel ops vectors for each metalanguage
 *        used in this driver (module).
 *      + Register control block properties
 */

/* 
 * -----------------------------------------------------------------------------
 * Management operations init section:
 * -----------------------------------------------------------------------------
 */
STATIC udi_primary_init_t scsimap_primary_init = {
	&scsimap_mgmt_ops,
	scsimap_default_op_flags,	/* op_flags */
	SCSIMAP_MGMT_SCRATCH,		/* mgmt_scratch_size */
	0,				/* enumeration attr list length */
	sizeof (scsimap_region_data_t),
	0,				/* child data size */
	1,				/* path_handles */
};

/* 
 * -----------------------------------------------------------------------------
 * Meta operations init section:
 * -----------------------------------------------------------------------------
 */
STATIC udi_ops_init_t scsimap_ops_init_list[] = {
	{
	 SCSI_OP_IDX,
	 SCSIMAP_SCSI_META,		/* meta index [from udiprops.txt] */
	 UDI_SCSI_PD_OPS_NUM,
	 0,				/* Channel context size */
	 (udi_ops_vector_t *)&scsimap_scsi_pd_ops,
	 scsimap_default_op_flags	/* op_flags */
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
STATIC udi_cb_init_t scsimap_cb_init_list[] = {
	{
	 SCSI_BIND_CB_IDX,		/* SCSI Bind CB         */
	 SCSIMAP_SCSI_META,		/* from udiprops.txt    */
	 UDI_SCSI_BIND_CB_NUM,		/* meta cb_num          */
	 SCSI_BIND_SCRATCH,		/* scratch requirement  */
	 0,				/* inline size          */
	 NULL				/* inline layout        */
	 }
	,
	{
	 SCSI_IO_CB_IDX,		/* SCSI i/o CB          */
	 SCSIMAP_SCSI_META,		/* from udiprops.txt    */
	 UDI_SCSI_IO_CB_NUM,		/* meta cb_num          */
	 SCSI_IO_SCRATCH,		/* scratch requirement  */
	 UDI_SCSI_MAX_CDB_LEN,		/* inline size          */
	 NULL				/* inline layout        */
	 }
	,
	{
	 SCSI_CTL_CB_IDX,		/* SCSI ctl CB          */
	 SCSIMAP_SCSI_META,		/* from udiprops.txt    */
	 UDI_SCSI_CTL_CB_NUM,		/* meta cb_num          */
	 SCSI_CTL_SCRATCH,		/* scratch requirement  */
	 0,				/* inline size          */
	 NULL				/* inline layout        */
	 }
	,
	{
	 SCSI_EVENT_CB_IDX,		/* SCSI event CB        */
	 SCSIMAP_SCSI_META,		/* from udiprops.txt    */
	 UDI_SCSI_EVENT_CB_NUM,		/* meta cb_num          */
	 SCSI_EVENT_SCRATCH,		/* scratch requirement  */
	 0,				/* inline size          */
	 NULL				/* inline layout        */
	 }
	,
	{
	 0				/* Terminator */
	 }
};

udi_init_t udi_init_info = {
	&scsimap_primary_init,
	NULL,				/* Secondary init list */
	scsimap_ops_init_list,
	scsimap_cb_init_list,
	NULL,				/* gcb init list */
	NULL,				/* cb select list */
};

STATIC void scsimap_init(void);
STATIC void scsimap_bind_to_parent(udi_cb_t *);
STATIC void scsimap_alloc_io_req_cb_cbfn(udi_cb_t *,
					 udi_cb_t *);
STATIC void scsimap_get_instance_attr(udi_cb_t *,
				      udi_instance_attr_type_t,
				      udi_size_t);
STATIC void scsimap_func(scsimap_region_data_t *,
			 scsimap_req_t *);
STATIC void scsimap_scsi_bind_start(udi_scsi_bind_cb_t *);
STATIC void scsimap_send_io_req(udi_cb_t *,
				udi_buf_t *);
STATIC void scsimap_send_ctl_req(udi_cb_t *,
				 udi_cb_t *);
STATIC scsimap_hba_t *scsimap_hba_alloc(scsimap_region_data_t * rdata);
STATIC void scsimap_hba_attach(scsimap_hba_t *,
			       scsimap_region_data_t *);
STATIC void scsimap_hba_detach(scsimap_hba_t *,
			       scsimap_region_data_t *);
STATIC void scsimap_putq(scsimap_region_data_t *,
			 scsimap_req_t *);
STATIC void scsimap_next(scsimap_region_data_t *);
STATIC udi_scsi_io_cb_t *scsimap_get_io_cb(scsimap_region_data_t *);
STATIC void scsimap_free_io_cb(scsimap_region_data_t *,
			       udi_scsi_io_cb_t *);

STATIC udi_boolean_t scsimap_init_done = FALSE;
STATIC udi_queue_t scsimap_hba_list;
STATIC _OSDEP_MUTEX_T scsimap_hba_list_lock;
STATIC _OSDEP_ATOMIC_INT_T scsimap_instance_counter;

/* 
 * send a scsi request
 */
STATIC int
scsimap_send_req(scsimap_req_t * req)
{
	struct scsimap_scsi_addr *sa = &req->addr;
	scsimap_hba_t *hba;
	scsimap_region_data_t *rdata;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_send_req: req=%p\n", req);
#endif
	udi_assert(sa);

	hba = req->hba;

	rdata = VALID_RDATA_INDEX(hba, sa->scsi_bus, sa->scsi_target,
				  sa->scsi_lun) ? SCSIMAP_GET_RDATA(hba,
								    sa->
								    scsi_bus,
								    sa->
								    scsi_target,
								    sa->
								    scsi_lun) :
		NULL;

	if (rdata == (scsimap_region_data_t *) NULL ||
	    (hba->hba_rdata->scsi_addr.scsi_bus == sa->scsi_bus &&
	     hba->hba_rdata->scsi_addr.scsi_target == sa->scsi_target &&
	     hba->hba_rdata->scsi_addr.scsi_lun == sa->scsi_lun)) 
		return SCSIMAP_NO_SUCH_DEVICE;

	req->rdata = rdata;

	scsimap_putq(rdata, req);
	scsimap_next(rdata);
	return SCSIMAP_OK;
}

STATIC void
scsimap_device_add_complete(scsimap_region_data_t * rdata)
{
	udi_channel_event_complete(UDI_MCB(rdata->mgmt_cb,
					   udi_channel_event_cb_t),
				   UDI_OK);
}

/* 
 * UDI entry point for scsi_channel_event
 */
STATIC void
scsimap_scsi_channel_event_ind(udi_channel_event_cb_t *channel_event_cb)
{
#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_scsi_channel_event_ind\n");
#endif
	switch (channel_event_cb->event) {

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
 *      + Save info from the bind_to_parent_req control block for later use.
 *      + Get attributes from device node and save them in region data.
 */
STATIC void
scsimap_bind_to_parent(udi_cb_t *gcb)
{
	scsimap_region_data_t *rdata = gcb->context;
	udi_channel_event_cb_t *channel_event_cb =
		UDI_MCB(gcb, udi_channel_event_cb_t);
	/*
	 * Use the pre-allocated bind cb 
	 */
	udi_scsi_bind_cb_t *scsi_bind_cb =
		UDI_MCB(channel_event_cb->params.internal_bound.bind_cb,
			udi_scsi_bind_cb_t);

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_bind_to_parent\n");
#endif
	rdata->mgmt_cb = gcb;		/* save the udi_channel_event_cb_t */
	rdata->scsi_bind_cb = scsi_bind_cb;	/* save the bind cb */

	rdata->buf_path =
		channel_event_cb->params.parent_bound.path_handles[0];

	UDI_QUEUE_INIT(&rdata->device_link);
	UDI_QUEUE_INIT(&rdata->alloc_q);
	UDI_QUEUE_INIT(&rdata->io_q);
	UDI_QUEUE_INIT(&rdata->iio_q);
	UDI_QUEUE_INIT(&rdata->ctl_q);
	UDI_QUEUE_INIT(&rdata->cb_res_pool);
	_OSDEP_MUTEX_INIT(&rdata->r_lock, "rdata lock");
	rdata->num_cbs = 0;

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
scsimap_get_instance_attr(udi_cb_t *gcb,
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
		/*
		 * check if this is the HBA device 
		 */
		rdata->cbfn_type = SCSIMAP_GET_MULTI_LUN_ATTR;
		udi_instance_attr_get(scsimap_get_instance_attr,
				      gcb, "scsi_multi_lun", 0,
				      (void *)&rdata->multi_lun,
				      sizeof (rdata->multi_lun));
		break;
	case SCSIMAP_GET_MULTI_LUN_ATTR:
		/*
		 * check if this is the HBA device 
		 */
		if (rdata->multi_lun) {
			/*
			 * Get other attributes associated with multi-lun 
			 */
			rdata->cbfn_type = SCSIMAP_GET_MAX_BUSES_ATTR;
			udi_instance_attr_get(scsimap_get_instance_attr,
					      gcb, "scsi_max_buses", 0,
					      (void *)&rdata->max_buses,
					      sizeof (rdata->max_buses));
		} else {
			/*
			 * Not a multi-lun device 
			 */
			rdata->cbfn_type = SCSIMAP_GET_BUS_ATTR;
			udi_instance_attr_get(scsimap_get_instance_attr,
					      gcb, "scsi_bus", 0,
					      (void *)&rdata->scsi_addr.
					      scsi_bus,
					      sizeof (rdata->scsi_addr.
						      scsi_bus));
		}
		break;
	case SCSIMAP_GET_MAX_BUSES_ATTR:
		udi_assert(sizeof (rdata->max_buses) == actual_length);
		rdata->cbfn_type = SCSIMAP_GET_MAX_TGTS_ATTR;
		udi_instance_attr_get(scsimap_get_instance_attr,
				      gcb, "scsi_max_tgts", 0,
				      (void *)&rdata->max_targets,
				      sizeof (rdata->max_targets));
		break;
	case SCSIMAP_GET_MAX_TGTS_ATTR:
		udi_assert(sizeof (rdata->max_targets) == actual_length);
		rdata->cbfn_type = SCSIMAP_GET_MAX_LUNS_ATTR;
		udi_instance_attr_get(scsimap_get_instance_attr,
				      gcb, "scsi_max_luns", 0,
				      (void *)&rdata->max_luns,
				      sizeof (rdata->max_luns));
		break;
	case SCSIMAP_GET_MAX_LUNS_ATTR:
		udi_assert(sizeof (rdata->max_luns) == actual_length);

		/*
		 * Initialize the HBA data structure here 
		 */
		osdep_scsimap_device_add(rdata);
		break;
	case SCSIMAP_GET_BUS_ATTR:
		udi_assert(sizeof (rdata->scsi_addr.scsi_bus) ==
			   actual_length);
		rdata->cbfn_type = SCSIMAP_GET_TGT_ATTR;
		udi_instance_attr_get(scsimap_get_instance_attr,
				      gcb, "scsi_target", 0,
				      (void *)&rdata->scsi_addr.scsi_target,
				      sizeof (rdata->scsi_addr.scsi_target));
		break;
	case SCSIMAP_GET_TGT_ATTR:
		udi_assert(sizeof (rdata->scsi_addr.scsi_target) ==
			   actual_length);
		rdata->cbfn_type = SCSIMAP_GET_LUN_ATTR;
		udi_instance_attr_get(scsimap_get_instance_attr,
				      gcb, "scsi_lun", 0,
				      (void *)bind_scratch->lun_attr,
				      sizeof (rdata->scsi_addr.scsi_lun));
		break;

	case SCSIMAP_GET_LUN_ATTR:
		udi_assert(sizeof (bind_scratch->lun_attr) == actual_length);

		/*
		 * Just use the second byte out of the SAM-2 style LUN struct
		 */
		rdata->scsi_addr.scsi_lun = bind_scratch->lun_attr[1];
		rdata->cbfn_type = SCSIMAP_GET_INQ_ATTR;
		udi_instance_attr_get(scsimap_get_instance_attr,
				      gcb, "scsi_inquiry", 0,
				      (void *)&rdata->inquiry_data[0],
				      sizeof (rdata->inquiry_data));
		break;

	case SCSIMAP_GET_INQ_ATTR:
		udi_assert(sizeof (rdata->inquiry_data) == actual_length);
		/*
		 * Start the bind sequence
		 */
		scsimap_scsi_bind_start(UDI_MCB(gcb, udi_scsi_bind_cb_t));

		break;

	default:
		break;
	}
}

/* 
 * Initializes the control block and calls udi_scsi_bind_req ().
 */
STATIC void
scsimap_scsi_bind_start(udi_scsi_bind_cb_t *scsi_bind_cb)
{
	scsimap_region_data_t *rdata = UDI_GCB(scsi_bind_cb)->context;
	udi_ubit16_t queue_depth;
#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_scsi_bind_start\n");
#endif

#ifdef NOTYET
	/*
	 * (TODO) set some events here?
	 */

	/*
	 * If the scsi devices supports command queuing then set
	 * the queue_depth to SCSIMAP_MAX_QUEUE_DEPTH otherwise set
	 * the queue depth to 1.
	 */
	scsi_bind_cb->bind_req.queue_depth =
		(SCSIMAP_IS_CMDQ(rdata->inquiry_data)) ?
		SCSIMAP_MAX_QUEUE_DEPTH : 1;

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
#else  /* NOTYET */

	scsi_bind_cb->events = 0;
#endif
	/*
	 * Send a bind request to the HBA driver 
	 */
	queue_depth = (SCSIMAP_IS_CMDQ(rdata->inquiry_data)) ?
		SCSIMAP_MAX_QUEUE_DEPTH : 1;
	udi_scsi_bind_req(scsi_bind_cb,
			  UDI_SCSI_BIND_EXCLUSIVE,
			  queue_depth, SCSIMAP_MAX_SENSE_LEN, 0);
}

/* 
 * UDI entry point for scsi_bind_ack
 */
STATIC void
scsimap_scsi_bind_ack(udi_scsi_bind_cb_t *scsi_bind_cb,
		      udi_ubit32_t hd_timeout_increase,
		      udi_status_t status)
{
	scsimap_region_data_t *rdata = UDI_GCB(scsi_bind_cb)->context;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_scsi_bind_ack: status=0x%x\n", status);
#endif

	if (status != UDI_OK) {
		/*
		 * (TODO) Log the error 
		 */

		/*
		 * free the bind cb 
		 */
		udi_cb_free(UDI_GCB(scsi_bind_cb));

		/*
		 * Complete the channel event 
		 */
		udi_channel_event_complete(UDI_MCB(rdata->mgmt_cb,
						   udi_channel_event_cb_t),
					   UDI_STAT_CANNOT_BIND);

		return;
	}

	/*
	 * These are per device values 
	 */
	/*
	 * rdata->max_temp_bind_excl = max_temp_bind_excl; 
	 */
	rdata->hd_timeout_increase = hd_timeout_increase;

	/*
	 * We keep track of the new <scsi_bind_cb> so that the
	 * OS-specific mapper can have a handle to use when it needs to
	 * allocates its own control blocks.
	 * Note that the scsi_bind_cb may have been reallocated so we
	 * need to recapture it here.
	 */
	rdata->scsi_bind_cb = scsi_bind_cb;

	/*
	 * Allocate a pool of I/O CBs 
	 */

	udi_cb_alloc_batch(scsimap_alloc_io_req_cb_cbfn,
			   UDI_GCB(rdata->scsi_bind_cb),
			   SCSI_IO_CB_IDX, SCSIMAP_MAX_IO_CBS, FALSE, 0, NULL);
}

/* 
 * Call back function for scsi control block allocation.
 */
STATIC void
scsimap_alloc_io_req_cb_cbfn(udi_cb_t *gcb,
			     udi_cb_t *new_cb)
{
	scsimap_region_data_t *rdata = gcb->context;
	udi_scsi_io_cb_t *scsi_io_cb;
	scsimap_cb_res_t *cb_res;
	udi_index_t i;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_alloc_io_req_cb_cbfn\n");
#endif

	for (i = 0; i < SCSIMAP_MAX_IO_CBS; i++) {
		scsi_io_cb = UDI_MCB(new_cb, udi_scsi_io_cb_t);

		cb_res = UDI_GCB(scsi_io_cb)->scratch;
		cb_res->cb = scsi_io_cb;
		UDI_ENQUEUE_TAIL(&rdata->cb_res_pool, &cb_res->link);
		new_cb = new_cb->initiator_context;
	}

	rdata->num_cbs = SCSIMAP_MAX_IO_CBS;

	osdep_scsimap_device_add(rdata);

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("leaving scsimap_scsi_bind_ack\n");
#endif
}

/* 
 * UDI entry point for scsi_unbind_ack
 */
STATIC void
scsimap_scsi_unbind_ack(udi_scsi_bind_cb_t *scsi_bind_cb)
{
	scsimap_region_data_t *rdata = UDI_GCB(scsi_bind_cb)->context;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_scsi_unbind_ack\n");
#endif

	/*
	 * Release any held CB and buffer which were used by the scsi_xfer_req
	 * support code
	 */
	udi_cb_free(UDI_GCB(scsi_bind_cb));

	osdep_scsimap_device_rm(rdata);
}

STATIC void
scsimap_device_rm_complete(scsimap_region_data_t * rdata)
{
	/*
	 * Complete the devmgmt UNBIND operation.
	 */
	udi_devmgmt_ack(UDI_MCB(rdata->mgmt_cb, udi_mgmt_cb_t),
			0,
			UDI_OK);
}

/* 
 * UDI entry point for scsi_io_ack
 */
STATIC void
scsimap_scsi_io_ack(udi_scsi_io_cb_t *scsi_io_cb)
{
	scsimap_region_data_t *rdata = UDI_GCB(scsi_io_cb)->context;
	scsimap_req_t *req =
		(scsimap_req_t *) scsi_io_cb->gcb.initiator_context;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_scsi_io_ack\n");
#endif
	/*
	 * Invalidate the sense data, it is no longer valid 
	 */
	rdata->r_flags &= ~SCSIMAP_RDATA_SENSE_VALID;

	if (scsi_io_cb->data_buf != NULL && req->dataptr) {
		udi_assert(scsi_io_cb->data_buf->buf_size == req->datasize);
		req->residual = req->datasize - scsi_io_cb->data_buf->buf_size;
		udi_buf_free(scsi_io_cb->data_buf);
	}

	osdep_scsimap_io_ack(req);

	_OSDEP_MUTEX_LOCK(&rdata->r_lock);

	/*
	 * Return the CB resource back to the pool 
	 */
	scsimap_free_io_cb(rdata, scsi_io_cb);

	/*
	 * send next request 
	 */
	scsimap_next(rdata);
}

/* 
 * UDI entry point for scsi_io_nak
 */
STATIC void
scsimap_scsi_io_nak(udi_scsi_io_cb_t *scsi_io_cb,
		    udi_status_t req_status,
		    udi_ubit8_t scsi_status,
		    udi_ubit8_t sense_status,
		    udi_buf_t *sense_buf)
{
	scsimap_region_data_t *rdata = UDI_GCB(scsi_io_cb)->context;
	scsimap_req_t *req =
		(scsimap_req_t *) UDI_GCB(scsi_io_cb)->initiator_context;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_scsi_io_nak\n");
#endif

	/*
	 * Sense data is no longer valid.
	 */
#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("invalidating sense data for rdata=%p\n", rdata);
#endif
	rdata->r_flags &= ~SCSIMAP_RDATA_SENSE_VALID;

	if (scsi_io_cb->data_buf != NULL) {
		udi_buf_free(scsi_io_cb->data_buf);
		req->residual = req->datasize - scsi_io_cb->data_buf->buf_size;
	}

	if (req_status == UDI_SCSI_STAT_NONZERO_STATUS_BYTE) {
		if (scsi_status == SCSIMAP_STATUS_CHK_CON) {
#ifdef SCSIMAP_DEBUG
			_OSDEP_PRINTF
				("SCSI status is check condition, sense data is %ssupplied.\n",
				 sense_buf != NULL ? "" : "not ");
#endif
			/*
			 * Copy sense data to rdata->sense
			 */
			udi_buf_read(sense_buf, 0, rdata->sense_len,
				     rdata->sense);
			rdata->sense_len = sense_buf->buf_size;
			rdata->r_flags |= SCSIMAP_RDATA_SENSE_VALID;

			/*
			 * HBA driver allocates sense_buf and we are
			 * supposed to free the sense_buf.
			 */
			udi_buf_free(sense_buf);
		}
	}

	osdep_scsimap_io_nak(req, req_status, scsi_status, sense_status);

	_OSDEP_MUTEX_LOCK(&rdata->r_lock);

	/*
	 * Return the CB resource back to the pool 
	 */
	scsimap_free_io_cb(rdata, scsi_io_cb);

	scsimap_next(rdata);
}

/* 
 * UDI entry point for scsi_event_ind
 */
STATIC void
scsimap_scsi_event_ind(udi_scsi_event_cb_t *cb)
{
#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_scsi_event_ind\n");
#endif

}

/*
 * STATIC void
 * scsimap_next (scsimap_region_data_t *rdata)
 *
 * Description:
 * Attempt to send the next job on the device queue.
 *
 * Try acquiring resources.
 *
 * For an SCSIMAP_IO_TYPE or SCSIMAP_IIO_TYPE requests the resources needed are
 * scsi control block and udi_buf_t.
 * For SCSIMAP_CTL_TYPE requests only scsi control block resource is needed.
 *
 * Calling/Exit State:
 *  rdata->r_lock is held on entry.
 *  None on exit.
 */
STATIC void
scsimap_next(scsimap_region_data_t * rdata)
{
	scsimap_req_t *req;
	udi_scsi_io_cb_t *scsi_io_cb;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_next\n");
#endif
	req = SCSIMAP_GET_NEXT_JOB(rdata);
	if (req == NULL) {
		_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);
		return;
	}

	if (req->type == SCSIMAP_CTL_TYPE) {
		_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);
		scsimap_func(rdata, req);
		_OSDEP_MUTEX_LOCK(&rdata->r_lock);
		scsimap_next(rdata);
		return;
	}

	if (*((udi_ubit8_t *)req->command) == SCSIMAP_REQUEST_SENSE) {

		/*
		 * If the scsi command is REQUEST SENSE and sense data is valid
		 * then call osdep_scsimap_sense ()
		 */
		if (rdata->r_flags & SCSIMAP_RDATA_SENSE_VALID) {
			rdata->r_flags &= ~SCSIMAP_RDATA_SENSE_VALID;
			_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);
			osdep_scsimap_sense(req, rdata->sense);
			_OSDEP_MUTEX_LOCK(&rdata->r_lock);
			scsimap_next(rdata);
			return;
		}
	}

	if (*((udi_ubit8_t *)req->command) == SCSIMAP_INQUIRY) {
		_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);
		osdep_scsimap_inquiry(req, rdata->inquiry_data);
		_OSDEP_MUTEX_LOCK(&rdata->r_lock);
		scsimap_next(rdata);
		return;
	}

	scsi_io_cb = scsimap_get_io_cb(rdata);
	if (scsi_io_cb == NULL) {

		/*
		 * put the job back on the alloc queue and return 
		 */
		UDI_ENQUEUE_TAIL(&rdata->alloc_q, &req->link);
		_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);
		return;
	}

	_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);

	/*
	 * Now we have the request and the control block.
	 * Send the request
	 */

	UDI_GCB(scsi_io_cb)->initiator_context = req;

	/*
	 * Initialize cb and call udi_scsi_io_req () 
	 */
	scsi_io_cb->flags = 0;
	if (req->datasize) {
		scsi_io_cb->flags = req->mode;
	}

	/*
	 * Copy command and bytes from req->command to cdb 
	 */
	scsi_io_cb->cdb_len = req->cmdsize;
	udi_memset(scsi_io_cb->cdb_ptr, 0, UDI_SCSI_MAX_CDB_LEN);
	udi_memcpy(scsi_io_cb->cdb_ptr, req->command, req->cmdsize);

	if (req->timeout == 0 ||
	    !(rdata->hba->hba_flags & SCSIMAP_HBA_TIMEOUT_ON))

		/*
		 * (TODO) Is '0' the correct value for not timing ?
		 */
		scsi_io_cb->timeout = 0;
	else
		scsi_io_cb->timeout =
			req->timeout + rdata->hd_timeout_increase;

	/*
	 * (TODO) set the other flag values
	 * get the correct attribute value
	 */
	scsi_io_cb->attribute = UDI_SCSI_SIMPLE_TASK;

	/*
	 * the osdep code has to implement dma_map operations for this
	 * buffer so it can efficiently map the memory to DMA while still 
	 * satisfying the DMA constraints for this HBA
	 */
	if (req->datasize) {
		_udi_buf_extern_init(scsimap_send_io_req, UDI_GCB(scsi_io_cb),
				     req->dataptr, req->datasize,
				     &osdep_scsimap_xbops, req->context,
				     rdata->buf_path);
	} else {
		scsimap_send_io_req(UDI_GCB(scsi_io_cb), NULL);
	}
}

STATIC void
scsimap_func(scsimap_region_data_t * rdata,
	     scsimap_req_t * req)
{
	udi_cb_t *gcb = UDI_GCB(rdata->scsi_bind_cb);
	scsimap_bind_scratch_t *bind_scratch = gcb->scratch;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_func: rdata=%p, req=%p\n", rdata, req);
#endif

	/*
	 * Save the req 
	 */
	bind_scratch->req = req;

	/*
	 * Allocate a control CB and issue the control command 
	 */
	udi_cb_alloc(scsimap_send_ctl_req, gcb, SCSI_CTL_CB_IDX, gcb->channel);
}

/* 
 * Call back function for allocating a control block for scsi ctl req.
 * Initialize the control block and call udi_scsi_ctl_req ().
 */
STATIC void
scsimap_send_ctl_req(udi_cb_t *gcb,
		     udi_cb_t *new_cb)
{
	udi_scsi_ctl_cb_t *scsi_ctl_cb = UDI_MCB(new_cb, udi_scsi_ctl_cb_t);
	scsimap_req_t *req = ((scsimap_bind_scratch_t *) gcb->scratch)->req;

#ifdef SCSIMAP_DEBUG
	scsimap_region_data_t *rdata = gcb->context;
	scsimap_scsi_addr_t *sa = &rdata->scsi_addr;

	_OSDEP_PRINTF
		("scsimap_send_ctl_req: gcb=%p, new_cb=%p, req=%p, rdata=%p, sa=%p (%d,%d,%d)\n",
		 gcb, new_cb, req, rdata, sa, (udi_ubit32_t)sa->scsi_bus,
		 (udi_ubit32_t)sa->scsi_target, (udi_ubit32_t)sa->scsi_lun);
#endif

	/*
	 * Initialize cb and send UDI_SCSI_CTL_RESET_DEVICE
	 * control request to the HBA instance.
	 */
	UDI_GCB(scsi_ctl_cb)->initiator_context = req;

	scsi_ctl_cb->ctrl_func = req->func;

	udi_scsi_ctl_req(scsi_ctl_cb);
}

/* 
 * UDI entry point for scsi_ctl_ack
 */
STATIC void
scsimap_scsi_ctl_ack(udi_scsi_ctl_cb_t *scsi_ctl_cb,
		     udi_status_t status)
{
	scsimap_req_t *req =
		(scsimap_req_t *) UDI_GCB(scsi_ctl_cb)->initiator_context;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_scsi_ctl_ack\n");
#endif

	osdep_scsimap_ctl_ack(req, status);

	/*
	 * free the ctl cb 
	 */
	udi_cb_free(UDI_GCB(scsi_ctl_cb));
}

/* 
 * This routine is called by the resource allocation code after
 * resources (scsi control block and udi_buf_t) are acquired
 * for the i/o request specified by req.
 * This is also a call back function for udi_buf_t resource allocation.
 */
STATIC void
scsimap_send_io_req(udi_cb_t *gcb,
		    udi_buf_t *new_buf)
{
	scsimap_region_data_t *rdata = gcb->context;
	scsimap_req_t *req = gcb->initiator_context;
	udi_scsi_io_cb_t *scsi_io_cb = UDI_MCB(gcb, udi_scsi_io_cb_t);

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF
		("scsimap_send_io_req: gcb=%p, new_buf=%p, rdata=%p, req=%p, cb=%p, cb->flags=%x cb->attribute=%x\n",
		 gcb, new_buf, rdata, req, scsi_io_cb, scsi_io_cb->flags,
		 scsi_io_cb->attribute);
#endif

	ASSERT((new_buf == NULL) || (new_buf->buf_size == req->datasize));

	scsi_io_cb->data_buf = new_buf;
	udi_scsi_io_req(scsi_io_cb);

	_OSDEP_MUTEX_LOCK(&rdata->r_lock);
	scsimap_next(rdata);
}

/* 
 * Calling/Exit State:
 *  rdata->r_lock is held on entry.
 *  rdata->r_lock is held on exit.
 */
STATIC udi_scsi_io_cb_t *
scsimap_get_io_cb(scsimap_region_data_t * rdata)
{
	udi_queue_t *elem;
	scsimap_cb_res_t *cb_res;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_get_io_cb\n");
#endif

	if (!UDI_QUEUE_EMPTY(&rdata->cb_res_pool)) {
		elem = UDI_DEQUEUE_HEAD(&rdata->cb_res_pool);
		cb_res = UDI_BASE_STRUCT(elem, scsimap_cb_res_t, link);
		return cb_res->cb;
	}
	return NULL;
}

/* 
 * Calling/Exit State:
 *  rdata->r_lock is held on entry.
 *  rdata->r_lock is held on exit.
 */
STATIC void
scsimap_free_io_cb(scsimap_region_data_t * rdata,
		   udi_scsi_io_cb_t *scsi_io_cb)
{
	scsimap_cb_res_t *cb_res = UDI_GCB(scsi_io_cb)->scratch;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_free_io_cb\n");
#endif

	cb_res->cb = scsi_io_cb;

	UDI_ENQUEUE_TAIL(&rdata->cb_res_pool, &cb_res->link);
}


STATIC void
scsimap_devmgmt_req(udi_mgmt_cb_t *cb,
		    udi_ubit8_t mgmt_op,
		    udi_ubit8_t parent_id)
{
	scsimap_region_data_t *rdata = UDI_GCB(cb)->context;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_devmgmt_req\n");
#endif

	switch (mgmt_op) {
	case UDI_DMGMT_UNBIND:
		/*
		 * unbind from parent 
		 */
		rdata->mgmt_cb = UDI_GCB(cb);
		if (rdata->multi_lun) {
			/*
			 * If this is a multi-lun PD, simply call bind_ack 
			 */
			scsimap_scsi_unbind_ack(rdata->scsi_bind_cb);
		} else {
			udi_scsi_unbind_req(rdata->scsi_bind_cb);
		}
		/*
		 * unbind_ack will do the devmgmt_ack 
		 */
		return;
	default:
		break;
	}
	udi_devmgmt_ack(cb, 0, UDI_OK);

}

STATIC void
scsimap_final_cleanup_req(udi_mgmt_cb_t *cb)
{
	scsimap_region_data_t *rdata = UDI_GCB(cb)->context;

	/*
	 * (FIXME) inspect and see if any region-local or other data needs to be
	 * released here. 
	 */

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_final_cleanup_req\n");
#endif

	osdep_scsimap_region_cleanup(rdata);

	/*
	 * This block of code should rellay be moved into os specific part 
	 */
	_OSDEP_ATOMIC_INT_DECR(&scsimap_instance_counter);
	if (_OSDEP_ATOMIC_INT_READ(&scsimap_instance_counter) == 0) {
#ifdef NOTYET
		/*
		 * Note that this routine is called for each instance. We can't
		 * stop the daemon or free the resources until all the instances
		 * are gone and we are sure that this is our final moment.
		 */
		_OSDEP_MUTEX_DEINIT(&scsimap_hba_list_lock);

		osdep_scsimap_deinit();
#endif /* NOTYET */
	}

	_OSDEP_MUTEX_DEINIT(&rdata->r_lock);

	udi_final_cleanup_ack(cb);
}

/* 
 * Enqueue request on appropriate queue depending on request type.
 *
 * Calling/Exit State:
 *  None on entry
 *  rdata->r_lock is held on exit.
 */
STATIC void
scsimap_putq(scsimap_region_data_t * rdata,
	     scsimap_req_t * req)
{
#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_putq\n");
#endif

	_OSDEP_MUTEX_LOCK(&rdata->r_lock);
	if (req->type == SCSIMAP_IO_TYPE) {
		UDI_ENQUEUE_TAIL(&rdata->io_q, &req->link);
	} else if (req->type == SCSIMAP_IIO_TYPE) {
		UDI_ENQUEUE_TAIL(&rdata->iio_q, &req->link);
	} else {
		UDI_ENQUEUE_TAIL(&rdata->ctl_q, &req->link);
	}
}

STATIC void
scsimap_init()
{
#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_init\n");
#endif
	UDI_QUEUE_INIT(&scsimap_hba_list);
	_OSDEP_MUTEX_INIT(&scsimap_hba_list_lock, "scsimap hba list lock");

	osdep_scsimap_init();
}

/*
 * allocate static data
 */
STATIC void
scsimap_usage_ind(udi_usage_cb_t *cb,
		  udi_ubit8_t resource_level)
{
#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_usage_ind\n");
#endif

	/*
	 * This code should really be moved into os specific part 
	 */
	if (!scsimap_init_done) {
		_OSDEP_ATOMIC_INT_INIT(&scsimap_instance_counter, 0);
		scsimap_init_done = TRUE;
	}
	_OSDEP_ATOMIC_INT_INCR(&scsimap_instance_counter);
	if (_OSDEP_ATOMIC_INT_READ(&scsimap_instance_counter) == 1) {
		scsimap_init();
	}

	osdep_scsimap_region_init(rdata);

	udi_static_usage(cb, resource_level);
}

STATIC void
scsimap_enumerate_req(udi_enumerate_cb_t *cb,
		      udi_ubit8_t enumeration_level)
{
	udi_enumerate_no_children(cb, enumeration_level);
}
