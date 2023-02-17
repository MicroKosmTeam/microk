/*
 * ========================================================================
 * FILE: scsimap.h
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

#define SCSIMAP_MAX_SENSE_LEN           SENSE_LEN
#define SCSIMAP_MAX_QUEUE_DEPTH         8
#define SCSIMAP_INQ_LEN                 36

#define SCSIMAP_MAX_IO_CBS	16 /* Maximum number if pre-allocated I/O CBs */

/*
 * Input: Pointer to Inquiry data
 * Returns:
 *      non-zero        if wide (16 or 32 bit) data transfer is supported.
 *      zero            if wide data transfer is not supported.
 */
#define SCSIMAP_IS_WDTR(inq)            (inq[7] & 0x60)

/*
 * Input: Pointer to Inquiry data
 * Returns:
 *      non-zero        if synchronous data transfer is supported.
 *      zero            if synchronous data transfer is not supported.
 */
#define SCSIMAP_IS_SDTR(inq)            (inq[7] & 0x10)

/*
 * Input: Pointer to Inquiry data
 * Returns:
 *      non-zero        if command queuing is supported.
 *      zero            if command queuing is not supported.
 */
#define SCSIMAP_IS_CMDQ(inq)            (inq[7] & 0x02)

/*
 * "where" parameter of scsimap_sblk_enqueue() and scsimap_sblk_dequeue ()
 *  takes one of the following values.
 */
#define SCSIMAP_QHEAD                   0
#define SCSIMAP_QTAIL                   1

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

#define SCSIMAP_ALLOC_BUS_STRUCT        0
#define SCSIMAP_ALLOC_TGT_STRUCT        1
#define SCSIMAP_ALLOC_LUN_STRUCT        2

#define UDI_SIZEOF(x)                   ((udi_size_t)sizeof(x))

/*
 * Defines for the "res_held" field of scsimap_sblk structure
 */
#define SCSIMAP_RES_SCSI_CB             1
#define SCSIMAP_RES_BUF                 2

/*
 * Defines for the "flags" field of scsimap_region_data structure
 */
#define SCSIMAP_SENSE_DATA_VALID        1
#define SCSIMAP_SUSPEND                 2

/*
 * Defines for the "flags" field of scsimap_ha structure
 */
#define SCSIMAP_TIMEOUT_OFF             1

#define ATOKEN(rdata)                   ((rdata)->init_context.first_token)
#define SCSIMAP_INIT_ATOKEN(rdata, context, func, flags)        \
        {                                                       \
                ATOKEN(rdata)->call_context = (void *)(context);\
                ATOKEN(rdata)->errorf = (func);                 \
        } 


#ifdef SCSIMAP_DEBUG
#define SCSIMAP_ASSERT(invariant, msg)  {                                 \
                                            if (!(invariant))             \
                                                cmn_err (CE_PANIC, (msg));\
                                        }
#else
#define SCSIMAP_ASSERT(invariant, msg)  
#endif

/* 
 * In the following structure the types of c, b, t, and l are
 * u_longs in order to support larger address space of fibre
 * channel.
 */
typedef struct scsimap_scsi_addr {
        u_long                  c;      /* local controller number */
        u_long                  b;      /* bus number */
        u_long                  t;      /* target id */
        u_long                  l;      /* lun id */
} scsimap_scsi_addr_t;

typedef struct scsimap_cb_res {
	udi_queue_t	link;
	void		*cb;
} scsimap_cb_res_t;

/*
 * Region data 
 */
typedef struct scsimap_region_data {
        udi_init_context_t      init_context;
	udi_queue_t		link;
	udi_scsi_bind_cb_t	*scsi_bind_cb;
	udi_cb_t		*mgmt_cb; /* hold the mgmt cb */

        scsimap_scsi_addr_t      scsi_addr;

	udi_boolean_t	multi_lun; /* Multi-lun PD */
	udi_ubit32_t	max_buses; /* max number of buses for this hba */
	udi_ubit32_t	max_targets; /* max number of targets for this hba */
	udi_ubit32_t	max_luns; /* max number of luns for this hba */

        /*
         * HBA driver's scratch size requirement. 
         * HBA driver passes this information during scsi_bind_ack.
         */
        udi_size_t              hba_scratch_size;

        udi_ubit8_t             inquiry_data[SCSIMAP_INQ_LEN];
        udi_ubit8_t             cbfn_type;
        struct scsimap_sblk	*sbq;
        int                     flags;
	udi_queue_t		alloc_q;
	udi_queue_t		reqp_q;

	udi_index_t		num_cbs; /* Number of pre-allocated CBs */

        /*
         * Sense data.
         * Valid only if SCSIMAP_SENSE_DATA_VALID bit is set in "flags".
         */
        udi_ubit8_t             sense [SCSIMAP_MAX_SENSE_LEN];
        udi_size_t              sense_len;
	udi_queue_t		cb_res_pool;	/* I/O CB pool */
	udi_channel_event_cb_t	*channel_event_cb;

        /*
         * atoken_owner, if non-null, indicates that an allocation 
         * operation is in progress using the alloc token on behalf of 
         * the atoken_owner.
         */
        struct scsimap_sblk	*atoken_owner;
	udi_buf_path_t		buf_path;
	_OSDEP_MUTEX_T		r_lock;
	struct _scsimap_hba	*hbap;
} scsimap_region_data_t;

typedef struct scsimap_lun {
        scsimap_region_data_t   *rdata;
} scsimap_lun_t;

#define MAX_EXLUS NAD
typedef struct scsimap_target {
        scsimap_lun_t           *lun[MAX_EXLUS];
} scsimap_target_t;

#define MAX_EXTCS NADCTLS
typedef struct scsimap_bus {
        scsimap_target_t        *target[MAX_EXTCS];
        u_long                  scsi_id; /* scsi target id of the host */
} scsimap_bus_t;

#define MAX_EXHAS	16

#define MAX_BUS	4
typedef struct scsimap_ha {
        scsimap_bus_t           *bus[MAX_BUS];
	HAINFO			*hainfo;
	struct Shareg_ex	*hareg;
        int                     flags;
        int                     num_children;
	int			num_children_bound;
	int			synchronous;
	_udi_dev_node_t		*parent_node;	/* ptr to parent node */
	void			*completion; /* synchronous completion event */
} scsimap_ha_t;

typedef struct _scsimap_hba {
	udi_queue_t		Q;
	char			*name;
	scsimap_ha_t		scsimap_ha[MAX_EXHAS];
} scsimap_hba_t;
        
/*
 * Per-request structure.
 */
typedef struct scsimap_sblk {
	REQ_IO			*reqp;
        scsimap_region_data_t	*rdata;
        struct scsimap_sblk	*next;
        struct scsimap_sblk	*prev;
        void			*cb;
        int			res_held;
	udi_queue_t		link;
	int			comp_code;
	int			retry_count;
} scsimap_sblk_t;

/*
 * Get pointer to scsimap_bus structure given controller number and bus
 * number.
 */
#define SCSIMAP_GET_BUSP(c, b)                                                \
        ((c != -1) ? (scsimap_ha[c].bus[b]) : ((scsimap_bus_t *) NULL))

/*
 * Get rdata associated with controller number 'c', bus number 'b'
 * target number 't' and lun number 'l'.
 * NULL is given if any of c, b, t or l is invalid.
 */
#define SCSIMAP_GET_RDATA(c, b, t, l)                                         \
        ((c != -1 &&                                                          \
            hbap->scsimap_ha[c].bus[b] != (scsimap_bus_t *) NULL &&           \
            hbap->scsimap_ha[c].bus[b]->target[t] !=			      \
		(scsimap_target_t *) NULL &&				      \
            hbap->scsimap_ha[c].bus[b]->target[t]->lun[l] !=                  \
		(scsimap_lun_t *) NULL)					      \
                ? hbap->scsimap_ha[c].bus[b]->target[t]->lun[l]->rdata        \
                : (scsimap_region_data_t *) NULL)

