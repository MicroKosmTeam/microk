/*
 * File: freebsd/net/netmap_freebsd.c
 *
 * FreeBSD-specific network mapper code
 *
 * We provide the FreeBSD entry points, which are implemented by calls
 * into the common net mapper code, and the osnsr_* entry points
 * called by the common net mapper code.
 */

/* und: UDI NIC DRIVER */

/*
 * Copyright (c) 2001  Kevin Van Maren.  All rights reserved.
 *
 * $Copyright udi_reference:
 * $
 */

#define UDI_MEM_ZERO 0

#include <udi.h>
#include <udi_env.h>
#include <udi_nic.h>

/* Standard FreeBSD network driver includes */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#include <net/bpf.h>

#include <sys/sockio.h>
#include <sys/bus.h>
#include <machine/bus.h>
#include <sys/rman.h>
#include <machine/resource.h>

#include <net/ethernet.h>
#include <net/if_arp.h>

#include "netmap_freebsd.h"
#include "netmap.h"


/*
 * *************************************************************************
 * FreeBSD "ifp" entry points
 * *************************************************************************
 */

static void und_init(void *);
static void und_start(struct ifnet *);
static int und_ioctl(struct ifnet *, u_long, caddr_t);

/*
 * FreeBSD "media" entry points
 */
static int und_mediachange(struct ifnet *);
static void und_mediastatus(struct ifnet *, struct ifmediareq *);

/* XXX: Get media list from parent UDI driver */
static const int und_media_default[] = {
	IFM_ETHER|IFM_10_T,
	IFM_ETHER|IFM_10_T|IFM_FDX,
	IFM_ETHER|IFM_100_TX,
	IFM_ETHER|IFM_100_TX|IFM_FDX,
	IFM_ETHER|IFM_1000_TX,
	IFM_ETHER|IFM_1000_TX|IFM_FDX,
	IFM_ETHER|IFM_AUTO,
};


/* XXX: Use UDI _OSDEP locking calls */

/*
 * Internal helper functions
 */
static void my_stop(struct und_softc *sc);
static void my_setfilt(struct und_softc *sc);
static void my_setmedia(struct und_softc *sc, int media);

/*
 * *************************************************************************
 * Routines exported by the common netmapper code
 * *************************************************************************
 */

static void NSR_enable(udi_cb_t *gcb, void (*cbfn)(udi_cb_t *, udi_status_t));
static void NSR_disable(udi_cb_t *gcb, void (*cbfn)(udi_cb_t *, udi_status_t));
static void NSR_return_packet(udi_nic_rx_cb_t *cb);
static void NSR_ctrl_req(udi_nic_ctrl_cb_t *cb);
static nsrmap_Tx_region_data_t *NSR_txdata(nsrmap_region_data_t *rdata);

/*
 * *************************************************************************
 * Routines exported to the common netmapper code
 * *************************************************************************
 */

static void osnsr_init(nsrmap_region_data_t *);
static udi_ubit32_t osnsr_max_xfer_size(udi_ubit8_t);
static udi_ubit32_t osnsr_min_xfer_size(udi_ubit8_t);
static void osnsr_rx_packet(udi_nic_rx_cb_t *, udi_boolean_t);
static void osnsr_bind_done(udi_channel_event_cb_t *, udi_status_t);
static void osnsr_unbind_done(udi_nic_cb_t *);
static void osnsr_ctrl_ack(udi_nic_ctrl_cb_t *, udi_status_t);
static void osnsr_final_cleanup(udi_mgmt_cb_t *);
static udi_ubit32_t osnsr_mac_addr_size(udi_ubit8_t);
static void osnsr_status_ind(udi_nic_status_cb_t *);



/*
 * *************************************************************************
 * ifp helper routines
 * *************************************************************************
 */


/*
 * This callback gets executed after the ND has completed the
 * request.  The process-level thread that initiated the request
 * is woken and allowed to continue.
 */
static void
my_enable_disable_callback(udi_cb_t *gcb, udi_status_t status)
{
	nsrmap_region_data_t *rdata = gcb->context;
	struct und_softc *sc = rdata->osdep_ptr;

	sc->enable_cb = gcb;
#if 0
	sc->enable_status = status;
#endif
	/* Wake up the blocked caller */
	_OSDEP_EVENT_SIGNAL(&sc->enable_ev);

	/* XXX: if enable/disable pending, process it now */
}


/*
 * Ok, need to update the filtering on the hardware.
 */
static void
my_setfilt(struct und_softc *sc)
{
	nsrmap_region_data_t *rdata = sc->rdata;
	struct ifnet *ifp = &sc->arpcom.ac_if;
#if 0
	int all_mcasts = (ifp->if_flags & IFF_ALLMULTI) ? 1 : 0;
	int promisc = (ifp->if_flags & IFF_PROMISC) ? 1 : 0;
#endif
	udi_nic_ctrl_cb_t *cb = sc->setfilt_cb;

	udi_debug_printf("in my_setfilt\n");

	/* XXX: Force promisc setting hack */
	ifp->if_flags |= IFF_PROMISC;

	if (rdata == NULL)
		udi_debug_printf("Null rdata here\n");

	udi_assert(rdata->link_info.mac_addr_len == 6);

	if (cb == NULL) {
		/* XXX: Mark as needing another update */
		return;
	}

	/*
	 * Need to "cache" what we tell the ND in our softc
	 * structure, since updating the multicast list is a
	 * stateful operation, and requires info about the previous
	 * state.
	 */
	/* XXX: look at Linux's osnsr_set_multicast_list, or UW equiv */
#if 0
	rdata->link_info.max_perfect_multicast
#endif

	/*
	 * For now, just punt and run in promiscuous mode!
	 * We can fix it "for real" later...
	 */
	cb->command = UDI_NIC_PROMISC_ON;
	cb->indicator = 0;	/* unused */
	cb->data_buf = NULL;

	/* Issue software reset to the driver */
	sc->setfilt_cb = NULL;
	NSR_ctrl_req(cb);
}


/*
 * Ok, change the media the nic is using.
 */
static void
my_setmedia(struct und_softc *sc, int media)
{
	udi_debug_printf("in my_setmedia (%d).  Ignoring.\n", media);

	/*
	 * XXX:
	 * The way to to do this is to update the persistent static
	 * driver properties for the UDI parent driver, and then send
	 * it an enable() request to force it to rescan them and
	 * adjust the media accordingly.
	 * We don't do that yet...
	 */
}


/*
 * Stop the interface. Cancels the statistics updater and resets
 * the interface.
 */
static void
my_stop(struct und_softc *sc)
{
	udi_cb_t *gcb = sc->enable_cb;
	struct ifnet *ifp = &sc->arpcom.ac_if;

	if (!gcb) {
		/* Can't do it... no cb... flag to do later */
		udi_debug_printf("und: no enable_cb\n");
		/* XXX */
		return;
	}
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
#if 0
	ifp->if_timer = 0;

	/* Cancel stats updater. */
	udi_timer_cancel();
	/* keep stats cb?  timer cb? */
	udi_cb_free(???);

#endif

	/* Issue software reset to the driver */
	sc->enable_cb = NULL;
	NSR_disable(gcb, my_enable_disable_callback);

	/* Block until the request has completed */
	_OSDEP_EVENT_WAIT(&sc->enable_ev);
	ifp->if_flags &= ~IFF_RUNNING;
}

/*
 * *************************************************************************
 * FreeBSD "ifp" entry points
 * *************************************************************************
 */

/*
 * Reset and initialize the hardware.
 */
static void
und_init(void *xsc)
{
	struct und_softc *sc = xsc;
	udi_cb_t *gcb = sc->enable_cb;
	struct ifnet *ifp = &sc->arpcom.ac_if;

	/* XXX */

	if (!gcb) {
		/* Can't do it... no cb... flag to do later */
		udi_debug_printf("und: und_init no enable_cb\n");
		/* XXX */
		return;
	}
#if 0
	my_setmedia(sc, sc->sc_media.ifm_cur->ifm_media);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/* Start stats updater. */
	sc->stat_ch = timeout(und_stats_update, sc, hz);
#endif

	sc->enable_cb = NULL;
	NSR_enable(gcb, my_enable_disable_callback);
	/* Block until the request has completed */
	_OSDEP_EVENT_WAIT(&sc->enable_ev);
	ifp->if_flags |= IFF_RUNNING;

	/* XXX: Update mcast list now? */
}

static void
my_mbuf_free(void *mbuf, udi_ubit8_t *mem, udi_size_t size)
{
	/*
	 * Note that each *piece* of an mbuf chain is freed independently
	 * by the common code... so call m_free(), not m_freem()
	 */
	m_free(mbuf);
}


static _udi_xbops_t mbuf_xbops = { my_mbuf_free, NULL, NULL };

typedef struct {
	void *next;
	void *pcb;
} xmit_scratch_t;

/*
 * We've been "kicked" by the OS to send more packets (if possible)
 * This piece (xmit path) runs with the TX rdata.
 * Rest of the file runs with the RX/ctrl (normal) rdata.
 */
static void und_start_1(udi_cb_t *gcb, udi_buf_t *new_buf);

static void
und_start(struct ifnet *ifp)
{
	struct und_softc *sc = ifp->if_softc;
	struct und_cb_tx *txp;
	nsrmap_Tx_region_data_t *txrdata = sc->txrdata;
	xmit_scratch_t *scratch;

	txp = NULL;

	/* NSR_UDI_QUEUE_SIZE(&txrdata->cbq) */

	udi_assert(txrdata != NULL);

	/* While there are more packets to be transmitted... */
	while (ifp->if_snd.ifq_head != NULL) {
		udi_cb_t *gcb;
		struct mbuf *mb_head;
		nsr_cb_t *pcb; /* ??? */

		/*
		 * XXX: Why am I doing this?  The control blocks
		 * contain chaining -- so why is there an extra
		 * queue of chained control blocks?
		 */
		/* Dequeue the next xmit control block */
		pcb = (nsr_cb_t *)NSR_UDI_DEQUEUE_HEAD(&txrdata->cbq);
		if (pcb == NULL) {
			/* Busy; don't bother us again */
			ifp->if_flags |= IFF_OACTIVE;
			return;
		}
		gcb = pcb->cbp;

		NSR_SET_SCRATCH(gcb->scratch, pcb);	/* why? */

		/* Grab the next packet to transmit. */
		IF_DEQUEUE(&ifp->if_snd, mb_head);

		/* Pass packet to bpf if there is a listener. */
		if (ifp->if_bpf)
			bpf_mtap(ifp, mb_head);

		if (mb_head->m_len == 0) {
			udi_debug_printf("xmit empty first piece\n");
			/* Will the env do the right thing for this case? */
		}

		/* XXX: Check scratch size for the gcb */
		scratch = gcb->scratch;
		scratch->next = mb_head->m_next;
		scratch->pcb = pcb;

		/* Create a udi buffer from the mbuf chain */
		_udi_buf_extern_init(und_start_1, gcb,
			mtod(mb_head, void *), mb_head->m_len,
			&mbuf_xbops, mb_head, txrdata->buf_path);
	}
	ifp->if_flags &= ~IFF_OACTIVE;
}


static void
und_start_1(udi_cb_t *gcb, udi_buf_t *new_buf)
{
	nsrmap_Tx_region_data_t *txrdata = gcb->context;
	struct mbuf *mb_next, *m;
	udi_nic_tx_cb_t *txcb;
	nsr_cb_t *pcb;
	xmit_scratch_t *scratch = gcb->scratch;

	mb_next = scratch->next;
	pcb = scratch->pcb;

	/*
	 * Issue calls to extend the external buffer chain.
	 * Note: _udi_buf_extern_add calls blocking memory allocations.
	 * There is no fundumental reason this could not be performed
	 * asynch as we iterate over the bmuf (state in scratch).
	 */
	/* Go through the mbuf, and copy each piece... */
	for (m = mb_next; m != NULL; m = m->m_next) {
		void *src;
		int size;

		while ((m) && (m->m_len == 0)) {
			/* free 0-byte mbuf piece now */
#if 0
			udi_debug_printf("xmit 0 byte mbuf piece\n");
#endif
			m = m_free(m);
		}
		if (m == NULL)
			break;
		src = mtod(m, void *);
		size = m->m_len;

		/* Does not return a new udi_buf_t? */
		_udi_buf_extern_add(new_buf, src, size,
			&mbuf_xbops, m);
	}

	/* Update pcb->cbp as gcb may have been reallocated? */
	pcb->cbp = gcb;

	txcb = UDI_MCB(gcb, udi_nic_tx_cb_t);
	txcb->tx_buf = new_buf;
	txcb->chain = NULL;

	NSR_UDI_ENQUEUE_TAIL(&txrdata->onq, (udi_queue_t *)pcb);

	udi_nd_tx_req(txcb);
}


/*
 * OS wants to set something, or get some info.
 * It's so demanding that way...
 */
static int
und_ioctl(struct ifnet *ifp, u_long command, caddr_t data)
{
	struct und_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int error = 0;

	switch(command) {
	case SIOCSIFADDR:
	case SIOCGIFADDR:
	case SIOCSIFMTU:
		error = ether_ioctl(ifp, command, data);
		break;
	case SIOCSIFFLAGS:
		/* Do we need to kick it, set filtering, or stop it? */
		if (ifp->if_flags & IFF_UP) {
			if (ifp->if_flags & IFF_RUNNING) {
				my_setfilt(sc);
			} else {
				und_init(sc);
			}
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				my_stop(sc);
		}
		error = 0;
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		my_setfilt(sc);
		error = 0;
		break;
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, command);
		break;
	default:
		error = EINVAL;
		break;
	}

	return(error);

}


/*
 * *************************************************************************
 * FreeBSD media entry points
 * *************************************************************************
 */

/*
 * Set the media
 */
static int
und_mediachange(struct ifnet *ifp)
{

	struct und_softc *sc = ifp->if_softc;
	struct ifmedia *ifm = &sc->sc_media;

	/* XXX */
	if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
		return (EINVAL);

#if 0
	my_setmedia(sc, ifm->ifm_media);
#endif
	return (0);

}


/*
 * Get the current media
 */
static void
und_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct fxp_softc *sc = ifp->if_softc;
#if 0
	int flags, stsflags;

	ifmr->ifm_status = IFM_AVALID;
	ifmr->ifm_status = IFM_ACTIVE;

	IFM_AUTO

	IFM_HDX

	ifmr->ifm_active = IFM_ETHER;
	ifmr->ifm_active |= IFM_100_TX;
	ifmr->ifm_active |= IFM_10_T;
	ifmr->ifm_active |= IFM_FDX;
#endif
}


/*
 * *************************************************************************
 * Routines exported to the common netmapper code.
 * *************************************************************************
 */

/*
 * Scratch gcb's allocated internally.
 */
#define HAVE_GCB_LIST 1
static udi_gcb_init_t nsr_gcb_init_list[] = {
	{
		NSRMAP_NIC_ENABLE_CB_IDX, 0
	},
	{ 0, }
};

/* rdata->osdep_ptr == softc   What about dev?  */

/*
 * Called from the common mapper's first usage_ind.  Has no cb :-(
 */
static void
osnsr_init(nsrmap_region_data_t *rdata)
{
	struct und_softc *sc;

	rdata->osdep_ptr = (netmap_osdep_t *)&rdata->osdep;
	sc = rdata->osdep_ptr;

	udi_debug_printf("UND: osnsr_init\n");
	_OSDEP_EVENT_INIT(&sc->enable_ev);

	sc->rdata = rdata;
	udi_assert(sc->rdata != NULL);

#if 0
	/* Not available yet.. */
	sc->txrdata = NSR_txdata(rdata);
	udi_assert(sc->txrdata != NULL);
#endif
	/* Would be the perfect place to allocate our scratch enable cb... */
}


/*
 * Binding is complete (success of failure).
 * The interface is now ready to be prodded by the OS.
 */
static udi_cb_alloc_call_t osnsr_bind_done_1;
static udi_cb_alloc_call_t osnsr_bind_done_2;

static void
osnsr_bind_done(udi_channel_event_cb_t *cb, udi_status_t status)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;
	nsrmap_Tx_region_data_t *txrdata;
	struct und_softc *sc = rdata->osdep_ptr;

	/*
	 * XXX: Bind is done, but we don't get the bind_cb?
	 * So we have to `steal' the info by groking the common code.
	 */

	/* We must set this up before we try to transmit */
	sc->txrdata = NSR_txdata(rdata);
	udi_assert(sc->txrdata != NULL);

	/* XXX: we should use the separate osdep tx space */
	txrdata = sc->txrdata;
	txrdata->osdep_tx_ptr = sc;

	/* XXX: Assert only called once per instance */

	udi_assert(status == UDI_OK);
	/* How do we deal with bad status? */
	if (status != UDI_OK) {
		udi_channel_event_complete(cb, status);
		return;
	}

	udi_debug_printf("UDI: osnsr_bind_done\n");

	/* Alloc control cb */
	udi_cb_alloc(osnsr_bind_done_1, UDI_GCB(cb), NSRMAP_CTRL_CB_IDX,
		rdata->bind_info.bind_channel);
}

static void
osnsr_bind_done_1(udi_cb_t *gcb, udi_cb_t *new_cb)
{
	nsrmap_region_data_t *rdata = gcb->context;
	struct und_softc *sc = rdata->osdep_ptr;

	sc->setfilt_cb = UDI_MCB(new_cb, udi_nic_ctrl_cb_t);

	/* Alloc enable/disable gcb */
	udi_cb_alloc(osnsr_bind_done_2, gcb, NSRMAP_NIC_ENABLE_CB_IDX,
		UDI_NULL_CHANNEL);
}

static void
osnsr_bind_done_2(udi_cb_t *gcb, udi_cb_t *new_cb)
{
	nsrmap_region_data_t *rdata = gcb->context;
	struct und_softc *sc = rdata->osdep_ptr;
	struct ifnet *ifp = &sc->arpcom.ac_if;
	const int *media;
	int defmedia, nmedia, i;

	sc->enable_cb = new_cb;

#if 0
	ifp->if_unit = device_get_unit(dev);
#else
	/* XXX: Get instance number */
#endif
	ifp->if_name = "und";


	/* Initialize media */
	/* XXX */
	media = und_media_default;
	nmedia = sizeof(und_media_default) / sizeof(und_media_default[0]);
	defmedia = IFM_ETHER|IFM_AUTO;

	ifmedia_init(&sc->sc_media, 0, und_mediachange, und_mediastatus);
	for (i = 0; i < nmedia; i++) {
		ifmedia_add(&sc->sc_media, media[i], 0, NULL);
	}
	ifmedia_set(&sc->sc_media, defmedia);

	/* Get the MAC addr */
	udi_memcpy(sc->arpcom.ac_enaddr, rdata->link_info.mac_addr, 6
		/* addr_len */);

#if 0
	device_printf(dev, "Ethernet address %6D\n",
		sc->arpcom.ac_enaddr, ":");
#else
	printf("und%d: Ethernet address %6D\n", ifp->if_unit,
		sc->arpcom.ac_enaddr, ":");
#endif

	/* XXX */
	ifp->if_output = ether_output;
	ifp->if_baudrate = 100000000; /* XXX */
	ifp->if_init = und_init;
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = und_ioctl;
	ifp->if_start = und_start;
	/* ifp->if_watchdog = XXX */

	/* read this: */
	/* und->if_multiaddrs; */
	ifp->if_snd.ifq_maxlen = IFQ_MAXLEN;	/* XXX */

	ether_ifattach(ifp, ETHER_BPF_SUPPORTED);

	/* ether_ifattach sets MTU to 1514; adjust it if necessary */
	if (rdata->link_info.max_pdu_size != 1514) {
		ifp->if_mtu = rdata->link_info.max_pdu_size;
		udi_debug_printf("und: setting MTU to %d\n", ifp->if_mtu);
	}

#if 0	/* XXX: Ethernet only */
	if (rdata->link_info.mac_addr_len != 6) {
		ifp->if_addrlen = rdata->link_info.mac_addr_len;
	}
	ifp->if_hdrlen
#endif

	/* ifp->if_snd.ifq_maxlen = XXX; */
	udi_channel_event_complete(UDI_MCB(gcb, udi_channel_event_cb_t),
		UDI_OK);
}

/*
 * Unbind request has been processed, as a result of devmgmt request.
 * Network driver is now gone.  Remove net device from system and free memory.
 */
static void
osnsr_unbind_done(udi_nic_cb_t *cb)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;
	struct und_softc *sc = rdata->osdep_ptr;

	udi_debug_printf("und XX: osnsr_unbind_done\n"); /* ifp->if_unit */

	/* ND has been disconnected by the MA */

	/* Detach from the stack */
	ether_ifdetach(&sc->arpcom.ac_if, ETHER_BPF_SUPPORTED);

	/* XXX: Free all our allocated memory */

	ifmedia_removeall(&sc->sc_media);
}

/*
 * Asynchronous notification of a network status change:
 * link up, down, or reset.
 */
static void
osnsr_status_ind(udi_nic_status_cb_t *cb)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;

	udi_debug_printf("und XX: osnsr_status_ind\n");

	/*
	 * XXX: Adjust our status based on the event type.
	 */
	switch (cb->event) {
	case UDI_NIC_LINK_RESET:
		udi_debug_printf("Link reset\n");
		break;
	case UDI_NIC_LINK_DOWN:
		udi_debug_printf("Link down\n");
		break;
	case UDI_NIC_LINK_UP:
		udi_debug_printf("Link up\n");
		break;
	default:
		udi_debug_printf("osnsr_status_ind: Unknown event (%d)\n",
			cb->event);
	}
}

/*
 * Called from nsrmap_ctrl_ack as a result of our performing a
 * NSR_ctrl_req() operation.
 */
static void
osnsr_ctrl_ack(udi_nic_ctrl_cb_t *cb, udi_status_t status)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;
	struct und_softc *sc = rdata->osdep_ptr;

	udi_debug_printf("und XX: osnsr_ctrl_ack (%d)\n", status);

	sc->setfilt_cb = cb;
	/* XXX: See if we need to update the filtering again */
}


/*
 * The osnsr_rx_packet() is responsible for dequeueing containers from
 * Rx onq and for handling expedited vs non-expedited requests.
 * The containers will be returned to Rx cbq by NSR_return_packet
 */
static void
osnsr_rx_packet(udi_nic_rx_cb_t *rxcb, udi_boolean_t expedited)
{
	nsrmap_region_data_t *rdata = UDI_GCB(rxcb)->context;
	nsr_cb_t *pcb;
	udi_nic_rx_cb_t *tcb;
	struct und_softc *sc = rdata->osdep_ptr;
	struct ifnet *ifp = &sc->arpcom.ac_if;

#if 0
	udi_debug_printf("und XX: osnsr_rx_packet\n");
#endif

	/*
	 * XXX: Why am I doing this?  Why are we using a queue
	 * instead of the built-in cb chaining?
	 */
	pcb = (nsr_cb_t *)NSR_UDI_DEQUEUE_HEAD(&rdata->rxonq);
	udi_assert(pcb);
	pcb->cbp = UDI_GCB(rxcb);
	/* added this line... */
	NSR_SET_SCRATCH(pcb->cbp->scratch, pcb);

	/*
	 * XXX: We should keep a pool of mbufs pre-allocated for
	 * receive processing.
	 */
	for (tcb = rxcb; tcb; tcb = tcb->chain) {
		udi_buf_t *buf = tcb->rx_buf;
		udi_size_t size = buf->buf_size;
		struct mbuf *m;
		struct ether_header *eh;

		if (tcb->chain)
			printf("rx chain\n");
		/* XXX: FIXME */
		if (size > MCLBYTES) {
			udi_debug_printf("discard jumbo (%d)\n", size);
			continue;
		}

		/* We "know" that it must be at least 60, but we do need the header */
		if (size < 14) {
			udi_debug_printf("discard runt (%d)\n", size);
			continue;
		}

		/*
		 * We should use external mbufs, but it looks like it
		 * will be a hassle: we only get the pointer and size
		 * for free (no cookie/handle for the udi_buf_t).
		 * NetBSD has a void * paramter for external frees.
		 *
		 * m_devget will allocate cluster mbufs as necessary,
		 * *and* call our copy routine.  Can't beat that...
		 * except we don't have a cookie for our udi_buf_t.  Oops.
		 */

		/* Allocate an mbuf, and do udi_buf_read() into it */

		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
			udi_debug_printf("MGETHDR failed\n");
			/* um...skip the rest of these packets */

			/*
			 * TODO: Save state and resume RX processing later
			 * instead of throwing out these packets.
			 */
			break;
		}
		/*
		 * We only deal with a single mbuf for now.
		 * Normally BSD uses cluser mbufs only if it won't fit
		 * in two regular mbufs.
		 */
#if 0
		if (len >= MINCLSIZE) {
#endif
			MCLGET(m, M_DONTWAIT);
			if (m->m_ext.ext_buf == NULL) {
				printf("NULL ext_buf\n");
				/* oops */
				m_free(m);
#if 1
				m = NULL;
#endif
				break;
			}
#if 0
		} else {
			/* XXX */
		}
#endif
		m->m_pkthdr.rcvif = ifp;

		/* XXX: Ethernet only */

		/* XXX: ELIMINATE THIS COPY */
		udi_buf_read(buf, 0, size, m->m_data);
		m->m_pkthdr.len = m->m_len = size;

		eh = mtod(m, struct ether_header *);
#if 0
		/* XXX: Force align the payload */
#endif
		/* Remove Ethernet header from mbuf and pass it on. */
		m_adj(m, sizeof(struct ether_header));

		ether_input(ifp, eh, m);
	}
	/* Return all the control blocks in one chunk */
	NSR_return_packet(rxcb);
}

/*
 * Called from the common code when we get more control blocks.
 * If we have IFF_OACTIVE set, we know that we need to use them.
 */
static void
osnsr_tx_rdy(nsrmap_Tx_region_data_t *txrdata)
{
	struct und_softc *sc = txrdata->osdep_tx_ptr;
	struct ifnet *ifp = &sc->arpcom.ac_if;

	/* See if we have to use these new cbs */
	if (ifp->if_flags & IFF_OACTIVE) {
		und_start(ifp);
	}
}

/*
 * Called from the common code's final_cleanup_req entry point.
 */
static void
osnsr_final_cleanup(udi_mgmt_cb_t *cb)
{
	udi_debug_printf("und XX: osnsr_final_cleanup\n");
}


/*
 * Returns the maximum packet size for a given media type.
 * This is only called if the driver does not specify a size.
 */
static udi_ubit32_t
osnsr_max_xfer_size(udi_ubit8_t media_type)
{
	/* XXX */
	switch (media_type) {
	case UDI_NIC_ETHER:
	case UDI_NIC_FASTETHER:
	case UDI_NIC_GIGETHER:
		return 1514;
	default:
		return 1514;
	}
}

/*
 * Returns the minimum packet size for a given media type.
 * This is only called if the driver does not specify a size.
 */
static udi_ubit32_t
osnsr_min_xfer_size(udi_ubit8_t media_type)
{
	/* XXX */
	switch (media_type) {
	case UDI_NIC_ETHER:
	case UDI_NIC_FASTETHER:
	case UDI_NIC_GIGETHER:
		return 14;
	default:
		return 14;
	}
}

/*
 * Returns the link level header size.
 * Only called if the network driver did not specify one.
 */
static udi_ubit32_t
osnsr_mac_addr_size(udi_ubit8_t media_type)
{
	/* XXX */
	switch (media_type) {
	case UDI_NIC_ETHER:
	case UDI_NIC_FASTETHER:
	case UDI_NIC_GIGETHER:
		return 6;
	default:
		return 6;
	}
}

/*
 * *************************************************************************
 */



/*
 * Pull in the common code:
 */
#include "netcommon/netmap.c"

