
/*
 * File: 
 *
 * Common definitions for Pseudo Network UDI device drivers
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

	/* Assumes that Public section is first, Private_section is second */

#ifndef	_PSEUDO_ND_H
#define	_PSEUDO_ND_H

#undef	Public_section_begin
#undef	Public_section_end
#undef	Private_section_begin
#undef	Private_section_end
#undef DBG_MGMT
#undef DBG_ND

#define	Public_section_begin
#define	Public_section_end

#ifdef	Private_visible	       /* Define for INTERNAL private access */
#define	Private_section_begin  unsigned long private_begin;
#define	Private_section_end    unsigned long unused;
#else
#define	Private_section_begin  unsigned long private_begin; struct {
#define	Private_section_end    unsigned long unused; } private;
#endif

#ifndef STATIC
#define	STATIC static
#endif	/* STATIC */

#undef	Private_visible


#ifndef	Offset
#define	Offset(T,F)	((udi_ubit32_t)( &((T*)NULL)->F ))
#endif
#define	Public_section_size(S)	(Offset(S, private_begin))
#define	Private_section_size(S)	(sizeof(S) - Public_section_size(S))

/*
 * Standard Type Definitions 
 */


#undef	Private
/* When debugging it may be helpful to null define the Private macro */
#define	Private STATIC


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


typedef struct	_dll_head {
	udi_queue_t	Q;
	udi_ubit32_t	numelem;
} _dll_head_t;

 
/*
 * ========================================================================
 * FILE: pseudond.h
 * ========================================================================
 */

#ifdef DEBUG
#define PSEUDOND_DEBUG

#define	DBG_MGMT_VAL	0x1		/* Debug Management Meta */
#define	DBG_ND_VAL	0x2		/* Debug Network Meta	 */
#define	DBG_GIO_VAL	0x4		/* Debug GIO Meta	 */

static udi_ubit32_t	pseudo_dbg = DBG_MGMT_VAL | DBG_GIO_VAL | DBG_ND_VAL;

#define	DBG_MGMT(x)	if (pseudo_dbg & DBG_MGMT_VAL) { \
				udi_debug_printf x ; } else
#define	DBG_GIO(x)	if (pseudo_dbg & DBG_GIO_VAL) { \
				udi_debug_printf x ; } else
#define	DBG_ND(x)	if (pseudo_dbg & DBG_ND_VAL) { \
				udi_debug_printf x ; } else

#else

#define	DBG_MGMT(x)
#define	DBG_GIO(x)
#define	DBG_ND(x)

#endif	/* DEBUG */

#ifdef PSEUDOND_DEBUG
#  define	PSEUDOND_ASSERT(invariant) udi_assert(invariant)
#else
#  define PSEUDOND_ASSERT(invariant)  
#endif	/* PSEUDOND_DEBUG */


#define	ND_NUM_METAS		2	/* Support 2 meta languages, NSR and  */
					/* GIO				      */

#if !defined(ND_NSR_META) || !defined(ND_GIO_META)
#error "ND_NSR_META and ND_GIO_META _MUST_ be defined"
#define	ND_NSR_META		2	/* Must match static driver properties*/
#define	ND_GIO_META		1	/*	"	"	"	"     */
#endif
				
typedef struct pseudo_bind_info {
        udi_channel_t   bind_channel;	/* Handle to channel used to communicate
					   with the driver */
        udi_bind_cb_t   *bind_cb;	/* Pointer to bind_cb which is passed by
					   the MA during bind_to_parent_req. */
} pseudo_bind_info_t;

/*
 * Transmit and Receive channel spawn indices
 */
#define	ND_TX_CHAN		1
#define	ND_RX_CHAN		2

/*
 * Transmit and Receive region indices
 */
#define	ND_TX_REGION		1
#define	ND_RX_REGION		2

    /** Container data structure for alloc tokens.  This structure is used
    *** to provide the queueing and context information needed to maintain
    *** driver specific information with each alloc token.
    **/
typedef struct Pseudo_cb_struct		Pseudo_cb_t;
typedef struct Pseudo_cbseg_struct	Pseudo_cbseg_t;
typedef struct Card_s			Pseudo_Card_t;

typedef enum { TX, RX, Misc } cb_type_t;

struct Pseudo_cb_struct {
    udi_queue_t		cb_next;      /* Overlay for UDI_QUEUE_xxx */
    Pseudo_Card_t 	*cb_card;     /* Owning Card (region) data struct */
    cb_type_t     	cb_type;      /* Primary type (usage) of this token */
    udi_cb_t 		*cbt;         /* The alloc_token itself;
				         (call_context -> this structure) */
#if 0
    udi_dma_handle_t 	cb_dmah;      /* Associated TX DMA Handle */
#endif
    udi_buf_t       	*cb_buffer;    /* Pre-allocated pre-mapped buffer */
#if 0
    udi_scgth_t    	*cb_sgl;      /* Scatter-Gather list for pre-mapping */
#endif
    udi_boolean_t   	cb_has_dmah;  /* Set if this has a dma handle */
    udi_boolean_t   	cb_has_buffer;/* Set if this has a pre-allocated buf */
    udi_boolean_t   	cb_is_primary;/* Set if this is the primary token */

    void           	*cb_scratch;  /* Private scratch space: note that this
				         does not point to any memory unless
					 explicitly set by the token's user */
};

#define CB_SETSCRATCH(pCb,Val)      { (pCb)->cb_scratch = (void *)(Val);}
#define CB_GETSCRATCH(pCb,Var,Type) \
                     { (Var) = (Type)(((udi_ubit32_t)((pCb)->cb_scratch)));}


struct Pseudo_cbseg_struct {
    udi_queue_t	      tseg_next;   /* Overlay for UDI_QUEUE_xxx */
    void             *tseg_mem;    /* Pointer to this token segment memory */
    udi_size_t        tseg_size;   /* Size of this token segment in bytes */
};

/*
 * Deferred operation queue. These entries correspond to control requests
 * received from the NSR while the card is in the process of disabling
 * after receiving a udi_nd_disable_req. Once the card has progressed to the
 * ND_NSR_BOUND state [from ND_NSR_DISABLING] the deferred operation will be
 * resumed.
 */
typedef enum {
	_udi_nd_enable_req,		/* udi_nd_enable_req() operation */
	_udi_nd_unbind_req		/* udi_nd_unbind_req() operation */
} pseudo_defer_ops_t;

typedef struct {
	udi_queue_t		Q;
	pseudo_defer_ops_t	op;
	udi_nic_cb_t		*cb;
} pseudo_defer_el_t;

/*
 * State definitions - reflected in Meta_state
 */
typedef enum { 
	ND_UNBOUND, 			/* ND available 		*/
	ND_GIO_BOUND, 			/* ND bound to GIO		*/
	ND_NSR_BINDING, 		/* ND binding to NSR		*/
	ND_NSR_BOUND,			/* ND bound to NSR		*/
	ND_NSR_ENABLED,			/* ND enabled			*/
	ND_NSR_ACTIVE,			/* ND active transmit/receive	*/
	ND_NSR_DISABLING,		/* udi_nd_disable_req in prog.	*/
	ND_NSR_UNBINDING		/* ND unbinding from NSR	*/
} pseudo_meta_state_t;

/*
 * Attribute state machine - used to query all udiprops.txt provided attributes
 */
typedef enum {
	ND_ATTR_LINK_SPEED=0,		/* %link_speed			*/
	ND_ATTR_CPU_SAVER,		/* %cpu_saver			*/
	ND_ATTR_SPECIOUS_PARAM		/* %utterly_specious_parameter  */
} pseudo_attr_state_t;

    /** Region data structure; 1/region = 1/device node = 1/Adapter.
    *** This is the primary data structure used by this driver to maintain
    *** information about the current adapter being controlled.
    **/

struct Card_s {
    udi_init_context_t   Card_initcontext;  /* Must be first in UDI! */

    Public_section_begin
	char id[32];
        udi_ubit32_t Cardstat_ipackets;
        udi_ubit32_t Cardstat_ierrors;
        udi_ubit32_t Cardstat_ioverrun;
	udi_ubit32_t Cardstat_idiscards;
        udi_ubit32_t Cardstat_opackets;
        udi_ubit32_t Cardstat_oerrors;
        udi_ubit32_t Cardstat_ounderrun;
	udi_ubit32_t Cardstat_odiscards;
        udi_ubit32_t Cardstat_collisions;
	udi_ubit32_t Card_num;			/* Instance # for this device */
    Public_section_end

    Private_section_begin
               /** Management Operations **/
        udi_channel_t	Card_BUS;        /* Bus Bridge Channel */
        udi_bus_bind_cb_t *Card_BUS_cb;    /* Bus Bridge Bind control block */

        udi_channel_t	Card_NSR;        /* NSR Bind Channel */
        udi_channel_t	Card_Tx_chan;	 /* NSR Data Transmit Channel */
	udi_channel_t	Card_Rx_chan;	 /* NSR Data Receive Channel */
	udi_index_t	Card_Tx_idx;	 /* NSR Data Transmit Index */
	udi_index_t	Card_Rx_idx;	 /* NSR Data Receive Index */
	udi_cb_t	*Card_Tx_cb;	 /* Initial udi_nic_tx_cb_t storage */
	udi_cb_t	*Card_Rx_cb;	 /* Initial udi_nic_rx_cb_t storage */
        udi_cb_t	*Card_NSR_cb;    /* NSR Bind control block storage */
        udi_cb_t	*Card_NSR_netcb; /* Network control block storage */

	udi_channel_t	Card_GIO;	 /* GIO Bind Channel */
	udi_cb_t	*Card_GIO_cb;	 /* GIO Bind control block */

	udi_cb_t	*Card_scratch_cb; /* Storage for CB to be freed */
		/** Operation and Status **/
	udi_ubit32_t 	Card_state;	 /* Alloc state variable for this Card*/
	pseudo_meta_state_t Meta_state; /* State of meta attachment */
	_dll_head_t	Card_defer_op;	 /* List of deferred ops to process */
	udi_ubit32_t	Card_opmode;	 /* Internal copy of OPMODE_CSR */
        udi_cb_t 	*Card_netind_cb; /* Network Indication Control Blk */

		/* Attribute values [from udiprops.txt] */
	udi_ubit32_t	Card_link_speed; /* Link speed (bps) */
	udi_ubit32_t	Card_cpu_saver;	 /* CPU saver setting */
	udi_ubit32_t	Card_specious;	 /* Long parameter name */
	pseudo_attr_state_t Card_attr;	 /* Current attribute being queried */

		/** Administration and Identification **/
        _dll_head_t 	cbsegq;          /* Queue of token segment structs */
        udi_size_t  	cbqmemsize;      /* Size of token Q allocated memory */
        _dll_head_t 	cbq;             /* Queue of Pseudo_cb_t structs */

	udi_ubit32_t  	Card_idnum;	/* Index into Pseudo_valid_ids */

        udi_ubit32_t  	Card_resource_rqst; /* Bit map of allocating resources */
        udi_ubit32_t  	Card_resources;   /* Bitmap of allocated resources */
        udi_ubit8_t   	Card_macaddr[6];  /* Card's MAC address */
        udi_ubit8_t   	Card_fact_macaddr[6]; /* Card's factory MAC address */
	udi_cb_t	*Card_link_cb;	  /* Link-UP CB			    */
	udi_ubit32_t	Card_max_pdu;	  /* Maximum PDU size for link_info */
	udi_ubit32_t	Card_min_pdu;	  /* Minimum PDU size for link_info */
	udi_ubit8_t	Card_media_type;  /* Network Media type		    */
	udi_ubit8_t	Card_link_status; /* Link status of card	    */
	udi_dma_constraints_t Card_dmacon;  /* DMA constraints		    */
	udi_ubit32_t	Card_hdrlen;	  /* Length of header preamble	    */
	udi_ubit32_t	Card_trailerlen;  /* Length of trailer postamble    */

		/** Receive Control (ring mode) **/
	udi_ubit32_t  Card_RXRingOff;	/* PIO offset: RX PDL array start */
	udi_ubit32_t  Card_RXRingEnd;	/* PIO offset: last valid RX PDL + 1 */
	udi_ubit32_t  Card_RXRingWR;	/* PIO offset: current RXPDL to fill */
	udi_ubit32_t  Card_RXRingRD;	/* PIO offset: current RX PDL to get */
        udi_ubit32_t  Card_rxmapping;   /* # RX mappings underway --> rxdoq */
        _dll_head_t   Card_rxatq;       /* RX alloc token queue */
        _dll_head_t   Card_rxdoq;       /* RX currently posted to device */
        _dll_head_t   Card_rxcbq;       /* RX control block queue */
        _dll_head_t   Card_rxdoneq;     /* RX alloc tokens waiting for rxcb */

		/** Transmit Control **/
	udi_ubit32_t  Card_TXRingOff;	/* PIO offset of TX PDL array start*/
	udi_ubit32_t  Card_TXRingEnd;	/* PIO offset of last TX PDL + 1 */
	udi_ubit32_t  Card_TXRingWR;	/* PIO offset of current TX PDL */
	udi_ubit32_t  Card_TXRingWRprev;/* PIO offset of prev TX PDL done */
	udi_ubit32_t  Card_TXRingRD;	/* PIO offset: current TX PDL to get */
        _dll_head_t   Card_txatq;       /* TX alloc token queue */
        _dll_head_t   Card_txcbq;       /* List of TX cntrl blks for NSR */
        _dll_head_t   Card_txrdyq;      /* Q of TX pkts ready to transmit */
        _dll_head_t   Card_txdoq;       /* Q of TX pkts being transmitted */


                  /** Statistics (Internal and External) **/
	udi_ubit32_t  Card_RX_blocked;	/* # times RX blocked: no resources */

	udi_ubit32_t  Card_stat_ISR_RX_pkt;   /* # RX pkt ints */
	udi_ubit32_t  Card_stat_ISR_RX_error; /* # RX error ints */
	udi_ubit32_t  Card_stat_last_RX_error; /* mask of last RX error */
	udi_ubit32_t  Card_stat_ISR_TXspace;  /* # TX space available ints */
	udi_ubit32_t  Card_stat_ISR_TX_error; /* # TX error ints */
	udi_ubit32_t  Card_stat_last_TX_error; /* mask of last TX error */

	udi_ubit32_t  Card_stat_TX_sendblocked;   /* # TX send blocks */
	udi_ubit32_t  Card_stat_TX_noPDLs;    /* # TX send fails: no PDLs */
	udi_ubit32_t  Card_stat_TX_RingFull;  /* # TX send fails: no PDLs */
	udi_ubit32_t  Card_stat_TX_cachebash; /* # TX ring cache bashes */
	udi_ubit32_t  Card_stat_RX_noPDLs;    /* # RX post fails: no PDLs */
	udi_ubit32_t  Card_stat_RX_allocf;    /* # RX alloc fails: no bufs */
	udi_ubit32_t  Card_stat_RX_incompl;   /* # RX incomplete aborts */
	udi_ubit32_t  Card_stat_RX_cachebash; /* # RX ring cache bashes */
	
		/** Initialisation flag **/

	void 		*Card_Pcp;		/* Back ref to ourselves */
    Private_section_end
};


/* Card_resources bit values: */
#define CardR_TXRing_Mem    0x0001    /* Transmit Ring PIO Mapped Memory */
#define CardR_RXRing_Mem    0x0002    /* Receive Ring PIO Mapped Memory */
#define CardR_SFrame_Mem    0x0004    /* Setup Frame PIO Mapped Memory */
#define CardR_NetInd_CB     0x0008    /* Network Indication Control Block */
#define CardR_TXRing_DMAH   0x0010    /* Transmit Ring DMA Handle */
#define CardR_RXRing_DMAH   0x0020    /* Receive Ring DMA Handle */
#define CardR_SFrame_DMAH   0x0040    /* Setup Frame DMA Handle */
#define CardR_TX_CB         0x0080    /* Transmit Control Block */
#define CardR_TXRing_Map    0x0100    /* Transmit Ring DMA Mapped SGList */
#define CardR_RXRing_Map    0x0200    /* Receive Ring DMA Mapped SGList */
#define CardR_SFrame_Map    0x0400    /* Setup Frame DMA Mapped SGList */
#define CardR_RX_CB         0x0800    /* Receive Control Block */
#define CardR_MIIPHY        0x1000    /* MII PHY Management Memory */
#define CardR_MIIPHY_Timer  0x2000    /* MII PHY Enable Timer token */
#define CardR_ISR_CB        0x4000    /* Interrupt Registration Cntrl Blk */
#define CardR_Watchdog      0x8000    /* Watchdog Timer token */

/** Valid values for Card_state **/
#define	CardS_Enabled		0x00001	/* MAC is active */
#define CardS_Deadman           0x00002 /* Deadman timer (watchdog) active */
#define	CardS_WakeupSend	0x00004	/* Set to wakeup sender on send compl*/
#define CardS_Do_Notify         0x00008 /* Do nsr_status_ind when get rsrcs */
#define	CardS_DevConnected	0x00010	/* Card is connected to BUS */
#define	CardS_ISR_Handling	0x00020	/* ISR has been/is being registered */
#define CardS_PHY_Initialized   0x00040 /* MIIPHY module initialization */
#define	CardS_Enabling		0x00100	/* Card is in process of enabling */
#define	CardS_LinkUp		0x00200	/* Link is active */
#define CardS_HashFilter        0x00400 /* RX hash filtering (!RX exact) */
#define CardS_PIOMapped         0x01000 /* PIO register space is mapped */
#define CardS_PIOSlowMapped     0x02000 /* PIO space slow mapping */
#define CardS_PIOTickMapped     0x04000 /* PIO  space tick count mapping */
#define CardS_Have_BufConstr    0x08000 /* Allocated buffer constraints */
#define CardS_AllocFail         0x10000 /* Alloc_Resources failed */
#define CardS_Disabling		0x20000	/* Card is in process of disabling */

/*
 * Tune'ables
 */

/* How big should the TX and RX Rings be made?  This shouldn't be pushed
 * over a page size because the rings must be contiguous.  Additionally,
 * there's some gaps and header structs that must be taken into account.
 */

#define RX_RING_LENGTH  0x400
#define TX_RING_LENGTH  (0x1000-256)


/* How "big" is each ring entry (PDL) including gaps between entries.  The
 * gaps should be set so that there's one PDL for each cache line to prevent
 * cache problems where the Card and Host write to the same cache line.
 * The minimum for this is 0x10 which is the actual size of the PDL itself.
 */
#define PDL_SIZE     0x20


/* Now compute the number of entries in each ring */
#define NUM_RX_PDL      RX_RING_LENGTH / PDL_SIZE
#define NUM_TX_PDL	TX_RING_LENGTH / PDL_SIZE


/* How many physical (DMA) fragments will we allow the buffers to be */
#define	MAX_TX_FRAGS	8     /* arbitrary <= 256; choose wisely */
#define	MAX_RX_FRAGS	2     /*REQD to have a 1:1 between RX bufs and PDLs */


/* # of tokens to allocate.  This is strongly recommended to be >= NUM_TX_PDL
 * because these token structures also contain dma handles used for transmit
 * operations, so the number of transmit control blocks will be tuned down
 * to this level if it is not large enough.
 */
#define NUM_TOKENS   (NUM_TX_PDL + NUM_RX_PDL)


/* # of transmit buffers to pre-allocate and pre-map. */
#define TX_STOCK_LEVEL  NUM_TX_PDL

/*
 * GIO interface definitions
 */
#define	PSEUDOND_GIO_MEDIA	33		/* gio_op_t value */

typedef struct pseudond_media {
	udi_ubit32_t	max_pdu_size;
	udi_ubit32_t	min_pdu_size;
	udi_ubit8_t	media_type;
} pseudond_media_t;

#endif	/* _PSEUDO_ND_H */



