
/*
 * src/env/uw/udi_osdep.c
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

#include <udi_env.h>
#include <udi_std_sprops.h>
#include <udid.h>

#ifdef _KERNEL_HEADERS
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/conf.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <util/cmn_err.h>
#include <util/errno.h>
#include <util/mod/moddefs.h>
#include <util/param.h>
#include <util/types.h>
#include <util/moddrv.h>
#include <svc/utsname.h>
#else
#include <sys/cm_i386at.h>
#include <sys/cmn_err.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/lwp.h>
#include <sys/moddefs.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/resmgr.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/moddrv.h>
#include <sys/utsname.h>
#endif

event_t _udi_alloc_event;

static void _udi_alloc_daemon(void *arg);
static int _udi_kill_daemon;
static event_t _udi_dead_daemon;

static udi_boolean_t _udi_scsi_driver(_udi_driver_t *);
static udi_boolean_t _udi_module_is_meta(char *sprops_text, int sprops_size, char **provides);
void _udi_sync_devmgmt_req(_udi_dev_node_t *node,
			   _OSDEP_EVENT_T *event,
			   udi_status_t *status,
			   udi_ubit8_t *flags,
			   udi_ubit8_t mgmt_op,
			   udi_ubit8_t parent_ID);
void _udi_MA_unbind(_udi_dev_node_t *node,
		    _udi_generic_call_t *callback,
		    void *callbackarg);
void udid_drv_detach(void);

/*
 * The following does not conform to DDI and used here only to support
 * sysdump. Basically, sysdump calls the interrupt handler directly via
 * cm_intr_call. We want to intercept the interrupt handler to drive the
 * region daemon and alloc daemon.
 */
extern int cm_intr_call(void *intr_cookie);

/*
 * This structure is directly taken from mod_intr.c. This structure must
 * match in size with the one defined in mod_intr.c.
 */
typedef struct cm_intr_cookie {
	struct intr_info ic_intr_info;
	int (*ic_handler) ();
} cm_intr_cookie_t;

/* 
 * Support for region daemon.
 */
static void _udi_region_daemon(void *arg);
_OSDEP_MUTEX_T _udi_region_q_lock;
_OSDEP_EVENT_T _udi_region_q_event;
udi_queue_t _udi_region_q_head;

/* Set to '-1' for all available tests to be turned on. */
int _udi_force_fail = 0;

#if INSTRUMENT_MALLOC
static _OSDEP_SIMPLE_MUTEX_T _udi_allocation_log_lock;
static udi_queue_t _udi_allocation_log;
static void _udi_dump_mempool(void);
#endif

_OSDEP_EVENT_T udi_ev;
static void
udi_unload_complete(void)
{
	_OSDEP_EVENT_SIGNAL(&udi_ev);
}

static void
_udi_alloc_daemon(void *arg)
{
	_OSDEP_ASSERT(getpl() == PLBASE);

	for (;;) {
		if (!_udi_alloc_daemon_work())
			EVENT_WAIT(&_udi_alloc_event, PRIMEM);
		else
			EVENT_CLEAR(&_udi_alloc_event);
		if (_udi_kill_daemon) {
			EVENT_SIGNAL(&_udi_dead_daemon, PRIMED);
			kthread_exit();
			/*
			 * NOTREACHED 
			 */
		}
	}
}


/*
 * NOTE: This function should be moved into common code (just like
 *       _udi_alloc_daemon_work().
 * A helper function to be called from the region daemon (which is
 * so os-dependent as to be  unrepresentable here) to do one piece of
 * work that may be queued on _udi_region_q_head.
 */
udi_boolean_t
_udi_region_daemon_work(void)
{
	udi_queue_t *elem;
	_udi_region_t *rgn;

	_OSDEP_MUTEX_LOCK(&_udi_region_q_lock);

	if (UDI_QUEUE_EMPTY(&_udi_region_q_head)) {
		_OSDEP_MUTEX_UNLOCK(&_udi_region_q_lock);
		return FALSE;
	}

	/*
	 * Note that we have to grab and release the mutex
	 * on the deferred region queue while in the loop becuase
	 * the region we're about to run may need to queue something
	 * onto the very region we're running.
	 *
	 * A potential optimization is to reduce the number of
	 * lock roundtrips in this exercise...
	 */
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

	return TRUE;
}

static void
_udi_region_daemon(void *arg)
{
	_OSDEP_ASSERT(getpl() == PLBASE);

	for (;;) {
		if (!_udi_region_daemon_work())
			EVENT_WAIT(&_udi_region_q_event, primed);
		else
			EVENT_CLEAR(&_udi_region_q_event);
		if (_udi_kill_daemon) {
			EVENT_SIGNAL(&_udi_dead_daemon, PRIMED);
			kthread_exit();
			/*
			 * NOTREACHED 
			 */
		}
	}
}

/*
 * _unload is called by the system upon driver unload of a UnixWare DDI8 module.
 * This is entry point responsible for teardown of the core environment.  
 * Mappers, drivers, and metas provide their own _unload().
 */
int
_unload(void)
{
	_udi_MA_deinit(udi_unload_complete);
	_OSDEP_EVENT_WAIT(&udi_ev);
	_udi_kill_daemon = 1;
	EVENT_SIGNAL(&_udi_alloc_event, 0);
	EVENT_WAIT(&_udi_dead_daemon, PRIMED);
	udid_drv_detach();
#if INSTRUMENT_MALLOC
	_udi_dump_mempool();
#endif

	return 0;
}

/*
 * XXX - Including <sys/ddi.h> above hoses the definition of PRIMEM,
 * so we wait and do it down here instead.
 */

#define _DDI 8
#ifdef _KERNEL_HEADERS
#include <io/ddi.h>
#include <io/autoconf/confmgr/confmgr.h>
#else
#include <sys/ddi.h>
#include <sys/confmgr.h>
#endif


/*
 * For (future) DDI8 interface.
 */
static int udid_open(void *idata,
		     channel_t * channelp,
		     int oflags,
		     cred_t * crp,
		     queue_t * q);
static int udid_close(void *idata,
		      channel_t channel,
		      int oflags,
		      cred_t * crp,
		      queue_t * q);
static int udid_devinfo(void *idata,
			channel_t channel,
			di_parm_t parm,
			void *valp);
static int udid_ioctl(void *idata,
		      channel_t channel,
		      int cmd,
		      void *arg,
		      int oflags,
		      cred_t * crp,
		      int *rvalp);

static drvops_t udid_drvops = {
	NULL,				/* d_config */
	udid_open,			/* d_open */
	udid_close,			/* d_close */
	udid_devinfo,			/* d_devinfo */
	NULL,				/* d_biostart */
	udid_ioctl,			/* d_ioctl */
	NULL,				/* d_drvctl */
	NULL,				/* d_mmap */
};

static drvinfo_t udid_drvinfo = {
	&udid_drvops,			/* drv_ops structure */
	"udi",				/* drv_name */
	D_MP,				/* drv_flags */
	NULL,				/* We aren't streams */
	1				/* drv_maxchan */
};

STATIC int _udi_driver_config(cfg_func_t func,
			      void *idata,
			      rm_key_t rmkey);

STATIC drvops_t _udi_driver_drvops = {
	_udi_driver_config
};



void
udid_drv_detach(void)
{
	drv_detach(&udid_drvinfo);
}

/*
 * _load is called by the system upon driver load of a UnixWare DDI8 module.
 * This is the main entry point of the driver and therefore the environment.
 */
int
_load(void)
{
	int i;

	EVENT_INIT(&udi_ev);
	EVENT_INIT(&_udi_alloc_event);
	EVENT_INIT(&_udi_dead_daemon);

#if INSTRUMENT_MALLOC
	_OSDEP_SIMPLE_MUTEX_INIT(&_udi_allocation_log_lock,
				 "UDI mem alloc log");
	UDI_QUEUE_INIT(&_udi_allocation_log);
#endif
	_udi_alloc_init();
	_udi_MA_init();

	/*
	 * Spawn alloc daemon 
	 */
	if (0 != kthread_spawn(_udi_alloc_daemon, NULL, "udi_memd", NULL)) {
		cmn_err(CE_PANIC, "Failed to create UDI alloc daemon");
		/*
		 * NOTREACHED 
		 */
	}

	/*
	 * And the region daemon 
	 */
	UDI_QUEUE_INIT(&_udi_region_q_head);
	_OSDEP_MUTEX_INIT(&_udi_region_q_lock, "udi_rgn_q");
	_OSDEP_EVENT_INIT(&_udi_region_q_event);

	if (0 != kthread_spawn(_udi_region_daemon, NULL, "udi_rgnd", NULL)) {
		cmn_err(CE_PANIC, "Failed to create UDI alloc daemon");
		/*
		 * NOTREACHED 
		 */
	}
	drv_attach(&udid_drvinfo);
	return 0;			/* Success */
}

/*
 * External entry point called by kernel's mod code (or possibly glue code
 * for systems lacking such support) to register a driver with the UDI 
 * environment.
 *   initp - pointer to this module's udi_{init,meta}_info.
 *   sprops_text - pointer to ascii text image of udiprops.txt
 *   sprops_length - length of above.
 * Returns a handle that should be passed back to _udi_module_unload()
 * to deregister that module.   Only _udi_module_load and _udi_module_unload
 * know the contents of that handle is a _udi_module_data_t *.
 */

typedef struct {
        _udi_driver_t * md_driver;
        const char *md_name;
        void * md_meta;     
	struct interface *md_interface;
} _udi_module_data_t;

void *
_udi_module_load(void *initp, char *sprops_text, int sprops_length)
{
	_OSDEP_DRIVER_T driver_info = { NULL };
	udi_boolean_t is_meta;
	_udi_driver_t *driver;
	const char *myname;
	char *provided_meta;
	_udi_module_data_t *md = _OSDEP_MEM_ALLOC(sizeof(*md), 0, UDI_WAITOK);
	
	driver_info.sprops = udi_std_sprops_parse(sprops_text, 
						  sprops_length, NULL);
	is_meta = _udi_module_is_meta(driver_info.sprops->props_text,
				      driver_info.sprops->props_len,
				      &provided_meta) ;
	driver_info.sprops->is_meta = is_meta;
	myname = _udi_get_shortname_from_sprops(driver_info.sprops);
	if (is_meta) {
		char provided_metab[512];
		char *s = provided_meta;
		char *d = provided_metab;
		int meta_name_len;
		char *p;
		int symbols_count = 0;
		int i = 0;
		unsigned long version;
		struct interface* interface;
		struct intfc_sym* intfc_sym;
		extern struct interface interfaces[];
		struct interface *ip, *pp;
		udi_mei_init_t *meta_info = initp;


		/* 
		 * Copy the 'provides' name from the sprops to a local
		 * buffer.   This is  temporary until we can add an
		 * 'sprops_get_provides...' facility.
		 */
		while (*s != ' ') {
			*d++ = *s++;
		}
		version = strtoul(s, NULL, 16);
		*d = '\0';

		/* 
		 * First pass simply counts them.
		 */

		for (p = s; p < driver_info.sprops->props_text + driver_info.sprops->props_len; p++) {
			if (strncmp(p, "symbols ", 8) == 0) {
				symbols_count++;
			}
			p += strlen(p) + 0;

		}

		interface = _OSDEP_MEM_ALLOC(sizeof(struct interface) + 
				(sizeof(struct intfc_sym) * (symbols_count+1)), 
				0, UDI_WAITOK);
		intfc_sym = (struct intfc_sym*) ((char *)interface + 
				sizeof(*interface));
		
		for (p = s; p < driver_info.sprops->props_text + 
				driver_info.sprops->props_len; p++) {
			if (strncmp(p, "symbols ", 8) == 0) {
				intfc_sym[i].ifs_name = p+8;
				i++;
			}
		}
		meta_name_len = strlen(provided_metab);
		interface->if_name = _OSDEP_MEM_ALLOC(meta_name_len+1,
					0, UDI_WAITOK);
		strcpy(interface->if_name, provided_metab);
		/* FIXME: get rid of "100" here */
		interface->if_version = _OSDEP_MEM_ALLOC(100, 0, UDI_WAITOK);
		udi_snprintf(interface->if_version, 100, "%x.%02x", 
			version / 256, version % 256);
		interface->if_symbols = intfc_sym;

		/* Add three: leading space, two null terminators. */
		interface->if_depends = 
			_OSDEP_MEM_ALLOC(strlen(myname)+3, 0, UDI_WAITOK);
		interface->if_depends[0] = ' ';
		strcpy(&interface->if_depends[1], myname);

		/*
		 * Walk through kernel's linked list.  Append our entry
		 * to end.  Replace one if it already exists.
		 */
		for (ip = interfaces; ip; pp = ip, ip = ip->if_next_intfc) {
			if (ip->if_next_intfc == NULL) {
				ip->if_next_intfc = interface;
				break;
			}
			if (strcmp(ip->if_name, interface->if_name) == 0) {
				interface->if_next_intfc = ip->if_next_intfc;
				pp->if_next_intfc = interface;
				break;
			}
		}

		md->md_interface = interface;
		md->md_name = myname;
		/*
		 * Avoid MA meta load for driver libraries.
		 */
		if (meta_info && meta_info->ops_vec_template_list) {
			md->md_meta = _udi_MA_meta_load(provided_metab, 
							initp, 
							driver_info);
		}
		return md;
	}

	driver = _udi_MA_driver_load(myname, initp, driver_info);
	if (driver != NULL) {
		int r;
		r = _udi_MA_install_driver(driver);
		if (r != UDI_OK) {
			return NULL;
		}
		if (_udi_scsi_driver(driver)) {
			drvinfo_t *drvinfo;
			int retval;
			extern drvops_t _udi_driver_drvops;
			/*
			 * Allocate and build a drvinfo structure here 
			 */

			drvinfo = _OSDEP_MEM_ALLOC(sizeof (drvinfo_t),
						   UDI_MEM_NOZERO, UDI_WAITOK);
			drvinfo->drv_ops = &_udi_driver_drvops;
			drvinfo->drv_name = myname;
			drvinfo->drv_flags = D_MP | D_HOT;
			drvinfo->drv_str = NULL;
			drvinfo->drv_maxchan = 0;

			if ((retval = drv_attach(drvinfo))) {
				_OSDEP_MEM_FREE(drvinfo);
				return NULL;
			}

			driver->drv_osinfo.drvinfo = drvinfo;
		}
		md->md_driver = driver;
	}
	return md;
}

/*
 * Given a handle returned by an earlier _udi_module_load, unbind and
 * destroy that module regardless of type.
 * 
 * Called by the kernel upon unload of a module.
 */

int
_udi_module_unload(void *mod)
{
	_udi_module_data_t *md = (_udi_module_data_t *) mod;

	if (md == NULL) {
		return 0;
	}

	if (md->md_driver) {
		_udi_driver_t *drv = md->md_driver;

		if (_udi_scsi_driver(md->md_driver)) {
			drvinfo_t *drvinfo;

			drvinfo = drv->drv_osinfo.drvinfo;
			drv->drv_osinfo.drvinfo = NULL;
			drv_detach(drvinfo);
			_OSDEP_MEM_FREE(drvinfo);
			UDI_QUEUE_REMOVE(&drv->link);
		}
		_udi_MA_destroy_driver_by_name(drv->drv_name);
	}

	if (md->md_meta) {
		_udi_MA_meta_unload(md->md_name);
	}

	/*
	 * Do not dynamicly unload interfaces on UnixWare 7.
	 * They weren't dynamically loaded, so we can't unload them.
	 * On OpenUNIX 8, this works becuase we have a Secret 
	 * Handshake with the mod code.
	 */
	if (md->md_interface && utsname.version[0] != '7') {
		extern struct interface interfaces[];
		struct interface *ip;

		/* 
		 * Walk through the list of interfaces.  
		 * Unchain ourselves.
		 */
		for (ip = interfaces; ip; ip = ip->if_next_intfc) {
			if (ip->if_next_intfc == md->md_interface) {
				ip->if_next_intfc = md->md_interface->if_next_intfc;
				break;
			}
		}

		_OSDEP_MEM_FREE(md->md_interface->if_version);
		_OSDEP_MEM_FREE(md->md_interface->if_name);
		_OSDEP_MEM_FREE(md->md_interface->if_depends);
		_OSDEP_MEM_FREE(md->md_interface);
	}
	_OSDEP_MEM_FREE(md);
	return 0;
}

/*
 * open() entry point is called when an application opens a channel.
 */

/* ARGSUSED */
static int
udid_open(void *idata,
	  channel_t * channelp,
	  int oflags,
	  cred_t * crp,
	  queue_t * q)
{
	int err;

	/*
	 * Most drivers don't do open redirection, so they just use
	 * (*channelp) as is. If this driver did open redirection,
	 * it would pick an appropriate channel number, change *channelp
	 * accordingly, and proceed to open that channel.
	 */

	/*
	 * Reading/Writing to this device
	 * requires privilege.
	 */
	if ((oflags & FWRITE) && (err = drv_priv(crp)))
		return err;
	return 0;
}

/*
 * close() entry point is called when an application closes a channel,
 * but only when the last close for that channel occurs.
 */

/* ARGSUSED */
static int
udid_close(void *idata,
	   channel_t channel,
	   int oflags,
	   cred_t * crp,
	   queue_t * q)
{

	return 0;
}

/*
 * devinfo() entry point is called to query device information on an
 * open channel.
 */

/* ARGSUSED */
static int
udid_devinfo(void *idata,
	     channel_t channel,
	     di_parm_t parm,
	     void *valp)
{
	return 0;
}

/*
 * ioctl() entry point is called when an application uses ioctl(2)
 * to perform a driver-specific operation on an open channel.
 *
 * This is the only entry point that has user context and thus
 * can call copyin() et al.
 */

/* ARGSUSED */
static int
udid_ioctl(void *idata,
	   channel_t channel,
	   int cmd,
	   void *user_arg,
	   int oflags,
	   cred_t * crp,
	   int *rvalp)
{
	int err;
	_udi_dev_node_t *node;
	udid_instance_attr_t arg;
	void *val;
	udi_boolean_t persistent;

	switch (cmd) {
	case UDID_INSTANCE_ATTR_GET:{

			err = copyin((char *)user_arg, (char *)&arg,
				     sizeof (udid_instance_attr_t));
			if (err == -1)
				return EFAULT;
			arg.status = UDID_OK;
			val = kmem_zalloc(arg.length, KM_SLEEP);
			node = _udi_find_node_by_rmkey(arg.key);
			if (node == NULL) {
				arg.status = UDID_NODE_NOT_FOUND;
				break;
			}
			switch (arg.name[0]) {
			case '^':
				node = node->parent_node;
				break;
			case '@':
				node = _udi_get_child_node(node, arg.child_id);
			}
			arg.actual_size =
				_udi_instance_attr_get(node, arg.name, val,
						       arg.length,
						       &arg.attr_type);
			err = copyout((char *)val, (char *)arg.value,
				      arg.actual_size);
			kmem_free(val, arg.length);
			if (err == -1) {
				return EFAULT;
			}
			break;
		}
	case UDID_INSTANCE_ATTR_SET:{

			err = copyin((char *)user_arg, (char *)&arg,
				     sizeof (udid_instance_attr_t));
			if (err == -1)
				return EFAULT;
			arg.status = UDID_OK;
			val = kmem_zalloc(arg.length, KM_SLEEP);
			err = copyin((char *)arg.value, (char *)val,
				     arg.length);
			if (err == -1) {
				kmem_free(val, arg.length);
				return EFAULT;
			}
			node = _udi_find_node_by_rmkey(arg.key);
			if (node == NULL) {
				arg.status = UDID_NODE_NOT_FOUND;
				break;
			}
			switch (arg.name[0]) {
			case '^':
				node = node->parent_node;
			case '$':
				break;
			case '%':
			case '@':
				persistent = TRUE;
				break;
			default:
				node = _udi_get_child_node(node, arg.child_id);
			}

			arg.status =
				_udi_instance_attr_set(node, arg.name, val,
						       arg.length,
						       arg.attr_type,
						       persistent);
			kmem_free(val, arg.length);
			break;
		}
	default:
		return EINVAL;
	}
	err = copyout((char *)&arg, (char *)user_arg,
		      sizeof (udid_instance_attr_t));
	if (err == -1) {
		return EFAULT;
	}
	return 0;
}

udi_status_t
_udi_register_isr(int (*isr) (),
		  udi_cb_t *gcb,
		  void *context)
{
	struct rm_args rma;
	cm_num_t irq;
	boolean_t was_empty;

	/*
	 * Note that we want the child node 
	 */
	_udi_dev_node_t *node = _UDI_GCB_TO_CHAN(gcb)->
		other_end->chan_region->reg_node;
	_OSDEP_DEV_NODE_T *osinfo = &node->node_osinfo;

	rma.rm_key = osinfo->rm_key;
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
	if (!cm_intr_attach(rma.rm_key, isr, context, &udid_drvinfo,
			    &osinfo->cookie)) {
		/*
		 * cm_intr_attach failed 
		 */
		return UDI_STAT_MISTAKEN_IDENTITY;
	}

	return UDI_OK;
}

udi_status_t
_udi_unregister_isr(int (*isr) (),
		    udi_cb_t *gcb,
		    void *context)
{
	/*
	 * Note that we want the child node 
	 */
	_udi_dev_node_t *node = _UDI_GCB_TO_CHAN(gcb)->
		other_end->chan_region->reg_node;
	_OSDEP_DEV_NODE_T *osinfo = &node->node_osinfo;

	cm_intr_detach(osinfo->cookie);
	return UDI_OK;
}

STATIC void
_udi_unbind_done(void *arg)
{

	_OSDEP_EVENT_T *ev = (_OSDEP_EVENT_T *)arg;

	_OSDEP_EVENT_SIGNAL(&(*ev));
}

STATIC int
_udi_driver_config(cfg_func_t func,
		   void *idata,
		   rm_key_t rmkey)
{

	_udi_dev_node_t *node = idata;

	switch (func) {

	case CFG_ADD:{
			void **idatap = idata;
			cm_args_t cma;
			char modname[RM_MAXPARAMLEN];
			_udi_driver_t *drv;
			_udi_dev_node_t *node;
			udi_queue_t *elem, *tmp, *nelem, *ntmp;

			/*
			 * Find the node and return it in idata
			 */

			/*
			 * find the driver first 
			 */
			cma.cm_key = rmkey;
			cma.cm_n = 0;
			cma.cm_param = CM_MODNAME;
			cma.cm_val = modname;
			cma.cm_vallen = sizeof (modname);
			cm_begin_trans(cma.cm_key, RM_READ);
			cm_getval(&cma);
			cm_end_trans(cma.cm_key);

			drv = _udi_driver_lookup_by_name(modname);
			if (drv != NULL) {
				/*
				 * Found the driver, find the node now 
				 */
				UDI_QUEUE_FOREACH(&drv->node_list, nelem, ntmp) {
					node = UDI_BASE_STRUCT(nelem,
							       _udi_dev_node_t,
							       drv_node_list);

					if (node->node_osinfo.rm_key == rmkey) {
						/*
						 * found the node 
						 */
						/*
						 * Wait for enumeration to complete 
						 */
						_OSDEP_EVENT_WAIT(&node->
								  node_osinfo.
								  enum_done_ev);
						*idatap = node;
						return 0;
					}
				}
			}
			return ENODEV;
		}
	case CFG_SUSPEND:{
			_OSDEP_EVENT_T event;
			udi_status_t status;
			udi_ubit8_t flags;

			_OSDEP_EVENT_INIT(&event);
			_udi_sync_devmgmt_req(node, &event, &status, &flags,
					      UDI_DMGMT_SUSPEND, 0);
			_OSDEP_EVENT_DEINIT(&event);
			break;
		}
	case CFG_RESUME:{
			_OSDEP_EVENT_T event;
			udi_status_t status;
			udi_ubit8_t flags;

			_OSDEP_EVENT_INIT(&event);
			_udi_sync_devmgmt_req(node, &event, &status, &flags,
					      UDI_DMGMT_RESUME, 0);
			_OSDEP_EVENT_DEINIT(&event);
			break;
		}
	case CFG_REMOVE:{
			_OSDEP_EVENT_T event;

			_OSDEP_EVENT_INIT(&event);
			_udi_MA_unbind(node, &_udi_unbind_done,
				       (void *)&event);
			_OSDEP_EVENT_WAIT(&event);
			_OSDEP_EVENT_DEINIT(&event);
			break;
		}
	case CFG_MODIFY:
		/*
		 * Nothing to be done 
		 */
		break;

	case CFG_VERIFY:
	default:
		return EOPNOTSUPP;
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

	mem = kmem_alloc(MEM_ALLOC_PAD_SIZE + sizeof (_mblk_header_t) + size,
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

#if MEM_ALLOC_PAD_SIZE
	udi_memset((char *)dbuf + size, 0x33, MEM_ALLOC_PAD_SIZE);
#endif

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
	_mblk_header_t *mem = (_mblk_header_t *)((char *)buf -
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

#if MEM_ALLOC_PAD_SIZE
	size += MEM_ALLOC_PAD_SIZE;
	{
		int z;
		int cnt = 0;
		unsigned char *zb = (unsigned char *)buf + mem->size;

		for (z = 0; z < MEM_ALLOC_PAD_SIZE; z++) {
			if (zb[z] != 0x33) {
				cnt++;
				printf("%d: %02x ", z, zb[z]);
				if (cnt > 32) {
					break;
				}
			}
		}
		if (cnt) {
			_OSDEP_DEBUG_BREAK;
		}
	}
#endif /*  MEM_ALLOC_PAD_SIZE */

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
		_mblk_header_t *mblk = (_mblk_header_t *)e;

		sz += mblk->size;
		debug_printf("Allocated from %s:%d. size is %d(0x%x)/%p\n",
			     mblk->allocator_fname,
			     mblk->allocator_linenum,
			     mblk->size,
			     mblk->size,
			     (char *)mblk + sizeof (_mblk_header_t));
		if (debug_output_aborted())
			break;
	}
	debug_printf("Total allocated memory: %d(0x%x) bytes\n", sz, sz);
}

#endif

/*
 * Function to fill in udi_limit_t struct for region creation.
 */
void
_udi_get_limits_t(udi_limits_t *limits)
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
}

void *
udi_memchr(const void *s,
	   udi_ubit8_t c,
	   udi_size_t n)
{
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
}

void *
udi_memset(void *s,
	   udi_ubit8_t c,
	   udi_size_t n)
{
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
}

#if LATER

/*
 * Queue Management Routines
 */
void
udi_enqueue(udi_queue_t *element,
	    udi_queue_t *queue)
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
udi_dequeue(udi_queue_t *element)
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

	child_meta_max = _udi_osdep_sprops_count(drv, UDI_SP_CHILD_BINDOPS);

	/*
	 * Obtain all child metas 
	 */
	for (m_idx = 0; m_idx < child_meta_max; m_idx++) {
		meta_idx = _udi_osdep_sprops_get_bindops(drv,
							 m_idx,
							 UDI_SP_CHILD_BINDOPS,
							 UDI_SP_OP_METAIDX);
		meta_name =
			(char *)_udi_osdep_sprops_get_meta_if(drv, meta_idx);

		if (udi_strcmp(meta_name, "udi_scsi") == 0)
			return TRUE;
	}
	return FALSE;
}

/*
 * Returns TRUE if the given .udiprops section represents a 
 * metalanguage library.
 */
static udi_boolean_t 
_udi_module_is_meta(char *sprops_text, int sprops_size, char **provides)
{
	char *sprops;
	int idx = 0;
	int i;
	
	sprops = sprops_text;
	for (idx = 0; idx < sprops_size; i++) {
		if (udi_strncmp(&sprops[idx], "provides ", 9) == 0) {
			*provides = &sprops[idx+9];
			return TRUE;
		}
		idx = idx + strlen(&sprops[idx]) + 1;
	}
	*provides = NULL;
	return FALSE;

}

static int
_udi_sysdump_handler(void *intr_cookie)
{
	while (_udi_alloc_daemon_work() || _udi_region_daemon_work());

	return cm_intr_call(intr_cookie);
}

int
_udi_sysdump_alloc(_udi_dev_node_t *parent_node,
		   void **intr_cookie)
{
	struct cm_intr_cookie *cookiep;

	cookiep =
		_OSDEP_MEM_ALLOC(sizeof (struct cm_intr_cookie), 0,
				 UDI_WAITOK);
	cookiep->ic_intr_info.int_idata = parent_node->node_osinfo.cookie;
	cookiep->ic_handler = _udi_sysdump_handler;

	*intr_cookie = cookiep;

	return TRUE;
}

void
_udi_sysdump_free(void **intr_cookie)
{
	_OSDEP_MEM_FREE(*intr_cookie);
	*intr_cookie = NULL;
}
