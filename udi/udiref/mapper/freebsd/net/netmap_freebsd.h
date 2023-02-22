/*
 * File: freebsd/net/netmap_freebsd.h
 *
 * FreeBSD-specific NIC mapper private decls.
 */

/*
 * Copyright (c) 2001  Kevin Van Maren.  All rights reserved.
 * $Copyright udi_reference:
 * $
 */

#ifndef NETMAP_FREEBSD_H
#define	NETMAP_FREEBSD_H

#define	NETMAP_OSDEP_H

typedef struct und_softc {
	struct arpcom arpcom;
	struct ifmedia sc_media;
	void *rdata;		/* point back to common recv/ctrl region data */
	void *txrdata;		/* point back to common xmit region data */

	/* Used to block process-level activity until asynch call completes */
	_OSDEP_EVENT_T enable_ev;
	udi_cb_t *enable_cb;

	/* Single cb used to serialize control requests */
	udi_nic_ctrl_cb_t *setfilt_cb;

} netmap_osdep_t;

#define	NETMAP_OSDEP_SIZE	sizeof(netmap_osdep_t)


/* Local gcb types */
#define NSRMAP_NIC_ENABLE_CB_IDX 20

#endif /* NETMAP_FREEBSD_H */
