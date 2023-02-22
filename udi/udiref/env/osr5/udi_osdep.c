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
 * Written for SCO by: Rob Tarte
 * At: Pacific CodeWorks, Inc.
 * Contact: http://www.pacificcodeworks.com
 */

/*
 *	Copyright 1998 The Santa Cruz Operation, Inc.  All Rights Reserved.
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF SCO, Inc.
 *	The copyright notice above does not evidence any
 *	actual or intended publication of such source code.
 */
#define UDI_PCI_VERSION	0x101

#include <udi_env.h>	/* Common Environment Interfaces */
#include <udi_pci.h>	/* UDI PCI Bus Bindings */
#include <udi_sprops.h>

#include <sys/types.h>
#include <sys/cmn_err.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/immu.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/ci/ciintr.h>
#include <sys/xdebug.h>
#include <sys/scsi.h>
#include <sys/pci.h>
#include <sys/seg.h>
#include <sys/reg.h>
#include <sys/map.h>
#include <sys/stream.h>
#include <sys/clock.h>
#include <limits.h>

/*
 * Defines: ------------------------------------------------------------------
 */
#define MEMPOOL_SIZE		2000000

#define OSDEP_UDI_META		1
#define	OSDEP_UDI_MAPPER	2
#define OSDEP_UDI_DRIVER	3

/*
 * Interrupt stubs: ----------------------------------------------------------
 */
/*
 * Interrupt stub template that will be filled in.
 * This code must be dword aligned.
 */
#define STUB_CONTEXT	0x01
#define STUB_ROUTINE	0x06

unsigned char	_osdep_int_stub[] = {
	0x68, 0x00, 0x00, 0x00, 0x00,	/* pushl $0x0 (STUB_CONTEXT) */
	0xe8, 0x00, 0x00, 0x00, 0x00,	/* call near 0x00 (STUB_ROUTINE) */
	0x58,				/* popl %eax */
	0xc3				/* ret */
};


/*
 * Structures: ----------------------------------------------------------------
 */
struct udi_init_entry {
	udi_queue_t			init_list;
	udi_queue_t			load_list;
 	const char			*modname;
	_udi_driver_t			*drv;
	_udi_sprops_idx_t		*sprops;
	void				*init_info;
	int				flag;
	struct pci_devinfo		devinfo;
	unsigned			loaded;
};

struct interrupt_info {
	udi_queue_t		link;
	driver_info		dinf;
	udi_ubit32_t		vendor_id;
	udi_ubit32_t		device_id;
	struct pci_devinfo	devinfo;
	unsigned char		*interrupt_stub;
};

/*
 * A header that is prepended to all allocated requests.
 * We stash the size in this header.   It could later be used to
 * store debugging info such as the address of the allocation caller.
 */
typedef struct {
	size_t		size;
	ulong_t		stamp;
	unsigned	mem_type;
} _mblk_header_t;

struct scsi_entry_point {
	udi_queue_t	Q;
	char		*name;
	int		(*entry_routine)(REQ_IO *);
};

/*
 * Debug: --------------------------------------------------------------------
 */
#define _UDI_MAX_DEBUG_PRINTF_LEN	100

#ifdef DEBUG_ALLOC
#define	_UDI_MEM_STAMP	0xDEADBEEF
#define FENCE_SIZE	0x10
#else
#define FENCE_SIZE	0x0
#endif

#ifdef BTLD_DEBUG
void insert_symbols();
#endif

#define PRINT_BUF_SIZE		(20*1024)

struct osdep_print_buffer {
	int	print_offset;
	int	print_wrap;
	char	*print_buffer;
};

#ifdef DEBUG_OSDEP
struct osdep_print_buffer	_udi_osdep_print_buffer[1];
_OSDEP_MUTEX_T			udi_print_lock;
#define NUM_LINES		10
#endif

#if defined(DEBUG_OSDEP) || defined(_OSDEP_LOCK_DEBUG)
#define ESC			0x1b
#endif

#ifdef DEBUG_OSDEP
int _osdep_udi_buffer_flag = 1;
#else
int _osdep_udi_buffer_flag = 0;
#endif

#ifdef DEBUG_ALLOC
struct udi_mem_stats	_udi_mem_stats[OSDEP_NUM_MEM_TYPE + 1];
#endif

#ifdef _OSDEP_LOCK_DEBUG
struct _mutex	*udi_mutex_head;
unsigned	udi_check_locks;

extern struct lockb	lock_clock;

#define LOCK_CLOCK()		ilockb(&lock_clock)
#define UNLOCK_CLOCK(x)		iunlockb(&lock_clock, (x))
#endif

#ifdef DEBUG_ALLOC
int	_osdep_num_alloc;

struct	memory_info {
	void		*memptr;
	void		*caller1;
	void		*caller2;
	void		*caller3;
};

#define MEMORY_INFO_TABLE_SIZE	10000
struct memory_info	memory_info_table[MEMORY_INFO_TABLE_SIZE];

int	osdep_save_stack(unsigned *, int);

#define UNREGISTER_STACK_SIZE	3

unsigned	unregister_stack[UNREGISTER_STACK_SIZE];

#endif

#ifdef BTLD_DEBUG
#pragma pack(1)
struct old_sent {
	long	se_vaddr;
	char	se_flags;
	char	*se_name;
};

struct udi_symbol_entry { char *name; void (*ptr)(); };

extern struct udi_symbol_entry	udi_symbol_table[];
extern int			udi_num_symbols;

extern struct old_sent		*db_symtable[];
extern int			db_nsymbols;
extern int			db_symtablesize;

#define SF_TEXT	0x01
#define SF_DATA	0x02
#define SF_ANY	(SF_TEXT|SF_DATA)
#endif


/*
 * Prototypes: ---------------------------------------------------------------
 */
int		_osdep_register_interrupt(struct udi_init_entry *, int, int,
			_udi_driver_t *);
int		_osdep_register_mapper(_udi_driver_t *, struct pci_devinfo);
int		_osdep_register_scsi(_udi_driver_t *);
int		_udi_mapper_load(const char *, udi_init_t *,
			_udi_sprops_idx_t *);
udi_boolean_t	_udi_has_unloaded_provider(char *, _udi_sprops_idx_t *);
udi_boolean_t	_udi_check_driver_load();
int		udi_start_daemons();
STATIC void	_udi_alloc_daemon(void);
STATIC void	_udi_region_daemon(void);
int		udi_io_init();
int		udi_post_init();
_udi_dev_node_t *_udi_find_node(struct udi_init_entry *);
struct udi_init_entry *_udi_add_driver(char *,_udi_sprops_idx_t *,
		udi_init_t *, unsigned);
static void	udi_unload_complete(void);
static udi_boolean_t _udi_scsi_driver(_udi_driver_t *);
void		_udi_free_memory(struct _osdep_udi_freemem *);

/*
 * Externs: -------------------------------------------------------------------
 */
extern unsigned char activeintr;
extern void _udi_MA_init(void);
extern int (*init_tbl[])();
extern int (*io_init[])();
extern int io_init_nslots;
extern struct _osdep_udi_freemem	*_osdep_freelist;
extern int printf(char *, ...);
extern void idistributed(driver_info *);
extern int Sharegister(SHAREG *);
extern int newdaemon();
extern int get_intr_arg(int, int);


/*
 * Globals: -------------------------------------------------------------------
 */
/* 
 * Support for region daemon.
 */
_OSDEP_MUTEX_T		_udi_free_lock;
int			_udi_init_time = 1;
_OSDEP_EVENT_T		_udi_alloc_event;
struct scsi_info	*_udi_scsi_info_head;

STATIC _OSDEP_MUTEX_T	_udi_region_q_lock;
STATIC _OSDEP_EVENT_T	_udi_region_q_event;
STATIC udi_queue_t	_udi_region_q_head;

STATIC _OSDEP_MUTEX_T	_osdep_local_free_lock;
STATIC _OSDEP_EVENT_T	udi_ev;
STATIC int		udi_btld_boot;
STATIC char		*udi_scsi_driver_name;
STATIC unsigned		_osdep_interrupt_entry;
STATIC unsigned char	*_osdep_interrupt_table;
STATIC int		_udi_alloc_daemon_started;
STATIC int		_udi_region_daemon_started;
STATIC udi_queue_t	_udi_scsi_entry;
STATIC char		*_udi_mem_pool;
STATIC struct map	*_udi_mem_map;
STATIC void		(*_udi_main_return)(unsigned);
STATIC int		(*_udi_first_init_tbl_entry)();
STATIC int		(*_udi_last_io_init_entry)();
STATIC udi_queue_t	_udi_interrupt_info;
STATIC udi_queue_t	_udi_init_list;
STATIC udi_queue_t	_udi_load_list;
STATIC int		_udi_driver_init_done;
STATIC int		_udi_poll_count;

STATIC int		_udi_interrupts_disabled = 1;
STATIC int 		_udi_init_done = 0;




/*
 * ---------------------------------------------------------------------------
 * This startup sequence is very sensitive.  We have to do the meta/driver
 * loads at init time so that we will have enough information to program
 * the PIC.  All of the information used to program the PIC must be passed
 * to the kernel before mp_picinit().  All of the information about which
 * SCSI HBAs are in the system must be determined before scsi_config.  We
 * need interrupts for device enumration, so _udi_MA_install_driver must
 * be called for drivers after mp_picinit().  We have accomplished all of this
 * by straddling driver load across mp_picinit().
 *
 * The initialization sequence looks like:
 *
 * main()
 *  ...
 *  ... main stuff
 *  ...
 *  io_init[]()
 *    ... 
 *    ... init routines
 *    ...
 *    udiinit()    ------------------+
 *      udi_init()                   |
 *      _udi_MA_meta_load()          |
 *    unicinit()                     |
 *      _udi_MA_meta_load()          |
 *    uMscsinit()  ------------------+ ... Can happen in any order.
 *      _udi_MA_driver_load()        | ... The order is mdevice dependent.
 *        _udi_MA_install_driver()   |
 *    uMnicinit()  ------------------+ ... Some intermediate steps have been
 *      _udi_MA_driver_load()        | ... left out.
 *        _udi_MA_install_driver()   |
 *    eeeudinit()  ------------------+
 *      _udi_MA_driver_load()        |
 *    udptinit()  -------------------+
 *      udi_regsiter_scsi_entry() ---|
 *      _udi_MA_driver_load()        |
 *                                   :
 *    ...
 *    ... all other init routines. we steal the last entry
 *    ...
 *    udi_post_init()              
 *      _osdep_register_interrupt()  ... must happen before picinit
 *                                   ... there are no clock interrupts available
 *                                   ... here
 * 
 *  scsi_config() ... we have to know what HBAs are in the system at this point.
 *
 *  mp_picinit()  ... the PIC is initialized.  We cannot call idistributed
 *                ... after this point.
 *
 *  hd_config()
 *
 *  init_tbl[0]()                ... we steal the first entry
 *    udi_io_init()
 *      _udi_MA_install_driver() ... finish the second half of driver load
 *                               ... now that interrupts are enabled
 *        ...
 *          _udi_register_isr()
 *    iinit()
 *      srmountfun()             ... root drive is mounted
 *
 *   ... main exits
 *   udi_start_daemons()         ... start memory and region daemons
 *
 *   ... much much later ...
 *
 * udihalt() ... flush HBA caches  
 *
 * ---------------------------------------------------------------------------
 */

/*
 * ---------------------------------------------------------------------------
 * Init, load and unload
 */
/*
 * This routine is called by the init routine wrapper that is generated
 * by the tools.
 */
int
_udi_driver_load(
const char		*modname,
udi_init_t		*init_info,
_udi_sprops_idx_t	*sprops
)
{
	struct udi_init_entry	*driver_entryp;

	udi_init(0);

	driver_entryp = _udi_add_driver((char *)modname, sprops, 
		(void *)init_info, OSDEP_UDI_DRIVER);

	if (driver_entryp == NULL)
	{
		return -EIO;
	}

	while (_udi_check_driver_load() == TRUE)
	{
		/*
		 * If we loaded a driver, try again.
		 */
		continue;
	}

	return 0;
}

/*
 * This routine is called by the init routine wrapper that is generated
 * by the tools.
 */
int
_udi_meta_load(
const char		*modname,
udi_mei_init_t		*meta_info,
_udi_sprops_idx_t	*sprops
)
{
	struct udi_init_entry	*driver_entryp;

	udi_init(0);

	driver_entryp = _udi_add_driver((char *)modname, sprops, 
		(void *)meta_info, OSDEP_UDI_META);

	if (driver_entryp == NULL)
	{
		return -EIO;
	}

	while (_udi_check_driver_load() == TRUE)
	{
		/*
		 * If we loaded a driver, try again.
		 */
		continue;
	}

	return 0;
}


#if 0
/* This routine is not called anymore.
/*
 * This routine is called by the init routine wrapper that is generated
 * by the tools.
 */
STATIC
int
_udi_mapper_load(
const char		*modname,
udi_init_t		*init_info,
_udi_sprops_idx_t	*sprops
)
{
	_udi_driver_t		*drv;
	struct udi_init_entry	*driver_entryp;

#ifdef DEBUG_OSDEP
	udi_debug_printf("_udi_mapper_load %s\n", modname);
#endif

	udi_init(0);

	driver_entryp = _udi_add_driver((char *)modname, sprops,
		(void *)init_info, OSDEP_UDI_MAPPER);

	if (driver_entryp == NULL)
	{
		return -EIO;
	}

	while (_udi_check_driver_load() == TRUE)
	{
		/*
		 * If we loaded a driver, try again.
		 */
		continue;
	}

	return 0;
}
#endif

/*
 * We are keeping a list of drivers for various reasons:
 * 
 * 1) The driver "init" routine is loaded in order of mdevice entry.
 *    We may need to delay a udi driver load until something that it
 *    depends on has been loaded.
 *
 * 2) In halt we need to find the node associated with a driver.  We
 *    search down this list of drivers to find a matching node.
 *
 * 3) In udi_io_init we run the deferred _udi_MA_install_driver for
 *    the UDI drivers (not mappers or metas).
 *
 * This routine gets called for every driver init routine.
 *    
 */
STATIC
struct udi_init_entry *
_udi_add_driver(
char			*modname,
_udi_sprops_idx_t	*sprops,
udi_init_t		*init_info,
unsigned		flag
)
{
	struct udi_init_entry	*driver_entryp;

	driver_entryp = _udi_kmem_alloc(sizeof(struct udi_init_entry), 0, 0);

	if (driver_entryp == NULL)
	{
		return NULL;
	}

	driver_entryp->modname = modname;

	driver_entryp->sprops = sprops;

	driver_entryp->flag = flag;

	driver_entryp->init_info = (void *)init_info;

	UDI_ENQUEUE_TAIL(&_udi_init_list, &driver_entryp->init_list);

	driver_entryp->drv = NULL;

	return driver_entryp;
}

/*
 * This routine is run once from the driver/mapper/meta load.  It doesn't
 * matter which driver ends up calling this routine first, this is all code
 * that needs to run before any UDI driver can load.
 */
STATIC
int
udi_init(
int	stack_location
)
{
	driver_info	dinf;
	int		i;
	register int	(**initptr)();
	unsigned	*ebp;

	if (_udi_init_done)
	{
		return 0;
	}

	printcfg("udi", 0, 0, -1, -1, "UDI environment");

	_udi_init_done = 1;

#ifdef DEBUG_OSDEP
	/*
	 * We turn off the print buffer if debug is not enabled.  All debug
	 * prints will go to the console.
	 */
	_udi_osdep_print_buffer[0].print_buffer = 
		(char *)kmem_zalloc(PRINT_BUF_SIZE, KM_NOSLEEP);
#endif

	/*
	 * A hack to start our daemons after main returns.  This
	 * gets us a clean stack, guarantees that we are not spawning
	 * at init time, avoids checks to see if we should spawn
	 * the daemons during run time, and allows us to spawn the
	 * daemons as low order pids (no functionality, but more tidy).
	 */
	if (_udi_main_return == NULL)
	{
		ebp = *((unsigned **)(&stack_location - 2));

		ebp = *((unsigned **)ebp);

		ebp = *((unsigned **)ebp);

		/*
		 * If we are booting from a BTLD floppy, we have one
		 * more stack frame ... udibinit().
		 */
		if (udi_btld_boot)
		{
			ebp = *((unsigned **)ebp);
		}

		_udi_main_return = *((void (**)())(ebp + 1));

		*((int (**)())(ebp + 1)) = udi_start_daemons;
	}

#ifdef DEBUG_OSDEP
	/*
	 * This is a lock that we use to protect the print buffer.
	 */
	_OSDEP_MUTEX_INIT(&udi_print_lock, "print lock");
#endif

	/*
	 * This is a lock that we use to protect the free memory list.
	 */
	_OSDEP_MUTEX_INIT(&_udi_free_lock, "free lock");
	_OSDEP_MUTEX_INIT(&_osdep_local_free_lock, "free lock");

	/*
	 * This lock is used by the alloc daemon
	 */
	_OSDEP_EVENT_INIT(&_udi_alloc_event);

#ifdef BTLD_DEBUG
	insert_symbols();
#endif

	/*
	 * We are going to steal the first entry in the init_tbl
	 */
	_udi_first_init_tbl_entry = init_tbl[0];

	init_tbl[0] = udi_io_init;

	/*
	 * Private pool of memory to handle issues with allocating memory
	 * at interrupt time in 5.0.5.
	 */
	_udi_mem_pool = (char *)sptalloc(btoc(MEMPOOL_SIZE), PG_P|PG_RW, 0, 0);

	if (_udi_mem_pool == NULL)
	{
#ifdef DEBUG_OSDEP
		calldebug();
#endif
		return;
	}

	/*
	 * The map structure that we will use to manage our private pool of
	 * memory.
	 */
	udi_memset(_udi_mem_pool, 0x0, MEMPOOL_SIZE);

	_udi_mem_map = (struct map *)sptalloc(
		btoc((MEMPOOL_SIZE >> 4) * sizeof(struct map)),
		PG_P|PG_RW, 0, 0);

	if (_udi_mem_map == NULL)
	{
#ifdef DEBUG_OSDEP
		calldebug();
#endif
		return;
	}

	udi_memset(_udi_mem_map, 0x0, (MEMPOOL_SIZE >> 4) * sizeof(struct map));

	/*
	 * Initialize the map
	 */
	mapinit(_udi_mem_map, MEMPOOL_SIZE >> 4);

	/*
	 * Make all of the memory that we have allocated available in the map.
	 */
	mfree(_udi_mem_map, MEMPOOL_SIZE, (unsigned int)_udi_mem_pool);

	/*
	 * Initialize common code.
	 */
	_udi_alloc_init();

	_udi_MA_init();

	/*
	 * Create a queue of interrupt information from the various drivers
	 * that are loaded.  We need to gather the information ahead of time,
	 * so that we can match it with the drivers as they are loaded.
	 * Unfortunately, we have to register the ISR's before picinit, which
	 * means that we cannot do enumeration.
	 */
	UDI_QUEUE_INIT(&_udi_interrupt_info);

	UDI_QUEUE_INIT(&_udi_region_q_head);

	UDI_QUEUE_INIT(&_udi_scsi_entry);

	UDI_QUEUE_INIT(&_udi_init_list);

	UDI_QUEUE_INIT(&_udi_load_list);

	/*
	 * Don't bother checking the last entry, we will take it no
	 * matter what (io_init_nslots is really the number of entries - 1).
	 */
	for (initptr = io_init, i = 0 ; i < io_init_nslots ; initptr++, i++)
	{
		if (*initptr == NULL)
		{
			break;
		}
	}

	_udi_last_io_init_entry = *initptr;

	*initptr = udi_post_init;

	/*
	 * Allocate the interrupt table that we will populate with interrupt
	 * stubs.
	 */
	_osdep_interrupt_table = (unsigned char *)sptalloc(1, PG_P|PG_RW, 0, 0);

	return 0;
}

/*
 * Given an interface name and version, find a matching driver.  We use
 * this routine to find a pointer to the driver information for
 * a driver that provides an interface that is required by another driver.
 */
STATIC
struct udi_init_entry *
_udi_find_required_driver(
char			*require_name,
udi_ubit32_t		require_version
)
{
	udi_ubit32_t		num_provides;
	struct udi_init_entry	*driver_entryp;
	unsigned		i;
	_udi_driver_t		tmp_drv;
	char			*provide_name;
	udi_ubit32_t		provide_version;
	udi_queue_t		*temp_elem;
	udi_queue_t		*elem;

#ifdef DEBUG_OSDEP
	udi_debug_printf("... requires %s 0x%x\n", require_name,
		require_version);
	udi_debug_printf("   ... checking drivers");
#endif

	UDI_QUEUE_FOREACH(&_udi_init_list, elem, temp_elem)
	{
		driver_entryp = UDI_BASE_STRUCT(elem, struct udi_init_entry,
			init_list);
#ifdef DEBUG_OSDEP
		udi_debug_printf(" %s", driver_entryp->modname);
#endif
		tmp_drv.drv_osinfo.sprops = driver_entryp->sprops;

		num_provides = _udi_osdep_sprops_count(&tmp_drv,
			UDI_SP_PROVIDE);

		for (i = 0 ; i < num_provides ; i++)
		{
			provide_name = _udi_osdep_sprops_get_prov_if(
				&tmp_drv, i);

			provide_version =
				_udi_osdep_sprops_get_prov_ver(
					&tmp_drv, i);

			if (provide_version != require_version)
			{
				continue;
			}

			if (strcmp(provide_name, require_name) != 0)
			{
				continue;
			}
#ifdef DEBUG_OSDEP
			udi_debug_printf(" found it!\n");
#endif
			return driver_entryp;
		}
	}

#ifdef DEBUG_OSDEP
	udi_debug_printf(" didn't find it!\n");
#endif

	/*
	 * A driver providing this interface has not been registered yet.
	 */
	return NULL;
}

/*
 * This routine checks to see if a driver is depending ("requires") on an
 * interface that belongs to a driver that has not been loaded yet.
 */
STATIC
udi_boolean_t
_udi_has_unloaded_provider(
char			*modname,
_udi_sprops_idx_t	*sprops
)
{
	udi_ubit32_t		num_requires;
	_udi_driver_t		tmp_drv;
	unsigned		i;
	char			*require_name;
	udi_ubit32_t		require_version;
	struct udi_init_entry	*driver_entryp;

	tmp_drv.drv_osinfo.sprops = sprops;

	/*
	 * Get the number of interfaces that this driver "requires".  These
	 * drivers must be loaded first.
	 */
	num_requires = _udi_osdep_sprops_count(&tmp_drv, UDI_SP_REQUIRE);

#ifdef DEBUG_OSDEP
	udi_debug_printf("checking to see if %s has unloaded providers ...\n",
		modname);
#endif

	/*
	 * "udi" is a weird case becuase it requires itself.  We will just
	 * say that it does not have unloaded dependencies, so it will go
	 * ahead and load.
	 */
	if (strcmp(modname, "udi") == 0)
	{
#ifdef DEBUG_OSDEP
		udi_debug_printf("special case ... no\n");
#endif
		return FALSE;
	}

	/*
	 * Loop through all of the interfaces that this driver requires ...
	 */
	for (i = 0 ; i < num_requires ; i++)
	{
		require_name = _udi_osdep_sprops_get_require_if(&tmp_drv, i);

		require_version = 
			_udi_osdep_sprops_get_require_ver(&tmp_drv, i);

		/*
		 * Find the driver associated with the interface that is
		 * required by this driver.
		 */
		driver_entryp = _udi_find_required_driver(require_name,
			require_version);

		if (driver_entryp == NULL)
		{
			/*
			 * A driver providing this interface has not been
			 * registered yet.  This driver has has an uloaded
			 * dependency.
			 */
			return TRUE;
		}

		/*
		 * The driver providing this interface has been registered,
		 * but has not loaded yet.  It may be waiting for a driver
		 * that provides an interface that it needs to be loaded.
		 */
		if (driver_entryp->loaded == 0)
		{
#ifdef DEBUG_OSDEP
			udi_debug_printf("      not loaded :(\n");
#endif

			/*
			 * If we find one that is not loaded, then
			 * we have an unloaded dependency.
			 */
			return TRUE;
		}

		/*
		 * So far so good ...
		 */
#ifdef DEBUG_OSDEP
		udi_debug_printf("      loaded :)\n");
#endif
	}

	/*
	 * Couldn't find an unloaded dependency.
	 */
	return FALSE;
}

/*
 * Check to see if we can load a driver.  Load drivers if they can be loaded.
 */
STATIC
udi_boolean_t
_udi_check_driver_load()
{
	unsigned		i;
	udi_ubit32_t		num_requires;
	struct udi_init_entry	*driver_entryp;
	struct udi_init_entry	*tmp_driver_entryp;
	_udi_driver_t		*drv;
	_OSDEP_DRIVER_T		default_osdep_driver;
	udi_status_t		ret;
	udi_boolean_t		loaded_driver;
	udi_queue_t		*temp_elem;
	udi_queue_t		*elem;
	char			*provide_name;
	_udi_driver_t		tmp_drv;

	loaded_driver = FALSE;

	UDI_QUEUE_FOREACH(&_udi_init_list, elem, temp_elem)
	{
		driver_entryp = UDI_BASE_STRUCT(elem, struct udi_init_entry,
			init_list);

		if (driver_entryp->loaded)
		{
			/*
			 * Already loaded.  Not interesting.
			 */
			continue;
		}

		/*
		 * Check to see if this driver depends on an interface that
		 * is provided by a driver that has not loaded yet.  If it
		 * does, skip it.
		 */
		if (_udi_has_unloaded_provider((char *)driver_entryp->modname,
			driver_entryp->sprops) == TRUE)
		{
			continue;
		}

		/*
		 * The driver can be loaded.
		 */

		default_osdep_driver.sprops = driver_entryp->sprops;

		loaded_driver = TRUE;

		UDI_ENQUEUE_TAIL(&_udi_load_list, &driver_entryp->load_list);

		/*
		 * load it ...
		 */
		switch (driver_entryp->flag)
		{
		case OSDEP_UDI_DRIVER:
			drv = _udi_MA_driver_load(driver_entryp->modname,
				(udi_init_t *)driver_entryp->init_info,
				default_osdep_driver);

			if (drv == NULL)
			{
#ifdef DEBUG_OSDEP
				udi_debug_printf(
					"_udi_MA_driver_load of %s failed\n",
					driver_entryp->modname);
#endif
				break;
			}

			driver_entryp->drv = drv;

			driver_entryp->loaded = 1;

			break;

		case OSDEP_UDI_META:
			tmp_drv.drv_osinfo.sprops = driver_entryp->sprops;

			provide_name = _udi_osdep_sprops_get_prov_if(
				&tmp_drv, 0);

			_udi_MA_meta_load(provide_name,
				(udi_mei_init_t *)driver_entryp->init_info,
				default_osdep_driver);

			driver_entryp->loaded = 1;

			break;

#if 0
		case OSDEP_UDI_MAPPER:
			drv = _udi_MA_driver_load(driver_entryp->modname,
				(udi_init_t *)driver_entryp->init_info,
				default_osdep_driver);

			if (drv == NULL)
			{
#ifdef DEBUG_OSDEP
				udi_debug_printf(
					"_udi_MA_driver_load of %s failed\n",
					driver_entryp->modname);
#endif
				break;
			}

			driver_entryp->drv = drv;

			driver_entryp->loaded = 1;

			/*
			 * If this is a mapper, then we can go ahead and
			 * call install_driver because mappers don't have
			 * interrupt handlers and don't need interrupt handlers
			 * to be enabled.
			 */
			ret = _udi_MA_install_driver(drv);

			if (ret != UDI_OK)
			{
#ifdef DEBUG_OSDEP
				udi_debug_printf("install_driver %s failed\n",
					driver_entryp->modname);
#endif
				break;
			}
#ifdef DEBUG_OSDEP
			udi_debug_printf("install_driver %s success\n",
				driver_entryp->modname);
#endif

			break;
#endif
		}
	}

	/*
	 * If we loaded a driver, let the caller know.
	 */
	return loaded_driver;
}

/*
 * This call is made from the init routine that is generated in the udi_glue.c.
 * Look at tools/osr5/osdep_make.c to find the call.  This allows us to have
 * an entry routine that can be used for the Sharegister calls.  We were using
 * a generic scsi entry routine, but all the UDI drivers showed up as 
 * "udi_scsi".
 */
void
udi_register_scsi_entry(
char		*name,
int		(*entry_routine)(REQ_IO *)
)
{
	struct scsi_entry_point	*scsi_entryp;

	udi_init(0);

	scsi_entryp = _udi_kmem_alloc(sizeof(struct scsi_entry_point), 0, 0);

	if (scsi_entryp == NULL)
	{
		return;
	}

	scsi_entryp->name = name;

	scsi_entryp->entry_routine = entry_routine;

	UDI_ENQUEUE_TAIL(&_udi_scsi_entry, &scsi_entryp->Q);
}


/*
 * See the comment above.  This routine is run as the very last init routine.
 * We steal the last entry and insert our post routine.  We will run through
 * all of the UDI drivers that we have found and register interrupts for the
 * ones that need them.  We have to do this before mp_picinit().
 */
STATIC
int
udi_post_init(
)
{
	struct udi_init_entry	*driver_entryp;
	int			dev_line_cnt;
	int			i;
	int			j;
	int			nattr;
	char			*name;
	udi_ubit32_t		value;
	int			vendor_id;
	int			device_id;
	int			ret;
	udi_queue_t		*temp_elem;
	udi_queue_t		*elem;

	/*
	 * Run what was the last init routine.
	 */
	if (_udi_last_io_init_entry != NULL)
	{
		(*_udi_last_io_init_entry)();
	}
	name = (char *)_udi_kmem_alloc(512, 0, 0);

	UDI_QUEUE_FOREACH(&_udi_init_list, elem, temp_elem)
	{
		driver_entryp = UDI_BASE_STRUCT(elem, struct udi_init_entry,
			init_list);

		if (driver_entryp->flag != OSDEP_UDI_DRIVER)
		{
			continue;
		}

#ifdef DEBUG_OSDEP
		udi_debug_printf("driver: %s drv = 0x%x\n",
			driver_entryp->modname, driver_entryp->drv);
#endif

		dev_line_cnt = _udi_osdep_sprops_count(driver_entryp->drv,
			UDI_SP_DEVICE);

#ifdef DEBUG_OSDEP
		udi_debug_printf("number of lines = %d\n", dev_line_cnt);
#endif

		for (i = 0 ; i < dev_line_cnt ; i++)
		{
			nattr = _udi_osdep_sprops_get_dev_nattr(
				driver_entryp->drv, i);

#ifdef DEBUG_OSDEP
			udi_debug_printf("number of attributes = 0x%x\n",
				nattr);
#endif

			vendor_id = -1;

			device_id = -1;

			for (j = 0 ; j < nattr ; j++)
			{
				_udi_osdep_sprops_get_dev_attr_name(
					driver_entryp->drv, i, j, name);

#ifdef DEBUG_OSDEP
				udi_debug_printf("name = %s - ", name);
#endif

				if (kstrcmp(name, "bus_type") == 0)
				{
					continue;
				}

				_udi_osdep_sprops_get_dev_attr_value(
					driver_entryp->drv, name,
					i, UDI_ATTR_UBIT32, &value);

				if (kstrcmp(name, "pci_vendor_id") == 0)
				{
					vendor_id = value;
				}

				if (kstrcmp(name, "pci_device_id") == 0)
				{
					device_id = value;
				}

#ifdef DEBUG_OSDEP
				udi_debug_printf("value = 0x%x - ", value);
#endif
			}

#ifdef DEBUG_OSDEP
			udi_debug_printf("\n");
#endif

			if ((vendor_id == -1) || (device_id == -1))
			{
				continue;
			}

#ifdef DEBUG_OSDEP
			udi_debug_printf("ID: 0x%x 0x%x\n", vendor_id,
				device_id);
#endif

			/*
			 * Register interrupts
			 */
			ret = _osdep_register_interrupt(driver_entryp,
				vendor_id, device_id, driver_entryp->drv);

			if (ret)
			{
				break;
			}
		}
	}

	_udi_kmem_free(name);

	return 0;
}

/*
 * This will be our stub interrupt handler until we can register the
 * actual handler.
 */
STATIC
void
_osdep_null_isr()
{
}

/*
 * The interrupt registration has to be split into two parts.  We have
 * to program the PIC before interrupts are enabled.  We need to have
 * interrupts enabled to perform enumeration.  What we do is to peek
 * into the drivers as they are registered, and register an interrupt
 * handler stub that is associated with the PCI ID.  We will fill in the
 * stub with valid data later.
 */
STATIC
int
_osdep_register_interrupt(
struct udi_init_entry	*driver_entryp,
int			vendor_id,
int			device_id,
_udi_driver_t		*drv
)
{
	struct pci_devinfo		*dp;
	int				irq;
	int				index;
	struct pci_devinfo		infptr;
	struct interrupt_info		*int_infop;
	int				found_one;
	unsigned char			*interrupt_stub;

	index = 0;

	found_one = 0;

	while (pci_search(vendor_id, device_id, 0xffff, 0xffff,
		index, &infptr) != 0)
	{
		found_one = 1;

		int_infop =
			_udi_kmem_alloc(sizeof(struct interrupt_info), 0, 0);

		irq = IDIST_PCI_IRQ(infptr.slotnum, infptr.funcnum);

		int_infop->dinf.version = DRV_INFO_VERS_2;
		int_infop->dinf.bus = infptr.busnum;
		int_infop->dinf.weight = int_infop->dinf.ipl = 5;
		int_infop->dinf.irq = irq;
		int_infop->dinf.route = IROUTE_GLOBAL;
		int_infop->dinf.mode = IMODE_SHARED_CDRIVERIPL;
		int_infop->dinf.processor_mask = remap_driver_cpu(
			DRIVER_CPU_DEFAULT, ICPU_EXANY);

		int_infop->dinf.name = (char *)driver_entryp->modname;

		interrupt_stub = _osdep_interrupt_table +
			(sizeof(_osdep_int_stub) * _osdep_interrupt_entry);

		int_infop->dinf.intr = (int (*)())interrupt_stub;

		bcopy((caddr_t)_osdep_int_stub, (caddr_t)interrupt_stub,
			sizeof(_osdep_int_stub));

		int_infop->interrupt_stub = interrupt_stub;

		/*
		 * Fill in a stub interrupt routine
		 */
		*(unsigned *)(int_infop->interrupt_stub + STUB_ROUTINE) =
			(unsigned char *)_osdep_null_isr -
				int_infop->interrupt_stub - (STUB_ROUTINE + 4);

		_osdep_interrupt_entry++;

#ifdef DEBUG_OSDEP
		udi_debug_printf("registering interrupt: 0x%x:0x%x 0x%x\n",
			vendor_id, device_id, irq);
#endif
		/*
		 * Register the interrupt with the OS.
		 */
		idistributed(&int_infop->dinf);

		bcopy((caddr_t)&infptr, (caddr_t)&int_infop->devinfo,
			sizeof(struct pci_devinfo));

		bcopy((caddr_t)&infptr, (caddr_t)&driver_entryp->devinfo,
			sizeof(struct pci_devinfo));

		UDI_ENQUEUE_TAIL(&_udi_interrupt_info, &int_infop->link);

		_osdep_register_mapper(drv, infptr);

		index++;
	}

	return found_one;
}


STATIC
int
_osdep_register_mapper(
_udi_driver_t		*drv,
struct pci_devinfo	devinfop
)
{
	int			index;
	int			meta_line_cnt;
	int			i;
	char			*name;

	meta_line_cnt = _udi_osdep_sprops_count(drv, UDI_SP_META);

	for (i = 0 ; i < meta_line_cnt ; i++)
	{
		index = _udi_osdep_sprops_get_meta_idx(drv, i);

		name = _udi_osdep_sprops_get_meta_if(drv, index);

#ifdef DEBUG_OSDEP
		udi_debug_printf("meta index 0x%x - name = %s\n", index, name);
#endif

		if (kstrcmp(name, "udi_scsi") == 0)
		{
			/*
			 * We have to call Sharegister before scsi_config()
			 * in main.  We have enough information at this point
			 * to know which
			 */
			_osdep_register_scsi(drv);

			continue;
		}
	}
}

/*
 * Look up the scsi entry that was registered earlier.  This routine is
 * called at udi_post_init time.
 */
STATIC
struct scsi_entry_point *
udi_find_scsi_entry(
char	*name
)
{
	struct scsi_entry_point	*scsi_entryp;
	udi_queue_t		*temp_elem;
	udi_queue_t		*elem;

	UDI_QUEUE_FOREACH(&_udi_scsi_entry, elem, temp_elem)
	{
		scsi_entryp = UDI_BASE_STRUCT(elem, struct scsi_entry_point, Q);

		if (udi_strcmp(name, scsi_entryp->name) == 0)
		{
			return scsi_entryp;
		}
	}

	return NULL;
}


/*
 * We have to call Sharegister before scsi_config() in main.
 * The scsi entry points were registered with UDI during their init routines,
 * and now we will register them with the OS.  Unfortunately, this has to
 * happen before enumeration, because scsi_config() is called before
 * mp_picinit().
 */
STATIC
int
_osdep_register_scsi(
_udi_driver_t	*drv
)
{
	struct scsi_entry_point	*scsi_entryp;
	struct scsi_info	*sinfop;
	struct Shareg_ex	*regp;
	HAINFO			*infop;

	sinfop = _udi_kmem_alloc(sizeof(struct scsi_info), 0, 0);

	regp = &sinfop->regp;

	infop = &sinfop->infop;

	sinfop->drvp = drv;

	sinfop->next = _udi_scsi_info_head;

	_udi_scsi_info_head = sinfop;

	scsi_entryp = udi_find_scsi_entry((char *)drv->drv_name);

	regp->entry = scsi_entryp->entry_routine;

	scsi_distributed(scsi_entryp->entry_routine);

	regp->version = SHAREG_VERSION2;
	regp->root_id = 0;	/* cross your fingers */
	regp->in_use = 0;	/* not pre-configured in mscsi(F) */
	regp->mode = IMODE_NONE; /* Directly attached in bridge mapper
				  * using idistributed() */
	regp->route = IROUTE_GLOBAL;	/* Just in case */
	regp->processor_mask = ICPU_ANY;

	/*
	 * Provide information about the Host to the Peripheral
	 * in the  "xx_info" struct.  Note that if you need to
	 * be able to specify that  the host driver can accept
	 * commands at interrupt time (do_buffer = 1), you must
	 * verify that v_maj = SCSI_V3_VERSION in scsi_disk_info
	 * at SCSI_INIT.
	 */
	infop->do_sg = 0; /* for now */
	infop->do_buffer = 0; /* since we need to kmem_alloc */
	infop->do_tagged = 0; /* for now */
	infop->do_drive32 = 1; /* XXX - shouldn't assume this here */
	infop->do_nomaps = 1; /*XXX-shouldn't assume this(what is it?)*/
	infop->do_reset = 1; /* maybe */
	infop->do_abort = 0; /* ??? */
	infop->do_cache_ctl = 0; /* ??? */
	infop->has_cache = 0; /* ??? */

	Sharegister((SHAREG *)regp);

#ifdef DEBUG_OSDEP
	udi_debug_printf("called Sharegister\n");
#endif

	/*
	 * This is for BTLD's.  The printcfg has to match the installing
	 * driver name.
	 */
	if (udi_scsi_driver_name != NULL)
	{
		printcfg("adapter", 0, 0, -1, -1, "type=%s SCSI Host Adapter",
			udi_scsi_driver_name);
	}
	else
	{
		printcfg("adapter", 0, 0, -1, -1, "type=%s SCSI Host Adapter",
			drv->drv_name);
	}
}

/*
 * This routine is called after mp_picinit().
 * We now have interrupts enabled, and we can start enumeration.
 */
STATIC
int
udi_io_init(
)
{
#ifdef DEBUG_OSDEP
	udi_debug_printf("udi_io_init\n");
#endif
	/*
	 * This flag controls whether we spin waiting for an event, or if
	 * we set a timeout.
	 */
	_udi_interrupts_disabled = 0;

	/*
	 * Finish driver load.
	 */
	_udi_driver_init();

	/*
	 * Among other things, this flag controls whether we spin waiting
	 * for events or not.  After this point, we will no longer spin
	 * waiting for events.
	 */
	_udi_init_time = 0;

	/*
	 * Execute the init_tbl entry point that we stole.
	 */
	(*_udi_first_init_tbl_entry)();

	return 0;
}

/*
 * Register the ISR.  We have actually already register the ISR with the
 * kernel, so this routine matches up the interrupt handler that has been
 * setup with the actual driver that will be using it.  We fill in the
 * interrupt handler stub with real data.
 */
udi_status_t
_udi_register_isr(
void				(*isr)(),
void				*context,
void				*bridge_osinfo,
_udi_intr_info_t		*intr_osinfo
)
{
	_OSDEP_BRIDGE_CHILD_DATA_T	*osinfop;
	udi_queue_t			*elem;
	udi_queue_t			*temp_elem;
	struct interrupt_info		*int_infop;

#ifdef DEBUG_OSDEP
	udi_debug_printf("registering isr\n");
#endif

	osinfop = (_OSDEP_BRIDGE_CHILD_DATA_T *)bridge_osinfo;

	intr_osinfo->isr_func = isr;

	intr_osinfo->context = context;

	UDI_QUEUE_FOREACH(&_udi_interrupt_info, elem, temp_elem)
	{
		int_infop = UDI_BASE_STRUCT(elem, struct interrupt_info, link);

		if (int_infop->devinfo.busnum != osinfop->devinfo.busnum)
			continue;

		if (int_infop->devinfo.funcnum != osinfop->devinfo.funcnum)
			continue;

		if (int_infop->devinfo.slotnum != osinfop->devinfo.slotnum)
			continue;

		/*
		 * We have a match.  We have registered this interrupt with
		 * the OS already.
		 */
		/*
		 * Get the interrupt.
		 */
		intr_osinfo->irq = 
			get_intr_arg(int_infop->devinfo.busnum,
				SHAREG_PCI_INTVEC(int_infop->devinfo.slotnum,
					int_infop->devinfo.funcnum));

		/*
		 * Fill in the stub context and isr.
		 */
		*(unsigned *)(int_infop->interrupt_stub + STUB_CONTEXT) =
			(unsigned)context;

		*(unsigned *)(int_infop->interrupt_stub + STUB_ROUTINE) =
			(unsigned char *)isr - int_infop->interrupt_stub
				- (STUB_ROUTINE + 4);

#ifdef DEBUG_OSDEP
		udi_debug_printf("irq = 0x%x\n", intr_osinfo->irq);
#endif
	}

	return UDI_OK;
}

udi_status_t
_udi_unregister_isr(
udi_dev_node_t		_node,
void			(*isr)(),
void			*context,
_udi_intr_info_t	*osinfo
)
{
	return UDI_OK;
}


/*
 * This routine does the dirty work for udi_io_init().  At this time
 * we will finish driver load.  The mappers and metas are already loaded
 * at this point, so this only handles load for the drivers (things that
 * need interrupts enabled).
 */
void
_udi_driver_init(
)
{
	udi_status_t			ret;
	struct udi_init_entry		*driver_entryp;
	udi_queue_t			*temp_elem;
	udi_queue_t			*elem;

	if (_udi_driver_init_done)
	{
		return;
	}

	_udi_driver_init_done = 1;

#ifdef DEBUG_OSDEP
	udi_debug_printf("_udi_driver_init\n");
#endif

	UDI_QUEUE_FOREACH(&_udi_load_list, elem, temp_elem)
	{
		driver_entryp = UDI_BASE_STRUCT(elem, struct udi_init_entry,
			load_list);

#ifdef DEBUG_OSDEP
		udi_debug_printf("_udi_driver_init %s\n",
			driver_entryp->modname);
#endif

		/*
		 * If the driver was mapper or meta, then it is already
		 * loaded.  We only deferred the loads for the drivers
		 * that will need interrupts enabled in order to load.
		 */
		if (driver_entryp->flag != OSDEP_UDI_DRIVER)
		{
#ifdef DEBUG_OSDEP
			udi_debug_printf("_udi_driver_init %s not DRIVER\n",
				driver_entryp->modname);
#endif

			continue;
		}

		if (driver_entryp->drv == NULL)
		{
#ifdef DEBUG_OSDEP
			udi_debug_printf("_udi_driver_init %s - drv == NULL\n",
				driver_entryp->modname);
#endif

			continue;
		}

#ifdef DEBUG_OSDEP
		udi_debug_printf("_udi_driver_init %s - install_driver\n",
			driver_entryp->modname);
#endif

		ret = _udi_MA_install_driver(driver_entryp->drv);

		if (ret != UDI_OK)
		{
#ifdef DEBUG_OSDEP
			udi_debug_printf("install_driver %s failed\n",
				driver_entryp->modname);
#endif
		}
	}

	return;
}

/*
 * ---------------------------------------------------------------------------
 * Halt code
 */
/*
 * Entry point for system/driver shutdown.  Only the "udi" and "udibtld"
 * driver have this entry point registered.  Among other things, this
 * is where the SCSI drivers will flush their caches before system halt.
 */
void
udihalt()
{
	struct udi_init_entry	*driver_entryp;
	int			s;
	_udi_driver_t		*drv;
	udi_queue_t		*elem;
	udi_queue_t		*temp_elem;
	_udi_dev_node_t		*node;

	/*
	 * Halt is similar to init time.  We do not want to sleep
	 * waiting for events to happen.
	 */
	_udi_init_time = 1;

	s = spl0();

	UDI_QUEUE_FOREACH(&_udi_init_list, elem, temp_elem)
	{
		driver_entryp = UDI_BASE_STRUCT(elem, struct udi_init_entry,
			init_list);

		switch (driver_entryp->flag)
		{
		case OSDEP_UDI_META:
			_udi_MA_meta_unload(driver_entryp->modname);
			break;

#if 0
		case OSDEP_UDI_MAPPER:
			_udi_MA_destroy_driver_by_name(driver_entryp->modname);
			break;
#endif

		case OSDEP_UDI_DRIVER:

			drv = _udi_driver_lookup_by_name(
				driver_entryp->modname);

			if (drv == NULL)
			{
#ifdef DEBUG_OSDEP
				udi_debug_printf("_udi_driver_unload: %s not "
					"found\n", driver_entryp->modname);
#endif

				continue;
			}

			node = _udi_find_node(driver_entryp);

			if (node == NULL)
			{
#ifdef DEBUG_OSDEP
				udi_debug_printf("no nodes %s\n",
					driver_entryp->modname);
#endif

				continue;
			}

			/*
			 * This hack is from UnixWare.  The net code currently
			 * doesn't unload.  nsrmap_unbind_ack does not change
			 * the state to UNBOUND, which means that
			 * nsrmap_release_resources never gets called, which
			 * means that NSR_GIO_FREE_RESOURCES never gets sent
			 * to the appropriate region, so that udi_gio_xfer_ack
			 * never gets called and is never completed in
			 * nsrmap_gio_xfer_ack.  nsrmap_gio_xfer_ack never calls
			 * udi_gio_unbind_req, which would result in a call to
			 * nsrmap_gio_unbind_ack, which should set the
			 * tx_event_cb to NULL.  If the tx_event_cb was NULL,
			 * then nsrmap_final_cleanup_req would call
			 * udi_final_cleanup_ack, which would cause _udi_unbind4
			 * to be called, which would eventually complete the
			 * _udi_MA_unbind.
			 * 
			 */
			if (_udi_scsi_driver(drv))
			{
				_OSDEP_EVENT_T	*event;
				udi_ubit8_t	*flags;
				udi_status_t	*status;	

				event = (_OSDEP_EVENT_T *)_udi_kmem_alloc(
					sizeof(_OSDEP_EVENT_T), 0, 0);

				status = (udi_status_t *)_udi_kmem_alloc(
					sizeof(udi_status_t), 0, 0);

				flags = (udi_ubit8_t *)_udi_kmem_alloc(
					sizeof(udi_ubit8_t), 0, 0);

				_OSDEP_EVENT_INIT(event);

				_udi_sync_devmgmt_req(node, event,
					status, flags, UDI_DMGMT_SUSPEND, 0);

				_OSDEP_EVENT_DEINIT(event);

				_udi_kmem_free(flags);

				_udi_kmem_free(status);

				_udi_kmem_free(event);

				_udi_MA_destroy_driver(drv);
			}
#if 0
			else
			{
				_udi_MA_destroy_driver_by_name(
					driver_entryp->modname);
			}
#endif

			break;
		}
	}

	_udi_MA_deinit(udi_unload_complete);

	_OSDEP_EVENT_WAIT(&udi_ev);

	splx(s);
}

/*
 * Used for halt
 */
STATIC
void
udi_unload_complete(void)
{
	_OSDEP_EVENT_SIGNAL(&udi_ev);
}

/*
 * We need to be able to match a driver to its node.  This is for halt.
 */
STATIC
_udi_dev_node_t *
_udi_find_node(
struct udi_init_entry	*driver_entryp
)
{
	udi_queue_t		*elem;
	udi_queue_t		*temp_elem;
	_udi_dev_node_t		*node;

	if (UDI_QUEUE_EMPTY(&driver_entryp->drv->node_list))
	{
		return NULL;
	}

	/*
	 * Found the driver, find the node now
	 */
	UDI_QUEUE_FOREACH(&driver_entryp->drv->node_list, elem, temp_elem)
	{
		node = UDI_BASE_STRUCT(elem, _udi_dev_node_t, drv_node_list);

#ifdef DEBUG_OSDEP
		udi_debug_printf("node = 0x%x\n", node);
#endif

		if (driver_entryp->devinfo.busnum !=
			node->node_osinfo.devinfo.busnum)
		{
			continue;
		}

		if (driver_entryp->devinfo.funcnum !=
			node->node_osinfo.devinfo.funcnum)
		{
			continue;
		}

		if (driver_entryp->devinfo.slotnum !=
			node->node_osinfo.devinfo.slotnum)
		{
			continue;
		}

		break;
	}

	if (elem == &driver_entryp->drv->node_list)
	{
		return NULL;
	}

	return node;
}

/*
 * Determine whether the driver is a scsi driver or not.  udihalt needs
 * to know this.
 */
STATIC
udi_boolean_t
_udi_scsi_driver(
_udi_driver_t *drv
)
{
	udi_ubit32_t	m_idx;
	udi_ubit32_t	child_meta_max;
	udi_ubit32_t	meta_idx;
	const char	*meta_name;

	child_meta_max = _udi_osdep_sprops_count(drv, UDI_SP_CHILD_BINDOPS);

	/*
	 * Obtain all child metas 
	 */
	for (m_idx = 0; m_idx < child_meta_max; m_idx++)
	{
		meta_idx = _udi_osdep_sprops_get_bindops(
				drv,
				m_idx,
				UDI_SP_CHILD_BINDOPS,
				UDI_SP_OP_METAIDX);

		meta_name = (char *)_udi_osdep_sprops_get_meta_if(
			drv, meta_idx);

		if (udi_strcmp(meta_name, "udi_scsi") == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}


/*
 * ---------------------------------------------------------------------------
 * Daemons:
 */
/*
 * This routine is called on exit from main().  We patch ourselves into the
 * return stack of main so that we can start our daemons after "init", but
 * before main initialization is over.
 */
STATIC
int
udi_start_daemons(
unsigned	retval
)
{
	register unsigned		*eip;
	volatile register unsigned	ret;

	/*
	 * This is a trick to get the return value from main (%eax),
	 * and preserve it.  udi_get_ret() was moved from this routine
	 * because the optimizer kept getting rid of it.
	 */
	ret = udi_get_ret();

	/*
	 * If we are init time, or if this routine is being called by
	 * something other than pid 0, then we are not really done with
	 * main yet.  This is the case with MPX.
	 */
	if ((_udi_init_time != 0) || (u.u_procp->p_pid != 0))
	{
		eip = (unsigned *)(&retval - 1);

		*(eip) = (unsigned)_udi_main_return;

		return ret;
	}

	if (_udi_alloc_daemon_started == 0)
	{
		_udi_spawn_alloc_daemon();
	}

	if (_udi_region_daemon_started == 0)
	{
		_udi_spawn_region_daemon();
	}

	/*
	 * Patch the real return %eip back into the stack.
	 */
	eip = (unsigned *)(&retval - 1);

	*(eip) = (unsigned)_udi_main_return;

	/*
	 * Return our saved %eax.
	 */
	return ret;
}

/*
 * Start the allocation daemon.  This is done once after main.
 */
STATIC
int
_udi_spawn_alloc_daemon(void)
{
	int	rval1;

#ifdef DEBUG_OSDEP
	udi_debug_printf("Spawning alloc daemon...\n");
#endif
	_udi_alloc_daemon_started = 1;

	rval1 = u.u_rval1;

	/* Spawn alloc daemon */
	if (newdaemon())
	{
		u.u_cstime = 0;

		u.u_stime = 0;

		u.u_cutime = 0;

		bcopy("udi_alloc", u.u_psargs, 10);

		bcopy("udi_alloc", u.u_comm, 10);

		multiprocessor_ready();

		_udi_alloc_daemon();

		cmn_err(CE_PANIC, "udi_alloc_daemon exited");
		/* NOTREACHED */
	}

	u.u_rval1 = rval1;

	return 0;
}

/*
 * Start the region daemon.  This is done once after main.
 */
STATIC
int
_udi_spawn_region_daemon(void)
{
	int	rval1;

#ifdef DEBUG_OSDEP
	udi_debug_printf("Spawning region daemon...\n");
#endif
	_udi_region_daemon_started = 1;

	rval1 = u.u_rval1;

	/* Spawn region daemon */
	if (newdaemon())
	{
		u.u_cstime = 0;

		u.u_stime = 0;

		u.u_cutime = 0;

		bcopy("udi_region", u.u_psargs, 10);

		bcopy("udi_region", u.u_comm, 10);

		/* multiprocessor_ready(); */
		_udi_region_daemon();

		cmn_err(CE_PANIC, "udi_region_daemon exited");
		/* NOTREACHED */
	}

	u.u_rval1 = rval1;

	return 0;
}

void
_udi_osdep_region_schedule(
_udi_region_t	*region
)
{
	if (!_udi_region_daemon_started)
	{
		return;
	}

	region->in_sched_q = TRUE;

	REGION_UNLOCK(region);

	_OSDEP_MUTEX_LOCK(&_udi_region_q_lock);

	UDI_ENQUEUE_TAIL(&_udi_region_q_head, &region->reg_sched_link);

	_OSDEP_MUTEX_UNLOCK(&_udi_region_q_lock);

	_OSDEP_EVENT_SIGNAL(&_udi_region_q_event);
}

STATIC
void
_udi_region_daemon(
)
{
	for (;;)
	{
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

		while (!UDI_QUEUE_EMPTY(&_udi_region_q_head))
		{
			elem = UDI_DEQUEUE_HEAD(&_udi_region_q_head);

			_OSDEP_MUTEX_UNLOCK(&_udi_region_q_lock);

			rgn = UDI_BASE_STRUCT(elem, _udi_region_t,
					      reg_sched_link);

			rgn->in_sched_q = FALSE;

			_udi_run_inactive_region(rgn);

			_OSDEP_MUTEX_LOCK(&_udi_region_q_lock);
		}

		_OSDEP_MUTEX_UNLOCK(&_udi_region_q_lock);

		_OSDEP_EVENT_WAIT(&_udi_region_q_event);
	}
}

/*
 * This is the alloc daemon.  It is woken up whenever there is memory
 * to be freed to the system.
 */
STATIC
void
_udi_alloc_daemon(void)
{
	struct _osdep_udi_freemem	*freep;

	for (;;)
	{
		_OSDEP_MUTEX_LOCK(&_udi_free_lock);

		freep = _osdep_freelist;

		_osdep_freelist = NULL;

		_OSDEP_MUTEX_UNLOCK(&_udi_free_lock);

		if (freep != NULL)
		{
			_udi_free_memory(freep);
		}

		if (!_udi_alloc_daemon_work())
		{
			_OSDEP_EVENT_WAIT(&_udi_alloc_event);
		}
	}
}

/*
 * ---------------------------------------------------------------------------
 * Kernel memory allocation routines
 */
/*
 * This routine is called by the alloc daemon when there is memory
 * to be freed.  This deferred memory free method means that memory
 * is always freed during a safe context.  We have marked the memory
 * as to what type it is, so that we will know which pool to free it
 * back to.
 */
STATIC
void
_udi_free_memory(
struct _osdep_udi_freemem	*freep
)
{
	struct _osdep_udi_freemem	*next_freep;
	struct _osdep_udi_freemem	*local_freep;
#ifdef DEBUG_ALLOC
	unsigned			mem_type;
#endif

#ifdef DEBUG_ALLOC_VERBOSE
	udi_debug_printf("freeing ");
#endif

	local_freep = NULL;

	while (freep != NULL)
	{
		next_freep = freep->next;

#ifdef DEBUG_ALLOC_VERBOSE
		udi_debug_printf("[0x%x:0x%x]", freep, freep->mem_type);
#endif
#ifdef DEBUG_ALLOC
		mem_type = freep->mem_type;
#endif
		_OSDEP_ASSERT(freep->mem_type != 0);

		switch (freep->mem_type)
		{
		case OSDEP_MEM_TYPE_GETCPAGES:
			freecpages((pfn_t)freep, btoc(freep->length));
			break;

		case OSDEP_MEM_TYPE_SPTALLOC:
			sptfree((caddr_t)freep, btoc(freep->length), 1);
			break;

		case OSDEP_MEM_TYPE_KMEM:
			kmem_free(freep, freep->length);
			break;

		case OSDEP_MEM_TYPE_LOCAL:
			freep->next = local_freep;
			local_freep = freep;
			break;

		default:
#ifdef DEBUG_ALLOC
			calldebug();
#endif
			freep->mem_type = 0;

			break;
		}

#ifdef DEBUG_ALLOC
		qincl((caddr_t)&_udi_mem_stats[mem_type].num_free);
#endif
		freep = next_freep;
	}

	if (local_freep != NULL)
	{
		_OSDEP_MUTEX_LOCK(&_osdep_local_free_lock);

		while (local_freep != NULL)
		{
			next_freep = local_freep->next;

			mfree(_udi_mem_map, (unsigned int)local_freep->length,
				(int)local_freep);

			local_freep = next_freep;
		}

		_OSDEP_MUTEX_UNLOCK(&_osdep_local_free_lock);
	}

#ifdef DEBUG_ALLOC_VERBOSE
	udi_debug_printf("\n");
#endif
}

void
_udi_osdep_new_alloc(void)
{
	if (_udi_alloc_daemon_started)
	{
		_OSDEP_EVENT_SIGNAL(&_udi_alloc_event);
	}
	else
	{
		while (_udi_alloc_daemon_work())
		{
			continue;
		}
	}
}

/*
 * The strategy is to allocate memory from kmem_alloc if at all possible.
 * If that fails, we will then try to allocate memory from our private pool.
 * If that fails .... game over.
 */
void *
_udi_kmem_alloc(
size_t	size,
int	flags,
int	sleep_ok
)
{
	_mblk_header_t	*mem;	/* Actual requested block, including header */
	void		*ret_mem;
	unsigned	total_size;
	int		sleep_flag;

	if (activeintr || mp_qrunning() || !sleep_ok || _udi_init_time)
	{
		sleep_flag = KM_NOSLEEP;
	}
	else
	{
		sleep_flag = KM_SLEEP;
	}

	/*
	 * Align the size so that we don't ever ask for non aligned memory
	 * from any pool.
	 */
	size = (size + (sizeof(unsigned) - 1)) & (~(sizeof(unsigned) - 1));

	total_size = sizeof(_mblk_header_t) + size + (FENCE_SIZE * 2);

	/*
	 * Make sure the whole thing is aligned
	 */
	_OSDEP_ASSERT((total_size & (sizeof(unsigned) - 1)) == 0);

	mem = (_mblk_header_t *)kmem_alloc(total_size, sleep_flag | KM_NO_DMA);

	if (mem == NULL)
	{
#ifdef DEBUG_ALLOC_VERBOSE
		udi_debug_printf("kmem_alloc failed - size 0x%x\n", total_size);
#endif

		_OSDEP_MUTEX_LOCK(&_osdep_local_free_lock);

		mem = (_mblk_header_t *)malloc(_udi_mem_map, total_size);

		_OSDEP_MUTEX_UNLOCK(&_osdep_local_free_lock);

		if (mem == NULL)
		{
#ifdef DEBUG_OSDEP
			udi_debug_printf("malloc failed - size 0x%x\n",
				total_size);

			calldebug();
#endif
			return NULL;
		}

		mem->mem_type = OSDEP_MEM_TYPE_LOCAL;
	}
	else
	{
		mem->mem_type = OSDEP_MEM_TYPE_KMEM;
	}

	ret_mem = (char *)((char *)mem + sizeof(_mblk_header_t) + FENCE_SIZE);

	/*
	 * Make sure our return value is aligned
	 */
	_OSDEP_ASSERT(((unsigned)ret_mem & (sizeof(unsigned) - 1)) == 0);

	if ((flags & UDI_MEM_NOZERO) != UDI_MEM_NOZERO)
	{
		udi_memset(ret_mem, 0x0, size);
	}
#ifdef DEBUG_ALLOC
	else
	{
		udi_memset(ret_mem, 0xab, size);
	}
#endif

	mem->size = size;

#ifdef DEBUG_ALLOC
	/*
	 * write a fence pattern around the data to try to catch anyone
	 * who walks of the end of memory.
	 */
	_OSDEP_ASSERT(mem->mem_type != 0);

	qincl((caddr_t)&_udi_mem_stats[mem->mem_type].num_alloc);

	udi_memset((char *)ret_mem - FENCE_SIZE, 0xaa, FENCE_SIZE);

	udi_memset((char *)ret_mem + size, 0xab, FENCE_SIZE);

#ifdef DEBUG_ALLOC_VERBOSE
	udi_debug_printf("_udi_kmem_alloc: 0x%x\n", mem);
#endif

	mem->stamp = _UDI_MEM_STAMP;

	/*
	 * Register this memory so that we can catch leaks.
	 */
	osdep_register_memory(mem);
#endif

	return ret_mem;
}

void
_udi_kmem_free(
void	*ret_mem
)
{
	_mblk_header_t			*mem;
	struct _osdep_udi_freemem	*freep;
	int				i;

	/*
	 * Walk backward in our buffer to find the header.  Use that
	 * info for kmem_free.
	 */
	mem = (_mblk_header_t*)((char *) ret_mem - sizeof(_mblk_header_t) -
		FENCE_SIZE);

#ifdef DEBUG_ALLOC
	if (mem->stamp != _UDI_MEM_STAMP)
	{
		cmn_err(CE_PANIC, 
			"_udi_kmem_free: Expected stamp %p, got %p\n",
			_UDI_MEM_STAMP, mem->stamp);
		/* NOTREACHED */
	}

	for (i = 0 ; i < FENCE_SIZE ; i++)
	{
		if (*((unsigned char *)ret_mem - FENCE_SIZE + i) != 0xaa)
		{
			udi_debug_printf("leading fence corrupted\n");

			calldebug();
		}

		if (*((unsigned char *)ret_mem + mem->size + i) != 0xab)
		{
			udi_debug_printf("trailing fence corrupted\n");

			calldebug();
		}
	}

	mem->stamp = 0xDEADBABE;

#ifdef DEBUG_ALLOC_VERBOSE
	udi_debug_printf("_udi_kmem_free: 0x%x\n", mem);
#endif

	udi_memset(ret_mem, 0xa5, mem->size);
#endif

	freep = (struct _osdep_udi_freemem *)mem;

	freep->mem_type = mem->mem_type;

	freep->length = sizeof(_mblk_header_t) + mem->size + (FENCE_SIZE * 2);

#ifdef DEBUG_ALLOC
	_OSDEP_ASSERT(mem->mem_type != 0);

	qincl((caddr_t)&_udi_mem_stats[mem->mem_type].num_defer_free);

	osdep_unregister_memory(mem);
#endif
	if (freep->mem_type == OSDEP_MEM_TYPE_LOCAL)
	{
		_OSDEP_MUTEX_LOCK(&_osdep_local_free_lock);

		mfree(_udi_mem_map, (unsigned int)freep->length, (int)freep);

		_OSDEP_MUTEX_UNLOCK(&_osdep_local_free_lock);
	}
	else if (activeintr || mp_qrunning() || _udi_init_time)
	{
		/*
		 * DPC's would be better!!
		 */
		_OSDEP_MUTEX_LOCK(&_udi_free_lock);

		freep->next = _osdep_freelist;

		_osdep_freelist = freep;

		_OSDEP_MUTEX_UNLOCK(&_udi_free_lock);

		_OSDEP_EVENT_SIGNAL(&_udi_alloc_event);
	}
	else
	{
		freep->next = NULL;

		_udi_free_memory(freep);
	}
}

/*
 * ---------------------------------------------------------------------------
 * String manipulation routines and common helper functions
 */
void *
udi_memcpy(void *s1,
	   const void *s2,
	   udi_size_t n)
{
	bcopy((caddr_t)s2, (caddr_t)s1, n);

	return (s1);
}

void *
udi_memmove(
	void *s1,
	const void *s2,
	udi_size_t n)
{
	char	*src;
	char	*dest;

	src = (char *)s2;
	dest = (char *)s1;

	if (src >= dest)
	{
		while (n-- != 0)
		{
			*dest++ = *src++;
		}
	}
	else
	{
		src += n;
		dest += n;

		while (n-- != 0)
		{
			*--dest = *--src;
		}
	}

	return s1;
}


void *
udi_memchr(
const void *s,
udi_ubit8_t c,
udi_size_t n
)
{
	return (void *)memchr(s, c, n);
}

void
__udi_assert(
	const	char *str,
	const	char *file,
	int	      line )
{
	cmn_err(CE_PANIC, "Assertion ( %s ) Failed: File = %s, Line %d",
		str, file, line);
}

/*
 * Function to fill in udi_limit_t struct for region creation.
 */
void
_udi_get_limits_t(
	udi_limits_t *limits)
{
	/* These could ultimately be tied into the KMA */
	limits->max_legal_alloc = 128*1024;
	limits->max_safe_alloc  = UDI_MIN_ALLOC_LIMIT;
	limits->max_trace_log_formatted_len = _OSDEP_TRLOG_LIMIT;
	limits->max_instance_attr_len = UDI_MIN_INSTANCE_ATTR_LIMIT;
	limits->min_curtime_res = 1000000000 / Hz;
	limits->min_timer_res = limits->min_curtime_res;
}

udi_ubit32_t
udi_strtou32(
const char	*nptr,
char		**endptr,
int		base
)
{
        register const char *s = nptr;
        register unsigned long acc;
        register int c;
        register unsigned long cutoff;
        register int neg = 0, any, cutlim;

        /*
         * Skip white space and pick up leading +/- sign if any.
         * If base is 0, allow 0x for hex and 0 for octal, else
         * assume decimal; if base is already 16, allow 0x.
         */
        do {
                c = *s++;
        } while (c == ' ' || c == '\t');
        if (c == '-') {
                neg = 1;
                c = *s++;
        } else if (c == '+')
                c = *s++;
        if ((base == 0 || base == 16) &&
            c == '0' && (*s == 'x' || *s == 'X')) {
                c = s[1];
                s += 2;
                base = 16;
        }
        if (base == 0)
                base = c == '0' ? 8 : 10;

        cutoff = neg ? -(unsigned long)1 : ULONG_MAX;
        cutlim = cutoff % (unsigned long)base;
        cutoff /= (unsigned long)base;
        for (acc = 0, any = 0;; c = *s++) {
                if (c >= '0' && c <= '9')
                        c -= '0';
                else if (c >= 'a' && c <= 'f')
                        c -= 'a' - 10;
                else if (c >= 'A' && c <= 'F')
                        c -= 'A' - 10;
                else
                        break;
                if (c >= base)
                        break;
                if (any < 0 || acc > cutoff || acc == cutoff && c > cutlim)
                        any = -1;
                else {
                        any = 1;
                        acc *= base;
                        acc += c;
                }
        }
        if (any < 0) {
                acc = neg ? -1 : ULONG_MAX;
        } else if (neg)
                acc = -acc;
        if (endptr != 0)
                *endptr = (char *)(any ? s - 1 : nptr);
        return (acc);
}

#ifdef REALLY_READ_FROM_FILES
/*
 * We implement all of this with the tools.  See osdep_make.c
 */
/*
 * ---------------------------------------------------------------------------
 * Message file
 */
char *
_udi_msgfile_search(
const char	*filename,
udi_ubit32_t	msgnum,
const char	*locale
)
{
	return NULL;
}

void
_udi_readfile_getdata (const char *filename, udi_size_t offset, char *buf,
			udi_size_t buf_len, udi_size_t *act_len)
{
}
#endif

/*
 * ---------------------------------------------------------------------------
 * Event code
 */
void
_udi_osdep_event_init(
_udi_osdep_event_t	*ev
)
{
	ev->event_state = 0;

	_OSDEP_MUTEX_INIT(&ev->event_lock, "event lock");
}

void
_udi_osdep_event_deinit(
_udi_osdep_event_t	*ev
)
{
	_OSDEP_MUTEX_DEINIT(&ev->event_lock);
}

void
_udi_osdep_event_signal(
_udi_osdep_event_t	*ev
)
{
	_OSDEP_MUTEX_LOCK(&ev->event_lock);

	ev->event_state = 1;

	wakeup((caddr_t)ev);

	_OSDEP_MUTEX_UNLOCK(&ev->event_lock);
}

void
_udi_osdep_event_wait(
_udi_osdep_event_t	*ev
)
{
	int	s2;

	_udi_poll_count = 0;

	_OSDEP_MUTEX_LOCK(&ev->event_lock);

	while (ev->event_state == 0)
	{
		if (_udi_init_time)
		{
			/*
			 * If we are at init time, then we do not have a
			 * context that we can sleep in.
			 */
			s2 = spl0();

			if (_udi_poll_count == 400000)
			{
#ifdef DEBUG_OSDEP
				udi_debug_printf("_udi_osdep_event_wait: "
					"timeout\n");

				calldebug();
#endif
				_udi_poll_count = 0;
			}

			_OSDEP_MUTEX_UNLOCK(&ev->event_lock);

			suspend(10);

			_OSDEP_MUTEX_LOCK(&ev->event_lock);

			_udi_poll_count++;

			splx(s2);

			continue;
		}

		_OSDEP_MUTEX_UNLOCK(&ev->event_lock);

		sleep((caddr_t)ev, PCATCH | PWAIT);

		_OSDEP_MUTEX_LOCK(&ev->event_lock);
	}

	ev->event_state = 0;

	_OSDEP_MUTEX_UNLOCK(&ev->event_lock);
}

/* #define TIMER_DEBUG */
#ifdef TIMER_DEBUG
struct timer_info {
	int		(*callback)();
	caddr_t		arg;
};
#endif


/*
 * ---------------------------------------------------------------------------
 * Timer code
 */

#ifdef TIMER_DEBUG
STATIC
void
udi_timer_callback(
struct timer_info	*tinfop
)
{
	int	s;

	asm("sti");
	s = spl1();

	(*tinfop->callback)(tinfop->arg);

	splx(s);

	_udi_kmem_free(tinfop);

}
#endif

_OSDEP_TIMER_T
udi_osdep_timer_start(
udi_timer_expired_call_t	*callback,
udi_cb_t			*gcb,
uint_t				ticks,
udi_boolean_t			flag
)
{
	if (_udi_interrupts_disabled)
	{
		suspend(((ticks * 1000) * (1000 / Hz)) + 1);

		(*callback)(gcb);

		return 0;
	}
#ifdef TIMER_DEBUG
	{
		struct timer_info	*tinfop;

		tinfop = (struct timer_info *)_udi_kmem_alloc(
			sizeof(struct timer_info), 0, 0);

		tinfop->callback = callback;
		tinfop->arg = (caddr_t)gcb;

		return timeout((int (*)())udi_timer_callback,
			(caddr_t)tinfop, ticks);
		
	}
#else
	return timeout((int (*)())callback, (caddr_t)gcb, ticks);
#endif
}

int
udi_osdep_timer_cancel(
udi_timer_expired_call_t	*callback,
udi_cb_t			*gcb,
_OSDEP_TIMER_T			timer_ID
)
{
	untimeout(timer_ID);
}

/*
 * ---------------------------------------------------------------------------
 * Printing and print buffer debug code
 */
int
_udi_osdep_printf(
const char	*format,
...
)
{
	va_list				arg;
	char				s[_UDI_MAX_DEBUG_PRINTF_LEN];
#ifdef DEBUG_OSDEP
	char				*s2;
	struct osdep_print_buffer	*printbp;
#endif

	va_start(arg, format);

	udi_vsnprintf((char *)s, _UDI_MAX_DEBUG_PRINTF_LEN, format, arg);

	va_end(arg);

	if (_osdep_udi_buffer_flag == 0)
	{
		return printf(s);
	}

#ifdef DEBUG_OSDEP
	_OSDEP_MUTEX_LOCK(&udi_print_lock);

	printbp = &_udi_osdep_print_buffer[0];

	s2 = s;

	while (*s2 != '\0')
	{
		printbp->print_buffer[printbp->print_offset] = *s2;

		printbp->print_offset++;

		if (printbp->print_offset == PRINT_BUF_SIZE)
		{
			printbp->print_wrap = 1;

			printbp->print_offset = 0;
		}

		s2++;
	}

	_OSDEP_MUTEX_UNLOCK(&udi_print_lock);
#endif
}


#ifdef DEBUG_OSDEP
STATIC
void
_osdep_udi_dump_chars(
char		*s1,
char		*s2,
int		*line,
int		*page
)
{
	char	*s;
	char	c;
	int	num_lines;

	num_lines = NUM_LINES;

	for (s = s1 ; s != s2 ; s++)
	{

		if (*s != '\n')
		{
			continue;
		}

		(*line)++;

		if (*line % num_lines)
		{
			continue;
		}

		printf("------------ page %d -------------", *page);

		c = getchar();

		printf("\r                                    \r");

		(*page)++;

		if ((c == ESC) || (c == 'q'))
		{
			printf("\n");

			return;
		}

		if (c == ' ')
		{
			num_lines = NUM_LINES;
		}
		else
		{
			num_lines = 1;
		}
	}
}

void
dump_buffer()
{
	int				line;
	int				page;
	struct osdep_print_buffer	*printbp;

	printbp = &_udi_osdep_print_buffer[0];

	line = 0;
	page = 1;

	if (printbp->print_wrap)
	{
		_osdep_udi_dump_chars(
			&printbp->print_buffer[printbp->print_offset],
			&printbp->print_buffer[PRINT_BUF_SIZE],
			&line, &page);

		if (line == -1)
			return;
	}

	_osdep_udi_dump_chars(
		&printbp->print_buffer[0],
		&printbp->print_buffer[printbp->print_offset],
		&line, &page);
}

void
dump_find_string(
char *match_string
)
{
	int				line;
	int				page;
	struct osdep_print_buffer	*printbp;
	char				*s;
	char				c;
	int				num_matched;
	int				len;
	char				*s1;
	char				*s2;

	printbp = &_udi_osdep_print_buffer[0];

	line = 0;
	page = 1;
	num_matched = 0;

	len = udi_strlen(match_string);

	if (printbp->print_wrap)
	{
		s1 = &printbp->print_buffer[printbp->print_offset];

		s2 = &printbp->print_buffer[PRINT_BUF_SIZE];

		for (s = s1 ; s != s2 ; s++)
		{
			if (num_matched == len)
			{
				printf("match page %d\n", page);

				num_matched = 0;
			}

			if (*s == *(match_string + num_matched))
			{
				num_matched++;
			}
			else
			{
				num_matched = 0;
			}

			if (*s != '\n')
			{
				continue;
			}

			line++;

			if (line % NUM_LINES)
			{
				continue;
			}

			page++;
		}
	}

	s1 = &printbp->print_buffer[0];

	s2 = &printbp->print_buffer[printbp->print_offset];

	for (s = s1 ; s != s2 ; s++)
	{
		if (num_matched == len)
		{
			printf("match page %d\n", page);

			num_matched = 0;
		}

		if (*s == *(match_string + num_matched))
		{
			num_matched++;
		}
		else
		{
			num_matched = 0;
		}

		if (*s != '\n')
		{
			continue;
		}

		line++;

		if (line % NUM_LINES)
		{
			continue;
		}

		page++;
	}
}
#endif /* DEBUG_OSDEP */

#ifdef _OSDEP_LOCK_DEBUG
STATIC
void
_udi_mutex_init(
struct _mutex	*mutexp,
char		*lock_name
)
{
	if (((unsigned)mutexp >= UVUBLK) && ((unsigned)mutexp <= UVUBEND))
	{
		calldebug();
	}

	mutexp->lock.ciflag = 0;
	mutexp->lock.ciowner = 0;
	mutexp->lock_count = 0;

	mutexp->max_lock_time = 0;
	mutexp->min_lock_time = 0;
	mutexp->total_lock_time = 0;
	mutexp->total_lock_time_ns = 0;

	mutexp->lock_start = 0;
	mutexp->lock_start_ns = 0;

	mutexp->lock_name = lock_name;

	mutexp->next = udi_mutex_head;

	udi_mutex_head = mutexp;
}

STATIC
void
_udi_mutex_deinit(
struct _mutex	*mutexp
)
{
	struct _mutex	*tmp_mutexp;
	struct _mutex	*last_mutexp;

	last_mutexp = NULL;

	for (tmp_mutexp = udi_mutex_head ; tmp_mutexp != NULL ;
		tmp_mutexp = tmp_mutexp->next)
	{
		if (tmp_mutexp == mutexp)
		{
			if (tmp_mutexp == udi_mutex_head)
			{
				udi_mutex_head = mutexp->next;
			}
			else
			{
				last_mutexp->next = mutexp->next;
			}

			mutexp->next = NULL;

			return;
		}

		last_mutexp = tmp_mutexp;
	}

	calldebug();
}

STATIC
void
_udi_mutex_lock(
struct _mutex	*mutexp
)
{
	int	s;

	mutexp->saved_pl = lockb(&mutexp->lock);

	if (udi_check_locks)
	{
		s = LOCK_CLOCK();

		get_clock_ns(&mutexp->lock_start, &mutexp->lock_start_ns);

		UNLOCK_CLOCK(s);
	}
}

STATIC
void
_udi_mutex_unlock(
struct _mutex	*mutexp
)
{
	unsigned	lock_end;
	unsigned	lock_end_ns;
	unsigned	lock_time;
	unsigned	lock_time_ns;
	int		s;

	if (udi_check_locks)
	{
		s = LOCK_CLOCK();

		get_clock_ns(&lock_end, &lock_end_ns);

		UNLOCK_CLOCK(s);

		if (mutexp->lock_start_ns > lock_end_ns)
		{
			lock_end_ns = lock_end_ns + 1000000000;
			lock_end = lock_end - 1;
		}

		lock_time = lock_end - mutexp->lock_start;
		lock_time_ns = lock_end_ns - mutexp->lock_start_ns;

		if (lock_time >= 1)
		{
			calldebug();
		}

		if (lock_time_ns > mutexp->max_lock_time)
		{
			mutexp->max_lock_time = lock_time_ns;
		}

		if ((mutexp->min_lock_time == 0) ||
			(lock_time_ns < mutexp->min_lock_time))
		{
			mutexp->min_lock_time  = lock_time_ns;
		}

		mutexp->lock_count++;

		mutexp->total_lock_time_ns += lock_time_ns;

		if (mutexp->total_lock_time_ns >= 1000000000)
		{
			mutexp->total_lock_time_ns -= 1000000000;

			mutexp->total_lock_time += 1;
		}
	}

	unlockb(&mutexp->lock, mutexp->saved_pl);
}

char	*udi_dump_lock_header = 
	"lock:     num      max      min      total(s:ns)       name";

STATIC
void
_udi_dump_locks(
)
{
	struct _mutex	*mutexp;
	unsigned	lines;
	char		c;

	lines = 0;

	printf("%s\n", udi_dump_lock_header);

	for (mutexp = udi_mutex_head ; mutexp != NULL ; mutexp = mutexp->next,
		lines++)
	{
		printf("%x: %x %x %x %x:%x %s\n",
			mutexp,
			mutexp->lock_count,
			mutexp->max_lock_time,
			mutexp->min_lock_time,
			mutexp->total_lock_time,
			mutexp->total_lock_time_ns,
			mutexp->lock_name);

		if (((lines + 1) % 8) == 0)
		{
			c = getchar();

			if ((c == ESC) || (c == 'q'))
			{
				return;
			}

			printf("%s\n", udi_dump_lock_header);
		}
	}
}
#endif /* _OSDEP_LOCK_DEBUG */

#ifdef DEBUG_ALLOC
STATIC
void
osdep_register_memory(
void *memptr
)
{
	unsigned	i;

	_osdep_num_alloc++;

	for (i = 0 ; i < MEMORY_INFO_TABLE_SIZE ; i++)
	{
		if (memory_info_table[i].memptr == NULL)
		{
			break;
		}
	}

	if (i == MEMORY_INFO_TABLE_SIZE)
	{
		return;
	}

	memory_info_table[i].memptr = memptr;

	osdep_save_stack((unsigned *)&memory_info_table[i].caller1, 3);
}

STATIC
void
osdep_unregister_memory(
void *memptr
)
{
	unsigned	i;
	unsigned	full;
	int		count;

	count = osdep_save_stack(&unregister_stack, UNREGISTER_STACK_SIZE);

#ifdef DEBUG_ALLOC_VERBOSE
	udi_debug_printf("unregister_stack: ");


	for (i = 0 ; i < count ; i++)
	{
		udi_debug_printf("0x%x ", unregister_stack[i]);
	}

	udi_debug_printf("\n");
#endif

	full = 1;

	for (i = 0 ; i < MEMORY_INFO_TABLE_SIZE ; i++)
	{
		if (memory_info_table[i].memptr == memptr)
		{
			memory_info_table[i].memptr = NULL;
			memory_info_table[i].caller1 = NULL;
			memory_info_table[i].caller2 = NULL;
			memory_info_table[i].caller3 = NULL;
			break;
		}
		else if (memory_info_table[i].memptr == NULL)
		{
			full = 0;
		}
	}

	if ((i == MEMORY_INFO_TABLE_SIZE) && !full)
	{
		calldebug();
	}

	_osdep_num_alloc--;
}

STATIC
unsigned int
is_good_ebp(ebp)
unsigned int ebp;
{
	if((ebp & 0x3) != 0)
		return(0);		/* not long word alined */
	if(ebp > (unsigned)&u.u_stack[KSTKSZ])
		return(0);		/* below the stack bounds */
	if(ebp < (unsigned)&u.u_stack[0])
		return(0);		/* above the stack bounds */
	return(1);
}

#define C_CALL_FRAME	0
#define TRAP_FRAME	1
#define BAD_FRAME	2

STATIC
frame_type(ebp)
unsigned int *ebp;
{
	if (((ebp[GS] & 0xFFFF) == 0)
			&& ((ebp[FS] & 0xFFFF) == 0)
			&& ((ebp[DS] & 0xFFFF) == KDSSEL)
			&& ((ebp[ES] & 0xFFFF) == KDSSEL) )
	{
		return TRAP_FRAME;
	}

	if (is_good_ebp(*ebp))
	{
		return C_CALL_FRAME;
	}

	return BAD_FRAME;
}

/*
 *  In a non-buggy kernel there are only two type of kernel stack frames:
 * C-call and interrupt.  This stack tracer would be confused easily
 * by a corrupted stack.  But it's default action when it gets confused
 * is to stop and return.  So it should be safe.
 */
STATIC
int
osdep_save_stack(
unsigned	*calladdr,
int		num
)
{
	register unsigned	*ebp;
	unsigned		*get_ebp();
	int			count;

	ebp = get_ebp();

	count = 0;

	while( count != num && is_good_ebp((unsigned int)ebp) )
	{
		switch(frame_type(ebp))
		{
		case C_CALL_FRAME:
			*calladdr = ebp[1];
			ebp = (unsigned *) *ebp;
			break;

		case TRAP_FRAME:
			*calladdr = ebp[EIP];
			ebp = (unsigned *) ebp[EBP];
			break;

		case BAD_FRAME:
			*calladdr = 0;
			return count;
		}

		count++;

		calladdr++;
	}

	return count;
}
#endif /* DEBUG_ALLOC */

#ifdef BTLD_DEBUG
STATIC
void
insert_symbols()
{
	unsigned		i;
	unsigned		j;
	struct old_sent		*xtable;
	struct old_sent		tmp_sent;
	unsigned char		*sym_name;
	unsigned		sort_offset;

	udi_debug_printf("Patching symbol table:\n");

	xtable = (struct old_sent *)((unsigned char *)db_symtable +
		sizeof(unsigned)) + db_nsymbols;

	for (i = 0 ; i < udi_num_symbols ; i++)
	{
		xtable->se_vaddr = (unsigned)udi_symbol_table[i].ptr;
		xtable->se_flags = SF_ANY;
		xtable->se_name = udi_symbol_table[i].name;

		xtable++;

		db_nsymbols++;

		if ((unsigned)xtable >= 
			((unsigned)db_symtable + db_symtablesize))
		{
			udi_debug_printf("Table overfow\n");
			break;
		}
	}

	sort_offset = db_nsymbols;

	for (j = 0 ; j < db_nsymbols ; j++)
	{
		xtable = (struct old_sent *)((unsigned char *)db_symtable +
			sizeof(unsigned));

		for (i = 0 ; i < sort_offset ; i++)
		{
			if (xtable->se_vaddr < (xtable+1)->se_vaddr)
			{
				tmp_sent = *xtable;
				*xtable = *(xtable + 1);
				*(xtable + 1) = tmp_sent;
			}
			xtable++;
		}

		sort_offset--;
	}
}
#pragma pack()
#endif /* BTLD_DEBUG */
