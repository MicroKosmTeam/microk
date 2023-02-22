#define UDI_VERSION	0x101
#define UDI_NIC_VERSION	0x101

#include <udi.h>
#include <udi_nic.h>


#define WHIPIT_NSR_TX_CHAN		1
#define WHIPIT_NSR_RX_CHAN		2

#define WHIPIT_ESAP			0x1109

#define GET_CB_FROM_CHAIN(cb_chain, cb)					\
	((cb_chain) ?							\
	(cb = cb_chain, cb_chain = cb->chain, cb->chain = NULL, TRUE) :	\
	(cb = NULL, FALSE))


typedef enum
{
	WHIPIT_MGMT_META_IDX = 0,
	WHIPIT_GIO_META_IDX,
	WHIPIT_NIC_META_IDX,
	WHIPIT_META_COUNT
}
whipit_meta_indices_t;

typedef enum
{
	WHIPIT_GIO_PROVIDER_OPS_IDX = 1,
	WHIPIT_NIC_CTRL_OPS_IDX,
	WHIPIT_NIC_TX_OPS_IDX,
	WHIPIT_NIC_RX_OPS_IDX
}
whipit_ops_indices_t;

typedef enum
{
	WHIPIT_GIO_XFER_NIC_CB_IDX = 1,
	WHIPIT_GIO_XFER_SCSI_CB_IDX,
	WHIPIT_NIC_STD_CB_IDX,
	WHIPIT_NIC_BIND_CB_IDX,
	WHIPIT_NIC_CTRL_CB_IDX,
	WHIPIT_NIC_STATUS_CB_IDX,
	WHIPIT_NIC_INFO_CB_IDX,
	WHIPIT_NIC_TX_CB_IDX,
	WHIPIT_NIC_RX_CB_IDX,
	WHIPIT_TIMER_CB_IDX
}
whipit_cb_indices_t;

typedef struct {
	udi_ubit8_t			bytes[6];
} whipit_eaddr_t;

static whipit_eaddr_t WHIPIT_BROADCAST = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef enum {
	WHIPIT_PAYLOAD_HEARTBEAT,
	WHIPIT_PAYLOAD_DATA
}
whipit_payload_type_t;

#define WHIPIT_MIN_PAYLOAD		64
#define WHIPIT_MAX_PAYLOAD		1400

typedef struct
{
	udi_size_t			frame_length;
	struct {
		whipit_eaddr_t		dst_mac;
		whipit_eaddr_t		src_mac;
		udi_ubit16_t		type_len;
	} ethernet_II_header;
	udi_ubit16_t			pad0;
	whipit_payload_type_t		payload_type;
	union {
		struct {
			udi_ubit32_t	count;
		} heartbeat;
		udi_ubit8_t		data[WHIPIT_MAX_PAYLOAD];
	} u_payload;
} 
whipit_nsr_tx_scratch_t;


typedef struct {
        udi_init_context_t		init_context;

	udi_channel_t			nd_ctrl_chan;
	udi_channel_t			nd_tx_chan;
	udi_channel_t			nd_rx_chan;
	udi_buf_path_t			tx_buf_path;
	udi_buf_path_t			rx_buf_path;
	udi_nic_tx_cb_t			*tx_cbs;
	udi_nic_rx_cb_t			*rx_cbs;
	udi_ubit32_t			rx_cbs_allocated;
	udi_ubit32_t			rx_hw_threshold;
	udi_cb_t			*master_timer_cb;
	udi_size_t			rx_buf_size;
	udi_dma_constraints_t		rx_constraints;
	udi_timestamp_t			heartbeat_time;

	whipit_eaddr_t			curr_eaddr;
	udi_ubit8_t			link_status;
	udi_boolean_t			link_status_valid;

	udi_ubit32_t			heartbeat_count;

	udi_ubit32_t			tx_byte_count;
	udi_ubit32_t			rx_byte_count;
	udi_ubit32_t			tx_Mbps;
	udi_ubit32_t			rx_Mbps;

	udi_ubit32_t			tx_delay_usec;
	udi_ubit32_t			tx_frame_bytes;
	udi_ubit32_t			tx_loop;

} whipit_region_data_t;

static udi_usage_ind_op_t		whipit_usage_ind;
static udi_devmgmt_req_op_t		whipit_devmgmt_req;
static udi_final_cleanup_req_op_t	whipit_final_cleanup_req;

static udi_mgmt_ops_t whipit_mgmt_ops = {
	whipit_usage_ind,
	udi_enumerate_no_children,
	whipit_devmgmt_req,
	whipit_final_cleanup_req
};


static udi_channel_event_ind_op_t	whipit_gio_channel_event;
static udi_gio_bind_req_op_t		whipit_gio_bind_req;
static udi_gio_unbind_req_op_t		whipit_gio_unbind_req;
static udi_gio_xfer_req_op_t		whipit_gio_xfer_req;

static udi_gio_provider_ops_t whipit_gio_provider_ops = {
	whipit_gio_channel_event,
	whipit_gio_bind_req,
	whipit_gio_unbind_req,
	whipit_gio_xfer_req,
	udi_gio_event_res_unused
};


static udi_channel_event_ind_op_t	whipit_nsr_ctrl_channel_event;
static udi_nsr_bind_ack_op_t		whipit_nsr_bind_ack;
static udi_nsr_unbind_ack_op_t		whipit_nsr_unbind_ack;
static udi_nsr_enable_ack_op_t		whipit_nsr_enable_ack;
static udi_nsr_ctrl_ack_op_t		whipit_nsr_ctrl_ack;
static udi_nsr_info_ack_op_t		whipit_nsr_info_ack;
static udi_nsr_status_ind_op_t		whipit_nsr_status_ind;

static udi_channel_event_ind_op_t	whipit_nsr_tx_channel_event;
static udi_nsr_tx_rdy_op_t		whipit_nsr_tx_rdy;

static udi_channel_event_ind_op_t	whipit_nsr_rx_channel_event;
static udi_nsr_rx_ind_op_t		whipit_nsr_rx_ind;

static udi_nsr_ctrl_ops_t whipit_nsr_ctrl_ops = {
	whipit_nsr_ctrl_channel_event,
	whipit_nsr_bind_ack,
	whipit_nsr_unbind_ack,
	whipit_nsr_enable_ack,
	whipit_nsr_ctrl_ack,
	whipit_nsr_info_ack,
	whipit_nsr_status_ind,
};

static udi_nsr_tx_ops_t whipit_nsr_tx_ops = {
	whipit_nsr_tx_channel_event,
	whipit_nsr_tx_rdy
};

static udi_nsr_rx_ops_t whipit_nsr_rx_ops = {
	whipit_nsr_rx_channel_event,
	whipit_nsr_rx_ind,
	whipit_nsr_rx_ind
};


static udi_ubit8_t whipit_default_op_flags[] = {
	0, 0, 0, 0, 0
};

static udi_primary_init_t whipit_primary_init = {
	&whipit_mgmt_ops,
	whipit_default_op_flags,
	0,
	0,
	sizeof(whipit_region_data_t),
	0,
	2,
};

static udi_ops_init_t whipit_ops_init_list[] = {
	{
		WHIPIT_GIO_PROVIDER_OPS_IDX,
		WHIPIT_GIO_META_IDX,
		UDI_GIO_PROVIDER_OPS_NUM,
		0,
		(udi_ops_vector_t*)&whipit_gio_provider_ops,
		whipit_default_op_flags,
	},
	{
		WHIPIT_NIC_CTRL_OPS_IDX,
		WHIPIT_NIC_META_IDX,
		UDI_NSR_CTRL_OPS_NUM,
		0,
		(udi_ops_vector_t *)&whipit_nsr_ctrl_ops,
		whipit_default_op_flags,
	},
	{
		WHIPIT_NIC_TX_OPS_IDX,
		WHIPIT_NIC_META_IDX,
		UDI_NSR_TX_OPS_NUM,
		0,
		(udi_ops_vector_t *)&whipit_nsr_tx_ops,
		whipit_default_op_flags,
	},
	{
		WHIPIT_NIC_RX_OPS_IDX,
		WHIPIT_NIC_META_IDX,
		UDI_NSR_RX_OPS_NUM,
		0,
		(udi_ops_vector_t *)&whipit_nsr_rx_ops,
		whipit_default_op_flags,
	},
	{
		0
	}
};


static udi_cb_init_t whipit_cb_init_list[] = { 
	{
		WHIPIT_GIO_XFER_NIC_CB_IDX,
		WHIPIT_GIO_META_IDX,
		UDI_GIO_XFER_CB_NUM,
		0,
		0,
		NULL
	},
	{
		WHIPIT_GIO_XFER_SCSI_CB_IDX,
		WHIPIT_GIO_META_IDX,
		UDI_GIO_XFER_CB_NUM,
		0,
		0,
		NULL
	},
	{
		WHIPIT_NIC_STD_CB_IDX,
		WHIPIT_NIC_META_IDX,
		UDI_NIC_STD_CB_NUM,
		0,
		0,
		NULL
	},
	{
		WHIPIT_NIC_BIND_CB_IDX,
		WHIPIT_NIC_META_IDX,
		UDI_NIC_BIND_CB_NUM,
		0,
		0,
		NULL
	},
	{
		WHIPIT_NIC_CTRL_CB_IDX,
		WHIPIT_NIC_META_IDX,
		UDI_NIC_CTRL_CB_NUM,
		0,
		0,
		NULL
	},
	{
		WHIPIT_NIC_STATUS_CB_IDX,
		WHIPIT_NIC_META_IDX,
		UDI_NIC_STATUS_CB_NUM,
		0,
		0,
		NULL
	},
	{
		WHIPIT_NIC_INFO_CB_IDX,
		WHIPIT_NIC_META_IDX,
		UDI_NIC_INFO_CB_NUM,
		0,
		0,
		NULL
	},
	{
		WHIPIT_NIC_TX_CB_IDX,
		WHIPIT_NIC_META_IDX,
		UDI_NIC_TX_CB_NUM,
		sizeof(whipit_nsr_tx_scratch_t),
		0,
		NULL
	},
	{
		WHIPIT_NIC_RX_CB_IDX,
		WHIPIT_NIC_META_IDX,
		UDI_NIC_RX_CB_NUM,
		0,
		0,
		NULL
	},
	{
		0
	}
};


static udi_gcb_init_t whipit_gcb_init_list[] = {
	{
		WHIPIT_TIMER_CB_IDX,
		0
	},
	{
		0
	}
};


udi_init_t udi_init_info = {
	&whipit_primary_init,
	NULL,
	whipit_ops_init_list,
	whipit_cb_init_list,
	whipit_gcb_init_list,
	NULL
};


static udi_cb_alloc_call_t		whipit_nsr_nd_bind_cb;
static udi_cb_alloc_call_t		whipit_nsr_nd_enable_cb;
static udi_cb_alloc_call_t		whipit_nsr_nd_unbind_cb;
static udi_cb_alloc_call_t		whipit_nsr_timer_cb;
static udi_channel_spawn_call_t		whipit_nsr_tx_chan_spawn;
static udi_channel_spawn_call_t		whipit_nsr_rx_chan_spawn;
static udi_buf_write_call_t		whipit_nsr_rx_allocate_CB;
static udi_cb_alloc_call_t		whipit_nsr_rx_allocate_buf;
static udi_dma_constraints_attr_set_call_t
					whipit_nsr_rx_allocate_constraints;
static udi_timer_tick_call_t		whipit_nsr_master_timer;

static void whipit_nsr_tx(udi_nic_tx_cb_t *tx_cb);
static void whipit_nsr_tx_buf(udi_cb_t *gcb, udi_buf_t * new_dst_buf);


static void
whipit_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	whipit_region_data_t *rdata = UDI_GCB(cb)->context;

	udi_debug_printf("whipit_rdata = %P\n", rdata);

	rdata->tx_delay_usec = 100 * 1000;
	rdata->tx_frame_bytes = 1400;
	
	udi_usage_res(cb);

	return;
}


static void
whipit_devmgmt_req(udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op,
	udi_ubit8_t parent_id)
{
	whipit_region_data_t *rdata = UDI_GCB(cb)->context;

	udi_trace_write(&rdata->init_context,
		UDI_TREVENT_INTERNAL_1, 0, 1400,
		mgmt_op, parent_id);

	switch (mgmt_op) {
	case UDI_DMGMT_UNBIND:
		if (rdata->master_timer_cb) {
			udi_timer_cancel(rdata->master_timer_cb);
			udi_cb_free(rdata->master_timer_cb);
			rdata->master_timer_cb = NULL;
		}
		udi_cb_alloc(whipit_nsr_nd_unbind_cb, 
			UDI_GCB(cb),
			WHIPIT_NIC_STD_CB_IDX, 
			rdata->nd_ctrl_chan);
		break;
	case UDI_DMGMT_PREPARE_TO_SUSPEND:
	case UDI_DMGMT_SUSPEND:
	case UDI_DMGMT_SHUTDOWN:
	case UDI_DMGMT_PARENT_SUSPENDED:
	case UDI_DMGMT_RESUME:
		udi_devmgmt_ack(cb, 0, UDI_STAT_NOT_SUPPORTED);
		break;
	default:
		udi_devmgmt_ack(cb, 0, UDI_STAT_NOT_UNDERSTOOD);
		break;
	}

	return;
}


static void
whipit_final_cleanup_req(udi_mgmt_cb_t *cb)
{
	whipit_region_data_t *rdata = UDI_GCB(cb)->context;

	udi_trace_write(&rdata->init_context,
		UDI_TREVENT_INTERNAL_1, 0, 1410);

	udi_final_cleanup_ack(cb);

	return;
}


static void
whipit_gio_channel_event(udi_channel_event_cb_t *cb)
{
	whipit_region_data_t *rdata = UDI_GCB(cb)->context;

	udi_trace_write(&rdata->init_context,
		UDI_TREVENT_INTERNAL_1, 0, 1500,
		cb->event);

	switch (cb->event)
	{
	case UDI_CHANNEL_CLOSED:
		udi_channel_event_complete(cb, UDI_OK);
		udi_channel_close(UDI_GCB(cb)->channel);
		break;
	default:
		udi_channel_event_complete(cb, UDI_OK);
		break;
	}

	return;
}


static void
whipit_gio_bind_req(udi_gio_bind_cb_t *gio_bind_cb)
{
	whipit_region_data_t *rdata = UDI_GCB(gio_bind_cb)->context;

	udi_trace_write(&rdata->init_context,
		UDI_TREVENT_INTERNAL_1, 0, 1501);

	udi_gio_bind_ack(gio_bind_cb, 0, 0, UDI_OK);

	return;
}


static void 
whipit_gio_unbind_req(udi_gio_bind_cb_t *gio_bind_cb) 
{
	whipit_region_data_t *rdata = UDI_GCB(gio_bind_cb)->context;

	udi_trace_write(&rdata->init_context,
		UDI_TREVENT_INTERNAL_1, 0, 1502);

	udi_gio_unbind_ack(gio_bind_cb);

	return;
}


static void 
whipit_gio_xfer_req(udi_gio_xfer_cb_t *gio_xfer_cb)
{
	whipit_region_data_t *rdata = UDI_GCB(gio_xfer_cb)->context;

	udi_trace_write(&rdata->init_context,
		UDI_TREVENT_INTERNAL_1, 0, 1503,
		gio_xfer_cb->op);

	udi_gio_xfer_ack(gio_xfer_cb);

	return;
}


static void
whipit_nsr_ctrl_channel_event(udi_channel_event_cb_t *cb)
{
	whipit_region_data_t *rdata = UDI_GCB(cb)->context;
	udi_channel_t chan = UDI_GCB(cb)->channel;

	udi_trace_write(&rdata->init_context,
		UDI_TREVENT_INTERNAL_1, 0, 1600,
		cb->event);

	switch(cb->event) {
	case UDI_CHANNEL_CLOSED:
		udi_channel_event_complete(cb, UDI_OK);
		udi_channel_close(chan);
		break;
	case UDI_CHANNEL_BOUND:
		rdata->nd_ctrl_chan = chan;
		rdata->rx_buf_path = cb->params.parent_bound.path_handles[0];
		rdata->tx_buf_path = cb->params.parent_bound.path_handles[1];
		udi_cb_alloc(whipit_nsr_nd_bind_cb, 
			UDI_GCB(cb),
			WHIPIT_NIC_BIND_CB_IDX, 
			rdata->nd_ctrl_chan);
		break;
	default:
		udi_channel_event_complete(cb, UDI_OK);
		break;
	}

	return;
}


static void
whipit_nsr_tx_channel_event(udi_channel_event_cb_t *cb)
{
	whipit_region_data_t *rdata = UDI_GCB(cb)->context;
	udi_channel_t chan = UDI_GCB(cb)->channel;

	udi_trace_write(&rdata->init_context,
		UDI_TREVENT_INTERNAL_1, 0, 1601,
		cb->event);

	switch(cb->event) {
	case UDI_CHANNEL_CLOSED:
		udi_channel_event_complete(cb, UDI_OK);
		udi_channel_close(chan);
		break;
	default:
		udi_channel_event_complete(cb, UDI_OK);
		break;
	}

	return;
}


static void
whipit_nsr_rx_channel_event(udi_channel_event_cb_t *cb)
{
	whipit_region_data_t *rdata = UDI_GCB(cb)->context;
	udi_channel_t chan = UDI_GCB(cb)->channel;

	udi_trace_write(&rdata->init_context,
		UDI_TREVENT_INTERNAL_1, 0, 1602,
		cb->event);

	switch(cb->event) {
	case UDI_CHANNEL_CLOSED:
		udi_channel_event_complete(cb, UDI_OK);
		udi_channel_close(chan);
		break;
	default:
		udi_channel_event_complete(cb, UDI_OK);
		break;
	}

	return;
}


static void
whipit_nsr_bind_ack(udi_nic_bind_cb_t *cb, udi_status_t status)
{
	whipit_region_data_t *rdata = UDI_GCB(cb)->context;
	udi_channel_event_cb_t *channel_event_cb;

	udi_trace_write(&rdata->init_context,
		UDI_TREVENT_INTERNAL_1, 0, 1610,
		status, cb->media_type, cb->min_pdu_size, 
		cb->max_pdu_size, cb->rx_hw_threshold, 
		cb->mac_addr_len, cb->mac_addr[0],
		cb->mac_addr[1], cb->mac_addr[2],
		cb->mac_addr[3], cb->mac_addr[4],
		cb->mac_addr[5]);
	
	channel_event_cb = UDI_MCB(UDI_GCB(cb)->initiator_context, 
		udi_channel_event_cb_t);

	if (status == UDI_OK) {
		rdata->rx_buf_size = cb->max_pdu_size;
		rdata->rx_hw_threshold = cb->rx_hw_threshold;
		rdata->curr_eaddr = *((whipit_eaddr_t *)&cb->mac_addr);
		whipit_nsr_rx_allocate_CB(UDI_GCB(channel_event_cb),
			NULL);
	} else {
		udi_channel_event_complete(channel_event_cb, status);
	}
	
	udi_cb_free(UDI_GCB(cb));

	return;
}


static void
whipit_nsr_unbind_ack(udi_nic_cb_t *cb, udi_status_t status)
{
	whipit_region_data_t *rdata = UDI_GCB(cb)->context;
	udi_mgmt_cb_t *mgmt_cb;
	udi_nic_rx_cb_t *rx_cb;

	udi_trace_write(&rdata->init_context,
		UDI_TREVENT_INTERNAL_1, 0, 1611,
		status);

	while (GET_CB_FROM_CHAIN(rdata->rx_cbs, rx_cb)) {
		udi_buf_free(rx_cb->rx_buf);
		udi_cb_free(UDI_GCB(rx_cb));
	}

	rdata->nd_tx_chan = UDI_NULL_CHANNEL;
	rdata->nd_rx_chan = UDI_NULL_CHANNEL;
	
	mgmt_cb = UDI_MCB(UDI_GCB(cb)->initiator_context, udi_mgmt_cb_t);
	udi_cb_free(UDI_GCB(cb));

	udi_devmgmt_ack(mgmt_cb, 0, status);

	return;
}


static void
whipit_nsr_enable_ack(udi_nic_cb_t *cb, udi_status_t status)
{
	whipit_region_data_t *rdata = UDI_GCB(cb)->context;
	udi_channel_event_cb_t *channel_event_cb;

	udi_trace_write(&rdata->init_context,
		UDI_TREVENT_INTERNAL_1, 0, 1612,
		status);

	channel_event_cb = UDI_MCB(UDI_GCB(cb)->initiator_context, 
		udi_channel_event_cb_t);
	udi_channel_event_complete(channel_event_cb, status);

	udi_cb_alloc(whipit_nsr_timer_cb, 
		UDI_GCB(cb),
		WHIPIT_TIMER_CB_IDX,
		rdata->nd_ctrl_chan);
	
	udi_debug_printf("whipit_rdata = %P\n", rdata);

	return;
}


static void
whipit_nsr_ctrl_ack(udi_nic_ctrl_cb_t *cb, udi_status_t status)
{
	whipit_region_data_t *rdata = UDI_GCB(cb)->context;
	udi_gio_xfer_cb_t *gio_xfer_cb = UDI_GCB(cb)->initiator_context;

	udi_trace_write(&rdata->init_context,
		UDI_TREVENT_INTERNAL_1, 0, 1613,
		status);

	udi_cb_free(UDI_GCB(cb));

	return;
}


static void
whipit_nsr_info_ack(udi_nic_info_cb_t *cb)
{
	whipit_region_data_t *rdata = UDI_GCB(cb)->context;

	udi_trace_write(&rdata->init_context,
		UDI_TREVENT_INTERNAL_1, 0, 1614,
		cb->interface_is_active, cb->link_is_active,
		cb->is_full_duplex, cb->link_mbps);

	udi_cb_free(UDI_GCB(cb));

	return;
}


static void
whipit_nsr_status_ind(udi_nic_status_cb_t *cb)
{
	whipit_region_data_t *rdata = UDI_GCB(cb)->context;

	udi_trace_write(&rdata->init_context,
		UDI_TREVENT_INTERNAL_1, 0, 1615,
		cb->event);

	rdata->link_status_valid = TRUE;
	rdata->link_status = cb->event;

	if (cb->event == UDI_NIC_LINK_UP) {
		if (rdata->rx_cbs) {
			udi_nd_rx_rdy(rdata->rx_cbs);
			rdata->rx_cbs = NULL;
		}
	}
	
	udi_cb_free(UDI_GCB(cb));

	return;
}


static void
whipit_nsr_tx_rdy(udi_nic_tx_cb_t *cb)
{
	whipit_region_data_t *rdata = UDI_GCB(cb)->context;
	udi_nic_tx_cb_t *tx_cb;

#if 0
	udi_trace_write(&rdata->init_context,
		UDI_TREVENT_INTERNAL_1, 0, 1616,
		cb);
#endif

	while (cb) {
		tx_cb = cb;
		cb = cb->chain;
		tx_cb->chain = NULL;
		if (tx_cb->tx_buf)
			whipit_nsr_tx_buf(UDI_GCB(tx_cb), tx_cb->tx_buf);
		else
			whipit_nsr_tx(tx_cb);
	}
	
	return;
}


static void
whipit_nsr_rx_ind(udi_nic_rx_cb_t *cb)
{
	whipit_region_data_t *rdata = UDI_GCB(cb)->context;
	udi_nic_rx_cb_t *rx_cb;

#if 0
	udi_trace_write(&rdata->init_context,
		UDI_TREVENT_INTERNAL_1, 0, 1617,
		cb);
#endif

	for (rx_cb = cb; rx_cb; rx_cb = rx_cb->chain) {
		rdata->rx_byte_count += rx_cb->rx_buf->buf_size;
	}
	
	udi_nd_rx_rdy(cb);

	return;
}


static void
whipit_nsr_nd_bind_cb(udi_cb_t *gcb, udi_cb_t *new_cb)
{
	whipit_region_data_t *rdata = gcb->context;
	
	new_cb->initiator_context = gcb;

 	if (UDI_HANDLE_IS_NULL(rdata->nd_tx_chan, udi_channel_t)) {
 		udi_channel_spawn(whipit_nsr_tx_chan_spawn, 
 			new_cb, rdata->nd_ctrl_chan,
 			WHIPIT_NSR_TX_CHAN, WHIPIT_NIC_TX_OPS_IDX, 
 			rdata);
 	} else {
 		udi_nd_bind_req(UDI_MCB(new_cb, udi_nic_bind_cb_t),
 			WHIPIT_NSR_TX_CHAN, WHIPIT_NSR_RX_CHAN);
 	}

	return;
}


static void
whipit_nsr_tx_chan_spawn(udi_cb_t *gcb, udi_channel_t chan)
{
	whipit_region_data_t *rdata = gcb->context;

	rdata->nd_tx_chan = chan;

	udi_channel_spawn(whipit_nsr_rx_chan_spawn, 
		gcb, rdata->nd_ctrl_chan,
		WHIPIT_NSR_RX_CHAN, WHIPIT_NIC_RX_OPS_IDX,
		rdata);

	return;
}


static void
whipit_nsr_rx_chan_spawn(udi_cb_t *gcb, udi_channel_t chan)
{
	whipit_region_data_t *rdata = gcb->context;

	rdata->nd_rx_chan = chan;

	udi_nd_bind_req(UDI_MCB(gcb, udi_nic_bind_cb_t), 
		WHIPIT_NSR_TX_CHAN, WHIPIT_NSR_RX_CHAN);

	return;
}


static void
whipit_nsr_nd_enable_cb(udi_cb_t *gcb, udi_cb_t *new_cb)
{
	new_cb->initiator_context = gcb;

	udi_nd_enable_req(UDI_MCB(new_cb, udi_nic_cb_t));

	return;
}


static void
whipit_nsr_timer_cb(udi_cb_t *gcb, udi_cb_t *new_cb)
{
	whipit_region_data_t *rdata = gcb->context;
	udi_time_t heartbeat_interval = {1, 0};
	
	udi_cb_free(gcb);

	rdata->master_timer_cb = new_cb;

	rdata->heartbeat_time = udi_time_current();
	udi_timer_start_repeating(whipit_nsr_master_timer,
		rdata->master_timer_cb, heartbeat_interval);

	return;
}


static void
whipit_nsr_nd_unbind_cb(udi_cb_t *gcb, udi_cb_t *new_cb)
{
	new_cb->initiator_context = gcb;

#if 0
	udi_nd_disable_req(UDI_MCB(new_cb, udi_nic_cb_t));
#endif
	udi_nd_unbind_req(UDI_MCB(new_cb, udi_nic_cb_t));

	return;
}


static void
whipit_nsr_rx_allocate_CB(udi_cb_t *gcb, udi_buf_t *new_buf)
{
	whipit_region_data_t *rdata = gcb->context;
	udi_nic_rx_cb_t *nic_rx_cb = rdata->rx_cbs;

	if (new_buf) {
		nic_rx_cb->rx_buf = new_buf;
		nic_rx_cb->rx_buf->buf_size = 0;
	}

	if (rdata->rx_cbs_allocated < rdata->rx_hw_threshold) {
		udi_cb_alloc(whipit_nsr_rx_allocate_buf,
			gcb,
			WHIPIT_NIC_RX_CB_IDX,
			rdata->nd_rx_chan);
	} else {
		udi_cb_alloc(whipit_nsr_nd_enable_cb, 
			gcb,
			WHIPIT_NIC_STD_CB_IDX, 
			rdata->nd_ctrl_chan);
	}

	return;
}


static void
whipit_nsr_rx_allocate_buf(
	udi_cb_t *gcb,
	udi_cb_t *new_cb)
{
	whipit_region_data_t *rdata = gcb->context;
	udi_nic_rx_cb_t *nic_rx_cb = UDI_MCB(new_cb, udi_nic_rx_cb_t);

	nic_rx_cb->rx_buf = NULL;
	nic_rx_cb->chain  = rdata->rx_cbs;
	rdata->rx_cbs = nic_rx_cb;
	
	rdata->rx_cbs_allocated++;

	if (UDI_HANDLE_IS_NULL(rdata->rx_constraints, udi_constraints_t)) {
		udi_dma_constraints_attr_set(whipit_nsr_rx_allocate_constraints,
			gcb,
			UDI_NULL_DMA_CONSTRAINTS,
			NULL, 0, 0);
	} else {
		UDI_BUF_ALLOC(whipit_nsr_rx_allocate_CB, 
			gcb, NULL, 
			rdata->rx_buf_size, 
			rdata->rx_buf_path);
	}

	return;
}


static void
whipit_nsr_rx_allocate_constraints(udi_cb_t * gcb,
	udi_dma_constraints_t new_dst_constraints,
	udi_status_t status)
{
	whipit_region_data_t *rdata = gcb->context;

	rdata->rx_constraints = new_dst_constraints;

	UDI_BUF_ALLOC(whipit_nsr_rx_allocate_CB, 
		gcb, NULL, 
		rdata->rx_buf_size, 
		rdata->rx_buf_path);

	return;
}


static void
whipit_nsr_master_timer(void *context, udi_ubit32_t nmissed)
{
	whipit_region_data_t *rdata = context;
	udi_time_t time_between;
	udi_ubit32_t usec;

	time_between = udi_time_since(rdata->heartbeat_time);
	rdata->heartbeat_time = udi_time_current();

	++rdata->heartbeat_count;

	usec = (time_between.seconds * 1000 * 1000) + 
		(time_between.nanoseconds / 1000);
	
	if (usec > 0) {
		rdata->tx_Mbps = (rdata->tx_byte_count * 76) / 
			(usec * 10);
		rdata->rx_Mbps = (rdata->rx_byte_count * 76) / 
			(usec * 10);
	} else {
		rdata->tx_Mbps = 0;
		rdata->rx_Mbps = 0;
	}

	rdata->tx_byte_count = 0;
	rdata->rx_byte_count = 0;
	
	return;
}


static void
whipit_nsr_tx(udi_nic_tx_cb_t *tx_cb)
{
	whipit_region_data_t *rdata = UDI_GCB(tx_cb)->context;
	whipit_nsr_tx_scratch_t *scratch;
	udi_ubit32_t tx_frame_bytes = rdata->tx_frame_bytes;

	if (tx_frame_bytes < WHIPIT_MIN_PAYLOAD)
		tx_frame_bytes = WHIPIT_MIN_PAYLOAD;
	if (tx_frame_bytes > WHIPIT_MAX_PAYLOAD)
		tx_frame_bytes = WHIPIT_MAX_PAYLOAD;
	
	scratch = (whipit_nsr_tx_scratch_t *)
		UDI_GCB(tx_cb)->scratch;
	
	scratch->frame_length = sizeof(scratch->ethernet_II_header) + 
		sizeof(scratch->payload_type) + tx_frame_bytes;
	scratch->ethernet_II_header.dst_mac = WHIPIT_BROADCAST;
	scratch->ethernet_II_header.src_mac = rdata->curr_eaddr;
	scratch->ethernet_II_header.type_len = WHIPIT_ESAP;
	scratch->payload_type = WHIPIT_PAYLOAD_DATA;
	/* udi_memset(scratch->u_payload.data, 0xBA, tx_frame_bytes); */
	
	UDI_BUF_ALLOC(whipit_nsr_tx_buf,
		UDI_GCB(tx_cb), &scratch->ethernet_II_header, 
		scratch->frame_length, rdata->tx_buf_path);
	
	return;
}


static void
whipit_nsr_tx_buf(udi_cb_t *gcb, udi_buf_t * new_dst_buf)
{
	whipit_region_data_t *rdata = gcb->context;
	udi_nic_tx_cb_t *tx_cb = UDI_MCB(gcb, udi_nic_tx_cb_t);

	tx_cb->tx_buf = new_dst_buf;
	rdata->tx_byte_count += tx_cb->tx_buf->buf_size;
	udi_nd_tx_req(tx_cb);
	
	return;
}




