
/*
 * File: linux/net/netmap_linux.c
 *
 * Linux specific network mapper code
 *
 * Provides the osnsr_xxxxx routines called out of the nsrmap driver and the
 * interfaces to the Linux net device system.
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
 * To do:
 *
 * - Don't copy received data
 * - Keep stats
 */

#define UDI_MEM_ZERO 0

#define	UDI_NIC_VERSION	0x101		/*
					 * Version 1.01 
					 */
#include "udi_osdep_opaquedefs.h"

#include <udi_env.h>			/*
					 * O.S. Environment 
					 */
#include <udi_nic.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/autoconf.h>
#include <linux/module.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
#include <linux/kcomp.h>
#endif

#include "netmap_linux.h"		/*
					 * Local version 
					 */
#include "netmap.h"

/*
 * Forward declarations
 */

STATIC void NSR_enable(udi_cb_t *, void (*)(udi_cb_t *, udi_status_t));
STATIC void NSR_disable(udi_cb_t *, void (*)(udi_cb_t *, udi_status_t));
STATIC void NSR_return_packet(udi_nic_rx_cb_t *);
STATIC void NSR_ctrl_req(udi_nic_ctrl_cb_t *);
STATIC nsrmap_Tx_region_data_t *NSR_txdata(nsrmap_region_data_t *);

/*
 * -----------------------------------------------------------------------------
 * OS-specific Interface routines [called by netmap.c]
 * -----------------------------------------------------------------------------
 */
STATIC void osnsr_init(nsrmap_region_data_t *);
STATIC void osnsr_bind_done(udi_channel_event_cb_t *, udi_status_t);
STATIC void osnsr_unbind_done(udi_nic_cb_t *);
STATIC void osnsr_status_ind(udi_nic_status_cb_t *);
STATIC udi_ubit32_t osnsr_max_xfer_size(udi_ubit8_t);
STATIC udi_ubit32_t osnsr_min_xfer_size(udi_ubit8_t);
STATIC void osnsr_rx_packet(udi_nic_rx_cb_t *, udi_boolean_t);
STATIC void osnsr_ctrl_ack(udi_nic_ctrl_cb_t *, udi_status_t);
STATIC udi_ubit32_t osnsr_mac_addr_size(udi_ubit8_t);
STATIC void osnsr_final_cleanup(udi_mgmt_cb_t *);

/*
 * Other mapper routines
 */

STATIC void osnsr_bind_done2(udi_cb_t *, void *);
STATIC void osnsr_bind_done3(udi_cb_t *, udi_cb_t *);
STATIC void osnsr_txbuf_alloc(udi_cb_t *, udi_buf_t *);
STATIC void osnsr_process_rxcb2(udi_cb_t *, udi_buf_t *);

STATIC __inline udi_cb_t *OSNSR_GET_BIND_CB(nsrmap_region_data_t *);
STATIC __inline void OSNSR_RETURN_BIND_CB(nsrmap_region_data_t *, udi_cb_t *);

/*
 * Linux device methods
 */

STATIC int osnsr_open(struct net_device *);
STATIC int osnsr_stop(struct net_device *);
STATIC int osnsr_start_xmit(struct sk_buff *, struct net_device *);
STATIC struct net_device_stats *osnsr_get_stats(struct net_device *);
STATIC void osnsr_set_multicast_list(struct net_device *);
STATIC int osnsr_set_mac_address(struct net_device *, void *addr);


/*
 * Linux media-specific stuff
 */

#ifdef CONFIG_TR
#include <linux/trdevice.h>
#endif

#ifdef CONFIG_FDDI
#include <linux/fddidevice.h>
#endif

#ifdef CONFIG_NET_FC
#include <linux/fcdevice.h>

/*
 * This is defined here.
 */
static unsigned short fc_type_trans(struct sk_buff *, struct net_device *);
#endif

/*
 * OS specific code:
 */

/*
 * -----------------------------------------------------------------------------
 * OS-specific routines called by nsrmap.c
 * -----------------------------------------------------------------------------
 */

/*
 * osnsr_init:
 * ----------
 * Perform OS-specific initialisation of region-local data. Must not block.
 *
 * Calling Context:
 *	Primary region
 */
STATIC void
osnsr_init(nsrmap_region_data_t * rdata)
{
#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_init (rdata %p)\n", rdata);
#endif
	rdata->osdep_ptr = (netmap_osdep_t *)&rdata->osdep;
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_init\n");
#endif
}

/*
 * osnsr_bind_done:
 * ---------------
 * Called when the mapper <-> driver bind has been completed. At this point
 * the driver should become available for OS use. We allocate and fill in a
 * device structure and call register_netdevice to hook it up to the protocol
 * stack.
 */
STATIC void
osnsr_bind_done(udi_channel_event_cb_t *cb, udi_status_t status)
{
	udi_ubit32_t devsize;

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_bind_done (cb %p status %d)\n", cb, status);
#endif

	if (status != UDI_OK) {
		udi_channel_event_complete(cb, status);
#ifdef TRACE_PROC
		_OSDEP_PRINTF("-osnsr_bind_done (status %d)\n", status);
#endif
		return;
	}

	/*
	 * increment the mapper module's usage count, so it cannot be
	 * unloaded until it's no longer used
	 */
	MOD_INC_USE_COUNT;
	/*
	 * Allocate memory for device struct. System will assign a name
	 * like "eth0", "tr0", etc. On 2.2.x kernels, reserve 16 extra bytes to
	 * store the name.
	 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
	devsize = sizeof (struct net_device) + sizeof (osnsr_info_t) + 16;
#else
	devsize = sizeof (struct net_device) + sizeof (osnsr_info_t);
#endif

	udi_mem_alloc(osnsr_bind_done2, UDI_GCB(cb), devsize, UDI_MEM_ZERO);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_bind_done\n");
#endif
}

STATIC void
osnsr_bind_done2(udi_cb_t *gcb, void *mem)
{
	struct net_device *devp = mem;
	nsrmap_region_data_t *rdata = gcb->context;
	osnsr_info_t *infop;

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_bind_done2 (cb %p mem %p)\n", gcb, mem);
#endif
	infop = (osnsr_info_t *)(devp + 1);
	devp->priv = infop;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
	devp->name = (char *)(infop + 1);
#endif
	_OSDEP_SIMPLE_MUTEX_INIT(&infop->lock, "");
	infop->rdata = rdata;
	infop->txdata = NSR_txdata(rdata);

	rdata->osdep_ptr->devp = devp;
	infop->txdata->osdep_tx_ptr = devp;

	udi_cb_alloc_batch(osnsr_bind_done3, gcb, NSRMAP_NET_CB_IDX, 
			OSNSR_BIND_CB_NUM, FALSE, 0, UDI_NULL_BUF_PATH);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_bind_done2\n");
#endif
}

STATIC void
osnsr_bind_done3(udi_cb_t *gcb, udi_cb_t* new_cb)
{
	nsrmap_region_data_t *rdata = gcb->context;
	struct net_device *devp = rdata->osdep_ptr->devp;
	udi_channel_event_cb_t *cb = UDI_MCB(gcb, udi_channel_event_cb_t);
	
#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_bind_done3 (cb %p new_cb %p)\n", gcb, new_cb);
#endif
	rdata->osdep_ptr->bind_cb_pool = new_cb;

	/*
	 * Use provided function to set up device struct for known types.
	 */

	switch (rdata->link_info.media_type) {

	case UDI_NIC_ETHER:
	case UDI_NIC_FASTETHER:
	case UDI_NIC_GIGETHER:
		init_etherdev(devp, 0);
		break;

#ifdef CONFIG_TR
	case UDI_NIC_TOKEN:
		init_trdev(devp, 0);
		break;
#endif

#ifdef CONFIG_FDDI
	case UDI_NIC_FDDI:
		/*
		 * XXX
		 * There isn't a nice fddi init function that will assign
		 * us a name.
		 */
		udi_strcpy(devp->name, "ufddi0");
		fddi_setup(devp);
		break;
#endif

#ifdef CONFIG_NET_FC
	case UDI_NIC_FC:
		init_fcdev(devp, 0);
		break;
#endif

	case UDI_NIC_VGANYLAN:
	case UDI_NIC_ATM:
	case UDI_NIC_MISCMEDIA:
	default:
		/*
		 * XXX 
		 */
		udi_channel_event_complete(cb, UDI_STAT_CANNOT_BIND);
#ifdef TRACE_PROC
		_OSDEP_PRINTF("-osnsr_bind_done2, unsupported media type %d\n", 
				rdata->link_info.media_type);
#endif
		return;
	}

	/*
	 * Take the MTU from the ND just in case.
	 */

	devp->mtu = rdata->link_info.max_pdu_size;

	/*
	 * Copy address to device struct.
	 */

	devp->addr_len = rdata->link_info.mac_addr_len;
	udi_memcpy(devp->dev_addr, rdata->link_info.mac_addr, devp->addr_len);

	/*
	 * Set up device methods.
	 * XXX
	 */

	devp->open = osnsr_open;
	devp->stop = osnsr_stop;
	devp->hard_start_xmit = osnsr_start_xmit;
	devp->set_multicast_list = osnsr_set_multicast_list;
	devp->set_mac_address = osnsr_set_mac_address;

	/*
	 * get_stats is set up below AFTER calling register_netdev, which
	 * causes get_stats to be called. This won't work until the bind
	 * completes.
	 */

	/*
	 * Register the device.
	 */

	if (register_netdev(devp) == 0) {
		udi_channel_event_complete(cb, UDI_OK);
	} else {
		udi_channel_event_complete(cb, UDI_STAT_CANNOT_BIND);
	}

	/*
	 * Now this is safe.
	 */

	devp->get_stats = osnsr_get_stats;

#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_bind_done2\n");
#endif
}

/*
 * osnsr_status_ind:
 * ----------------
 * Called to inform OS of a NIC status change (e.g. cable-loss, hw-reset,
 * cable-present)
 */
STATIC void
osnsr_status_ind(udi_nic_status_cb_t *cb)
{
#ifdef TRACE_PROC
	_OSDEP_PRINTF("osnsr_status_ind (cb %p)\n", cb);
#endif
}

/*
 * osnsr_max_xfer_size:
 * -------------------
 * Return the OS specific maximum transfer size for a given media type.
 * This will depend on the configuration layout of the network stack +
 * demux modules.
 */
STATIC udi_ubit32_t
osnsr_max_xfer_size(udi_ubit8_t media_type)
{
#ifdef TRACE_PROC
	_OSDEP_PRINTF("osnsr_max_xfer_size (media_type %d)\n", media_type);
#endif
	switch (media_type) {
	case UDI_NIC_FDDI:
		return 4500;
	case UDI_NIC_TOKEN:
		return 17839;
	case UDI_NIC_ETHER:
	case UDI_NIC_FASTETHER:
	case UDI_NIC_GIGETHER:
	case UDI_NIC_VGANYLAN:
	case UDI_NIC_ATM:
	case UDI_NIC_FC:
	default:
		return 1500;		/*
					 * Ethernet II PDU size 
					 */
	}
}

/*
 * osnsr_min_xfer_size:
 * -------------------
 * Return the OS specific minimum transfer size for a given media type.
 */
STATIC udi_ubit32_t
osnsr_min_xfer_size(udi_ubit8_t media_type)
{
#ifdef TRACE_PROC
	_OSDEP_PRINTF("osnsr_min_xfer_size (media_type %d)\n", media_type);
#endif
	/*
	 * XXX 
	 */
	switch (media_type) {
	case UDI_NIC_FDDI:
	case UDI_NIC_ETHER:
	case UDI_NIC_TOKEN:
	case UDI_NIC_FASTETHER:
	case UDI_NIC_GIGETHER:
	case UDI_NIC_VGANYLAN:
	case UDI_NIC_ATM:
	case UDI_NIC_FC:
	default:
		return 14;		/*
					 * Link-layer header 
					 */
	}
}

/*
 * osnsr_mac_addr_size:
 * -------------------
 * Return the OS specific MAC address size for a given media type.
 */
STATIC udi_ubit32_t
osnsr_mac_addr_size(udi_ubit8_t media_type)
{
#ifdef TRACE_PROC
	_OSDEP_PRINTF("osnsr_mac_addr_size (media_type %d)\n", media_type);
#endif
	/*
	 * XXX 
	 */
	switch (media_type) {
	default:
		return 6;
	}
}

/*
 * Various callbacks
 */

STATIC void
osnsr_enable_complete(udi_cb_t *cb, udi_status_t status)
{
	nsrmap_region_data_t *rdata = cb->context;
#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_enable_complete (cb %p status %d)\n", cb, status);
#endif
	rdata->open_status = status;
	OSNSR_RETURN_BIND_CB(rdata, cb);
	_OSDEP_SEM_V(&rdata->osdep_ptr->enable_sem);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_enable_complete\n");
#endif
}

STATIC void
osnsr_disable_complete(udi_cb_t *cb, udi_status_t status)
{
	nsrmap_region_data_t *rdata = cb->context;
#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_disable_complete (cb %p status %d)\n", cb,
			status);
#endif
	OSNSR_RETURN_BIND_CB(rdata, cb);

	_OSDEP_SEM_V(&rdata->osdep_ptr->disable_sem);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_disable_complete\n");
#endif
}

/*
 * Support functions
 */
STATIC __inline udi_cb_t*
OSNSR_GET_BIND_CB(nsrmap_region_data_t *rdata) 
{
	udi_cb_t * new_pool, *bind_cb;
	bind_cb = rdata->osdep_ptr->bind_cb_pool;

	_OSDEP_ASSERT(bind_cb && "we've run out of bind cb's!");

	if (bind_cb) {
		new_pool = bind_cb->initiator_context;
		rdata->osdep_ptr->bind_cb_pool = new_pool;

	}
	return bind_cb;
}

STATIC __inline void
OSNSR_RETURN_BIND_CB(nsrmap_region_data_t *rdata, udi_cb_t *bind_cb)
{
	bind_cb->initiator_context = rdata->osdep_ptr->bind_cb_pool;
	rdata->osdep_ptr->bind_cb_pool = bind_cb;
}

/*
 * Device Methods
 */

STATIC int
osnsr_open(struct net_device *devp)
{
	osnsr_info_t *infop = devp->priv;
	nsrmap_region_data_t *rdata = infop->rdata;
	udi_cb_t *cb;

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_open (devp %p)\n", devp);
#endif

	_OSDEP_SEM_INIT(&rdata->osdep_ptr->enable_sem, 0);

	cb = OSNSR_GET_BIND_CB(rdata);
	/*
	 * increment the ND module's usage count
	 */
	__MOD_INC_USE_COUNT(_UDI_CB_TO_CHAN
			    (_UDI_CB_TO_HEADER(cb))->other_end->
			    chan_region->reg_driver->drv_osinfo.mod);
	NSR_enable(cb, osnsr_enable_complete);
	_OSDEP_SEM_P(&rdata->osdep_ptr->enable_sem);
	_OSDEP_SEM_DEINIT(&rdata->osdep_ptr->enable_sem);
	if (rdata->open_status != UDI_OK) {
#ifdef TRACE_PROC
		_OSDEP_PRINTF("-osnsr_open (return _EIO)\n");
#endif
		return -EIO;
	}

	infop->queue_stopped = 0;
	netif_start_queue(devp);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_open\n");
#endif
	return 0;
}

STATIC int
osnsr_stop(struct net_device *devp)
{
	osnsr_info_t *infop = devp->priv;
	nsrmap_region_data_t *rdata = infop->rdata;
	udi_cb_t *cb;
#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_stop (devp %p)\n", devp);
#endif
#if 0
	struct dev_mc_list *saved;

	/*
	 * We need to tell the NIC driver to flush its multicast list.
	 * But, we also need to make sure the kernel trashes its list
	 * too, so we trick the set_multicast_list entry point into
	 * flushing the multicast table by making it look empty.
	 */
	saved = devp->mc_list;
	devp->mc_list = NULL;
	_OSDEP_PRINTF("Freeing multicast list.\n");
	osnsr_set_multicast_list(devp);
	_OSDEP_PRINTF("Done freeing multicast list.\n");
	/* Restore kernel-owned mc_list so it can clean it up later. */
	devp->mc_list = saved;
#else
	while (infop->mc_cur) {
		struct dev_mc_list *mc;

		mc = infop->mc_cur->next;
		kfree(infop->mc_cur);
		infop->mc_cur = mc;
	}
#endif

	netif_stop_queue(devp);

	_OSDEP_SEM_INIT(&rdata->osdep_ptr->disable_sem, 0);

	NSR_disable(OSNSR_GET_BIND_CB(rdata), osnsr_disable_complete);

	_OSDEP_SEM_P(&rdata->osdep_ptr->disable_sem);
	_OSDEP_SEM_DEINIT(&rdata->osdep_ptr->disable_sem);
	/*
	 * decrement the ND module's usage count
	 */
	cb = OSNSR_GET_BIND_CB(rdata);
	__MOD_DEC_USE_COUNT(_UDI_CB_TO_CHAN
			    (_UDI_CB_TO_HEADER(cb))->other_end->
			    chan_region->reg_driver->drv_osinfo.mod);
	OSNSR_RETURN_BIND_CB(rdata, cb);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_stop\n");
#endif
	return 0;
}

/*
 * xbops for socket buffers
 */

STATIC void
osnsr_skb_free(void *context, udi_ubit8_t *space, udi_size_t size)
{
#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_skb_free (context %p space %p size %ld)\n",
			context, space, size);
#endif
	if (context)
		if (in_interrupt()) 
			dev_kfree_skb_irq((struct sk_buff *)context);
		else
			dev_kfree_skb((struct sk_buff *)context);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_skb_free\n");
#endif
}

extern void _osdep_dma_build_scgth(struct _udi_dma_handle *, void *,
				   udi_size_t, int);

STATIC void *
osnsr_skb_map(void *context, udi_ubit8_t *space, udi_size_t size,
	      struct _udi_dma_handle *dmah)
{
#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_skb_map (context %p space %p size %ld dmah %p)\n",
			context, space, size, dmah);
#endif
	_osdep_dma_build_scgth(dmah, (void *)space, size, 0);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_skb_map\n");
#endif
	return (void *)space;
}

STATIC void
osnsr_skb_unmap(void *context, udi_ubit8_t *space, udi_size_t size,
		struct _udi_dma_handle *dmah)
{
#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_skb_unmap (context %p space %p size %ld dmah "
		"%p)\n", context, space, size, dmah);
#endif
	/*
	 * DO NOTHING
	 */
	(void)context;
	(void)space;
	(void)size;
	(void)dmah;
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_skb_unmap\n");
#endif
}

_udi_xbops_t osnsr_skb_xbops = {
	osnsr_skb_free
#ifdef _UDI_PHYSIO_SUPPORTED
		, osnsr_skb_map,
	osnsr_skb_unmap
#endif					/*
					 * _UDI_PHYSIO_SUPPORTED 
					 */
};

/*
 * osnsr_start_xmit -- called from OS to transmit a packet.
 */

STATIC int
osnsr_start_xmit(struct sk_buff *skb, struct net_device *devp)
{
	osnsr_info_t *infop = devp->priv;
	nsrmap_Tx_region_data_t *rdata = infop->txdata;
	nsr_cb_t *pcb;
	udi_cb_t *gcb;
	udi_buf_t *tx_buf;
	_udi_region_t *region;

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_start_xmit (skb %p devp %p)\n", skb, devp);
#endif

#ifdef NET_NOISY
	_OSDEP_PRINTF("NSR queued(%d)\n", NSR_UDI_QUEUE_SIZE(&rdata->cbq));
#endif

	region = _UDI_CB_TO_CHAN(_UDI_CB_TO_HEADER(rdata->my_cb))->chan_region;
	REGION_LOCK(region);

	pcb = (nsr_cb_t *)NSR_UDI_DEQUEUE_HEAD(&rdata->cbq);
	/*
	 * If there aren't any more Tx CBs available, stop the tx queue.
	 */
	if (NSR_UDI_QUEUE_EMPTY(&rdata->cbq)) {
		netif_stop_queue(devp);
#ifdef NET_NOISY		
		_OSDEP_PRINTF("stopping queue\n");
#endif
		infop->queue_stopped = 1;
		REGION_UNLOCK(region);
		return -1;
	}
	REGION_UNLOCK(region);

	devp->trans_start = jiffies;
	gcb = pcb->cbp;
	NSR_SET_SCRATCH(gcb->scratch, pcb);

	tx_buf = UDI_MCB(gcb, udi_nic_tx_cb_t)->tx_buf;

	if (tx_buf) {
		rdata->n_packets++;
		infop->stats.tx_bytes += tx_buf->buf_size;
		udi_buf_free(tx_buf);
	}

	_udi_buf_extern_init(osnsr_txbuf_alloc, gcb, skb->data, skb->len,
			     &osnsr_skb_xbops, skb, rdata->buf_path);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_start_xmit\n");
#endif
	return 0;
}

STATIC void
osnsr_txbuf_alloc(udi_cb_t *gcb, udi_buf_t *new_buf)
{
	nsrmap_Tx_region_data_t *rdata = gcb->context;
	nsr_cb_t *pcb;
	udi_nic_tx_cb_t *txcb;
	_udi_region_t *region;

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_txbuf_alloc (gcb %p, new_buf %p)\n",
	       gcb, new_buf);
#endif
	NSR_GET_SCRATCH(gcb->scratch, pcb);

	/*
	 * Update pcb->cbp as gcb may have been reallocated 
	 */
	pcb->cbp = gcb;

	txcb = UDI_MCB(gcb, udi_nic_tx_cb_t);

	txcb->tx_buf = new_buf;

	region = _UDI_CB_TO_CHAN(_UDI_CB_TO_HEADER(gcb))->chan_region;
	REGION_LOCK(region);
	NSR_UDI_ENQUEUE_TAIL(&rdata->onq, (udi_queue_t *)pcb);
	REGION_UNLOCK(region);

	udi_nd_tx_req(txcb);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_txbuf_alloc\n");
#endif


}

STATIC void
osnsr_return_stats(udi_cb_t *gcb)
{
	nsrmap_region_data_t *rdata = gcb->context;
	udi_nic_info_cb_t *cb = UDI_MCB(gcb, udi_nic_info_cb_t);
	struct net_device *devp = rdata->osdep_ptr->devp;
	osnsr_info_t *infop = devp->priv;
	struct net_device_stats *sp = &infop->stats;

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_return_stats (cb %p)\n", cb);
#endif

	rdata->next_func = NULL;

	sp->tx_packets = cb->tx_packets;
	sp->rx_packets = cb->rx_packets;
	sp->tx_errors = cb->tx_errors;
	sp->rx_errors = cb->rx_errors;
	sp->tx_dropped += cb->tx_discards;
	sp->rx_dropped += cb->rx_discards;
	sp->collisions = cb->collisions;
	/*
	 * XXX Are these correct?
	 */
	sp->tx_fifo_errors = cb->tx_underrun;
	sp->rx_fifo_errors = cb->rx_overrun;

	udi_cb_free(gcb);

	_OSDEP_SEM_V(&rdata->osdep_ptr->info_sem);

#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_return_stats\n");
#endif
}

STATIC void
osnsr_info_cb(udi_cb_t *gcb, udi_cb_t *new_cb)
{
	nsrmap_region_data_t *rdata = gcb->context;
	udi_nic_info_cb_t *info_cb = UDI_MCB(new_cb, udi_nic_info_cb_t);

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+entered osnsr_info_cb (new_cb %p)\n", new_cb);
#endif

	rdata->next_func = osnsr_return_stats;
	udi_nd_info_req(info_cb, 0);
	
	OSNSR_RETURN_BIND_CB(rdata, gcb);

#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_info_cb\n");
#endif
}

STATIC struct net_device_stats *
osnsr_get_stats(struct net_device *devp)
{
	osnsr_info_t *infop = devp->priv;
	nsrmap_region_data_t *rdata = infop->rdata;

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_get_stats (devp %p)\n", devp);
#endif
	
	if (in_interrupt()) {
#ifdef TRACE_PROC
		_OSDEP_PRINTF("-osnsr_get_stats (return NULL [interrupt])\n");
#endif
		return NULL;
	}

	_OSDEP_SEM_INIT(&rdata->osdep_ptr->info_sem, 0);

	udi_cb_alloc(osnsr_info_cb, OSNSR_GET_BIND_CB(rdata), 
			NSRMAP_INFO_CB_IDX, rdata->bind_info.bind_channel);

#ifdef NET_NOISY
	_OSDEP_PRINTF("osnsr_get_stats: waiting\n");
#endif
	_OSDEP_SEM_P(&rdata->osdep_ptr->info_sem);
	_OSDEP_SEM_DEINIT(&rdata->osdep_ptr->info_sem);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_get_stats\n");
#endif

	return &infop->stats;
}

STATIC void osnsr_process_ctrl_req(udi_nic_ctrl_cb_t *cb);

STATIC void
osnsr_ctrl_req_buf(udi_cb_t *gcb, udi_buf_t *buf)
{
	udi_nic_ctrl_cb_t *cb = UDI_MCB(gcb, udi_nic_ctrl_cb_t);

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_ctrl_req_buf (gcb %p buf %p)\n", gcb, buf);
#endif
	cb->data_buf = buf;
	osnsr_process_ctrl_req(cb);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_ctrl_req_buf\n");
#endif
}

/*
 * xbops for malloc'ed memory
 */

STATIC void
osnsr_mem_free(void *context, udi_ubit8_t *space, udi_size_t size)
{
#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_mem_free (context %p space %p size %ld)\n",
			context, space, size);
#endif
	_OSDEP_MEM_FREE(context);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_mem_free\n");
#endif
	
}

_udi_xbops_t osnsr_mem_xbops = {
	osnsr_mem_free
#ifdef _UDI_PHYSIO_SUPPORTED
		, NULL,
	NULL
#endif					/* _UDI_PHYSIO_SUPPORTED */
};

void
osnsr_dump_mca(char *addr, udi_size_t size)
{
#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_dump_mca (addr %p size %ld)\n", addr, size);
#endif
	switch (size) {
	default:
	case 6:
		_OSDEP_PRINTF("%02X:%02X:%02X:%02X:%02X:%02X",
			      addr[0],
			      addr[1], addr[2], addr[3], addr[4], addr[5]);
		break;
	case 8:
		_OSDEP_PRINTF("%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
			      addr[0],
			      addr[1],
			      addr[2],
			      addr[3], addr[4], addr[5], addr[6], addr[7]);
		break;
	}
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_dump_mca\n");
#endif
}

STATIC void
osnsr_process_ctrl_req(udi_nic_ctrl_cb_t *cb)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;
	struct net_device *devp = rdata->osdep_ptr->devp;
	osnsr_info_t *infop = devp->priv;
	int changed, size;
	struct dev_mc_list *mc, *nmc;
	char *bp;

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_process_ctrl_req (cb %p)\n", cb);
#endif
	/*
	 * Do one operation.
	 */

	if (infop->mc_add || infop->mc_del || infop->mac_addr) {
		if (!cb->data_buf) {
			if (infop->mc_add) {
				/*
				 * size is add_size + add_size + *NEW* current size 
				 */
				size = (infop->add_size << 1) +
					infop->cur_size;
				infop->cur_size += infop->add_size;
			} else if (infop->mc_del) {
				/*
				 * size is del_size + *NEW* current size = old current size
				 */
				size = infop->cur_size;
				infop->cur_size -= infop->del_size;
			} else {
				size = (int)devp->addr_len;
			}
			infop->buf =
				_OSDEP_MEM_ALLOC(size, UDI_MEM_ZERO,
						 UDI_WAITOK);
			_udi_buf_extern_init(osnsr_ctrl_req_buf, UDI_GCB(cb),
					     infop->buf, size,
					     &osnsr_mem_xbops, infop->buf,
					     rdata->buf_path);
#ifdef TRACE_PROC
			_OSDEP_PRINTF("-osnsr_process_ctrl_req\n");
#endif
			return;
		}
		bp = infop->buf;
		if (infop->mc_add || infop->mc_del) {
			cb->indicator = 0;
			if (infop->mc_add) {
#ifdef DEBUG
				_OSDEP_PRINTF("Adding multicast address: ");
				osnsr_dump_mca(infop->mc_add->dmi_addr,
					       infop->mc_add->dmi_addrlen);
				_OSDEP_PRINTF("\n");
#endif
				for (mc = infop->mc_add; mc; mc = nmc) {
					udi_memcpy(bp, mc->dmi_addr,
						   mc->dmi_addrlen);
					bp += mc->dmi_addrlen;
					nmc = mc->next;
					mc->next = infop->mc_cur;
					infop->mc_cur = mc;
					cb->indicator++;
				}
				infop->mc_add = NULL;
				cb->command = UDI_NIC_ADD_MULTI;
			} else {
#ifdef DEBUG
				_OSDEP_PRINTF("Removing multicast address: ");
				osnsr_dump_mca(infop->mc_del->dmi_addr,
					       infop->mc_del->dmi_addrlen);
				_OSDEP_PRINTF("\n");
#endif
				for (mc = infop->mc_del; mc; mc = nmc) {
					udi_memcpy(bp, mc->dmi_addr,
						   mc->dmi_addrlen);
					bp += mc->dmi_addrlen;
					nmc = mc->next;
					kfree(mc);
					cb->indicator++;
				}
				infop->mc_del = NULL;
				cb->command = UDI_NIC_DEL_MULTI;
			}
			/*
			 * Need to add the whole current list at the end.
			 */
			for (mc = infop->mc_cur; mc; mc = mc->next) {
				udi_memcpy(bp, mc->dmi_addr, mc->dmi_addrlen);
				bp += mc->dmi_addrlen;
			}
		} else {
			udi_memcpy(bp, infop->mac_addr, devp->addr_len);
			cb->command = UDI_NIC_SET_CURR_MAC;
			cb->indicator = devp->addr_len;
			kfree(infop->mac_addr);
			infop->mac_addr = NULL;
		}
	} else if ((changed = infop->flags_new ^ infop->flags_current)) {
		if (changed & IFF_PROMISC) {
			if (infop->flags_new & IFF_PROMISC) {
				cb->command = UDI_NIC_PROMISC_ON;
				infop->flags_current |= IFF_PROMISC;
			} else {
				cb->command = UDI_NIC_PROMISC_OFF;
				infop->flags_current &= ~IFF_PROMISC;
			}
		} else if (changed & IFF_ALLMULTI) {
			if (infop->flags_new & IFF_ALLMULTI) {
				cb->command = UDI_NIC_ALLMULTI_ON;
				infop->flags_current |= IFF_ALLMULTI;
			} else {
				cb->command = UDI_NIC_ALLMULTI_OFF;
				infop->flags_current &= ~IFF_ALLMULTI;
			}
		}
	} else {
		/*
		 * All done.
		 */
		udi_cb_free(UDI_GCB(cb));
#ifdef TRACE_PROC
		_OSDEP_PRINTF("-osnsr_process_ctrl_req\n");
#endif
		return;
	}

	/*
	 * Send the request.
	 */

	NSR_ctrl_req(cb);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_process_ctrl_req\n");
#endif
}


/*
 * osnsr_tx_rdy:
 * -------------
 * There are more tx cb's available, wake up the tx queue, if it slept at all.
 */
STATIC void
osnsr_tx_rdy(nsrmap_Tx_region_data_t * rdata)
{
	struct net_device *devp = (struct net_device *)rdata->osdep_tx_ptr;
	osnsr_info_t *infop = devp->priv;

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_tx_rdy (rdata %p)\n", rdata);
#endif
	if (infop->queue_stopped) {
		infop->queue_stopped = 0;
		netif_wake_queue(devp);
#ifdef DEBUG
		_OSDEP_PRINTF("waking queue");
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
		/*
		 * Reschedule the kernel's net queue drainer.
		 */
		mark_bh(NET_BH);
#endif
	}	
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_tx_rdy\n");
#endif
}


/*
 * osnsr_ctrl_ack:
 * --------------
 * Called on completion of an earlier NSR_ctrl_req(). Continue with next
 * request.
 */
STATIC void
osnsr_ctrl_ack(udi_nic_ctrl_cb_t *cb, udi_status_t status)
{

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_ctrl_ack (status %d)\n", status);
#endif

	/*
	 * Free buffer if any.
	 */

	if (cb->data_buf) {
		udi_buf_free(cb->data_buf);
		cb->data_buf = NULL;
	}

	/*
	 * Process next request.
	 */
	osnsr_process_ctrl_req(cb);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_ctrl_ack\n");
#endif
}

STATIC void
osnsr_ctrl_req_cb(udi_cb_t *gcb, udi_cb_t *new_cb)
{
	nsrmap_region_data_t *rdata = gcb->context;
#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_ctrl_req_cb (gcb %p new_cb %p)\n", gcb, new_cb);
#endif
	OSNSR_RETURN_BIND_CB(rdata, gcb);

	osnsr_process_ctrl_req(UDI_MCB(new_cb, udi_nic_ctrl_cb_t));
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_ctrl_req_cb\n");
#endif
}

STATIC void
osnsr_set_multicast_list(struct net_device *devp)
{
	osnsr_info_t *infop = devp->priv;
	nsrmap_region_data_t *rdata = infop->rdata;
	struct dev_mc_list *curmc, *newmc, *nmc;
#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_set_multicast_list (devp %p)\n", devp);
#endif
	/*
	 * First, figure out what we need to change.
	 */

	infop->flags_new = devp->flags & (IFF_ALLMULTI | IFF_PROMISC);

	/*
	 * Compare new multicast list to current and make lists of addrs
	 * to be added/deleted.
	 */

	curmc = infop->mc_cur;
	infop->mc_cur = NULL;
	infop->mc_add = NULL;
	infop->mc_del = NULL;
	infop->cur_size = 0;
	infop->add_size = 0;
	infop->del_size = 0;
	infop->mac_addr = NULL;

	for (; curmc; curmc = nmc) {
		nmc = curmc->next;
		for (newmc = devp->mc_list; newmc; newmc = newmc->next) {
			if (newmc->dmi_addrlen == curmc->dmi_addrlen &&
			    udi_memcmp(newmc->dmi_addr, curmc->dmi_addr,
				       curmc->dmi_addrlen) == 0) {
				break;
			}
		}
		if (newmc) {
			curmc->next = infop->mc_cur;
			infop->mc_cur = curmc;
			infop->cur_size += curmc->dmi_addrlen;
		} else {
			curmc->next = infop->mc_del;
			infop->mc_del = curmc;
			infop->del_size += curmc->dmi_addrlen;
		}
	}

	for (newmc = devp->mc_list; newmc; newmc = newmc->next) {
		for (curmc = infop->mc_cur; curmc; curmc = curmc->next) {
			if (newmc->dmi_addrlen == curmc->dmi_addrlen &&
			    udi_memcmp(newmc->dmi_addr, curmc->dmi_addr,
				       curmc->dmi_addrlen) == 0) {
				break;
			}
		}
		if (!curmc) {
			nmc = kmalloc(sizeof (struct dev_mc_list),
				      in_interrupt()? GFP_ATOMIC : GFP_KERNEL);
			
			/*
			 * Silently fail the set_multicast_list OS request
			 * if we can't get our memory.
			 */
			if (!nmc) {
#ifdef TRACE_PROC 
				_OSDEP_PRINTF("-osnsr_set_multicast_list "
					      "[interrupt, mem alloc fail]\n");
#endif
				return;	
			}

			*nmc = *newmc;
			nmc->next = infop->mc_add;
			infop->mc_add = nmc;
			infop->add_size += nmc->dmi_addrlen;
		}
	}

	/*
	 * Return if nothing to do.
	 */

	if ((infop->flags_new == infop->flags_current)
	    && !infop->mc_add && !infop->mc_del) {
#ifdef TRACE_PROC
		_OSDEP_PRINTF("-osnsr_set_multicast_list [nothing to do]\n");
#endif
		return;
	}

	/*
	 * Allocate a CB
	 */
	udi_cb_alloc(osnsr_ctrl_req_cb, OSNSR_GET_BIND_CB(rdata), 
			NSRMAP_CTRL_CB_IDX, rdata->bind_info.bind_channel);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_set_multicast_list\n");
#endif
}

STATIC int
osnsr_set_mac_address(struct net_device *devp, void *p)
{
	osnsr_info_t *infop = devp->priv;
	nsrmap_region_data_t *rdata = infop->rdata;
	struct sockaddr *addr = (struct sockaddr *)p;

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_set_mac_address (devp %p p %p)\n", devp, p);
#endif
	infop->mac_addr = kmalloc(devp->addr_len, in_interrupt()? GFP_ATOMIC :
				  GFP_KERNEL);

	/*
	 * Fail the OS set mac address request if we can't get the memory for
	 * the mac address.
	 */
	if (!infop->mac_addr) {
#ifdef TRACE_PROC
		_OSDEP_PRINTF("-osnsr_set_mac_address (return -ENOMEM) "
			      "[interrupt, fail mem alloc]\n");
#endif
		return -ENOMEM;
	}

	udi_memcpy(infop->mac_addr, addr->sa_data, devp->addr_len);
	udi_memcpy(devp->dev_addr, addr->sa_data, devp->addr_len);
	infop->mc_add = NULL;
	infop->mc_del = NULL;
	infop->add_size = 0;
	infop->del_size = 0;

	udi_cb_alloc(osnsr_ctrl_req_cb, OSNSR_GET_BIND_CB(rdata), 
			NSRMAP_CTRL_CB_IDX, rdata->bind_info.bind_channel);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_set_mac_address\n");
#endif
	return 0;
}


#ifdef CONFIG_NET_FC

/*
 * For some reason, this isn't in /usr/src/linux/net/802 like the others.
 * This was taken from /usr/src/linux/drivers/fc/iph5526.c.
 */

static unsigned short
fc_type_trans(struct sk_buff *skb, struct net_device *dev)
{
	struct fch_hdr *fch = (struct fch_hdr *)skb->data;
	struct fcllc *fcllc;
#ifdef TRACE_PROC
	_OSDEP_PRINTF("+fc_type_trans (skb %p dev %p)\n", skb, dev);
#endif
	skb->mac.raw = skb->data;
	fcllc = (struct fcllc *)(skb->data + sizeof (struct fch_hdr) + 2);
	skb_pull(skb, sizeof (struct fch_hdr) + 2);

	if (*fch->daddr & 1) {
		if (!memcmp(fch->daddr, dev->broadcast, FC_ALEN))
			skb->pkt_type = PACKET_BROADCAST;
		else
			skb->pkt_type = PACKET_MULTICAST;
	} else if (dev->flags & IFF_PROMISC) {
		if (memcmp(fch->daddr, dev->dev_addr, FC_ALEN))
			skb->pkt_type = PACKET_OTHERHOST;
	}

	/*
	 * Strip the SNAP header from ARP packets since we don't * pass them
	 * through to the 802.2/SNAP layers. 
	 */
	if (fcllc->dsap == EXTENDED_SAP &&
	    (fcllc->ethertype == ntohs(ETH_P_IP) ||
	     fcllc->ethertype == ntohs(ETH_P_ARP))) {
		skb_pull(skb, sizeof (struct fcllc));
#ifdef TRACE_PROC
		_OSDEP_PRINTF("-fc_type_trans [ARP]\n");
#endif
		return fcllc->ethertype;
	}
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-fc_type_trans\n");
#endif
	return ntohs(ETH_P_802_2);
}

#endif /*
        * CONFIG_NET_FC 
        */

/*
 * osnsr_process_rxcb -- create a sk_buff to carry the data in the UDI buffer
 * associated with the CB and hand it off to the OS. The packet is dropped
 * if unable to allocate a sk_buff. The CB is returned when the OS is done
 * with the data.
 */

STATIC void
osnsr_process_rxcb(udi_nic_rx_cb_t *rxcb)
{
	nsrmap_region_data_t *rdata = UDI_GCB(rxcb)->context;
	struct net_device *devp = rdata->osdep_ptr->devp;
	osnsr_info_t *infop = devp->priv;
	udi_buf_t *bp = rxcb->rx_buf;
	udi_size_t size = bp->buf_size;
	struct sk_buff *skb, *new_skb = NULL;
	unsigned char *dp;

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_process_rxcb (rxcb %p)\n", rxcb);
#endif
	_OSDEP_SIMPLE_MUTEX_LOCK(&infop->lock);
	skb = _UDI_XBUF_CONTEXT(bp);
	if (skb && (void *)skb->dev != (void *)&__this_module) {
		/* 
		 * The buffer is not an sk_buff. This can happen if the buffer had 
		 * to be remapped during udi_dma_buf_map and we now got dma memory.
		 * Or, if a driver calls udi_dma_mem_to_buf on our buffer.
		 */

		skb = dev_alloc_skb(size + 2);
		skb_reserve(skb, 2);
		if (skb) {
			skb_put(skb, size);
			udi_buf_read(bp, 0, size, skb->data);
		} else {
			infop->stats.rx_dropped++;
			_OSDEP_SIMPLE_MUTEX_UNLOCK(&infop->lock);
			NSR_return_packet(rxcb);
			return;
		}
	} else {
		new_skb = dev_alloc_skb(size + 2);
		skb_reserve(new_skb, 2);
		if (!new_skb) {
			infop->stats.rx_dropped++;
			_OSDEP_SIMPLE_MUTEX_UNLOCK(&infop->lock);
			NSR_return_packet(rxcb);
			return;
		} else {
			new_skb->dev = (struct net_device *)&__this_module;
		}
		skb_put(skb, size);
		dp = skb->data;
	}
	skb->dev = devp;

#ifdef _NET_NOISY
	{
		int i;

		for (i = 0; i < size; i++)
			_OSDEP_PRINTF("%02x ", dp[i]);
		_OSDEP_PRINTF("\n");
	}
#endif
	switch (rdata->link_info.media_type) {
	case UDI_NIC_ETHER:
	case UDI_NIC_FASTETHER:
	case UDI_NIC_GIGETHER:
		skb->protocol = eth_type_trans(skb, devp);
		break;

#ifdef CONFIG_TR
	case UDI_NIC_TOKEN:
		skb->protocol = tr_type_trans(skb, devp);
		break;
#endif

#ifdef CONFIG_FDDI
	case UDI_NIC_FDDI:
		skb->protocol = fddi_type_trans(skb, devp);
		break;
#endif

#ifdef CONFIG_NET_FC
	case UDI_NIC_FC:
		skb->protocol = fc_type_trans(skb, devp);
		break;
#endif

	default:
		/*
		 * XXX 
		 */
		_OSDEP_SIMPLE_MUTEX_UNLOCK(&infop->lock);
		NSR_return_packet(rxcb);
		return;
	}

	/*
	 * TODO: notify the OS if a packet has already been checksummed.
	 */
	skb->ip_summed = CHECKSUM_NONE;
	
	infop->stats.rx_bytes += size;
	netif_rx(skb);

	if (new_skb) {
		/*
		 * Avoid freeing the skb, this is done in the kernel.
		 */
		_UDI_SET_XBUF_CONTEXT(bp, NULL);
		udi_buf_free(bp);
		_OSDEP_SIMPLE_MUTEX_UNLOCK(&infop->lock);
		_udi_buf_extern_init(osnsr_process_rxcb2, UDI_GCB(rxcb),
				     new_skb->data, size, 
				     &osnsr_skb_xbops, (void *)new_skb,
				     rdata->osdep_ptr->path_handle);
	} else {
		_OSDEP_SIMPLE_MUTEX_UNLOCK(&infop->lock);
		NSR_return_packet(rxcb);
	}

#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_process_rxcb\n");
#endif
}

STATIC void
osnsr_process_rxcb2(udi_cb_t *gcb, udi_buf_t *buf)
{
	udi_nic_rx_cb_t *rxcb = UDI_MCB(gcb, udi_nic_rx_cb_t);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_process_rxcb2 (gcb %p buf %p)\n", gcb, buf);
#endif
	rxcb->rx_buf = buf;
	NSR_return_packet(rxcb);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_process_rxcb2\n");
#endif
}

/*
 * osnsr_rx_packet -- called by common code to process a chain
 * of received packets.
 */

STATIC void
osnsr_rx_packet(udi_nic_rx_cb_t *rxcb, udi_boolean_t expedited)
{
	nsrmap_region_data_t *rdata = UDI_GCB(rxcb)->context;
	udi_nic_rx_cb_t *tcb, *next;
	nsr_cb_t *pcb;

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_rx_packet (rxcb %p expedited %d)\n", rxcb,
			expedited);
#endif

	udi_assert(rdata->rdatap == rdata);
	rdata->n_packets++;

	for (tcb = rxcb; tcb; tcb = next) {

		next = tcb->chain;
		tcb->chain = NULL;

		/*
		 * Get a container for this request from the list of owned 
		 * containers. This will be moved back to the rxonq when the 
		 * packet is returned to the NSR via NSR_return_packet. 
		 * Until that time the container is in limbo and can only be 
		 * accessed via rxcb->gcb.scratch
		 */

		NSRMAP_ASSERT(!NSR_UDI_QUEUE_EMPTY(&rdata->rxonq));
		pcb = (nsr_cb_t *)NSR_UDI_DEQUEUE_HEAD(&rdata->rxonq);
		pcb->cbp = UDI_GCB(tcb);
		NSR_SET_SCRATCH(pcb->cbp->scratch, pcb);

		osnsr_process_rxcb(tcb);
	}
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_rx_packet\n");
#endif
}

/*
 * osnsr_unbind_done-- remove net device from system and free memory. 
 */

STATIC void
osnsr_unbind_done(udi_nic_cb_t *cb)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;
	struct net_device *devp = rdata->osdep_ptr->devp;
	osnsr_info_t *infop = devp->priv;

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_unbind_done (cb %p)\n", cb);
#endif
	/*
	 * The unregister causes get_stats to be called, which won't work
	 * since we're unbound.
	 */

	devp->get_stats = NULL;
	unregister_netdev(devp);
	_OSDEP_MUTEX_DEINIT(&infop->lock);
	udi_mem_free(devp);

	/*
	 * decrement the mapper module's usage count, so it can be
	 * unloaded from now on
	 */
	MOD_DEC_USE_COUNT;
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_unbind_done\n");
#endif
}

/*
 * osnsr_final_cleanup:
 * ----------
 * Undo things done in osnsr_init.
 */

STATIC void
osnsr_final_cleanup(udi_mgmt_cb_t *cb)
{
#ifdef TRACE_PROC
	_OSDEP_PRINTF("osnsr_final_cleanup (cb %p)\n", cb);
#endif
}

STATIC void osnsr_got_rxcb(udi_cb_t *, udi_cb_t *);
STATIC void osnsr_got_rxbuf(udi_cb_t *, udi_buf_t *);

STATIC void
osnsr_cb_alloc_batch(udi_cb_alloc_batch_call_t *callback, udi_cb_t *gcb,
		     nsrmap_region_data_t * rdata)
{
	udi_index_t count = rdata->link_info.rx_hw_threshold;
	udi_size_t buf_size = rdata->buf_size + rdata->trailer_len;
	udi_buf_path_t path_handle = rdata->parent_buf_path;
	netmap_osdep_t *osdp = rdata->osdep_ptr;

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_cb_alloc_batch (callback %p gcb %p rdata %p)\n",
			callback, gcb, rdata);
#endif

	_OSDEP_ASSERT(count != 0);

	osdp->callback = callback;
	osdp->count = count;
	osdp->buf_size = buf_size;
	osdp->path_handle = path_handle;
	osdp->orig_context = gcb->context;
	gcb->context = osdp;
	osdp->rxcb_chain1 = NULL;
	udi_cb_alloc(osnsr_got_rxcb, gcb, NSRMAP_RX_CB_IDX, gcb->channel);
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_cb_alloc_batch\n");
#endif
}

STATIC void
osnsr_got_rxcb(udi_cb_t *gcb, udi_cb_t *new_cb)
{
	netmap_osdep_t *osdp = gcb->context;
	udi_nic_rx_cb_t *rxcb = UDI_MCB(new_cb, udi_nic_rx_cb_t);
	struct sk_buff *skb;

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_got_rxcb (gcb %p new_cb %p)\n", gcb, new_cb);
#endif

	rxcb->chain = osdp->rxcb_chain1;
	osdp->rxcb_chain1 = rxcb;
	if (--osdp->count)
		udi_cb_alloc(osnsr_got_rxcb, gcb, NSRMAP_RX_CB_IDX,
			     gcb->channel);
	else {
		osdp->rxcb_chain2 = NULL;
		skb = dev_alloc_skb(osdp->buf_size + 2);
		_OSDEP_ASSERT(skb);
		skb_reserve(skb, 2);
		skb->dev = (struct net_device *)&__this_module;
		_udi_buf_extern_init(osnsr_got_rxbuf, gcb, skb->data, 
				     osdp->buf_size, &osnsr_skb_xbops,
				     (void *)skb, osdp->path_handle);

	}
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_got_rxcb\n");
#endif
}

STATIC void
osnsr_got_rxbuf(udi_cb_t *gcb, udi_buf_t *buf)
{
	netmap_osdep_t *osdp = gcb->context;
	udi_nic_rx_cb_t *rxcb;
	struct sk_buff *skb;
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);

#ifdef TRACE_PROC
	_OSDEP_PRINTF("+osnsr_got_rxbuf (gcb %p buf %p)\n", gcb, buf);
#endif

	rxcb = osdp->rxcb_chain1;
	osdp->rxcb_chain1 = osdp->rxcb_chain1->chain;
	rxcb->chain = osdp->rxcb_chain2;
	osdp->rxcb_chain2 = rxcb;
	rxcb->rx_buf = buf;

	if (osdp->rxcb_chain1) {
		skb = dev_alloc_skb(osdp->buf_size + 2);
		_OSDEP_ASSERT(skb);
		skb_reserve(skb, 2);
		_OSDEP_ASSERT(skb);
		skb->dev = (struct net_device *)&__this_module;
		_udi_buf_extern_init(osnsr_got_rxbuf, gcb, skb->data, 
				     osdp->buf_size, &osnsr_skb_xbops,
				     (void *)skb, osdp->path_handle);

	} else {
		gcb->context = osdp->orig_context;
		SET_RESULT_UDI_CT_CB_ALLOC(&cb->cb_callargs.allocm,
					   UDI_GCB(osdp->rxcb_chain2));
		cb->cb_flags |= _UDI_CB_CALLBACK;
		osdp->rxcb_chain2 = NULL;
		cb->cb_func = (_udi_callback_func_t *)osdp->callback;
		_udi_queue_callback(cb);
	}
#ifdef TRACE_PROC
	_OSDEP_PRINTF("-osnsr_got_rxbuf\n");
#endif
}

STATIC void
mapper_init()
{
#ifdef TRACE_PROC
	_OSDEP_PRINTF("mapper_init\n");
#endif
}

STATIC void
mapper_deinit()
{
#ifdef TRACE_PROC
	_OSDEP_PRINTF("mapper_deinit\n");
#endif
}

/*
 * Common Code:
 */
#include "niccommon/netmap.c"
