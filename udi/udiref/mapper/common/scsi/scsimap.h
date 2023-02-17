
/*
 * File: mapper/common/scsi/scsimap.h
 *
 * SCSI mapper private decls and defines.
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

#include <udi.h>
#include <udi_env.h>
#include <udi_scsi.h>

#define SCSIMAP_MAX_SENSE_LEN		255
#define SCSIMAP_MAX_QUEUE_DEPTH		8
#define SCSIMAP_INQ_LEN			36

#define SCSIMAP_MAX_IO_CBS		16	/* Maximum number of pre-allocated I/O CBs */

#define SCSIMAP_NO_SUCH_DEVICE	-1

#define SCSIMAP_OK	0

#ifndef OSDEP_SCSIMAP_MAX_HBAS
#define OSDEP_SCSIMAP_MAX_HBAS 16	/* aribitrary value */
#endif

#define SCSIMAP_PROCESSOR_TYPE		0x03	/* SCSI PROCESSOR */

#define SCSIMAP_STATUS_CHK_CON		0x02	/* SCSI status CHECK CONDITION */

#define SCSIMAP_REQUEST_SENSE		0x03	/* SCSI command REQUEST SENSE */
#define SCSIMAP_INQUIRY			0x12	/* SCSI command INQUIRY */

#define SCSIMAP_CTL_TYPE		1
#define SCSIMAP_IO_TYPE			2
#define SCSIMAP_IIO_TYPE		3

/*
 * Input: Pointer to Inquiry data
 * Returns:
 *	non-zero	if wide (16 or 32 bit) data transfer is supported.
 *	zero		if wide data transfer is not supported.
 */
#define SCSIMAP_IS_WDTR(inq)		(inq[7] & 0x60)

/*
 * Input: Pointer to Inquiry data
 * Returns:
 *	non-zero	if synchronous data transfer is supported.
 *	zero		if synchronous data transfer is not supported.
 */
#define SCSIMAP_IS_SDTR(inq)		(inq[7] & 0x10)

/*
 * Input: Pointer to Inquiry data
 * Returns:
 *	non-zero	if command queuing is supported.
 *	zero		if command queuing is not supported.
 */
#define SCSIMAP_IS_CMDQ(inq)		(inq[7] & 0x02)

#define SCSIMAP_ID_TYPE(inq)		(inq[0] & 0x1F)

#define SCSIMAP_VENDOR(inq)		(&inq[8])

#define SCSIMAP_PRODUCT(inq)		(&inq[16])

#define SCSIMAP_REVISION(inq)		(&inq[32])

/*
 * "cbfn_type" field of scsimap_region_data structure takes one of the
 * following values.
 */
#define SCSIMAP_START_GET_ATTR		0
#define SCSIMAP_GET_BUS_ATTR		1
#define SCSIMAP_GET_TGT_ATTR		2
#define SCSIMAP_GET_LUN_ATTR		3
#define SCSIMAP_GET_INQ_ATTR		4
#define SCSIMAP_GET_MULTI_LUN_ATTR	5
#define SCSIMAP_GET_MAX_BUSES_ATTR	6
#define SCSIMAP_GET_MAX_TGTS_ATTR	7
#define SCSIMAP_GET_MAX_LUNS_ATTR	8

/*
 * Defines for the "r_flags" field of scsimap_region_data structure
 */
#define SCSIMAP_RDATA_ATTACHED		0x01
#define SCSIMAP_RDATA_SUSPENDED	 	0x02

#define SCSIMAP_RDATA_SENSE_VALID 	0x10

/*
 * Defines for the "hba_flags" field of scsimap_hba_data_t
 */
#define SCSIMAP_HBA_ATTACH_PENDING	0x01
#define SCSIMAP_HBA_ATTACHED		0x02

#define SCSIMAP_HBA_TIMEOUT_ON	 	0x10

typedef union {
	void *req;
	udi_ubit8_t lun_attr[8];
} scsimap_bind_scratch_t;

/*
 * A pool of control blocks
 */

typedef struct scsimap_cb_res {
	udi_queue_t link;
	void *cb;
} scsimap_cb_res_t;


/*
 * A scsi bus
 */

/* value for chan_flag */
#define SCSIMAP_CHAN_OK		0x01

typedef struct scsimap_channel {
	udi_ubit8_t chan_flag;
	udi_ubit32_t chan_id;
} scsimap_channel_t;

/*
 * A SCSI device address
 */

typedef struct scsimap_scsi_addr {
	udi_ubit32_t scsi_bus;
	udi_ubit32_t scsi_target;
	udi_ubit32_t scsi_lun;
} scsimap_scsi_addr_t;

/*
 * Encapsulate a SCSI request
 */

typedef struct scsimap_req {
	scsimap_scsi_addr_t addr;
	void *dataptr;
	udi_size_t datasize;
	udi_size_t residual;
	void *command;
	udi_size_t cmdsize;
	udi_ubit8_t mode;
	udi_ubit8_t type;
	udi_queue_t link;
	udi_ubit8_t func;
	udi_ubit32_t timeout;
	struct scsimap_region_data *rdata;
	struct scsimap_hba *hba;
	void *context;
	OSDEP_SCSIMAP_REQ_T osdep;
} scsimap_req_t;

/*
 * SCSI Mapper region data 
 */
typedef struct scsimap_region_data {
	udi_init_context_t init_context;	/* the region's initial context */
	udi_queue_t link;		/* link so rdatas are queuable */
	udi_buf_path_t buf_path;
	struct scsimap_hba *hba;	/* ptr to this devices' hostadapter */
	udi_scsi_bind_cb_t *scsi_bind_cb;	/* the cb for the channel to the hba */
	udi_cb_t *mgmt_cb;		/* hold the mgmt cb */
	udi_index_t num_cbs;		/* number of pre-allocated CBs */
	udi_ubit8_t cbfn_type;		/* type of function to perform in scsimap_mem_alloc_cbfn */
	udi_ubit8_t r_flags;		/* flags for this device */
	udi_queue_t cb_res_pool;	/* I/O CB pool */
	udi_queue_t device_link;	/* a link to the next devices attached to the same HBA */
	udi_queue_t io_q;		/* a queue of io requests */
	udi_queue_t iio_q;		/* a queue of immediate requests */
	udi_queue_t ctl_q;		/* a queue of control requests */
	udi_queue_t alloc_q;		/* a queue of requests used if no more cbs are available */
	udi_ubit8_t inquiry_data[SCSIMAP_INQ_LEN];	/* the device's inquiry data */
	udi_ubit8_t sense[SCSIMAP_MAX_SENSE_LEN];
	udi_size_t sense_len;
	scsimap_scsi_addr_t scsi_addr;	/* this device's scsi address */

	/*
	 * These values are temporarily stored here until there is a better
	 * way of finding these out. These really belong in the hba struct 
	 */
	udi_boolean_t multi_lun;	/* Multi-lun PD */
	udi_ubit32_t max_buses;		/* maximum number of buses for this hba */
	udi_ubit32_t max_targets;	/* maximum number of targets for this hba */
	udi_ubit32_t max_luns;		/* maximum number of luns for this hba */

	udi_ubit32_t max_temp_bind_excl;	/* maximum number of temporary
						 * exclusive binds for this hba */
	udi_ubit32_t hd_timeout_increase;	/* extra timeout value */
	_OSDEP_MUTEX_T r_lock;		/* a region-wide lock */
	OSDEP_SCSIMAP_RDATA_T osdep;	/* os-dependent additions */
} scsimap_region_data_t;


typedef struct scsimap_hba {
	scsimap_region_data_t **hba_devices;	/* array of pointers to devices attached to this hba */
	scsimap_channel_t *hba_chan;	/* array of scsi bus structs */
	_udi_dev_node_t *parent_node;	/* ptr to the parent node */
	scsimap_region_data_t *hba_rdata;	/* rdata of the multi-lun PD */
	char *hba_drvname;		/* the name of the HBA's driver */
	udi_ubit8_t tgt_shift;		/* shift value for target */
	udi_ubit8_t bus_shift;		/* shift value for bus */
	udi_ubit8_t hba_num_devices;	/* the number of attached devs */
	udi_queue_t device_list;	/* list of devices */
	udi_queue_t link;		/* link to the next hba struct */
	udi_ubit32_t max_buses;		/* maximum number of buses for this hba */
	udi_ubit32_t max_targets;	/* maximum number of targets for this hba */
	udi_ubit16_t max_luns;		/* maximum number of luns for this hba */
	udi_ubit16_t max_temp_bind_excl;	/* maximum number of exclusive temporary
						 * binds */
	udi_ubit8_t hba_flags;		/* flags controlling the hba */
	_OSDEP_MUTEX_T hba_lock;
	OSDEP_SCSIMAP_HBA_T osdep;	/* osdep part of the struct */
} scsimap_hba_t;

#ifdef OUT
typedef struct scsimap_lun {
	scsimap_region_data_t *rdata;
} scsimap_lun_t;

typedef struct scsimap_target {
	scsimap_lun_t *lun[MAX_EXLUS];
} scsimap_target_t;

typedef struct scsimap_bus {
	scsimap_target_t *target[MAX_EXTCS];
	u_long scsi_id;			/* scsi target id of the host */
} scsimap_bus_t;
#endif

#define SCSIMAP_GET_NEXT_JOB(rdata)	\
	(!UDI_QUEUE_EMPTY(&(rdata)->alloc_q)) ? UDI_BASE_STRUCT( \
		UDI_DEQUEUE_HEAD(&(rdata)->alloc_q), scsimap_req_t, link) : \
	(!UDI_QUEUE_EMPTY(&(rdata)->ctl_q)) ? UDI_BASE_STRUCT( \
		UDI_DEQUEUE_HEAD(&(rdata)->ctl_q), scsimap_req_t, link) : \
	(!UDI_QUEUE_EMPTY(&(rdata)->iio_q)) ? UDI_BASE_STRUCT( \
		UDI_DEQUEUE_HEAD(&(rdata)->iio_q), scsimap_req_t, link) : \
	(!((rdata)->r_flags & SCSIMAP_RDATA_SUSPENDED) && \
	 !UDI_QUEUE_EMPTY(&(rdata)->io_q)) ? UDI_BASE_STRUCT( \
		UDI_DEQUEUE_HEAD(&(rdata)->io_q), scsimap_req_t, link) : NULL

#define TGTVAL(hba,t)		((t) << hba->tgt_shift)
#define BUSVAL(hba,b)		((b) << hba->bus_shift)
#define DEVOFF(hba,b,t,l)	(BUSVAL(hba,b) | TGTVAL(hba,t) | (l))

#define SCSIMAP_GET_RDATA(hba, b, t, l) hba->hba_devices[DEVOFF(hba,b,t,l)]

#define VALID_RDATA_INDEX(hba, b, t, l) (DEVOFF(hba, b, t, l) < \
                                        hba->max_luns * hba->max_targets * \
                                        hba->max_buses)

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

/*
 * This is for those mappers which would like to handle the channel ops
 * on their own.
 */
#ifndef _scsimap_usage_ind
#define _scsimap_usage_ind	scsimap_usage_ind
#endif
#ifndef _scsimap_enumerate_req
#define _scsimap_enumerate_req	scsimap_enumerate_req
#endif
#ifndef _scsimap_devmgmt_req
#define _scsimap_devmgmt_req	scsimap_devmgmt_req
#endif
#ifndef _scsimap_final_cleanup_req
#define _scsimap_final_cleanup_req scsimap_final_cleanup_req
#endif

/* SCSI Interface routines */
#ifndef _scsimap_scsi_channel_event_ind
#define _scsimap_scsi_channel_event_ind scsimap_scsi_channel_event_ind
#endif
#ifndef	_scsimap_scsi_bind_ack
#define _scsimap_scsi_bind_ack	scsimap_scsi_bind_ack
#endif
#ifndef _scsimap_scsi_unbind_ack
#define _scsimap_scsi_unbind_ack scsimap_scsi_unbind_ack
#endif
#ifndef	_scsimap_scsi_io_ack
#define _scsimap_scsi_io_ack	scsimap_scsi_io_ack
#endif
#ifndef _scsimap_scsi_io_nak
#define _scsimap_scsi_io_nak scsimap_scsi_io_nak
#endif
#ifndef _scsimap_scsi_ctl_ack
#define _scsimap_scsi_ctl_ack scsimap_scsi_ctl_ack
#endif
#ifndef _scsimap_scsi_event_ind
#define _scsimap_scsi_event_ind scsimap_scsi_event_ind
#endif
#ifndef _scsimap_usage_ind
#define _scsimap_usage_ind scsimap_usage_ind
#endif
