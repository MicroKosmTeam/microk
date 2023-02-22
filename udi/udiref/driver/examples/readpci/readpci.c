
/*
 * File: udi_cmos/udi_cmos.c
 *
 * UDI CMOS RAM Sample Driver
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


/*
 * ====================================================================
 * Note: This driver does not actually use interrupts, but if it did,
 *	 the code would look something like the DO_INTERRUPTS
 *	 conditional code.
 */

/*
 * We must define these version symbols before including the header
 * files, to indicate the interface versions we're using.
 */
#define UDI_VERSION 0x101
#define UDI_PHYSIO_VERSION 0x101

#include <udi.h>	/* Standard UDI Definitions */
#include <udi_physio.h>	/* Physical I/O Extensions */

/*
 * PCI Configuration space header.   Common to all PCI devices.
 */
typedef struct {
	udi_ubit16_t pcicfg_vendor_id;
	udi_ubit16_t pcicfg_device_id;
	udi_ubit16_t pcicfg_command;
	udi_ubit16_t pcicfg_status;
	udi_ubit8_t  pcicfg_revision_id;
	udi_ubit8_t  pcicfg_prog_if;
	udi_ubit8_t  pcicfg_subclass;
	udi_ubit8_t  pcicfg_class;
	udi_ubit8_t pcicfg_cache_line_size;
	udi_ubit8_t pcicfg_latency_timer;
	udi_ubit8_t pcicfg_header_type;
	udi_ubit8_t pcicfg_bist;
	udi_ubit32_t pcicfg_bar[6];
} pcicfg_t;

typedef struct {
	udi_ubit32_t vendor_id;
	udi_ubit32_t device_id;
	udi_ubit32_t revision_id;
	udi_ubit32_t class_code;
	udi_ubit32_t subclass_code;
	udi_ubit32_t status;
	udi_ubit32_t bar[6];
} my_pci_t;

/*
 * Region data structure for this driver.
 */
typedef struct {
	/* init_context is filled in by the environment: */
        udi_init_context_t	init_context;
	udi_bus_bind_cb_t	*bus_bind_cb;
	udi_pio_handle_t	trans_read;
	udi_pio_handle_t	le_dump_handle;
	udi_pio_handle_t	be_dump_handle;
	udi_boolean_t		dump_be;
	unsigned char 		*dump_buffer;
	udi_ubit32_t		size;
	my_pci_t 		*my_pci;
} cmos_region_data_t;


/*
 * CB indexes for each of the types of control blocks used.
 */
#define BUS_BIND_CB_IDX		4

/*
 * Scratch sizes for each control block type.
 */
#define BUS_BIND_SCRATCH	0

/*
 * Ops indexes for each of the entry point ops vectors used.
 */
#define BUS_DEVICE_OPS_IDX	2

/*
 * Driver entry points for the Management Metalanguage.
 */
static udi_devmgmt_req_op_t cmos_devmgmt_req;
static udi_final_cleanup_req_op_t cmos_final_cleanup_req;

static udi_mgmt_ops_t cmos_mgmt_ops = {
	udi_static_usage,
	udi_enumerate_no_children,
	cmos_devmgmt_req,
	cmos_final_cleanup_req
};

/*
 * Driver "bottom-side" entry points for the Bus Bridge Metalanguage.
 */

static udi_channel_event_ind_op_t cmos_parent_channel_event;
static udi_bus_bind_ack_op_t cmos_bus_bind_ack;
static udi_bus_unbind_ack_op_t cmos_bus_unbind_ack;

static udi_bus_device_ops_t cmos_bus_device_ops = {
	cmos_parent_channel_event,
	cmos_bus_bind_ack,
	cmos_bus_unbind_ack,
	udi_intr_attach_ack_unused,
	udi_intr_detach_ack_unused
};

/*
 * default op_flags without any special flags set
 */
static udi_ubit8_t cmos_default_op_flags[] = {
	0, 0, 0, 0, 0
};

/*
 * PIO properties of the physical I/O registers on the CMOS device.
 */
#define CMOS_REGSET	2	/* first (and only) register set */
#define CMOS_BASE	0xe2000000	/* base address within this regset */
#define CMOS_LENGTH	256	/* two bytes worth of registers */

#define DUMP_REGSET	2
#define DUMP_BASE	0xe2000000
#define DUMP_LENGTH	64

/*
 * PIO offsets for various device registers, relative to the base of
 * the device's register set.
 */
#define CMOS_ADDR	0	/* address register */
#define CMOS_DATA	1	/* data register */

/*
 * Other device properties.
 */
#define CMOS_DEVSIZE	0x40	/* # data bytes supported by device */
#define CMOS_RDONLY_SZ	0x0E	/* first 14 bytes are read-only */

/*
 * PIO trans lists for access to whatever device we map to.
 */
static udi_pio_trans_t readpci_trans_read[] = {
/* { UDI_PIO_DEBUG, UDI_PIO_2BYTE, 0xffff}, */

	/* Read vendor ID */
	{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_2BYTE, offsetof(pcicfg_t, pcicfg_vendor_id) },
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, offsetof(my_pci_t, vendor_id) },
	{ UDI_PIO_STORE+UDI_PIO_MEM+UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0 }, 

	/* Read device ID */
	{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_2BYTE, offsetof(pcicfg_t, pcicfg_device_id) },
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, offsetof(my_pci_t, device_id) },
	{ UDI_PIO_STORE+UDI_PIO_MEM+UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0 }, 

	/* Read status */
	{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_2BYTE, offsetof(pcicfg_t, pcicfg_status) },
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, offsetof(my_pci_t, status) },
	{ UDI_PIO_STORE+UDI_PIO_MEM+UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0 }, 

	/* Read class */
	{ UDI_PIO_IN+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_1BYTE, offsetof(pcicfg_t, pcicfg_class) },
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, offsetof(my_pci_t, class_code) },
	{ UDI_PIO_STORE+UDI_PIO_MEM+UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0 }, 

	/* Loop through the BARs, knowing they're contig on both sides */
        /* mem register base */
        { UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, offsetof(my_pci_t,bar)},
        /* pio register base */
        { UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, offsetof(pcicfg_t,pcicfg_bar)},
        /* R2 = Count */
        { UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, 6},

        { UDI_PIO_REP_IN_IND, UDI_PIO_4BYTE,
                UDI_PIO_REP_ARGS(UDI_PIO_MEM,
                        UDI_PIO_R0, 1,
                        UDI_PIO_R1, 1,
                        UDI_PIO_R2)},



	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 },
};

static udi_pio_trans_t dumper_list[] = {
/*	{ UDI_PIO_LABEL, 0, UDI_PIO_1BYTE}, */

        /* R0 = mem register base */
        { UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0 },
        /* R1 = pio register base */
        { UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0 },
        /* R2 = Count */
        { UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, DUMP_LENGTH},

        { UDI_PIO_REP_IN_IND, UDI_PIO_1BYTE,
                UDI_PIO_REP_ARGS(UDI_PIO_MEM,
                        UDI_PIO_R0, 1,
                        UDI_PIO_R1, 1,
                        UDI_PIO_R2)},

	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 },

	{ UDI_PIO_LABEL, 0, UDI_PIO_2BYTE},
        /* R0 = mem register base */
        { UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0 },
        /* R1 = pio register base */
        { UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0 },
        /* R2 = Count */
        { UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, DUMP_LENGTH/2},

        { UDI_PIO_REP_IN_IND, UDI_PIO_2BYTE,
                UDI_PIO_REP_ARGS(UDI_PIO_MEM,
                        UDI_PIO_R0, 1,
                        UDI_PIO_R1, 1,
                        UDI_PIO_R2)},
	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 },

	{ UDI_PIO_LABEL, 0, UDI_PIO_4BYTE},
        /* R0 = mem register base */
        { UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0 },
        /* R1 = pio register base */
        { UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0 },
        /* R2 = Count */
        { UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, DUMP_LENGTH/4},

        { UDI_PIO_REP_IN_IND, UDI_PIO_4BYTE,
                UDI_PIO_REP_ARGS(UDI_PIO_MEM,
                        UDI_PIO_R0, 1,
                        UDI_PIO_R1, 1,
                        UDI_PIO_R2)},
	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 },
};

/*
 * --------------------------------------------------------------------
 * Management operations init section:
 * --------------------------------------------------------------------
 */
static udi_primary_init_t udi_cmos_primary_init_info = {
	&cmos_mgmt_ops,
	cmos_default_op_flags,	/* op_flags */
	0,			/* mgmt_scratch_size */
	0,                      /* enumerate attr list length */
	sizeof(cmos_region_data_t), /* rdata size */
	0,			/* no children, so no enumeration */
	0,		/* buf path*/
};

/*
 * --------------------------------------------------------------------
 * Meta operations init section:
 * --------------------------------------------------------------------
 */
static udi_ops_init_t udi_cmos_ops_init_list[] = {
	{
		BUS_DEVICE_OPS_IDX,
		CMOS_BRIDGE_META,	/* meta index
					 * [from udiprops.txt] */
		UDI_BUS_DEVICE_OPS_NUM,
		0,			/* no channel context */
		(udi_ops_vector_t *)&cmos_bus_device_ops,
		cmos_default_op_flags	/* op_flags */
	},
	{
		0	/* Terminator */
	}
};

/*
 * --------------------------------------------------------------------
 * Control Block init section:
 * --------------------------------------------------------------------
 */
static udi_cb_init_t udi_cmos_cb_init_list[] = {
	{
		BUS_BIND_CB_IDX,	/* Bridge Bind CB	*/
		CMOS_BRIDGE_META,	/* from udiprops.txt	*/
		UDI_BUS_BIND_CB_NUM,	/* meta cb_num		*/
		BUS_BIND_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
	{
		0			/* Terminator */
	}
};

udi_init_t udi_init_info = {
	&udi_cmos_primary_init_info,
	NULL,				/* secondary_init_list */
	udi_cmos_ops_init_list,
	udi_cmos_cb_init_list,
	NULL,				/* gcb_init_list */
	NULL				/* cb_select_list */
};

/*
 * --------------------------------------------------------------------
 *
 * --------------------------------------------------------------------
 */

static void
cmos_parent_channel_event(udi_channel_event_cb_t *channel_event_cb)
{
	udi_bus_bind_cb_t *bus_bind_cb;
	switch (channel_event_cb->event) {
	case UDI_CHANNEL_BOUND:

		bus_bind_cb = UDI_MCB(channel_event_cb->
			params.parent_bound.bind_cb, udi_bus_bind_cb_t);

		/* Keep a link back to the channel event CB for the ack. */
		UDI_GCB(bus_bind_cb)->initiator_context = channel_event_cb;

		/* Bind to the parent bus bridge driver. */
		udi_bus_bind_req(bus_bind_cb);
		break;
	default:
		udi_channel_event_complete(channel_event_cb, UDI_OK);
	}
}


static udi_pio_map_call_t cmos_bus_bind_ack_1;
static udi_pio_map_call_t got_le_dump_mapping;
static udi_pio_map_call_t got_be_dump_mapping;
static udi_mem_alloc_call_t cmos_bus_bind_ack_2;
static udi_mem_alloc_call_t got_dump_buffer;

static void
cmos_bus_bind_ack(
	udi_bus_bind_cb_t *bus_bind_cb,
	udi_dma_constraints_t constraints,
	udi_ubit8_t preferred_endianness,
	udi_status_t status)
{
	cmos_region_data_t *rdata = bus_bind_cb->gcb.context;
	udi_channel_event_cb_t *channel_event_cb =
					bus_bind_cb->gcb.initiator_context;

	/*
	 * Don't need to do anything with preferred_endianness, since
	 * our device doesn't do DMA. Even if it did,
	 * preferred_endianness is only used with bi-endianness devices
	 * that can change endianness.
	 */
	udi_dma_constraints_free(constraints);

	/*
	 * Save the bus bind control block for unbinding later.
	 */
	rdata->bus_bind_cb = bus_bind_cb;

	if (status != UDI_OK) {
		udi_channel_event_complete( channel_event_cb,
						UDI_STAT_CANNOT_BIND);
		return;
	}

	/*
	 * Now we have access to our hardware. Set up the PIO mappings
	 * we'll need later.
	 */
	udi_pio_map(cmos_bus_bind_ack_1, UDI_GCB(bus_bind_cb),
		    CMOS_REGSET, CMOS_BASE, CMOS_LENGTH,
		    readpci_trans_read,
		    sizeof(readpci_trans_read) / sizeof(udi_pio_trans_t),
		    UDI_PIO_LITTLE_ENDIAN, 0, 0);
}

static void
cmos_bus_bind_ack_1(
	udi_cb_t *gcb,
	udi_pio_handle_t new_pio_handle)
{
	cmos_region_data_t *rdata = gcb->context;

	/* Save the PIO handle for later use. */
	rdata->trans_read = new_pio_handle;

	udi_mem_alloc(cmos_bus_bind_ack_2, gcb, sizeof(*rdata->my_pci), 
			UDI_MEM_MOVABLE);
}

static udi_pio_trans_call_t cmos_trans_done;
static udi_pio_trans_call_t le_trans_done;
static udi_pio_trans_call_t be_trans_done;

static void
cmos_bus_bind_ack_2(
	udi_cb_t *gcb,
	void *new_mem)
{
	cmos_region_data_t *rdata = gcb->context;
	rdata->my_pci = new_mem;

	udi_pio_trans(cmos_trans_done,
		gcb,
		rdata->trans_read,
		0,
		NULL,
		rdata->my_pci);
}

static void
cmos_trans_done(
	udi_cb_t *gcb,
	udi_buf_t *buf,
	udi_status_t status,
	udi_ubit16_t result)
{
	cmos_region_data_t *rdata = gcb->context;
/* 	udi_channel_event_cb_t *channel_event_cb = gcb->initiator_context; */
	my_pci_t *my_pci = rdata->my_pci;
	int i;


	udi_debug_printf("Vendor %04x, Device %04x\n", 
			   my_pci->vendor_id, my_pci->device_id);
	udi_debug_printf("%<0-7=CLASS:0=Ancient:1=Mass storage:2=Network controller:3=Display controller:4=Multimedia device:5=Memory controller:6=Bridge device:7=Simple comm controller:8=Base system:9=Input device:10=Docking stations11=Processor:12=Serial bus controller>\n",my_pci->class_code);
	udi_debug_printf("%<10-9=DEVSEL:0=fast:1=medium:2=slow:3=reserved,8=Data Parity Reported,7=Fast back-to-back capable,5=66Mhz capable>\n", my_pci->status);
	for (i=0;i<6;i++) {
		udi_ubit32_t bar = my_pci->bar[i];
		if (bar == 0) {
			continue;
		}
		if (bar & 1) {
			udi_debug_printf("I/O at 0x%x [0x%08x].\n", bar & ~0x1f, bar);
		} else {
			udi_debug_printf("%<3=Prefetchable,~3=Non-prefetchable,1-2=Type:0=32 bit:1=20 bit:2=64 bit:3=Reserved> at 0x%08x [0x%08x].\n", bar, bar & ~0x1f, bar);
		}
	}

	udi_pio_map(got_le_dump_mapping, gcb,
		    DUMP_REGSET, DUMP_BASE, DUMP_LENGTH,
		    dumper_list,
		    sizeof(dumper_list) / sizeof(udi_pio_trans_t),
		    UDI_PIO_LITTLE_ENDIAN, 0, 0);
}

static void
got_le_dump_mapping(
	udi_cb_t *gcb,
	udi_pio_handle_t new_pio_handle)
{
	cmos_region_data_t *rdata = gcb->context;

	/* Save the PIO handle for later use. */
	rdata->le_dump_handle = new_pio_handle;

	udi_pio_map(got_be_dump_mapping, gcb,
		    DUMP_REGSET, DUMP_BASE, DUMP_LENGTH,
		    dumper_list,
		    sizeof(dumper_list) / sizeof(udi_pio_trans_t),
		    UDI_PIO_BIG_ENDIAN, 0, 0);
}

static void
got_be_dump_mapping(
	udi_cb_t *gcb,
	udi_pio_handle_t new_pio_handle)
{
	cmos_region_data_t *rdata = gcb->context;

	/* Save the PIO handle for later use. */
	rdata->be_dump_handle = new_pio_handle;

	udi_mem_alloc(got_dump_buffer, gcb, DUMP_LENGTH, UDI_MEM_MOVABLE);
}

static void
got_dump_buffer(
	udi_cb_t *gcb,
	void *new_mem)
{
	cmos_region_data_t *rdata = gcb->context;
	rdata->dump_buffer = new_mem;
	
	udi_pio_trans(le_trans_done,
		gcb,
		rdata->le_dump_handle,
		rdata->size,
		NULL,
		rdata->dump_buffer);
}

static void
le_trans_done(
	udi_cb_t *gcb,
	udi_buf_t *buf,
	udi_status_t status,
	udi_ubit16_t result)
{
	cmos_region_data_t *rdata = gcb->context; 
	udi_channel_event_cb_t *channel_event_cb = gcb->initiator_context;
	int i;

	switch(rdata->size) {
	case UDI_PIO_1BYTE:
		for (i=0;i<DUMP_LENGTH;i++) {
			if (i % 16 == 0)
				udi_debug_printf("\n%04x: ", i);
			udi_debug_printf("%02x ",  
				*(udi_ubit8_t*)&rdata->dump_buffer[i]);
		}
		udi_debug_printf("\n");
		break;
	case UDI_PIO_2BYTE:
		for (i=0;i<DUMP_LENGTH;i+=2) {
			if (i % 16 == 0)
				udi_debug_printf("\n%04x: ", i);
			udi_debug_printf("%04x  ",  
				*(udi_ubit16_t*)&rdata->dump_buffer[i]);
		}
		udi_debug_printf("\n");
		break;
	case UDI_PIO_4BYTE:
		for (i=0;i<DUMP_LENGTH;i+=4) {
			if (i % 16 == 0)
				udi_debug_printf("\n%04x: ", i);
			udi_debug_printf("%08x    ",
				*(udi_ubit32_t*)&rdata->dump_buffer[i]);
		}
		udi_debug_printf("\n");
		break;
	default:
		break;
	}

	rdata->size++;

	if (rdata->size >= UDI_PIO_8BYTE) {
		if (rdata->dump_be) {
			/* Let the MA know we've completed binding. */
			udi_channel_event_complete(channel_event_cb, UDI_OK); 
			return;
		} else {
			rdata->size = 0;
			udi_debug_printf("Big Endian\n");
			rdata->dump_be = TRUE;
		}
		
	}
	udi_memset(rdata->dump_buffer, 0, DUMP_LENGTH);
	udi_pio_trans(le_trans_done,
		gcb,
		rdata->dump_be ? rdata->be_dump_handle : rdata->le_dump_handle,
		rdata->size,
		NULL,
		rdata->dump_buffer);
}

static void
cmos_devmgmt_req(
	udi_mgmt_cb_t *cb,
	udi_ubit8_t mgmt_op,
	udi_ubit8_t parent_id)
{
	cmos_region_data_t *rdata = cb->gcb.context;

	switch (mgmt_op) {
	case UDI_DMGMT_UNBIND:
		/* Keep a link back to this CB for use in the ack. */
		rdata->bus_bind_cb->gcb.initiator_context = cb;

		/* Do the metalanguage-specific unbind. */
		udi_bus_unbind_req(rdata->bus_bind_cb);
		break;
	default:
		udi_devmgmt_ack(cb, 0, UDI_OK);
	}
}

static void
cmos_bus_unbind_ack(udi_bus_bind_cb_t *bus_bind_cb)
{
	udi_mgmt_cb_t *cb = bus_bind_cb->gcb.initiator_context;
	cmos_region_data_t *rdata = UDI_GCB(bus_bind_cb)->context;

	udi_pio_unmap(rdata->trans_read);
	udi_mem_free(rdata->my_pci);

	udi_cb_free(UDI_GCB(bus_bind_cb));

	udi_devmgmt_ack(cb, 0, UDI_OK);
}

static void
cmos_final_cleanup_req(udi_mgmt_cb_t *cb)
{
	/*
	 * We have nothing to free that wasn't already freed by
	 * unbinding children and parents.
	 */
	udi_final_cleanup_ack(cb);
}
