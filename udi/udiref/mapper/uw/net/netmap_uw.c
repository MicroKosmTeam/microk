
/*
 * File: 
 *
 * UnixWare 7.1.x specific network mapper code
 *
 * Provides the osnsr_xxxxx routines called out of the nsrmap driver and the
 * interfaces to the UnixWare MDI network driver model.
 *
 * Copyright udi_reference:
 * 
 * 
 *    Copyright (c) 2001 Unisys, Inc.  All rights reserved.
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
 */

#define	_DDI	8			/* DDI version 8 */

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/strstat.h>
#include <sys/conf.h>
#include <sys/cmn_err.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/ksynch.h>
#include <sys/lwp.h>
#include <sys/cred.h>
#include <sys/mdi.h>

#define	UDI_NIC_VERSION	0x101		/* Version 1.0 */
#include <udi_env.h>			/* O.S. Environment */
#include <udi_nic.h>

#include "netmap_uw.h"			/* Local version */
#include "niccommon/netmap.h"

#include <sys/ddi.h>

#if 1

/*
 * The net mapper uses this indirectly, it really should go with
 * the FSPIN_* fn's in udi_osdep.h.
 */
extern void FSPIN_INIT(fspin_t *);
#endif

#define	MY_MIN(a, b)	((a) < (b) ? (a) : (b))

/* #define VERBOSE_DEBUG *//*
 * Set to log function entry events. 
 */

/*
 * Constants
 */

#define MAPPER_NAME "udiMnic"		/* O.S. Specific */

#define	NSR_SINGLE_RX_BUF		/* Only 1 Rx CB per mblk */

#ifndef NSR_WATCHDOG_NSECS
#define	NSR_WATCHDOG_NSECS	0
#endif
#ifndef	NSR_WATCHDOG_SECS
#define	NSR_WATCHDOG_SECS	1
#endif

/*
 * Global driver-specific data [for OS use only]
 */
int nNSRmappers;			/* # of NSR instances */

typedef struct {
	udi_queue_t Q;
	const char *modname;
	int nrefs;
	void *drvinfop;
} nsrmap_mod_t;

udi_queue_t nsrmap_mod_Q;		/* Queue of bound driver modnames */


/*
 * Forward declarations
 */

STATIC void NSR_enable(udi_cb_t *,
		       void (*)(udi_cb_t *,
				udi_status_t));
STATIC void NSR_disable(udi_cb_t *,
			void (*)(udi_cb_t *,
				 udi_status_t));
STATIC void NSR_return_packet(udi_nic_rx_cb_t *);
STATIC void osnsr_tx_packet(udi_cb_t *);
STATIC void NSR_ctrl_req(udi_nic_ctrl_cb_t *);
STATIC void nsrmap_make_macaddr(char *,
				udi_ubit8_t *,
				udi_ubit8_t);
STATIC nsrmap_Tx_region_data_t *NSR_txdata(nsrmap_region_data_t *);

/*
 * -----------------------------------------------------------------------------
 * OS-specific Interface routines [called by netmap.c]
 * -----------------------------------------------------------------------------
 */
STATIC void osnsr_init(nsrmap_region_data_t *);
STATIC void osnsr_bind_done(udi_channel_event_cb_t *,
			    udi_status_t);
STATIC void osnsr_unbind_done(udi_nic_cb_t *);
STATIC void osnsr_status_ind(udi_nic_status_cb_t *);
STATIC udi_ubit32_t osnsr_max_xfer_size(udi_ubit8_t);
STATIC udi_ubit32_t osnsr_min_xfer_size(udi_ubit8_t);
STATIC void osnsr_rx_packet(udi_nic_rx_cb_t *,
			    udi_boolean_t);
STATIC void osnsr_ctrl_ack(udi_nic_ctrl_cb_t *,
			   udi_status_t);
STATIC udi_ubit32_t osnsr_mac_addr_size(udi_ubit8_t media_type);
STATIC void osnsr_final_cleanup(udi_mgmt_cb_t *);

/*
 * -----------------------------------------------------------------------------
 * OS-specific mapper routines [not called from netmap.c]
 * -----------------------------------------------------------------------------
 */
STATIC udi_buf_write_call_t _osnsr_txbuf_alloc;
STATIC void l_nsrmap_ioctl(queue_t *,
			   mblk_t *);
STATIC udi_cb_alloc_call_t l_nsrmap_ioctl_cb1;
STATIC udi_buf_write_call_t l_nsrmap_ioctl_cb2;
STATIC udi_buf_write_call_t l_nsrmap_ioctl_cb3;
STATIC udi_boolean_t l_netmap_mca_add(udi_ubit8_t *,
				      netmap_osdep_t *,
				      udi_ubit8_t media_type);
STATIC udi_boolean_t l_netmap_mca_del(udi_ubit8_t *,
				      netmap_osdep_t *,
				      udi_ubit8_t media_type);
STATIC udi_boolean_t l_netmap_mca_found(udi_ubit8_t *,
					netmap_osdep_t *,
					udi_ubit8_t media_type);
STATIC udi_boolean_t l_netmap_mca_filter(udi_nic_rx_cb_t *,
					 udi_ubit8_t *,
					 udi_ubit8_t media_type);
STATIC void l_osnsr_open_complete(udi_cb_t *,
				  udi_status_t);
STATIC nsrmap_mod_t *nsrmap_find_modname(const char *);
STATIC void l_osnsr_return_packet(udi_cb_t *);

STATIC void l_osnsr_enqueue_callback(udi_cb_t *,
				     void (*)(udi_cb_t *));

#if NSR_MCA_DEBUG
STATIC void nsr_dump_media_addr(udi_ubit8_t media_type,
				udi_ubit8_t *addr);
STATIC void
nsr_dump_media_addr(udi_ubit8_t media_type,
		    udi_ubit8_t *addr)
{
	udi_size_t addr_size = osnsr_mac_addr_size(media_type);

	/*
	 * TBD: sizes larger than 6 bytes should be handled. 
	 */
	cmn_err(CE_WARN, "%02X:%02X:%02X:%02X:%02X:%02X",
		addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}


STATIC void nsr_dump_mctable_mp(mblk_t * mctable_mp,
				udi_ubit8_t media_type);
STATIC void
nsr_dump_mctable_mp(mblk_t * mctable_mp,
		    udi_ubit8_t media_type)
{
	mac_mcast_t *mca_info;
	udi_ubit8_t *start, *end, *cur, *mctable;
	udi_ubit32_t counter = 0;
	udi_size_t addr_size = osnsr_mac_addr_size(media_type);

	mca_info = (mac_mcast_t *) mctable_mp->b_rptr;
	mctable = (udi_ubit8_t *)mca_info + mca_info->mac_mca_offset;
	start = cur = mctable;
	end = mctable + mca_info->mac_mca_length;
	cmn_err(CE_NOTE, "mac_mca_offset: %d", mca_info->mac_mca_offset);
	NSRMAP_ASSERT((signed int)mca_info->mac_mca_offset > 0);
	cmn_err(CE_NOTE, "mac_mca_length: %d", mca_info->mac_mca_length);
	NSRMAP_ASSERT((signed int)mca_info->mac_mca_length >= 0);
	cmn_err(CE_NOTE, "mac_mca_count: %d", mca_info->mac_mca_count);
	NSRMAP_ASSERT((signed int)mca_info->mac_mca_count >= 0);
	NSRMAP_ASSERT(mca_info->mac_mca_length % addr_size == 0);
	while (cur < end) {
		cmn_err(CE_NOTE, "tbl[%d]=", counter);
		nsr_dump_media_addr(media_type, cur);
		counter++;
		cur += addr_size;
	}
}
#endif


/*
 * -----------------------------------------------------------------------------
 * OS-specific environment routines 
 * -----------------------------------------------------------------------------
 */
mblk_t *udi_rxcb_to_mblk(udi_nic_rx_cb_t *,
			 void (*)(udi_cb_t *),
			 udi_cb_t *);
STATIC void l_udi_delay_callback(void *);

/*
 * OS specific code:
 */

/*
 * Global driver-specific data [for OS use only]
 */

typedef struct nsrmap_init_data {
	udi_ubit32_t instance;		/* Mapper instance */
	void *rdata;			/* Primary region local data */
	void *txdata;			/* Transmit region local data */
	udi_boolean_t first_open;	/* Set if this is first open */
	rm_key_t key;			/* This resmgr key */
} nsrmap_init_data_t;

STATIC void nsrmap_start(void);
STATIC int nsrmap_devinfo(void *,
			  channel_t,
			  di_parm_t,
			  void *);
STATIC int nsrmap_config(cfg_func_t,
			 void *,
			 rm_key_t);
STATIC int nsrmap_open(void *,
		       channel_t *,
		       int,
		       cred_t *,
		       queue_t *);
STATIC int nsrmap_close(void *,
			channel_t,
			int,
			cred_t *,
			queue_t *);
STATIC int nsrmap_wput(queue_t *,
		       mblk_t *);
STATIC int nsrmap_wsrv(queue_t *);
STATIC void nsrmap_ioctl(queue_t *,
			 mblk_t *);
STATIC void nsrmap_ioctl1(queue_t *,
			  mblk_t *);
STATIC void nsrmap_proto(queue_t *,
			 mblk_t *);
STATIC void nsrmap_proto1(queue_t *,
			  mblk_t *);
STATIC void nsrmap_data(queue_t *,
			mblk_t *,
			udi_boolean_t);
STATIC void nsrmap_data1(queue_t *,
			 mblk_t *,
			 udi_boolean_t);
STATIC void l_nsrmap_get_stats(queue_t *,
			       mblk_t *,
			       void (*)(udi_cb_t *));
STATIC void l_nsrmap_get_stats_cbfn(udi_cb_t *,
				    udi_cb_t *);
STATIC void l_nsrmap_return_stats(udi_cb_t *);
STATIC void l_nsrmap_return_local_stats(udi_cb_t *);
STATIC void nsrmap_set_stropts(queue_t *,
			       nsrmap_init_data_t *);

STATIC void NSR_thread(void *);

STATIC drvops_t nsrmap_ops = {
	nsrmap_config,			/* config */
	nsrmap_open,			/* open   */
	nsrmap_close,			/* close  */
	nsrmap_devinfo,			/* devinfo */
	NULL,				/* no biostart */
	NULL,				/* no ioctl    */
	NULL,				/* no devctl    */
	NULL				/* no mmap      */
};

static macaddr_t netmap_broad = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

/*
 * _osnsr_find_rdata:
 * -----------------
 * Scan the bound mapper<->driver links until we either find a matching
 * idata->key <=> rdata->osdep_ptr->drvr_key or exhaust the list.
 * Return the region data if a match is found, NULL otherwise
 * Update idatap->instance with the index of the matching rdata
 */
nsrmap_region_data_t *
_osnsr_find_rdata(nsrmap_init_data_t * idatap,
		  char *name)
{
	udi_ubit32_t i;
	nsrmap_region_data_t *rdata;

	for (i = 0;; i++) {
		rdata = _udi_MA_local_data(name, i);
		if (rdata == NULL)
			return NULL;
		else {
			if (rdata->osdep_ptr->drvr_key == idatap->key) {
				idatap->instance = i;
				return rdata;
			}
		}
	}
}

STATIC struct module_stat nsrmap_wstats;	/* Write statistics */
STATIC struct module_stat nsrmap_rstats;	/* Read statistics  */

/*
 * Streams High and Low water-marks
 */
#ifndef	NSR_HIWAT
#define	NSR_HIWAT	0x10000
#endif /* NSR_HIWAT */
#ifndef	NSR_LOWAT
#define	NSR_LOWAT	0x5000
#endif /* NSR_LOWAT */

struct module_info nsrmap_minfo = {
	0, MAPPER_NAME, 0, INFPSZ, NSR_HIWAT, NSR_LOWAT
};

struct qinit nsrmap_rinit = {
	NULL, NULL, NULL, NULL, NULL, &nsrmap_minfo,
	&nsrmap_rstats
};

struct qinit nsrmap_winit = {
	nsrmap_wput, nsrmap_wsrv, NULL, NULL, NULL, &nsrmap_minfo,
	&nsrmap_wstats
};

struct streamtab udiM_netinfo = { &nsrmap_rinit, &nsrmap_winit, 0, 0 };

const drvinfo_t nsrmap_drvinfo = {
	&nsrmap_ops, MAPPER_NAME, D_MP, &udiM_netinfo, 1
};


STATIC int
nsrmap_devinfo(void *idatap,
	       channel_t chan,
	       di_parm_t parm,
	       void *valp)
{
	switch (parm) {
	case DI_MEDIA:
		*(char **)valp = "NIC";	/* short names here */
		return 0;
	case DI_SIZE:
	case DI_RBCBP:
	case DI_WBCBP:
	case DI_PHYS_HINT:
		return EOPNOTSUPP;
	default:
		return ENOSYS;
	}
	/*
	 * NOTREACHED 
	 */
}

STATIC int
nsrmap_config(cfg_func_t func,
	      void *idata,
	      rm_key_t key)
{
	void **idatap;
	nsrmap_init_data_t *init_data;

#if defined(_KERNEL) && defined(VERBOSE_DEBUG)
	cmn_err(CE_WARN, "!nsrmap_config(%p, %p, %p)", func, idata, key);
#endif /* _KERNEL && DEBUG */

	switch (func) {

	case CFG_ADD:
		idatap = (void **)idata;

		if (mdi_get_unit(key, NULL) == B_FALSE) {
			return ENXIO;
		}
		init_data =
			_OSDEP_MEM_ALLOC(sizeof (nsrmap_init_data_t), 0, 0);
		if (init_data == NULL) {
			/*
			 * No memory available, fail the CFG_ADD 
			 */
#if defined(_KERNEL)
			cmn_err(CE_WARN,
				"!nsrmap_config: CFG_ADD failed, no init memory avail\n");
#endif /* _KERNEL */
			return ENODEV;
		}
		init_data->instance = nNSRmappers;
		init_data->key = key;
		init_data->rdata = _osnsr_find_rdata(init_data, MAPPER_NAME);
		init_data->first_open = TRUE;

		*idatap = init_data;
		nNSRmappers++;
		return 0;

	case CFG_REMOVE:
		/*
		 * TODO: Cause a _udi_MA_unbind() to take place 
		 */
		nNSRmappers--;
		if (nNSRmappers < 0) {
			nNSRmappers = 0;
		}
		/*
		 * Release allocated init_data memory 
		 */
		_OSDEP_MEM_FREE(idata);
		return 0;

		/*
		 * TODO: Handle CFG_SUSPEND, CFG_RESUME 
		 */
	default:
		return EOPNOTSUPP;
	}
}

STATIC void *nsrmap_attach(const char *);
STATIC int nsrmap_detach(void *);

/*
 * nsrmap_attach:
 * -------------
 * Construct a drvinfo_t structure which can be drv_attach()d by the calling
 * driver. This is needed to allow the CFG_xxx resmgr initiated calls to be
 * correctly despatched to the wrapper driver's entry point which will hook
 * into the nsrmap_config routine.
 */
STATIC void *
nsrmap_attach(const char *modname)
{
	drvinfo_t *mydrvinfo;
	char *myname;

	mydrvinfo = (drvinfo_t *) _OSDEP_MEM_ALLOC(sizeof (drvinfo_t), 0, 0);
	myname = (char *)_OSDEP_MEM_ALLOC(udi_strlen(modname) + 1, 0, 0);
	if (mydrvinfo == NULL || myname == NULL) {
		if (mydrvinfo) {
			_OSDEP_MEM_FREE(mydrvinfo);
		}
		if (myname) {
			_OSDEP_MEM_FREE(myname);
		}
		return NULL;
	} else {
		mydrvinfo->drv_ops = nsrmap_drvinfo.drv_ops;
		udi_strcpy(myname, modname);
		mydrvinfo->drv_name = myname;
		mydrvinfo->drv_flags = nsrmap_drvinfo.drv_flags;
		mydrvinfo->drv_str = nsrmap_drvinfo.drv_str;
		mydrvinfo->drv_maxchan = nsrmap_drvinfo.drv_maxchan;

		return mydrvinfo;
	}
}

/*
 * nsrmap_detach:
 * -------------
 * Release the previously allocated drvinfo_t structure. Called after the
 * wrapper driver has drv_detach'd itself.
 */
STATIC int
nsrmap_detach(void *ptr)
{
	drvinfo_t *drvinfo = (drvinfo_t *) ptr;

	if (drvinfo) {

		_OSDEP_MEM_FREE((void *)(drvinfo->drv_name));
		_OSDEP_MEM_FREE(drvinfo);
	}

	return (0);
}

/*
 * NSR_enqueue:
 * -----------
 * Enqueue a request to the NSR_thread for execution in base context.
 *
 * Returns:
 *	0	on successful despatch
 *	1	if there is insufficient space for request
 */
STATIC int
NSR_enqueue(nsrmap_region_data_t * rdata,
	    queue_t * q,
	    mblk_t * mp,
	    NSR_thread_op_t op)
{
	netmap_osdep_t *osdp = rdata->osdep_ptr;
	thread_req_t *tp;

	NSR_THREAD_LOCK(&osdp->thread_mutex);
	tp = osdp->head;
	if (osdp->tail->next == tp) {
		osdp->thread_full = TRUE;
		NSR_THREAD_UNLOCK(&osdp->thread_mutex);
		return 1;
	} else {
		osdp->thread_full = FALSE;
	}
	tp->op = op;
	tp->q = q;
	tp->mp = mp;
	osdp->head = osdp->head->next;
	NSR_THREAD_UNLOCK(&osdp->thread_mutex);

	_OSDEP_EVENT_SIGNAL(&osdp->thread_ev);
	return 0;
}

/*
 * NSR_create_thread:
 * -----------------
 * Create a kernel thread to communicate with the NSR mapper. This allows
 * the UDI subsystem to call functions outside of streams context (e.g.
 * mdi_read_file) while allowing the stream to run in a non-blocking context
 *
 * Returns:
 *	0	success
 *	Error code on failure
 */
STATIC int
NSR_create_thread(nsrmap_region_data_t * rdata,
		  nsrmap_Tx_region_data_t * txdata)
{
	int i;
	thread_req_t *tp;
	netmap_osdep_t *osdp = rdata->osdep_ptr;
	int numelems;

	/*
	 * Allocate thread operation containers [txdata->cbq.numelem]
	 * The list is empty when head == tail and full when head->next == tail
	 * Requests are added to 'head' by the streams context routines and 
	 * removed from 'tail' by the kernel thread. EVENT_WAIT/EVENT_SIGNAL
	 * are used to initiate the requests
	 */
	osdp->tail = NULL;
	numelems = NSR_UDI_QUEUE_SIZE(&txdata->cbq);
	if (numelems < NETMAP_NUM_THREAD_REQS) {
		numelems = NETMAP_NUM_THREAD_REQS;
	}
	for (i = 0; i < numelems; i++) {
		tp = _OSDEP_MEM_ALLOC(sizeof (*tp), 0, 0);
		if ((tp == NULL) && (i == 0)) {
			cmn_err(CE_PANIC,
				"Insufficient memory for NSR thread queues");
		} else if (tp == NULL) {
			break;
		}
		if (osdp->tail == NULL) {
			osdp->tail = tp;
		}
		tp->next = osdp->head;
		osdp->head = tp;
	}
	osdp->tail->next = osdp->head;	/* Circular list */
	osdp->tail = osdp->head;	/* Initially empty */

	_OSDEP_EVENT_INIT(&osdp->thread_ev);

	i = kthread_spawn(NSR_thread, (void *)rdata, "NSR Thread",
			  &osdp->kthread_ID);

	return i;
}

/*
 * NSR_kill_thread:
 * ---------------
 * Issue an ExitThread request to the NSR thread and wait for completion.
 * Once we've joined the thread we free the data structures that were
 * allocated.
 */
STATIC void
NSR_kill_thread(nsrmap_region_data_t * rdata)
{
	thread_req_t *tp;
	netmap_osdep_t *osdp = rdata->osdep_ptr;

	NSR_THREAD_LOCK(&osdp->thread_mutex);
	tp = osdp->head;
	tp->op = ExitThread;
	tp->mp = NULL;
	if (osdp->head == osdp->tail) {
		osdp->head = osdp->head->next;	/* Advance request pointer */
	}
	NSR_THREAD_UNLOCK(&osdp->thread_mutex);

	_OSDEP_EVENT_SIGNAL(&osdp->thread_ev);	/* Wake up thread */

	(void)kthread_wait(osdp->kthread_ID);

	_OSDEP_EVENT_DEINIT(&osdp->thread_ev);

	osdp->head = osdp->tail = NULL;
}

/*
 * NSR_thread:
 * ----------
 * Kernel thread for handling streams requests outside the plstr context.
 * A request is taken from the rdata->osdep_ptr->tail element until head == tail
 * Once an empty queue is found we _OSDEP_EVENT_WAIT() for a new request.
 *
 * Requests handled:
 *	ExitThread	- kill thread
 *	SendIoctl	- Perform nsrmap_ioctl handling
 *	SendProto	- Perform nsrmap_proto handling
 *
 * Calling Context:
 *	plbase
 */
STATIC void
NSR_thread(void *arg)
{
	nsrmap_region_data_t *rdata = arg;
	netmap_osdep_t *osdp = rdata->osdep_ptr;
	thread_req_t *tp;

	for (;;) {
		_OSDEP_EVENT_WAIT(&osdp->thread_ev);
		NSR_THREAD_LOCK(&osdp->thread_mutex);
		while (osdp->tail != osdp->head) {
			tp = osdp->tail;
			osdp->tail = osdp->tail->next;
			osdp->thread_full = FALSE;
			switch (tp->op) {
			case ExitThread:
				osdp->head = tp;
				do {
					tp = osdp->tail->next;
					_OSDEP_MEM_FREE(osdp->tail);
					osdp->tail = tp;
				} while (osdp->tail != osdp->head);
				_OSDEP_MEM_FREE(osdp->head);
				NSR_THREAD_UNLOCK(&osdp->thread_mutex);
				kthread_exit();
				/*
				 * NOTREACHED 
				 */
				break;
			case SendIoctl:
				NSR_THREAD_UNLOCK(&osdp->thread_mutex);
				nsrmap_ioctl1(tp->q, tp->mp);
				NSR_THREAD_LOCK(&osdp->thread_mutex);
				break;
			case SendProto:
				NSR_THREAD_UNLOCK(&osdp->thread_mutex);
				nsrmap_proto1(tp->q, tp->mp);
				NSR_THREAD_LOCK(&osdp->thread_mutex);
				break;
#ifdef NSR_USE_THREAD
			case SendData:
			case SendDataPB:
				NSR_THREAD_UNLOCK(&osdp->thread_mutex);
				nsrmap_data1(tp->q, tp->mp,
					     tp->op == SendDataPB);
				NSR_THREAD_LOCK(&osdp->thread_mutex);
				break;
#endif
			default:
				udi_assert(0);
				break;
			}
		}
		NSR_THREAD_UNLOCK(&osdp->thread_mutex);
	}
}

/*
 * nsrmap_open:
 * -----------
 * Initiate link connection for given channel. Once we return the underlying
 * link is ready for data transfers.
 */
STATIC int
nsrmap_open(void *idatap,
	    channel_t * channelp,
	    int oflags,
	    cred_t * cred_p,
	    queue_t * q)
{
	nsrmap_init_data_t *gdata = idatap;
	nsrmap_region_data_t *rdata = (nsrmap_region_data_t *) gdata->rdata;
	channel_t channel = *channelp;
	void **nlistp;
	udi_ubit32_t ii;
	int rval;
	char *namestr;			/* Name of ND */
	char *addrstr;			/* String for MAC address */
	_udi_channel_t *ch;

	if (gdata->rdata == NULL) {
		gdata->rdata = _osnsr_find_rdata(gdata, MAPPER_NAME);
		if (gdata->rdata == NULL) {
			return ENODEV;
		}
		rdata = (nsrmap_region_data_t *) gdata->rdata;
	}

	nsrmap_rstats.ms_ocnt++;
	nsrmap_wstats.ms_ocnt++;

	/*
	 * Issue mdi_printfcfg on first open of the device 
	 */
	if (gdata->first_open) {

		/*
		 * Obtain the Transmit region reference 
		 */
		gdata->txdata = NSR_txdata(rdata);
		if (gdata->txdata == NULL) {
			return ENODEV;
		}

		gdata->first_open = FALSE;

		/*
		 * Allocate space for the MAC address 
		 */
		addrstr = _OSDEP_MEM_ALLOC(rdata->link_info.mac_addr_len * 3,
					   0, UDI_WAITOK);

		nsrmap_make_macaddr(addrstr, rdata->link_info.mac_addr,
				    rdata->link_info.mac_addr_len);

		/*
		 * Find name of underlying ND device 
		 */
		ch = (_udi_channel_t *)rdata->link_info.gcb.channel;
		namestr =
			(char *)ch->other_end->chan_region->reg_driver->
			drv_name;
		mdi_printcfg(namestr, 0, 0, -1, -1, "addr=%s", addrstr);

		_OSDEP_MEM_FREE(addrstr);
	}
#if defined(_KERNEL) && defined(VERBOSE_DEBUG)
	cmn_err(CE_WARN, "!nsrmap_open(%p, %p, %p, %p, %p)", idatap, channel,
		oflags, cred_p, q);
#endif /* _KERNEL && DEBUG */

	if (q->q_ptr) {
		/*
		 * We've already been initialised so fail 
		 */
		return EBUSY;
	}

	if (rdata == NULL) {
		return ENXIO;
	} else {
		/*
		 * Enforce single open() access 
		 */
		_OSDEP_MUTEX_LOCK(&rdata->osdep_ptr->op_mutex);
		if (rdata->nopens > 0) {
			_OSDEP_MUTEX_UNLOCK(&rdata->osdep_ptr->op_mutex);
			return EBUSY;
		} else {
			rdata->nopens++;
		}
		_OSDEP_MUTEX_UNLOCK(&rdata->osdep_ptr->op_mutex);

		/*
		 * Spawn a kthread to handle calls into the NSR from outside
		 * streams context (i.e. not at plstr)
		 */
		rval = NSR_create_thread(rdata, gdata->txdata);
		if (rval != 0) {
#if defined(_KERNEL) && defined(DEBUG)
			cmn_err(CE_WARN,
				"!nsrmap_open: NSR_create_thread returned %d\n",
				rval);
#endif /* _KERNEL && DEBUG */
			return rval;
		}
#if defined(_KERNEL) && defined(VERBOSE_DEBUG)
		cmn_err(CE_WARN,
			"!nsrmap_open: B4 NSR_enable, rdata->osdep = %p",
			rdata->osdep_ptr);
#endif /* _KERNEL && DEBUG */

		NSR_enable(rdata->bind_cb, l_osnsr_open_complete);
		_OSDEP_EVENT_WAIT(&rdata->osdep_ptr->op_close_sv);

#if defined(_KERNEL) && defined(VERBOSE_DEBUG)
		cmn_err(CE_CONT,
			"!nsrmap_open: After NSR_enable, rdata->osdep = %p",
			rdata->osdep_ptr);
#endif /* _KERNEL && DEBUG */

		/*
		 * Enable the queue 
		 */
		if (rdata->open_status == UDI_OK) {
			nsrmap_Tx_region_data_t *txdata;
			netmap_osdep_tx_t *txosdep;

			q->q_ptr = WR(q)->q_ptr = gdata;
			rdata->osdep_ptr->upQ = RD(q);

			txdata = gdata->txdata;
			txosdep = (netmap_osdep_tx_t *) txdata->osdep_tx;
			txosdep->wep = &(rdata->osdep_ptr->we);
			txosdep->wep->enableq = (queue_t *) NULL;
			_OSDEP_SIMPLE_MUTEX_INIT(&txosdep->wep->enable_mutex,
						 "NSR Write Enable Mutex");
			txdata->osdep_tx_ptr = txosdep;

			qprocson(RD(q));
			nsrmap_set_stropts(RD(q), gdata);
		}

		return (rdata->open_status);
	}
}

/*
 * nsrmap_set_stropts:
 * ------------------
 * Change the streams options on our read queue head to match what the 
 * underlying ND can support. This is determined from the max_pdu/min_pdu size
 * and the number of transmit CBs which were initially passed to us.
 *
 * NOTE: We can only use M_SETOPTS to affect the read-size queue options. We
 *	 set the write-side ones by directly manipulating the q_minpsz,
 *	 q_maxpsz, q_hiwat and q_lowat fields.
 */
STATIC void
nsrmap_set_stropts(queue_t * q,
		   nsrmap_init_data_t * gdata)
{
	nsrmap_region_data_t *rdata = gdata->rdata;
	nsrmap_Tx_region_data_t *txdata = gdata->txdata;
	int numelems;			/* # of Tx CB elements        */
	ulong lowat, hiwat;		/* Streams low/high water mark */
	long minpsz, maxpsz;		/* Min/Max packet size        */
	mblk_t *mp;
	struct stroptions *optp;
	queue_t *wq = WR(q);		/* Write-side queue           */

	minpsz = (long)rdata->link_info.min_pdu_size;
	maxpsz = (long)rdata->link_info.max_pdu_size;

	numelems = NSR_UDI_QUEUE_SIZE(&txdata->cbq);
	if (numelems == 0) {
		lowat = NSR_LOWAT;
		hiwat = NSR_HIWAT;
	} else {
		hiwat = (ulong) (maxpsz) * (ulong) numelems;
		lowat = hiwat / 2;
	}

	mp = allocb(sizeof (struct stroptions), BPRI_MED);
	if (mp == NULL) {
		return;
	}

	mp->b_datap->db_type = M_SETOPTS;
	mp->b_rptr = mp->b_datap->db_base;
	mp->b_wptr = mp->b_rptr + sizeof (struct stroptions);
	bzero(mp->b_rptr, sizeof (struct stroptions));

	optp = (struct stroptions *)mp->b_rptr;
	optp->so_flags = SO_MAXPSZ | SO_MINPSZ | SO_HIWAT | SO_LOWAT;
	optp->so_maxpsz = maxpsz;
	optp->so_minpsz = minpsz;
	optp->so_hiwat = hiwat;
	optp->so_lowat = lowat;

	/*
	 * Modify the write-q characteristics
	 */
	wq->q_minpsz = minpsz;
	wq->q_maxpsz = maxpsz;
	wq->q_hiwat = hiwat;
	wq->q_lowat = lowat;

	putnext(q, mp);
}

STATIC void
l_osnsr_open_complete(udi_cb_t *cb,
		      udi_status_t status)
{
	nsrmap_region_data_t *rdata = cb->context;

	rdata->open_status = status;

	_OSDEP_EVENT_SIGNAL(&rdata->osdep_ptr->op_close_sv);
}

/*
 * nsrmap_close:
 * ------------
 * Tear down link for given channel. Once we return the link is no longer
 * available for data transfer
 */
STATIC int
nsrmap_close(void *idata,
	     channel_t channel,
	     int oflag,
	     cred_t * credp,
	     queue_t * q)
{
	nsrmap_init_data_t *gdata = (nsrmap_init_data_t *) q->q_ptr;
	nsrmap_region_data_t *rdata = (nsrmap_region_data_t *) gdata->rdata;

	qprocsoff(RD(q));

	nsrmap_rstats.ms_ccnt++;
	nsrmap_wstats.ms_ccnt++;

#if defined(_KERNEL) && defined(VERBOSE_DEBUG)
	cmn_err(CE_WARN, "!nsrmap_close(%p, %p, %p, %p, %p)", idata, channel,
		oflag, credp, q);
	cmn_err(CE_CONT, "!rdata = %p", rdata);
#endif /* _KERNEL && DEBUG */
	if (rdata == NULL) {
		return -1;
	} else {
		netmap_osdep_wrenable_t *wep;

		wep = &rdata->osdep_ptr->we;
		_OSDEP_SIMPLE_MUTEX_LOCK(&wep->enable_mutex);
		wep->enableq = (queue_t *) NULL;
		_OSDEP_SIMPLE_MUTEX_UNLOCK(&wep->enable_mutex);

		NSR_disable(rdata->bind_cb, l_osnsr_open_complete);

		/*
		 * Wait for NSR_disable to complete 
		 */
		_OSDEP_EVENT_WAIT(&rdata->osdep_ptr->op_close_sv);

#if 0
		if (BTST(rdata->NSR_state, NsrmapS_TxWaiting)) {
			_OSDEP_EVENT_SIGNAL(&rdata->osdep_ptr->sv1);
		}
		if (BTST(rdata->NSR_state, NsrmapS_RxWaiting)) {
			_OSDEP_EVENT_SIGNAL(&rdata->osdep_ptr->sv2);
		}
#endif

		rdata->osdep_ptr->upQ = NULL;
		rdata->osdep_ptr->bound = FALSE;

		if (BTST(rdata->NSR_state, NsrmapS_Promiscuous)) {
			rdata->osdep_ptr->ioc_cmd = NSR_UDI_NIC_PROMISC_OFF;
			/*
			 * Allocate the ctrl_cb for this request 
			 */
			udi_cb_alloc(l_nsrmap_ioctl_cb1, rdata->bind_cb,
				     NSRMAP_CTRL_CB_IDX,
				     rdata->bind_info.bind_channel);

			/*
			 * Wait for the osnsr_ctrl_ack to complete.
			 */
			_OSDEP_EVENT_WAIT(&rdata->osdep_ptr->ioc_sv);
		}

		_OSDEP_MUTEX_LOCK(&rdata->osdep_ptr->op_mutex);
		rdata->nopens = 0;
		_OSDEP_MUTEX_UNLOCK(&rdata->osdep_ptr->op_mutex);

#if defined(_KERNEL) && defined(VERBOSE_DEBUG)
		cmn_err(CE_WARN, "!nsrmap_close: rdata->osdep_ptr = %p",
			rdata->osdep_ptr);
#endif /* _KERNEL && DEBUG */
	}

	q->q_ptr = RD(q)->q_ptr = NULL;

	NSR_kill_thread(rdata);
	return 0;
}

/*
 * nsrmap_wput:
 * -----------
 * Lower write-side put procedure. Parse the incoming data request and perform
 * the appropriate action. For M_DATA messages, transfer the data block to the
 * ND. For M_IOCTL, perform the requested action.
 *
 * Return Value:
 *	Ignored
 */
STATIC int
nsrmap_wput(queue_t * q,
	    mblk_t * mp)
{

	nsrmap_wstats.ms_pcnt++;	/* Only Write */

	if (q->q_ptr == NULL) {
#ifdef _KERNEL
		cmn_err(CE_WARN, "!nsrmap_wput: NULL q_ptr");
#endif /* _KERNEL */
		freemsg(mp);
		return 0;
	}

	switch (mp->b_datap->db_type) {
	case M_IOCTL:
		nsrmap_ioctl(q, mp);
		break;
	case M_PROTO:
	case M_PCPROTO:
		nsrmap_proto(q, mp);
		break;
	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW)
			flushq(q, FLUSHDATA);
		if (*mp->b_rptr & FLUSHR) {
			flushq(RD(q), FLUSHDATA);
			*mp->b_rptr &= ~FLUSHW;
			qreply(q, mp);
		} else {
			freemsg(mp);
		}
		break;
	case M_DATA:
		nsrmap_data(q, mp, FALSE);
		break;
	default:
		freemsg(mp);
	}
	return 0;
}

/*
 * nsrmap_wsrv:
 * -----------
 * Lower write-side service routine. This is called to handle any deferred Tx
 * requests which were enqueued by the nsrmap_data() routine.
 */
STATIC int
nsrmap_wsrv(queue_t * q)
{
	nsrmap_init_data_t *gdata = q->q_ptr;
	nsrmap_region_data_t *rdata = gdata->rdata;
	nsrmap_Tx_region_data_t *txdata = gdata->txdata;
	netmap_osdep_t *osdp = rdata->osdep_ptr;
	mblk_t *mp;
	udi_time_t intvl;


	/*
	 * Drain all outstanding requests if we're not in the ACTIVE state 
	 */
	if (rdata->State != ACTIVE) {
		cmn_err(CE_WARN,
			"!nsrmap_wsrv: Outstanding data in non ACTIVE state");
		while ((mp = getq(q)) != NULL) {
			txdata->n_discards++;
			freemsg(mp);
		}
		return 0;
	}
	if (osdp->we.enableq != NULL)	/* No locking here (read-only access) */
		return 0;

	while ((mp = getq(q)) != NULL) {
		if (NSR_UDI_QUEUE_EMPTY(&txdata->cbq) ||
#ifdef NSR_USE_THREAD
		    osdp->thread_full
#else
		    0
#endif
			) {
			putbq(q, mp);
			/*
			 * No locking needed here since qenable was not
			 * previously scheduled 
			 */
			osdp->we.enableq = q;
			return 0;
		}

		if (mp->b_datap->db_type == M_DATA)
			nsrmap_data1(q, mp, TRUE);
		else {
			udi_debug_printf("nsrmap_wsrv: dropped msg type %d\n",
					 mp->b_datap->db_type);
			freemsg(mp);
		}
	}
	return 0;
}

/*
 * nsrmap_ioctl:
 * ------------
 * Handle ioctl streams message.
 * We simply enqueue a request to the kernel thread to execute our message.
 * It will complete the processing at plbase.
 *
 * Calling Context:
 *	plstr
 */
STATIC void
nsrmap_ioctl(queue_t * q,
	     mblk_t * mp)
{
	nsrmap_init_data_t *gdata = q->q_ptr;
	nsrmap_region_data_t *rdata = gdata->rdata;

#ifdef NSR_USE_THREAD
	if (NSR_enqueue(rdata, q, mp, SendIoctl)) {
		putbq(q, mp);
	}
#else
	nsrmap_ioctl1(q, mp);
#endif
	return;
}

/*
 * nsrmap_ioctl1:
 * -------------
 * plbase ioctl handling. Actually perform the hard-work of processing the
 * ioctl into its UDI equivalent and returning the result to the user
 *
 * Calling Context:
 *	plbase
 */
STATIC void
nsrmap_ioctl1(queue_t * q,
	      mblk_t * mp)
{
	nsrmap_init_data_t *gdata = q->q_ptr;
	nsrmap_region_data_t *rdata = gdata->rdata;
	struct iocblk *iocp = (struct iocblk *)mp->b_rptr;
	unsigned char *data;
	udi_size_t mca_size;
	netmap_osdep_t *osdp = rdata->osdep_ptr;

	data = (mp->b_cont) ? mp->b_cont->b_rptr : NULL;
	iocp->ioc_error = 0;
	iocp->ioc_rval = 0;

#ifdef NSR_PARANOID
	udi_assert((mp->b_rptr >= mp->b_datap->db_base) &&
		   (mp->b_wptr >= mp->b_rptr) &&
		   (mp->b_wptr <= mp->b_datap->db_lim));
#endif /* NSR_PARANOID */


#if defined(_KERNEL) && defined(VERBOSE_DEBUG)
	cmn_err(CE_WARN, "!nsrmap_ioctl(%p, %p) rdata = %p", q, mp, rdata);
#endif /* _KERNEL && DEBUG */

	switch ((udi_ubit32_t)iocp->ioc_cmd) {
	case MACIOC_GETSTAT:
		if (data == NULL) {
			iocp->ioc_error = EINVAL;
		} else {
			switch (rdata->link_info.media_type) {
			case UDI_NIC_ETHER:
			case UDI_NIC_FASTETHER:
			case UDI_NIC_GIGETHER:	/* Check This */
				if (iocp->ioc_count < sizeof (mac_stats_eth_t)) {
					iocp->ioc_error = EINVAL;
				} else {
					iocp->ioc_count =
						sizeof (mac_stats_eth_t);
					l_nsrmap_get_stats(q, mp,
							   l_nsrmap_return_stats);
					return;
				}
				break;
			case UDI_NIC_TOKEN:
				if (iocp->ioc_count < sizeof (mac_stats_tr_t)) {
					iocp->ioc_error = EINVAL;
				} else {
					iocp->ioc_count =
						sizeof (mac_stats_tr_t);
					l_nsrmap_get_stats(q, mp,
							   l_nsrmap_return_stats);
					return;
				}
				break;
			case UDI_NIC_FDDI:
				if (iocp->ioc_count <
				    sizeof (mac_stats_fddi_t)) {
					iocp->ioc_error = EINVAL;
				} else {
					iocp->ioc_count =
						sizeof (mac_stats_fddi_t);
					l_nsrmap_get_stats(q, mp,
							   l_nsrmap_return_stats);
					return;
				}
				break;
			default:
				iocp->ioc_error = EINVAL;
				break;
			}
		}
		break;

	case MACIOC_CLRSTAT:		/* Clear MAC statistics */
		if (drv_priv(iocp->ioc_cr) == EPERM) {
			iocp->ioc_error = EPERM;
		} else {
			udi_memset(&osdp->s_u.t, 0, NETMAP_STAT_SIZE);
			iocp->ioc_count = 0;
			if (mp->b_cont) {
				freemsg(mp->b_cont);
				mp->b_cont = NULL;
			}
			/*
			 * TODO: Issue a udi_nd_ctrl_req to reset ND stats 
			 */
		}
		break;

	case MACIOC_GETRADDR:		/* Get factory MAC address */
	case MACIOC_GETADDR:		/* Get MAC address */
	case MACIOC_SETMCA:
	case MACIOC_DELMCA:
	case MACIOC_SETALLMCA:
	case MACIOC_DELALLMCA:
	case NSR_UDI_NIC_PROMISC_ON:	/* Promiscuous mode ON      */
	case MACIOC_PROMISC:		/* Enable Promiscuous mode -- reset on close() */
	case MACIOC_SETADDR:
	case NSR_UDI_NIC_BAD_RXPKT:	/* User initiated ioctl     */
	case NSR_UDI_NIC_HW_RESET:	/* Reset the hardware       */
	case NSR_UDI_NIC_PROMISC_OFF:	/* Promiscuous mode OFF     */
		/*
		 * Verify that we have sufficient privilege to perform the op 
		 */
		if (drv_priv(iocp->ioc_cr) == EPERM) {
			iocp->ioc_error = EPERM;
		} else {
			l_nsrmap_ioctl(q, mp);
		}
		break;
	case MACIOC_GETMCSIZ:		/* Size of Multicast Table */
		if (drv_priv(iocp->ioc_cr) == EPERM) {
			iocp->ioc_error = EPERM;
		} else {
			/*
			 * Get mdi mca table 
			 */
			mblk_t *mctable_mp;
			mac_mcast_t *mca_info;
			udi_ubit8_t *mca_table;
			udi_size_t addr_size;

			addr_size =
				osnsr_mac_addr_size(rdata->link_info.
						    media_type);
			mctable_mp = mdi_get_mctable(osdp->mac_cookie);
			if (mctable_mp == NULL) {
				iocp->ioc_error = EAGAIN;
				break;
			}
#if NSR_MCA_DEBUG
			cmn_err(CE_WARN, "GETMCSZ: dump:");
			nsr_dump_mctable_mp(mctable_mp,
					    rdata->link_info.media_type);
#endif
			mca_info = (mac_mcast_t *) mctable_mp->b_rptr;
			mca_table = (udi_ubit8_t *)mca_info
				+ mca_info->mac_mca_offset;
			iocp->ioc_rval = mca_info->mac_mca_length;
			freemsg(mctable_mp);
		}
		break;
	case MACIOC_GETMCA:		/* Return active Multicast table */
		if (drv_priv(iocp->ioc_cr) == EPERM) {
			iocp->ioc_error = EPERM;
		} else {
			/*
			 * Get mdi mca table 
			 */
			mblk_t *mctable_mp;
			mac_mcast_t *mca_info;
			udi_ubit8_t *mca_table;
			udi_size_t addr_size;

			addr_size =
				osnsr_mac_addr_size(rdata->link_info.
						    media_type);
			mctable_mp = mdi_get_mctable(osdp->mac_cookie);
			if (mctable_mp == NULL) {
				iocp->ioc_error = EAGAIN;
				break;
			}
#if NSR_MCA_DEBUG
			cmn_err(CE_WARN, "GETMCA: dump:");
			nsr_dump_mctable_mp(mctable_mp,
					    rdata->link_info.media_type);
#endif
			mca_info = (mac_mcast_t *) mctable_mp->b_rptr;
			mca_table = (udi_ubit8_t *)mca_info
				+ mca_info->mac_mca_offset;
			/*
			 * Copy the minimum of mca_size, iocp->ioc_count 
			 */
			mca_size = MY_MIN(mca_info->mac_mca_length,
					  iocp->ioc_count);
			udi_memcpy(data, mca_table, mca_size);
			freemsg(mctable_mp);
			mp->b_cont->b_wptr = mp->b_cont->b_rptr + mca_size;
			iocp->ioc_count = mca_size;
		}
		break;
	case NSR_UDI_NIC_BAD_RXPKT_INTERN:	/* Driver initiated ioctl   */
		l_nsrmap_ioctl(q, mp);
		break;
	default:
		iocp->ioc_error = EINVAL;
		break;
	}

	if (iocp->ioc_error) {
		mp->b_datap->db_type = M_IOCNAK;
		iocp->ioc_count = 0;
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
		}
	} else {
		mp->b_datap->db_type = M_IOCACK;
	}

	if (iocp->ioc_cmd == NSR_UDI_NIC_BAD_RXPKT_INTERN) {
		if (iocp->ioc_error) {
			cmn_err(CE_WARN, "!UDI_NIC_BAD_RXPKT failed");
		}
		freemsg(mp);
	} else {
		qreply(q, mp);
	}
	return;
}

/*
 * l_nsrmap_ioctl:
 * -------------
 * Issue corresponding udi_nd_ctrl_req calls for the MDI ioctl requests that
 * need data transfer between user and kernel. On completion of the call
 * the ioctl will be completed by nsrmap_ioctl returning with M_IOCACK for
 * success, or M_IOCNAK for failure.
 * This routine will block waiting for the UDI completion handshake (via
 * osnsr_ctrl_ack).
 *
 * Return Value:
 *	None.
 *
 * Calling Context:
 *	Blockable
 */
STATIC void
l_nsrmap_ioctl(queue_t * q,
	       mblk_t * mp)
{
	nsrmap_init_data_t *gdata = q->q_ptr;
	nsrmap_region_data_t *rdata = gdata->rdata;
	struct iocblk *iocp = (struct iocblk *)mp->b_rptr;
	unsigned char *data;
	netmap_osdep_t *osdp = rdata->osdep_ptr;
	nsr_macaddr_t l_macaddr;	/* MAC related command */
	void *ret_locn = NULL;
	udi_size_t ret_len = 0;

	data = (mp->b_cont) ? mp->b_cont->b_rptr : NULL;

	l_macaddr.ioc_cmd = iocp->ioc_cmd;
	l_macaddr.ioc_len = data ? mp->b_cont->b_wptr - data : 0;
	l_macaddr.ioc_data = data;

	switch (iocp->ioc_cmd) {
	case MACIOC_SETALLMCA:		/* Enable all Multicast addresses */
	case MACIOC_PROMISC:		/* Enable Promiscuous mode - reset on close() */
		/*
		 * No data needed for this operation 
		 */
		osdp->ioc_cmd = iocp->ioc_cmd;
		osdp->ioc_cmdp = NULL;
		osdp->ioc_datap = NULL;
		break;
	case MACIOC_GETRADDR:		/* Get factory MAC address */
	case MACIOC_GETADDR:		/* Get MAC address */
		ret_locn = data;
		ret_len = l_macaddr.ioc_len;
		/*
		 * FALLTHROUGH 
		 */
	default:
		osdp->ioc_cmd = iocp->ioc_cmd;
		osdp->ioc_datap = data;
		osdp->ioc_cmdp = &l_macaddr;
	}

	/*
	 * Allocate the ctrl_cb for this request 
	 */
	udi_cb_alloc(l_nsrmap_ioctl_cb1, rdata->bind_cb, NSRMAP_CTRL_CB_IDX,
		     rdata->bind_info.bind_channel);

	/*
	 * Wait for the osnsr_ctrl_ack to complete. At this point we will know
	 * if there is data to return.
	 */
	_OSDEP_EVENT_WAIT(&osdp->ioc_sv);

	/*
	 * Update the message block b_wptr fields if there was data which needs
	 * to be returned to the caller. 
	 */

	if (osdp->ioc_retval != UDI_OK) {
		iocp->ioc_error = EINVAL;
		goto bailout;
	} else {
		iocp->ioc_error = 0;
	}

	if (osdp->ioc_copyout) {
		mp->b_cont->b_wptr = mp->b_cont->b_rptr + ret_len;
	}

      bailout:
	/*
	 * Release buffer (if any)
	 */
	udi_buf_free(UDI_MCB(osdp->ioc_cb, udi_nic_ctrl_cb_t)->data_buf);

	/*
	 * Return the CB storage that we allocated
	 */
	udi_cb_free(osdp->ioc_cb);
	return;
}

/*
 * l_nsrmap_ioctl_cb1:
 * -----------------
 * Called on completion of the udi_cb_alloc for the udi_nic_ctrl_cb_t to be
 * used for the NSR_ctrl_req().
 * This routine is responsible for filling in the necessary fields of the new
 * cb and for updating the ND-specific multicast address lists where needed.
 */
STATIC void
l_nsrmap_ioctl_cb1(udi_cb_t *gcb,
		   udi_cb_t *new_cb)
{
	nsrmap_region_data_t *rdata = gcb->context;
	udi_nic_ctrl_cb_t *ctrl_cb = UDI_MCB(new_cb, udi_nic_ctrl_cb_t);
	netmap_osdep_t *osdp = rdata->osdep_ptr;
	udi_size_t addr_size;
	nsr_macaddr_t *macaddrp = (nsr_macaddr_t *) osdp->ioc_cmdp;
	macaddr_t l_macaddr;
	udi_boolean_t out_of_state = FALSE;

	/*
	 * Get OS dependent MAC address size for DELMCA, SETMCA commands 
	 */
	addr_size = osnsr_mac_addr_size(rdata->link_info.media_type);

	/*
	 * Save reference to ctrl_cb for completion 
	 */
	osdp->ioc_cb = new_cb;

	osdp->ioc_copyout = FALSE;
	osdp->ioc_retval = UDI_OK;

	/*
	 * Handle state changes and out-of-state issues 
	 */
	switch (osdp->ioc_cmd) {
	case NSR_UDI_NIC_PROMISC_ON:
	case MACIOC_PROMISC:
		osdp->multicast_state = NSR_UDI_NIC_PROMISC_ON;
		break;

	case NSR_UDI_NIC_PROMISC_OFF:
		osdp->multicast_state = NSR_UDI_NIC_PROMISC_OFF;
		break;

	case MACIOC_SETMCA:
	case MACIOC_DELMCA:
		if (osdp->multicast_state == UDI_NIC_ALLMULTI_ON ||
		    osdp->multicast_state == NSR_UDI_NIC_PROMISC_ON) {
			out_of_state = TRUE;
		}
		break;

	case MACIOC_SETALLMCA:
		if (osdp->multicast_state == NSR_UDI_NIC_PROMISC_ON) {
			out_of_state = TRUE;
		} else {
			osdp->multicast_state = UDI_NIC_ALLMULTI_ON;
		}
		break;

	case MACIOC_DELALLMCA:
		if (osdp->multicast_state == NSR_UDI_NIC_PROMISC_ON) {
			out_of_state = TRUE;
		} else {
			osdp->multicast_state = UDI_NIC_ALLMULTI_OFF;
		}
		break;
	}
	if (out_of_state) {
#if NSR_MCA_DEBUG
		cmn_err(CE_WARN, "nsrmap_ioctl_cb1: out of state (old:%d "
			" new:%d)", osdp->multicast_state, osdp->ioc_cmd);
#endif
		osdp->ioc_retval = UDI_STAT_NOT_UNDERSTOOD;
		osdp->ioc_copyout = FALSE;
		_OSDEP_EVENT_SIGNAL(&osdp->ioc_sv);
		return;
	}

	/*
	 * Do actual ioctl handling 
	 */

	switch (osdp->ioc_cmd) {

	case MACIOC_SETMCA:		/* Enable MCA held in arg */
		ctrl_cb->command = UDI_NIC_ADD_MULTI;
		ctrl_cb->indicator = 1;	/* Only 1 address */
		udi_memcpy(&l_macaddr, macaddrp->ioc_data, sizeof (l_macaddr));
		if (!UDI_MCA(l_macaddr, rdata->link_info.media_type)) {
			/*
			 * UnixWare's SETMCA says that only multicast
			 * addresses should be added to the multicast table.
			 */
			osdp->ioc_retval = UDI_STAT_NOT_UNDERSTOOD;
			osdp->ioc_copyout = FALSE;
			_OSDEP_EVENT_SIGNAL(&osdp->ioc_sv);
			break;
		} else if (l_netmap_mca_found(macaddrp->ioc_data, osdp,
					      rdata->link_info.media_type)) {
			/*
			 * Already set, do nothing 
			 */
			osdp->ioc_retval = UDI_OK;
			osdp->ioc_copyout = FALSE;
			_OSDEP_EVENT_SIGNAL(&osdp->ioc_sv);
			break;
		} else {
			udi_boolean_t add_failure;

			add_failure = l_netmap_mca_add((udi_ubit8_t *)osdp->
						       ioc_datap, osdp,
						       rdata->link_info.
						       media_type);
			if (add_failure) {
				osdp->ioc_retval = UDI_STAT_RESOURCE_UNAVAIL;
				osdp->ioc_copyout = FALSE;
				_OSDEP_EVENT_SIGNAL(&osdp->ioc_sv);
				break;
			}
			/*
			 * If the ND can't perfectly match,
			 * NSR needs to filter.
			 */
			osdp->nsr_should_filter_mcas =
				osdp->MCA_list.n_entries >
				rdata->link_info.max_perfect_multicast;

			/*
			 * We only accept one new MCA per SETMCA call.
			 * So, the transition to the first MCA over the
			 * max the ND can handle is the only time the ND
			 * is told to enable ALLMULTI_ON.
			 */
			if (osdp->MCA_list.n_entries ==
			    (rdata->link_info.max_total_multicast + 1)) {
#ifdef NSR_MCA_DEBUG
				cmn_err(CE_WARN,
					"!(ADDMCA)NSR to ND: enable ALLMULTI_ON");
#endif
				/*
				 * HW cant hold this many mca's.
				 * ND needs to be put into allmulti_on
				 * mode. Don't bother telling the ND
				 * about the new MCA, the NSR is
				 * handling filtering here from now on.
				 */
				osdp->nsr_should_filter_mcas = 1;
				ctrl_cb->command = UDI_NIC_ALLMULTI_ON;

				NSR_ctrl_req(ctrl_cb);
			} else if (osdp->MCA_list.n_entries >
				   rdata->link_info.max_total_multicast) {
				/*
				 * Don't bother the ND. It's already in
				 * ALLMULTI_ON mode.
				 */
				osdp->ioc_retval = UDI_OK;
				osdp->ioc_copyout = FALSE;
				_OSDEP_EVENT_SIGNAL(&osdp->ioc_sv);
			} else {
#ifdef NSR_MCA_DEBUG
				cmn_err(CE_WARN,
					"!(ADDMCA)NSR to ND: Program new table.");
#endif
				/*
				 * The ND hardware can handle this many
				 * addresses. Program them.
				 */

				if (osdp->MCA_list.n_entries > 0) {
					UDI_BUF_ALLOC(l_nsrmap_ioctl_cb2,
						      UDI_GCB(ctrl_cb),
						      osdp->ioc_datap,
						      addr_size,
						      rdata->buf_path);
				} else {
					UDI_BUF_ALLOC(l_nsrmap_ioctl_cb3,
						      UDI_GCB(ctrl_cb),
						      osdp->ioc_datap,
						      addr_size,
						      rdata->buf_path);
				}
			}
		}
		break;

	case MACIOC_DELMCA:		/* Disable MCA */
		ctrl_cb->command = UDI_NIC_DEL_MULTI;
		ctrl_cb->indicator = 1;	/* Only 1 address */
		udi_memcpy(&l_macaddr, macaddrp->ioc_data, sizeof (l_macaddr));
		if (!l_netmap_mca_found(macaddrp->ioc_data, osdp,
					rdata->link_info.media_type)) {
			/*
			 * Not a valid MCA, fail the request 
			 */
			osdp->ioc_retval = UDI_STAT_NOT_UNDERSTOOD;
			osdp->ioc_copyout = FALSE;
			_OSDEP_EVENT_SIGNAL(&osdp->ioc_sv);
		} else {
			udi_boolean_t del_failure;

			del_failure = l_netmap_mca_del((udi_ubit8_t *)osdp->
						       ioc_datap, osdp,
						       rdata->link_info.
						       media_type);
			if (del_failure) {
				osdp->ioc_retval = UDI_STAT_RESOURCE_UNAVAIL;
				osdp->ioc_copyout = FALSE;
				_OSDEP_EVENT_SIGNAL(&osdp->ioc_sv);
				break;
			}
			/*
			 * If the ND can't perfectly match,
			 * NSR needs to filter.
			 */
			osdp->nsr_should_filter_mcas =
				osdp->MCA_list.n_entries >
				rdata->link_info.max_perfect_multicast;

			/*
			 * Update Multicast table  -- if we have no MCA
			 * entries left we do not need to allocate an
			 * extra buffer to contain the new table
			 */

			if (osdp->MCA_list.n_entries <=
			    rdata->link_info.max_total_multicast) {
#ifdef NSR_MCA_DEBUG
				cmn_err(CE_WARN, "!(DELMCA)NSR to ND:"
					" Program new table (%d-entries).",
					osdp->MCA_list.n_entries);
#endif
				/*
				 * ND can be programmed with the mca table
				 */
				if (osdp->MCA_list.n_entries > 0) {
					UDI_BUF_ALLOC(l_nsrmap_ioctl_cb2,
						      UDI_GCB(ctrl_cb),
						      osdp->ioc_datap,
						      addr_size,
						      rdata->buf_path);
				} else {
					UDI_BUF_ALLOC(l_nsrmap_ioctl_cb3,
						      UDI_GCB(ctrl_cb),
						      osdp->ioc_datap,
						      addr_size,
						      rdata->buf_path);
				}
			} else {
				/*
				 * Else, the ND should already be in ALLMULTI_ON
				 * mode. Ack the ioctl, but don't tell the ND
				 * something it doesn't need to know about.
				 */
				osdp->ioc_retval = UDI_OK;
				osdp->ioc_copyout = FALSE;
				_OSDEP_EVENT_SIGNAL(&osdp->ioc_sv);
			}
		}
		break;

	case MACIOC_SETALLMCA:		/* Enable all Multicast Addresses */
		ctrl_cb->command = UDI_NIC_ALLMULTI_ON;
		osdp->nsr_should_filter_mcas = 0;

		NSR_ctrl_req(ctrl_cb);
		break;

	case MACIOC_DELALLMCA:		/* Revert to programmed MCA table */
		osdp->nsr_should_filter_mcas =
			osdp->MCA_list.n_entries >
			rdata->link_info.max_perfect_multicast;

		if (osdp->MCA_list.n_entries <=
		    rdata->link_info.max_total_multicast) {
#ifdef NSR_MCA_DEBUG
			cmn_err(CE_WARN, "!(DELALLMCA)NSR to ND:"
				" Program new table (%d-entries).",
				osdp->MCA_list.n_entries);
#endif
			/*
			 * The hardware can handle a reprogramming of the
			 * MCA list.
			 */
			ctrl_cb->command = UDI_NIC_ALLMULTI_OFF;
			ctrl_cb->indicator = osdp->MCA_list.n_entries;

			if (osdp->MCA_list.n_entries > 0) {
				UDI_BUF_ALLOC(l_nsrmap_ioctl_cb2,
					      UDI_GCB(ctrl_cb),
					      osdp->ioc_datap,
					      addr_size, rdata->buf_path);
			} else {
				UDI_BUF_ALLOC(l_nsrmap_ioctl_cb3,
					      UDI_GCB(ctrl_cb),
					      osdp->ioc_datap,
					      addr_size, rdata->buf_path);
			}
		} else {
#ifdef NSR_MCA_DEBUG
			cmn_err(CE_WARN,
				"!(DELALLMCA)NSR to ND: enable ALLMULTI_ON.");
#endif
			/*
			 * DELALLMCA will keep the driver in ALLMULTI_ON
			 * if the current table is too large for the
			 * hardware to handle.
			 */
			ctrl_cb->command = UDI_NIC_ALLMULTI_ON;
			osdp->nsr_should_filter_mcas = 1;

			NSR_ctrl_req(ctrl_cb);
		}
		break;

	case NSR_UDI_NIC_PROMISC_ON:
	case MACIOC_PROMISC:		/* Enable promiscuous mode      */
		ctrl_cb->command = UDI_NIC_PROMISC_ON;

		NSR_ctrl_req(ctrl_cb);
		break;

	case NSR_UDI_NIC_PROMISC_OFF:
		ctrl_cb->command = UDI_NIC_PROMISC_OFF;

		NSR_ctrl_req(ctrl_cb);
		break;

	case NSR_UDI_NIC_HW_RESET:
		ctrl_cb->command = UDI_NIC_HW_RESET;

		NSR_ctrl_req(ctrl_cb);
		break;

	case MACIOC_GETADDR:		/* Get current MAC address      */
	case MACIOC_GETRADDR:		/* Get Factory MAC address      */
		if (osdp->ioc_cmd == MACIOC_GETADDR) {
			ctrl_cb->command = UDI_NIC_GET_CURR_MAC;
		} else {
			ctrl_cb->command = UDI_NIC_GET_FACT_MAC;
		}
		osdp->ioc_copyout = TRUE;

		/*
		 * Allocate a buffer large enough for UDI_NIC_MAC_ADDRESS_SIZE
		 */
		UDI_BUF_ALLOC(l_nsrmap_ioctl_cb3, UDI_GCB(ctrl_cb), NULL,
			      UDI_NIC_MAC_ADDRESS_SIZE, rdata->buf_path);
		break;

	case MACIOC_SETADDR:		/* Set MAC address              */
		macaddrp = (nsr_macaddr_t *) osdp->ioc_cmdp;

		ctrl_cb->command = UDI_NIC_SET_CURR_MAC;
		ctrl_cb->indicator = macaddrp->ioc_len;

		/*
		 * Allocate a data buffer containing the MAC address 
		 */
		UDI_BUF_ALLOC(l_nsrmap_ioctl_cb3, UDI_GCB(ctrl_cb),
			      osdp->ioc_datap, (udi_size_t)macaddrp->ioc_len,
			      rdata->buf_path);
		break;

	case NSR_UDI_NIC_BAD_RXPKT_INTERN:
		ctrl_cb->command = UDI_NIC_BAD_RXPKT;
		ctrl_cb->indicator = 1;
		NSR_ctrl_req(ctrl_cb);
		break;

	case NSR_UDI_NIC_BAD_RXPKT:
		ctrl_cb->command = UDI_NIC_BAD_RXPKT;
		udi_memcpy(&ctrl_cb->indicator, macaddrp->ioc_data,
			   sizeof (ctrl_cb->indicator));
		NSR_ctrl_req(ctrl_cb);
		break;

	default:
		/*
		 * Unsupported ioctl operation, fail the request 
		 */
		osdp->ioc_retval = UDI_STAT_NOT_UNDERSTOOD;
		_OSDEP_EVENT_SIGNAL(&osdp->ioc_sv);
		break;
	}

	return;
}

/*
 * l_nsrmap_ioctl_cb2:
 * -----------------
 * Callback function for buffer allocation when using Multicast addresses.
 * This routine will append the full multicast table to the end of the new_buf
 * and then schedule the NSR_ctrl_req()
 */
STATIC void
l_nsrmap_ioctl_cb2(udi_cb_t *gcb,
		   udi_buf_t *new_buf)
{
	nsrmap_region_data_t *rdata = gcb->context;
	udi_nic_ctrl_cb_t *ctrl_cb = UDI_MCB(gcb, udi_nic_ctrl_cb_t);
	udi_size_t addr_size;
	netmap_osdep_t *osdp = rdata->osdep_ptr;
	mac_mcast_t *mca_info;
	udi_ubit8_t *mca_table;
	udi_size_t mca_table_size;

	ctrl_cb->data_buf = new_buf;

	addr_size = osnsr_mac_addr_size(rdata->link_info.media_type);

#ifdef NSR_MCA_DEBUG
	cmn_err(CE_WARN, "!(ioctl_cb2) command:%d perfect:%d total:%d cur:%d",
		ctrl_cb->command,
		rdata->link_info.max_perfect_multicast,
		rdata->link_info.max_total_multicast,
		osdp->MCA_list.n_entries);
#endif

	/*
	 * The mca_table could be yanked out under our feet during
	 * the udi_buf_write. That isn't very likely though.
	 */
	NSRMAP_ASSERT(osdp->MCA_list.mca_table_mp);
	NSRMAP_ASSERT(osdp->MCA_list.mca_table_mp->b_rptr);
	mca_info = (mac_mcast_t *) osdp->MCA_list.mca_table_mp->b_rptr;
	NSRMAP_ASSERT(mca_info->mac_mca_offset != 0);
	mca_table = (udi_ubit8_t *)mca_info + mca_info->mac_mca_offset;
	mca_table_size = mca_info->mac_mca_length;

	/*
	 * Copy the new multicast address table to the end of the buffer.
	 * This will have been updated by the l_netmap_mca_add, l_netmap_mca_del
	 * routines before we are called
	 */
	udi_buf_write(l_nsrmap_ioctl_cb3, gcb, mca_table, mca_table_size, new_buf, new_buf->buf_size,	/* Append */
		      0,		/* Don't replace */
		      UDI_NULL_BUF_PATH);
}

/*
 * l_nsrmap_ioctl_cb3:
 * -----------------
 * Callback function to send the completed udi_nic_ctrl_cb_t down to the ND.
 * The buffer has been completely filled with all applicable data by this time.
 */
STATIC void
l_nsrmap_ioctl_cb3(udi_cb_t *gcb,
		   udi_buf_t *new_buf)
{
	nsrmap_region_data_t *rdata = gcb->context;
	udi_nic_ctrl_cb_t *ctrl_cb = UDI_MCB(gcb, udi_nic_ctrl_cb_t);

	/*
	 * Release old (stale) buffer if it's been used
	 */
	if (ctrl_cb->data_buf != new_buf) {
		udi_buf_free(ctrl_cb->data_buf);
	}

	ctrl_cb->data_buf = new_buf;

	NSR_ctrl_req(ctrl_cb);
}

/*
 * l_netmap_mca_add:
 * ---------------
 * Add a new MAC address to the list of multicast addresses already active.
 * The internal multicast table (for passing a copy to the ND)
 * is always reallocated.
 * If a memory allocation error occurred, TRUE is returned else FALSE.
 */
STATIC udi_boolean_t
l_netmap_mca_add(udi_ubit8_t *p,	/* MAC address */

		 netmap_osdep_t * osdp,	/* MCA list reference */

		 udi_ubit8_t media_type)
{					/* MCA address type */
	udi_size_t addr_size;
	mblk_t *old_table, *adjusted_mp, *mctable_mp, *new_addr_mp;
	mac_mcast_t *mca_info;
	udi_size_t mca_table_size;

	addr_size = osnsr_mac_addr_size(media_type);
#if NSR_MCA_DEBUG
	cmn_err(CE_WARN, "netmap_mca_add: Adding ");
	nsr_dump_media_addr(media_type, p);
#endif

	/*
	 * Get the MDI multicast table 
	 */
	mctable_mp = mdi_get_mctable(osdp->mac_cookie);
	if (mctable_mp == NULL) {
		cmn_err(CE_WARN, "!netmap_mca_add: invalid mdi mctable");
		return TRUE;
	}
#if NSR_MCA_DEBUG
	cmn_err(CE_WARN, "mca_add: dump before:");
	nsr_dump_mctable_mp(mctable_mp, media_type);
#endif

	/*
	 * Allocate memory for the new entry 
	 */
	new_addr_mp = allocb(addr_size, BPRI_LO);	/* new entry, attach it. */
	if (new_addr_mp == NULL) {
		freemsg(mctable_mp);
		return TRUE;
	}

	/*
	 * Copy in the new entry 
	 */
	udi_memcpy(new_addr_mp->b_rptr, p, addr_size);
	new_addr_mp->b_wptr = new_addr_mp->b_rptr + addr_size;

	/*
	 * Tack it onto the original mctable 
	 */
	mctable_mp->b_cont = new_addr_mp;
	/*
	 * Chop off extra bytes MDI was using at end of the original mctable 
	 */
	mca_info = (mac_mcast_t *) mctable_mp->b_rptr;
	mctable_mp->b_wptr = mctable_mp->b_rptr +
		mca_info->mac_mca_offset + mca_info->mac_mca_length;
	/*
	 * Merge the data blocks. 
	 */
	adjusted_mp = msgpullup(mctable_mp, -1);
	freemsg(mctable_mp);
	if (adjusted_mp == NULL) {
		return TRUE;
	}
	mctable_mp = adjusted_mp;
	mca_info = (mac_mcast_t *) mctable_mp->b_rptr;

	_OSDEP_SIMPLE_MUTEX_LOCK(&osdp->mca_mutex);
	old_table = osdp->MCA_list.mca_table_mp;
	NSRMAP_ASSERT(mctable_mp != NULL);
	NSRMAP_ASSERT(mca_info->mac_mca_offset != 0);
	osdp->MCA_list.mca_table_mp = mctable_mp;
	mca_info->mac_mca_count += 1;
	mca_info->mac_mca_length += addr_size;
	osdp->MCA_list.n_entries = mca_info->mac_mca_count;
#if NSR_MCA_DEBUG
	cmn_err(CE_NOTE, "mca_add: dump after:");
	nsr_dump_mctable_mp(mctable_mp, media_type);
#endif
	_OSDEP_SIMPLE_MUTEX_UNLOCK(&osdp->mca_mutex);
	if (old_table)
		freemsg(old_table);

	return FALSE;
}

/*
 * l_netmap_mca_del:
 * ---------------
 * Delete the specified multicast address from the MCA_list.
 * The internal multicast table (for passing a copy to the ND)
 * is always reallocated.
 */
STATIC udi_boolean_t
l_netmap_mca_del(udi_ubit8_t *p,	/* MAC address */

		 netmap_osdep_t * osdp,	/* MCA list reference */

		 udi_ubit8_t media_type)
{					/* MCA address type */
	udi_ubit8_t *q, *q_start, *q_end;
	udi_size_t addr_size;
	mblk_t *mctable_mp, *old_table;
	mac_mcast_t *mca_info;

#if NSR_MCA_DEBUG
	cmn_err(CE_WARN, "netmap_mca_del: Removing ");
	nsr_dump_media_addr(media_type, p);
#endif
	addr_size = osnsr_mac_addr_size(media_type);

	/*
	 * Get the MDI multicast table 
	 */
	mctable_mp = mdi_get_mctable(osdp->mac_cookie);
	if (mctable_mp == NULL) {
		cmn_err(CE_WARN, "!netmap_mca_add: invalid mdi mctable");
		return TRUE;
	}
#if NSR_MCA_DEBUG
	cmn_err(CE_WARN, "mca_del: dump before");
	nsr_dump_mctable_mp(mctable_mp, media_type);
#endif
	mca_info = (mac_mcast_t *) mctable_mp->b_rptr;
	q_start = (udi_ubit8_t *)mctable_mp->b_rptr + mca_info->mac_mca_offset;
	q_end = q_start + mca_info->mac_mca_length;
	if ((mca_info->mac_mca_count <= 0) ||
	    (mca_info->mac_mca_offset <= 0) ||
	    (mca_info->mac_mca_length <= 0)) {
#if NSR_MCA_DEBUG
		cmn_err(CE_WARN, "netmap_mca_del: invalid mdi mca info");
#endif
		return TRUE;
	}

	/*
	 * Find address in old list, copy start..address to new list 
	 */

	for (q = q_start; q < q_end; q += addr_size) {
		if (udi_memcmp(q, p, addr_size) == 0) {
			mblk_t *new_list_mp;

			udi_memset(q, 0xC0, addr_size);
			/*
			 * Copy entries skipping entry <q> 
			 */
			new_list_mp = mctable_mp;
			if (q < q_end - addr_size) {
				udi_memmove(q, q + addr_size,
					    q_end - q - addr_size);
			}
			_OSDEP_SIMPLE_MUTEX_LOCK(&osdp->mca_mutex);
			mca_info = (mac_mcast_t *) new_list_mp->b_rptr;
			mca_info->mac_mca_count -= 1;
			mca_info->mac_mca_length -= addr_size;
			osdp->MCA_list.n_entries = mca_info->mac_mca_count;
			if (osdp->MCA_list.mca_table_mp) {
				freemsg(osdp->MCA_list.mca_table_mp);
			}
			osdp->MCA_list.mca_table_mp = new_list_mp;
#if NSR_MCA_DEBUG
			cmn_err(CE_WARN, "mca_del: dump after");
			nsr_dump_mctable_mp(new_list_mp, media_type);
#endif
			_OSDEP_SIMPLE_MUTEX_UNLOCK(&osdp->mca_mutex);
			return FALSE;
		}
	}
	return TRUE;
}

/*
 * l_netmap_mca_found:
 * -----------------
 * Scan the MCA list held in <osdp> looking for a match against the MAC address
 * referenced by <p>
 */
STATIC udi_boolean_t
l_netmap_mca_found(udi_ubit8_t *p,	/* MAC address */

		   netmap_osdep_t * osdp,	/* OS-dependent MCA list reference */

		   udi_ubit8_t media_type)
{					/* MCA address type */
	/*
	 * returns true if the multicast address is valid for this adapter 
	 */
	return mdi_valid_mca(osdp->mac_cookie, p);
}

/*
 * l_nsrmap_get_stats:
 * -----------------
 * Allocate a udi_nic_info_cb_t and issue a udi_nd_info_req to obtain the
 * driver specific counts. On completion, (*cbfn) will be called
 * with the instantiated driver-specific info.
 */
STATIC void
l_nsrmap_get_stats(queue_t * q,
		   mblk_t * mp,
		   void (*cbfn) (udi_cb_t *))
{
	nsrmap_init_data_t *gdata = q->q_ptr;
	nsrmap_region_data_t *rdata = gdata->rdata;
	netmap_osdep_t *osdp = rdata->osdep_ptr;

	rdata->next_func = cbfn;
	osdp->scratch_msg = (void *)mp;
	osdp->scratch_ptr = (void *)q;

	udi_cb_alloc(l_nsrmap_get_stats_cbfn, rdata->bind_cb,
		     NSRMAP_INFO_CB_IDX, rdata->bind_info.bind_channel);
}

STATIC void
l_nsrmap_get_stats_cbfn(udi_cb_t *gcb,
			udi_cb_t *new_cb)
{
	nsrmap_region_data_t *rdata = gcb->context;
	udi_nic_info_cb_t *info_cb = UDI_MCB(new_cb, udi_nic_info_cb_t);

	UDI_GCB(info_cb)->channel = rdata->bind_info.bind_channel;
	udi_nd_info_req(info_cb, 0);
}

STATIC void
l_nsrmap_return_stats(udi_cb_t *cb)
{
	nsrmap_region_data_t *rdata = cb->context;
	mblk_t *mp = (mblk_t *) rdata->osdep_ptr->scratch_msg;
	queue_t *q = (queue_t *) rdata->osdep_ptr->scratch_ptr;
	struct iocblk *iocp = (struct iocblk *)mp->b_rptr;
	unsigned char *data;
	udi_nic_info_cb_t *info_cb = UDI_MCB(cb, udi_nic_info_cb_t);

	rdata->next_func = NULL;
	rdata->osdep_ptr->scratch_msg = NULL;
	rdata->osdep_ptr->scratch_ptr = NULL;

	data = mp->b_cont ? mp->b_cont->b_rptr : NULL;

	if (data == NULL) {
		iocp->ioc_error = EINVAL;
		mp->b_datap->db_type = M_IOCNAK;
	} else {
		mac_stats_eth_t *eth_stats;

		switch (rdata->link_info.media_type) {
		case UDI_NIC_ETHER:
		case UDI_NIC_FASTETHER:
		case UDI_NIC_GIGETHER:
			/*
			 * Both sides (UDI and MDI) support a rich set
			 * of statistics.   MDI seems to keep its own 
			 * packet counts elsewhere.   Map between them
			 * here.
			 */
			eth_stats = &rdata->osdep_ptr->s_u.e;
			eth_stats->mac_align = 0;
			eth_stats->mac_badsum = info_cb->rx_errors;
			eth_stats->mac_sqetesterrors = 0;
			eth_stats->mac_frame_def = 0;
			eth_stats->mac_oframe_coll = 0;
			eth_stats->mac_xs_coll = 0;
			eth_stats->mac_tx_errors = info_cb->tx_discards;
			eth_stats->mac_carrier = 0;
			eth_stats->mac_badlen = 0;
			eth_stats->mac_no_resource = 0;
			eth_stats->mac_colltable[0] = info_cb->collisions;
			eth_stats->mac_spur_intr = 0;
			eth_stats->mac_frame_nosr = info_cb->rx_discards;
			eth_stats->mac_baddma = info_cb->tx_underrun +
				info_cb->rx_overrun;
			eth_stats->mac_timeouts = 0;
			udi_memcpy(data, eth_stats, sizeof (mac_stats_eth_t));
			mp->b_cont->b_wptr = mp->b_cont->b_rptr
				+ sizeof (mac_stats_eth_t);
			break;
		case UDI_NIC_TOKEN:
			/*
			 * TODO: Update mac_stats_tr_t from driver info 
			 */
			udi_memcpy(data,
				   &rdata->osdep_ptr->s_u.t,
				   sizeof (mac_stats_tr_t));
			mp->b_cont->b_wptr = mp->b_cont->b_rptr
				+ sizeof (mac_stats_tr_t);
			break;
		case UDI_NIC_FDDI:
			/*
			 * TODO: Update mac_stats_fddi_t from driver info 
			 */
			udi_memcpy(data,
				   &rdata->osdep_ptr->s_u.f,
				   sizeof (mac_stats_fddi_t));
			mp->b_cont->b_wptr = mp->b_cont->b_rptr
				+ sizeof (mac_stats_fddi_t);
			break;
		}

		iocp->ioc_error = 0;
		mp->b_datap->db_type = M_IOCACK;
	}

	qreply(q, mp);

	udi_cb_free(cb);

}

/*
 * l_nsrmap_return_local_stats:
 * --------------------------
 * Callback function for mapper-initiated stats request. This is typically used
 * by the MAC_INFO_REQ handling code to obtain the underlying speed of the
 * driver. In this case we simply need to update the region local data with
 * the information and awaken the blocked user request.
 */
STATIC void
l_nsrmap_return_local_stats(udi_cb_t *cb)
{
	nsrmap_region_data_t *rdata = cb->context;
	udi_nic_info_cb_t *info_cb = UDI_MCB(cb, udi_nic_info_cb_t);

	rdata->osdep_ptr->link_bps = info_cb->link_bps;
	rdata->osdep_ptr->link_mbps = info_cb->link_mbps;

	/*
	 * Wake up the blocked user request 
	 */
	_OSDEP_EVENT_SIGNAL(&rdata->osdep_ptr->op_close_sv);

	udi_cb_free(cb);

}

/*
 * nsrmap_valid_type:
 * -----------------
 * Return TRUE if the passed media_type matches one of our supported types,
 * FALSE otherwise
 */
STATIC udi_boolean_t
nsrmap_valid_type(udi_ubit32_t media_type)
{
	switch (media_type) {
	case UDI_NIC_ETHER:
	case UDI_NIC_TOKEN:
	case UDI_NIC_FASTETHER:
	case UDI_NIC_GIGETHER:
	case UDI_NIC_FDDI:
		return TRUE;
	default:
		return FALSE;
	}
}

/*
 * nsrmap_type:
 * -----------
 * Convert the given UDI media type into the corresponding UnixWare MDI
 * mac_type value.
 */
STATIC udi_ubit32_t
nsrmap_type(udi_ubit32_t media_type)
{
	switch (media_type) {
	case UDI_NIC_ETHER:
	case UDI_NIC_FASTETHER:
	case UDI_NIC_GIGETHER:
		return MAC_CSMACD;
	case UDI_NIC_FDDI:
		return MAC_FDDI;
	case UDI_NIC_TOKEN:
		return MAC_TPR;
	default:
		udi_assert(0);
	}
}

/*
 * nsrmap_proto:
 * ------------
 * Handle MDI protocol messages
 *
 * Calling Context:
 *	plstr
 */
STATIC void
nsrmap_proto(queue_t * q,
	     mblk_t * mp)
{
	nsrmap_init_data_t *gdata = q->q_ptr;
	nsrmap_region_data_t *rdata = gdata->rdata;

#ifdef NSR_USE_THREAD
	if (NSR_enqueue(rdata, q, mp, SendProto)) {
		putbq(q, mp);
	}
#else
	nsrmap_proto1(q, mp);
#endif
	return;
}

/*
 * nsrmap_proto1:
 * -------------
 * Handle MDI protocol messages
 *
 * Calling Context:
 *	plbase
 */
STATIC void
nsrmap_proto1(queue_t * q,
	      mblk_t * mp)
{
	nsrmap_init_data_t *gdata = q->q_ptr;
	nsrmap_region_data_t *rdata = gdata->rdata;
	mac_info_ack_t *info_ack;
	union MAC_primitives *prim;
	mblk_t *reply_mp, *newmp;
	struct iocblk *iocp;

	prim = (union MAC_primitives *)mp->b_rptr;

#ifdef NSR_PARANOID
	udi_assert((mp->b_rptr >= mp->b_datap->db_base) &&
		   (mp->b_wptr >= mp->b_rptr) &&
		   (mp->b_wptr <= mp->b_datap->db_lim));
#endif /* NSR_PARANOID */

#if defined(_KERNEL) && defined(VERBOSE_DEBUG)
	cmn_err(CE_WARN, "!nsrmap_proto(%p, %p) rdata = %p", q, mp, rdata);
#endif /* _KERNEL && DEBUG */
	switch (prim->mac_primitive) {
	case MAC_INFO_REQ:
		if ((reply_mp =
		     allocb(sizeof (mac_info_ack_t), BPRI_HI)) == NULL) {
#ifdef _KERNEL
			cmn_err(CE_WARN, "!nsrmap_proto - out of STREAMs");
#endif /* _KERNEL */
		} else {
			/*
			 * TODO: Issue a udi_nd_info_req to get speed of card 
			 */
			if (!nsrmap_valid_type(rdata->link_info.media_type))
				break;

			/*
			 * Get the link speed from the driver 
			 */
			l_nsrmap_get_stats(q, mp, l_nsrmap_return_local_stats);

			_OSDEP_EVENT_WAIT(&rdata->osdep_ptr->op_close_sv);

			info_ack = (mac_info_ack_t *) reply_mp->b_rptr;
			reply_mp->b_wptr += sizeof (mac_info_ack_t);
			reply_mp->b_datap->db_type = M_PCPROTO;
			info_ack->mac_primitive = MAC_INFO_ACK;
			info_ack->mac_max_sdu = rdata->link_info.max_pdu_size;
			info_ack->mac_min_sdu = rdata->link_info.min_pdu_size;
			info_ack->mac_mac_type =
				nsrmap_type(rdata->link_info.media_type);
			info_ack->mac_driver_version = MDI_VERSION;
			/*
			 * Convert link speed to bits/s 
			 */
			if (rdata->osdep_ptr->link_bps) {
				info_ack->mac_if_speed =
					rdata->osdep_ptr->link_bps;
			} else {
				info_ack->mac_if_speed =
					rdata->osdep_ptr->link_mbps * 1000000;
			}

			qreply(q, reply_mp);

			/*
			 * Issue a udi_nd_ctrl_req to enable bad packets 
			 */
			newmp = allocb(sizeof (struct iocblk), BPRI_MED);
			if (newmp != NULL) {
				newmp->b_datap->db_type = M_IOCTL;
				newmp->b_rptr = newmp->b_datap->db_base;
				newmp->b_wptr = newmp->b_rptr + sizeof (*iocp);
				udi_memset(newmp->b_rptr, 0, sizeof (*iocp));
				iocp = (struct iocblk *)newmp->b_rptr;
				iocp->ioc_cmd = NSR_UDI_NIC_BAD_RXPKT_INTERN;

				if (NSR_enqueue(rdata, q, newmp, SendIoctl)) {
					putbq(q, newmp);
				}
			}

		}
		break;

	case MAC_BIND_REQ:
		if (rdata->osdep_ptr->bound == TRUE) {
			mdi_macerrorack(RD(q), prim->mac_primitive,
					MAC_OUTSTATE);
			break;
		}
		rdata->osdep_ptr->bound = TRUE;
		/*
		 * Save the cookie for later use 
		 */
		rdata->osdep_ptr->mac_cookie = prim->bind_req.mac_cookie;
		mdi_macokack(RD(q), prim->mac_primitive);
		break;
	default:
		mdi_macerrorack(RD(q), prim->mac_primitive, MAC_BADPRIM);
		break;
	}

	freemsg(mp);
	return;
}

/*
 * nsrmap_data:
 * -----------
 * Submit data messages to UDI NSR
 *
 * Calling Context:
 *	plstr
 */
STATIC void
nsrmap_data(queue_t * q,
	    mblk_t * mp,
	    udi_boolean_t putback)
{
	nsrmap_init_data_t *gdata = q->q_ptr;
	nsrmap_region_data_t *rdata = gdata->rdata;

#if !defined(NSR_MDI_NO_LOOPBACK)
	netmap_osdep_t *osdp = rdata->osdep_ptr;
	macaddr_t *ea;
#endif /* ! NSR_MDI_NO_LOOPBACK */

#if !defined(NSR_MDI_NO_LOOPBACK)
	ea = (macaddr_t *) mp->b_rptr;
	if (BTST(rdata->NSR_state, NsrmapS_Promiscuous) || UDI_BCAST(*ea) ||
	    (UDI_MCA(*ea, rdata->link_info.media_type) &&
	     l_netmap_mca_found(*ea, osdp,
				rdata->link_info.media_type)) ||
	    mdi_addrs_equal(rdata->link_info.mac_addr, *ea)) {
		mdi_do_loopback(q, mp, 60);
	}
#endif /* ! NSR_MDI_NO_LOOPBACK */

#ifdef NSR_USE_THREAD
	if (NSR_enqueue(rdata, q, mp, putback ? SendDataPB : SendData)) {
		if (putback)
			putbq(q, mp);
		else
			putq(q, mp);
	}
#else
	nsrmap_data1(q, mp, putback);
#endif
	return;
}

/*
 * External Buffer manipulation routines.
 *
 * nsrmap_mblk_free:
 * ----------------
 * Called when the external udi_buf_t has been freed. Releases the associated
 * streams mblk back to the system.
 */
STATIC void
nsrmap_mblk_free(void *context,
		 udi_ubit8_t *space,
		 udi_size_t size)
{
	freeb((mblk_t *) context);
}

_udi_xbops_t nsrmap_xbops = {
	nsrmap_mblk_free
#ifdef _UDI_PHYSIO_SUPPORTED
		, NULL,
	NULL
#endif					/* _UDI_PHYSIO_SUPPORTED */
};

/*
 * nsrmap_data1:
 * ------------
 * Submit data messages to UDI NSR
 *
 * Calling Context:
 *	plbase
 */
STATIC void
nsrmap_data1(queue_t * q,
	     mblk_t * mp,
	     udi_boolean_t putback)
{
	nsrmap_init_data_t *gdata = q->q_ptr;
	nsrmap_Tx_region_data_t *rdata = gdata->txdata;
	udi_size_t totsize;
	nsr_cb_t *pcb;
	udi_cb_t *gcb;

	totsize = (udi_size_t)msgdsize(mp);

	/*
	 * Check for outsize message blocks -- shouldn't happen
	 */
	if (totsize > rdata->buf_size) {
		cmn_err(CE_WARN, "!nsrmap_data1: Oversize request [%d]",
			totsize);
		rdata->n_discards++;
		freemsg(mp);
		return;
	}
#ifdef NSR_PARANOID
	udi_assert((mp->b_rptr >= mp->b_datap->db_base) &&
		   (mp->b_wptr >= mp->b_rptr) &&
		   (mp->b_wptr <= mp->b_datap->db_lim));
#endif /* NSR_PARANOID */

	/*
	 * If there isn't a Tx CB available, defer processing this request.
	 * nsrmap_wsrv() will retry this.
	 */
	pcb = (nsr_cb_t *) NSR_UDI_DEQUEUE_HEAD(&rdata->cbq);
	if (!pcb) {
		if (putback)
			putbq(q, mp);
		else
			putq(q, mp);
		return;
	}

	gcb = pcb->cbp;
	NSR_DBG(udi_assert(gcb != NULL));
	NSR_SET_SCRATCH(gcb->scratch, pcb);
	NSR_SET_SCRATCH_1(gcb->scratch, 0);	/* scratch offset */
	NSR_SET_SCRATCH_2(gcb->scratch, mp);	/* scratch pointer */
	NSR_SET_SCRATCH_3(gcb->scratch, mp);	/* scratch message */

#ifdef NSR_USE_SERIALISATION
	/*
	 * Enqueue a callback to the NSR so that we benefit from the region
	 * serialisation
	 */
	l_osnsr_enqueue_callback(gcb, osnsr_tx_packet);
#else
	/*
	 * Dangerous ... no region serialisation 
	 */
	osnsr_tx_packet(gcb);
#endif /* NSR_USE_SERIALISATION */
}


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
	static int first_time = 1;
	netmap_osdep_t *osdp;

	rdata->osdep_ptr = (netmap_osdep_t *) & rdata->osdep;
	osdp = rdata->osdep_ptr;

#if 0
	_OSDEP_EVENT_INIT(&osdp->sv1);
	_OSDEP_EVENT_INIT(&osdp->sv2);
#endif
	_OSDEP_EVENT_INIT(&osdp->op_close_sv);
	_OSDEP_MUTEX_INIT(&osdp->op_mutex, "NSR Open Lock");
	_OSDEP_EVENT_INIT(&osdp->ioc_sv);

	/*
	 * Initialise the modname queue used to keep track of when its safe to
	 * remove a particular driver from the OS namespace
	 * Note: nsrmap_mod_Q is _global_ data so we must only initialise it
	 *       once (not on every region initialisation)
	 */

	if (first_time) {
		first_time = 0;
		UDI_QUEUE_INIT(&nsrmap_mod_Q);
	}
#ifdef NSR_USE_THREAD_LOCK
	_OSDEP_SIMPLE_MUTEX_INIT(&osdp->thread_mutex, "NSR Thread Lock");
#endif /* NSR_USE_THREAD_LOCK */

	_OSDEP_SIMPLE_MUTEX_INIT(&osdp->mca_mutex, "NSR Multicast List Lock");
	osdp->MCA_list.mca_table_mp = NULL;
}

/*
 * osnsr_final_cleanup:
 * -------------------
 * Release any held resources back to the system
 */
STATIC void
osnsr_final_cleanup(udi_mgmt_cb_t *cb)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;
	netmap_osdep_t *osdp = rdata->osdep_ptr;

#if 0
	_OSDEP_EVENT_DEINIT(&osdp->sv1);
	_OSDEP_EVENT_DEINIT(&osdp->sv2);
#endif
	_OSDEP_EVENT_DEINIT(&osdp->op_close_sv);
	_OSDEP_MUTEX_DEINIT(&osdp->op_mutex);
	_OSDEP_EVENT_DEINIT(&osdp->ioc_sv);

	_OSDEP_SIMPLE_MUTEX_DEINIT(&osdp->mca_mutex);
	if (osdp->MCA_list.mca_table_mp) {
		freemsg(osdp->MCA_list.mca_table_mp);
	}
	osdp->MCA_list.mca_table_mp = NULL;

#ifdef NSR_USE_THREAD_LOCK
	_OSDEP_SIMPLE_MUTEX_DEINIT(&osdp->thread_mutex);
#endif /* NSR_USE_THREAD_LOCK */

	return;
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
#if VERBOSE_DEBUG
	cmn_err(CE_WARN, "osnsr_status_ind: event = %p\n", cb->event);
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
		return 1514;		/* Ethernet II frame size */
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
		return 14;		/* Link-layer header */
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
	switch (media_type) {
	case UDI_NIC_ETHER:
	case UDI_NIC_TOKEN:
	case UDI_NIC_FASTETHER:
	case UDI_NIC_GIGETHER:
	case UDI_NIC_VGANYLAN:
	case UDI_NIC_FDDI:
	case UDI_NIC_MISCMEDIA:
	default:
		return 6;
	case UDI_NIC_ATM:
		return 20;
	case UDI_NIC_FC:
		return 8;
	}
}

/*
 * nsrmap_make_macaddr
 * -------------------
 * Convert the specified MAC address into a printable string. Maximum length
 * is given in <maclen>, destination buffer in <buff>
 * The buffer must be large enough for 3 * maclen bytes. This corresponds
 * to 2 digits per byte plus a separator and a trailing NULL.
 * E.g.  aa:bb:cc:dd:ee:ff\0
 */
STATIC void
nsrmap_make_macaddr(char *buff,
		    udi_ubit8_t *macaddr,
		    udi_ubit8_t maclen)
{
	udi_size_t macsize = (udi_size_t)(maclen * 3);
	udi_ubit8_t i;
	char *p;

	if (macsize == 0) {
		udi_snprintf(buff, udi_strlen("UNKNOWN") + 1, "UNKNOWN");
		return;
	}

	udi_memset(buff, 0, macsize);

	for (p = buff, i = 0; i < maclen; i++) {
		if (i == 0) {
			udi_snprintf(p, 3, "%02x", macaddr[i]);
			p += 2;
		} else {
			udi_snprintf(p, 4, ":%02x", macaddr[i]);
			p += 3;
		}
	}
}

/*
 * l_netmap_mca_filter:
 * ---------------
 * Filter multicast packets when...
 *   1) the ND is in imperfect matching mode
 *   2) the ND can't hold enough addresses.
 * Returns TRUE when the address was a multicast address that
 *   matched a filtered address, or the address wasn't a multicast
 *   address.
 * Returns FALSE when the multicast address didn't match the filter.
 */
STATIC udi_boolean_t
l_netmap_mca_filter(udi_nic_rx_cb_t *rxcb,
		    udi_ubit8_t *mac_addr,
		    udi_ubit8_t media_type)
{
	nsrmap_region_data_t *rdata = UDI_GCB(rxcb)->context;
	netmap_osdep_t *osdp = rdata->osdep_ptr;
	udi_boolean_t sendupstream = TRUE;

	if (UDI_MCA(mac_addr, media_type) &&
	    !BTST(rdata->NSR_state, NsrmapS_Promiscuous) &&
	    !UDI_BCAST(mac_addr) &&
	    osdp->nsr_should_filter_mcas &&
	    ((rxcb->addr_match == UDI_NIC_RX_UNKNOWN)
	     || (rxcb->addr_match == UDI_NIC_RX_HASH))) {
		/*
		 * mdi_valid_mca could be:
		 *     _OSDEP_VALIDATE_MCA(media_type, osdp, mcaddr)
		 */
		sendupstream = mdi_valid_mca(osdp->mac_cookie, mac_addr);
#ifdef NSR_MCA_DEBUG
		cmn_err(CE_WARN,
			"!(mca_filter)valid:%d mac_addr:%x:%x:%x:%x:%x:%x",
			sendupstream, mac_addr[0], mac_addr[1], mac_addr[2],
			mac_addr[3], mac_addr[4], mac_addr[5]);
#endif
	}
	return sendupstream;
}

/*
 * osnsr_rx_packet:
 * ---------------
 * Wake up any sleeping user getmsg() to signal that there's some data to
 * read.
 */
void
osnsr_rx_packet(udi_nic_rx_cb_t *rxcb,
		udi_boolean_t expedited)
{
	nsrmap_region_data_t *rdata = UDI_GCB(rxcb)->context;
	mblk_t *mp;
	udi_nic_rx_cb_t *tcb = rxcb, *next;
	queue_t *rxQ = rdata->osdep_ptr->upQ;
	udi_size_t amount;
	nsr_cb_t *pcb;
	udi_boolean_t return_packet = TRUE;

	udi_assert(rdata->rdatap == rdata);
	rdata->n_packets++;

	nsrmap_rstats.ms_pcnt++;	/* Increment Read stats */

#ifdef NSR_SINGLE_RX_BUF
	for (tcb = rxcb; tcb; tcb = next) {

		next = tcb->chain;
#endif /* NSR_SINGLE_RX_BUF */

		/*
		 * Get a container for this request from the list of owned 
		 * containers. This will be moved back to the rxonq when the 
		 * packet is returned to the NSR via NSR_return_packet. 
		 * Until that time the container is in limbo and can only be 
		 * accessed via rxcb->gcb.scratch
		 */
		pcb = (nsr_cb_t *) NSR_UDI_DEQUEUE_HEAD(&rdata->rxonq);
		NSRMAP_ASSERT(pcb != NULL);
		pcb->cbp = UDI_GCB(tcb);
		NSR_SET_SCRATCH(pcb->cbp->scratch, pcb);

#ifdef NSR_SINGLE_RX_BUF
		tcb->chain = NULL;	/* Break chain */
#endif /* NSR_SINGLE_RX_BUF */

		/*
		 * Pump the data up the receive Q. If there isn't a queue, just
		 * update the stats to show yet another dropped packet
		 */
		if (rdata->osdep_ptr->upQ) {
			mp = udi_rxcb_to_mblk(tcb, l_osnsr_return_packet,
					      UDI_GCB(tcb));
			if (mp == NULL) {
				/*
				 * Drop packet and update n_discards 
				 */
				rdata->n_discards++;
				NSR_return_packet(tcb);
			} else {
				if (l_netmap_mca_filter(tcb, mp->b_rptr,
							rdata->link_info.
							media_type)) {
					/*
					 * Push data upstream 
					 */
					return_packet = FALSE;
					putnext(rxQ, mp);
				} else {
					/*
					 * Drop packet and update n_discards 
					 */
					rdata->n_discards++;
					NSR_return_packet(tcb);
				}
			}
		} else {
			rdata->n_discards++;
			NSR_return_packet(tcb);
		}
#ifdef NSR_SINGLE_RX_BUF
	}
#endif /* NSR_SINGLE_RX_BUF */
}

/*
 * l_osnsr_return_packet:
 * --------------------
 * Wrapper function for NSR_return_packet.
 * Schedule a callback to the NSR Rx region to call NSR_return_packet when the
 * region becomes inactive.
 *
 * Calling Context:
 *	strdaemon
 */
STATIC void
l_osnsr_return_packet(udi_cb_t *cb)
{
#ifdef NSR_USE_SERIALISATION
	l_osnsr_enqueue_callback(cb, (void (*)(udi_cb_t *))NSR_return_packet);
#else
	/*
	 * Dangerous ... no serialisation 
	 */
	NSR_return_packet(UDI_MCB(cb, udi_nic_rx_cb_t));
#endif /* NSR_USE_SERIALISATION */
}

STATIC void
osnsr_tx_rdy(nsrmap_Tx_region_data_t * txdata)
{
	netmap_osdep_tx_t *txosdep;
	queue_t *q;

	if (!(txosdep = txdata->osdep_tx_ptr))
		return;

	_OSDEP_SIMPLE_MUTEX_LOCK(&txosdep->wep->enable_mutex);
	q = (queue_t *) txosdep->wep->enableq;
	txosdep->wep->enableq = NULL;
	_OSDEP_SIMPLE_MUTEX_UNLOCK(&txosdep->wep->enable_mutex);

	if (q)
		qenable(q);
}


/*
 * l_osnsr_enqueue_callback:
 * -----------------------
 * Schedule a callback to the NSR region. This serialises all access to the
 * region data structures and simplifies the locking requirements for the
 * structures
 *
 * Calling Context:
 *	Kernel
 */
STATIC void
l_osnsr_enqueue_callback(udi_cb_t *gcb,
			 void (*cbfn) (udi_cb_t *))
{
	_udi_channel_t *chan = (_udi_channel_t *)gcb->channel;
	_udi_region_t *region = chan->chan_region;
	_udi_cb_t *_cb = _UDI_CB_TO_HEADER(gcb);

	_cb->cb_flags |= _UDI_CB_CALLBACK;
	_cb->cb_func = (_udi_callback_func_t *)cbfn;
	/*
	 * Sleaze alert: we only need a callback that takes one arg.
	 * so we hijack the _UDI_CT_CANCEL op_idx. This should result
	 * in a delayed call to (*cbfn)( gcb )
	 */
	_cb->op_idx = _UDI_CT_CANCEL;

	/*
	 * Lock region as we now may contend with already active thread 
	 */
	REGION_LOCK(region);
	/*
	 * Append the callback to the end of the region's requests 
	 */
	_UDI_REGION_Q_APPEND(region, &_cb->cb_qlink);

	/*
	 * First try to run the region directly. 
	 */
	if (_udi_set_region_active(region)) {
		UDI_RUN_REGION_QUEUE(region);
	} else {
		if (!REGION_IS_ACTIVE(region) && !REGION_IN_SCHED_Q(region)) {
			/*
			 * Schedule this region to become active 
			 */
			_OSDEP_REGION_SCHEDULE(region);
			return;
		}
	}
	REGION_UNLOCK(region);
}

/*
 * osnsr_tx_packet:
 * ---------------
 * Callback routine which starts a transmit request off in NSR context.
 * The original request information is stored in the scratch area and can be
 * accessed to find the request size etc.
 *
 * Calling Context:
 *	NSR	[callback scheduled by l_osnsr_enqueue_callback]
 */
STATIC void
osnsr_tx_packet(udi_cb_t *gcb)
{
	nsrmap_Tx_region_data_t *rdata = gcb->context;
	mblk_t *mp;
	udi_size_t totsize;

	NSR_GET_SCRATCH_3(gcb->scratch, mp);	/* Original message */
	totsize = msgdsize(mp);

	/*
	 * Check for oversize Tx message. This _shouldn't_ happen.
	 */
	if (totsize > rdata->buf_size) {
		cmn_err(CE_WARN, "osnsr_tx_packet: Oversize request [%d]",
			totsize);
		rdata->n_discards++;
		freemsg(mp);
		return;
	}
	/*
	 * Set the completion urgent flag for NFS packets.
	 * We have divine knowledge that NFS packets (and a few others)
	 * will always meet this condition.
	 */
	UDI_MCB(gcb, udi_nic_tx_cb_t)->completion_urgent = FALSE;
	if (mp->b_cont)
		if (mp->b_cont->b_cont)
			if (mp->b_cont->b_cont->b_datap)
				if (mp->b_cont->b_cont->b_datap->db_frtnp)
					UDI_MCB(gcb,
						udi_nic_tx_cb_t)->
						completion_urgent = TRUE;

#if NSR_OLD_SLOW_COPY

	/*
	 * Allocate sufficient space to hold the whole message. This will be
	 * filled in by the _osnsr_txbuf_alloc code
	 */

	totsize += rdata->trailer_len;
	UDI_BUF_ALLOC(_osnsr_txbuf_alloc, gcb, NULL, totsize, rdata->buf_path);
#else
	/*
	 * Allocate an external buffer to reference the mp->b_rptr data.
	 * We need to extend this buffer if the mblk_t list is chained.
	 */
	totsize = (udi_size_t)(mp->b_wptr - mp->b_rptr);
	_udi_buf_extern_init(_osnsr_txbuf_alloc, gcb, mp->b_rptr,
			     totsize, &nsrmap_xbops, mp, rdata->buf_path);
#endif /* NSR_OLD_SLOW_COPY */
}


#if NSR_OLD_SLOW_COPY

/*
 * _osnsr_txbuf_alloc:
 * ------------------
 * Callback for transmit buffer allocation. The data has not been put in the
 * buffer. We need to copy the data into the buffer from the current message
 * position until all the data has been processed.
 * Then call the NSR to transmit it.
 *
 * Calling Context:
 *	NSR 
 */
STATIC void
_osnsr_txbuf_alloc(udi_cb_t *gcb,
		   udi_buf_t *new_buf)
{
	nsrmap_Tx_region_data_t *rdata = gcb->context;
	nsr_cb_t *pcb;
	udi_nic_tx_cb_t *txcb;
	mblk_t *mp, *orig_mp;
	udi_size_t offset, new_offset;
	udi_size_t msgsize;

	NSR_GET_SCRATCH_1(gcb->scratch, offset);
	NSR_GET_SCRATCH_2(gcb->scratch, mp);

	udi_assert(new_buf != NULL);

	if (mp == NULL) {
		/*
		 * All data has been copied into the buffer, now we can free
		 * the originating streams message and send the new data
		 * downstream
		 */

		/*
		 * TODO: Handle resource starvation correctly 
		 */

		NSR_GET_SCRATCH_3(gcb->scratch, orig_mp);
		freemsg(orig_mp);

		NSR_SET_SCRATCH_1(gcb->scratch, 0);
		NSR_SET_SCRATCH_2(gcb->scratch, 0);
		NSR_SET_SCRATCH_3(gcb->scratch, 0);

		rdata->n_packets++;
		NSR_GET_SCRATCH(gcb->scratch, pcb);

		/*
		 * Update pcb->cbp as gcb may have been reallocated 
		 */
		pcb->cbp = gcb;

		txcb = UDI_MCB(gcb, udi_nic_tx_cb_t);

		txcb->tx_buf = new_buf;

		NSR_UDI_ENQUEUE_TAIL(&rdata->onq, (udi_queue_t *)pcb);

		udi_nd_tx_req(txcb);
	} else {
		/*
		 * Copy this <mp>s data bytes into our buffer
		 */
		msgsize = (udi_size_t)(mp->b_wptr - mp->b_rptr);
		new_offset = offset + msgsize;
		NSR_SET_SCRATCH_1(gcb->scratch, (void *)new_offset);
		NSR_SET_SCRATCH_2(gcb->scratch, mp->b_cont);

		udi_buf_write(_osnsr_txbuf_alloc, gcb, mp->b_rptr, msgsize,
			      new_buf, offset, msgsize, UDI_NULL_BUF_PATH);
	}
}
#else

/*
 * _osnsr_txbuf_alloc:
 * ------------------
 * Callback for transmit buffer allocation. The data has been referenced by
 * the buffer. We need to extend the <new_buf> to handle chained messages.
 * Once we've exhausted the chain we can issue the udi_nd_tx_req(). The original
 * message will be returned to the system pool when the UDI buffer is freed.
 *
 * Calling Context:
 *	NSR 
 */
STATIC void
_osnsr_txbuf_alloc(udi_cb_t *gcb,
		   udi_buf_t *new_buf)
{
	nsrmap_Tx_region_data_t *rdata = gcb->context;
	nsr_cb_t *pcb;
	udi_nic_tx_cb_t *txcb;
	mblk_t *mp, *p_mp;
	udi_size_t msgsize;

	NSR_GET_SCRATCH_2(gcb->scratch, mp);

	udi_assert(new_buf != NULL);

	/*
	 * Issue _udi_buf_extern_add() calls to extend the external buffer
	 * NOTE: This will block.
	 */
	for (mp = (p_mp = mp)->b_cont; mp; mp = (p_mp = mp)->b_cont) {
		p_mp->b_cont = NULL;
		msgsize = (udi_size_t)(mp->b_wptr - mp->b_rptr);
		_udi_buf_extern_add(new_buf, mp->b_rptr, msgsize,
				    &nsrmap_xbops, mp);
	}

	/*
	 * External buffer allocation/extension complete. We now issue the
	 * request to the ND. When the buffer is udi_buf_free'd, we return
	 * the original message to the system [see nsrmap_xbops]
	 */
	NSR_SET_SCRATCH_1(gcb->scratch, 0);
	NSR_SET_SCRATCH_2(gcb->scratch, 0);
	NSR_SET_SCRATCH_3(gcb->scratch, 0);

	rdata->n_packets++;
	NSR_GET_SCRATCH(gcb->scratch, pcb);

	/*
	 * Update pcb->cbp as gcb may have been reallocated 
	 */
	pcb->cbp = gcb;

	txcb = UDI_MCB(gcb, udi_nic_tx_cb_t);

	txcb->tx_buf = new_buf;
	NSR_UDI_ENQUEUE_TAIL(&rdata->onq, (udi_queue_t *)pcb);

	udi_nd_tx_req(txcb);
}
#endif /* NSR_OLD_SLOW_COPY */

/*
 * osnsr_bind_done:
 * ---------------
 * Called when the mapper <-> driver bind has been completed. At this point
 * the driver should become available for OS use. To do this we need to make
 * its entry-points refer to ours [the mapper's].
 * We do this by constructing a drvinfo_t structure based on intimate knowledge
 * of the driver name (from its static properties), and save this into our
 * OS-specific region of the nsrmap_region_data_t. We destroy the association
 * when we are explicitly UNBOUND (from the devmgmt_req code)
 */
STATIC void
osnsr_bind_done(udi_channel_event_cb_t *cb,
		udi_status_t status)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;
	_udi_channel_t *ch = (_udi_channel_t *)UDI_GCB(cb)->channel;
	const char *modname;
	drvinfo_t *mydrvinfo;
	nsrmap_mod_t *modp;
	rm_key_t nd_key;

	/*
	 * Obtain the module name of the driver at the other end of the bind 
	 */
	modname =
		(const char *)ch->other_end->chan_region->reg_driver->drv_name;

	/*
	 * Obtain the ND resmgr key from the other end of the channel 
	 */
	nd_key = ch->other_end->chan_region->reg_node->node_osinfo.rm_key;

	if (nd_key == 0) {
		udi_sbit32_t i;

		/*
		 * Try to find a rm_key for the driver.  If this network
		 * driver was a child of the bridge mapper, the key
		 * would already be set.  Otherwise, we find the rm_key
		 * (required by netcfg, so it should exist if we are here).
		 * Note that a remsgr key will need to be added manually
		 * for pseudo NIC devices (ala "addcmos").
		 *
		 * We attempt to support multiple instances here.
		 * For single-instance support just hard-code:
		 *	nd_key = _Compat_cm_getbrdkey(modname, 0);
		 *	ch->other_end->chan_region->reg_node->
		 *		node_osinfo.rm_key = nd_key;
		 * A "better way" would be appreciated (such as dynamically
		 * creating rm_key entries for pseudo devices).
		 */

		i = _udi_MA_dev_to_instance(modname,
			ch->other_end->chan_region->reg_node);
		if (i >= 0)
			nd_key = _Compat_cm_getbrdkey(modname, i);

		/* Update the rm_key so we have it later */
		ch->other_end->chan_region->reg_node->node_osinfo.rm_key =
			nd_key;
	}

	if (status == UDI_OK) {
		/*
		 * Construct a drvinfo_t for this driver 
		 */
		mydrvinfo = nsrmap_attach(modname);

		if (mydrvinfo == NULL) {
			/*
			 * We failed, so fail the bind 
			 */
			udi_channel_event_complete(cb, UDI_STAT_CANNOT_BIND);
		} else {
			/*
			 * Add this current modname to our internal list of
			 * bound drivers and increment the count
			 */
			rdata->osdep_ptr->drvr_key = nd_key;

			if (modp = nsrmap_find_modname(modname)) {
				modp->nrefs++;
				(void)nsrmap_detach(mydrvinfo);
				udi_channel_event_complete(cb, UDI_OK);
			} else {
				modp = _OSDEP_MEM_ALLOC(sizeof (nsrmap_mod_t),
							0, UDI_WAITOK);
				modp->modname = modname;
				modp->nrefs = 1;
				modp->drvinfop = (void *)mydrvinfo;
				UDI_ENQUEUE_TAIL(&nsrmap_mod_Q,
						 (udi_queue_t *)modp);

				/*
				 * Initiate the CFG_ADD sequence... 
				 */
				(void)drv_attach(mydrvinfo);

				udi_channel_event_complete(cb, UDI_OK);
			}
		}
	} else {
		udi_channel_event_complete(cb, status);
	}
}

/*
 * nsrmap_find_modname:
 * -------------------
 * Scan the list of already bound modules for <modname>. If found return a
 * reference to the entry. Otherwise return NULL
 */
STATIC nsrmap_mod_t *
nsrmap_find_modname(const char *modname)
{
	udi_queue_t *elem, *tmp;
	nsrmap_mod_t *modp;

	UDI_QUEUE_FOREACH(&nsrmap_mod_Q, elem, tmp) {
		modp = (nsrmap_mod_t *) elem;
		if (udi_strcmp(modname, modp->modname) == 0) {
			return modp;
		}
	}
	return NULL;
}

/*
 * osnsr_unbind_done:
 * -----------------
 * Called on completion of udi_nd_unbind_req before the common code responds
 * to the UDI_DMGMT_UNBIND request. We need to remove the OS mapping if this
 * is the last reference to the target driver.
 */
STATIC void
osnsr_unbind_done(udi_nic_cb_t *cb)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;
	_udi_channel_t *ch = (_udi_channel_t *)UDI_GCB(cb)->channel;
	const char *modname;
	drvinfo_t *mydrvinfo;
	nsrmap_mod_t *modp;

	/*
	 * Obtain the module name of the driver at the other end of the bind 
	 */
	modname =
		(const char *)ch->other_end->chan_region->reg_driver->drv_name;

	modp = nsrmap_find_modname(modname);

	if (modp) {
		modp->nrefs--;

		if (modp->nrefs == 0) {
			/*
			 * Last driver instance, detach from OS
			 */
			UDI_QUEUE_REMOVE((udi_queue_t *)modp);

			cmn_err(CE_WARN,
				"!osnsr_unbind_done: detaching %s\n", modname);

			(void)drv_detach((drvinfo_t *) modp->drvinfop);
			(void)nsrmap_detach(modp->drvinfop);

			_OSDEP_MEM_FREE(modp);
		}
	}

	return;
}

/*
 * osnsr_ctrl_ack:
 * --------------
 * Called on completion of an earlier NSR_ctrl_req(). This needs to complete
 * the pending request and return control to the user application.
 */
STATIC void
osnsr_ctrl_ack(udi_nic_ctrl_cb_t *cb,
	       udi_status_t status)
{
	nsrmap_region_data_t *rdata = UDI_GCB(cb)->context;
	netmap_osdep_t *osdp = rdata->osdep_ptr;
	nsr_macaddr_t *macaddrp;
	udi_size_t xfer_sz;

	osdp->ioc_copyout = FALSE;
	osdp->ioc_retval = status;

	osdp->ioc_cb = UDI_GCB(cb);

	if (status != UDI_OK) {
		switch (cb->command) {
		case UDI_NIC_ADD_MULTI:
			osdp->nsr_should_filter_mcas =
				(osdp->MCA_list.n_entries - 1) >
				rdata->link_info.max_perfect_multicast;
			break;
		case UDI_NIC_DEL_MULTI:
			osdp->nsr_should_filter_mcas =
				(osdp->MCA_list.n_entries + 1) >
				rdata->link_info.max_perfect_multicast;
			break;
		}
		/*
		 * Failure 
		 */
		_OSDEP_EVENT_SIGNAL(&osdp->ioc_sv);
		return;
	}

	switch (cb->command) {
	case UDI_NIC_ADD_MULTI:
	case UDI_NIC_DEL_MULTI:
	case UDI_NIC_ALLMULTI_ON:
	case UDI_NIC_ALLMULTI_OFF:
	case UDI_NIC_SET_CURR_MAC:
	case UDI_NIC_HW_RESET:
	case UDI_NIC_BAD_RXPKT:
		/*
		 * No data to transfer to user 
		 */
		break;
	case UDI_NIC_PROMISC_ON:
		BSET(rdata->NSR_state, NsrmapS_Promiscuous);
		break;
	case UDI_NIC_PROMISC_OFF:
		BCLR(rdata->NSR_state, NsrmapS_Promiscuous);
		break;
	case UDI_NIC_GET_CURR_MAC:
	case UDI_NIC_GET_FACT_MAC:
		/*
		 * Return a MAC address to the user-supplied mblk_t 
		 */
		macaddrp = osdp->ioc_cmdp;

		/*
		 * Write the buffer data to the pre-allocated mblk_t 
		 */
		xfer_sz = MY_MIN(cb->indicator, macaddrp->ioc_len);
		udi_buf_read(cb->data_buf, 0, xfer_sz, osdp->ioc_datap);

		osdp->ioc_copyout = TRUE;
		break;
	default:
		udi_assert(0);
		/*
		 * NOTREACHED 
		 */
	}

	/*
	 * Awaken blocked ioctl. This completes the transfer of the data to the
	 * user process
	 */
	_OSDEP_EVENT_SIGNAL(&osdp->ioc_sv);
	return;

}

/*
 * =============================================================================
 * Mblk to UDI Buffer conversion routines
 * =============================================================================
 */

typedef struct {
	void (*cbfn) (udi_cb_t *);
	udi_cb_t *cb;
	udi_size_t totsize;
	udi_boolean_t do_callback;
} _udi_buf_free_rtn_t;

typedef struct {
	_udi_buf_free_rtn_t *freep;
	udi_size_t mb_size;
} _udi_buf_free_arg_t;

typedef struct {
	frtn_t frtn;
	_udi_buf_free_arg_t farg;
} _udi_buf_frtn_t;

/*
 * udi_rxcb_to_mblk:
 * ----------------
 * Convert a given UDI rxcb to an mblk chain. No data copies are performed
 * and the buffer must not be freed until the freeb() occurs higher up the
 * stack. We traverse the whole linked list [via rxcb->chain] and construct
 * a corresponding linked list of mblk's one per buffer segment. Each message
 * is linked via the b_next/b_prev field.
 * NOTE: We (silently) truncate oversize data messages to max_pdu_size bytes.
 *	 If we don't do this the transmit path will drop the packets when they
 *	 reappear after the stack has processed them.
 */
mblk_t *
udi_rxcb_to_mblk(udi_nic_rx_cb_t *rxcb,	/* Received CB containing data */

		 void (*cbfn) (udi_cb_t *),	/* Completion function */

		 udi_cb_t *cb)
{					/* Completion argument */
	udi_buf_t *bp;			/* Rx Buffer */
	mblk_t *m, *top = 0, *msg_hd = 0, *next;
	mblk_t **mp = &top;
	udi_size_t totsize = 0;		/* Complete Rx buffer size */
	udi_size_t amount = 0;		/* buffer size */
	_udi_buf_t *src_buf;		/* Internal buffer access */
	_udi_buf_dataseg_t *segment;
	udi_size_t tocopy, seglen;
	_udi_buf_free_rtn_t *free_ptr;
	_udi_buf_frtn_t *free_rtn;	/* Arg for esballoc */
	udi_nic_rx_cb_t *tcb;
	nsrmap_region_data_t *rdata = UDI_GCB(rxcb)->context;

	/*
	 * Determine size of whole (linked) rxcb chain
	 */
	for (tcb = rxcb; tcb; tcb = tcb->chain) {
		if (tcb->rx_status != UDI_OK) {
			continue;
		} else if (tcb->rx_buf == NULL) {
			return NULL;
		}
		totsize += tcb->rx_buf->buf_size;
	}
	if (totsize == 0)
		return NULL;

	/*
	 * Adjust total size to avoid copying extraneous trailer data up the
	 * stack. The maximum size of _valid_ data is max_pdu_size, so any
	 * data which is present is just bogus (e.g. pad + FCS for CSMA/CD)
	 */
	if (totsize > rdata->link_info.max_pdu_size) {
		totsize = rdata->link_info.max_pdu_size;
	}

	/*
	 * Allocate a call-back freeing routine. The specified (cbfn) will be
	 * executed when totsize reaches zero (i.e. all associated mblks have
	 * been freeb'd
	 */
	free_ptr = kmem_alloc(sizeof (*free_ptr), KM_NOSLEEP);
	if (free_ptr == NULL)
		return NULL;

	free_ptr->cbfn = cbfn;
	free_ptr->cb = cb;
	free_ptr->totsize = totsize;
	free_ptr->do_callback = FALSE;

	for (tcb = rxcb; tcb; tcb = tcb->chain) {
		udi_boolean_t new_msg = TRUE;
		mblk_t *new_mp;

		amount = MY_MIN(tcb->rx_buf->buf_size, totsize);
		bp = tcb->rx_buf;
		src_buf = (_udi_buf_t *)bp;

		/*
		 * Don't pass up any packets which have a failure status.
		 * TODO: make this configurable
		 */
		if (tcb->rx_status != UDI_OK) {
			netmap_osdep_t *osdp = rdata->osdep_ptr;
			udi_boolean_t cksum, underrun, overrun, dribble,
				frame, mac_err, other;
			cksum = tcb->rx_status & UDI_NIC_RX_BADCKSUM;
			underrun = tcb->rx_status & UDI_NIC_RX_UNDERRUN;
			overrun = tcb->rx_status & UDI_NIC_RX_OVERRUN;
			dribble = tcb->rx_status & UDI_NIC_RX_DRIBBLE;
			frame = tcb->rx_status & UDI_NIC_RX_FRAME_ERR;
			mac_err = tcb->rx_status & UDI_NIC_RX_MAC_ERR;
			other = tcb->rx_status & UDI_NIC_RX_OTHER_ERR;

			switch (rdata->link_info.media_type) {
			case UDI_NIC_ETHER:
			case UDI_NIC_FASTETHER:
			case UDI_NIC_GIGETHER:
				{
					mac_stats_eth_t *es = &osdp->s_u.e;

					(cksum ? es->mac_badsum++ : 0);
					(underrun ? es->mac_baddma++ : 0);
					(overrun ? es->mac_badlen++ : 0);
					(dribble ? es->mac_align++ : 0);
					(frame ? es->mac_no_resource++ : 0);
					(mac_err ? es->mac_no_resource++ : 0);
					(other ? es->mac_no_resource++ : 0);
				}
				break;
			case UDI_NIC_TOKEN:
				{
					mac_stats_tr_t *ts = &osdp->s_u.t;

					(cksum ? ts->mac_lineerrors++ : 0);
					(underrun ? ts->mac_baddma++ : 0);
					(overrun ? ts->mac_baddma++ : 0);
					(dribble ? ts->mac_lineerrors++ : 0);
					(frame ? ts->mac_harderrors++ : 0);
					(mac_err ? ts->mac_harderrors++ : 0);
					(other ? ts->mac_softerrors++ : 0);
				}
				break;
			case UDI_NIC_FDDI:
				{
					mac_stats_fddi_t *fs = &osdp->s_u.f;

					fs->mac_error_cts++;
				}
				break;
			default:
				break;
			}
			rdata->n_discards++;
			continue;
		} else {
			segment =
				(_udi_buf_dataseg_t *)(src_buf->buf_contents);
			tocopy = amount;
		}

		while (tocopy && segment) {

			/*
			 * Determine if this segment has any data, or if we
			 * need to skip it.
			 */
			seglen = (segment->bd_end - segment->bd_start);
			if (seglen > tocopy) {
				seglen = tocopy;
			}

			if (seglen == 0) {
				segment = segment->bd_next;
				continue;
			}

			/*
			 * Allocate a _udi_buf_frtn_t for this message block. 
			 * This has a reference to the original free_ptr which 
			 * allows multiple linked mblk_t's to call back the 
			 * completion function once all of them have been freed.
			 * We allocate one esballoc'd mblk per buffer segment.
			 */

			free_rtn = kmem_alloc(sizeof (*free_rtn), KM_NOSLEEP);

			if (free_rtn == NULL) {
				for (msg_hd = top; msg_hd; msg_hd = next) {
					next = msg_hd->b_next;
					top = msg_hd;
					while (top) {
						m = top->b_cont;
						freeb(top);
						top = m;
					}
				}
				kmem_free(free_ptr, sizeof (*free_ptr));
				return NULL;
			}

			/*
			 * Allocate a message block to reference the supplied 
			 * buffer data.
			 */
			free_rtn->farg.freep = free_ptr;
			free_rtn->farg.mb_size = seglen;
			free_rtn->frtn.free_func = l_udi_delay_callback;
			free_rtn->frtn.free_arg = (char *)&free_rtn->farg;

			m = esballoc((uchar_t *) segment->bd_start,
				     (int)seglen, BPRI_LO, &free_rtn->frtn);

			if (m == NULL) {
				while (top) {
					m = top->b_cont;
					freeb(top);
					top = m;
				}
				kmem_free(free_ptr, sizeof (*free_ptr));
				goto bail_out;
			} else {
				m->b_wptr = m->b_rptr + seglen;
				/*
				 * Sleaze alert: forcibly break any CPU binding
				 * for the strdaemon free code
				 */
				m->b_datap->db_cpu = 0;
			}

			/*
			 * Save a reference to the start of this data message 
			 */
			if (new_msg) {
				new_mp = m;
				new_msg = FALSE;
			}

			*mp = m;
			mp = &m->b_cont;
			tocopy -= seglen;
			segment = segment->bd_next;
		}

		/*
		 * Handle case where we've exhausted the actual data contents
		 * of the buffer [valid data], but have not reached the totsize
		 * count. This occurs if the ND doesn't pass full-size packets
		 * up to us.
		 */
		if (tocopy > 0) {
			free_ptr->totsize -= tocopy;
		}

		/*
		 * Finished with this message, move mp to the start of the next
		 * message location [&new_mp->b_next]. This creates a singly
		 * linked list. We traverse this when all of the rxcb chain
		 * has been processed and fill the reverse links in then.
		 */
		mp = &new_mp->b_next;
	}
	/*
	 * Create the back-link [b_prev] chain 
	 */
	for (m = top; m->b_next; m = m->b_next) {
		m->b_next->b_prev = m;
	}
      bail_out:
	if (top) {
		/*
		 * Allow callback to be executed when freeb'ing the message 
		 */
		free_ptr->do_callback = TRUE;
	}
	return top;
}

/*
 * l_udi_delay_callback:
 * -------------------
 * Called as part of the final freeb process. This routine is responsible for
 * calling the cbfn (referenced via arg->freep in arg) when the total memory 
 * size reaches zero and do_callback is set.
 * Note: This routine is run from the strdaemon context and is _outside_ the
 * normal NSR execution context. See l_osnsr_return_packet for more details.
 */
STATIC void
l_udi_delay_callback(void *arg)
{					/* _udi_buf_free_arg_t argument             */
	_udi_buf_free_arg_t *free_arg = (_udi_buf_free_arg_t *) arg;
	_udi_buf_free_rtn_t *free_ptr = free_arg->freep;
	_udi_buf_frtn_t *frtnp;
	udi_size_t seg_sz;

	frtnp = UDI_BASE_STRUCT(arg, _udi_buf_frtn_t, farg);

	free_ptr->totsize -= free_arg->mb_size;

	if (free_ptr->totsize == 0 && free_ptr->do_callback) {
		(*free_ptr->cbfn) (free_ptr->cb);
	}
	/*
	 * Release free_ptr when we've freed all of the linked mblks 
	 */
	if (free_ptr->totsize == 0) {
		kmem_free(free_ptr, sizeof (*free_ptr));
	}

	/*
	 * Release frtnp allocated memory 
	 */
	kmem_free(frtnp, sizeof (*frtnp));
}

/*
 * Common Code:
 */
#include "niccommon/netmap.c"
