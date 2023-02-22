
/*
 * File: udi_dpt/udi_dpt.h
 *
 * Example driver for DPT SmartCache and SmartRaid drivers.
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

/* Debug and non-debug equivalent macros */

#if defined(DPT_DEBUG)

#define DPT_TRACE(args)		udi_trace_write args 
#define DPT_ASSERT(stmt)	udi_assert(stmt) 
#define STATIC			/**/
#define DPT_DEBUG_VAR(x)	x

#else

#define DPT_TRACE(arg)		/**/
#define DPT_ASSERT(stmt)	/**/
#if !defined STATIC
#  define STATIC			static
#endif
#define DPT_DEBUG_VAR(x)

#endif /* defined(DPT_DEBUG) */

/*
 *-----------------------------------------------------------------------
 * UDI related definitions
 *-----------------------------------------------------------------------
 */ 
/*
 * Scratch sizes for each control block type.
 */
#define DPT_MGMT_SCRATCH	0
#define SCSI_BIND_SCRATCH	sizeof(dpt_scsi_bind_scratch_t)
#define SCSI_IO_SCRATCH		sizeof(dpt_req_t)
#define SCSI_CTL_SCRATCH	0
#define SCSI_EVENT_SCRATCH	0
#define BUS_BIND_SCRATCH	sizeof(dpt_pio_scratch_t)
#define INTR_ATTACH_SCRATCH	0
#define INTR_EVENT_SCRATCH	0
#define INTR_DETACH_SCRATCH	0
#define DPT_TIMER_SCRATCH	0

/*
 * PIO properties of the physical I/O registers we'll use on the DPT device.
 */
#define DPT_REGSET	0	/* first (and only) register set */
#define DPT_BASE	0	/* base address within this register set */
#define DPT_LENGTH	0x10	/* 16 bytes worth of registers */
#define DPT_PACE	0	/* wait 0 microseconds between accesses */

#define DPT_INTR_SPAWN_IDX	0

/*
 * SCSI transfer scratch offsets for PIO trans list processing.
 */
#define DPT_PIO_SCRATCH_PADDR		offsetof(dpt_pio_scratch_t, paddr)
#define DPT_PIO_SCRATCH_CMD		offsetof(dpt_pio_scratch_t, cmd)
#define DPT_PIO_SCRATCH_EATA_CFG	offsetof(dpt_pio_scratch_t, eata_cfgp)

/*
 * Operational codes for group zero commands used in the driver
 */

#define SS_TEST		0X00		/* Test unit ready	*/
#define SS_REQSEN	0X03		/* Request sense	*/
#define SS_INQUIR	0X12		/* Inquire		*/
#define SS_LOCK		0X1E		/* Prevent/Allow	*/

/************************************************************************* 
**		Controller IO Register Offset Definitions		** 
*************************************************************************/ 
#define DPT_HA_COMMAND		0x07	/* Command register		*/ 
#define DPT_HA_STATUS		0x07	/* Status register		*/ 
#define DPT_HA_DMA_BASE		0x02	/* LSB for DMA Physical Address */ 
#define DPT_HA_ERROR		0x01	/* Error register		*/ 
#define DPT_HA_DATA		0x00	/* Data In/Out register	*/ 
#define DPT_HA_AUX_STATUS	0x08	/* Auxiliary Status register	*/
	
#define DPT_HA_AUX_BUSY		0x01	/* Aux Reg Busy bit.	*/ 
#define DPT_HA_AUX_INTR		0x02	/* Aux Reg Interrupt Pending.   */
	
#define DPT_HA_ST_ERROR		0x01	/* HA_STATUS register bit defs  */ 
#define DPT_HA_ST_INDEX		0x02
#define DPT_HA_ST_CORRCTD	0x04
#define DPT_HA_ST_DRQ		0x08
#define DPT_HA_ST_SEEK_COMP	0x10
#define DPT_HA_ST_WRT_FLT	0x20
#define DPT_HA_ST_READY		0x40
#define DPT_HA_ST_BUSY		0x80
	
#define DPT_HA_ER_DAM		0x01	/* HA_ERROR register bit defs   */ 
#define DPT_HA_ER_TRK_0		0x02
#define DPT_HA_ER_ABORT		0x04
#define DPT_HA_ER_ID		0x10
#define DPT_HA_ER_DATA_ECC	0x40
#define DPT_HA_ER_BAD_BLOCK	0x80
	
/************************************************************************* 
**		Controller Commands Definitions				** 
*************************************************************************/ 
#define DPT_CP_READ_CFG_PIO	0xF0	/* Read Configuration Data, PIO */
#define DPT_CP_PIO_CMD		0xF2	/* Execute Command, PIO	*/
#define DPT_CP_TRUCATE_CMD	0xF4	/* Trunc Transfer Command, PIO  */
#define DPT_CP_EATA_RESET	0xF9	/* Reset Controller And SCSI Bus*/
#define DPT_CP_IMMEDIATE	0xFA	/* EATA Immediate command	*/
#define DPT_CP_READ_CFG_DMA	0xFD	/* Read Configuration Data, DMA */ 
#define DPT_CP_DMA_CMD		0xFF	/* Execute Command, DMA	*/
#define DPT_ECS_EMULATE_SEN	0xD4	/* Get Emmulation Sense Data	*/
	
#define DPT_CP_ABORT_CMD	0x03
#define DPT_CP_ABORT_MSG	0x06

/************************************************************************* 
**		EATA Command/Status Packet Definitions			** 
*************************************************************************/ 
#define DPT_HA_DATA_IN		0x80
#define DPT_HA_DATA_OUT		0x40
#define DPT_HA_INTERPRET	0x20
#define DPT_HA_SCATTER		0x08
#define DPT_HA_AUTO_REQ_SEN	0x04
#define DPT_HA_HBA_INIT		0x02
#define DPT_HA_SCSI_RESET	0x01
	
#define DPT_SP_GOOD		0x00	/* No error			*/
#define DPT_SP_SELTO		0x01	/* Device Selection Time Out	*/
#define DPT_SP_CMDTO		0x02	/* Device Command Time Out	*/
#define DPT_SP_RESET		0x03	/* SCSI Bus was RESET !		*/
#define DPT_SP_INITPWR		0x04	/* Initial Controller Power Up  */
#define DPT_SP_UBUSPHASE	0x05	/* Unexpected BUS Phase		*/
#define DPT_SP_UBUSFREE		0x06	/* Unexpected BUS Free		*/
#define DPT_SP_BUSPARITY	0x07	/* SCSI Bus Parity Error	*/
#define DPT_SP_SHUNG		0x08	/* SCSI Bus Hung		*/
#define DPT_SP_UMSGRJCT		0x09	/* Unexpected Message Reject	*/
#define DPT_SP_RSTSTUCK		0x0A	/* SCSI Bus Reset Stuck		*/
#define DPT_SP_RSENSFAIL	0x0B	/* Auto-Request Sense Failed	*/
#define DPT_SP_PARITY		0x0C	/* HBA Memory Parity error	*/
#define DPT_SP_CPABRTNA		0x0D	/* CP aborted - NOT on Bus	*/
#define DPT_SP_CPABORTED	0x0E	/* CP aborted - WAS on Bus	*/
#define DPT_SP_CPRST_NA		0x0F	/* CP was reset - NOT on Bus	*/
#define DPT_SP_CPRESET		0x10	/* CP was reset - WAS on Bus	*/
#define DPT_SP_ECCERR		0x11	/* Controller RAM ECC Error     */
#define DPT_SP_PCIPARITY	0x12	/* PCI Parity Error		*/
#define DPT_SP_PCIMASTER	0x13	/* PCI Master Abort		*/
#define DPT_SP_PCITARGET	0x14	/* PCI Target Abort		*/
#define DPT_SP_PCISIGNAL	0x15	/* PCI Signaled Target Abort	*/

#define DPT_HA_STATUS_MASK	0x7F
#define DPT_HA_STATUS_EOC	0x80

/* SCSI (Target/Device) Status */
#define DPT_SP_S_GOOD		0x00	/* No Error, Good Status */
#define DPT_SP_S_CKCON		0x02	/* Check Condition */
#define DPT_SP_S_METCON		0x04	/* Condition Met, Good Status */
#define DPT_SP_S_BUSY		0x08	/* Device Busy */
#define DPT_SP_S_QFULL		0x28	/* Device Queue Full */

#define DPT_HA_IDENTIFY_MSG	0x80
#define DPT_HA_DISCO_RECO	0x40	/* Disconnect/Reconnect	*/
	
#define DPT_SET_ATTR_BOOLEAN(attr, name, val)	\
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_BOOLEAN; \
		(attr)->attr_length = sizeof(udi_boolean_t); \
		UDI_ATTR32_SET((attr)->attr_value, (val))

#define DPT_SET_ATTR32(attr, name, val)	\
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_UBIT32; \
		(attr)->attr_length = sizeof(udi_ubit32_t); \
		UDI_ATTR32_SET((attr)->attr_value, (val))

#define DPT_SET_ATTR_ARRAY8(attr, name, val, len) \
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_ARRAY8; \
		(attr)->attr_length = (len); \
		udi_memcpy((attr)->attr_value, (val), (len))

#define DPT_SET_ATTR_STRING(attr, name, val, len) \
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_STRING; \
		(attr)->attr_length = (len); \
		udi_strncpy_rtrim((char *)(attr)->attr_value, (val), (len))

/*
 * DPT Specific PCI I/O Address Adjustment
 */
#define DPT_PCI_OFFSET		0x10

/*******************************************************************************
** ReadConfig data structure - this structure contains the EATA Configuration **
*******************************************************************************/
#define HBA_BUS_TYPE_LENGTH	32	/* Length For Bus Type Field	*/ 

typedef struct dpt_eata_cfg {
	udi_ubit8_t ConfigLength[4];	/* Len in bytes after this field.*/
	udi_ubit8_t EATAsignature[4];	/* EATA Signature Field		*/
	udi_ubit8_t EATAversion;	/* EATA Version Number		*/
#ifdef OUT
	udi_ubit8_t OverLapCmds:1;	/* TRUE if overlapped cmds supported  */
	udi_ubit8_t TargetMode:1;	/* TRUE if target mode supported*/
	udi_ubit8_t TrunNotNec:1;
	udi_ubit8_t MoreSupported:1;
	udi_ubit8_t DMAsupported:1;	/* TRUE if DMA Mode Supported	*/
	udi_ubit8_t DMAChannelValid:1;	/* TRUE if DMA Channel Valid.	*/
	udi_ubit8_t ATAdevice:1;
	udi_ubit8_t HBAvalid:1;	/* TRUE if HBA field is valid	*/
#else
	udi_ubit8_t capabilities;
#endif
	udi_ubit8_t PadLength[2];
	udi_ubit8_t Reserved0;		/* Reserved field		*/
	udi_ubit8_t Chan_2_ID;		/* Channel 2 Host Adapter ID	*/
	udi_ubit8_t Chan_1_ID;		/* Channel 1 Host Adapter ID	*/
	udi_ubit8_t Chan_0_ID;		/* Channel 0 Host Adapter ID	*/
	udi_ubit8_t CPlength[4];	/* Command Packet Length	*/
	udi_ubit8_t SPlength[4];	/* Status Packet Length		*/
	udi_ubit8_t QueueSize[2];	/* Controller Que depth		*/
	udi_ubit8_t Reserved1[2];	/* Reserved field		*/
	udi_ubit8_t SG_Size[2];	/* Number Of S. G. Elements Supported */
#ifdef OUT
	udi_ubit8_t IRQ_Number:4;	/* IRQ Ctlr is on ... ie 14,15,12*/
	udi_ubit8_t IRQ_Trigger:1;	/* 0 =Edge Trigger, 1 =Level Trigger  */
	udi_ubit8_t Secondary:1;	/* TRUE if ctlr not parked on 0x1F0   */
	udi_ubit8_t DMA_Channel:2;	/* DMA Channel used if PM2011	*/
#else
	udi_ubit8_t params;
#endif
	udi_ubit8_t Reserved2;		/* Reserved Field		*/
#ifdef OUT
	udi_ubit8_t Disable:1;		/* Secondary I/O Address Disabled*/
	udi_ubit8_t ForceAddr:1;	/* PCI Forced To EISA/ISA Address*/
	udi_ubit8_t Reserved3:6;	/* Reserved Field		*/
#else
	udi_ubit8_t ioaddr;
#endif
	udi_ubit8_t max_chan_tgt;	/* Bits 0-4 : maxmum target id
					   supported.
					   Bits 5-7 maximum number of buses 
					   (channels) supported on this HBA */
	udi_ubit8_t max_lun;		/* Maximum LUN Supported	*/

#ifdef OUT
	udi_ubit8_t Reserved4:6;	/* Reserved Field		*/
	udi_ubit8_t PCIbus:1;		/* PCI Adapter Flag		*/
	udi_ubit8_t EISAbus:1;		/* EISA Adapter Flag		*/
#else
	udi_ubit8_t busval;
#endif
	udi_ubit8_t RaidNum;		/* Raid Host Adapter Number	*/
	udi_ubit8_t Reserved5;		/* Reserved Field			*/
	udi_ubit8_t pad[512 - 38];
} dpt_eata_cfg_t;

typedef struct dpt_pio_scratch {
	udi_ubit32_t	paddr;
	udi_ubit8_t	cmd;
	udi_ubit32_t	eata_cfgp;
} dpt_pio_scratch_t;


typedef struct dpt_scsi_bind_scratch {
	udi_dma_handle_t dma_handle;
} dpt_scsi_bind_scratch_t;

#define	DPT_VID_LEN	8		/* inquiry vendor id length	*/
#define DPT_PID_LEN	16		/* inquiry product id length	*/
#define DPT_REV_LEN	4		/* inquiry revision num length	*/

/* The Inquiry data sturcture  */
typedef struct dpt_ident{
	udi_ubit8_t  id_dev_pqual;	/* Bits 0-4 are device type and
					   bits 5-7 are Peripheral qualifier */
	udi_ubit8_t  id_dqual_rmb;	/* Bits 0-6 are device type qualifier
					   and bit 7 is removable media */
	udi_ubit8_t  id_ver;		/* Version					  */
	udi_ubit8_t  id_format;		/* Bits 0-3 are response data format
					   and bits 4-7 are reserved	*/
	udi_ubit8_t  id_len;		/* Additional data length   */
	udi_ubit8_t  id_vu[3];		/* Vendor unique - swapit	*/
	udi_ubit8_t  id_vendor[DPT_VID_LEN];	/* Vendor ID			*/
	udi_ubit8_t  id_prod[DPT_PID_LEN];	/* Product ID		   */
	udi_ubit8_t  id_revnum[DPT_REV_LEN];	/* Revision number	  */
} dpt_ident_t;

#define INQUIRY_SIZE	36 /* sizeof(dpt_ident_t); */

/* Peripheral device type */
#define DPT_ID_NODEV	0x1f		/* Logical unit not present */

/* Peripheral qualifier */
#define DPT_ID_QOK	0x00		/* Peripheral currently connected */
#define DPT_ID_QODEV	0x01		/* No physical device on this LU  */
#define DPT_ID_QNOLU	0x03		/* Target not capable of supporting 
					   a physical device on this LU */

#define DPT_DTYPE(x)		((x) & 0x1F)
#define DPT_PQUAL(x)		((x) >> 5)

#define DPT_SENSE_SIZE	18

#define DPT_SIMPLE_TAG	0
#define DPT_HEADQ_TAG	1
#define DPT_ORDERED_TAG	2

#define DPT_MAX_CHANNELS	3  /* Maximum number of scsi channels (buses) */

#ifdef OUT
#define MAX_DEVICES	16 /* shortcut: hardcode max dev's per adapter */
#define MAX_SENSE_DATA	24 /* shortcut hardcode sense length for common case */
#define DEVICE_NUM_REQUESTS 30 /* shortcut: hardcoded max-active per target */
#define DEVICE_NUM_CTRL	5 /* shortcut: hardcoded max-active per target */
#define DEVICE_TOTAL_RESOURCES (DEVICE_NUM_REQUESTS+DEVICE_NUM_CTRL)

#define MAX_RDATA_RESOURCES DEVICE_TOTAL_RESOURCES /* * MAX_DEVICES */
#endif /* OUT */

/* Number of jobs per device */
#define DPT_MIN_REQS	1
#define DPT_LOW_REQS	16
#define DPT_MAX_REQS	32

/* Number of intr event cbs */
#define DPT_MIN_INTR_EVENT_CB	2	/* Min. no. of intr event cbs before
					 * interrupts are disabled.
					 */
#define DPT_MAX_INTR_EVENT_CB	64	/* Max number of intr event cbs */

#define DPT_MAX_ENUM_ATTR		14
#define DPT_LOCATOR_ATTR_LEN		27
#define DPT_IDENTIFIER_ATTR_LEN		45

/* 
 * ================================================================
 * Typedefs to resolve forward refereences
 * ================================================================
 */
typedef struct dpt_req		dpt_req_t;
typedef struct dpt_device_info	dpt_device_info_t;
typedef struct dpt_region_data	dpt_region_data_t;

/* 
 * ================================================================
 * Sub-structures
 * ================================================================
 */

/* Values for d_state field */
#define DPT_NORMAL	0x00
#define DPT_SUSPEND	0x01
#define DPT_FLUSH	0x02

struct dpt_device_info {
	udi_queue_t	link;
	udi_ubit8_t	d_state;	/* device state */
	udi_size_t	active_reqs;	/* current active */
	udi_queue_t	active_q;	/* active dpt_req_t */
	udi_queue_t	tag_q;		/* queue of those over qdepth */
	udi_queue_t	suspend_q;	/* suspend queue */
	udi_ubit32_t	scsi_bus;
	udi_ubit32_t	tgt_id;
	udi_ubit32_t	lun_id;
	dpt_ident_t	inquiry_data;
	udi_ubit32_t	child_ID;
	dpt_region_data_t *rdata;	/* back pointer to region data */
	udi_dma_constraints_t constraints;
	udi_ubit16_t	num_resources;	/* Num of resources (CCBs, dma handles)
					 * allocated per target */
	udi_ubit16_t	events;
	udi_ubit16_t	bind_flags;
	udi_ubit16_t	queue_depth;
	udi_ubit16_t	max_sense_len;
	udi_ubit16_t	aen_buf_size;
};

#ifdef OUT
struct cp_bits {
	udi_ubit8_t SReset:1;
	udi_ubit8_t HBAInit:1;
	udi_ubit8_t ReqSen:1;
	udi_ubit8_t Scatter:1;
	udi_ubit8_t Resrvd:1;
	udi_ubit8_t Interpret:1;
	udi_ubit8_t DataOut:1;
	udi_ubit8_t DataIn:1;
};
#endif /* OUT */

typedef struct dpt_status {

	udi_ubit8_t	s_ha_status;
	udi_ubit8_t	s_scsi_status;
	udi_ubit8_t	s_buf_len[2];	/* length of incoming write buffer*/
	udi_ubit8_t	s_inv_resid[4];
	union {
		struct dpt_ccb	*s_vp;	/* Command Packet Vir Address.	*/
		udi_ubit8_t	s_va[4]; /* Command Packet Other Info.   */
	} s_cp;
	udi_ubit8_t	s_id_message;
	udi_ubit8_t	s_que_message;
	udi_ubit8_t	s_tag_message;
	udi_ubit8_t	s_messages[5];
	udi_ubit8_t	s_buf_off[4];	/* offset of incoming write buffer*/
} dpt_status_t;

typedef void (*dpt_ccb_callback_t)(dpt_region_data_t *, struct dpt_ccb *);

/*
 * Command Control Block
 */
/*COM: Explain this structure. Separate the device memory peice*/
/*COM: from the host peice.*/
typedef struct dpt_ccb {
	/*** EATA Packet sent to ctlr ***/
	udi_ubit8_t CPop;		/* 00: Operation Control byte.	*/
	udi_ubit8_t ReqLen;		/* 01: Request Sense Length.	*/
	udi_ubit8_t Unused[3];		/* 02: Reserved		*/
	udi_ubit8_t CPfwnest;		/* 05: */
	udi_ubit8_t CPinhibit;		/* 06: */
	udi_ubit8_t CPbusid;		/* 07: scsi_id:5, bus:3 	*/
	udi_ubit8_t CPmsg0;		/* 08: Identify/DiscoReco... Msg   */
	udi_ubit8_t CPmsg1;		/* 09:				*/
	udi_ubit8_t CPmsg2;		/* 0A:				*/
	udi_ubit8_t CPmsg3;		/* 0B:				*/
	udi_ubit8_t CPcdb[12];		/* 0C: Embedded SCSI CDB.	*/
	udi_ubit32_t CPdataLen;		/* 18: Transfer Length.	*/
	union {
		struct dpt_ccb *vp;	/* 1C: Command Packet Vir Address. */
		udi_ubit8_t	va[4];	/* 1C: Command Packet Other Info.  */
	} CPaddr;
	udi_ubit32_t	CPdataDMA;	/* 20: Data Physical Address.	*/
	udi_ubit32_t	CPstatDMA;	/* 24: Status Packet Phy Address.  */
	udi_ubit32_t	CPsenseDMA;	/* 28: ReqSense Data Phy Address.  */

	dpt_status_t	CP_status;	/* command status packet */
	udi_ubit8_t	CP_sense[DPT_SENSE_SIZE]; /* Sense data		*/
	dpt_ccb_callback_t c_callback; /* Callback Routine	*/
	udi_ubit32_t	c_time;		/* Timeout count (msecs)	*/
	void		*c_bind;	/* Associated SCSI Request Block */
	udi_scgth_t	*c_sg;		/* CCB sg list,contains CCB phys addr */
	udi_boolean_t	c_must_swap;	/* CCB must_swap info */
	udi_dma_handle_t c_dma_handle;	/* CCB dma handle */
	udi_dma_handle_t buf_dma_handle; /* buf dma handle */
	udi_queue_t	link;
} dpt_ccb_t;
	
/* 
 * ================================================================
 * Per-adapter structure,
 * aka region data
 * ================================================================
 */

typedef struct dpt_eata_info {
	/* parameters for handling dma_mem_alloc calls for EATA config*/
	dpt_eata_cfg_t		*eata_cfgp;
	udi_dma_handle_t	dma_handle;
	udi_scgth_t		*scgth;
	udi_boolean_t		must_swap;
} dpt_eata_info_t;

/* 
 * ================================================================
 * Per-request structure.
 * ================================================================
 */
struct dpt_req	{
	udi_queue_t	Q;
	udi_scsi_io_cb_t *scsi_io_cb;	/* Pointer to io CB */
	dpt_ccb_t	*ccb;		/* command control block */
	udi_status_t	req_status;	/* request status during exception */
	udi_ubit8_t	scsi_status;	/* scsi status during exception */
	udi_ubit32_t	timeout;
#ifdef NOTYET
	struct dpt_req	*abort_req;	 /* ptr to req to be aborted */
#endif
};

typedef struct dpt_scsi_addr {
	udi_ubit8_t		bus;
	udi_ubit8_t		target;
	udi_ubit8_t		lun;
} dpt_scsi_addr_t;

struct dpt_scan_info {
	dpt_scsi_addr_t		addr;
	dpt_ident_t		*inq_data;
	udi_dma_handle_t	inq_dma_handle;
	udi_scgth_t		*inq_sg;
	udi_boolean_t		inq_must_swap;
	dpt_ccb_t		*scan_ccb;
};

#define DPT_CMD_PIO_HANDLE	0
#define DPT_INTR_PIO_HANDLE	1
#define DPT_CONFIG_PIO_HANDLE	2
#define DPT_ABORT_PIO_HANDLE	3
#define DPT_RESET_PIO_HANDLE	4

#define DPT_MAX_PIO_HANDLE	5

/* init_stat values */
#define DPT_RDATA_INIT_DONE	0x01

struct dpt_region_data {
	udi_init_context_t	init_context;	/* env-provided goodies */
	udi_ubit8_t		init_state;	/* state of region data */
	udi_ubit8_t		resource_level;	/* Resource usage level */
	udi_queue_t		pending_jobs;	/* List of pending CCBs */
	udi_queue_t		alloc_q;	/* allocation dpt_req_t */
	udi_queue_t		timer_q;	/* Timer queue */
	udi_queue_t		device_list;	/* linked list of devices */
	udi_cb_t		*mgmt_cb;	/* mgmt CB place holder */
	udi_bus_bind_cb_t	*bus_bind_cb;	/* parent (bus) bind CB */
	udi_intr_attach_cb_t	*intr_attach_cb;
	udi_cb_t		*timer_cb;	/* timer cb */
	udi_timestamp_t		timestamp;
	udi_channel_t		interrupt_channel; /* for talking to bridge */
	udi_dma_constraints_t	ccb_constraints; /* CCB limitations */
	udi_dma_constraints_t	dma_constraints; /* DMA limitations */
	udi_dma_handle_t	dma_handle;
	udi_index_t		ha_channel_id[DPT_MAX_CHANNELS]; /* chan ids */
	udi_index_t		num_devices;	/* how many devices found? */
	udi_index_t	num_intr_event_cb;	/* number of intr event cbs */
	/* trace masks */
	udi_trevent_t	trace_mask;	/* meta-independent */
	udi_trevent_t	mgmt_trace_mask;
	udi_trevent_t	scsi_trace_mask;
	udi_trevent_t	bus_trace_mask;
	udi_queue_t	ccb_pool;	/* pool of pre-allocated ccb's */
	dpt_eata_info_t	eata_info;	/* Per adapter config info */
	struct dpt_scan_info scan_req; /* parameters for controlling scan */
	udi_ubit32_t	dpt_child_id;	/* child identifiers */
	udi_ubit8_t	pio_index;
	udi_pio_handle_t pio_handle[DPT_MAX_PIO_HANDLE];
	udi_buf_path_t	buf_path;
};

/*
 * Child Context
 */
typedef struct dpt_child_context {
	udi_child_chan_context_t	chan_context;
	dpt_device_info_t		*device_info;
} dpt_child_context_t;

/*
 * Time calculation macros used in this driver
 */

/* default timeout values */
#define DPT_DEFAULT_TIMEOUT_SECS	5	/* seconds */
#define DPT_DEFAULT_TIMEOUT_NSECS	0	/* nanoseconds */

/* check if t1 is less than t2 */
#define DPT_TIMER_LT(t1, t2)	((t1.seconds <= t2.seconds) && \
			 ((t1.seconds < t2.seconds) || \
			 (t1.nanoseconds < t2.nanoseconds)))

/* convert time_t into milliseconds */
#define DPT_TIMER_MSEC(t)	(t.seconds * 1000) + (t.nanoseconds / 1000000)

/*
 *---------------------------------------------------------------------
 * Tracing and logging 
 *	See udiprops.txt for Message Numbers 
 *---------------------------------------------------------------------
 */

/* Driver specific Trace Events */
#define DPT_TREVENT_ERROR	UDI_TREVENT_INTERNAL_1
#define DPT_TREVENT_WARN	UDI_TREVENT_INTERNAL_2
#define DPT_TREVENT_INFO	UDI_TREVENT_INTERNAL_3

/* Supported trace masks */
#define DPT_MGMT_TRACE_MASK	(UDI_TREVENT_LOCAL_PROC_ENTRY | \
					UDI_TREVENT_LOCAL_PROC_EXIT)
#define DPT_SCSI_TRACE_MASK	(UDI_TREVENT_LOCAL_PROC_ENTRY | \
					UDI_TREVENT_LOCAL_PROC_EXIT)
#define DPT_BUS_TRACE_MASK	(UDI_TREVENT_LOCAL_PROC_ENTRY | \
					UDI_TREVENT_LOCAL_PROC_EXIT)


