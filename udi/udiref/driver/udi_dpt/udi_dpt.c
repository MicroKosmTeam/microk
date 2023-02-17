/*
 * File: udi_dpt/udi_dpt.c
 *
 * Sample UDI Driver for DPT SmartCache and SmartRaid cards.
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
#define UDI_PHYSIO_VERSION 0x101
#define UDI_PCI_VERSION 0x101
#define UDI_SCSI_VERSION 0x101

#include <udi.h>	/* Standard UDI Definitions */
#include <udi_scsi.h>	/* SCSI metalanguage */
#include <udi_physio.h>	/* Physical I/O Extensions */
#include <udi_pci.h>	/* PCI Definitions */

#include "udi_dpt.h"

/*
 * CB indexes for each of the types of control blocks used in the driver.
 */
#define SCSI_BIND_CB_IDX	1
#define SCSI_IO_CB_IDX		2
#define SCSI_CTL_CB_IDX		3
#define SCSI_EVENT_CB_IDX	4
#define BUS_BIND_CB_IDX		5
#define INTR_ATTACH_CB_IDX	6
#define INTR_EVENT_CB_IDX	7
#define INTR_DETACH_CB_IDX	8

#define DPT_TIMER_GCB_IDX	9

/*
 * Ops indexes for each of the entry point ops vectors used.
 */
#define SCSI_OPS_IDX		1
#define BUS_DEVICE_OPS_IDX	2
#define INTR_HANDLER_OPS_IDX	3

/*
 * Driver entry points for the Management Metalanguage.
 */
STATIC udi_usage_ind_op_t dpt_usage_ind;
STATIC udi_enumerate_req_op_t dpt_enumerate_req;
STATIC udi_devmgmt_req_op_t dpt_devmgmt_req;
STATIC udi_final_cleanup_req_op_t dpt_final_cleanup_req;

STATIC udi_mgmt_ops_t dpt_mgmt_ops = {
	dpt_usage_ind,
	dpt_enumerate_req,
	dpt_devmgmt_req,
	dpt_final_cleanup_req
};

/*
 * Driver "top-side" entry points for the SCSI Metalanguage.
 */

STATIC udi_channel_event_ind_op_t dpt_scsi_channel_event_ind;
STATIC udi_scsi_bind_req_op_t dpt_scsi_bind_req;
STATIC udi_scsi_unbind_req_op_t dpt_scsi_unbind_req;
STATIC udi_scsi_io_req_op_t dpt_scsi_io_req;
STATIC udi_scsi_ctl_req_op_t dpt_scsi_ctl_req;
STATIC udi_scsi_event_res_op_t dpt_scsi_event_res;

STATIC udi_scsi_hd_ops_t dpt_scsi_hd_ops = {
	dpt_scsi_channel_event_ind,
	dpt_scsi_bind_req,
	dpt_scsi_unbind_req,
	dpt_scsi_io_req,
	dpt_scsi_ctl_req,
	dpt_scsi_event_res
};

/*
 * Driver "bottom-side" entry points for the Bus Bridge Metalanguage.
 */

STATIC udi_channel_event_ind_op_t dpt_bus_channel_event_ind;
STATIC udi_bus_bind_ack_op_t dpt_bus_bind_ack;
STATIC udi_bus_unbind_ack_op_t dpt_bus_unbind_ack;
STATIC udi_intr_attach_ack_op_t dpt_intr_attach_ack;
STATIC udi_intr_detach_ack_op_t dpt_intr_detach_ack;

STATIC udi_bus_device_ops_t dpt_bus_device_ops = {
	dpt_bus_channel_event_ind,
	dpt_bus_bind_ack,
	dpt_bus_unbind_ack,
	dpt_intr_attach_ack,
	dpt_intr_detach_ack
};

STATIC udi_channel_event_ind_op_t dpt_intr_channel_event_ind;
STATIC udi_intr_event_ind_op_t dpt_intr_event_ind;

STATIC udi_intr_handler_ops_t dpt_intr_handler_ops = {
	dpt_intr_channel_event_ind,
	dpt_intr_event_ind
};

/*
 * default op_flags without any bits set
 */
STATIC const udi_ubit8_t dpt_default_op_flags[] = {
	0, 0, 0, 0, 0, 0
};

/*
 * ================================================================
 * main driver initialization structure
 *
 * Driver-specific module initialization.
 *
 * Tasks for this operation:
 *	+ Initialize primary region properties
 *	+ Register the channel ops vectors for each metalanguage
 *	  used in this driver (module).
 *	+ Register control block properties
 */

/*
 * ----------------------------------------------------------------
 * Management operations init section:
 * ----------------------------------------------------------------
 */
STATIC udi_primary_init_t	dpt_primary_init = {
	&dpt_mgmt_ops,			/* mgmt meta ops */
	dpt_default_op_flags,	/* op_flags */
	DPT_MGMT_SCRATCH,		/* mgmt_scratch_size */
	DPT_MAX_ENUM_ATTR,		/* enumeration attr list length */
	sizeof(dpt_region_data_t),	/* region data size */
	sizeof(dpt_device_info_t),	/* Child data size */
	1,		/* buf path */
};

/*
 * ---------------------------------------------------------------
 * Meta operations init section:
 * ---------------------------------------------------------------
 */
STATIC udi_ops_init_t	dpt_ops_init_list[] = {
	{
		SCSI_OPS_IDX,
		DPT_SCSI_META_IDX,	/* meta index [from udiprops.txt] */
		UDI_SCSI_HD_OPS_NUM,	/* scsi meta ops */
		sizeof(dpt_child_context_t),	/* Child context size */
		(udi_ops_vector_t *)&dpt_scsi_hd_ops,
		dpt_default_op_flags	/* op_flags */
	},
	{
		BUS_DEVICE_OPS_IDX,
		DPT_BRIDGE_META_IDX,	/* bridge meta index */
		UDI_BUS_DEVICE_OPS_NUM,	/* bridge meta ops */
		0,			/* Child context size */
		(udi_ops_vector_t *)&dpt_bus_device_ops,
		dpt_default_op_flags	/* op_flags */
	},
	{
		INTR_HANDLER_OPS_IDX,
		DPT_BRIDGE_META_IDX,	/* bridge meta index */
		UDI_BUS_INTR_HANDLER_OPS_NUM,
		0,			/* Child context size */
		(udi_ops_vector_t *)&dpt_intr_handler_ops,
		dpt_default_op_flags	/* op_flags */
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

STATIC udi_cb_init_t	dpt_cb_init_list[] = {
	{
		SCSI_BIND_CB_IDX,	/* SCSI Bind CB		*/
		DPT_SCSI_META_IDX,	/* from udiprops.txt 	*/
		UDI_SCSI_BIND_CB_NUM,	/* meta cb_num		*/
		SCSI_BIND_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
	{
		SCSI_IO_CB_IDX,		/* SCSI i/o CB		*/
		DPT_SCSI_META_IDX,	/* from udiprops.txt 	*/
		UDI_SCSI_IO_CB_NUM,	/* meta cb_num		*/
		SCSI_IO_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
	{
		SCSI_CTL_CB_IDX,	/* SCSI ctl CB		*/
		DPT_SCSI_META_IDX,	/* from udiprops.txt 	*/
		UDI_SCSI_CTL_CB_NUM,	/* meta cb_num		*/
		SCSI_CTL_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
	{
		SCSI_EVENT_CB_IDX,	/* SCSI event CB		*/
		DPT_SCSI_META_IDX,	/* from udiprops.txt 	*/
		UDI_SCSI_EVENT_CB_NUM,	/* meta cb_num		*/
		SCSI_EVENT_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
	{
		BUS_BIND_CB_IDX,	/* Bridge Bind CB	*/
		DPT_BRIDGE_META_IDX,	/* from udiprops.txt	*/
		UDI_BUS_BIND_CB_NUM,	/* meta cb_num		*/
		BUS_BIND_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
	{
		INTR_ATTACH_CB_IDX,	/* Interrupt Attach CB	*/
		DPT_BRIDGE_META_IDX,	/* from udiprops.txt	*/
		UDI_BUS_INTR_ATTACH_CB_NUM,	/* meta cb_num	*/
		INTR_ATTACH_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
	{
		INTR_EVENT_CB_IDX,	/* Interrupt Event CB	*/
		DPT_BRIDGE_META_IDX,	/* from udiprops.txt	*/
		UDI_BUS_INTR_EVENT_CB_NUM,	/* meta cb_num		*/
		INTR_EVENT_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
	{
		INTR_DETACH_CB_IDX,	/* Interrupt Attach CB	*/
		DPT_BRIDGE_META_IDX,	/* from udiprops.txt	*/
		UDI_BUS_INTR_DETACH_CB_NUM,	/* meta cb_num	*/
		INTR_DETACH_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
	{
		0			/* Terminator */
	}
};

STATIC udi_gcb_init_t	dpt_gcb_init_list[] = {
	
	{
		DPT_TIMER_GCB_IDX,	/* timer CB */
		DPT_TIMER_SCRATCH	/* scratch requirement */
	},
	{
		0			/* Terminator */
	}
};

udi_init_t udi_init_info = {
	&dpt_primary_init,
	NULL, /* Secondary init list */
	dpt_ops_init_list, 
	dpt_cb_init_list,
	dpt_gcb_init_list, 
	NULL, /* cb select list */
};

/*
 * PIO trans lists for sending commands to the controller
 */
STATIC udi_pio_trans_t dpt_cmd_pio_trans[] = {
	/***** Spin for unit to go non-busy *****/

	/* Label 1 */
	{ UDI_PIO_LABEL, 0, 1 },

		/* R0 <- device[DPT_HA_AUX_STATUS] */
		{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_1BYTE, 
			DPT_HA_AUX_STATUS },

		/* R0 &= DPT_HA_AUX_BUSY */
		{ UDI_PIO_AND_IMM + UDI_PIO_R0, UDI_PIO_1BYTE,
			DPT_HA_AUX_BUSY },

		/* If R0 != 0, back to top of loop */
		{ UDI_PIO_CSKIP+UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_Z },

	/* goto Label 1 */
 	{ UDI_PIO_BRANCH, 0, 1 },

/* //TODO: fix the never ending loop. Use a counter. */

	/* R0 <- DPT_PIO_SCRATCH_PADDR {offset into scratch of phys address} */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, DPT_PIO_SCRATCH_PADDR },
	{ UDI_PIO_LOAD+UDI_PIO_SCRATCH+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1 },

	/* The half-word stores and shifting are here are so we don't have
	 * to map the reg set unaligned in order to write this little endian
	 * 32-bit word to offset "2" on the card.  (It's a hardware 
	 * restriction that comes from the device's ISA heritage.)
	 * This code is endian-safe.
	 */
	/* device[DPT_HA_DMA_BASE] = R1 & 0xffff*/
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_2BYTE, 
		DPT_HA_DMA_BASE },
	/* R1 >>= 16 */
	{ UDI_PIO_SHIFT_RIGHT + UDI_PIO_R1, UDI_PIO_4BYTE, 16 },
	/* device[DPT_HA_DMA_BASE+2] = R1 */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_2BYTE, 
		DPT_HA_DMA_BASE + 2},

	/* R0 <- DPT_PIO_SCRATCH_CMD {offset into scratch of cmd} */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, DPT_PIO_SCRATCH_CMD },

	/* R1 <- cmd (which we just read out of scratch) */
	{ UDI_PIO_LOAD+UDI_PIO_SCRATCH+UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R1 },

	/* device[DPT_HA_COMMAND] = R1 */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_1BYTE,
		DPT_HA_COMMAND },

	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0}
};

/*
 * This is the interrupt preprocessor.   It simply determines if an 
 * interrupt was ours or not.  If so, it "claims" it.
 */
STATIC udi_pio_trans_t dpt_intr_pio_trans[] = {

	/* PIO 0   Enable device interrupts, service as normal (i.e. fallthru)
	   any that are now pending. Exists implicitly */

	/* R0 <- 00  Immediate Function Subcode (Enable HBA Interrupts) */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 0x00 },
	/* device[0x05] = R0 */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_1BYTE, 0x06},
	/* R0 <- 04  Immediate Function Code (Set HBA Interrupt Mode) */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 0x04 },
	/* device[0x06] = R0 */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_1BYTE, 0x05},
	/* R0 = DPT_CP_IMMEDIATE */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, DPT_CP_IMMEDIATE },
	/* device[DPT_HA_COMMAND] = R0 */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_1BYTE,
		DPT_HA_COMMAND },
	/* R3 = 0, indicating normal return */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R3, UDI_PIO_2BYTE, 0x00 },
	{ UDI_PIO_BRANCH, 0, 10 },

	/* PIO 1   Normal interrupt service. */
	{ UDI_PIO_LABEL, 0, 1 },
	/* R3 = 0, indicating normal return */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R3, UDI_PIO_2BYTE, 0x00 },

	{ UDI_PIO_LABEL, 0, 10 },
	/* R0 = device[DPT_HA_AUX_STATUS] */
	{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_1BYTE,
		DPT_HA_AUX_STATUS },

	/* R0 &= DPT_HA_AUX_INTR */
	{ UDI_PIO_AND_IMM + UDI_PIO_R0, UDI_PIO_1BYTE, DPT_HA_AUX_INTR },

	/* If R0 == 0, skip over the instructs to return unclaimed */
	{ UDI_PIO_CSKIP+UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_Z },
	{ UDI_PIO_BRANCH, 0, 20 },
		/* R1 = return value from trans list. */
		{ UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 
		  UDI_INTR_UNCLAIMED },
		/* R2 = our offset into scratch, a constant of zero. */
		{ UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, 0},
		/* Store R1 into offset R2 (constant 0) into scratch space. */
		{ UDI_PIO_STORE + UDI_PIO_SCRATCH + UDI_PIO_R2, 
			UDI_PIO_1BYTE, UDI_PIO_R1},
 		{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 },
 
	{ UDI_PIO_LABEL, 0, 20 },
 	/* Clear the interrupt */
 	/* R0 = device[DPT_HA_STATUS] */
 	{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_1BYTE, DPT_HA_STATUS },
	/* if R3 != 0 (i.e 2 or 3), skip over to take the appropriate action */
	{ UDI_PIO_CSKIP+UDI_PIO_R3, UDI_PIO_1BYTE, UDI_PIO_NZ },
	/* R0 contains the status byte */
	{ UDI_PIO_END, UDI_PIO_2BYTE, UDI_PIO_R0 },

	/* We are running at PIO 2 or 3, disable interrupts */

	/* R1 <- 01  Immediate Function Subcode (Disable HBA Interrupts) */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_2BYTE, 0x01 },
	/* device[0x05] = R1 */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_1BYTE, 0x06},
	/* R1 <- 04  Immediate Function Code (Set HBA Interrupt Mode) */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_2BYTE, 0x04 },
	/* device[0x06] = R1 */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_1BYTE, 0x05},
	/* R1 = DPT_CP_IMMEDIATE */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_2BYTE, DPT_CP_IMMEDIATE },
	/* device[DPT_HA_COMMAND] = R1 */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_1BYTE,
		DPT_HA_COMMAND },
	/* R0 contains the status byte */
	{ UDI_PIO_END, UDI_PIO_2BYTE, UDI_PIO_R0 },

	/* PIO 2   Service any pending interrupts and disable device interrupts.
	 */
	{ UDI_PIO_LABEL, 0, 2 },
	/* R3 = 2, indicating that we are running at PIO 2 */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R3, UDI_PIO_2BYTE, 0x02 },
	{ UDI_PIO_BRANCH, 0, 10 },

	/* PIO 3   Clear device (overrun) interrupt.   Toss any data. */
	{ UDI_PIO_LABEL, 0, 3 },
	/* R3 = 3, indicating that we are running at PIO 3 */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R3, UDI_PIO_2BYTE, 0x03 },
	{ UDI_PIO_BRANCH, 0, 10 }
};

/*
 * PIO trans lists for access to the DPT EATA configuration
 */
STATIC udi_pio_trans_t dpt_config_pio_trans[] = {

	/***** Spin for unit to go non-busy *****/

	{ UDI_PIO_LABEL, 0, 1 },
		/* R0 <- device[DPT_HA_STATUS] */
		{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_1BYTE, 
			DPT_HA_STATUS },

		/* R0 &= DPT_HA_ST_BUSY */
		{ UDI_PIO_AND_IMM + UDI_PIO_R0, UDI_PIO_1BYTE, DPT_HA_ST_BUSY },

		/* If R0 != 0, back to top of loop */
		{ UDI_PIO_CSKIP+UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_Z },

	/* go back to label 1 */
	{ UDI_PIO_BRANCH, 0, 1 },

	/***** write "Read Config PIO" command *****/
	/* R0 <- DPT_CP_READ_CFG_PIO */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, DPT_CP_READ_CFG_PIO },

	/* device[DPT_HA_COMMAND] register  = R0  */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_1BYTE,
		DPT_HA_COMMAND },

	/* R1 == the value in the status register when the h/w has digested
	 * this command. */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_2BYTE, 
		DPT_HA_ST_DRQ + DPT_HA_ST_SEEK_COMP + DPT_HA_ST_READY },

	/**** Spin for DRQ interrupt ****/
	{ UDI_PIO_LABEL, 0, 2 },

		/* R0 <- inb(dpt_base + DPT_HA_STATUS) */
		{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_1BYTE, 
			DPT_HA_STATUS },

		/* R0 -= the pattern we're lookign for.   (i.e. "zero"  */
		/* when equal) */
		{ UDI_PIO_SUB + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R1 }, 

		/* If R0 == R1, skip next instruction */
		{ UDI_PIO_CSKIP+UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_Z },

	/* goto label 2 */
	{ UDI_PIO_BRANCH, 0, 2 },

	/* R0 <- our offset into the 'mem' arg to udi_pio_trans().  */
        { UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 0 },
	/* R1 <- PIO base address */
        { UDI_PIO_LOAD_IMM+UDI_PIO_R1, UDI_PIO_2BYTE, DPT_HA_DATA },
	/* R2 <- count.  We know EATA is 256 16-bit values.  */
        { UDI_PIO_LOAD_IMM+UDI_PIO_R2, UDI_PIO_2BYTE, 512/2  },

	/* Now, bring the data in as a bulk transfer. */
        { UDI_PIO_REP_IN_IND, UDI_PIO_2BYTE,               
          UDI_PIO_REP_ARGS(UDI_PIO_MEM,
                UDI_PIO_R0 /* mem reg */, 1  /* mem stride */,
                UDI_PIO_R1 /* pio reg */, 0  /* pio stride */,
                UDI_PIO_R2)} ,

	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 }
	
};

STATIC udi_pio_trans_t dpt_abort_pio_trans[] = {

	/***** Spin for unit to go non-busy *****/

	/* Label 1 */
	{ UDI_PIO_LABEL, 0, 1 },

		/* R0 <- device[DPT_HA_AUX_STATUS] */
		{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_1BYTE, 
			DPT_HA_AUX_STATUS },

		/* R0 &= DPT_HA_AUX_BUSY */
		{ UDI_PIO_AND_IMM + UDI_PIO_R0, UDI_PIO_1BYTE,
			DPT_HA_AUX_BUSY },

		/* If R0 != 0, back to top of loop */
		{ UDI_PIO_CSKIP+UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_Z },

	/* goto Label 1 */
 	{ UDI_PIO_BRANCH, 0, 1 },

/* //TODO: fix the never ending loop. Use a counter. */

	/* R0 <- DPT_PIO_SCRATCH_PADDR {offset into scratch of phys address} */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, DPT_PIO_SCRATCH_PADDR },
	{ UDI_PIO_LOAD+UDI_PIO_SCRATCH+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1 },

	/* device[DPT_HA_DMA_BASE] = R1 & 0xffff*/
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_2BYTE, 
		DPT_HA_DMA_BASE },
	/* R1 >>= 16 */
	{ UDI_PIO_SHIFT_RIGHT + UDI_PIO_R1, UDI_PIO_4BYTE, 16 },
	/* device[DPT_HA_DMA_BASE + 2] = R1 */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1, UDI_PIO_2BYTE, 
		DPT_HA_DMA_BASE + 2},

	/* R0 = DPT_CP_ABORT_CMD */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, DPT_CP_ABORT_CMD },
	/* device[6] = R0 */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_1BYTE, 6 },

	/* R0 = DPT_CP_IMMEDIATE */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, DPT_CP_ABORT_CMD },
	/* device[DPT_HA_COMMAND] = R0 */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_1BYTE,
		DPT_HA_COMMAND },

	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0}
};

STATIC udi_pio_trans_t dpt_reset_pio_trans[] = {
	/* R0 = device[DPT_HA_STATUS] */
	{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_1BYTE, DPT_HA_STATUS },

	/* R0 &= DPT_HA_ST_SEEK_COMP | DPT_HA_ST_READY */
	{ UDI_PIO_AND_IMM+UDI_PIO_R0, UDI_PIO_1BYTE,
		DPT_HA_ST_SEEK_COMP|DPT_HA_ST_READY },

	/* If R0 == 0, we are done */
		{ UDI_PIO_CSKIP+UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_NZ },
	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 },

	/* R0 = DPT_CP_EATA_RESET */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, DPT_CP_EATA_RESET },
	/* device[DPT_HA_COMMAND] = R0 */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_1BYTE,
		DPT_HA_COMMAND },

	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0}
};

typedef struct dpt_pio_table {
	udi_pio_trans_t	*trans_list;
	udi_ubit16_t	trans_length;
	udi_ubit32_t	mapping_flags;
} dpt_pio_table_t;

/*
 * PIO handles
 * Notes:
 *	Make sure that the following macro definitions match the index
 *	in the dpt_pio table.
 *	If more PIO handles are needed, increase the size of pio_handle
 *	array stored in region data by increasing DPT_MAX_PIO_HANDLE
 */

STATIC const dpt_pio_table_t	dpt_pio_table[] = {
	{ dpt_cmd_pio_trans,
		sizeof dpt_cmd_pio_trans / sizeof(udi_pio_trans_t),
		UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER },
	{ dpt_intr_pio_trans,
		sizeof dpt_intr_pio_trans / sizeof(udi_pio_trans_t),
		UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER },
	{ dpt_config_pio_trans,
		sizeof dpt_config_pio_trans / sizeof(udi_pio_trans_t),
		UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER },
	{ dpt_abort_pio_trans,
		sizeof dpt_abort_pio_trans / sizeof(udi_pio_trans_t),
		UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER },
	{ dpt_reset_pio_trans,
		sizeof dpt_reset_pio_trans / sizeof(udi_pio_trans_t),
		UDI_PIO_LITTLE_ENDIAN|UDI_PIO_STRICTORDER }
};
	
STATIC const udi_ubit16_t dpt_pio_table_len = 
		sizeof dpt_pio_table / sizeof(dpt_pio_table_t);

STATIC const udi_dma_constraints_attr_spec_t dpt_dma_attr_list[] = {
	{ UDI_DMA_ADDRESSABLE_BITS, 32 },
	{ UDI_DMA_ALIGNMENT_BITS, 4 }, /* TODO - check this */
	{ UDI_DMA_SCGTH_MAX_ELEMENTS, 32 }, /* TODO - check this */
	{ UDI_DMA_SCGTH_ENDIANNESS, UDI_DMA_BIG_ENDIAN },
	{ UDI_DMA_SCGTH_MAX_SEGMENTS, 1 },
	{ UDI_DMA_ELEMENT_LENGTH_BITS, 16 } /* TODO - check this */
};

STATIC const udi_ubit16_t dpt_dma_attr_length = 
		sizeof(dpt_dma_attr_list) /
		sizeof(udi_dma_constraints_attr_spec_t);

STATIC const udi_dma_constraints_attr_spec_t dpt_ccb_attr_list[] = {
	{ UDI_DMA_ADDRESSABLE_BITS, 32 },
	{ UDI_DMA_ALIGNMENT_BITS, 4 },
	{ UDI_DMA_SCGTH_MAX_ELEMENTS, 1 },
	{ UDI_DMA_SCGTH_FORMAT, UDI_SCGTH_32|UDI_SCGTH_DRIVER_MAPPED },
	{ UDI_DMA_SCGTH_ENDIANNESS, UDI_DMA_BIG_ENDIAN },
	{ UDI_DMA_SCGTH_MAX_SEGMENTS, 1 },
	{ UDI_DMA_ELEMENT_LENGTH_BITS, 16 } /* TODO - check this */
};

STATIC const udi_ubit16_t dpt_ccb_attr_length = 
		sizeof dpt_ccb_attr_list /
		sizeof(udi_dma_constraints_attr_spec_t);

/* 
 * ================================================================
 * Internal function prototypes (those not passed in UDI vectors)
 * Because of the UDI callback mechanism, most of these are 
 * returns from UDI resource requests.
 * ================================================================
 */

/* dpt_usage_ind */
STATIC void dpt_usage_ind_failure(udi_cb_t *gcb, udi_status_t correlated_status);

/* dpt_bus_bind_ack */
STATIC void dpt_bus_bind_ccb_constraints_set( udi_cb_t *gcb,
		udi_dma_constraints_t constraints, udi_status_t status);

STATIC void dpt_bus_bind_dma_constraints_set( udi_cb_t *gcb,
		udi_dma_constraints_t new_constraints, udi_status_t status);

STATIC void dpt_bus_bind_intr_attach_cb_alloc( udi_cb_t *gcb,
		udi_channel_t interrupt_channel);

/* dpt_intr_attach_ack */
STATIC void dpt_intr_event_rdy(udi_cb_t *gcb, udi_cb_t *new_cb);
STATIC void dpt_bus_bind_get_reg_handle (udi_cb_t *gcb, void *eata_cfg);
STATIC void dpt_bus_bind_get_pio_handle(udi_cb_t *gcb,
		udi_pio_handle_t pio_handle);

STATIC void dpt_bus_bind_intr_attach(udi_cb_t *gcb, udi_cb_t *new_cb);
STATIC void dpt_bus_bind_init_adapter(udi_cb_t *gcb,
		udi_buf_t * buf, udi_status_t status, udi_ubit16_t result);

STATIC void dpt_bus_bind_setup_inquiry_buffer(udi_cb_t *gcb,
		udi_dma_handle_t dma_handle,
		void *inquiry_buf,
		udi_size_t actual_gap,
		udi_boolean_t single_element,
		udi_scgth_t *inquiry_sg,
		udi_boolean_t must_swap);
STATIC void dpt_bus_bind_alloc_scan_ccb(udi_cb_t *gcb,
		udi_dma_handle_t dma_handle,
		void *ccb,
		udi_size_t actual_gap,
		udi_boolean_t single_element,
		udi_scgth_t *ccb_sg,
		udi_boolean_t must_swap);
STATIC void dpt_bus_channel_event_ind_failure(udi_cb_t *gcb,
		udi_status_t correlated_status);

STATIC void dpt_enumerate_multilun_pd(dpt_region_data_t *rdata);
STATIC void dpt_enumerate_scan(dpt_region_data_t *rdata, dpt_scsi_addr_t *addr);
STATIC void dpt_enumerate_scan_next(dpt_region_data_t *rdata);
STATIC void dpt_enumerate_scan_done(dpt_region_data_t *rdata, dpt_ccb_t *cp);

STATIC dpt_device_info_t *dpt_find_device(dpt_region_data_t *rdata, 
		udi_ubit32_t child_id);
STATIC void dpt_scsi_bind_alloc_ccb(udi_cb_t *gcb,
		udi_dma_handle_t dma_handle,
		void *ccb,
		udi_size_t actual_gap,
		udi_boolean_t single_element,
		udi_scgth_t *ccb_sg,
		udi_boolean_t must_swap);
STATIC void dpt_scsi_bind_alloc_dma_handle(udi_cb_t *gcb,
		udi_dma_handle_t dma_handle);

STATIC udi_dma_buf_map_call_t dpt_ioreq_sg_acquired;

STATIC void dpt_sense_buf_callback(udi_cb_t *gcb, udi_buf_t *sense_buf);

STATIC void dpt_prep_ccb(dpt_ccb_t *ccb);
STATIC dpt_ccb_t * dpt_get_ccb(dpt_region_data_t *rdata);
STATIC void dpt_free_ccb(dpt_region_data_t *rdata, dpt_ccb_t *ccb);
STATIC void dpt_send_dma_cmd(dpt_region_data_t *rdata, dpt_ccb_t *cp);
STATIC void dpt_abort_req(dpt_req_t *req);
STATIC void dpt_send_dma_cmd_done(udi_cb_t *gcb, udi_buf_t *buf,
		udi_status_t status, udi_ubit16_t result);
STATIC void dpt_scsi_comp(udi_scsi_io_cb_t *scsi_io_cb,
		udi_status_t req_status, udi_ubit8_t scsi_status,
		udi_ubit8_t sense_status, udi_buf_t *sense_buf);

STATIC void dpt_scsi_io_req_done(dpt_region_data_t *rdata, dpt_ccb_t *cp);
STATIC void dpt_scsi_io_req_failure(udi_cb_t *gcb,
		udi_status_t correlated_status);

STATIC void dpt_devmgmt_intr_detach(udi_cb_t *gcb, udi_cb_t *new_cb);

STATIC void dpt_suspend(dpt_region_data_t *rdata);
STATIC void dpt_resume(dpt_region_data_t *rdata);
STATIC void dpt_flush(dpt_device_info_t *device_info);
STATIC void dpt_flush_done(dpt_region_data_t *rdata, dpt_ccb_t *cp);

STATIC void dpt_timer_insert(dpt_req_t *req);
STATIC void dpt_timer_remove(dpt_req_t *req);
STATIC udi_timer_expired_call_t	dpt_timer_callback;

STATIC void dpt_print_rdata_reqs(dpt_region_data_t *rdata);
STATIC void dpt_print_device_reqs(dpt_device_info_t *device_info);
STATIC void dpt_print_req(dpt_req_t *req);
STATIC void dpt_print_active_ccbs(dpt_region_data_t *rdata);
STATIC void dpt_print_ccb(dpt_ccb_t *ccb);

/*
 * dpt_usage_ind
 *
 * This is the driver's first look at the region data, or rdata,
 * data structure.  The driver should update the meta specific
 * TRACE mask, change resource allocation, and call udi_usage_res().
 *
 * This is the first operation issued over the management
 * channel our newly created region.
 * 
 * TODO:
 *	 Add support for re-allocating/deallocting resources
 * NOTE:
 *	 For message numbers, see "udiprops.txt"
 * MESSAGES: 1000 - 1099
 */

void
dpt_usage_ind(udi_usage_cb_t *usage_cb, udi_ubit8_t resource_level)
{
	dpt_region_data_t *rdata = UDI_GCB(usage_cb)->context;

	DPT_ASSERT(usage_cb);
	DPT_ASSERT(rdata);


	DPT_TRACE((&rdata->init_context, UDI_TREVENT_LOCAL_PROC_ENTRY,
		DPT_MGMT_META_IDX, 1010, /* message number, see udiprops.txt */
		usage_cb, resource_level));

	DPT_TRACE((&rdata->init_context, DPT_TREVENT_INFO,
		DPT_MGMT_META_IDX, 1020, /* message number, see udiprops.txt */
		usage_cb->meta_idx, usage_cb->trace_mask));

	/*
	 * Add the trace event types indicated in the trace mask of the
	 * control block that are supported, then update the control block's
	 * trace mask before responding.
	 */
	switch (usage_cb->meta_idx) {
		case DPT_MGMT_META_IDX:
			usage_cb->trace_mask &= DPT_MGMT_TRACE_MASK;
			rdata->mgmt_trace_mask = usage_cb->trace_mask;
			break;

		case DPT_SCSI_META_IDX:
			usage_cb->trace_mask &= DPT_SCSI_TRACE_MASK;
			rdata->scsi_trace_mask = usage_cb->trace_mask;
			break;

		case DPT_BRIDGE_META_IDX:
			usage_cb->trace_mask &= DPT_BUS_TRACE_MASK;
			rdata->bus_trace_mask = usage_cb->trace_mask;
			break;

		default:
			udi_log_write(dpt_usage_ind_failure, UDI_GCB(usage_cb),
				UDI_TREVENT_EXTERNAL_ERROR, UDI_LOG_ERROR,
				DPT_MGMT_META_IDX, 0,
				1030, /* message number, see udiprops.txt */
				usage_cb->meta_idx);
			break;
	}

	rdata->resource_level = resource_level;

	/*
	 * Not prepared to reallocate/deallocate resources yet!
	 * Currently the driver considers only intial resource level
	 * usage specification. All subsequent resource level changes
	 * are ignored.
	 */
	switch (resource_level) {
		case UDI_RESOURCES_CRITICAL:
		case UDI_RESOURCES_LOW:
		case UDI_RESOURCES_NORMAL:
		case UDI_RESOURCES_PLENTIFUL:
			/* TODO -
			 * Adjust the resource usage level
			 */
			break;
		default:
			udi_log_write(dpt_usage_ind_failure,
				UDI_GCB(usage_cb),
				UDI_TREVENT_EXTERNAL_ERROR,
				UDI_LOG_ERROR,
				DPT_MGMT_META_IDX, 0,
				1040, /* message number */
				resource_level);
			break;
	}

	if (!(rdata->init_state & DPT_RDATA_INIT_DONE)) {
		/* Initialize the region data structure */
		UDI_QUEUE_INIT(&rdata->pending_jobs);
		UDI_QUEUE_INIT(&rdata->device_list);
		UDI_QUEUE_INIT(&rdata->alloc_q);
		UDI_QUEUE_INIT(&rdata->ccb_pool);
		UDI_QUEUE_INIT(&rdata->timer_q);
		/* Indicate the state change */
		rdata->init_state |= DPT_RDATA_INIT_DONE;
	}

	/* Acknowledge the resource and trace mask change. */
	udi_usage_res(usage_cb);

	DPT_TRACE((&rdata->init_context, UDI_TREVENT_LOCAL_PROC_EXIT,
		DPT_MGMT_META_IDX, 1090 /* see udiprops.txt */));

	return;
}

/*
 * dpt_usage_ind_failure
 * 
 *	Callback from udi_log_write() for usage indicator failures.
 * MESSAGES: 1100 - 1199
 */
STATIC void
dpt_usage_ind_failure(udi_cb_t *gcb,
	udi_status_t correlated_status)
{
	DPT_DEBUG_VAR(dpt_region_data_t *rdata = (dpt_region_data_t *) gcb->context);

	DPT_ASSERT(gcb);
	DPT_ASSERT(rdata);

	DPT_TRACE((&rdata->init_context, UDI_TREVENT_LOCAL_PROC_ENTRY,
		DPT_MGMT_META_IDX, 1110, /* See udiprops.txt for message */
		gcb, correlated_status));

	udi_usage_res(UDI_MCB(gcb, udi_usage_cb_t));

	DPT_TRACE((&rdata->init_context, UDI_TREVENT_LOCAL_PROC_EXIT,
		DPT_MGMT_META_IDX, 1110)); /* See udiprops.txt for message */
	return;
}

/* 
 * ================================================================
 * dpt_bus_channel_event_ind -
 * Channel event indication operation on bus-driver channel
 *
 * Tasks :
 *	+ Save info the channel_event cb for later use.
 *	+ Initiate the bus-driver bind sequence
 * Notes:
 *
 * MESSAGES: 1200 - 1299
 * ================================================================
 */

STATIC void
dpt_bus_channel_event_ind(udi_channel_event_cb_t *channel_event_cb)
{
	dpt_region_data_t *rdata = UDI_GCB(channel_event_cb)->context;

	switch (channel_event_cb->event) {
	
	case UDI_CHANNEL_CLOSED:
		break;

	case UDI_CHANNEL_BOUND: {
		udi_bus_bind_cb_t *bus_bind_cb = UDI_MCB(
			channel_event_cb->params.parent_bound.bind_cb, 
			udi_bus_bind_cb_t);

		rdata->mgmt_cb = UDI_GCB(channel_event_cb);
		rdata->buf_path = channel_event_cb->
			params.parent_bound.path_handles[0];

		udi_bus_bind_req (bus_bind_cb);
		return;
	}

	case UDI_CHANNEL_OP_ABORTED:
		break;
	}

	udi_channel_event_complete(channel_event_cb, UDI_OK);
}

/*
 * ================================================================
 * dpt_bus_bind_ack -
 * MESSAGES: 1400 - 1499
 * ================================================================
 */
STATIC void
dpt_bus_bind_ack(
	udi_bus_bind_cb_t *bus_bind_cb,
	udi_dma_constraints_t constraints,
	udi_ubit8_t preferred_endianness,
	udi_status_t status)
{
	dpt_region_data_t *rdata = UDI_GCB(bus_bind_cb)->context;

	DPT_ASSERT(bus_bind_cb);
	DPT_ASSERT(rdata);

	DPT_TRACE((&rdata->init_context, UDI_TREVENT_LOCAL_PROC_ENTRY,
			DPT_BRIDGE_META_IDX, 1410 /* see udiprops.txt */,
			bus_bind_cb, preferred_endianness, status));

	rdata->bus_bind_cb = bus_bind_cb;

	if (status != UDI_OK) {
		/* Log event and post failure to parent bind request */
		udi_log_write(dpt_bus_channel_event_ind_failure,
			rdata->mgmt_cb, UDI_TREVENT_EXTERNAL_ERROR,
			UDI_LOG_ERROR, DPT_BRIDGE_META_IDX, status,
			1420 /* see udiprops.txt */, status);
		return;
	}

	/* 
	 * use the preallocated constraints as my dma constraints
	 * then make a copy to use as ccb constraints, after merging
	 * in my constraints attributes
	 */
	rdata->dma_constraints = constraints;
	udi_dma_constraints_attr_set(dpt_bus_bind_ccb_constraints_set,
                UDI_GCB(bus_bind_cb), constraints,
		dpt_ccb_attr_list, dpt_ccb_attr_length,
		UDI_DMA_CONSTRAINTS_COPY);
}

/*
 * MESSAGES: 1800 - 1899
 */
STATIC void
dpt_bus_bind_ccb_constraints_set(udi_cb_t *gcb,
	udi_dma_constraints_t constraints,
	udi_status_t status)
{
	dpt_region_data_t *rdata = gcb->context;

	/* TODO - check the status */

	rdata->ccb_constraints = constraints;

	/* Set the DMA constraint attributes */
	udi_dma_constraints_attr_set(dpt_bus_bind_dma_constraints_set,
                gcb, rdata->dma_constraints,
		dpt_dma_attr_list, dpt_dma_attr_length, 0);
}

/*
 * MESSAGES: 1900 - 1999
 */
STATIC void
dpt_bus_bind_dma_constraints_set( udi_cb_t *gcb,
	udi_dma_constraints_t constraints, udi_status_t status)
{
	dpt_region_data_t *rdata = gcb->context;

	/* TODO - check the status */

	rdata->dma_constraints = constraints;

	/* Allocate space for EATA configuration data */
	udi_mem_alloc(dpt_bus_bind_get_reg_handle, gcb,
		sizeof(dpt_eata_cfg_t), UDI_MEM_MOVABLE);
}

/*
 * ================================================================
 * dpt_bus_bind_get_reg_handle
 * We've just acquired the handle to PCI space.
 * Map the equivalent for register space 
 * MESSAGES: 2000 - 2099
 * ================================================================
 */
STATIC void
dpt_bus_bind_get_reg_handle (udi_cb_t *gcb, void *eata_cfg)
{
	dpt_region_data_t *rdata = gcb->context;

	rdata->eata_info.eata_cfgp = (dpt_eata_cfg_t *)eata_cfg;

	rdata->pio_index = 0;
	udi_pio_map(dpt_bus_bind_get_pio_handle, gcb,
			UDI_PCI_BAR_0, DPT_PCI_OFFSET, DPT_LENGTH,
			dpt_pio_table[rdata->pio_index].trans_list,
			dpt_pio_table[rdata->pio_index].trans_length,
			dpt_pio_table[rdata->pio_index].mapping_flags, 
			DPT_PACE, 0);
}

/*
 * ================================================================
 * dpt_bus_bind_get_pio_handles
 * MESSAGES: 2100 - 2199
 * ================================================================
 */
STATIC void
dpt_bus_bind_get_pio_handle(udi_cb_t *gcb,
		udi_pio_handle_t pio_handle)
{
	dpt_region_data_t *rdata = gcb->context;

	rdata->pio_handle[rdata->pio_index++] = pio_handle;

	if (rdata->pio_index < dpt_pio_table_len) {
		udi_pio_map( dpt_bus_bind_get_pio_handle, gcb,
			UDI_PCI_BAR_0, DPT_PCI_OFFSET, DPT_LENGTH,
			dpt_pio_table[rdata->pio_index].trans_list,
			dpt_pio_table[rdata->pio_index].trans_length,
			dpt_pio_table[rdata->pio_index].mapping_flags, 
			DPT_PACE, 0);
		return;
	}

	/*
	 * Start the interrupt attachment process.
	 * 	spawn our end of the channel 
	 *	the bridge will spawn its end
	 */
	udi_channel_spawn( dpt_bus_bind_intr_attach_cb_alloc,
		gcb, gcb->channel,
		DPT_INTR_SPAWN_IDX,
		INTR_HANDLER_OPS_IDX,
		rdata
	);
}

/*
 * MESSAGES: 2200 - 2299
 */
STATIC void
dpt_bus_bind_intr_attach_cb_alloc( udi_cb_t *gcb,
	udi_channel_t interrupt_channel)
{
	dpt_region_data_t *rdata = gcb->context;

	/* Save the interrupt channel */
	rdata->interrupt_channel = interrupt_channel;

	/*
	 * Allocate an interrupt attach control block to communicate with
	 * the interrupt dispatcher driver to attach its interrupt handler.
	 */
	udi_cb_alloc(dpt_bus_bind_intr_attach,
			gcb, INTR_ATTACH_CB_IDX,
			gcb->channel);
}

/*
 * ================================================================
 * dpt_bus_bind_intr_attach
 * MESSAGES: 2300 - 2399
 * ================================================================
 */
STATIC void
dpt_bus_bind_intr_attach(udi_cb_t *gcb, udi_cb_t *new_cb)
{
	dpt_region_data_t *rdata = gcb->context;
	udi_intr_attach_cb_t *intr_attach_cb =
				UDI_MCB(new_cb, udi_intr_attach_cb_t);

	/* 
	 * spawn index is used by the environment to find the interrupt channel 
	 */
	intr_attach_cb->interrupt_idx = DPT_INTR_SPAWN_IDX;
	intr_attach_cb->min_event_pend = DPT_MIN_INTR_EVENT_CB;
	intr_attach_cb->preprocessing_handle =
					rdata->pio_handle[DPT_INTR_PIO_HANDLE];
	udi_intr_attach_req (intr_attach_cb);
}

/*
 * ================================================================
 * dpt_intr_attach_ack
 * MESSAGES: 2400 - 2499
 * ================================================================
 */
STATIC void
dpt_intr_attach_ack ( udi_intr_attach_cb_t *intr_attach_cb,
			 udi_status_t status )
{
	dpt_region_data_t *rdata = UDI_GCB(intr_attach_cb)->context;

	DPT_TRACE((&rdata->init_context, UDI_TREVENT_LOCAL_PROC_ENTRY,
			DPT_BRIDGE_META_IDX, 2410 /* see udiprops.txt */,
			intr_attach_cb, status));

	if (status != UDI_OK) {
		/* Interrupt attachment failed. Fail the bind */

		/* Free the intr_attach_cb */
		udi_cb_free(UDI_GCB(intr_attach_cb));

		/* close the channel */

		/* Unmap the pio handles */

		/* Free the EATA memory */

		/* Free the bind cb */

		/* Log event and post failure to parent bind request */
		udi_log_write(dpt_bus_channel_event_ind_failure,
			rdata->mgmt_cb, UDI_TREVENT_EXTERNAL_ERROR,
			UDI_LOG_ERROR, DPT_BRIDGE_META_IDX, status,
			2420 /* see udiprops.txt */, status);
		return;
	}

	rdata->intr_attach_cb = intr_attach_cb;

	/*
	 * Allocate and register the intr_event_cb with the dispatcher
	 */

	rdata->num_intr_event_cb = 0;
	udi_cb_alloc(dpt_intr_event_rdy, UDI_GCB(intr_attach_cb),
		INTR_EVENT_CB_IDX, rdata->interrupt_channel);

}
STATIC void
dpt_intr_event_rdy(udi_cb_t *gcb, udi_cb_t *new_cb)
{
	dpt_region_data_t *rdata = gcb->context;
	udi_intr_event_cb_t *intr_event_cb =
			UDI_MCB(new_cb, udi_intr_event_cb_t);

	udi_intr_event_rdy(intr_event_cb);
	if (++rdata->num_intr_event_cb < DPT_MAX_INTR_EVENT_CB) {
		udi_cb_alloc(dpt_intr_event_rdy, gcb,
				INTR_EVENT_CB_IDX, rdata->interrupt_channel);
		return;
	}
	
	/*
	 * Get the EATA configuration
	 */
	udi_pio_trans(dpt_bus_bind_init_adapter,
		gcb,
		rdata->pio_handle[DPT_CONFIG_PIO_HANDLE],
		0, NULL, rdata->eata_info.eata_cfgp);
}

/*
 * ================================================================
 * dpt_bus_bind_init_adapter
 * MESSAGES: 2500 - 2599
 * ================================================================
 */
STATIC void
dpt_bus_bind_init_adapter(udi_cb_t *gcb,
	udi_buf_t *buf,
	udi_status_t status,
	udi_ubit16_t result)
{
	dpt_region_data_t *rdata = gcb->context;
	dpt_eata_cfg_t *eata_cfg = rdata->eata_info.eata_cfgp;

        /*
	 * Verify that it is an EATA Controller
	 */
	if (udi_strncmp((char *)eata_cfg->EATAsignature, "EATA", 4)) {
		udi_debug_printf("Initialize failed config %p\n", eata_cfg);
		/* TODO - reset the controller and try the pio trans again */
		udi_assert(0);
	}

	rdata->ha_channel_id[0] = eata_cfg->Chan_0_ID;
	rdata->ha_channel_id[1] = eata_cfg->Chan_1_ID;
	rdata->ha_channel_id[2] = eata_cfg->Chan_2_ID;

	/*
	 * Allocate DMAable memory for inquiry buffer
	 */
	udi_dma_mem_alloc(dpt_bus_bind_setup_inquiry_buffer,
		gcb, rdata->dma_constraints,
		UDI_DMA_IN|UDI_DMA_NEVERSWAP,
		1, sizeof(dpt_ident_t), 0);
}

/*
 * MESSAGES: 2600 - 2699
 */
STATIC void
dpt_bus_bind_setup_inquiry_buffer(udi_cb_t *gcb,
		udi_dma_handle_t dma_handle,
		void *inquiry_buf,
		udi_size_t actual_gap,
		udi_boolean_t single_element,
		udi_scgth_t *inquiry_sg,
		udi_boolean_t must_swap)
{
	dpt_region_data_t *rdata = gcb->context;

	rdata->scan_req.inq_dma_handle = dma_handle;
	rdata->scan_req.inq_data = inquiry_buf;
	rdata->scan_req.inq_sg = inquiry_sg;
	rdata->scan_req.inq_must_swap = must_swap;
	
	DPT_ASSERT(single_element == TRUE);

	/* Allocate a CCB for device scanning */

	/*
	 * Allocate DMAable memory for CCBs
	 */
	udi_dma_mem_alloc(dpt_bus_bind_alloc_scan_ccb,
		gcb, rdata->ccb_constraints,
		UDI_DMA_IN|UDI_DMA_OUT|UDI_DMA_BIG_ENDIAN,
		1, sizeof(dpt_ccb_t), 0);
}

STATIC void
dpt_bus_bind_alloc_scan_ccb(udi_cb_t *gcb,
		udi_dma_handle_t dma_handle,
		void *ccb,
		udi_size_t actual_gap,
		udi_boolean_t single_element,
		udi_scgth_t *ccb_sg,
		udi_boolean_t must_swap)
{
	dpt_region_data_t *rdata = gcb->context;

	dpt_ccb_t	*cp = ccb;

	cp->c_dma_handle = dma_handle;
	cp->c_must_swap = must_swap;
	cp->c_sg = ccb_sg;

	dpt_prep_ccb(cp);

	rdata->scan_req.scan_ccb = cp;
	
	/*
	 * Complete the channel_event operation now
	 */

#ifdef NOTYET
	rdata->timeout_granularity.seconds = DPT_DEFAULT_TIMEOUT_SECS;
	rdata->timeout_granularity.nanoseconds = DPT_DEFAULT_TIMEOUT_NSECS;
#endif

	udi_channel_event_complete(
		UDI_MCB(rdata->mgmt_cb, udi_channel_event_cb_t),
		UDI_OK);
	rdata->mgmt_cb = NULL; /* Invalidate the cached mgmt cb */
}

/*
 * dpt_channel_event_failure -
 *	Handle bus_device channel event failure
 *
 * MESSAGES: 2900 - 2999
 */

STATIC void
dpt_bus_channel_event_ind_failure(udi_cb_t *gcb,
		udi_status_t correlated_status)
{
	dpt_region_data_t	*rdata = gcb->context;

	DPT_ASSERT(rdata);

	DPT_TRACE((&rdata->init_context, UDI_TREVENT_LOCAL_PROC_ENTRY,
			DPT_BRIDGE_META_IDX, 2910 /* see udiprops.txt */,
			gcb, correlated_status));

	/* TODO:
	 * Free any resources allocated
	 */

	udi_channel_event_complete(
			UDI_MCB(rdata->mgmt_cb, udi_channel_event_cb_t),
			correlated_status);
	rdata->mgmt_cb = NULL; /* Complete the channel operation */

	DPT_TRACE((&rdata->init_context, UDI_TREVENT_LOCAL_PROC_EXIT,
			DPT_BRIDGE_META_IDX, 2990 /* see udiprops.txt */));
	
}

/*
 * ================================================================
 * dpt_enumerate_req 
 *	Enumerate SCSI devices.
 *	We are called repeatedly until flag set to say it is last device..
 *	Enumeration result is usually ok or done.
 * Notes:
 *	Memory for the child device is already allocated. The space
 *	requirement for the child data is provided in the init struct.
 * MESSAGES: 3000 - 3099
 * ================================================================
 */

STATIC void
dpt_enumerate_req( udi_enumerate_cb_t *enumerate_cb,
		udi_ubit8_t enumeration_level)
{
	dpt_region_data_t *rdata = UDI_GCB(enumerate_cb)->context;

	rdata->mgmt_cb = UDI_GCB(enumerate_cb);

	/* just look for valid input */

	switch (enumeration_level) {
		case UDI_ENUMERATE_START:
		case UDI_ENUMERATE_START_RESCAN:
			/* TODO - Implement filter processing */
			/* start from beginning */
			/* Enumerate the multi-lun PD first */
			dpt_enumerate_multilun_pd(rdata);
			break;
		case UDI_ENUMERATE_NEXT:
			dpt_enumerate_scan(rdata, &rdata->scan_req.addr);
			break;
		case UDI_ENUMERATE_NEW:
			/* Hot plug devices. assert for now. 
			 * should hold on to CB until hot plug event, override
			 * from MA or unbind from parent
			 */
			udi_enumerate_ack(enumerate_cb, 
				UDI_ENUMERATE_FAILED, 0);
			break;
		case UDI_ENUMERATE_DIRECTED:
			/*
			 * In this call, the MA wants us to make believe
			 * that we found a specific device.
			 */

			/*
			 *	Respond FAILED for now.	
			 */
			udi_enumerate_ack(enumerate_cb,
				UDI_ENUMERATE_FAILED, SCSI_OPS_IDX);
			rdata->mgmt_cb = NULL; /* invalidate the mgmt_cb */

			break;
		case UDI_ENUMERATE_RELEASE:
			/*
			 * TODO: release child related resources
			 */
			udi_enumerate_ack(enumerate_cb,
				UDI_ENUMERATE_RELEASED, 0);
			break;
		default:
			/* Unknown option assert for now. */
			/* udi_assert(0); */
			udi_enumerate_ack(enumerate_cb,
				UDI_ENUMERATE_DONE, SCSI_OPS_IDX);
			rdata->mgmt_cb = NULL; /* invalidate the mgmt_cb */
			break;
	}
}

/*
 * ================================================================
 * dpt_enumerate_scan
 * Launch scan for ID specified by rdata->scan_target_index
 * by transmitting an Inquiry command.
 * MESSAGES: 3100 - 3199
 * ================================================================
 */
STATIC void
dpt_enumerate_scan(dpt_region_data_t *rdata, dpt_scsi_addr_t *addr)
{
	dpt_ccb_t	*cp;
	udi_ubit8_t max_bus = rdata->eata_info.eata_cfgp->max_chan_tgt >> 5;

	if (addr->bus > max_bus) {
		addr->bus = 0;
		/* We are done enumerating */
		udi_enumerate_ack(UDI_MCB(rdata->mgmt_cb, udi_enumerate_cb_t),
				UDI_ENUMERATE_DONE, SCSI_OPS_IDX);
		/* invalidate the mgmt_cb */
		rdata->mgmt_cb = NULL;
		return;
	}

	cp = rdata->scan_req.scan_ccb;

	/* cp->c_bind = rdata; */

	cp->c_callback = dpt_enumerate_scan_done;

	/* Build an EATA command packet */

	cp->c_time      = 0;
	if (addr->target == rdata->ha_channel_id[addr->bus])
		cp->CPop = DPT_HA_AUTO_REQ_SEN | DPT_HA_DATA_IN |
							DPT_HA_INTERPRET;
	else
		cp->CPop = DPT_HA_AUTO_REQ_SEN | DPT_HA_DATA_IN;
	cp->CPbusid       = addr->bus << 5 | addr->target;
	cp->CPmsg0      = (DPT_HA_IDENTIFY_MSG | DPT_HA_DISCO_RECO) + addr->lun;

	/*
	 * We don't have to do any swapping here as we are directly
	 * copying the first element of the scatter/gather list
	 * which is already appropriately swapped.
	 */
	cp->CPdataDMA	= rdata->scan_req.inq_sg->
				scgth_elements.el32p[0].block_busaddr;
	cp->CPdataLen	= rdata->scan_req.inq_sg->
				scgth_elements.el32p[0].block_length;

	cp->CPcdb[0] = SS_INQUIR;
	cp->CPcdb[1] = 0;
	cp->CPcdb[2] = 0;
	cp->CPcdb[3] = 0;
	cp->CPcdb[4] = (udi_ubit8_t)INQUIRY_SIZE;

	UDI_ENQUEUE_TAIL(&rdata->pending_jobs, &cp->link);

	dpt_send_dma_cmd(rdata, cp);
}

STATIC void
dpt_enumerate_multilun_pd(dpt_region_data_t *rdata)
{
	udi_enumerate_cb_t *enumerate_cb =
				UDI_MCB(rdata->mgmt_cb, udi_enumerate_cb_t);
	dpt_device_info_t *device_info = enumerate_cb->child_data;
	udi_instance_attr_list_t *attr_list = enumerate_cb->attr_list;
	udi_ubit8_t max_bus = rdata->eata_info.eata_cfgp->max_chan_tgt >> 5;
	udi_ubit8_t max_tgt = rdata->eata_info.eata_cfgp->max_chan_tgt & 0x1F;
	udi_ubit8_t max_lun = rdata->eata_info.eata_cfgp->max_lun;

	/* zero out the device info struct */
	udi_memset(device_info, 0, sizeof(dpt_device_info_t));

	device_info->rdata = rdata;

	DPT_SET_ATTR_BOOLEAN(attr_list, "scsi_multi_lun", TRUE);
	attr_list++;
	DPT_SET_ATTR32(attr_list, "scsi_max_buses", max_bus + 1);
	attr_list++;
	DPT_SET_ATTR32(attr_list, "scsi_max_tgts", max_tgt + 1);
	attr_list++;
	DPT_SET_ATTR32(attr_list, "scsi_max_luns", max_lun + 1);
	attr_list++;

	udi_assert((attr_list - enumerate_cb->attr_list) <= DPT_MAX_ENUM_ATTR);
	enumerate_cb->child_ID = device_info->child_ID = rdata->dpt_child_id++;
	enumerate_cb->attr_valid_length = attr_list - enumerate_cb->attr_list;
	UDI_QUEUE_INIT(&device_info->active_q);
	UDI_QUEUE_INIT(&device_info->tag_q);
	UDI_QUEUE_INIT(&device_info->suspend_q);
	UDI_ENQUEUE_TAIL(&rdata->device_list, &device_info->link);

	udi_enumerate_ack(enumerate_cb, UDI_ENUMERATE_OK, SCSI_OPS_IDX);
	rdata->mgmt_cb = NULL;	/* invalidate the mgmt CB */
}

/*
 * ================================================================
 * dpt_enumerate_scan_done
 * If scan found a device, then populate a dpt_device_info_t for it.
 * Otherwise, send next command.
 * MESSAGES: 3300 - 3399
 * ================================================================
 */
STATIC void
dpt_enumerate_scan_done(dpt_region_data_t *rdata, dpt_ccb_t *cp)
{
	udi_enumerate_cb_t *enumerate_cb =
				UDI_MCB(rdata->mgmt_cb, udi_enumerate_cb_t);
	dpt_device_info_t *device_info;
	char	locator_buf[DPT_LOCATOR_ATTR_LEN];
	char	identifier[DPT_IDENTIFIER_ATTR_LEN];
	udi_ubit8_t	lun[8]; /* 8 byte SCSI LUN as per SAM-2 */
	udi_instance_attr_list_t *attr_list;
	dpt_scsi_addr_t *addr = &rdata->scan_req.addr;
	/*udi_ubit8_t max_bus = rdata->eata_info.eata_cfgp->max_chan_tgt >> 5;*/
	udi_ubit8_t max_tgt = rdata->eata_info.eata_cfgp->max_chan_tgt & 0x1F;
	udi_ubit8_t max_lun = rdata->eata_info.eata_cfgp->max_lun;

	if ((cp->CP_status.s_ha_status != 0) ||
	    (DPT_DTYPE(rdata->scan_req.inq_data->id_dev_pqual) ==
							DPT_ID_NODEV) ||
	    (DPT_PQUAL(rdata->scan_req.inq_data->id_dev_pqual) ==
							DPT_ID_QNOLU)) {
		/*
		 * One of the following occured:
		 * - Selection timeout
		 * - physical device not connected
		 */

		/*
		 * If something is there, it must respond at LUN 0.
		 * skip all the luns if we didn't find anything at LUN 0.
		 */
		if (addr->lun == 0)
			addr->lun = max_lun;

		/* Enumerate the next device on this channel */
		if (++addr->lun > max_lun) {
			addr->lun = 0;
			if (++addr->target > max_tgt) {
				addr->target = 0;
				++addr->bus;
			}
		}
		dpt_enumerate_scan(rdata, addr);
		return;
	}

	/*
	 * Fill it out with everything we know (inquiry data saved in region
	 * data, target id) for the management agent and PD's. 
	 */

	device_info = enumerate_cb->child_data;
	attr_list = enumerate_cb->attr_list;

	/* zero out the device info struct */
	udi_memset(device_info, 0, sizeof(dpt_device_info_t));

	device_info->scsi_bus = addr->bus;
	device_info->tgt_id = addr->target;
	device_info->lun_id = addr->lun;

	device_info->rdata = rdata;

	/* transfer inquiry data from DMAable to 'normal' memory in device */

	udi_memcpy((udi_ubit8_t *)&device_info->inquiry_data,
			rdata->scan_req.inq_data, INQUIRY_SIZE);

	rdata->num_devices++;

	/* construct the locator attribute */

	udi_snprintf(locator_buf, sizeof(locator_buf), "%02X%0X%0X%0X%0X", 
		device_info->scsi_bus & 0xFF,
		0, device_info->tgt_id,
		0, device_info->lun_id);

	/*
	 * Now set the attributes ...
	 */

	DPT_SET_ATTR32(attr_list, "scsi_bus", device_info->scsi_bus);
	attr_list++;
	DPT_SET_ATTR32(attr_list, "scsi_target", device_info->tgt_id);
	attr_list++;

	/*
	 * Construct the LUN attribute 
	 * Spec note: When specifying or interpreting the 8-byte LUN value
	 * for non-SCSI-3 case, the LUN value shall conform to the
	 * Single Level LUN Structure as per SAM-2 (i.e byte 0 is zero,
	 * byte 1 contains the LUN value and remaining 6 bytes are zero).
	 */

	udi_memset(lun, 0, sizeof(lun));
	lun[1] = device_info->lun_id;
	DPT_SET_ATTR_ARRAY8(attr_list, "scsi_lun", lun, sizeof(lun));
	attr_list++;

	DPT_SET_ATTR32(attr_list, "scsi_dev_pqual",
	    		DPT_PQUAL(device_info->inquiry_data.id_dev_pqual));
	attr_list++;
	DPT_SET_ATTR32(attr_list, "scsi_dev_type", 
	    		DPT_DTYPE(device_info->inquiry_data.id_dev_pqual));
	attr_list++;

	DPT_SET_ATTR_STRING(attr_list, "scsi_vendor_id",
		(char *)&device_info->inquiry_data.id_vendor, DPT_VID_LEN);
	attr_list++;
	DPT_SET_ATTR_STRING(attr_list, "scsi_product_id",
		(char *)&device_info->inquiry_data.id_prod, DPT_PID_LEN);
	attr_list++;
	DPT_SET_ATTR_STRING(attr_list, "scsi_product_rev",
		(char *)&device_info->inquiry_data.id_revnum, DPT_REV_LEN);
	attr_list++;
	DPT_SET_ATTR_ARRAY8(attr_list, "scsi_inquiry",
		(udi_ubit8_t *)&device_info->inquiry_data, INQUIRY_SIZE);
	attr_list++;
	/*
	 * Enumerate scsi_tgt_wwid and scsi_lun_wwid attributes if available
	 * here.
	 */
	udi_scsi_inquiry_to_string((udi_ubit8_t *)&device_info->inquiry_data, 
		INQUIRY_SIZE, identifier);
	DPT_SET_ATTR_STRING(attr_list, "identifier",
		identifier, udi_strlen(identifier));
	attr_list++;
	DPT_SET_ATTR_STRING(attr_list, "address_locator",
		locator_buf, udi_strlen(locator_buf));
	attr_list++;
	if (addr->target == rdata->ha_channel_id[addr->bus]) {
		DPT_SET_ATTR_BOOLEAN(attr_list, "scsi_target_mode_supported",
			FALSE);
		attr_list++;
	}	
	udi_assert((attr_list - enumerate_cb->attr_list) <= DPT_MAX_ENUM_ATTR);
	enumerate_cb->child_ID = device_info->child_ID = rdata->dpt_child_id++;
	enumerate_cb->attr_valid_length = attr_list - enumerate_cb->attr_list;
	UDI_QUEUE_INIT(&device_info->active_q);
	UDI_QUEUE_INIT(&device_info->tag_q);
	UDI_QUEUE_INIT(&device_info->suspend_q);
	UDI_ENQUEUE_TAIL(&rdata->device_list, &device_info->link);

	/* Advance to the next device */
	/*
	 * If this was an HBA, it must respond at LUN 0.
	 * skip all non-zero luns for HBA
	 */
	if ((addr->lun == 0) && 
	    (addr->target == rdata->ha_channel_id[addr->bus]))
		addr->lun = max_lun;
	if (++addr->lun > max_lun) {
		addr->lun = 0;
		if (++addr->target > max_tgt) {
			addr->target = 0;
			++addr->bus;
		}
	}

	udi_enumerate_ack(enumerate_cb, UDI_ENUMERATE_OK, SCSI_OPS_IDX);
	rdata->mgmt_cb = NULL;	/* invalidate the mgmt CB */
}


/*
 * dpt_find_device
 * Searches region data for the linked device_info structure
 * which matches the Child ID passed in to the call.
 * MESSAGES: 3500 - 3599
 */
STATIC dpt_device_info_t *
dpt_find_device(dpt_region_data_t *rdata, udi_ubit32_t child_id)
{
	udi_queue_t *device_info, *tmp;
	UDI_QUEUE_FOREACH(&rdata->device_list, device_info, tmp) {
		if (((dpt_device_info_t *)device_info)->child_ID == child_id)
			return((dpt_device_info_t *)device_info);
	}

	return(dpt_device_info_t *)NULL; /* known to be NULL here */
}

/*
 * ====================================================================
 * BEGIN: udi_scsi_bind_req operation.
 *
 * The SCSI metalanguage specific binding operation is done here.
 * The tasks to be performed here are
 *	1. Setting up the target based data structures with the appropriate
 *	values as per requested by the PD through the bind request.
 *	2. Initializing target based structures.
 *	3. Set the sentinel timer resolution.
 *	4. Set the channel context to be the child info structure.
 *	5. Send a scsi bind reply.
 * Use udi_instance_attr_get to find out the target ID
 * of the device we're talking about.
 *
 * Use scratch area to to save attribute data.
 *
 * END: udi_scsi_bind_req operation
 * MESSAGES: 3600 - 3699
 * ========================================================================
 */
STATIC void
dpt_scsi_bind_req(udi_scsi_bind_cb_t *scsi_bind_cb,
		udi_ubit16_t bind_flags,
		udi_ubit16_t queue_depth,
		udi_ubit16_t	max_sense_len,
		udi_ubit16_t	aen_buf_size )
{
	dpt_child_context_t	*child_context = UDI_GCB(scsi_bind_cb)->context;
	dpt_region_data_t *rdata = child_context->chan_context.rdata;
	dpt_device_info_t *device_info;

	device_info = dpt_find_device(rdata,
			child_context->chan_context.child_ID);

	if (device_info != NULL) {
		/* Set the bind_channel's context to 
		   the associated device_info. */
		child_context->device_info = device_info;
	}
	else {
		udi_debug_printf("dpt: dpt_scsi_bind_req error: child_ID=0x%x\n",
			child_context->chan_context.child_ID);
		udi_scsi_bind_ack (scsi_bind_cb,
			0,  /* TODO: hd_timeout_increase */
			UDI_STAT_NOT_RESPONDING);
		return;
	}

	/*
	 * Having fetched the target ID from the instance attributes,
	 * associate it with a pre-initialized dpt_device_info_t,
	 * and set the channel context to that per-device structure.
	 */

	device_info->events	= scsi_bind_cb->events;

	/* TODO - check for bind_flags and validity of the bind */

	device_info->bind_flags = bind_flags;
	device_info->queue_depth = queue_depth;
	device_info->max_sense_len = max_sense_len;
	device_info->aen_buf_size = aen_buf_size;

	/* Save a copy of constraints object for this device. */
	device_info->constraints = rdata->dma_constraints;

	/*
	 * Allocate resources for this device
	 */

	device_info->num_resources = 0;
	/* Allocate DMA handle used for udi_dma_buf_map */
	udi_dma_prepare(dpt_scsi_bind_alloc_dma_handle,
		UDI_GCB(scsi_bind_cb),
		rdata->dma_constraints,
		UDI_DMA_IN|UDI_DMA_OUT);
}

/*
 * ================================================================
 *
 * Notes:
 *	Must use a CB with the correct scratch space 
 * MESSAGES: 2700 - 2799
 * ================================================================
 */
STATIC void
dpt_scsi_bind_alloc_dma_handle(udi_cb_t *gcb, udi_dma_handle_t dma_handle)
{
	dpt_device_info_t *device_info =
			((dpt_child_context_t *)gcb->context)->device_info;
	dpt_region_data_t *rdata = device_info->rdata;
	dpt_scsi_bind_scratch_t	*scratch = gcb->scratch;

	scratch->dma_handle = dma_handle;

	/*
	 * Allocate DMAable memory for CCBs
	 */
	udi_dma_mem_alloc(dpt_scsi_bind_alloc_ccb,
		gcb, rdata->ccb_constraints,
		UDI_DMA_IN|UDI_DMA_OUT|UDI_DMA_BIG_ENDIAN,
		1, sizeof(dpt_ccb_t), 0);
}

/*
 * MESSAGES: 2800 - 2899
 */
STATIC void
dpt_scsi_bind_alloc_ccb(udi_cb_t *gcb,
		udi_dma_handle_t dma_handle,
		void *ccb,
		udi_size_t actual_gap,
		udi_boolean_t single_element,
		udi_scgth_t *ccb_sg,
		udi_boolean_t must_swap)
{
	dpt_device_info_t *device_info =
			((dpt_child_context_t *)gcb->context)->device_info;
	dpt_region_data_t *rdata = device_info->rdata;
	dpt_scsi_bind_scratch_t	*scratch = gcb->scratch;

	dpt_ccb_t	*cp = ccb;

	cp->buf_dma_handle = scratch->dma_handle;
	cp->c_dma_handle = dma_handle;
	cp->c_must_swap = must_swap;
	cp->c_sg = ccb_sg;

	dpt_prep_ccb(cp);

	UDI_ENQUEUE_TAIL(&rdata->ccb_pool, &cp->link);

	if (++device_info->num_resources < device_info->queue_depth) {
		/* Allocate more resources */
		udi_dma_prepare(dpt_scsi_bind_alloc_dma_handle,
			gcb, rdata->dma_constraints,
			UDI_DMA_IN|UDI_DMA_OUT);
		return;
	}

	/*
	 * All CCBs have been allocated
	 * Complete the bind request
	 */
	udi_scsi_bind_ack (UDI_MCB(gcb, udi_scsi_bind_cb_t),
		0,  /* TODO: hd_timeout_increase */
		UDI_OK);
}

/*
 * ====================================================================
 * BEGIN: udi_scsi_io_req operation.
 *	Tasks:
 *
 *	This is where the real action happens. A scsi io request is
 *	received here. A CCB (command control block) is initialized
 *	and sent to the controller.
 *
 * END: udi_scsi_io_req operation.
 * MESSAGES: 3700 - 3799
 * ========================================================================
 */
STATIC void 
dpt_scsi_io_req(udi_scsi_io_cb_t *scsi_io_cb)
{
	dpt_device_info_t *device_info = ((dpt_child_context_t *)
				UDI_GCB(scsi_io_cb)->context)->device_info;
	dpt_region_data_t *rdata = device_info->rdata;
	udi_ubit32_t	direction;

	/* Use the scratch area for driver-visible request info */
	dpt_req_t	*req	= (dpt_req_t *) UDI_GCB(scsi_io_cb)->scratch;
	dpt_ccb_t	*cp;

	DPT_ASSERT(rdata);

	DPT_TRACE((&rdata->init_context, UDI_TREVENT_LOCAL_PROC_ENTRY,
		DPT_SCSI_META_IDX, 3710 /* see udiprops.txt */, scsi_io_cb));

	req->scsi_io_cb	= scsi_io_cb;

	/* TODO - check for QFULL condition */

	/*  check for DPT_SUSPEND condition */
	if (device_info->d_state & DPT_SUSPEND) {
		UDI_ENQUEUE_TAIL(&device_info->suspend_q, &req->Q);
		return;
	}

	if ((cp = dpt_get_ccb(rdata)) == NULL) {
		UDI_ENQUEUE_TAIL(&rdata->alloc_q, &req->Q);
		DPT_TRACE((&rdata->init_context, UDI_TREVENT_LOCAL_PROC_EXIT,
			DPT_SCSI_META_IDX, 3790 /* see udiprops.txt */));
		return;
	}

	req->ccb = cp;

#ifdef NOTYET
	if (device_info->active_reqs >= device_info->queue_depth) {
		/* TODO - return QFULL condition */
		UDI_ENQUEUE_TAIL(&device_info->tag_q, &req->Q);
		DPT_TRACE((&rdata->init_context, UDI_TREVENT_LOCAL_PROC_EXIT,
			DPT_SCSI_META_IDX, 3790 /* see udiprops.txt */));
		return;
	}
#endif
	cp->c_bind = scsi_io_cb;
	cp->c_callback = dpt_scsi_io_req_done;

	/* Build an EATA command packet */

	cp->c_time      = 0;	/* ??? */
	cp->CPop = DPT_HA_AUTO_REQ_SEN | DPT_HA_SCATTER |
			((scsi_io_cb->flags & UDI_SCSI_DATA_IN) ?
				DPT_HA_DATA_IN : DPT_HA_DATA_OUT);
	if (device_info->tgt_id == rdata->ha_channel_id[device_info->scsi_bus])
		cp->CPop |= DPT_HA_INTERPRET;
	cp->CPbusid     = ((udi_ubit8_t)device_info->scsi_bus << 5) |
					device_info->tgt_id;
	cp->CPmsg0      = (DPT_HA_IDENTIFY_MSG | DPT_HA_DISCO_RECO) +
					(udi_ubit8_t)(device_info->lun_id);
	udi_memcpy(cp->CPcdb, scsi_io_cb->cdb_ptr, scsi_io_cb->cdb_len);

	if (scsi_io_cb->data_buf != NULL) {

		direction = 0;
		if (scsi_io_cb->flags & UDI_SCSI_DATA_IN) {
			direction = UDI_DMA_IN;
		}
		else if (scsi_io_cb->flags & UDI_SCSI_DATA_OUT) {
			direction = UDI_DMA_OUT;
		}
		if (direction) {
			udi_dma_buf_map( dpt_ioreq_sg_acquired,
				UDI_GCB(scsi_io_cb),
				cp->buf_dma_handle,
				scsi_io_cb->data_buf,
				0,	/* offset */
				scsi_io_cb->data_buf->buf_size,
				direction);
			DPT_TRACE((&rdata->init_context,
				UDI_TREVENT_LOCAL_PROC_EXIT,
				DPT_SCSI_META_IDX,
				3790 /* see udiprops.txt */));
			return;
		}
	}

	cp->CPdataLen	= 0;

	UDI_ENQUEUE_TAIL(&rdata->pending_jobs, &cp->link);

	dpt_timer_insert(req);

	device_info->active_reqs++;

	dpt_send_dma_cmd(rdata, cp);

	DPT_TRACE((&rdata->init_context, UDI_TREVENT_LOCAL_PROC_EXIT,
			DPT_SCSI_META_IDX, 3790 /* see udiprops.txt */));
}

/*
 * MESSAGES: 3800 - 3899
 */
STATIC void
dpt_ioreq_sg_acquired(udi_cb_t *gcb, udi_scgth_t *sglist, 
		udi_boolean_t complete, udi_status_t status)
{
	udi_scsi_io_cb_t *scsi_io_cb = UDI_MCB(gcb, udi_scsi_io_cb_t);
	dpt_device_info_t *device_info = ((dpt_child_context_t *)
				UDI_GCB(scsi_io_cb)->context)->device_info;
	dpt_region_data_t *rdata = device_info->rdata;
	dpt_req_t	*req = (dpt_req_t *) gcb->scratch;
	dpt_ccb_t	*cp = req->ccb;

	if (cp->c_must_swap) {
		cp->CPdataDMA = UDI_ENDIAN_SWAP_32(sglist->
				scgth_first_segment.el32.block_busaddr);
		cp->CPdataLen = UDI_ENDIAN_SWAP_32(sglist->scgth_num_elements *
						sizeof(udi_scgth_element_32_t));
	}
	else {
		cp->CPdataDMA = sglist->scgth_first_segment.el32.block_busaddr;
		cp->CPdataLen = sglist->scgth_num_elements *
					sizeof(udi_scgth_element_32_t);
	}

	UDI_ENQUEUE_TAIL(&rdata->pending_jobs, &cp->link);

	dpt_timer_insert(req);

	device_info->active_reqs++;

	dpt_send_dma_cmd(rdata, cp);
}

/*
 * ========================================================================
 * dpt_scsi_io_req_done
 *
 * MESSAGES: 3900 - 3999
 * ========================================================================
 */
STATIC void
dpt_scsi_io_req_done(dpt_region_data_t *rdata, dpt_ccb_t *cp)
{
	udi_scsi_io_cb_t *scsi_io_cb = cp->c_bind;
	dpt_device_info_t *device_info = ((dpt_child_context_t *)
				UDI_GCB(scsi_io_cb)->context)->device_info;
	dpt_req_t	*req	= (dpt_req_t *)UDI_GCB(scsi_io_cb)->scratch;
	dpt_status_t	*sp = &cp->CP_status;	/* Pointer to status packet */
	udi_status_t	req_status;
	udi_ubit8_t	scsi_status;

	DPT_TRACE((&rdata->init_context, UDI_TREVENT_LOCAL_PROC_ENTRY,
		DPT_MGMT_META_IDX, 3910 /* see udiprops.txt */,
		rdata, cp));

	cp->c_bind = NULL; 

	device_info->active_reqs--;
	dpt_timer_remove(req);

#ifdef NOTYET
	/* start with zero residual until proven otherwise */
	scsi_io_cb->data_xfer_cnt	= scsi_io_cb->data_len;
#endif

	if (scsi_io_cb->data_buf != NULL) {
		scsi_io_cb->data_buf = udi_dma_buf_unmap(cp->buf_dma_handle, 
				 	 scsi_io_cb->data_buf->buf_size);
	}

	if ((sp->s_ha_status == 0) && (sp->s_scsi_status == 0)) {
		dpt_scsi_comp(scsi_io_cb, UDI_OK, 0, 0, NULL);
		DPT_TRACE((&rdata->init_context, UDI_TREVENT_LOCAL_PROC_EXIT,
			DPT_SCSI_META_IDX, 3990 /* see udiprops.txt */));
		return;
	}

	/*
	 * start exception handling
	 */

	sp = &cp->CP_status;	/* Pointer to status packet */

#ifdef DPT_DEBUG
	udi_debug_printf("dpt_scsi_io_req_done: Completion error; HA Status 0x%x, SCSI Status 0x%x\n",
			sp->s_ha_status, sp->s_scsi_status);
#endif

	/* set default values */
	req_status = UDI_SCSI_STAT_NONZERO_STATUS_BYTE;
	scsi_status = sp->s_scsi_status;

	switch(sp->s_ha_status) {
	case DPT_SP_SELTO:
		req_status = UDI_SCSI_STAT_SELECTION_TIMEOUT;
		DPT_TRACE((&rdata->init_context, DPT_TREVENT_ERROR,
			DPT_SCSI_META_IDX, 3921 /* see udiprops.txt */,
			scsi_io_cb->cdb_ptr[0]));
		break;

	case DPT_SP_CMDTO:
		req_status = UDI_STAT_TIMEOUT;
		DPT_TRACE((&rdata->init_context, DPT_TREVENT_ERROR,
			DPT_SCSI_META_IDX, 3922 /* see udiprops.txt */,
			scsi_io_cb->cdb_ptr[0]));
		break;

	case DPT_SP_RESET:
		req_status = UDI_STAT_TIMEOUT;
		DPT_TRACE((&rdata->init_context, DPT_TREVENT_ERROR,
			DPT_SCSI_META_IDX, 3923 /* see udiprops.txt */,
			scsi_io_cb->cdb_ptr[0]));
		break;

	case DPT_SP_INITPWR:
		req_status = UDI_OK;
		DPT_TRACE((&rdata->init_context, DPT_TREVENT_INFO,
			DPT_SCSI_META_IDX, 3924 /* see udiprops.txt */,
			scsi_io_cb->cdb_ptr[0]));
		break;

	case DPT_SP_UBUSPHASE:
		req_status = UDI_SCSI_STAT_UNEXPECTED_BUS_FREE;
		DPT_TRACE((&rdata->init_context, DPT_TREVENT_ERROR,
			DPT_SCSI_META_IDX, 3925 /* see udiprops.txt */,
			scsi_io_cb->cdb_ptr[0]));
		break;
	case DPT_SP_UBUSFREE:
		req_status = UDI_SCSI_STAT_DEVICE_PHASE_ERROR;
		DPT_TRACE((&rdata->init_context, DPT_TREVENT_ERROR,
			DPT_SCSI_META_IDX, 3926 /* see udiprops.txt */,
			scsi_io_cb->cdb_ptr[0]));
		break;

	case DPT_SP_BUSPARITY:
		req_status = UDI_SCSI_STAT_DEVICE_PARITY_ERROR;
		DPT_TRACE((&rdata->init_context, DPT_TREVENT_ERROR,
			DPT_SCSI_META_IDX, 3927 /* see udiprops.txt */,
			scsi_io_cb->cdb_ptr[0]));
		break;

	case DPT_SP_SHUNG:
		req_status = UDI_STAT_TIMEOUT;
		DPT_TRACE((&rdata->init_context, DPT_TREVENT_ERROR,
			DPT_SCSI_META_IDX, 3928 /* see udiprops.txt */,
			scsi_io_cb->cdb_ptr[0]));
		break;

	case DPT_SP_CPABRTNA:	/* CP aborted - NOT on Bus */
		req_status = UDI_STAT_ABORTED;
		DPT_TRACE((&rdata->init_context, DPT_TREVENT_INFO,
			DPT_SCSI_META_IDX, 3933 /* see udiprops.txt */,
			scsi_io_cb->cdb_ptr[0]));
		break;

	case DPT_SP_CPABORTED:	/* CP aborted - WAS on Bus */
		req_status = UDI_STAT_ABORTED;
		DPT_TRACE((&rdata->init_context, DPT_TREVENT_INFO,
			DPT_SCSI_META_IDX, 3934 /* see udiprops.txt */,
			scsi_io_cb->cdb_ptr[0]));
		break;

	default:
		/* Log event */
		DPT_TRACE((&rdata->init_context, DPT_TREVENT_ERROR,
			DPT_SCSI_META_IDX, 3945 /* see udiprops.txt */,
			scsi_io_cb->cdb_ptr[0],
			sp->s_ha_status, sp->s_scsi_status));
		break;
	}

	/* Look at the target status */
	if (sp->s_scsi_status == DPT_SP_S_CKCON) {
		/*
		 * Need to get sense data.
		 * Save the scsi status in the scratch space.
		 * We'll have to use zero for sense_status, since we don't
		 * get a status value from the controller.
		 */
		req->req_status = req_status;
		req->scsi_status = sp->s_scsi_status;

		/* sense data (hopefully) received */
		if (sp->s_ha_status != DPT_SP_RSENSFAIL) {

	 		/* Need to get a udi_buf_t for sense */
			UDI_BUF_ALLOC(dpt_sense_buf_callback,
				UDI_GCB(scsi_io_cb),
				&cp->CP_sense,
				DPT_SENSE_SIZE,
				rdata->buf_path);
	 		return;
		}
		else {
			/*
			 * Autosense failed. Controller doesn't say why.
			 */

			/* Log event */
			udi_log_write(dpt_scsi_io_req_failure,
				UDI_GCB(scsi_io_cb), UDI_TREVENT_LOG,
				UDI_LOG_ERROR, DPT_SCSI_META_IDX, UDI_OK,
				3950 /* see udiprops.txt */,
				scsi_io_cb->cdb_ptr[0],
				sp->s_ha_status, sp->s_scsi_status);
			return;
		}
	}

	/* transfer scsi status to UDI struct and return */
	dpt_scsi_comp(scsi_io_cb, req_status, sp->s_scsi_status, 0, NULL);

	DPT_TRACE((&rdata->init_context, UDI_TREVENT_LOCAL_PROC_EXIT,
		DPT_MGMT_META_IDX, 3990 /* see udiprops.txt */));
}

/*
 * MESSAGES: 4000 - 4099
 */
STATIC void 
dpt_sense_buf_callback (udi_cb_t *gcb, udi_buf_t *sense_buf)
{
	udi_scsi_io_cb_t *scsi_io_cb = UDI_MCB(gcb, udi_scsi_io_cb_t);
	dpt_req_t *req	= UDI_GCB(scsi_io_cb)->scratch;

	/* Restore and use the scsi status from the scratch space */
	dpt_scsi_comp(scsi_io_cb, req->req_status, req->scsi_status, 0,
		      sense_buf);
}

/*
 * MESSAGES: 4100 - 4149
 */
STATIC void
dpt_scsi_comp (udi_scsi_io_cb_t *scsi_io_cb, udi_status_t req_status,
		udi_ubit8_t scsi_status, udi_ubit8_t sense_status,
		udi_buf_t *sense_buf)
{
	dpt_device_info_t *device_info = ((dpt_child_context_t *)
				UDI_GCB(scsi_io_cb)->context)->device_info;
	dpt_region_data_t *rdata = device_info->rdata;
	dpt_req_t		*req	= UDI_GCB(scsi_io_cb)->scratch;

	dpt_free_ccb(rdata, req->ccb);

	/* return this beast to the PD */
	if (req_status == UDI_OK)
		udi_scsi_io_ack(scsi_io_cb);
	else
		udi_scsi_io_nak(scsi_io_cb, req_status, scsi_status,
				sense_status, sense_buf);

	if (!UDI_QUEUE_EMPTY(&rdata->alloc_q)) {
		req = UDI_BASE_STRUCT(
			UDI_DEQUEUE_HEAD(&rdata->alloc_q), dpt_req_t, Q);
		dpt_scsi_io_req (req->scsi_io_cb);
	}

	if (!UDI_QUEUE_EMPTY(&device_info->tag_q)) {
		req = UDI_BASE_STRUCT(
			UDI_DEQUEUE_HEAD(&device_info->tag_q), dpt_req_t, Q);
		dpt_scsi_io_req (req->scsi_io_cb);
	}
}

/*
 * dpt_scsi_io_req_failure -
 *	Handle I/O request failures
 *
 * MESSAGES: 4150 - 4199
 */

STATIC void
dpt_scsi_io_req_failure(udi_cb_t *gcb, udi_status_t correlated_status)
{
	DPT_DEBUG_VAR(dpt_region_data_t	*rdata = gcb->context;)
	udi_scsi_io_cb_t *scsi_io_cb = UDI_MCB(gcb, udi_scsi_io_cb_t);
	dpt_req_t *req	= UDI_GCB(scsi_io_cb)->scratch;

	DPT_ASSERT(rdata);

	DPT_TRACE((&rdata->init_context, UDI_TREVENT_LOCAL_PROC_ENTRY,
			DPT_SCSI_META_IDX, 4160 /* see udiprops.txt */,
			gcb, correlated_status));

	/* Complete the I/O request */
	/* Restore and use the scsi status from the scratch space */
	dpt_scsi_comp(scsi_io_cb, req->req_status, req->scsi_status, 0, NULL);

	DPT_TRACE((&rdata->init_context, UDI_TREVENT_LOCAL_PROC_EXIT,
			DPT_BRIDGE_META_IDX, 4190 /* see udiprops.txt */));
	
}
/*
 * ====================================================================
 * BEGIN: udi_scsi_ctl_req operation.
 *	Currently the operations that are supported are:
 *	1. UDI_SCSI_CONTROL_ABORT
 *		Get the original request that needs to be aborted.
 *		Contruct an CCB (easier said than done..)
 *		for the abort request and send it to the controller.
 *
 *	2. UDI_SCSI_CONTROL_SET_QUEUE_DEPTH.
 *		Set the queue depth to be the value presented
 *		by the control request.
 *
 * END: udi_scsi_ctl_req operation.
 * MESSAGES: 4200 - 4299
 * ========================================================================
 */
STATIC void
dpt_scsi_ctl_req(udi_scsi_ctl_cb_t *scsi_ctl_cb)
{
	dpt_device_info_t *device_info = ((dpt_child_context_t *)
				UDI_GCB(scsi_ctl_cb)->context)->device_info;
	dpt_region_data_t	*rdata = device_info->rdata;
	udi_status_t	status;
	dpt_req_t	*req;
	udi_queue_t	*elem, *tmp;

	switch (scsi_ctl_cb->ctrl_func) {
	case UDI_SCSI_CTL_ABORT_TASK_SET:
	case UDI_SCSI_CTL_CLEAR_TASK_SET:

		/* 
		 * Suspend the device queue.
		 * Search and abort all the active requests for this device.
		 */
		device_info->d_state |= DPT_SUSPEND;

		/* Search the alloc queue for any jobs for this device */
		UDI_QUEUE_FOREACH(&rdata->alloc_q, elem, tmp) {
			req = UDI_BASE_STRUCT(elem, dpt_req_t, Q);
			if (UDI_GCB(req->scsi_io_cb)->context == device_info) {
				/* Found a job for this device, kill it */
				UDI_QUEUE_REMOVE(&req->Q);
				udi_scsi_io_nak(req->scsi_io_cb,
						UDI_STAT_ABORTED, 0, 0, NULL);
			}
		}

		/* Kill all jobs on tag queue */
		UDI_QUEUE_FOREACH(&device_info->tag_q, elem, tmp) {
			req = UDI_BASE_STRUCT(elem, dpt_req_t, Q);
			UDI_QUEUE_REMOVE(&req->Q);
			udi_scsi_io_nak(req->scsi_io_cb,
					UDI_STAT_ABORTED, 0, 0, NULL);
		}

		/*
		 * Search the timer queue and kill all jobs which are
		 * already issued to the device.
		 * Build a CCB for each request and issue the abort.
		 * Ignore the completions on the original CCB?
		 */

		/* Search the alloc queue for any jobs for this device */
		UDI_QUEUE_FOREACH(&rdata->timer_q, elem, tmp) {
			req = UDI_BASE_STRUCT(elem, dpt_req_t, Q);
			if (UDI_GCB(req->scsi_io_cb)->context == device_info) {
				/* Found a job for this device, kill it */
				/* remove the job from the timeout queue */
				dpt_timer_remove(req);
				dpt_abort_req(req);
				/* TODO - check if this operation is
				 * synchronous. If not, make sure that a
				 * udi_scsi_ctl_ack is issued only once
				 * after all the jobs are aborted
				 */
			}
		}

		/*
		 * Search the active queue and kill all jobs which are already
		 * issued to the device. These are the jobs for which timeout
		 * was not specified and hence were not put on the timer queue.
		 * Build a CCB for each request and issue the abort.
		 * Ignore the completions on the original CCB?
		 */

		UDI_QUEUE_FOREACH(&device_info->active_q, elem, tmp) {
			req = UDI_BASE_STRUCT(elem, dpt_req_t, Q);
			dpt_abort_req(req);
			/* TODO - check if this operation is synchronous.
			 * If not, make sure that a udi_scsi_ctl_ack is
			 * issued only once after all the jobs are aborted
			 */
		}
		/* TODO - issue a udi_scsi_ctl_ack */
		return;

	case UDI_SCSI_CTL_LUN_RESET:
	case UDI_SCSI_CTL_TGT_RESET:
	case UDI_SCSI_CTL_BUS_RESET:
		/* TODO: reset device */
		status = UDI_STAT_NOT_SUPPORTED;
		break;


	case UDI_SCSI_CTL_CLEAR_ACA:
		status = UDI_STAT_NOT_SUPPORTED;
		break;

	case UDI_SCSI_CTL_SET_QUEUE_DEPTH:
		/* TODO -
		 * Do we have re-adjust the resources?
		 */
		device_info->queue_depth = scsi_ctl_cb->queue_depth;
		status = UDI_OK;
		break;
	default:
		/* TODO - log an error */
		status = UDI_STAT_NOT_UNDERSTOOD;
		break;
	}

	/* Complete the control operation */
	udi_scsi_ctl_ack(scsi_ctl_cb, status);
}


/*
 * ====================================================================
 * BEGIN: udi_intr_event_ind operation.
 *	Tasks:
 *	This is the interrupt handler.
 *
 * END: udi_intr_event_ind operation.
 * MESSAGES: 4400 - 4499
 * ========================================================================
 */
STATIC void
dpt_intr_event_ind( udi_intr_event_cb_t *intr_event_cb,
			udi_ubit8_t flags )
{
	dpt_region_data_t *rdata = (dpt_region_data_t *)
					UDI_GCB(intr_event_cb)->context;
	udi_queue_t	*elem, *tmp;
	dpt_ccb_t	*ccb;
#ifdef DPT_DEBUG
	udi_index_t	jobs = 0;
#endif

	/*
	 * Go thru the pending jobs to find out which one completed
	 */
	UDI_QUEUE_FOREACH(&rdata->pending_jobs, elem, tmp ) {
		ccb = UDI_BASE_STRUCT(elem, dpt_ccb_t, link);
/* //TODO: add a udi_dma_sync_here. */
		if (ccb->CP_status.s_ha_status & DPT_HA_STATUS_EOC) {
			/* Found the Job, remove it from the queue */
#ifdef DPT_DEBUG
			jobs++;
#endif
			UDI_QUEUE_REMOVE(&ccb->link);
			/* Clear the EOC bit */
			ccb->CP_status.s_ha_status &= DPT_HA_STATUS_MASK;
			(*ccb->c_callback)(rdata, ccb);
		}
	}
#ifdef DPT_DEBUG
	udi_debug_printf("dpt_intr_event_ind: %d jobs processed\n", jobs);
#endif
	udi_intr_event_rdy(intr_event_cb);
}

STATIC void 
dpt_scsi_unbind_req(udi_scsi_bind_cb_t *scsi_bind_cb) 
{
	dpt_device_info_t *device_info = ((dpt_child_context_t *)
				UDI_GCB(scsi_bind_cb)->context)->device_info;
	dpt_region_data_t	*rdata = device_info->rdata;
	dpt_req_t	*req;
	udi_queue_t	*elem, *tmp;

	/* TODO - Abort all pending requests */

	/* Search the timer queue for any jobs for this device */
	UDI_QUEUE_FOREACH(&rdata->timer_q, elem, tmp) {
		req = UDI_BASE_STRUCT(elem, dpt_req_t, Q);
		if (UDI_GCB(req->scsi_io_cb)->context == device_info) {
			/* Found a job for this device, kill it */
			/* remove the job from the timeout queue */
			dpt_timer_remove(req);
			dpt_abort_req(req);
		}
	}

	/* Free the DMA handles, CCBs and inquiry buffer */

	udi_scsi_unbind_ack(scsi_bind_cb);
}

STATIC void
dpt_scsi_event_res( udi_scsi_event_cb_t *scsi_event_cb)
{
}

STATIC void
dpt_scsi_channel_event_ind(udi_channel_event_cb_t *channel_event_cb)
{
udi_debug_printf("dpt_scsi_channel_event_ind: channel_event_cb %p event=0x%x\n",
	channel_event_cb, channel_event_cb->event);

	switch (channel_event_cb->event) {
	
	case UDI_CHANNEL_CLOSED:
		/* This can happen when PD is killed. */
		break;
	case UDI_CHANNEL_BOUND:
		/* This will never happen. */
		break;
	case UDI_CHANNEL_OP_ABORTED:
		/* This is used for abort */
		break;
	}

	udi_channel_event_complete(channel_event_cb, UDI_OK);
}

STATIC void
dpt_intr_channel_event_ind(udi_channel_event_cb_t *channel_event_cb)
{
	switch (channel_event_cb->event) {
	
	case UDI_CHANNEL_CLOSED:
		/* TODO - close the channel, if not already done by unbind */
		break;
	default:
		/* This should never happen */
		udi_assert(0);
	}

	udi_channel_event_complete(channel_event_cb, UDI_OK);
}

STATIC void
dpt_final_cleanup_req(udi_mgmt_cb_t *mgmt_cb)
{
	/*
	 * We have nothing else to free that wasn't already freed by
	 * unbinding from children and parents.
	 */

	udi_final_cleanup_ack(mgmt_cb);
}

STATIC void
dpt_devmgmt_req(udi_mgmt_cb_t *mgmt_cb, udi_ubit8_t mgmt_op,
		udi_ubit8_t parent_id)
{
	dpt_region_data_t *rdata = UDI_GCB(mgmt_cb)->context;
	udi_status_t	status = UDI_OK;

	switch (mgmt_op) {

	case UDI_DMGMT_PREPARE_TO_SUSPEND:
		status = UDI_STAT_NOT_SUPPORTED; /* Not supported yet */
		break;
	case UDI_DMGMT_SUSPEND:
	case UDI_DMGMT_SHUTDOWN:
		/* Keep the mgmt CB around for use in the ack. */
		rdata->mgmt_cb = UDI_GCB(mgmt_cb);

		dpt_suspend(rdata);
		/* udi_devmgmt_ack will be called when suspend completes */
		return;
	case UDI_DMGMT_PARENT_SUSPENDED:
		status = UDI_STAT_NOT_SUPPORTED; /* Not supported yet */
		break;
	case UDI_DMGMT_RESUME:
		dpt_resume(rdata);
		break;

	case UDI_DMGMT_UNBIND : 

		/* Keep the mgmt CB around for use in the ack. */
		rdata->mgmt_cb = UDI_GCB(mgmt_cb);

		/*
		 * Allocate an interrupt detach control block to communicate
		 * with the interrupt dispatcher driver to detach its
		 * interrupt handler.
		 */
		udi_cb_alloc(dpt_devmgmt_intr_detach,
			UDI_GCB(rdata->bus_bind_cb), INTR_DETACH_CB_IDX,
			UDI_GCB(rdata->bus_bind_cb)->channel);
		return;
	default:
		/* TODO - report an error */
		break;
	}

	udi_devmgmt_ack(mgmt_cb, 0, status);
}

/*
 * ================================================================
 * dpt_devmgmt_intr_detach
 * ================================================================
 */
STATIC void
dpt_devmgmt_intr_detach(udi_cb_t *gcb, udi_cb_t *new_cb)
{
	udi_intr_detach_cb_t *intr_detach_cb =
				UDI_MCB(new_cb, udi_intr_detach_cb_t);

	/* 
	 * spawn index is used by the environment to find the interrupt channel 
	 */
	intr_detach_cb->interrupt_idx = DPT_INTR_SPAWN_IDX;

	udi_intr_detach_req(intr_detach_cb);
}

STATIC void
dpt_intr_detach_ack( udi_intr_detach_cb_t *intr_detach_cb)
{
	dpt_region_data_t *rdata = UDI_GCB(intr_detach_cb)->context;

	/* Free the interrupt attach and detach cbs */

	udi_cb_free(UDI_GCB(intr_detach_cb));
	udi_cb_free(UDI_GCB(rdata->intr_attach_cb));

	/* Close the interrupt channel */

	udi_channel_close(rdata->interrupt_channel);
	rdata->interrupt_channel = UDI_NULL_CHANNEL;

	/*
	 * Issue a unbind request
	 */

	udi_bus_unbind_req(rdata->bus_bind_cb);
	return;
}

STATIC void
dpt_bus_unbind_ack(udi_bus_bind_cb_t *bus_bind_cb)
{
	dpt_region_data_t *rdata = UDI_GCB(bus_bind_cb)->context;
	udi_index_t	i;

	udi_cb_free(UDI_GCB(bus_bind_cb));
	rdata->bus_bind_cb = NULL;

	/* Free the PIO handles */
	for (i = 0; i < dpt_pio_table_len; i++) {
		if (i != DPT_INTR_PIO_HANDLE) {
			udi_pio_unmap(rdata->pio_handle[i]);
			rdata->pio_handle[i] = UDI_NULL_PIO_HANDLE;
		}
	}

	/* Free the scan CCB dma handle and associated resources
	 * (memory, scatter/gather list). */
	udi_dma_free(rdata->scan_req.scan_ccb->c_dma_handle);

	/* Free the inquiry resources */
	udi_dma_free(rdata->scan_req.inq_dma_handle);

	/* Free the EATA space that held config data. */
	udi_mem_free(rdata->eata_info.eata_cfgp);
	rdata->eata_info.eata_cfgp = NULL;

	/* Free the constraints handles. */
	udi_dma_constraints_free(rdata->dma_constraints);
	rdata->dma_constraints = NULL;
	udi_dma_constraints_free(rdata->ccb_constraints);
	rdata->ccb_constraints = NULL;

	/* Complete the devmgmt request */
	udi_devmgmt_ack(UDI_MCB(rdata->mgmt_cb, udi_mgmt_cb_t), 0, UDI_OK);
	rdata->mgmt_cb = NULL; /* Invalidate the cached mgmt cb */
}

/* 
 * ================================================================
 * Internal utility functions
 * ================================================================
 */

STATIC void
dpt_prep_ccb(dpt_ccb_t *cp)
{
	cp->CPaddr.vp = cp;

	/* Build an EATA command packet */

#ifdef OUT
	cp->CP_OpCode   = DPT_CP_DMA_CMD;
#endif
/* //TODO: Add comments to the effect that the ccb is partially filled here.	 */
	if (cp->c_must_swap) {
		/* add the offset of CP_status and CP_sense to the phys addr */
		cp->CPstatDMA = UDI_ENDIAN_SWAP_32(
			cp->c_sg->scgth_elements.el32p[0].block_busaddr +
					offsetof(dpt_ccb_t, CP_status));
		cp->CPsenseDMA = UDI_ENDIAN_SWAP_32(
			cp->c_sg->scgth_elements.el32p[0].block_busaddr +
					offsetof(dpt_ccb_t, CP_sense));
	}
	else {
		cp->CPstatDMA =
			cp->c_sg->scgth_elements.el32p[0].block_busaddr +
					offsetof(dpt_ccb_t, CP_status);
		cp->CPsenseDMA =
			cp->c_sg->scgth_elements.el32p[0].block_busaddr +
					offsetof(dpt_ccb_t, CP_sense);
	}
	cp->ReqLen	= DPT_SENSE_SIZE;
}

STATIC dpt_ccb_t *
dpt_get_ccb(dpt_region_data_t *rdata)
{
	udi_queue_t	*elem;

	if (!UDI_QUEUE_EMPTY(&rdata->ccb_pool)) {
		elem = UDI_DEQUEUE_HEAD(&rdata->ccb_pool);
		return UDI_BASE_STRUCT(elem, dpt_ccb_t, link);
	}
	return NULL;
}

STATIC void
dpt_free_ccb(dpt_region_data_t *rdata, dpt_ccb_t *ccb)
{
	UDI_ENQUEUE_TAIL(&rdata->ccb_pool, &ccb->link);
}

STATIC void
dpt_send_dma_cmd(dpt_region_data_t *rdata, dpt_ccb_t *cp)
{
	dpt_pio_scratch_t *pio_scratch = UDI_GCB(rdata->bus_bind_cb)->scratch;

	/*
	 * stuff the physical address of the scgth list in the scratch space
	 * and use UDI_PIO_SCRATCH pio trans
	 */
	pio_scratch->paddr = cp->c_sg->scgth_elements.el32p[0].block_busaddr;
	pio_scratch->cmd = DPT_CP_DMA_CMD;

/* //TODO: add a udi_dma_sync here. */

	udi_pio_trans(dpt_send_dma_cmd_done,
		UDI_GCB(rdata->bus_bind_cb),
		rdata->pio_handle[DPT_CMD_PIO_HANDLE],
		0,
		NULL,
		NULL);
}

STATIC void
dpt_abort_req(dpt_req_t *req)
{
	dpt_device_info_t *device_info = ((dpt_child_context_t *)
				UDI_GCB(req->scsi_io_cb)->context)->device_info;
	dpt_region_data_t *rdata = device_info->rdata;
	dpt_pio_scratch_t *pio_scratch = UDI_GCB(rdata->bus_bind_cb)->scratch;
	dpt_ccb_t *cp = req->ccb;

#ifdef DPT_DEBUG
	udi_debug_printf("dpt_abort_req: Aborting request -");
	dpt_print_req(req);
#endif

	/*
	 * stuff the physical address of the CCB in the scratch space
	 * and use UDI_PIO_SCRATCH pio trans
	 */
	pio_scratch->paddr = cp->c_sg->scgth_elements.el32p[0].block_busaddr;

	/* TODO: check if we need a udi_dma_sync here. */

	udi_pio_trans(dpt_send_dma_cmd_done,
		UDI_GCB(rdata->bus_bind_cb),
		rdata->pio_handle[DPT_ABORT_PIO_HANDLE],
		0, 
		NULL,
		NULL);
}

STATIC void
dpt_send_dma_cmd_done(udi_cb_t *gcb, udi_buf_t *buf, udi_status_t status,
		udi_ubit16_t result)
{
	/* TODO - Check the status */
}

STATIC void
dpt_suspend(dpt_region_data_t *rdata)
{
	dpt_device_info_t	*device_info;
	udi_queue_t	*elem, *tmp;

	/* First, stop the queues */
	UDI_QUEUE_FOREACH(&rdata->device_list, elem, tmp) {
		device_info = UDI_BASE_STRUCT(elem, dpt_device_info_t, link);
		device_info->d_state |= (DPT_SUSPEND|DPT_FLUSH);
			/* suspend the queue and mark them to be flushed */
	}
	/* Now, flush the queues */
	UDI_QUEUE_FOREACH(&rdata->device_list, elem, tmp) {
		device_info = UDI_BASE_STRUCT(elem, dpt_device_info_t, link);
		dpt_flush(device_info);	/* flush the queues */
	}
}

STATIC void
dpt_resume(dpt_region_data_t *rdata)
{
	dpt_device_info_t	*device_info;
	udi_queue_t	*elem, *tmp;
	dpt_req_t	*req;

	/* Start the queues */
	UDI_QUEUE_FOREACH(&rdata->device_list, elem, tmp) {
		device_info = UDI_BASE_STRUCT(elem, dpt_device_info_t, link);
		device_info->d_state &= ~DPT_SUSPEND;
		/* Restart the I/Os */
		if (!UDI_QUEUE_EMPTY(&device_info->suspend_q)) {
			req = UDI_BASE_STRUCT(
				UDI_DEQUEUE_HEAD(&device_info->suspend_q),
					dpt_req_t, Q);
			dpt_scsi_io_req (req->scsi_io_cb);
		}
	}
}

/*
 * STTAIC void
 * dpt_flush(dpt_device_info_t *device_info)
 *
 * Description:
 * Called by dpt_suspend, the cache on all DPT controllers is flushed
 *
 * Calling/Exit State:
 * None.
 */

STATIC void
dpt_flush(dpt_device_info_t *device_info)
{
	dpt_ccb_t *cp;
	dpt_region_data_t *rdata = device_info->rdata;

	/*
	 * Flush and invalidate cache for this device by issuing a 
	 * SCSI ALLOW MEDIA REMOVAL command.
	 * This will cause the HBA to flush all requests to the device and
	 * then invalidate its cache for that device.
	 */

#ifdef DPT_DEBUG
	udi_debug_printf("udi_dpt: Flushing cache, if present.");
#endif
	if ((cp = dpt_get_ccb(rdata)) == NULL) {
		udi_debug_printf(
			"dpt_flush: Unable to allocate ccb, can't flush cache");
		return;
	}

	cp->c_bind = device_info;
	cp->c_callback = dpt_flush_done;
	cp->c_time = 0;

	/* If this is controller, then we are done */
	if (device_info->tgt_id == rdata->ha_channel_id[device_info->scsi_bus]) {
		dpt_flush_done(rdata, cp);
		return;
	}

	/*
	 * Build the EATA Command Packet structure for
	 * a SCSI Prevent/Allow Cmd
	 */
	cp->CPop = 0;
	cp->CPcdb[0] = SS_LOCK;
	cp->CPcdb[1] = 0;
	cp->CPcdb[2] = 0;
	cp->CPcdb[3] = 0;
	cp->CPcdb[4] = 0;
	cp->CPcdb[5] = 0;
	cp->CPdataDMA = 0L;

	cp->CPbusid = ((udi_ubit8_t)device_info->scsi_bus << 5) |
				device_info->tgt_id;
	cp->CPmsg0  = (DPT_HA_IDENTIFY_MSG | DPT_HA_DISCO_RECO) +
				(udi_ubit8_t)(device_info->lun_id);

	UDI_ENQUEUE_TAIL(&rdata->pending_jobs, &cp->link);

	dpt_send_dma_cmd(rdata, cp);

	/*
	*+ DPT flush completed successfully.
	*/
#ifdef DPT_DEBUG
	udi_debug_printf("udi_dpt: Flushing complete.");
#endif /* DPT_DEBUG */
}

STATIC void
dpt_flush_done(dpt_region_data_t *rdata, dpt_ccb_t *cp)
{
	dpt_device_info_t *device_info = cp->c_bind;
	udi_queue_t *elem, *tmp;
	
	/* TODO - check the status of the command */

	dpt_free_ccb(rdata, cp);
	
	device_info->d_state &= ~DPT_FLUSH;

	/* check if the flush is complete */
	UDI_QUEUE_FOREACH(&rdata->device_list, elem, tmp) {
		device_info = UDI_BASE_STRUCT(elem, dpt_device_info_t, link);
		if (device_info->d_state & DPT_FLUSH)
			return;
	}

	/* Complete the devmgmt request */
	udi_devmgmt_ack(UDI_MCB(rdata->mgmt_cb, udi_mgmt_cb_t), 0, UDI_OK);
	rdata->mgmt_cb = NULL; /* Invalidate the cached mgmt cb */
	return;
}

/* 
 * ================================================================
 * Timer Service.
 *
 * The following code implements a software timeout mechanism using
 * UDI timer services. This code is needed only for those HBA controllers
 * which don't support hardware timeout mechanism.
 * ================================================================
 */

STATIC void
dpt_timer_insert(dpt_req_t *req)
{
	dpt_device_info_t *device_info = ((dpt_child_context_t *)
				UDI_GCB(req->scsi_io_cb)->context)->device_info;
	dpt_region_data_t *rdata = device_info->rdata;
	udi_time_t	interval;

	if (req->scsi_io_cb->timeout == 0) {
		/* timeout value not specified, put it on a non-timer queue */
		UDI_ENQUEUE_TAIL(&device_info->active_q, &req->Q);
		return;
	}

	/* timeout value is specified, put the request on a timer queue
	 * and start the timer for this request.
	 * Note that the timeout value is in milliseconds.
	 */
	interval.seconds = req->scsi_io_cb->timeout / 1000;
	interval.nanoseconds = (req->scsi_io_cb->timeout % 1000) * 1000 * 1000;
	udi_timer_start(dpt_timer_callback, UDI_GCB(req->scsi_io_cb), interval);

	UDI_ENQUEUE_TAIL(&rdata->timer_q, &req->Q);
}

STATIC void
dpt_timer_remove(dpt_req_t *req)
{
	/* Stop the timer if needed and remove the job from the
	 * appropriate queue */
	if (req->scsi_io_cb->timeout != 0) {
		udi_timer_cancel(UDI_GCB(req->scsi_io_cb));
		req->scsi_io_cb->timeout = 0;
	}
	UDI_QUEUE_REMOVE(&req->Q);
}

/*
 * ====================================================================
 * dpt_timer_callback
 * checks for timeouts.
 * MESSAGES: 4500 - 4599
 * ====================================================================
 */
STATIC void
dpt_timer_callback(udi_cb_t *gcb)
{
	dpt_region_data_t *rdata = gcb->context;
	dpt_req_t	*req = gcb->scratch;

	if (UDI_QUEUE_EMPTY(&rdata->timer_q)) {
		/* nothing to do, stop the timer and return */
		return;
	}	
#ifdef DPT_DEBUG
	udi_debug_printf("dpt_timer_callback: Job timed out -");
	dpt_print_req(req);
#endif
	/* timer expired, kill this job */
	UDI_QUEUE_REMOVE(&req->Q); /* remove the job from the timeout queue */
	dpt_abort_req(req);
}

STATIC void
dpt_print_rdata_reqs(dpt_region_data_t *rdata)
{
	udi_queue_t *device_info, *tmp;

	UDI_QUEUE_FOREACH(&rdata->device_list, device_info, tmp) {
		udi_debug_printf("Requests for device %p\n", device_info);
		dpt_print_device_reqs((dpt_device_info_t *)device_info);
	}
}

STATIC void
dpt_print_device_reqs(dpt_device_info_t *device_info)
{
	dpt_region_data_t	*rdata = device_info->rdata;
	dpt_req_t       *req;
	dpt_device_info_t *dinfo;
	udi_queue_t     *elem, *tmp;
	udi_ubit32_t	num_reqs;

	/* Search the alloc queue for any reqs for this device */
	num_reqs = 0;
	UDI_QUEUE_FOREACH(&rdata->alloc_q, elem, tmp) {
		req = UDI_BASE_STRUCT(elem, dpt_req_t, Q);
		dinfo = ((dpt_child_context_t *)
				UDI_GCB(req->scsi_io_cb)->context)->device_info;
		if (dinfo == device_info) {
			/* Found a req */
			dpt_print_req(req);
			num_reqs++;
		}
	}
	udi_debug_printf("Found %d reqs on rdata->alloc_q for this device\n",
			num_reqs);

	/* Look for reqs on tag queue */
	num_reqs = 0;
	UDI_QUEUE_FOREACH(&device_info->tag_q, elem, tmp) {
		req = UDI_BASE_STRUCT(elem, dpt_req_t, Q);
		/* Found a req */
		dpt_print_req(req);
		num_reqs++;
	}
	udi_debug_printf("Found %d reqs on device_info->tag_q for this device\n", num_reqs);

	/* Search the timer queue for any reqs for this device */
	num_reqs = 0;
	UDI_QUEUE_FOREACH(&rdata->timer_q, elem, tmp) {
		req = UDI_BASE_STRUCT(elem, dpt_req_t, Q);
		dinfo = ((dpt_child_context_t *)
				UDI_GCB(req->scsi_io_cb)->context)->device_info;
		if (dinfo == device_info) {
			/* Found a req */
			dpt_print_req(req);
			num_reqs++;
		}
	}
	udi_debug_printf("Found %d reqs on rdata->timer_q for this device\n",
			num_reqs);


	/* Search the active queue for any reqs for this device */
	num_reqs = 0;
	UDI_QUEUE_FOREACH(&device_info->active_q, elem, tmp) {
		req = UDI_BASE_STRUCT(elem, dpt_req_t, Q);
		/* Found a req */
		dpt_print_req(req);
		num_reqs++;
	}
	udi_debug_printf("Found %d reqs on device_info->active_q for this device\n",
		num_reqs);
}

STATIC void
dpt_print_req(dpt_req_t *req)
{
	udi_debug_printf("req=%p, scsi_io_cb=%p\n", req, req->scsi_io_cb);
}

STATIC void
dpt_print_active_ccbs(dpt_region_data_t *rdata)
{
	udi_queue_t *elem, *tmp;
	dpt_ccb_t *ccb;

	/* Go thru the pending jobs */
	UDI_QUEUE_FOREACH(&rdata->pending_jobs, elem, tmp ) {
		ccb = UDI_BASE_STRUCT(elem, dpt_ccb_t, link);
		dpt_print_ccb(ccb);
	}
}

STATIC void
dpt_print_ccb(dpt_ccb_t *ccb)
{
	udi_ubit32_t	i;

	udi_debug_printf("ccb=%p op=%02X ReqLen=%02X dataLen=%X busid=%02X ",
		ccb, ccb->CPop, ccb->ReqLen, ccb->CPdataLen, ccb->CPbusid);
	udi_debug_printf("c_bind=%p\n", ccb->c_bind);
	udi_debug_printf("status=%p dataDMA=%X statDMA=%X senseDMA=%X\n",
		&ccb->CP_status, ccb->CPdataDMA, ccb->CPstatDMA,
		ccb->CPsenseDMA);
	udi_debug_printf("cdb=");
	for (i = 0; i < 12; i++)
	 	udi_debug_printf("%02X", ccb->CPcdb[i]);
	udi_debug_printf("\n");
}

/* Since DPT is a big-endian device, must_swap should be set to 0 (means
 * no swapping needed) on big endian systems and should be set to 1 on
 * little endian systems.
 */
STATIC void
dpt_print_eata_cfg(dpt_eata_cfg_t *eata_cfg, udi_boolean_t must_swap)
{
	udi_debug_printf("EATAsignature=0x%x, EATAversion=0x%02X, capabilities=0x%02X\n",
			*(udi_ubit32_t *)eata_cfg->EATAsignature,
			eata_cfg->EATAversion, eata_cfg->capabilities);
	udi_debug_printf("CPlength=0x%x, SPlength=0x%x, QueueSize=0x%x, SG_Size=0x%x\n",
			must_swap ? UDI_ENDIAN_SWAP_32(
					*(udi_ubit32_t *)eata_cfg->CPlength) :
				*(udi_ubit32_t *)eata_cfg->CPlength,
			must_swap ? UDI_ENDIAN_SWAP_32(
					*(udi_ubit32_t *)eata_cfg->SPlength) :
				*(udi_ubit32_t *)eata_cfg->SPlength,
			must_swap ? UDI_ENDIAN_SWAP_16(
					*(udi_ubit16_t *)eata_cfg->QueueSize) :
				*(udi_ubit16_t *)eata_cfg->QueueSize,
			must_swap ? UDI_ENDIAN_SWAP_16(
					*(udi_ubit16_t *)eata_cfg->SG_Size) :
				*(udi_ubit16_t *)eata_cfg->SG_Size);
	udi_debug_printf("max_target=0x%02X, max_chan=0x%02X, max_lun=0x%02X\n",
			eata_cfg->max_chan_tgt & 0x1F,
			eata_cfg->max_chan_tgt >> 5, eata_cfg->max_lun);

}
