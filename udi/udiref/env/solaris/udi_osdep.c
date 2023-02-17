
/*
 * env/solaris/udi_osdep.c
 *
 * Environment routines which are OS-Dependent.
 *
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

#include "udi_osdep.h"
#include <udi_env.h>

int _UDI_debug_level = 1;

_OSDEP_EVENT_T _udi_alloc_event;

static void _udi_alloc_daemon(void);
static int _udi_kill_daemon;
static _OSDEP_EVENT_T _udi_dead_daemon;

/*
 * Queue of attached dev_info_t's available for use by the bridge mapper
 * Access to this queue must be controlled via the _udi_dev_info_mutex
 */
kmutex_t	_udi_dev_info_mutex;
udi_queue_t	_udi_dev_info_Q;

typedef struct _udi_dev_info {
	void		*dip;		/* Device Info reference */
	udi_queue_t	Q;		/* Queue element	 */
} _udi_dev_info_t;

/* 
 * Support for region daemon.
 */
static void _udi_region_daemon(void);
_OSDEP_MUTEX_T _udi_region_q_lock;
_OSDEP_EVENT_T _udi_region_q_event;
udi_queue_t _udi_region_q_head;

/* Set to '-1' for all available tests to be turned on. */
int _udi_force_fail = 0;

#if INSTRUMENT_MALLOC
static kmutex_t _udi_allocation_log_lock;
static udi_queue_t _udi_allocation_log;
static void _udi_dump_mempool(void);
#endif

_OSDEP_EVENT_T udi_ev;
static void
udi_unload_complete(void)
{
	_OSDEP_EVENT_SIGNAL(&udi_ev);
}

/*
 * _udi_init is called by the system upon driver load of a Solaris DDI module.
 * This is the main entry point of the driver and therefore the environment.
 */
int
_udi_init(void)
{
	int i;
	kthread_t	*allocd_thread, *regiond_thread;

	_OSDEP_EVENT_INIT(&udi_ev);
	_OSDEP_EVENT_INIT(&_udi_alloc_event);
	_OSDEP_EVENT_INIT(&_udi_dead_daemon);

#if INSTRUMENT_MALLOC
	mutex_init(&_udi_allocation_log_lock, NULL, MUTEX_ADAPTIVE, NULL);
	UDI_QUEUE_INIT(&_udi_allocation_log);
#endif
	mutex_init(&_udi_dev_info_mutex, NULL, MUTEX_ADAPTIVE, NULL);
	UDI_QUEUE_INIT(&_udi_dev_info_Q);
	_udi_alloc_init();
	_udi_MA_init();

	/*
	 * Spawn alloc daemon 
	 */
	allocd_thread = thread_create(NULL, PAGESIZE, _udi_alloc_daemon, 0, 0, 
				&p0, TS_RUN, minclsyspri);
	if (allocd_thread == NULL) {
		cmn_err(CE_PANIC, "Failed to create UDI alloc daemon");
		/*
		 * NOTREACHED 
		 */
	}
	if (_UDI_debug_level) {
		udi_debug_printf("_udi_init: alloc daemon @ %p", allocd_thread);
	}

	/*
	 * And the region daemon 
	 */
	UDI_QUEUE_INIT(&_udi_region_q_head);
	_OSDEP_MUTEX_INIT(&_udi_region_q_lock, "udi_rgn_q");
	_OSDEP_EVENT_INIT(&_udi_region_q_event);

	regiond_thread = thread_create(NULL, PAGESIZE, _udi_region_daemon, 0, 0,
				&p0, TS_RUN, minclsyspri);
	if (regiond_thread == NULL) {
		cmn_err(CE_PANIC, "Failed to create UDI region daemon");
		/*
		 * NOTREACHED 
		 */
	}
	if (_UDI_debug_level) {
		udi_debug_printf("_udi_init: region daemon @ %p", 
				 regiond_thread);
	}
	return 0;			/* Success */
}

/*
 * _unload is called by the system upon driver unload of a UnixWare DDI8 module.
 * This is entry point responsible for teardown of the core environment.  
 * Mappers, drivers, and metas provide their own _unload().
 */
int
_udi_fini(void)
{
	_udi_MA_deinit(udi_unload_complete);
	_OSDEP_EVENT_WAIT(&udi_ev);
	_udi_kill_daemon = 1;
	_OSDEP_EVENT_SIGNAL(&_udi_alloc_event);
	_OSDEP_EVENT_WAIT(&_udi_dead_daemon);

#if INSTRUMENT_MALLOC
	_udi_dump_mempool();
	mutex_destroy(&_udi_allocation_log_lock);
#endif
	mutex_destroy(&_udi_dev_info_mutex);

	return 0;
}

static void
_udi_alloc_daemon(void)
{
	for (;;) {
		if (!_udi_alloc_daemon_work()) {
			_OSDEP_EVENT_WAIT(&_udi_alloc_event);
		} else {
			_OSDEP_EVENT_CLEAR(&_udi_alloc_event);
		}
		if (_udi_kill_daemon) {
			_OSDEP_EVENT_SIGNAL(&_udi_dead_daemon);
			thread_exit();
			/*
			 * NOTREACHED 
			 */
		}
	}
}

int regiond;

static void
_udi_region_daemon(void)
{
	for (;;) {
		udi_queue_t *elem;
		_udi_region_t *rgn;

		/*
		 * Note that we have to grab and release the mutex
		 * on the deferred region queue while in the loop becuase
		 * the region we're about to run may need to queue something
		 * onto the very region we're running.
		 *
		 * A potential optimization is to reduce the number of
		 * lock roundtrips in this exercise...
		 */
		_OSDEP_MUTEX_LOCK(&_udi_region_q_lock);
		while (!UDI_QUEUE_EMPTY(&_udi_region_q_head)) {
			elem = UDI_DEQUEUE_HEAD(&_udi_region_q_head);
			_OSDEP_MUTEX_UNLOCK(&_udi_region_q_lock);
			rgn = UDI_BASE_STRUCT(elem, _udi_region_t,
					      reg_sched_link);

			rgn->in_sched_q = FALSE;
			_udi_run_inactive_region(rgn);
			_OSDEP_MUTEX_LOCK(&_udi_region_q_lock);
		}
		_OSDEP_MUTEX_UNLOCK(&_udi_region_q_lock);

		if (_udi_kill_daemon) {
			_OSDEP_EVENT_SIGNAL(&_udi_dead_daemon);
			thread_exit();
			/*
			 * NOTREACHED 
			 */
		}

		_OSDEP_EVENT_WAIT(&_udi_region_q_event);
regiond++;
	}
}

udi_status_t
_udi_register_isr(_OSDEP_BRIDGE_CHILD_DATA_T * rm_key,
		  int (*isr) (), void *context, _udi_intr_info_t * osinfo)
{
#if 0 || UNIXWARE
	struct rm_args rma;
	cm_num_t irq;
	boolean_t was_empty;

	rma.rm_key = *rm_key;
	if (rma.rm_key == RM_NULL_KEY)
		return UDI_STAT_MISTAKEN_IDENTITY;

	rma.rm_n = 0;
	strcpy(rma.rm_param, CM_IRQ);
	rma.rm_val = &irq;
	rma.rm_vallen = sizeof irq;
	if (_rm_getval(&rma, UIO_SYSSPACE) != 0 || rma.rm_vallen != sizeof irq)
		return UDI_STAT_MISTAKEN_IDENTITY;

	osinfo->irq = irq;

	/*
	 * Arrange for ISR to be called with our osinfo ptr 
	 * UnixWare kernel handles shared interrupts and has a 
	 * convenient calling convention so we take advantage of that.
	 */
	if (!cm_intr_attach(rma.rm_key, isr, context, &_udi_drvinfo,
		       &osinfo->cookie)) {
		/* cm_intr_attach failed */
		return UDI_STAT_MISTAKEN_IDENTITY;
	}

#endif	/* 0 || UNIXWARE */
	return UDI_OK;
}

udi_status_t
_udi_unregister_isr(_OSDEP_BRIDGE_CHILD_DATA_T * rm_key,
		    int (*isr) (), void *context, _udi_intr_info_t * osinfo)
{
#if 0 || UNIXWARE
	cm_intr_detach(osinfo->cookie);
#endif	/* 0 || UNIXWARE */
	return UDI_OK;
}

#ifdef NOTYET
udi_ubit32_t
_osdep_get_card_id(_udi_dev_node_t *node)
{
	cm_args_t cm;
	char brdid[12];
	udi_ubit32_t brdid_n;
	int err;

	cm.cm_key = node->node_osinfo.key;
	cm_begin_trans(cm.cm_key, RM_READ);
	cm.cm_param = CM_BRDID;
	cm.cm_val = &brdid;
	cm.cm_vallen = sizeof brdid;
	cm.cm_n = 0;
	err = cm_getval(&cm);
	cm_end_trans(cm.cm_key);
	_OSDEP_ASSERT(err == 0);

	brdid_n = (udi_ubit32_t)strtoul(brdid, NULL, 16);

	return ((brdid_n & 0xFFFF) << 16) + (brdid_n >> 16);
}
#endif /* NOTYET */

/*
 * Glue routines for UDI drivers.
 * Invoked by loadable-module _load and _unload routines provided in the 
 * drivers, mappers, and metas.
 */

int
_udi_driver_load(const char *modname,
		 udi_init_t * init_info,
		 _udi_sprops_idx_t * sprops)
{
	_udi_driver_t *drv;
	udi_status_t r;
	_OSDEP_DRIVER_T default_osdep_driver;
	int retval;

	default_osdep_driver.sprops = sprops;

	drv = _udi_MA_driver_load(modname, init_info, default_osdep_driver);

	r = _udi_MA_install_driver(drv);

	return (r == UDI_OK ? 0 : EIO);
}

int
_udi_driver_unload(const char *modname)
{
	_udi_driver_t	*drv;

	drv = _udi_driver_lookup_by_name(modname);
	if (drv == NULL) {
		udi_debug_printf("_udi_driver_unload: %s not found\n");
		return ENODEV;
	}

	_udi_MA_destroy_driver_by_name(modname);

	return 0;
}

/*
 * _udi_drv_attach(dip, cmd)
 * ---------------
 * Called to attach or resume operations for a new driver instance. This routine
 * must obtain the PCI configuration information from the <dip> and construct
 * an enumeration CB to make the system aware of the new device. The <dip> is
 * then added to _udi_dev_info_Q so that the bridge mapper can gain access to
 * the device and make it publicly available
 */
int
_udi_drv_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	int		rval = DDI_SUCCESS;
	_udi_dev_info_t	*devel;

	switch (cmd) {
	case DDI_ATTACH:
		devel = kmem_zalloc(sizeof(_udi_dev_info_t), KM_SLEEP);
		UDI_QUEUE_INIT(&devel->Q);
		devel->dip = (void *)dip;
		mutex_enter(&_udi_dev_info_mutex);
		UDI_ENQUEUE_TAIL(&_udi_dev_info_Q, &devel->Q);
		mutex_exit(&_udi_dev_info_mutex);
	case DDI_RESUME:
	default:
		rval = DDI_FAILURE;
	}

	return rval;
}

/*
 * _udi_drv_detach(dip, cmd):
 * ------------------
 * Called to detach the specified instance of the driver from the system. This
 * needs to issue the appropriate devmgmt request to cause the instance to 
 * be released.
 */

int
_udi_drv_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	int		rval = DDI_SUCCESS;
	_udi_dev_info_t	*devel;
	udi_queue_t	*elem, *tmp;

	switch (cmd) {
	case DDI_DETACH:
		/*
		 * Scan the list of outstanding dips which haven't been claimed
		 * by the bridge mapper. If we find ourselves on the list we
		 * must release the memory associated with this instance. 
		 * Normally the bridge mapper matching would free this, but this
		 * may not have happened yet
		 */
		mutex_enter(&_udi_dev_info_mutex);
		UDI_QUEUE_FOREACH(&_udi_dev_info_Q, elem, tmp) {
			devel = UDI_BASE_STRUCT(elem, _udi_dev_info_t, Q);
			if (devel->dip == (void *)dip) {
				/*
				 * dip still awaiting use, so we must free it
				 */
				UDI_QUEUE_REMOVE(elem);
				kmem_free(devel, sizeof(_udi_dev_info_t));
				break;
			}
		}
		mutex_exit(&_udi_dev_info_mutex);
		break;
	case DDI_SUSPEND:
	default:
		rval = DDI_FAILURE;
	}
	return rval;
}

#if 0
int
_udi_driver_config(int func,
		   rm_key_t key,
		   char *modname)
{
	if (udi_strcmp(modname, "pseudond") == 0) {
		if (mdi_get_unit(key, NULL) == B_FALSE) {
			return ENXIO;
		}
	}
	return 0;
}
#endif

#if 0 || UNIXWARE
int
_udi_driver_config(cfg_func_t func, void *idata, rm_key_t rmkey)
{

	_udi_dev_node_t *node = idata;

	switch (func) {

	case CFG_ADD: {
		void **idatap = idata;
		cm_args_t	cma;
		char modname[RM_MAXPARAMLEN];
		_udi_driver_t *drv;
		_udi_dev_node_t *node;
		udi_queue_t *elem, *tmp, *nelem, *ntmp;

		/*
		 * Find the node and return it in idata
		 */

		/* find the driver first */
		cma.cm_key = rmkey;
		cma.cm_n = 0;
		cma.cm_param = CM_MODNAME;
		cma.cm_val = modname;
		cma.cm_vallen = sizeof(modname);
		cm_begin_trans(cma.cm_key, RM_READ);
		cm_getval(&cma);
		cm_end_trans(cma.cm_key);

		drv = _udi_driver_lookup_by_name(modname);
		if (drv != NULL) {
			/* Found the driver, find the node now */
			UDI_QUEUE_FOREACH(&drv->node_list, nelem, ntmp) {
				node = UDI_BASE_STRUCT(nelem, _udi_dev_node_t,
								drv_node_list);
				if (node->node_osinfo.rm_key == rmkey) {
					/* found the node */
					*idatap = node;
					return 0;
				}
			}
		}
		return ENODEV;
	} break;

	case CFG_SUSPEND: 

		_udi_MA_suspend_resume(node, UDI_DMGMT_SUSPEND);
		break;
		
	case CFG_RESUME:

		_udi_MA_suspend_resume(node, UDI_DMGMT_RESUME);
		break;

	case CFG_REMOVE:

		_udi_MA_unbind(node);
		break;

	case CFG_MODIFY:
		/* Nothing to be done */
		break;

	case CFG_VERIFY:
	default:
		return EOPNOTSUPP;
	}

	return 0;
}
#endif	/* 0 || UNIXWARE */

/*
 * Glue routines for UDI external mappers.
 * Invoked by loadable-module _load and _unload routines.
 */

int
_udi_mapper_load(const char *modname,
		 udi_init_t * init_info,
		 _udi_sprops_idx_t * sprops)
{
	_udi_driver_t *drv;
	udi_status_t r;
	_OSDEP_DRIVER_T default_osdep_driver;

	default_osdep_driver.sprops = sprops;

	drv = _udi_MA_driver_load(modname, init_info, default_osdep_driver);

	r = _udi_MA_install_driver(drv);

	return (r == UDI_OK ? 0 : EIO);
}

int
_udi_mapper_unload(const char *modname)
{
	_udi_MA_destroy_driver_by_name(modname);
	return 0;
}

/*
 * Glue routines for UDI metalanguage libraries.
 * Invoked by loadable-module _load and _unload routines.
 */

int
_udi_meta_load(const char *modname,
	       udi_mei_init_t * meta_info,
	       _udi_sprops_idx_t * sprops)
{
	int retval;

	_OSDEP_DRIVER_T default_osdep_driver;

	default_osdep_driver.sprops = sprops;

	if (_UDI_debug_level) {
		udi_debug_printf("_udi_meta_load(%s,%p,%p)\n", modname,
			meta_info, sprops);
	}

	if (!strcmp(modname, "udi")) {
		retval = _udi_init();
		if (retval != 0) {
			cmn_err(CE_WARN, 
				"Could not initialise udi environment");
			return retval;
		}
	}
	

	(void)_udi_MA_meta_load(modname, meta_info, default_osdep_driver);

	return 0;
}

int
_udi_meta_unload(const char *modname)
{
	_udi_MA_meta_unload(modname);
	if (!strcmp(modname, "udi")) {
		(void)_udi_fini();
	}
	return 0;
}

/* 
 * Kernel memory allocator wrappers
 */

/*
 * A header that is prepended to all allocated requests.
 * We stash the size in this header.   It could later be used to
 * store debugging info such as the address of the allocation caller.
 * TODO: This entire wrapping thing would be unnecessary if our
 *       supporting free() didn't require the size to be passed in
 *       or if it could peek into the kernel memory pool to glean
 *       the sizes.
 */
typedef struct {
#if INSTRUMENT_MALLOC
	udi_queue_t mblk_qlink;
	unsigned int allocator_linenum;
	char *allocator_fname;
#endif
	size_t size;
	ulong_t stamp;

} _mblk_header_t;

#define	_UDI_MEM_STAMP	0xDEADBEEF


void *
_udi_wrapped_alloc(size_t size,
		   int flags,
		   int sleep_ok
#if INSTRUMENT_MALLOC
		   ,
		   int line,
		   char *fname
#endif
	)
{
	_mblk_header_t *mem;		/* Actual requested block, including header */
	char *dbuf;			/* Beginning of user-visible data buf */

	if ((_OSDEP_INSTRUMENT_ALLOC_FAIL) && (sleep_ok == UDI_NOWAIT)) {
		return 0;
	}

	mem = kmem_alloc(sizeof (_mblk_header_t) + size,
			 (sleep_ok) ? KM_SLEEP : KM_NOSLEEP);

	if (!mem) {
		return 0;
	}
	dbuf = (char *)((char *)mem + sizeof (_mblk_header_t));
	mem->size = size;

	if (!(flags & UDI_MEM_NOZERO)) {
		bzero(dbuf, size);
	}

	mem->stamp = _UDI_MEM_STAMP;

#if INSTRUMENT_MALLOC
	mem->allocator_linenum = line;
	mem->allocator_fname = fname;

	_OSDEP_SIMPLE_MUTEX_LOCK(&_udi_allocation_log_lock);
	UDI_ENQUEUE_TAIL(&_udi_allocation_log, &mem->mblk_qlink);
	_OSDEP_SIMPLE_MUTEX_UNLOCK(&_udi_allocation_log_lock);
#endif

	return dbuf;
}

void
_udi_wrapped_free(void *buf)
{
	/*
	 * Walk backward in our buffer to find the header.  Use that
	 * info for kmem_free.
	 */
	_mblk_header_t *mem = (_mblk_header_t *) ((char *)buf -

						  sizeof (_mblk_header_t));
	size_t size = mem->size + sizeof (_mblk_header_t);

	if (mem->stamp != _UDI_MEM_STAMP) {
		/*
		 * We've just attempted to free a corrupted memory block.
		 * Take the cowardly approach and ignore the free request
		 * but pop the user into the kernel debugger for analysis.
		 */

		cmn_err(CE_WARN,
			"_udi_wrapped_free: Expected stamp %p, got %p\n",
			_UDI_MEM_STAMP, mem->stamp);
		_OSDEP_DEBUG_BREAK;
		return;
		/*
		 * NOTREACHED 
		 */
	}
	mem->stamp = 0xDEADBABE;

#if INSTRUMENT_MALLOC
	_OSDEP_SIMPLE_MUTEX_LOCK(&_udi_allocation_log_lock);
	udi_dequeue(&mem->mblk_qlink);
	_OSDEP_SIMPLE_MUTEX_UNLOCK(&_udi_allocation_log_lock);
#endif

	kmem_free(mem, size);
}

#if INSTRUMENT_MALLOC
static void
_udi_dump_mempool(void)
{
	udi_queue_t *listp = &_udi_allocation_log;
	udi_queue_t *e, *t;
	udi_size_t sz = 0;

	UDI_QUEUE_FOREACH(listp, e, t) {
		_mblk_header_t *mblk = (_mblk_header_t *) e;

		sz += mblk->size;
		udi_debug_printf("Allocated from %s:%d. size is %d(0x%x)/%p\n",
			     mblk->allocator_fname,
			     mblk->allocator_linenum,
			     mblk->size,
			     mblk->size,
			     (char *)mblk + sizeof (_mblk_header_t));
		if (debug_output_aborted())
			break;
	}
	udi_debug_printf("Total allocated memory: %d(0x%x) bytes\n", sz, sz);
}

#endif

/*
 * dev_info_t queue manipulation routines
 *
 * _udi_dev_info_match:
 * -------------------
 * Scan the _udi_dev_info_Q looking for a match for the given attribute list
 * If *dipp is NULL we have no existing device to check against and we have to
 * exhaustively search all the dip's on the queue looking for PCI config space
 * matches.
 *
 * Returns:
 *	TRUE	if a match was found, with *dipp set to the matching value;
 *		the _udi_dev_info_Q entry is removed
 *	FALSE	if a match was not found. No change is made to _udi_dev_info_Q
 */

#define	_UDI_DEV_COMP(a, s, v1, v2)	\
	if (!udi_strcmp(a->attr_name, s)) { \
		if (v1 != v2) { \
			continue; \
		} else { \
			nmatched++; \
		} \
	} else

udi_boolean_t
_udi_dev_info_match(void 		     **dipp,
		    udi_instance_attr_list_t *attr_list, 
		    udi_ubit8_t 	     attr_length)
{
	udi_queue_t	*elem, *tmp;
	_udi_dev_info_t	*devel;
	dev_info_t	*dip;
	ddi_acc_handle_t handle;
	udi_ubit32_t	val;
	udi_ubit16_t	pci_vend_id, pci_dev_id;
	udi_ubit8_t	pci_rev_id, pci_base_class, pci_sub_class, pci_prog_if;
	udi_ubit16_t	pci_sub_vend_id, pci_sub_id;
	udi_ubit8_t	n;
	int		rval, length;
	pci_regspec_t	*pci_rp;
	udi_ubit32_t	pci_func, pci_dev, pci_bus;
	udi_ubit32_t	pci_unit_addr;
	udi_instance_attr_list_t	*current;
	int		nmatched;


	if (*dipp == NULL) {
		/* Exhaustive search of whole queue */
		mutex_enter( &_udi_dev_info_mutex );
		UDI_QUEUE_FOREACH(&_udi_dev_info_Q, elem, tmp) {
			devel = UDI_BASE_STRUCT(elem, _udi_dev_info_t, Q);
			dip = (dev_info_t *)devel->dip;
			if (pci_config_setup(dip, &handle) != DDI_SUCCESS) {
				mutex_exit(&_udi_dev_info_mutex);
				return FALSE;
			}
			pci_vend_id = pci_config_get16(handle, PCI_CONF_VENID);
			pci_dev_id  = pci_config_get16(handle, PCI_CONF_DEVID);
			pci_rev_id  = pci_config_get8(handle, PCI_CONF_REVID);
			pci_base_class = 
				pci_config_get8(handle, PCI_CONF_BASCLASS);
			pci_sub_class =
				pci_config_get8(handle, PCI_CONF_SUBCLASS);
			pci_prog_if =
				pci_config_get8(handle, PCI_CONF_PROGCLASS);
			pci_sub_vend_id = 
				pci_config_get16(handle, PCI_CONF_SUBVENID);
			pci_sub_id =
				pci_config_get16(handle, PCI_CONF_SUBSYSID);
			pci_config_teardown(&handle);

			/*
			 * Obtain the PCI "reg" property from the node. This
			 * contains the pci_unit_address [Function+Device+Bus]
			 */
			rval = ddi_getlongprop(DDI_DEV_T_NONE, dip, 
					       DDI_PROP_CANSLEEP, "reg",
					       (caddr_t)&pci_rp, &length);
			if (rval != DDI_SUCCESS) {
				continue;
			}
			pci_func = PCI_REG_FUNC_G(pci_rp->pci_phys_hi);
			pci_dev  = PCI_REG_DEV_G(pci_rp->pci_phys_hi);
			pci_bus  = PCI_REG_BUS_G(pci_rp->pci_phys_hi);

			kmem_free(pci_rp, length);

			pci_unit_addr = pci_func|(pci_dev << 3)|(pci_bus <<8);

			/*
			 * We now have all the PCI config info for this dip,
			 * Match against the attr_list entries
			 */
			current = attr_list;
			nmatched = 0;
			for (n = 0; n < attr_length; n++, current++) {
				val=UDI_ATTR32_GET(current->attr_value);
				_UDI_DEV_COMP(current, "pci_vendor_id", val, pci_vend_id);
				_UDI_DEV_COMP(current, "pci_device_id", val, pci_dev_id);
				_UDI_DEV_COMP(current, "pci_revision_id", val, pci_rev_id);
				_UDI_DEV_COMP(current, "pci_baseclass", val, pci_base_class);
				_UDI_DEV_COMP(current, "pci_sub_class", val, pci_sub_class);
				_UDI_DEV_COMP(current, "pci_prog_if", val, pci_prog_if);
				_UDI_DEV_COMP(current, "pci_subsystem_vendor_id", val, pci_sub_vend_id);
				_UDI_DEV_COMP(current, "pci_subsystem_id", val, pci_sub_id);
				_UDI_DEV_COMP(current, "pci_unit_address", val, pci_unit_addr);
				if (nmatched == attr_length) {
					/* Successful match, we've found it */
					*dipp = devel->dip;
					UDI_QUEUE_REMOVE(elem);
					mutex_exit( &_udi_dev_info_mutex );
					kmem_free(devel, sizeof(_udi_dev_info_t));
					return TRUE;
				}
			}
		}
		mutex_exit( &_udi_dev_info_mutex );
	} else {
		/* Search for a matching dip entry */
		mutex_enter( &_udi_dev_info_mutex );
		UDI_QUEUE_FOREACH(&_udi_dev_info_Q, elem, tmp) {
			devel = UDI_BASE_STRUCT(elem, _udi_dev_info_t, Q);
			if (devel->dip == *dipp) {
				UDI_QUEUE_REMOVE(elem);
				mutex_exit( &_udi_dev_info_mutex );
				kmem_free(devel, sizeof(_udi_dev_info_t));
				return TRUE;
			}
		}
		mutex_exit( &_udi_dev_info_mutex );
	}
	return FALSE;

}

/*
 * Function to fill in udi_limit_t struct for region creation.
 */
void
_udi_get_limits_t(udi_limits_t * limits)
{
	/*
	 * These could ultimately be tied into the KMA 
	 */
	limits->max_legal_alloc = 128 * 1024;
	limits->max_safe_alloc = UDI_MIN_ALLOC_LIMIT;
	limits->max_trace_log_formatted_len = _OSDEP_TRLOG_LIMIT;
	limits->max_instance_attr_len = UDI_MIN_INSTANCE_ATTR_LIMIT;
	limits->min_curtime_res = drv_hztousec(1) * 1000;
	limits->min_timer_res = limits->min_curtime_res;
}

/*
 * String manipulation routines
 */

void *
udi_memcpy(void *s1,
	   const void *s2,
	   udi_size_t n)
{
	bcopy(s2, s1, n);
	return (s1);
}

void *
udi_memmove(void *s1,
	    const void *s2,
	    udi_size_t n)
{
	ovbcopy((void *)s2, s1, n);
	return (s1);
}

void
__udi_assert(const char *str,
	     const char *file,
	     int line)
{
	cmn_err(CE_PANIC, "Assertion ( %s ) Failed: File = %s, Line %d",
		str, file, line);
}

/*
 *  Provide environment-optimized versions of these common utilities.
 */

char *
udi_strrchr(const char *s,
	    char c)
{
#if defined(i386)
	asm("movl	%edi, %edx");	/* Save the register */
	asm("movl	8(%ebp),%edi");	/* Memory location of start */
	asm("movb	12(%ebp),%al");	/* Byte searching for */
	asm("xor	%ecx,%ecx");	/* Zero out result */
	asm(".strrchr1:");
	asm("movb	(%edi),%bl");	/* Get the byte */
	asm("cmpb	$0,%bl");	/* Is it 0? */
	asm("jne	.strrchr2");	/* No, so continue */
	asm("jmp	.strrchr4");	/* Yes, We are done */
	asm(".strrchr2:");
	asm("cmpb	%al,(%edi)");	/* Check for byte match */
	asm("jne	.strrchr3");	/* No match */
	asm("movl	%edi,%ecx");	/* Match, so save pointer */
	asm(".strrchr3:");
	asm("inc	%edi");		/* Increment pointer */
	asm("jmp	.strrchr1");	/* Check next byte */
	asm(".strrchr4:");
	asm("movl	%ecx,%eax");	/* Return the pointer */
	asm("movl	%edx,%edi");	/* Restore register */
#endif	/* defined(i386) */
}

void *
udi_memchr(const void *s,
	   udi_ubit8_t c,
	   udi_size_t n)
{
#if defined(i386)
	asm("movl	%edi, %edx");	/* Save the register */
	asm("movl	8(%ebp),%edi");	/* Memory location of start */
	asm("movl	16(%ebp),%ebx");	/* Get the count */
	asm("movb	12(%ebp),%al");	/* Byte searching for */
	asm(".memchr1:");
	asm("cmpl	$0,%ebx");	/* Are we at 0? */
	asm("jne	.memchr2");	/* No, so continue */
	asm("movl	$0,%edi");	/* Set pointer to null */
	asm("jmp	.memchr3");	/* We are done */
	asm(".memchr2:");
	asm("cmpb	%al,(%edi)");	/* Check for byte match */
	asm("jz		.memchr3");
	asm("dec	%ebx");		/* Decrement counter */
	asm("inc	%edi");		/* Increment pointer */
	asm("jmp	.memchr1");
	asm(".memchr3:");
	asm("movl	%edi,%eax");	/* Return the pointer */
	asm("movl	%edx,%edi");	/* Restore register */
#endif	/* defined(i386) */
}

#if 0
void *
udi_memset(void *s,
	   udi_ubit8_t c,
	   udi_size_t n)
{
#if defined(i386)
	asm("movl	%edi, %edx");	/* Save the register */
	asm("movl	8(%ebp),%edi");	/* Memory location to start */
	asm("movb	12(%ebp),%al");	/* The byte to store */
	asm("movb	12(%ebp),%ah");	/* Replicate it for 1st word */
	asm("shll	$16,%eax");	/* Move it to the high word */
	asm("movb	12(%ebp),%al");
	asm("movb	12(%ebp),%ah");
	asm("movl	16(%ebp),%ecx");	/* count */
	asm("shrl	$2,%ecx");	/* # of dwords to do */
	asm("rep;	stosl");	/* set dwords first */
	asm("movb	16(%ebp),%cl");	/* ECX = low-order byte count */
	asm("andb	$3, %cl");	/* # remaining bytes to set */
	asm("rep;	stosb");	/* set remaining bytes */
	asm("movl	%edx,%edi");	/* Restore register */
	asm("movl	8(%ebp),%eax");	/* Return pointer */
#endif	/* defined(i386) */
}
#endif

#if 0
udi_sbit8_t
udi_memcmp(const void *s1, const void *s2, udi_size_t n)
{
	udi_sbit8_t	*p1 = (udi_sbit8_t *)s1, *p2 = (udi_sbit8_t *)s2;
	while (n > 0 && *p1 == *p2) {
		p1++;
		p2++;
		n--;
	}
	if (n == 0) {
		return 0;
	} else {
		return (*p1 < *p2 ? -1 : +1);
	}
}
#endif

#if LATER

/*
 * Queue Management Routines
 */
void
udi_enqueue(udi_queue_t * element,
	    udi_queue_t * queue)
{
	asm("movl	8(%ebp),%ebx");	/* ebx = e */
	asm("movl	12(%ebp),%ecx");	/* ecx = q */
	asm("movl	(%ecx),%edx");	/* edx = q->flink */
	asm("movl	%edx,(%ebx)");	/* e->flink = q->flink */
	asm("movl	%ebx,(%ecx)");	/* q->flink = e */
	asm("addl	$4,%edx");	/* edx = q->flink->rlink */
	asm("movl	%ebx,(%edx)");	/* q->flink->rlink = e */
	asm("addl	$4,%ebx");	/* ebx = e->rlink */
	asm("movl	%ecx,(%ebx)");	/* e->rlink = q */
}

udi_queue_t *
udi_dequeue(udi_queue_t * element)
{
	asm("movl	8(%ebp),%edx");	/* edx = e */
	asm("movl	$4,%ecx");
	asm("addl	%edx,%ecx");
	asm("movl	(%ecx),%ecx");	/* ecx = e->rlink */
	asm("movl	(%edx),%edx");	/* edx = e->flink */
	asm("movl	%edx,(%ecx)");	/* e->rlink->flink = e->flink */
	asm("addl	$4,%edx");
	asm("movl	%ecx,(%edx)");	/* e->flink->rlink = e->rlink */
	asm("movl	8(%ebp),%eax");	/* Return element */
}
#endif /* LATER */

/*
 * Tracing and logging support.
 */
udi_boolean_t
_udi_osdep_log_data(char *fmtbuf,
		    udi_size_t size,
		    struct _udi_cb *cb)
{
	/*
	 * We set cmn_err's severity from hte log's severity.  The 
	 * levels don't map in a very natural way.
	 */

	int sev;

	switch (cb->cb_callargs.logm.logm_severity) {
	case UDI_LOG_DISASTER:
	case UDI_LOG_ERROR:
	case UDI_LOG_WARNING:
		sev = CE_WARN;
		break;
	case UDI_LOG_INFORMATION:
		sev = CE_NOTE;
	}

	cmn_err(sev, "%s", fmtbuf);

	return UDI_OK;

}

static udi_boolean_t
_udi_scsi_driver(_udi_driver_t *drv)
{
	udi_ubit32_t m_idx, child_meta_max, meta_idx;
	const char *meta_name;

	child_meta_max = _osdep_sprops_count(drv, UDI_SP_CHILD_BINDOPS);

	/*
	 * Obtain all child metas 
	 */
	for (m_idx = 0; m_idx < child_meta_max; m_idx++) {
		meta_idx = _osdep_sprops_get_bindops(drv,
						     m_idx,
						     UDI_SP_CHILD_BINDOPS,
						     UDI_SP_OP_METAIDX);
		meta_name = (char *)_osdep_sprops_get_meta_if(drv, meta_idx);

		if (udi_strcmp(meta_name, "udi_scsi") == 0)
			return TRUE;
	}
	return FALSE;
}

/*
 * Atomic int handling functions
 */
void
_osdep_atomic_int_init( _osdep_atomic_int_t *atomic_intp, int ival )
{
	rw_init(&atomic_intp->amutex, NULL, RW_DRIVER, NULL);
	atomic_intp->aval = ival;
}

void
_osdep_atomic_int_incr( _osdep_atomic_int_t *atomic_intp )
{
	rw_enter( &atomic_intp->amutex, RW_WRITER );
	atomic_intp->aval++;
	rw_exit( &atomic_intp->amutex );
}

void
_osdep_atomic_int_decr( _osdep_atomic_int_t *atomic_intp )
{
	rw_enter( &atomic_intp->amutex, RW_WRITER );
	atomic_intp->aval--;
	rw_exit( &atomic_intp->amutex );
}

int
_osdep_atomic_int_read( _osdep_atomic_int_t *atomic_intp )
{
	int	retval;

	rw_enter( &atomic_intp->amutex, RW_READER );
	retval = atomic_intp->aval;
	rw_exit( &atomic_intp->amutex );

	return retval;
}

void
_osdep_atomic_int_deinit( _osdep_atomic_int_t *atomic_intp )
{
	rw_destroy( &atomic_intp->amutex );
}

/*
 * udi_strtou32:
 */
#define	isspace(x)	\
	((x) == ' ' || (x) == '\011' || (x) == '\012' || (x) == '\015')

#define isdigit(x)	\
	((x) >= '0' && (x) <= '9')

#define	isxdigit(x)	\
	(isdigit(x) || ((x) >= 'a' && (x) <= 'f') || ((x) >= 'A' && (x) <= 'F'))

#define	islower(x)	\
	((x) >= 'a' && (x) <= 'z')

#define	DIGIT(x)	\
	(isdigit(x) ? (x) - '0' : islower(x) ? (x) + 10 - 'a' : (x) + 10 - 'A')

#define	lisalnum(x)	\
	(isdigit(x) || ((x) >= 'a' && (x) <= 'z') || ((x) >= 'A' && (x) <= 'Z'))

udi_ubit32_t
udi_strtou32(const char *s, char **endptr, int base)
{
	const char 	**ptr = (const char **)endptr;
	const udi_ubit8_t *ustr = (const udi_ubit8_t *)s;
	udi_ubit32_t	val;
	udi_sbit32_t	c, xx;
	unsigned long	multmax;
	int		neg = 0;

	if (endptr != (char **)0)
		*ptr = (char *)ustr;	/* in case no number is formed */
	if (base < 0 || base > 36 || base == 1){
		return (0);	/* base is invalid */
	}
	if (!lisalnum(c = *ustr)) {
		while (isspace(c))
			c = *++ustr;
		switch (c) {
		case '-':
			neg++;
			/* FALL THROUGH */
		case '+':
			c = *++ustr;
		}
	}
	if (base == 0) {
		if (c != '0') {
			base = 10;
		} else if (ustr[1] == 'x' || ustr[1] == 'X') {
			base = 16;
		} else {
			base = 8;
		}
	}
	
	if (!lisalnum(c) || (xx = DIGIT(c)) >= base) {
		return (0);	/* no number formed */
	}
	if (base == 16 && c == '0' && (ustr[1] == 'x' || ustr[1] == 'X') &&
	    isxdigit(ustr[2])) {
	    	c = *(ustr += 2);
	}

	multmax = ULONG_MAX / (unsigned long)base;
	val = DIGIT(c);
	for (c = *++ustr; lisalnum(c) && (xx = DIGIT(c)) < base; ) {
		if (val > multmax) {
			goto overflow;
		}
		val *= base;
		if (ULONG_MAX - val < xx) {
			goto overflow;
		}
		val += xx;
		c = *++ustr;
	}
	if (ptr != (const char **)0) {
		*ptr = (char *)ustr;
	}
	return (neg ? -val : val);

overflow:
	for (c = *++ustr; lisalnum(c) && (xx = DIGIT(c)) < base; (c = *++ustr))
		;
	if (ptr != (const char **)0) {
		*ptr = (char *)ustr;
	}
	return (ULONG_MAX);
}
