
/*
 * File: mapper/common/net/netmap.h
 *
 * Net mapper private decls and defines.
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

#ifndef STATIC
#define STATIC static
#endif

#ifdef DEBUG
#define	NSRMAP_DEBUG

#define	DBG_MGMT_VAL	0x1		/* Debug Management Meta        */
#define	DBG_ND_VAL	0x2		/* Debug Network Meta           */
#define	DBG_GIO_VAL	0x4		/* Debug GIO Meta               */
#define	DBG_RX_VAL	0x8		/* Debug Rx region              */
#define	DBG_TX_VAL	0x10		/* Debug Tx region              */
#define	DBG_CTL_VAL	0x20		/* Debug Control region         */
#define	DBG_OS_VAL	0x40		/* Debug OS <-> NSR interface   */

#define	DBG_INTERN1_VAL	0x1		/* Debug Internal level 1 trace */
#define	DBG_INTERN2_VAL	0x2		/* Debug Internal level 2 trace */
#define	DBG_INTERN3_VAL	0x4		/* Debug Internal level 3 trace */

#ifndef NSR_DBG_LEVEL
#define	NSR_DBG_LEVEL	0
#endif /* NSR_DBG_LEVEL */
#ifndef NSR_DBG_META
#define	NSR_DBG_META	0
#endif /* NSR_DBG_META */

STATIC udi_ubit32_t nsrmap_meta_dbg = NSR_DBG_META;
STATIC udi_ubit32_t nsrmap_dbg_lvl = NSR_DBG_LEVEL;

#define DBG_META(m, l, x)	if ((nsrmap_meta_dbg & (m)) && \
				    (nsrmap_dbg_lvl & (l))) { \
					udi_debug_printf x ; \
				}

#define	DBG_MGMT1(x)	DBG_META( DBG_MGMT_VAL, DBG_INTERN1_VAL, x )
#define	DBG_MGMT2(x)	DBG_META( DBG_MGMT_VAL, DBG_INTERN2_VAL, x )
#define	DBG_MGMT3(x)	DBG_META( DBG_MGMT_VAL, DBG_INTERN2_VAL, x )

#define	DBG_ND1(x)	DBG_META( DBG_ND_VAL, DBG_INTERN1_VAL, x )
#define	DBG_ND2(x)	DBG_META( DBG_ND_VAL, DBG_INTERN2_VAL, x )
#define	DBG_ND3(x)	DBG_META( DBG_ND_VAL, DBG_INTERN3_VAL, x )

#define	DBG_GIO1(x)	DBG_META( DBG_GIO_VAL, DBG_INTERN1_VAL, x )
#define	DBG_GIO2(x)	DBG_META( DBG_GIO_VAL, DBG_INTERN2_VAL, x )
#define	DBG_GIO3(x)	DBG_META( DBG_GIO_VAL,	DBG_INTERN3_VAL, x )

#define	DBG_RX1(x)	DBG_META( DBG_RX_VAL, DBG_INTERN1_VAL, x )
#define	DBG_RX2(x)	DBG_META( DBG_RX_VAL, DBG_INTERN2_VAL, x )
#define	DBG_RX3(x)	DBG_META( DBG_RX_VAL, DBG_INTERN3_VAL, x )

#define	DBG_TX1(x)	DBG_META( DBG_TX_VAL, DBG_INTERN1_VAL, x )
#define	DBG_TX2(x)	DBG_META( DBG_TX_VAL, DBG_INTERN2_VAL, x )
#define	DBG_TX3(x)	DBG_META( DBG_TX_VAL, DBG_INTERN3_VAL, x )

#define	DBG_CTL1(x)	DBG_META( DBG_CTL_VAL, DBG_INTERN1_VAL, x )
#define	DBG_CTL2(x)	DBG_META( DBG_CTL_VAL, DBG_INTERN2_VAL, x )
#define	DBG_CTL3(x)	DBG_META( DBG_CTL_VAL, DBG_INTERN3_VAL, x )

#define	DBG_OS1(x)	DBG_META( DBG_OS_VAL, DBG_INTERN1_VAL, x )
#define	DBG_OS2(x)	DBG_META( DBG_OS_VAL, DBG_INTERN2_VAL, x )
#define	DBG_OS3(x)	DBG_META( DBG_OS_VAL, DBG_INTERN3_VAL, x )

#else

#define	DBG_MGMT1(x)
#define	DBG_MGMT2(x)
#define	DBG_MGMT3(x)
#define	DBG_ND1(x)
#define	DBG_ND2(x)
#define	DBG_ND3(x)
#define	DBG_GIO1(x)
#define	DBG_GIO2(x)
#define	DBG_GIO3(x)
#define	DBG_RX1(x)
#define	DBG_RX2(x)
#define	DBG_RX3(x)
#define	DBG_TX1(x)
#define	DBG_TX2(x)
#define	DBG_TX3(x)
#define	DBG_CTL1(x)
#define	DBG_CTL2(x)
#define	DBG_CTL3(x)
#define	DBG_OS1(x)
#define	DBG_OS2(x)
#define	DBG_OS3(x)

#endif /* DEBUG */

#ifdef NSRMAP_DEBUG
#define	NSR_DBG( s )	(s)
#define	NSRMAP_ASSERT(invariant)    udi_assert(invariant)
#else
#define	NSR_DBG( s )
#define	NSRMAP_ASSERT(invariant)
#endif /* NSRMAP_DEBUG */

#ifdef	KERNEL
#define	UDI_MEMCPY(s1, s2, n)	udi_memcpy(s1, s2, n)
#else
#define	UDI_MEMCPY(s1, s2, n)	memcpy(s1, s2, n)
#endif /* KERNEL */

#ifndef NETMAP_OSDEP_SIZE
#define NETMAP_OSDEP_SIZE	1
typedef void netmap_osdep_t;
#endif

#ifndef NETMAP_OSDEP_TX_SIZE
#define NETMAP_OSDEP_TX_SIZE	1
typedef void netmap_osdep_tx_t;
#endif

typedef struct nsrmap_bind_info {
	udi_channel_t bind_channel;
	udi_cb_t *bind_cb;
} nsrmap_bind_info_t;

/*
 * Scratch data manipulation macros. Each Tx/Rx CB has an associated scratch
 * area which can be accessed by NSR_SET_SCRATCH / NSR_GET_SCRATCH
 */
typedef struct nsrmap_scratch_data {
	void *pcb;
	void *scratch1;
	void *scratch2;
	void *scratch3;
} nsrmap_scratch_data_t;

#define	NSR_SET_SCRATCH( s, v ) (*(void **)(s) = (v))
#define	NSR_GET_SCRATCH( s, v ) (*(void **)&(v) = *(void **)(s))

#define	NSR_SET_SCRATCH_1( s, v ) \
	( (*(nsrmap_scratch_data_t **)(s))->scratch1 = (v) )
#define	NSR_SET_SCRATCH_2( s, v ) \
	( (*(nsrmap_scratch_data_t **)(s))->scratch2 = (v) )
#define	NSR_SET_SCRATCH_3( s, v ) \
	( (*(nsrmap_scratch_data_t **)(s))->scratch3 = (v) )

#define	NSR_GET_SCRATCH_1( s, v ) \
	(*(void **)&(v) = ((*(nsrmap_scratch_data_t **)(s))->scratch1) )
#define	NSR_GET_SCRATCH_2( s, v ) \
	(*(void **)&(v) = ((*(nsrmap_scratch_data_t **)(s))->scratch2) )
#define	NSR_GET_SCRATCH_3( s, v ) \
	(*(void **)&(v) = ((*(nsrmap_scratch_data_t **)(s))->scratch3) )

/*
 * Doubly linked list header definition. This is used to keep track of what
 * Rx and Tx control blocks and buffers are being used. The ND provides all
 * our Tx udi_nic_tx_cb_t blocks, we provide the buffer.
 * We provide all the Rx udi_nic_rx_cb_t and buffers via udi_nd_rx_rdy()
 * NOTE: If the OS mapper is not using UDI serialisation (by chaining requests
 *	 onto the region's list of outstanding cbs) we have to protect the queue
 *	 by a simple mutex. This will increase the amount of system time spent
 *	 acquiring and releasing locks.
 *	 If we are running using the locking scheme, we don't need another 
 *	 implicit lock around the 'numelem' member. Simply utilise the 'lock'
 *	 field before modifying the 'numelem' member.
 */
typedef struct _nsr_q_head {
	udi_queue_t Q;
#ifndef NSR_USE_SERIALISATION
	udi_sbit32_t numelem;
	_OSDEP_SIMPLE_MUTEX_T lock;
#else
	_OSDEP_ATOMIC_INT_T numelem;
#endif					/* NSR_USE_SERIALISATION */
} _nsr_q_head;


#ifdef NSR_USE_SERIALISATION
	/*
	 * Queue accessor macros without lock protection 
	 */
#define	NSR_UDI_QUEUE_EMPTY( q )			\
	( _OSDEP_ATOMIC_INT_READ( &(q)->numelem ) == 0 )

#define	NSR_UDI_QUEUE_SIZE( q )			\
	( _OSDEP_ATOMIC_INT_READ( &(q)->numelem ) )

#define	NSR_UDI_QUEUE_INIT( q )			\
	UDI_QUEUE_INIT( &(q)->Q );			\
	_OSDEP_ATOMIC_INT_INIT( &(q)->numelem, 0 );

#define	NSR_UDI_QUEUE_DEINIT( q )			\
	_OSDEP_ATOMIC_INT_DEINIT( &(q)->numelem );

#define	NSR_UDI_ENQUEUE_TAIL( q, s )			\
	UDI_ENQUEUE_TAIL( &(q)->Q, (s) );		\
	_OSDEP_ATOMIC_INT_INCR( &(q)->numelem );

#define	NSR_UDI_ENQUEUE_HEAD( q, s )			\
	UDI_ENQUEUE_HEAD( &(q)->Q, (s) );		\
	_OSDEP_ATOMIC_INT_INCR( &(q)->numelem );

#define	NSR_UDI_DEQUEUE_HEAD( q )			\
	( _OSDEP_ATOMIC_INT_DECR( &(q)->numelem ),	\
	  UDI_DEQUEUE_HEAD( &(q)->Q ) )

#else
	/*
	 * Lock protected queue accessor macros 
	 */
STATIC udi_sbit32_t _nsr_udi_queue_empty(_nsr_q_head *);
STATIC udi_sbit32_t _nsr_udi_queue_size(_nsr_q_head *);

#define	NSR_UDI_QUEUE_EMPTY( q ) 	_nsr_udi_queue_empty( q )

#define	NSR_UDI_QUEUE_SIZE( q )		_nsr_udi_queue_size( q )

#define	NSR_UDI_QUEUE_INIT( q )			\
	UDI_QUEUE_INIT( &(q)->Q );			\
	(q)->numelem = 0;	\
	_OSDEP_SIMPLE_MUTEX_INIT( &(q)->lock, "NSR Q Lock");

#define	NSR_UDI_QUEUE_DEINIT( q )			\
	_OSDEP_SIMPLE_MUTEX_DEINIT( &(q)->lock );

#define	NSR_UDI_ENQUEUE_TAIL( q, s )			\
	_OSDEP_SIMPLE_MUTEX_LOCK( &(q)->lock );		\
	UDI_ENQUEUE_TAIL( &(q)->Q, (s) );		\
	(q)->numelem++;					\
	_OSDEP_SIMPLE_MUTEX_UNLOCK( &(q)->lock );

#define	NSR_UDI_ENQUEUE_HEAD( q, s )			\
	_OSDEP_SIMPLE_MUTEX_LOCK( &(q)->lock );		\
	UDI_ENQUEUE_HEAD( &(q)->Q, (s) );		\
	(q)->numelem++;					\
	_OSDEP_SIMPLE_MUTEX_UNLOCK( &(q)->lock );

STATIC udi_queue_t *_nsr_udi_dequeue_head(_nsr_q_head *);

#define	NSR_UDI_DEQUEUE_HEAD( q )	_nsr_udi_dequeue_head( q )
#endif /* NSR_USE_SERIALISATION */

/*
 * Deferred operation structure. This is used to enqueue an operation that
 * cannot be currently scheduled due to an internal resource condition. E.g.,
 * receiving a DMGMT_UNBIND request while still completing an allocation thread.
 */
typedef struct _nsr_defer_op {
	udi_queue_t Q;			/* Queue of functions */
	void (*func) (udi_cb_t *);	/* Function to call */
	udi_cb_t *cb;			/* Argument to func */
} _nsr_defer_op_t;

typedef struct nsrmap_region_data nsrmap_region_data_t;
typedef struct nsrmap_Tx_region_data nsrmap_Tx_region_data_t;

/*
 * Wrapper around a udi_nic_XX_cb_t to allow us to queue up these requests
 */
typedef struct nsr_cb {
	udi_queue_t q;			/* Overlay for UDI_QUEUE_xxx    */
	nsrmap_region_data_t *rdatap;	/* Region data                  */
	udi_cb_t *cbp;			/* Associated udi_nic_XX_cb_t   */
	udi_buf_t *buf;			/* Buffer for this cb           */
	void *scratch;
} nsr_cb_t;

/*
 * Inter-region operation. The primary region constructs an nsrmap_gio_t
 * request which is passed to the secondary (transmit) region via GIO.
 * The transmit region performs the requested operation and returns status via
 * udi_gio_xfer_ack or udi_gio_xfer_nak
 */

/*
 * Inter-region request structure. Used by udi_gio_xfer_req to send info from
 * primary to secondary regions
 */
#define	NSR_GIO_SET_CHANNEL	0x0	/* Set region data channel to arg */
#define	NSR_GIO_SET_STATE	0x1	/* Set region state to arg        */
#define	NSR_GIO_ALLOC_RESOURCES	0x2	/* Allocate (Rx) Resources        */
#define	NSR_GIO_FREE_RESOURCES	0x3	/* Free all allocated resources   */
#define	NSR_GIO_SET_TRACEMASK	0x4	/* Set Region tracemask           */
#define	NSR_GIO_COMMIT_CBS	0x8	/* Pass Rx CBs/empty Tx CBs to ND */

typedef struct _nsrmap_resource {
	udi_ubit32_t n_cbs;
	udi_ubit32_t max_size;
} _nsrmap_resource_t;

typedef struct _nsrmap_trace {
	udi_index_t meta_idx;
	udi_trevent_t tracemask;
} _nsrmap_trace_t;

typedef struct _nsrmap_chan_param {
	udi_channel_t channel;
} _nsrmap_chan_param_t;

typedef struct nsrmap_gio {
	udi_ubit32_t cmd;
	union {
		_udi_nic_state_t st;	/* NSR state      */
		_nsrmap_resource_t res;	/* Resources      */
		_nsrmap_trace_t tr;	/* Tracing        */
	} un;
} nsrmap_gio_t;

/*
 * Region data - Primary region only
 */
struct nsrmap_region_data {
	udi_init_context_t init_context;
	nsrmap_bind_info_t bind_info;
	udi_channel_t tx_chan;		/* channel to Tx region */
	udi_gio_bind_cb_t *tx_bind_cb;	/* Tx GIO bind CB       */
	udi_gio_xfer_cb_t *tx_xfer_cb;	/* Tx GIO xfer CB       */
	udi_gio_event_cb_t *tx_event_cb;	/* Tx GIO event CB      */
	nsrmap_gio_t *gio_cmdp;		/* Command for tx_xfer_cb */
	udi_nic_bind_cb_t link_info;	/* Link info for bound ND */
	udi_cb_t *rx_cb;		/* For Rx spawn operation */
	udi_cb_t *bind_cb;		/* For udi_nd_bind_req    */
	udi_cb_t *scratch_cb;		/* For Enable/Disable     */
	void (*cbfn) (udi_cb_t *,
		      udi_status_t);
	udi_cb_t *timer_cb;		/* For deferq timeout chain */
	udi_cb_t *link_timer_cb;	/* For deferred link-up timer */
	_nsr_q_head rxcbq;		/* Unused Rx cbs        */
	_nsr_q_head rxonq;		/* Available for ND use */
	udi_mgmt_cb_t *fcr_cb;		/* final cleanup req cb */
	udi_ubit32_t Resource;		/* Resource allocated bitmap */
	udi_ubit32_t Resource_rqst;	/* Resource requested bitmap */
	udi_ubit32_t NSR_state;		/* Overall NSR state */
	udi_ubit32_t NSR_Rx_state;	/* NSR Rx region state */
	udi_ubit32_t NSR_Tx_state;	/* NSR Tx region state */
	udi_ubit32_t buf_size;		/* Max Rx/Tx buffer size */
	_nsr_q_head deferq;		/* Deferred operations */


	/*
	 * NSR dependent variables 
	 */
	udi_status_t open_status;	/* Status for OS open() call */
	udi_status_t bind_status;	/* Status of udi_nd_bind_req */
	udi_ubit32_t nopens;		/* Number of open()s active   */

	/*
	 * Transmit/Receive counters 
	 */
	udi_ubit32_t n_packets;
	udi_ubit32_t n_discards;	/* # of discards within NSR */

	/*
	 * Asynchronous completion mechanism 
	 */
	void (*next_func) (udi_cb_t *);

	/*
	 * Statistics 
	 */
	udi_ubit32_t n_rx_frees;	/* # of freed Rx buffers */

	/*
	 * OS-specific data structures 
	 */
	netmap_osdep_t *osdep_ptr;
	udi_ubit8_t osdep[NETMAP_OSDEP_SIZE];
	udi_ubit32_t capabilities;	/* OS-used ND capabilities */

	/*
	 * Back-pointer [set => initialised data] 
	 */
	void *rdatap;			/* Reference to &region-data */

	/*
	 * Trace masks 
	 */
	udi_trevent_t ml_tracemask[3];	/* Meta-language masks 0 => MA */
	/*
	 * 1 => Network Meta          
	 */
	/*
	 * 2 => GIO Meta              
	 */

	/*
	 * Inherited Constraints 
	 */
	udi_buf_path_t parent_buf_path;	/* Parent buf_path */
	udi_buf_path_t buf_path;	/* Inter-region buf_path */

	/*
	 * NSR state variable 
	 */
	_udi_nic_state_t State;		/* Current state of NSR */
	_udi_nic_state_t Rx_State;	/* Current state of Rx region */
	_udi_nic_state_t Tx_State;	/* Current state of Tx region */

	/*
	 * Frame Format information 
	 */
	udi_ubit32_t trailer_len;	/* Length of frame trailer */
};

/*
 * Secondary region data
 */
struct nsrmap_Tx_region_data {
	udi_init_context_t init_context;	/* Must be first */
	udi_cb_t *my_cb;		/* For all ND channel ops */
	udi_gio_xfer_cb_t *xfer_cb;	/* Current xfer CB */
	udi_cb_t *timer_cb;		/* Deferred timeout CB */
	/*
	 * NSR state variable 
	 */
	_udi_nic_state_t State;		/* Current state of NSR */

	/*
	 * CB tracking queues. Those available to NSR are held in 'cbq', 
	 * those posted to the ND are held in 'onq'
	 */
	_nsr_q_head cbq;		/* Unallocated cbs      */
	_nsr_q_head onq;		/* Posted to ND         */
	_nsr_q_head deferq;		/* Deferred operations queue */

	/*
	 * Internal region state tracking 
	 */
	udi_ubit32_t Resource;		/* Resource allocated bitmap */
	udi_ubit32_t Resource_rqst;	/* Resource requested bitmap */
	udi_ubit32_t NSR_state;		/* NSR <-> ND link state */
	udi_ubit32_t buf_size;		/* Max buffer size */

	/*
	 * Packet Counters 
	 */
	udi_ubit32_t n_packets;
	udi_ubit32_t n_discards;	/* # of discards within NSR */

	/*
	 * Trace masks 
	 */
	udi_trevent_t tracemask;	/* Local nsrmap tracemask */
	udi_trevent_t ml_tracemask[3];	/* Meta-language masks 0 => MA */
	/*
	 * 1 => Network Meta          
	 */
	/*
	 * 2 => GIO Meta              
	 */

	/*
	 * Asynchronous completion mechanism 
	 */
	void (*next_func) (udi_cb_t *);

	/*
	 * Inherited Constraints 
	 */
	udi_buf_path_t buf_path;	/* Tx buffer allocation path */

	/*
	 * Frame Format information 
	 */
	udi_ubit32_t trailer_len;	/* Length of frame trailer */

	/*
	 * Back-pointer [set => initialised data] 
	 */
	void *rdatap;			/* Reference to &region-data */

	/*
	 * Temporary holding area for Tx CBs (from ND) 
	 */
	udi_nic_tx_cb_t *ND_txcb;	/* From nsrmap_tx_rdy() */

	/*
	 * OS-specific data structures 
	 */
	netmap_osdep_tx_t *osdep_tx_ptr;
	udi_ubit8_t osdep_tx[NETMAP_OSDEP_TX_SIZE];
};

#define	NSRMAP_TRLOG_HEADER	"NSR: "
#define	NSRMAP_TRLOG_FMT_ADD	6	/* Space for header */

#ifndef UDI_TREVENT_META_MASK
#define	UDI_TREVENT_META_MASK	0x0000F9C0
#endif /* UDI_TREVENT_META_MASK */

/*
 * Transmit and Receive channel spawn indices -- must be the same between ND
 * and NSR for udi_channel_spawn() to correctly rendezvous
 * FIXME: This ought to be spec'd to allow complete vendor neutrality
 */
#define	NSR_TX_CHAN		1
#define	NSR_RX_CHAN		2

/*
 * State values for Resource bitmaps
 */
#define	NsrmapR_TX_CB	0x00000001	/* TX CB wanted         */
#define	NsrmapR_RX_CB	0x00000002	/* RX CB wanted         */

/*
 * Values for NSR_state
 */
#define	NsrmapS_LinkUp		0x00000001	/* Link established */
#define	NsrmapS_Enabled		0x00000002	/* Link Enabled  */
#define	NsrmapS_AllocFail	0x00000004	/* Allocation failed    */
#define	NsrmapS_Disabling	0x00000008	/* Disable in progress */
#define	NsrmapS_AllocDone	0x00000010	/* CBs allocated        */
#define NsrmapS_Enabling	0x00000020	/* Enable in progress */
#define	NsrmapS_Rx_Enabling	0x00000040	/* Rx Region in enable process */
#define	NsrmapS_Tx_Enabling	0x00000080	/* Tx Region in enable process */
#define	NsrmapS_Binding		0x00000100	/* Region is binding */
#define	NsrmapS_Bound		0x00000200	/* Region bound */
#define	NsrmapS_Anchoring	0x00000400	/* Region is anchoring channel */
#define	NsrmapS_Anchored	0x00000800	/* Region has anchored channel */
#define	NsrmapS_Constraints	0x00001000	/* Constraints have propagated */
#define	NsrmapS_TraceMaskSet	0x00002000	/* All tracemasks set         */
#define	NsrmapS_MgmtTraceSet	0x00004000	/* Mgmt meta tracemask set    */
#define	NsrmapS_GioTraceSet	0x00008000	/* GIO meta tracemask set     */
#define	NsrmapS_NicTraceSet	0x00010000	/* NIC meta tracemask set     */
#define	NsrmapS_CommitWanted	0x00020000	/* Tx CB commit wanted        */
#define	NsrmapS_AllocPending	0x00040000	/* Allocation outstanding     */
#define	NsrmapS_TxCmdActive	0x00080000	/* Command posted to Tx region */
#define	NsrmapS_TimerActive	0x00100000	/* deferq timeout active      */
#define	NsrmapS_LinkTimerActive	0x00200000	/* Link-UP timer active       */
#define	NsrmapS_Promiscuous	0x00400000	/* Receive-all mode enabled   */

/*
 * open_status values
 */
#define	NsrmapRx_Waiting	0x00000001	/* Waiting for Rx notification */
#define NsrmapTx_Waiting	0x00000002	/* Waiting for Tx CB          */

/*
 * Macros to handle bitmasks in a cleaner way.
 *	BSET	sets bits in reg from mask
 *	BCLR	clears bits in reg from mask
 *	BTST	ors bits in reg with mask; value is result
 *	BTEQ	like BTST, but checks if result exactly equals mask
 */
#define	BSET(reg, mask)	{ (reg) |= (mask); }
#define	BCLR(reg, mask)	{ (reg) &= ~(mask); }
#define	BTST(reg, mask) ((reg) & (mask))
#define	BTEQ(reg, mask) (((reg) & (mask)) == (mask))

/*
 * User-supplied request format [nsrmap_sendmsg], [nsrmap_getmsg]
 */
typedef struct nsrmap_uio {
	void *u_addr;			/* Address of user-supplied data */
	udi_size_t u_size;		/* Size of user data             */
} nsrmap_uio_t;

/*
 * Control Block Indices for NET metalanguage
 */
#define	NSRMAP_NET_CB_IDX	1
#define	NSRMAP_BIND_CB_IDX	2
#define	NSRMAP_CTRL_CB_IDX	3
#define	NSRMAP_STATUS_CB_IDX	4
#define NSRMAP_INFO_CB_IDX	5
#define	NSRMAP_TX_CB_IDX	6
#define	NSRMAP_RX_CB_IDX	7

/*
 * Control Block Indices for GIO metalanguage
 */
#define NSRMAP_GIO_BIND_CB_IDX	8
#define	NSRMAP_GIO_XFER_CB_IDX	9
#define	NSRMAP_GIO_EVENT_CB_IDX	10

#ifndef NSRMAP_CB_ALLOC_BATCH
#define NSRMAP_CB_ALLOC_BATCH(xxcallback, xxcb, xxrdata) \
	udi_cb_alloc_batch(xxcallback, xxcb, NSRMAP_RX_CB_IDX, \
			   xxrdata->link_info.rx_hw_threshold, \
			   TRUE, xxrdata->buf_size + xxrdata->trailer_len, \
			   xxrdata->parent_buf_path)
#endif
