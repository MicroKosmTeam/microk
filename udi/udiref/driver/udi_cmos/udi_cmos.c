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
 * Region data structure for this driver.
 */
typedef struct {
	/* init_context is filled in by the environment: */
        udi_init_context_t	init_context;
	udi_bus_bind_cb_t	*bus_bind_cb;
	/* PIO trans handles for reading and writing, respectively: */
	udi_pio_handle_t	trans_read;
	udi_pio_handle_t	trans_write;
} cmos_region_data_t;

/*
 * Scratch structures for various control block types.
 */
typedef struct {
	udi_ubit8_t	addr;		/* CMOS device address */
	udi_ubit8_t	count;		/* # bytes to transfer */
} cmos_gio_xfer_scratch_t;

/*
 * GIO transfer scratch offsets for PIO trans list processing.
 */
#define SCRATCH_ADDR		offsetof(cmos_gio_xfer_scratch_t, addr)
#define SCRATCH_COUNT		offsetof(cmos_gio_xfer_scratch_t, count)

/*
 * CB indexes for each of the types of control blocks used.
 */
#define GIO_BIND_CB_IDX		1
#define GIO_XFER_CB_IDX		2
#define GIO_EVENT_CB_IDX	3
#define BUS_BIND_CB_IDX		4
#if DO_INTERRUPTS
#define INTR_ATTACH_CB_IDX	5
#define INTR_EVENT_CB_IDX	6
#endif

/*
 * Scratch sizes for each control block type.
 */
#define GIO_BIND_SCRATCH	0
#define GIO_XFER_SCRATCH	sizeof(cmos_gio_xfer_scratch_t)
#define GIO_EVENT_SCRATCH	0
#define BUS_BIND_SCRATCH	0
#if DO_INTERRUPTS
#define INTR_ATTACH_SCRATCH	0
#define INTR_EVENT_SCRATCH	0
#endif

/*
 * Ops indexes for each of the entry point ops vectors used.
 */
#define GIO_OPS_IDX		1
#define BUS_DEVICE_OPS_IDX	2
#if DO_INTERRUPTS
#define INTR_HANDLER_OPS_IDX	3
#endif

/*
 * Driver entry points for the Management Metalanguage.
 */
static udi_enumerate_req_op_t cmos_enumerate_req;
static udi_devmgmt_req_op_t cmos_devmgmt_req;
static udi_final_cleanup_req_op_t cmos_final_cleanup_req;

static udi_mgmt_ops_t cmos_mgmt_ops = {
	udi_static_usage,
	cmos_enumerate_req,
	cmos_devmgmt_req,
	cmos_final_cleanup_req
};

/*
 * Driver "top-side" entry points for the GIO Metalanguage.
 */

static udi_channel_event_ind_op_t cmos_child_channel_event;
static udi_gio_bind_req_op_t cmos_gio_bind_req;
static udi_gio_unbind_req_op_t cmos_gio_unbind_req;
static udi_gio_xfer_req_op_t cmos_gio_xfer_req;

static udi_gio_provider_ops_t cmos_gio_provider_ops = {
	cmos_child_channel_event,
	cmos_gio_bind_req,
	cmos_gio_unbind_req,
	cmos_gio_xfer_req,
	udi_gio_event_res_unused
};

/*
 * Driver "bottom-side" entry points for the Bus Bridge Metalanguage.
 */

static udi_channel_event_ind_op_t cmos_parent_channel_event;
static udi_bus_bind_ack_op_t cmos_bus_bind_ack;
static udi_bus_unbind_ack_op_t cmos_bus_unbind_ack;
#if DO_INTERRUPTS
static udi_intr_attach_ack_op_t cmos_intr_attach_ack;
static udi_intr_detach_ack_op_t cmos_intr_detach_ack;
#endif

static udi_bus_device_ops_t cmos_bus_device_ops = {
	cmos_parent_channel_event,
	cmos_bus_bind_ack,
	cmos_bus_unbind_ack,
#if DO_INTERRUPTS
	cmos_intr_attach_ack,
	cmos_intr_detach_ack
#else
	udi_intr_attach_ack_unused,
	udi_intr_detach_ack_unused
#endif
};

#if DO_INTERRUPTS
static udi_channel_event_ind_op_t cmos_intr_channel_event;
static udi_intr_event_ind_op_t cmos_intr_event_ind;

static udi_intr_handler_ops_t cmos_intr_handler_ops = {
	cmos_intr_channel_event,
	cmos_intr_event_ind
};
#endif

static const udi_ubit8_t cmos_default_op_flags[5] = { 0, };

/*
 * PIO properties of the physical I/O registers on the CMOS device.
 */
#define CMOS_REGSET	1	/* first (and only) register set */
#define CMOS_BASE	0	/* base address within this regset */
#define CMOS_LENGTH	2	/* two bytes worth of registers */
#define CMOS_PACE	5	/* wait 5 microseconds between accesses
				 * (not really needed for this device,
				 * but illustrates use of pacing) */

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
 * PIO trans lists for access to the CMOS device.
 */
static udi_pio_trans_t cmos_trans_read[] = {
	/* R0 <- SCRATCH_ADDR {offset into scratch of address} */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, SCRATCH_ADDR },
	/* R1 <- address */
	{ UDI_PIO_LOAD+UDI_PIO_SCRATCH+UDI_PIO_R0,
				UDI_PIO_1BYTE, UDI_PIO_R1 },
	/* R0 <- SCRATCH_COUNT {offset into scratch of count} */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, SCRATCH_COUNT },
	/* R2 <- count */
	{ UDI_PIO_LOAD+UDI_PIO_SCRATCH+UDI_PIO_R0,
				UDI_PIO_1BYTE, UDI_PIO_R2 },
	/* R0 <- 0 {current buffer offset} */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 0 },
	/* begin main loop */
	{ UDI_PIO_LABEL, 0, 1 },
	/* output address from R1 to address register */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1,
				UDI_PIO_1BYTE, CMOS_ADDR },
	/* input value from data register into next buffer location */
	{ UDI_PIO_IN+UDI_PIO_BUF+UDI_PIO_R0,
				UDI_PIO_1BYTE, CMOS_DATA },
	/* decrement count (in R2) */
	{ UDI_PIO_ADD_IMM+UDI_PIO_R2, UDI_PIO_1BYTE, (udi_ubit8_t)-1 },
	/* if count is zero, we're done */
	{ UDI_PIO_CSKIP+UDI_PIO_R2, UDI_PIO_1BYTE, UDI_PIO_NZ },
	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 },
	/* increment address and buffer offset */
	{ UDI_PIO_ADD_IMM+UDI_PIO_R1, UDI_PIO_1BYTE, 1 },
	{ UDI_PIO_ADD_IMM+UDI_PIO_R0, UDI_PIO_1BYTE, 1 },
	/* go back to main loop */
	{ UDI_PIO_BRANCH, 0, 1 }
};
static udi_pio_trans_t cmos_trans_write[] = {
	/* R0 <- SCRATCH_ADDR {offset into scratch of address} */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, SCRATCH_ADDR },
	/* R1 <- address */
	{ UDI_PIO_LOAD+UDI_PIO_SCRATCH+UDI_PIO_R0,
				UDI_PIO_1BYTE, UDI_PIO_R1 },
	/* R0 <- SCRATCH_COUNT {offset into scratch of count} */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, SCRATCH_COUNT },
	/* R2 <- count */
	{ UDI_PIO_LOAD+UDI_PIO_SCRATCH+UDI_PIO_R0,
				UDI_PIO_1BYTE, UDI_PIO_R2 },
	/* R0 <- 0 {current buffer offset} */
	{ UDI_PIO_LOAD_IMM+UDI_PIO_R0, UDI_PIO_2BYTE, 0 },
	/* begin main loop */
	{ UDI_PIO_LABEL, 0, 1 },
	/* output address from R1 to address register */
	{ UDI_PIO_OUT+UDI_PIO_DIRECT+UDI_PIO_R1,
				UDI_PIO_1BYTE, CMOS_ADDR },
	/* output value from next buffer location to data register */
	{ UDI_PIO_OUT+UDI_PIO_BUF+UDI_PIO_R0,
				UDI_PIO_1BYTE, CMOS_DATA },
	/* decrement count (in R2) */
	{ UDI_PIO_ADD_IMM+UDI_PIO_R2, UDI_PIO_1BYTE, (udi_ubit8_t)-1 },
	/* if count is zero, we're done */
	{ UDI_PIO_CSKIP+UDI_PIO_R2, UDI_PIO_1BYTE, UDI_PIO_NZ },
	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0 },
	/* increment address and buffer offset */
	{ UDI_PIO_ADD_IMM+UDI_PIO_R1, UDI_PIO_1BYTE, 1 },
	{ UDI_PIO_ADD_IMM+UDI_PIO_R0, UDI_PIO_1BYTE, 1 },
	/* go back to main loop */
	{ UDI_PIO_BRANCH, 0, 1 }
};

/*
 * --------------------------------------------------------------------
 * Management operations init section:
 * --------------------------------------------------------------------
 */
static udi_primary_init_t udi_cmos_primary_init_info = {
	&cmos_mgmt_ops, cmos_default_op_flags,
	0,			/* mgmt_scratch_size */
	0,                      /* enumerate attr list length */
	sizeof(cmos_region_data_t), /* rdata size */
	0,			/* no children, so no enumeration */
	0,			/* buf path */
};

/*
 * --------------------------------------------------------------------
 * Meta operations init section:
 * --------------------------------------------------------------------
 */
static udi_ops_init_t udi_cmos_ops_init_list[] = {
	{
		GIO_OPS_IDX,
		CMOS_GIO_META,		/* meta index [from udiprops.txt] */
		UDI_GIO_PROVIDER_OPS_NUM,
		0,			/* no channel context */
		(udi_ops_vector_t *)&cmos_gio_provider_ops,
		cmos_default_op_flags	/* op_flags */
	},
	{
		BUS_DEVICE_OPS_IDX,
		CMOS_BRIDGE_META,	/* meta index [from udiprops.txt] */
		UDI_BUS_DEVICE_OPS_NUM,
		0,			/* no channel context */
		(udi_ops_vector_t *)&cmos_bus_device_ops,
		cmos_default_op_flags	/* op_flags */
	},
#if DO_INTERRUPTS
	{
		INTR_HANDLER_OPS_IDX,
		CMOS_BRIDGE_META,	/* meta index [from udiprops.txt] */
		UDI_BUS_INTR_HANDLER_OPS_NUM,
		0,			/* no channel context */
		(udi_ops_vector_t *)&cmos_intr_handler_ops,
		cmos_default_op_flags	/* op_flags */
	},
#endif /* DO_INTERRUPTS */
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
		GIO_BIND_CB_IDX,	/* GIO Bind CB		*/
		CMOS_GIO_META,		/* from udiprops.txt 	*/
		UDI_GIO_BIND_CB_NUM,	/* meta cb_num		*/
		GIO_BIND_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
	{
		GIO_XFER_CB_IDX,	/* GIO Xfer CB		*/
		CMOS_GIO_META,		/* from udiprops.txt 	*/
		UDI_GIO_XFER_CB_NUM,	/* meta cb_num		*/
		GIO_XFER_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
	{
		GIO_EVENT_CB_IDX,	/* GIO Event CB		*/
		CMOS_GIO_META,		/* from udiprops.txt	*/
		UDI_GIO_EVENT_CB_NUM,	/* meta cb_num		*/
		GIO_EVENT_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
	{
		BUS_BIND_CB_IDX,	/* Bridge Bind CB	*/
		CMOS_BRIDGE_META,	/* from udiprops.txt	*/
		UDI_BUS_BIND_CB_NUM,	/* meta cb_num		*/
		BUS_BIND_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
#if DO_INTERRUPTS
	{
		INTR_ATTACH_CB_IDX,	/* Interrupt Attach CB	*/
		CMOS_BRIDGE_META,	/* from udiprops.txt	*/
		UDI_BUS_INTR_ATTACH_CB_NUM,	/* meta cb_num	*/
		INTR_ATTACH_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
	{
		INTR_EVENT_CB_IDX,	/* Interrupt Event CB	*/
		CMOS_BRIDGE_META,	/* from udiprops.txt	*/
		UDI_BUS_INTR_EVENT_CB_NUM,	/* meta cb_num	*/
		INTR_EVENT_SCRATCH,	/* scratch requirement	*/
		0,			/* inline size		*/
		NULL			/* inline layout	*/
	},
#endif	/* DO_INTERRUPTS */
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
 * Start of actual code section.
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


static void cmos_bus_bind_ack_1(udi_cb_t *, udi_pio_handle_t);

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
	 * our device doesn't do DMA.  Even if it did,
	 * preferred_endianness is only used with bi-endianness devices
	 * that can change endianness.
	 */
	udi_dma_constraints_free(constraints);

	/*
	 * Save the bus bind control block for unbinding later.
	 */
	rdata->bus_bind_cb = bus_bind_cb;

	if (status != UDI_OK) {
		udi_channel_event_complete(channel_event_cb,
					   UDI_STAT_CANNOT_BIND);
		return;
	}

	/*
	 * Now we have access to our hardware.  Set up the PIO mappings
	 * we'll need later.
	 */
	udi_pio_map(cmos_bus_bind_ack_1, UDI_GCB(bus_bind_cb),
		    CMOS_REGSET, CMOS_BASE, CMOS_LENGTH,
		    cmos_trans_read,
		    sizeof cmos_trans_read / sizeof(udi_pio_trans_t),
		    UDI_PIO_NEVERSWAP|UDI_PIO_STRICTORDER, CMOS_PACE, 0);
}

static void cmos_bus_bind_ack_2(udi_cb_t *, udi_pio_handle_t);

static void
cmos_bus_bind_ack_1(
	udi_cb_t *gcb,
	udi_pio_handle_t new_pio_handle)
{
	cmos_region_data_t *rdata = gcb->context;

	/* Save the PIO handle for later use. */
	rdata->trans_read = new_pio_handle;

	udi_pio_map(cmos_bus_bind_ack_2, gcb,
		    CMOS_REGSET, CMOS_BASE, CMOS_LENGTH,
		    cmos_trans_write,
		    sizeof cmos_trans_write / sizeof(udi_pio_trans_t),
		    UDI_PIO_NEVERSWAP|UDI_PIO_STRICTORDER, CMOS_PACE, 0);
}

static void
cmos_bus_bind_ack_2(
	udi_cb_t *gcb,
	udi_pio_handle_t new_pio_handle)
{
	cmos_region_data_t *rdata = gcb->context;
	udi_channel_event_cb_t *channel_event_cb =
					gcb->initiator_context;

	/* Save the PIO handle for later use. */
	rdata->trans_write = new_pio_handle;

#if DO_INTERRUPTS
	/* Attach interrupts here... */
#endif

	/* Let the MA know we've completed binding. */
	udi_channel_event_complete(channel_event_cb, UDI_OK);
}

static void
cmos_gio_bind_req(udi_gio_bind_cb_t *gio_bind_cb)
{
	udi_gio_bind_ack(gio_bind_cb, CMOS_DEVSIZE, 0, UDI_OK);
}

static void 
cmos_gio_unbind_req(udi_gio_bind_cb_t *gio_bind_cb) 
{
	udi_gio_unbind_ack(gio_bind_cb);
}

static void
cmos_child_channel_event(udi_channel_event_cb_t *channel_event_cb)
{
	udi_channel_event_complete(channel_event_cb, UDI_OK);
}

static void cmos_do_read(udi_gio_xfer_cb_t *, udi_ubit8_t);
static void cmos_do_write(udi_gio_xfer_cb_t *, udi_ubit8_t);

static void 
cmos_gio_xfer_req(udi_gio_xfer_cb_t *gio_xfer_cb)
{
	udi_gio_rw_params_t *rw_params = gio_xfer_cb->tr_params;

	/*
	 * We can ignore the timeout parameter since all of our
	 * operations are synchronous.
	 */

	switch (gio_xfer_cb->op) {
	case UDI_GIO_OP_READ:
	        udi_assert(rw_params->offset_hi == 0 &&
		           rw_params->offset_lo <= CMOS_DEVSIZE &&
			   gio_xfer_cb->data_buf->buf_size <=
			   	CMOS_DEVSIZE - rw_params->offset_lo);
		cmos_do_read(gio_xfer_cb, rw_params->offset_lo);
		break;
	case UDI_GIO_OP_WRITE:
	        udi_assert(rw_params->offset_hi == 0 &&
		           rw_params->offset_lo <= CMOS_DEVSIZE &&
			   gio_xfer_cb->data_buf->buf_size <=
			   	CMOS_DEVSIZE - rw_params->offset_lo);
		cmos_do_write(gio_xfer_cb, rw_params->offset_lo);
		break;
	default:
		udi_gio_xfer_nak(gio_xfer_cb, UDI_STAT_NOT_UNDERSTOOD);
	}
}

static void cmos_do_read_1(udi_cb_t *, udi_buf_t *,
				udi_status_t, udi_ubit16_t);

static void
cmos_do_read(udi_gio_xfer_cb_t *gio_xfer_cb, udi_ubit8_t addr)
{
	cmos_region_data_t *rdata = gio_xfer_cb->gcb.context;
	cmos_gio_xfer_scratch_t *gio_xfer_scratch =
					gio_xfer_cb->gcb.scratch;

	/*
	 * Store address into first byte of scratch space,
	 * so the trans list can get at it.
	 */
	gio_xfer_scratch->addr = addr;
	gio_xfer_scratch->count = gio_xfer_cb->data_buf->buf_size;

	udi_pio_trans(cmos_do_read_1, UDI_GCB(gio_xfer_cb),
		      rdata->trans_read, 0,
		      gio_xfer_cb->data_buf, NULL);
}

static void
cmos_do_read_1(
	udi_cb_t *gcb,
	udi_buf_t *new_buf,
	udi_status_t status,
	udi_ubit16_t result)
{
	udi_gio_xfer_cb_t *gio_xfer_cb =
				UDI_MCB(gcb, udi_gio_xfer_cb_t);

	/* udi_pio_trans may create a new buffer. */
	gio_xfer_cb->data_buf = new_buf;

	if (status == UDI_OK)
		udi_gio_xfer_ack(gio_xfer_cb);
	else
		udi_gio_xfer_nak(gio_xfer_cb, status);
}

static void cmos_do_write_1(udi_cb_t *, udi_buf_t *,
				udi_status_t, udi_ubit16_t);

static void
cmos_do_write(udi_gio_xfer_cb_t *gio_xfer_cb, udi_ubit8_t addr)
{
	cmos_region_data_t *rdata = UDI_GCB(gio_xfer_cb)->context;
	cmos_gio_xfer_scratch_t *gio_xfer_scratch =
					gio_xfer_cb->gcb.scratch;

	/*
	 * The first CMOS_RDONLY_SZ bytes of this device are not
	 * allowed to be written through this driver.  Fail any attempt
	 * to write to these bytes.
	 */
	if (addr < CMOS_RDONLY_SZ) {
		udi_gio_xfer_nak(gio_xfer_cb,
				 UDI_STAT_MISTAKEN_IDENTITY);
		return;
	}

	/*
	 * Store address into first byte of scratch space,
	 * so the trans list can get at it.  The data to write
	 * will be accessed directly from the buffer.
	 */
	gio_xfer_scratch->addr = addr;
	gio_xfer_scratch->count = gio_xfer_cb->data_buf->buf_size;

	udi_pio_trans(cmos_do_write_1, UDI_GCB(gio_xfer_cb),
		      rdata->trans_write, 0,
		      gio_xfer_cb->data_buf, NULL);
}

static void
cmos_do_write_1(
	udi_cb_t *gcb,
	udi_buf_t *new_buf,
	udi_status_t status,
	udi_ubit16_t result)
{
	udi_gio_xfer_cb_t *gio_xfer_cb =
				UDI_MCB(gcb, udi_gio_xfer_cb_t);

	udi_buf_free(new_buf);
	gio_xfer_cb->data_buf = NULL;

	if (status == UDI_OK)
		udi_gio_xfer_ack(gio_xfer_cb);
	else
		udi_gio_xfer_nak(gio_xfer_cb, status);
}


static void
cmos_enumerate_req(
	udi_enumerate_cb_t *cb,
	udi_ubit8_t enum_level)
{

	udi_ubit8_t result;

	/*
	 * We don't enumerate any attributes; the meta alone should be
	 * enough to bind to the mapper.
	 */
	cb->attr_valid_length = 0;
        
	switch (enum_level) {
	case UDI_ENUMERATE_START:
	case UDI_ENUMERATE_START_RESCAN:
		result = UDI_ENUMERATE_OK;
		break;
         
	case UDI_ENUMERATE_NEXT:
		result = UDI_ENUMERATE_DONE;
		break;
         
	case UDI_ENUMERATE_RELEASE:
		/* release resources for the specified child */
		result = UDI_ENUMERATE_RELEASED;
		break;
           
	case UDI_ENUMERATE_NEW:
	case UDI_ENUMERATE_DIRECTED:
	default:
		result = UDI_ENUMERATE_FAILED;
	}
	udi_enumerate_ack(cb, result, GIO_OPS_IDX);
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
#if DO_INTERRUPTS
		/* Detach interrupts here... */
#endif
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
	cmos_region_data_t *rdata = bus_bind_cb->gcb.context;

	udi_pio_unmap(rdata->trans_read);
	udi_pio_unmap(rdata->trans_write);

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

#if DO_INTERRUPTS

static void
cmos_intr_attach_ack(
	udi_intr_attach_cb_t *intr_attach_cb,
	udi_status_t status)
{
	/* Complete interrupt attachment here... */
}

static void
cmos_intr_detach_ack(
	udi_intr_detach_cb_t *intr_detach_cb)
{
	/* Complete interrupt detachment here... */
}

static void
cmos_intr_event_ind(
	udi_intr_event_cb_t *intr_event_cb,
	udi_ubit8_t flags)
{
	cmos_region_data_t *rdata = UDI_GCB(intr_event_cb)->context;

	/* Handle interrupt... */

	/*
	 * Acknowledge the interrupt.  If PIO Preprocessing was used,
	 * intr_event_cb->intr_result will be populated by the PIO_END
	 * return value of the trans list.
	 */
	udi_intr_event_rdy(intr_event_cb);
}

static void
cmos_intr_channel_event(udi_channel_event_cb_t *channel_event_cb)
{
	udi_channel_event_complete(channel_event_cb, UDI_OK);
}

#endif /* DO_INTERRUPTS */
