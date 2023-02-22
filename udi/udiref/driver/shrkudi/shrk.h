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
/* shrkudi UDI NIC driver - sample driver for the Osicom 2300 PCI Fast Ethernet
 *	adapted from Matrox Shark driver
 */

/* PCI ID definitions */
#define SHRK_DEV_DEC_21143 0x0019

/* MAC address definitions */
#define SHRK_MACADDRSIZE 6
typedef udi_ubit8_t shrkaddr_t[SHRK_MACADDRSIZE];
typedef struct shrkmacaddr {
	udi_ubit8_t mac1;    /* high order byte */
	udi_ubit8_t mac0;    /* low order byte */
} shrkmacaddr_t;

/* operations vector indices */
#define SHRK_PRI_REG_IDX       1
#define SHRK_SEC_REG_IDX       2
#define SHRK_ND_CTRL_IDX       3
#define SHRK_BUS_BRIDGE_IDX    4
#define SHRK_ND_TX_IDX         5
#define SHRK_ND_RX_IDX         6
#define SHRK_INTR_HANDLER_IDX  7

/* control block indices */
#define SHRK_NIC_STD_CB_IDX    1      /* net standard control block */
#define SHRK_NIC_BIND_CB_IDX   2      /* net bind control block */
#define SHRK_NIC_CTRL_CB_IDX   3      /* net control ops control block */
#define SHRK_NIC_STATUS_CB_IDX 4      /* net status control block */
#define SHRK_NIC_INFO_CB_IDX   5      /* net info control block */
#define SHRK_NIC_TX_CB_IDX     6      /* net transmit control block */
#define SHRK_NIC_RX_CB_IDX     7      /* net receive control block */
#define SHRK_GIO_BIND_CB_IDX   8      /* gio bind control block */
#define SHRK_GIO_XFER_CB_IDX   9      /* gio transfer control block */
#define SHRK_GIO_EVENT_CB_IDX  10     /* gio event control block */
#define SHRK_BUS_BIND_CB_IDX   11     /* bus bind control block */
#define SHRK_BUS_INTR_ATTACH_CB_IDX 12 /* bus interrupt attach control block */
#define SHRK_BUS_INTR_EVENT_CB_IDX  13 /* bus interrupt attach control block */
#define SHRK_BUS_INTR_DETACH_CB_IDX 14 /* bus interrupt detach control block */
#define SHRK_GCB_CB_IDX 15	       /* generic control block */
#define SHRK_PROBE_CB_IDX 16	       /* generic control block with scratch */

#define SHRK_ATTR_LEN          5      /* enumeration attribute list length */

/* udi_pio_map argument structure */
typedef const struct shrk_pio_map_args {
	udi_ubit16_t rdata_idx;
	udi_ubit32_t regset_idx;
	udi_ubit32_t base_offset;
	udi_ubit32_t length;
       	udi_pio_trans_t *trans_list;
	udi_ubit16_t list_length;
	udi_ubit16_t pio_attributes;
	udi_ubit32_t pace;
	udi_index_t serialization_domain;
} shrk_pio_map_args_t;

/* indexes into pio handle array in rdata */
#define SHRK_PIO_WAKEUP		0
#define SHRK_PIO_READ_SROM	1
#define SHRK_PIO_MDIO_READ	2
#define SHRK_PIO_MDIO_WRITE	3
#define SHRK_PIO_NIC_RESET	4
#define SHRK_PIO_READ_GPREG	5
#define SHRK_PIO_TX_ENABLE	6
#define SHRK_PIO_TX_POLL	7
#define SHRK_PIO_RX_ENABLE	8
#define SHRK_PIO_CSR6_OR32	9
#define SHRK_PIO_CSR6_AND32	10
#define SHRK_PIO_ABORT		11

#define SHRK_PIO_COUNT		12

/* list of values for string options, and a code representing that value */
typedef const struct shrk_option_values {
	const char *name;
	udi_ubit16_t code;
} shrk_option_values_t;

/* associates an attribute with an index into the values in rdata */
typedef const struct shrk_instance_attr_name {
	udi_ubit16_t rdata_idx;		/* index into table in rdata */
	char *attr_name;
	udi_instance_attr_type_t attr_type;
	shrk_option_values_t *values;	/* string translation table */
	udi_ubit32_t defval;		/* default translated value */
} shrk_instance_attr_name_t;

/* indexes into attribute values in rdata */
#define SHRK_ATTR_LINK_SPEED	0
#define SHRK_ATTR_DUPLEX_MODE	1
#define SHRK_ATTR_RX_BUFFERS	2
#define SHRK_ATTR_TX_BUFFERS	3
#define SHRK_ATTR_PCI_VENDOR	4
#define SHRK_ATTR_PCI_DEVICE	5
#define SHRK_ATTR_PCI_REV	6
#define SHRK_ATTR_PCI_SUBVEND	7
#define SHRK_ATTR_PCI_SUBSYS	8

#define SHRK_ATTR_COUNT		9

/* table of link parameters and GP register values (for 21140 support) */
typedef const struct shrk_link_parms {
	udi_ubit32_t flags;
	udi_ubit32_t csr6;
	udi_ubit32_t gpreg;
} shrk_link_parms_t;

/* Ethernet headers and packet sizes */
#define SHRK_HEADERSZ    14        /* ethernet header size */
#define SHRK_MINPACK     60        /* minimum packet size including header */
#define SHRK_MAXPACK     (udi_ubit32_t)1518  /* max packet size header, crc */
#define SHRK_RXALIGN     2         /* bytes needed to longword align MAXPACK */
#define SHRK_RXMAXSZ     (SHRK_MAXPACK + SHRK_RXALIGN)
#define SHRK_TXMAXSZ     (SHRK_MAXPACK - 4)  /* subtract crc length */

/* Multicast address */
#define SHRK_MCA(ea) (((caddr_t)(ea))[0] & 1)
#define SHRK_MCA_CNT(c) (((c)->data_buf->buf_size / SHRK_MACADDRSIZE) - ((c)->indicator))


/*
 * Serial ROM as documented in DEC Serial ROM Application Note
 * Note: these are internal representations of the data contained in the srom
 */
typedef struct shrk_srom {
	udi_ubit16_t     svid;                 /* subsystem vendor ID */
	udi_ubit16_t     sid;                  /* subsystem ID */
	udi_ubit8_t      ID_reserved1[12];
	udi_ubit8_t      id_block_crc;         /* CRC8 calue of the ID block */
	udi_ubit8_t      ID_reserved2;
	udi_ubit8_t      srom_format_version;  /* current version is 0x03 */
	udi_ubit8_t      chip_count;           /* number of nics sharing srom */
	shrkaddr_t       hwaddr;               /* factory MAC address */
	udi_ubit8_t      chip_info[98];        /* chip specific stuff */
	udi_ubit16_t     reserved;
	udi_ubit16_t     crc;                  /* srom checksum up to crc */ 
} shrk_srom_t;
#define SHRK_SROM_MAC_OFF    20               /* hwaddr offset in srom */
#define SHRK_SROMSIZE (sizeof(shrk_srom_t))


/* Structure and definitions for receive and transmit data descriptors */
/* TODO3 merge these two structs into one since Desc Skip Len applies to both */

/* structure to associate a "free" flag with a dma handle */
typedef struct shrk_dma_element {
	struct shrk_dma_element *next;
	udi_dma_handle_t dmah;
} shrk_dma_element_t;

typedef struct rxd {
	volatile udi_ubit32_t  status;        /* RDES0 - Status */
	volatile udi_ubit32_t  cntrl;         /* control bits and byte counts */
	volatile udi_ubit32_t  paddr1;        /* physical address of buffer 1 */
	volatile udi_ubit32_t  paddr2;        /* physical addr of next descr */
	udi_nic_rx_cb_t        *rx_cb;        /* receive control block */
	shrk_dma_element_t     *rx_sdmah;     /* receive buffer dma handle */
} rxd_t;

typedef struct txd {
	volatile udi_ubit32_t  status;        /* TDES0 - Status */
	volatile udi_ubit32_t  cntrl;         /* control bits and byte counts */
	volatile udi_ubit32_t  paddr1;        /* physical address of buffer 1 */
	volatile udi_ubit32_t  paddr2;        /* physical addr of next descr */
	udi_nic_tx_cb_t        *tx_cb;        /* transmit control block */
	shrk_dma_element_t     *tx_sdmah;    /* transmit buffer dma handle */
} txd_t;

/* number of longwords to skip between two unchained hw descriptors */
#define SHRK_DESC_SKIP_LEN \
	((sizeof(void *) + sizeof(udi_dma_handle_t)) / sizeof(udi_ubit32_t))

/* endianness macros */
#define SHRK_SWAP_SCGTH(s, u) (((s)->scgth_must_swap == TRUE) ? \
	(UDI_ENDIAN_SWAP_32((udi_ubit32_t)u)) : ((udi_ubit32_t)u))
#define SHRK_SWAP_DESC(d, u) (((d)->swap == TRUE) ? \
	(UDI_ENDIAN_SWAP_32((udi_ubit32_t)u)) : ((udi_ubit32_t)u))

/* RDES0 - Receive Status */
#define RX_OWN           0x80000000U    /* Own Bit                      RDES0*/
#define RX_FF            0x40000000     /* Filtering Fail               RDES0*/
#define RX_FL            0x3FFF0000     /* Frame Length mask, incl CRC  RDES0*/
#define RX_ES            0x00008000     /* Error Summary                RDES0*/
#define RX_DE            0x00004000     /* Descriptor Error             RDES0*/
#define RX_DT            0x00003000     /* Data Type                    RDES0*/
#define RX_RF            0x00000800     /* Runt Frame                   RDES0*/
#define RX_MF            0x00000400     /* Multicast Frame              RDES0*/
#define RX_FS            0x00000200     /* First Descriptor             RDES0*/
#define RX_LS            0x00000100     /* Last Descriptor              RDES0*/
#define RX_TL            0x00000080     /* frame Too Long               RDES0*/
#define RX_CS            0x00000040     /* Collision Seen               RDES0*/
#define RX_FT            0x00000020     /* Frame Type                   RDES0*/
#define RX_RW            0x00000010     /* Receive Watchdog             RDES0*/
#define RX_RE            0x00000008     /* Report on mii Error          RDES0*/
#define RX_DB            0x00000004     /* Dribbling Bit                RDES0*/
#define RX_CE            0x00000002     /* Crc Error                    RDES0*/
#define RX_ZER           0x00000001     /* ZERo for legal length pkts   RDES0*/
#define RX_FL_BIT_POS    16             /* bit position of Frame Length RDES0*/
#define RX_CRC           4              /* sizeof Cyclical Redundancy Check   */

/* RDES1 - Receive control bits, byte counts of receive buffers */
#define RX_RER           0x02000000     /* Receive End of Ring          RDES1*/
#define RX_RCH           0x01000000     /* Receive 2nd address CHained  RDES1*/
#define RX_RBS2          0x003FF800     /* Receive Buffer Size 2        RDES1*/
#define RX_RBS1          0x000007FF     /* Receive Buffer Size 1        RDES1*/
#define RX_RBS2_BIT_POS  11             /* bit position of RBS2         RDES1*/

/* Miscellaneous receive macros */
#define SHRK_RX_BUFFERS_DEF 48          /* number of rx buffers */
#define SHRK_INTR_EVENT_CBS     16      /* number of intr_event_cb's */
#define SHRK_MIN_INTR_EVENT_CBS 2       /* dispatcher intr_event_cb threshold */

/* TDES0 - Transmit Status */
#define TX_OWN           0x80000000U    /* Own Bit                      TDES0*/
#define TX_ES            0x00008000     /* Error Summary                TDES0*/
#define TX_TO            0x00004000     /* transmit jabber TimeOut      TDES0*/
#define TX_LO            0x00000800     /* Loss Of carrier              TDES0*/
#define TX_NC            0x00000400     /* No Carrier                   TDES0*/
#define TX_LC            0x00000200     /* Late Collision               TDES0*/
#define TX_EC            0x00000100     /* Excessive Collisions         TDES0*/
#define TX_HF            0x00000080     /* Heartbeat Fail               TDES0*/
#define TX_CC            0x00000078     /* Collision Count              TDES0*/
#define TX_LF            0x00000004     /* Link Fail Report             TDES0*/
#define TX_UF            0x00000002     /* UnderFlow Error              TDES0*/
#define TX_DE            0x00000001     /* Deferred                     TDES0*/

#define TX_CC_BIT_POS    3     		/* collision cnt bit position   TDES0*/

/* TDES1 - Transmit control bits, byte counts of receive buffers */
#define TX_IC            0x80000000     /* Interrupt on Completion      TDES1*/
#define TX_LS            0x40000000     /* Last Segment                 TDES1*/
#define TX_FS            0x20000000     /* First Segment                TDES1*/
#define TX_FT1           0x10000000     /* Filter Type bit 1            TDES1*/
#define TX_SET           0x08000000     /* SETup packet                 TDES1*/
#define TX_AC            0x04000000     /* Add Crc disable              TDES1*/
#define TX_TER           0x02000000     /* Transmit End of Ring         TDES1*/
#define TX_TCH           0x01000000     /* Tx second address CHained    TDES1*/
#define TX_DPD           0x00800000     /* Disabled PaDding             TDES1*/
#define TX_FT0           0x00400000     /* Filter Type bit 0            TDES1*/
#define TX_TBS2          0x003FF800     /* Transmit Buffer Size 2       TDES1*/
#define TX_TBS1          0x000007FF     /* Transmit Buffer Size 1       TDES1*/
#define TX_TBS2_BIT_POS  11             /* bit position of TBS2         TDES1*/

/* Miscellaneous transmit macros */
#define SHRK_TX_BUFFERS_DEF 48          /* number of tx buffers */

/* size of both rings */
#ifndef SHRK_RING_SIZE
#ifdef SHRK_DEBUG
#define SHRK_RING_SIZE 4			/* tight for debugging */
#else
#define SHRK_RING_SIZE (4096 / sizeof(txd_t))	/* fits in 4K */
#endif
#endif /* SHRK_RING_SIZE */

/* maximum number of s/g elements to send down for one packet */
/* since two elements map to one descriptor, this is half a ring */
#define SHRK_TX_MAXELEM SHRK_RING_SIZE

/* producer/consumer descriptor macros */
#define SHRK_INC_MOD(i,mod) ((((i)+1) == (mod)) ? 0 : ((i)+1))
#define DSC_NEXT(x,N)    SHRK_INC_MOD((N), (x)->max_desc)
#define DSC_NEXT_CDI(x)  (DSC_NEXT((x),((x)->cdi)))
#define DSC_NEXT_PDI(x)  (DSC_NEXT((x),((x)->pdi)))
#define DSC_NAVAIL(x) (((x)->pdi <= (x)->cdi) ? \
	((x)->max_desc + (x)->pdi - (x)->cdi - 1) \
	: ((x)->pdi - (x)->cdi - 1))
#define DSC_ALLCLEAN(x)  ((x)->pdi == (x)->cdi)
#define DSC_CONS(x)      (&(x)->desc[(x)->cdi])  /* consumer descriptor */
#define DSC_PROD(x)      (&(x)->desc[(x)->pdi])  /* producer descriptor */
#define DSC_N(x,N)       (&(x)->desc[(N)])       /* descriptor[N] */

#define SHRK_TIMER_INTERVAL 3		  /* timer for link detect */
#define SHRK_SETUP_TIMEOUT  1		  /* backup timer for tx interrupt */

/* Primary (tx) region data structure - one allocated per ethernet port */
typedef struct shrktx_rdata {
	udi_init_context_t info;          /* initial context - must be first */
	udi_ubit32_t       flags;         /* driver flags */
	udi_trevent_t      ml_trace_mask[3]; /* per-metalanguage trace events */
	udi_ubit8_t        reslev;        /* environment resource levels */
	_udi_nic_state_t   nic_state;     /* network metalanguage state */
	udi_ubit16_t	   link_index;	  /* link parameter index */

	udi_dma_limits_t   dma_limits;    /* platform-specific dma limits */

	udi_nic_bind_cb_t  *bind_cb;      /* NSR bind control block */
	udi_channel_t      ctl_chan;      /* NSR bind/control channel */
	udi_index_t        tx_idx;        /* NSR tx channel index */
	udi_channel_t      tx_chan;       /* NSR tx channel */
	udi_index_t        rx_idx;        /* NSR rx channel index */
	udi_channel_t	   rx_chan;	  /* NSR (unanchored) rx channel */
	udi_bus_bind_cb_t  *bus_cb;       /* bus bind control block */

	udi_channel_t      gio_chan;      /* channel to secondary (rx) region */
	udi_gio_xfer_cb_t  *gio_xfer_cb;  /* gio xfer req/ack control block */
	udi_gio_event_cb_t *gio_event_cb; /* gio event control block */

	udi_boolean_t      reset_stats;   /* nic info reset_statistics */

	udi_ubit8_t	   *srom_contents;
	udi_ubit8_t        factaddr[SHRK_MACADDRSIZE];
					  /* factory ethernet address (srom) */
	shrkaddr_t         hwaddr;        /* current ethernet address */
	udi_nic_info_cb_t  stats;         /* MAC statistics */

	udi_sbit16_t	   index;	  /* general purpose recursion index */
	udi_pio_handle_t   snooze_pio;
	udi_pio_handle_t   pio_handles[SHRK_PIO_COUNT]; /* pio handles */
	udi_ubit32_t attr_values[SHRK_ATTR_COUNT]; /* translated attr values */
	udi_ubit8_t attr_scratch[UDI_MAX_ATTR_SIZE]; /* untranslated attr val */

	udi_nic_status_cb_t *stat_cb;     /* NSR status_ind control block */

	udi_nic_tx_cb_t    *tx_cb;	  /* pending transmit control blocks */
	udi_nic_tx_cb_t    *txrdy_cb;	  /* free transmit control blocks */
	udi_dma_constraints_t txd_cnst;   /* tx descriptors DMA constraints */
	udi_dma_handle_t   txd_dmah;      /* tx descriptors DMA handle */
	txd_t              *desc;         /* transmit descriptors list */
	udi_ubit32_t       cdi;           /* tx consumer descriptor index */
	udi_ubit32_t       pdi;           /* tx producer descriptor index */
	udi_ubit32_t       max_desc;      /* maximum number of tx descriptors */
	udi_ubit32_t       tx_cleanup;    /* clean cbs after this many descrs */
	udi_ubit32_t	   tx_freemask;	  /* how often to request interrupts */
	udi_ubit32_t       txpaddr;       /* phys addr of first tx descriptor */
	udi_ubit32_t       rxpaddr;       /* phys addr of first rx descriptor */
	udi_dma_constraints_t tx_cnst;    /* tx data DMA constraints */
	shrk_dma_element_t *tx_sdmah;	  /* tx DMA handles */
	shrk_dma_element_t *next_sdmah;	  /* next tx DMA handle */
	udi_boolean_t      swap;          /* driver must swap endianness */

	void               *setup_mem;    /* setup frame memory */
	udi_ubit32_t       setup_paddr;   /* phys addr of setup frame memory */
	udi_dma_handle_t   setup_dmah;    /* setup frame DMA handle */
	udi_ubit32_t       setup_cdi;     /* setup descriptor index */
	udi_boolean_t      setup_swap;    /* driver must swap endianness */

	udi_cb_t	   *log_cb;	  /* for async logging */
	udi_cb_t	   *timer_cb;	  /* link probe timer */
	udi_cb_t	   *probe_cb;     /* for pio_trans's in timer */
	udi_cb_t	   *setup_cb;     /* timer for setup frame response */

	udi_ubit32_t	   csr0;	  /* bus mode */
	udi_ubit32_t	   phy;		  /* phy address */
	udi_ubit32_t	   phy_status;	  /* phy status (register 1) */
	udi_ubit32_t	   phy_id;	  /* phy ID (registers 2 and 3) */
} shrktx_rdata_t;

/* Secondary (rx) region data structure - one allocated per ethernet port */
typedef struct shrkrx_rdata {
	udi_init_context_t info;          /* initial context - must be first */
	udi_ubit32_t       flags;         /* bus flags */
	udi_trevent_t      ml_trace_mask[3]; /* per-metalanguage trace events */
	udi_ubit8_t        reslev;        /* environment resource levels */

	udi_channel_t      gio_chan;      /* channel to primary region */
	udi_gio_xfer_cb_t  *gio_xfer_cb;  /* gio xfer req/ack control block */

	udi_index_t        bus_idx;       /* BUS rx region channel index */
	udi_channel_t      bus_chan;      /* BUS rx region channel */

	udi_index_t        rx_idx;        /* NSR rx region channel index */
	udi_channel_t      rx_chan;       /* NSR rx channel */

	udi_pio_handle_t   pio_intr_prep; /* interrupt preprocess pio handle */
	udi_channel_event_cb_t *chan_cb;

	udi_dma_constraints_t rxd_cnst;   /* rx desc/buf dma constraints */
	udi_dma_handle_t   rxd_dmah;      /* rx descriptors dma handle */
	rxd_t              *desc;         /* receive descriptors list */
	udi_ubit32_t       cdi;           /* rx consumer descriptor index */
	udi_ubit32_t       pdi;           /* rx producer descriptor index */
	udi_sbit16_t	   index;	  /* general purpose recursion index */
	udi_ubit32_t       max_desc;      /* maximum number rx descriptors */
	udi_ubit32_t       rxpaddr;       /* phys addr of first rx descriptor */
	udi_nic_rx_cb_t    *rxrdy_cb;     /* free receive control block chain */
	udi_nic_rx_cb_t    *rxcb;         /* pending first NIC received cb */
	shrk_dma_element_t *rx_sdmah;     /* rx dma handles */
	shrk_dma_element_t *next_sdmah;   /* next rx dma handle */
	udi_boolean_t      swap;          /* driver must swap endianness */

	udi_channel_t      intr_chan;     /* interrupt event channel */

	/* statistics - passed to primary region via gio event inline params */
	udi_ubit32_t       rx_packets;    /* receive packets */
	udi_ubit32_t       rx_errors;     /* receive errors */
	udi_ubit32_t       rx_discards;   /* receive packets discarded */
	udi_ubit32_t       tx_underrun;   /* transmit underruns */
	udi_ubit32_t       rx_overrun;    /* receive overruns */
	udi_ubit32_t       rx_desc_unavail; /* out of rx descriptors */

	udi_ubit32_t       intr_event_cbs;  /* number intr_event cbs alloc'd */
	udi_cb_t	   *log_cb;	    /* for async logging */
	_udi_nic_state_t   nic_state;     /* network metalanguage state */
} shrkrx_rdata_t;

/* Per-device flags - used by both regions */
#define SHRK_INIT          0x00000001	/* received usage_ind */
#define SHRK_ENABLING	   0x00000002	/* link timer hasn't fired yet */
#define SHRK_RUNNING	   0x00000004	/* device is working */
#define SHRK_SETUP	   0x00000008	/* setup watchdog timer is active */
#define SHRK_MCA_ALL       0x00000010   /* all multicast address reception */
#define SHRK_MCA_HASH      0x00000020   /* using imperfect address filtering */
#define SHRK_PROMISCUOUS   0x00000040   /* receive all address mode */
#define SHRK_SUSPENDED     0x00000100   /* NIC suspended */
#define SHRK_REMOVED       0x00000200   /* NIC removed */
#define SHRK_SPEED_100M    0x00000400   /* speed is 100 Mbps */
#define SHRK_DUPLEX_FULL   0x00000800   /* full duplex */
#define SHRK_BUS_UNBIND    0x00001000   /* handling bus_unbind */
#define SHRK_INTR_DETACH   0x00002000   /* handling interrupt detach */

/* GIO xfer op definitions -- initiated by secondary region */ 
#define SHRK_GIO_BUS_BIND    UDI_GIO_OP_CUSTOM     /* constraints from parent */
#define SHRK_GIO_TX_COMPLETE (UDI_GIO_OP_CUSTOM+1) /* transmit complete */

/* GIO xfer inline layout definitions */ 
typedef struct shrk_gio_xfer_params {
	/* bus bind parameters */
	udi_dma_constraints_t parent_constraints;
	udi_ubit32_t rxpaddr;
} shrk_gio_xfer_params_t;

static udi_layout_t shrk_gio_xfer_params_layout[] = {
	UDI_DL_DMA_CONSTRAINTS_T, /* parent_constraints */
	UDI_DL_UBIT32_T, 	  /* rxpaddr */
	UDI_DL_END
};

/* GIO event definitions -- initiated by primary region */ 
#define SHRK_GIO_GIO_BIND      0x00   /* processing gio_bind_req */
#define SHRK_GIO_NIC_BIND      0x01   /* processing nd_bind_req */
#define SHRK_GIO_NIC_ENABLE    0x02   /* processing nd_enable_req */
#define SHRK_GIO_NIC_LINK_UP   0x03   /* sent a link_up status */
#define SHRK_GIO_NIC_LINK_DOWN 0x04   /* sent a link_down status */
#define SHRK_GIO_NIC_INFO      0x05   /* processing nd_info_req */
#define SHRK_GIO_NIC_DISABLE   0x06   /* processing nd_disable_req */
#define SHRK_GIO_NIC_UNBIND    0x07   /* processing nd_unbind_req */

typedef struct shrk_gio_event_params {
	/* nic bind parameter */
	udi_channel_t rx_chan;	      /* receive channel to NSR */

	/* info parameters */
	udi_ubit32_t rx_packets;      /* receive packets */
	udi_ubit32_t rx_errors;       /* receive errors */
	udi_ubit32_t rx_discards;     /* receive packets discarded */
	udi_ubit32_t tx_underrun;     /* transmit underruns */
	udi_ubit32_t rx_overrun;      /* receive overruns */
	udi_ubit32_t rx_desc_unavail; /* out of rx descriptors */
	udi_boolean_t reset_stats;    /* reset statistics */
} shrk_gio_event_params_t;

static udi_layout_t shrk_gio_event_params_layout[] = {
	UDI_DL_CHANNEL_T,	      /* rx channel */
	UDI_DL_UBIT32_T,              /* receive packets */
	UDI_DL_UBIT32_T,              /* receive errors */
	UDI_DL_UBIT32_T,              /* receive packets discarded */
	UDI_DL_UBIT32_T,              /* transmit underruns */
	UDI_DL_UBIT32_T,              /* receive overruns */
	UDI_DL_UBIT32_T,              /* out of rx descriptors */
	UDI_DL_BOOLEAN_T,             /* reset statistics */
	UDI_DL_END
};

/* cb scratch layouts */
typedef struct {
	udi_dma_constraints_t parent_constraints;
} shrk_constraints_set_scratch_t;

typedef struct {
	udi_ubit32_t csr0;
	udi_ubit32_t csr6;
	udi_ubit32_t gpreg;
} shrk_nic_reset_scratch_t;

typedef struct {
	udi_ubit32_t txpaddr;
	udi_ubit32_t csr6_bits;
} shrk_txenab_scratch_t;

typedef struct {
	shrk_dma_element_t *sdmah;
} shrk_dma_scratch_t;

typedef union {
	shrk_nic_reset_scratch_t reset;
	shrk_constraints_set_scratch_t constraints;
} shrk_gio_xfer_scratch_t;

typedef union {
	shrk_nic_reset_scratch_t reset;
	shrk_txenab_scratch_t txenab;
} shrk_probe_scratch_t;

/* Per-device media definitions */
#define SHRK_MEDIA_10    0x01           /* 10 Mbps */
#define SHRK_MEDIA_100   0x02           /* 100 Mbps */
#define SHRK_MEDIA_FDX   0x04           /* Full Duplex */
#define SHRK_MEDIA_HDX   0x08           /* Half Duplex */

/* CSR0 - Bus Mode Register */
#define BM_WIE           0x01000000     /* Write and Invalidate Enable   CSR0*/
#define BM_RLE           0x00800000     /* Read Line Enable              CSR0*/
#define BM_RME           0x00200000     /* Read Multiple Enable          CSR0*/
#define BM_CACHE_8       0x00004000     /* 8-longword cache alignment    CSR0*/
#define BM_CACHE_16      0x00008000     /* 16-longword cache alignment   CSR0*/
#define BM_CACHE_32      0x0000C000     /* 32-longword cache alignment   CSR0*/
#define BM_PBL_32        0x00002000     /* 32-lw Programmable Burst Len  CSR0*/
#define BM_DSL_MASK      0x0000007C     /* Descriptor Skip Length        CSR0*/
#define BM_SWR           0x00000001     /* Software Reset                CSR0*/
#define BM_DSL_BIT_POS   2              /* bit position of DSL           CSR0*/

/* CSR5 - Status Register */
#define ST_EB_MASK       0x03800000     /* Error Bits Mask               CSR5*/
#define ST_EB_PARITY     0x00000000     /* Parity Error                  CSR5*/
#define ST_EB_MABORT     0x00800000     /* Master Abort                  CSR5*/
#define ST_EB_TABORT     0x01000000     /* Target Abort                  CSR5*/
#define ST_TS_MASK       0x00700000     /* Transmit Process State Mask   CSR5*/
#define ST_RPS_MASK      0x000E0000     /* Receive Proceess State Mask   CSR5*/
#define ST_NIS           0x00010000     /* Normal Interrupt Summary      CSR5*/
#define ST_AIS           0x00008000     /* Abnormal Interrupt Summary    CSR5*/
#define ST_ERI           0x00004000     /* Early Receive Interrupt       CSR5*/
#define ST_FBE           0x00002000     /* Fatal Bus Error               CSR5*/
#define ST_GTE           0x00000800     /* General Timer Expired         CSR5*/
#define ST_ETI           0x00000400     /* Early Transmit Interrupt      CSR5*/
#define ST_RWT           0x00000200     /* Receive Watchdog Timeout      CSR5*/
#define ST_RPS           0x00000100     /* Receive Process Stopped       CSR5*/
#define ST_RU            0x00000080     /* Receive Buffer Unavailable    CSR5*/
#define ST_RI            0x00000040     /* Receive Interrupt             CSR5*/
#define ST_UNF           0x00000020     /* Transmit Underflow            CSR5*/
#define ST_TJT           0x00000008     /* Transmit Jabber Timeout       CSR5*/
#define ST_TU            0x00000004     /* Transmit Buffer Unavailable   CSR5*/
#define ST_TPS           0x00000002     /* Transmit Process Stopped      CSR5*/
#define ST_TI            0x00000001     /* Transmit Interrupt            CSR5*/
#define STAT_CLR_LO16        0xEFEF     /* All status bits               CSR5*/
#define STAT_CLR_HI16    0x0001         /* All status bits               CSR5*/
 
/* CSR6 - Operating Mode Register */
#define OM_SC            0x80000000U    /* Special Capture Effect Enable CSR6*/
#define OM_RA            0x40000000     /* Receive All stats in RDES0<30>CSR6*/
#define OM_MBO           0x02000000     /* Must Be One                   CSR6*/
#define OM_SCR           0x01000000     /* Scrambler Mode                CSR6*/
#define OM_PCS           0x00800000     /* PCS Functions active          CSR6*/
#define OM_TTM           0x00400000     /* Transmit Threshold Mode       CSR6*/
#define OM_SF            0x00200000     /* Store and Forward             CSR6*/
#define OM_HBD           0x00080000     /* Heartbeat Disable             CSR6*/
#define OM_PS            0x00040000     /* Port Select                   CSR6*/
#define OM_CA            0x00020000     /* Capture Effect Enable         CSR6*/
#define OM_TR_MASK       0x0000C000     /* Threshold Control Bits        CSR6*/
#define OM_ST            0x00002000     /* Start/Stop Transmit command   CSR6*/
#define OM_FC            0x00001000     /* Force Collision mode          CSR6*/
#define OM_EXL           0x00000800     /* EXternal Loopback             CSR6*/
#define OM_INL           0x00000400     /* INternal Loopback             CSR6*/
#define OM_NOL           0x00000000     /* NOrmal Loopback               CSR6*/
#define OM_FD            0x00000200     /* Full Duplex mode              CSR6*/
#define OM_PM            0x00000080     /* Pass all Multicast            CSR6*/
#define OM_PR            0x00000040     /* PRomiscuous                   CSR6*/
#define OM_SB            0x00000020     /* Start/Stop Backoff Counter    CSR6*/
#define OM_IF            0x00000010     /* Inverse Filtering             CSR6*/
#define OM_PB            0x00000008     /* Pass Bad frames               CSR6*/
#define OM_HO            0x00000004     /* Hash-Only filtering mode      CSR6*/
#define OM_SR            0x00000002     /* Start/Stop Receive            CSR6*/
#define OM_HP            0x00000001     /* Hash/Perfect recieve filter   CSR6*/
#define CSR6_RESET_100 (OM_SC|OM_MBO|OM_SCR|OM_PCS|OM_SF|OM_HBD|OM_PS|OM_CA|OM_SB)
#define CSR6_RESET_10  (OM_SC|OM_MBO|OM_TTM|OM_SF|OM_CA|OM_SB)

/* CSR7 - Interrupt Enable Register */
#define IE_NI            0x00010000     /* Normal Interrupt summary enbl CSR7*/
#define IE_AI            0x00008000     /* Abnormal Interrupt summry enb CSR7*/
#define IE_ERE           0x00004000     /* Early Receive interrupt enbl  CSR7*/
#define IE_FBE           0x00002000     /* Fatal Bus Error enable        CSR7*/
#define IE_GPT           0x00000800     /* General Purpose Timer enable  CSR7*/
#define IE_ETE           0x00000400     /* Early Transmit interrupt Enbl CSR7*/
#define IE_RW            0x00000200     /* Receive Watchdog timeout enbl CSR7*/
#define IE_RS            0x00000100     /* Receive Stopped enable        CSR7*/
#define IE_RU            0x00000080     /* Receive buffer Unavail enbl   CSR7*/
#define IE_RI            0x00000040     /* Receive Interupt enable       CSR7*/
#define IE_UN            0x00000020     /* Underflow interrupt enable    CSR7*/
#define IE_TJ            0x00000008     /* Transmit Jabber timeout enbl  CSR7*/
#define IE_TU            0x00000004     /* Transmit buff Unavailable enb CSR7*/
#define IE_TS            0x00000002     /* Transmit Stopped enable       CSR7*/
#define IE_TI            0x00000001     /* Transmit Interrupt enable     CSR7*/

/* Enabled interrupts */
#define INT_ENAB_LO16 	(IE_FBE|IE_RU|IE_RI|IE_TJ|IE_TI)
#define INT_ENAB_HI16   (IE_NI>>16)

/* CSR8 - Missed Frames and Overflow Counter */
#define MFO_OCO          0x10000000     /* Overflow Counter Overflow     CSR8*/
#define MFO_FOC_MASK     0x0FFE0000     /* FIFO Overflow Counter         CSR8*/
#define MFO_MFO          0x00010000     /* Missed Frame Overflow         CSR8*/
#define MFO_MFC_MASK     0x0000FFFF     /* Missed Frame Counter          CSR8*/

/* CSR9 - Boot ROM, Serial ROM, and MII Management Register */
#define MII_MDI          0x00080000     /* MII Management read Data In   CSR9*/
#define MII_MDI_BIT_POS  19             /* bit position of MDI               */
#define MII_MII          0x00040000     /* MII Management Operation Mode CSR9*/
#define MII_MDO          0x00020000     /* MII Management write Data Out CSR9*/
#define MII_MDC          0x00010000     /* MII Management Clock          CSR9*/
#define ROM_RD           0x00004000     /* ROM Read Operation            CSR9*/
#define ROM_WR           0x00002000     /* ROM Write Operation           CSR9*/
#define ROM_BR           0x00001000     /* Boot ROM Select               CSR9*/
#define ROM_SR           0x00000800     /* Serial ROM Select             CSR9*/
#define ROM_REG          0x00000400     /* External Register Select      CSR9*/
#define ROM_DATA         0x000000FF     /* Boot ROM Data/SROM Control    CSR9*/
#define ROM_SDOUT        0x00000008     /* Serial ROM Data Out           CSR9*/
#define ROM_SDIN         0x00000004     /* Serial ROM Data In            CSR9*/
#define ROM_SCLK         0x00000002     /* Serial ROM Clock              CSR9*/
#define ROM_SCS          0x00000001     /* Serial ROM Chip Select        CSR9*/

/* CSR12 - 21140 General-Purpose Port Register */
#define GP_GPC           0x00000100     /* General-Purpose Control       CSR12*/
#define GP_OSI_GPCONTROL 0x0000000f     /* Osicom 2300 General-Purpose Control*/
/* Osicom 2300 Series General-Purpose Port Register */
#define GP_OSI_LPBKPDX   0x00000001     /* when=0, loopback to PDT and PDR   */
#define GP_OSI_LPBKXCVR  0x00000002     /* when=1, loopback to transceiver   */
#define GP_OSI_MODE_SEL  0x00000008     /* 0 => 10Mbps HDX, 1 => 10Mbps FDX  */
#define GP_OSI_FDPLX10M  0x00000010     /* when=1 10M FDX capability found   */
#define GP_OSI_SIGDET    0x00000020     /* when=1 signal detected on wire    */
#define GP_OSI_SYM_DET   0x00000040     /* when=1 descrambler locked on input*/
#define GP_OSI_LINK10M_L 0x00000080     /* when=1 10M SIA link pulse found   */
#define GP_OSI_HDX       0x01           /* Osicom GEP setting for half-duplex*/
#define GP_OSI_FDX       0x09           /* Osicom GEP setting for full-duplex*/

/* CSR offsets from base address */
#define SHRK_CSR0        0              /* CSR0: Bus mode              */
#define SHRK_CSR1        0x08           /* CSR1: Transmit Poll Demand  */
#define SHRK_CSR2        0x10           /* CSR2: Receive Poll Demand   */
#define SHRK_CSR3        0x18           /* CSR3: Receive descr addr    */
#define SHRK_CSR4        0x20           /* CSR5: Transmit descr addr   */
#define SHRK_CSR5        0x28           /* CSR5: Status                */
#define SHRK_CSR6        0x30           /* CSR6: Operating mode        */
#define SHRK_CSR7        0x38           /* CSR7: Interrupt Enable      */
#define SHRK_CSR8        0x40           /* CSR8: Missed Frames and Overflow */
#define SHRK_CSR9        0x48           /* CSR9: ROMs and MII          */
#define SHRK_CSR12       0x60           /* CSR12: 21140 General Purpose */
					/* CSR12: 21143 SIA Status     */
#define SHRK_CSR13       0x68           /* CSR13: 21143 SIA Connectivity */
#define SHRK_CSR14       0x70           /* CSR14: 21143 SIA Control    */
#define SHRK_CSR15       0x78           /* CSR15: Watchdog timer       */
					/* CSR15: 21143 General Purpose */
#define SHRK_PIO_LENGTH  0x80           /* 128 bytes of CSR registers  */

/* CFDD Configuration Register offset and Interphase bitfields */
#define DEC2114_CFDD 0x40

#define IFE_CFDD_SLEEP_MODE         0x80000000
#define IFE_CFDD_SNOOZE_MODE        0x40000000
#define IFE_CFDD_WAKEUP             0x00000000

/* definitions for setup frames */
#define SHRK_SET_SZ      192            /* size of the setup frame */
#define SHRK_SET_MAXPERF 16             /* maximum perfect filtering addrs */
#define SHRK_SET_MINADDR 2              /* minimum perfect filtering addrs */
#define SHRK_SET_INIT    0              /* setup cmd initial setup frame */
#define SHRK_SET_ADDR    1              /* setup cmd set mac addr */
#define SHRK_SET_ENABMC  2              /* setup cmd enable multicast addr */
#define SHRK_SET_DISABMC 3              /* setup cmd disable multicast addr */
#define SHRK_SET_NUMTXDS 2              /* use two txdescs per setup frame */

typedef struct imperfect {
	udi_ubit16_t     hash;
	udi_ubit16_t     dont_care;
} imperfect_t;
#define SHRK_HASH_SZ     (sizeof(udi_ubit16_t) * 8) 
#define SHRK_HASH_OFF    156            /* unicast addr offset in setup buf */

/* definitions for MII serial management */
#define SHRK_MAX_PHY     32             /* maximum phy address (5 bit addr) */

#define SHRK_MII_READ	 0x06		/* read command (with start prefix ) */
#define SHRK_MII_WRITE	 0x05		/* write command (with start prefix ) */

#define SHRK_BMCR        0x00           /* Basic Mode Control Register */ 
#define SHRK_BMSR        0x01           /* Basic Mode Status Register */ 
#define SHRK_PHYIDR1     0x02           /* PHY ID high */
#define SHRK_PHYIDR2     0x03           /* PHY ID low */
#define SHRK_ANAR        0x04           /* Auto-Negotiation Advertisement Reg*/
#define SHRK_ANAR_INIT   0x1e1          /* ANARinit: 10/100Base-T/TXfdx 802.3*/
#define SHRK_ANLPAR      0x05           /* Auto-Neg Link Partner Ability Reg */ 
#define SHRK_ANER        0x06           /* Auto-Negotiation Expansion Reg */
#define SHRK_PAR         0x19           /* Phy Address Register */ 

#define SHRK_BMCR_RESET  0x8000         /* Software Reset */ 
#define SHRK_BMCR_AUTO   0x1000         /* BMCR - enable auto negotiation */
#define SHRK_BMCR_ASTRT  0x0200         /* BMCR - restart auto negotiation */

#define SHRK_BMSR_ANC    0x0020         /* BMSR - Auto-Negotiation Complete */
#define SHRK_BMSR_ANA    0x0008         /* BMSR - Auto-Negotiation Ability */
#define SHRK_BMSR_LINK   0x0004         /* BMSR - link status */

#define SHRK_ANLPAR_T4    0x0200	/* ANLPAR - 100baseT4 */
#define SHRK_ANLPAR_TX_FD 0x0100	/* ANLPAR - 100baseTX full duplex */
#define SHRK_ANLPAR_TX    0x0080	/* ANLPAR - 100baseTX */
#define SHRK_ANLPAR_10_FD 0x0040	/* ANLPAR - 10baseT full duplex */
#define SHRK_ANLPAR_10    0x0020	/* ANLPAR - 10baseT */

/* Phy Address Register - also used in per-device media flags (mii_par) */
#define SHRK_PAR_AN      0x0400         /* Auto-Neg mode status (1=enabled) */
#define SHRK_PAR_FDX     0x0080         /* Full-DupleX mode */
#define SHRK_PAR_10      0x0040         /* 10Mbp/s operation */


#ifdef SHRK_DEBUG

/* TODO3 change DEBUG printfs to use tracing/logging */
#define DEBUG_VAR(x) x
#define	DEBUG00(x) { udi_debug_printf x ; }
/* DEBUG01 - function entry points */
#define	DEBUG01(x) if (shrkdebug&0x00000001) { udi_debug_printf x ; }
/* DEBUG02 - should be logging */
#define	DEBUG02(x) if (shrkdebug&0x00000002) { udi_debug_printf x ; }
/* DEBUG03 - state changes */
#define	DEBUG03(x) if (shrkdebug&0x00000004) { udi_debug_printf x ; }
#define	DEBUG04(x) if (shrkdebug&0x00000008) { udi_debug_printf x ; }
#define	DEBUG05(x) if (shrkdebug&0x00000010) { udi_debug_printf x ; }
#define	DEBUG06(x) if (shrkdebug&0x00000020) { udi_debug_printf x ; }
#define	DEBUG07(x) if (shrkdebug&0x00000040) { udi_debug_printf x ; }
#define	DEBUG08(x) if (shrkdebug&0x00000080) { udi_debug_printf x ; }
#define	DEBUG09(x) if (shrkdebug&0x00000100) { udi_debug_printf x ; }
#define	DEBUG10(x) if (shrkdebug&0x00000200) { udi_debug_printf x ; }
/* DEBUG11 teardown */
#define	DEBUG11(x) if (shrkdebug&0x00000400) { udi_debug_printf x ; }
/* DEBUG12 timer */
#define	DEBUG12(x) if (shrkdebug&0x00000800) { udi_debug_printf x ; }
/* DEBUG13-14 receive */
#define	DEBUG13(x) if (shrkdebug&0x00001000) { udi_debug_printf x ; }
#define	DEBUG14(x) if (shrkdebug&0x00002000) { udi_debug_printf x ; }
/* DEBUG15-16 transmit */
#define	DEBUG15(x) if (shrkdebug&0x00004000) { udi_debug_printf x ; }
#define	DEBUG16(x) if (shrkdebug&0x00008000) { udi_debug_printf x ; }

#else

#define DEBUG_VAR(x)

/* DEBUG00 - stub out printfs */
#define	DEBUG00(x)
#define	DEBUG01(x)
#define	DEBUG02(x)
#define	DEBUG03(x)
#define	DEBUG04(x)
#define	DEBUG05(x)
#define	DEBUG06(x)
#define	DEBUG07(x)
#define	DEBUG08(x)
#define	DEBUG09(x)
#define	DEBUG10(x)
#define	DEBUG11(x)
#define	DEBUG12(x)
#define	DEBUG13(x)
#define	DEBUG14(x)
#define	DEBUG15(x)
#define	DEBUG16(x)

#endif

/* Function prototypes - shrkudi.c */
void shrk_log(shrktx_rdata_t *, udi_status_t, udi_ubit32_t, ...);
void shrkrx_log(shrkrx_rdata_t *, udi_status_t, udi_ubit32_t, ...);
udi_log_write_call_t shrk_log_complete;
udi_log_write_call_t shrkrx_log_complete;
udi_log_write_call_t shrk_log_channel;
void shrk_nd_bind_ack(shrktx_rdata_t *, udi_status_t);
udi_log_write_call_t shrk_nd_bind_log;
void shrk_rxchan_written(udi_cb_t *, udi_buf_t *);
udi_pio_map_call_t shrk_pio_map;
void shrk_read_srom(udi_cb_t *, void *);
udi_pio_trans_call_t shrk_wakeup_done, shrk_process_srom;
udi_pio_trans_call_t shrk_mdio_read, shrk_mdio_read1, shrk_mdio_read2;
udi_pio_trans_call_t shrk_mdio_write;
udi_instance_attr_get_call_t shrk_instance_attr_get;
udi_pio_trans_call_t shrk_reset_complete;
udi_cb_alloc_call_t shrk_tx_cblist;
udi_cb_alloc_call_t shrk_tx_cb;
udi_channel_spawn_call_t shrk_rx_chan_spawned;
udi_channel_spawn_call_t shrk_tx_chan_spawned;
void shrk_nic_reset(udi_cb_t *);
udi_boolean_t shrk_check_link_index(shrktx_rdata_t *);
void shrk_new_link_index(shrktx_rdata_t *);
udi_pio_trans_call_t shrk_mii_link;
udi_pio_trans_call_t shrk_mii_anlpar;
udi_pio_trans_call_t shrk_gpreg_link;
udi_timer_tick_call_t shrk_timer;
udi_timer_expired_call_t shrk_setup_timer;
udi_cb_alloc_call_t shrk_status_ind_cb;
udi_cb_alloc_call_t shrk_probe_cb;
udi_cb_alloc_call_t shrk_timer_cb;
udi_cb_alloc_call_t shrk_setup_timer_cb;
udi_cb_alloc_call_t shrk_gio_event_cb;
udi_pio_trans_call_t shrk_disabled_pio;
udi_pio_trans_call_t shrk_nsr_ctrl_ack;
void shrk_ctrl_macaddr_written(udi_cb_t *, udi_buf_t *);
void shrk_nd_ctrl_req(udi_nic_ctrl_cb_t *);
udi_pio_trans_call_t shrk_rxenab_pio;
udi_pio_trans_call_t shrk_tx_done;
void shrk_tx(udi_cb_t *);
void shrk_sked_xmit(shrktx_rdata_t *dv);
udi_dma_buf_map_call_t shrk_tx_buf_map;
void shrkrx_sked_receive(shrkrx_rdata_t *);
udi_dma_buf_map_call_t shrkrx_rx_buf_map;
udi_cb_alloc_call_t shrkrx_gio_bind_cb;
void shrkrx_intr_detach_ack(udi_intr_detach_cb_t *);
udi_cb_alloc_call_t shrkrx_gio_close_cb;
udi_cb_alloc_call_t shrk_log_cb, shrkrx_log_cb;
udi_cb_alloc_call_t shrkrx_gioxfer_cb;
void shrkrx_rx_dmah(udi_cb_t *, udi_dma_handle_t);
void shrkrx_rx_chan_anchored(udi_cb_t *, udi_channel_t);
udi_cb_alloc_call_t shrkrx_intr_attach_cb;
void shrkrx_intr_chan(udi_cb_t *, udi_channel_t);
udi_pio_map_call_t shrkrx_intr_preprocess_map;
udi_dma_mem_alloc_call_t shrk_setup_alloc;
udi_dma_mem_alloc_call_t shrk_txdesc_alloc;
udi_dma_mem_alloc_call_t  shrkrx_rxdesc_alloc;
udi_mem_alloc_call_t shrk_dma_alloc;
udi_mem_alloc_call_t shrkrx_dma_alloc;
void shrk_tx_dmah(udi_cb_t *, udi_dma_handle_t);
udi_dma_constraints_attr_set_call_t shrk_tx_constraints_set;
udi_dma_constraints_attr_set_call_t shrk_txd_constraints_set;
udi_dma_constraints_attr_set_call_t shrkrx_rx_constraints_set;
void shrkrx_intr_buf(udi_cb_t *, udi_buf_t *);
udi_cb_alloc_call_t shrkrx_intr_cb;
void shrkrx_rx(shrkrx_rdata_t *);
void shrk_clean_ring(shrktx_rdata_t *);
void shrkrx_clean_ring(shrkrx_rdata_t *);
udi_pio_trans_call_t shrkrx_intr_complete_pio;
void shrk_addrstr(shrkaddr_t, char *);
udi_pio_trans_call_t shrk_addrs_setup;
void shrk_build_setup_frame(shrktx_rdata_t *, udi_nic_ctrl_cb_t *);
void shrk_send_setup_frame(udi_cb_t *, udi_ubit32_t);
udi_pio_trans_call_t shrk_setup_poll_done;
void shrk_nsr_enable_ack(udi_nic_cb_t *);
udi_pio_trans_call_t shrk_txenab_pio;
udi_pio_trans_call_t shrk_txenab_pio1;
void shrk_txcomplete(shrktx_rdata_t *);
void shrk_setup_complete(shrktx_rdata_t *, txd_t *);
void shrk_tx_free(shrktx_rdata_t *, txd_t *,
			udi_nic_tx_cb_t **, udi_nic_tx_cb_t **);
udi_ubit32_t shrk_crc32_mchash(udi_ubit8_t *);
void shrk_set_perfaddr(shrktx_rdata_t *, udi_ubit8_t *, const shrkaddr_t);
/* debug functions */
void shrktxset(void *);
void shrktxd(shrktx_rdata_t *);
void shrkrxd(shrkrx_rdata_t *);
void shrktxcb(shrktx_rdata_t *);
