
/*
 * File: env/posix/posix.c
 *
 * Common Posix Environment code.
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
 * This file is the equivalent of the udi_osdep.c in the various osdep
 * pieces.   It's named differently becuase we actually do still have some
 * concept of osdependent components for things that fall outside of the
 * domain of POSIX 1003.2.
 */

#include <unistd.h>
#include <stdlib.h>
#include <setjmp.h>
#include <sys/time.h>
#include <sys/times.h>

#include "udi_env.h"

/*
 * These switches exist for conditional compilation, and should be
 * enabled globally in your osdep.mk file.
 *
 *   MALLOC_POSIX_TYPES to make key osdep data types be tracked by malloc.
 *   _POSIX_SEMAPHORES to enable posix semaphores instead of pthread mutexes
 *      for atomic int's.
 */

#define POSIX_PTH_RETURN_MAGIC ((void*)0xDEADD00D)

/* Alloc daemon thread data */
pthread_t alloc_daemon_thread;
pthread_attr_t alloc_daemon_attribute;
sem_t alloc_daemon_running;
STATIC int alloc_daemon_started;
STATIC _OSDEP_EVENT_T alloc_daemon_event;
extern _OSDEP_MUTEX_T _udi_alloc_lock;	/* deinited by _udi_alloc_deinit */
extern udi_queue_t _udi_alloc_list;	/* so we can watch the allocd activity */

/* Alloc daemon thread data */
pthread_t timer_daemon_thread;
pthread_attr_t timer_daemon_attribute;
STATIC int timer_daemon_started;
STATIC _OSDEP_EVENT_T timer_daemon_event;

/* Region daemon thread data */
pthread_t rgn_daemon_thread;
pthread_attr_t rgn_daemon_attribute;
sem_t rgn_daemon_running;
_OSDEP_EVENT_T rgn_daemon_event;



/* call in each new pthread & main, except timer */
STATIC void osdep_prepare_pthread(void);

STATIC void osdep_create_daemons(void);
void osdep_signal_alloc_daemon(void);
STATIC int daemons_alive;

STATIC void osdep_destroy_daemons(void);
STATIC int kill_alloc_daemon_req;
STATIC int kill_rgn_daemon_req;


/* Command line configured variables */
STATIC unsigned int alloc_thresh;
STATIC udi_ubit32_t allocs_artificially_failed;
STATIC udi_ubit32_t allocs_elgible_to_fail;

unsigned int callback_limit;
unsigned int posix_force_cb_realloc;
unsigned int posix_malloc_checks_set;
unsigned int posix_defer_mei_call;


/* Allocation log */
typedef struct {
	udi_queue_t Q;
	udi_ubit32_t numelems;
} _alloc_log_t;

STATIC int alloc_log_initted;
STATIC _alloc_log_t allocation_log;
STATIC pthread_mutex_t allocation_log_lock;
volatile size_t Allocd_mem = 0;
volatile size_t Allocd_locks = 0;

STATIC void osdep_dump_mempool(void);


STATIC void osdep_timer_init(void);
STATIC void osdep_timer_deinit(void);

FILE *_udi_log_file;
udi_trevent_t def_tracemask = 0xffffffff;
udi_trevent_t def_ml_tracemask = 0xffffffff;

char *posix_me;				/* Remember our executable for parsing it's static props */

#include <semaphore.h>
sem_t posix_loader_busy;

/*
 * Sleaze Alert:
 *	We arbitrarily load all the metas that are needed as there is no
 *	simple (i.e. quick) way to do this
 */

void
posix_init(int argc,
	   char **argv,
	   char *testargs,
	   void (*test_parse_func) (int c,
				    void *optarg))
{
	char *s, allargs[512];		/* should be big enough... */
	int c;
	udi_ubit32_t meta_trace;

	/*
	 * Global variable initialization 
	 */

	posix_me = argv[0];
	_udi_debug_level = 0;
	sem_init(&posix_loader_busy, 0, 1);	/* allocate acquired */

	/*
	 * Read general command-line arguments 
	 */

	meta_trace = 0;
#if 0
	optind = 0;			/* kludge: try 2 make getopt restart 4 each posix_init */
#endif
	udi_strcpy(allargs, "T:I:t:l:d:");
	if (testargs)
		udi_strcat(allargs, testargs);
	while ((c = getopt(argc, argv, allargs)) != EOF) {
		switch (c) {
		case 'd':
			_udi_debug_level = udi_strtou32(optarg, NULL, 0);
			break;
		case 't':
			_udi_trlog_initial = udi_strtou32(optarg, NULL, 0);
			break;
		case 'I':
			meta_trace = udi_strtou32(optarg, NULL, 0);
			break;
		case 'T':
			_udi_trlog_ml_initial[meta_trace] =
				udi_strtou32(optarg, NULL, 0);
			break;
		case 'l':
			_udi_current_locale(optarg);
			break;
		default:
			if (test_parse_func)
				(*test_parse_func) (c, optarg);
			break;
		case '?':
			printf("Usage: %s [-T N] [-I N] [-t N] [-d N] [-l str] %s\n" "  -T N = ML Trace mask for ML specified by -I\n" "  -I N = ML index for -T ML trace mask\n" "  -t N = Default trace mask\n" "  -d N = Debug level\n" "  -l str = locale specifier\n", posix_me, testargs);
			exit(0);
		}
	}

	/*
	 * Get other Configuration/Control Information 
	 */

	s = getenv("POSIX_CALLBACK_LIMIT");

	if (s) {
		callback_limit = atoi(s);
	} else {
		callback_limit = 20;
	}

	/*
	 * If nonzero, force all cb's to be reallocated on each channel op. 
	 */
	posix_force_cb_realloc = getenv("POSIX_FORCE_CB_REALLOC") != NULL;
	/*
	 * If nonnull, force all channel ops  to be queued. 
	 */
	posix_defer_mei_call = getenv("POSIX_DEFER_MEI_CALL") != NULL;

	/*
	 * Make Posix environment aware of MALLOC_CHECKS setting 
	 */
	posix_malloc_checks_set = getenv("MALLOC_CHECKS") != NULL;

	/*
	 * Use pthread initialization here since it's a catch-22 for malloc debugging 
	 */
	{
		static pthread_mutexattr_t mutexattr;

		pthread_mutexattr_init(&mutexattr);
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_NORMAL);
		pthread_mutex_init(&allocation_log_lock, &mutexattr);
	}
	osdep_timer_init();		/* spawn timer daemon */
	_udi_alloc_init();
	_udi_MA_init();

	/*
	 * Initialize the Allocation Daemon 
	 */
	osdep_create_daemons();		/* spawns alloc and region daemons */
	sem_post(&posix_loader_busy);
}

_OSDEP_MUTEX_T my_region_q_lock;
udi_queue_t my_region_q_head;

STATIC _OSDEP_EVENT_T posix_MA_deinited;
STATIC udi_boolean_t posix_MA_is_dead = FALSE;

STATIC void
posix_deinit_cbfn(void)
{
	/*
	 * Stop the system timers and then return control to the application
	 */
	posix_MA_is_dead = TRUE;
	_OSDEP_EVENT_SIGNAL(&posix_MA_deinited);
}

void
posix_deinit(void)
{
	sem_wait(&posix_loader_busy);
	_OSDEP_EVENT_INIT(&posix_MA_deinited);
	_udi_MA_deinit(posix_deinit_cbfn);
	while (posix_MA_is_dead == FALSE) {
		_OSDEP_EVENT_WAIT(&posix_MA_deinited);
	}
	_OSDEP_EVENT_DEINIT(&posix_MA_deinited);

	osdep_destroy_daemons();	/* destroy posix pthreads */
	_udi_alloc_deinit();		/* deinit udi_allocd & udi_timerd data structures. */

	assert(Allocd_mem >= 0);
	if (Allocd_mem > 0) {
		fprintf(stderr, "Total memory outstanding = %d bytes\n",
			Allocd_mem);
	}
	/*
	 * Dump the outstanding memory pool if POSIX_DUMP_MEMPOOL is set in
	 * the environment. Provides a means of tracking rogue memory
	 * allocations
	 */
	if (getenv("POSIX_DUMP_MEMPOOL") != NULL) {
		osdep_dump_mempool();
	}
	if (allocs_artificially_failed) {
		/*
		 * FIXME: this division might generate an internal expression
		 * * overflow. 
		 */
		fprintf(stdout, "Artifically failed %u of %u (%d%%) "
			"memory allocations.\n",
			allocs_artificially_failed,
			allocs_elgible_to_fail,
			100 * allocs_artificially_failed /
			allocs_elgible_to_fail);
		fflush(stdout);
	}

	pthread_mutex_destroy(&allocation_log_lock);	/* no more allocation log */
	sem_post(&posix_loader_busy);
	sem_destroy(&posix_loader_busy);
}

void
_udi_MA_osdep_init(void)
{
	extern udi_mei_init_t udi_mgmt_meta_info;

	/* 
	 * Set to all NULLs via partial initializer.
	 */
	_OSDEP_DRIVER_T driver_info = { NULL };
	driver_info.sprops = _udi_get_elf_sprops(posix_me, NULL, "udi");

	/*
	 * In this environment, the Management Metalanguage is directly
	 * linked into the environment module and must be "loaded" explicitly.
	 */
	(void)_udi_MA_meta_load("udi_mgmt", &udi_mgmt_meta_info, driver_info);
}


/* posix_dev_node_init
 *
 * This function performs the initialization of a new device node that
 * is common to all POSIX-based environments.  Should be called from
 * the OS-POSIX-specific _OSDEP_DEV_NODE_INIT operation.  Returns 0 on
 * success, non-zero on failure.
 */
int
posix_dev_node_init(_udi_driver_t *driver,	/* New UDI node */
		    _udi_dev_node_t *udi_node) /* Parent UDI node */
{
	_OSDEP_DEV_NODE_T *osinfo = &udi_node->node_osinfo;
	char bus_type[33];
	udi_size_t length;
	udi_instance_attr_type_t attr_type;

	if (osinfo->posix_dev_node.backingstore == NULL) {
		/* first time initialization */
		osinfo->posix_dev_node.backingstore = tmpfile();
		if (!osinfo->posix_dev_node.backingstore) {
			perror("tmpfile() failed.");
		}
		udi_assert(osinfo->posix_dev_node.backingstore);
	}

        length = _udi_instance_attr_get(udi_node, "bus_type", bus_type,
                                        sizeof (bus_type), &attr_type);         
	if (length) {
		if (strncmp(bus_type, "pci", length) == 0) {
			osinfo->posix_dev_node.bus_type = bt_pci;
		} else if (strncmp(bus_type, "system", length) == 0) {
			osinfo->posix_dev_node.bus_type = bt_system;
			_udi_instance_attr_get(udi_node, "sysbus_mem_addr_lo",
				&osinfo->posix_dev_node.sysbus_data.sysbus_mem_addr_lo,
				4, &attr_type);
			_udi_instance_attr_get(udi_node, "sysbus_mem_addr_hi",
				&osinfo->posix_dev_node.sysbus_data.sysbus_mem_addr_hi,
				4, &attr_type);
			_udi_instance_attr_get(udi_node, "sysbus_io_addr_lo",
				&osinfo->posix_dev_node.sysbus_data.sysbus_io_addr_lo,
				4, &attr_type);
			_udi_instance_attr_get(udi_node, "sysbus_io_addr_hi",
				&osinfo->posix_dev_node.sysbus_data.sysbus_io_addr_hi,
				4, &attr_type);
		} 
	}
// printf("%d/%d, %s/%x\n", length, osinfo->posix_dev_node.bus_type, bus_type, osinfo->posix_dev_node.sysbus_data.sysbus_io_addr_lo);

	return 0;
}


/* posix_dev_node_deinit
 *
 * This function performs the initialization of a new device node that
 * is common to all POSIX-based environments.  Should be called from
 * the OS-POSIX-specific _OSDEP_DEV_NODE_DEINIT operation.  Returns 0 on
 * success, non-zero on failure.
 */
int
posix_dev_node_deinit(_udi_dev_node_t *udi_node) /* Parent UDI node */
{
	_OSDEP_DEV_NODE_T *osinfo = &udi_node->node_osinfo;

	osinfo->posix_dev_node.backingstore = tmpfile();
	udi_assert(osinfo->posix_dev_node.backingstore);
	if (osinfo->posix_dev_node.backingstore) {
		/* first time initialization */
		fclose(osinfo->posix_dev_node.backingstore);
		osinfo->posix_dev_node.backingstore = NULL;
	}
	return 0;
}


/*
 * Simple memory allocation daemon.
 * Sits in a thread of its own, looping for work to do and doing it.
 * When work is added to the list, this thread awakens.
 * The "hard" part of this exercise is done by _udi_alloc_daemon_work()
 * which is common code.   This is just a wrapper to call that.
 */

void *
_udi_alloc_daemon(void *dummy)
{
	osdep_prepare_pthread();
	sem_post(&alloc_daemon_running);

	for (;;) {
		/*
		 * alloc_daemon_work() will acquire and release locks
		 * on the allocation queue as needed
		 */

		/*
		 * As long as there's work in the queue, do it without
		 * * releasing and regrabbing our locks .
		 */
		while (_udi_alloc_daemon_work());

		_OSDEP_EVENT_WAIT(&alloc_daemon_event);

		pthread_testcancel();
		if (kill_alloc_daemon_req) {
			pthread_exit(POSIX_PTH_RETURN_MAGIC);
			return POSIX_PTH_RETURN_MAGIC;
		}
	}
}

void *
_udi_rgn_daemon(void *dummy)
{
	udi_queue_t *elem;
	_udi_region_t *rgn;

	osdep_prepare_pthread();

	sem_post(&rgn_daemon_running);

	for (;;) {
		_OSDEP_EVENT_WAIT(&rgn_daemon_event);

		_OSDEP_MUTEX_LOCK(&my_region_q_lock);
		while (!UDI_QUEUE_EMPTY(&my_region_q_head)) {
			elem = UDI_DEQUEUE_HEAD(&my_region_q_head);
			rgn = UDI_BASE_STRUCT(elem, _udi_region_t,
					      reg_sched_link);

			rgn->in_sched_q = FALSE;
			_OSDEP_MUTEX_UNLOCK(&my_region_q_lock);
			_udi_run_inactive_region(rgn);
			_OSDEP_MUTEX_LOCK(&my_region_q_lock);
		}
		_OSDEP_MUTEX_UNLOCK(&my_region_q_lock);

		pthread_testcancel();
		if (kill_rgn_daemon_req) {
			printf("Rgn daemon exiting\n");
			pthread_exit(POSIX_PTH_RETURN_MAGIC);
			return POSIX_PTH_RETURN_MAGIC;
		}
	}
}

/*
 * Called by posix_deinit() or other facility, this turns the allocation 
 * daemon off in a nice way.
 */
STATIC void
osdep_destroy_daemons(void)
{
	int rv;
	void *pth_ret;

	/*
	 * Don't bother cleaning up if they aren't alive. 
	 */
	if (!daemons_alive) {
		fprintf(stderr, "osdep_destroy_daemons needs daemons_alive\n");
		return;
	}

	kill_rgn_daemon_req = 1;
	_OSDEP_EVENT_SIGNAL(&rgn_daemon_event);

	/*
	 * Attempt to join, then cancel it... all else fails, send a kill. 
	 */
	rv = pthread_join(rgn_daemon_thread, &pth_ret);
	if (pth_ret != POSIX_PTH_RETURN_MAGIC) {
		fprintf(stderr, "join(rgn_daemon) returned bad magic 0x%p.\n",
			pth_ret);
	}

	rv = pthread_cancel(rgn_daemon_thread);

	kill_alloc_daemon_req = 1;
	_OSDEP_EVENT_SIGNAL(&alloc_daemon_event);
	rv = pthread_join(alloc_daemon_thread, &pth_ret);
	if (pth_ret != POSIX_PTH_RETURN_MAGIC) {
		fprintf(stderr,
			"join(alloc_daemon) returned bad magic 0x%p.\n",
			pth_ret);
	}

	rv = pthread_cancel(alloc_daemon_thread);
	(void)rv;

	osdep_timer_deinit();

	/*
	 * No more threads. Delete variables allocated in osdep_create_daemons 
	 */
	/*
	 * Delete allocd data 
	 */
	pthread_attr_destroy(&alloc_daemon_attribute);
	_OSDEP_EVENT_DEINIT(&alloc_daemon_event);

	/*
	 * Delete region daemon data 
	 */
	pthread_attr_destroy(&rgn_daemon_attribute);
	_OSDEP_MUTEX_DEINIT(&my_region_q_lock);
	_OSDEP_EVENT_DEINIT(&rgn_daemon_event);

	kill_alloc_daemon_req = 0;
	kill_rgn_daemon_req = 0;

	daemons_alive = 0;
}


/* 
 * Spawn a new daemon if necessary, otherwise awaken the 
 * current one.   This is called every time a memory request
 * is added to the memory allocation daemon's work list.
 */

void
osdep_signal_alloc_daemon(void)
{
	/*
	 * Awaken the daemon. Lock ugliness is in the macros. 
	 */
	_OSDEP_EVENT_SIGNAL(&alloc_daemon_event);
}

void
osdep_create_daemons(void)
{
	char *thresh_string;
	int rv;				/* temporary return value */

	alloc_daemon_started++;

	/*
	 * Configure the "main" thread signal mask. 
	 */
	osdep_prepare_pthread();

	/*
	 * Initialize various pthread components. 
	 */
	_OSDEP_EVENT_INIT(&alloc_daemon_event);
	sem_init(&alloc_daemon_running, 0, 0);
	pthread_attr_init(&alloc_daemon_attribute);
	pthread_attr_setdetachstate(&alloc_daemon_attribute,
				    PTHREAD_CREATE_JOINABLE);

	_OSDEP_EVENT_INIT(&rgn_daemon_event);
	_OSDEP_MUTEX_INIT(&my_region_q_lock, "my_region_q_lock");
	UDI_QUEUE_INIT(&my_region_q_head);
	sem_init(&rgn_daemon_running, 0, 0);
	pthread_attr_init(&rgn_daemon_attribute);
	pthread_attr_setdetachstate(&rgn_daemon_attribute,
				    PTHREAD_CREATE_JOINABLE);

	/*
	 * Tell threads that we really do want simulataneous 
	 * scheduling. We want to be globally scheduled, not
	 * just as part of the timeslice of the main pthread/process
	 * that's creating these threads.
	 * 
	 * Bound threads may not be available on all platforms so we
	 * ignore returns of EOPNOTSUPP.
	 */
	rv = pthread_attr_setscope(&alloc_daemon_attribute,
				   PTHREAD_SCOPE_SYSTEM);
	if (rv != 0 && rv != EOPNOTSUPP) {
		fprintf(stderr, "pthread_attr_setscope failed: %s\n", 
				strerror(errno));
		abort();
	}

	/*
	 * And let the daemon begin! 
	 */

	rv = pthread_create(&alloc_daemon_thread,
			    &alloc_daemon_attribute, _udi_alloc_daemon, NULL);
	assert(0 == rv);

	/*
	 * We'll start the region daemon here, too. 
	 */
	rv = pthread_create(&rgn_daemon_thread,
			    &rgn_daemon_attribute, _udi_rgn_daemon, NULL);
	assert(0 == rv);

	/*
	 * Determine what percentage of the allocs that we can
	 * * "safely" fail we will.  The env variable is multiplied
	 * * by RAND_MAX so we don't incur a divide in the allocator.
	 */

	thresh_string = getenv("POSIX_FAIL_MEM");
	if (thresh_string != NULL) {
		alloc_thresh = atoi(thresh_string) * (RAND_MAX / 100);
	}

	/*
	 * Do not proceed until the daemons have actually started 
	 */
	sem_wait(&alloc_daemon_running);
	sem_wait(&rgn_daemon_running);
	sem_destroy(&alloc_daemon_running);
	sem_destroy(&rgn_daemon_running);
	daemons_alive = 1;
}


#if 0

/*
 * printf is taboo in UDI common code. It's fine for POSIX land and POSIX
 * tests. In common code, use: udi_log_write, _udi_env_log_write, or
 * udi_trace_write.
 */

/* 
 *  We wrap printf mostly to prove that we can to improve the code
 *  coverage of udi_vsnprintf() and friends.
 *  FIXME:
 *  This is deficient : formatted buffers longer than pbuf are
 *  truncated.
 */
int
printf(const char *fmt,
       ...)
{
	va_list arg;
	char pbuf[1024];
	int len;

	va_start(arg, fmt);
#if 0
	len = udi_vsnprintf(pbuf, sizeof (pbuf), fmt, arg);
	len = (len >= sizeof (pbuf) ? (sizeof (pbuf) - 1) : len);
	pbuf[len] = 0;
	fputs(pbuf, stdout);
#else
	len = vprintf(fmt, arg);
#endif
	va_end(arg);
	fflush(stdout);
	return len;
}
#endif

/*
 * Support for udi_assert() macro.
 */
void
__udi_assert(const char *e,
	     const char *file,
	     int line)
{
	printf("UDI Assertion Failure at %s:%u: %s\n", file, line, e);
	fflush(stdout);
	abort();
}

void
posix_add_to_alloc_log(_mblk_header_t *mem)
{
	if (alloc_log_initted == 0) {
		alloc_log_initted++;
		UDI_QUEUE_INIT(&allocation_log.Q);
		allocation_log.numelems = 0;
	}

	pthread_mutex_lock(&allocation_log_lock);
	assert(Allocd_mem >= 0);
	Allocd_mem += mem->size;
	UDI_QUEUE_INIT(&mem->mblk_qlink);
	_POSIX_OSDEP_INIT_MBLK(mem);
	UDI_ENQUEUE_TAIL(&allocation_log.Q, &mem->mblk_qlink);
	allocation_log.numelems++;
	pthread_mutex_unlock(&allocation_log_lock);
}

void
posix_delete_from_alloc_log(_mblk_header_t *mem)
{
	pthread_mutex_lock(&allocation_log_lock);
	_POSIX_OSDEP_DEINIT_MBLK(mem);
	udi_dequeue(&mem->mblk_qlink);
	allocation_log.numelems--;
	Allocd_mem -= mem->size;
	assert(Allocd_mem >= 0);
	pthread_mutex_unlock(&allocation_log_lock);
}

/* 
 * A unique word we can write into the mblk_header to be reasonably
 * sure that we're looking at a valid buffer.
 */
STATIC const unsigned int allocator_signature = 0xdeadf00d;
STATIC const unsigned int allocator_freed = 0xdeadfeed;



void *
osdep_malloc(size_t size,
	     int flags,
	     int sleep_ok,
	     int line,
	     char *fname)
{
	char *dbuf;
	_mblk_header_t *mem;

	_OSDEP_ASSERT(size > 0);

	if (sleep_ok == UDI_NOWAIT) {
		allocs_elgible_to_fail++;
		if (_OSDEP_INSTRUMENT_ALLOC_FAIL) {
			allocs_artificially_failed++;
			return 0;
		}
	}

	mem = (_mblk_header_t *)malloc(size + sizeof (_mblk_header_t));
	if (!mem) {
		return 0;
	}
	dbuf = (char *)((char *)mem + sizeof (_mblk_header_t));
	mem->size = size;
	mem->allocator_linenum = line;
	mem->allocator_fname = fname;
	mem->signature = allocator_signature;

	if (!(flags & UDI_MEM_NOZERO)) {
		memset(dbuf, 0x00, size);
	} else {
		/*
		 * For debugging only 
		 */
		memset(dbuf, 0xCD, size);
	}

	posix_add_to_alloc_log(mem);

	return dbuf;
}

void
osdep_free(void *buf)
{
	udi_queue_t *e, *t;
	_mblk_header_t *mem, oldmem;
	char *saddr, *eaddr;

	/*
	 * Freeing a null pointer is never good.   If you don't believe
	 * * this to be true, just wait until we start walking backwards
	 * * from that address...
	 */

	_OSDEP_ASSERT(buf != NULL);

	mem = (_mblk_header_t *)((char *)buf - sizeof (_mblk_header_t));

	/*
	 *  Prove that we've called the deallocator with something that
	 *  we actually allocated.  Then we rip up the signature and 
	 *  stomp on it.
	 *  Additionally, we check that we haven't already freed it.
	 */

	oldmem = *mem;

	_OSDEP_ASSERT(mem->signature != allocator_freed);
	_OSDEP_ASSERT(mem->signature == allocator_signature);
	mem->signature = allocator_freed;

	posix_delete_from_alloc_log(mem);

	/*
	 * Pathological test to make sure we haven't blown away some still
	 * used data. Only do this when running with MALLOC_CHECKS set as the
	 * scanning of the list will _really_ slow things down.
	 */
	if (posix_malloc_checks_set) {
		saddr = (char *)mem;
		eaddr = saddr + mem->size;
		pthread_mutex_lock(&allocation_log_lock);
		UDI_QUEUE_FOREACH(&allocation_log.Q, e, t) {
			_OSDEP_ASSERT(!((char *)(e->next) >= saddr &&
					(char *)(e->next) <= eaddr));
			_OSDEP_ASSERT(!((char *)(e->prev) >= saddr &&
					(char *)(e->prev) <= eaddr));
		}
		pthread_mutex_unlock(&allocation_log_lock);
	}
	free(mem);
}

/* 
 * Provide a way to visually identify a memory buffer.   Probably should
 * only be called via debugger.
 */
void
osdep_memid(void *buf)
{
	_mblk_header_t *mem = (_mblk_header_t *)((char *)buf -
						 sizeof (_mblk_header_t));
	printf("Block is %d (0x%x) bytes long and was allocated by %s:%d\n",
	       mem->size, mem->size,
	       mem->allocator_fname, mem->allocator_linenum);
}

STATIC void
osdep_dump_mempool(void)
{
	udi_queue_t *listp = &allocation_log.Q;
	udi_queue_t *e, *t;
	unsigned sz = 0;

	UDI_QUEUE_FOREACH(listp, e, t) {
		_mblk_header_t *mblk = (_mblk_header_t *)e;

		sz += mblk->size;
		printf("Allocated from %s:%d. size is %d(0x%x)/%p\n",
		       mblk->allocator_fname,
		       mblk->allocator_linenum,
		       mblk->size,
		       mblk->size, (char *)mblk + sizeof (_mblk_header_t));
		_POSIX_OSDEP_DUMP_MBLK(mblk);
	}
	printf("Total allocated memory: %d(0x%x) bytes\n", sz, sz);
}

/*
 * The following is an attempt at providing a poor man's callout table based
 * timer implementation.
 * There'll be one SIGALRM based handler which just peruses the list of queued
 * timeouts and if it finds one that has expired, will call the associated
 * callback with the argument provided.
 * The period of the callback is as provided by the 
 * handler associated with the osdep_timer_start() callback.
 */

typedef struct {
	udi_queue_t Q;
	udi_timestamp_t submit;
	udi_time_t interval;
	udi_boolean_t repeat;
	void (*callback) (void *);
	void *arg;
	udi_boolean_t active;
} _udi_time_el_t;

typedef struct {
	udi_queue_t Q;
	_OSDEP_MUTEX_T lock;
} _udi_time_head_t;

_udi_time_head_t osdep_callout;		/* Active timer requests */
udi_queue_t osdep_expired_callout;	/* Expired timer requests */

/*
 * The core timeout handler. 
 * Scans the timeout list and executes any callback that has expired. If the
 * callback is for a repeating timer it is scheduled for curr_time + interval
 */
void
osdep_timer_handler(int sig)
{
	udi_queue_t *elem, *tmp;
	_udi_time_el_t *timel;
	udi_time_t diff;
	void (*cbfn) (void *);
	void *cbarg;
	udi_timestamp_t now;

	UDI_QUEUE_FOREACH(&osdep_callout.Q, elem, tmp) {
		/*
		 * Lock the queue to stop modifications from happening while
		 * we're processing this entry
		 */
		_OSDEP_MUTEX_LOCK(&osdep_callout.lock);
		timel = UDI_BASE_STRUCT(elem, _udi_time_el_t, Q);

		/*
		 * The following performs the same function as
		 *    diff = udi_time_since(timel->submit);
		 * which we cannot use as this requires the whole UDI env. to be
		 * present and sptest can't handle this. Sigh.
		 */
		_OSDEP_GET_CURRENT_TIME(now);
		_OSDEP_SUBTRACT_TIME(now, timel->submit, diff);
		if (((diff.seconds > timel->interval.seconds) ||
		     (((diff.seconds == timel->interval.seconds) &&
		       (diff.nanoseconds >= timel->interval.nanoseconds)) &&
		      (!timel->active)))) {
			UDI_QUEUE_REMOVE(elem);
			timel->active = TRUE;
#if 0
			printf("osdep_timer_handler: D.secs %d, D.nanosecs %d, Interval %d.%d\n", diff.seconds, diff.nanoseconds, timel->interval.seconds, timel->interval.nanoseconds);
#endif
			cbfn = timel->callback;
			cbarg = timel->arg;
			if (timel->repeat) {
				/*
				 * Enqueue repeat request
				 */
				_OSDEP_GET_CURRENT_TIME(timel->submit);
				UDI_ENQUEUE_TAIL(&osdep_callout.Q, elem);
			} else {
				UDI_ENQUEUE_TAIL(&osdep_expired_callout, elem);
			}
			/*
			 * Release lock before executing callback 
			 */
			_OSDEP_MUTEX_UNLOCK(&osdep_callout.lock);
			timel->active = FALSE;
			(*cbfn) (cbarg);
		} else {
			_OSDEP_MUTEX_UNLOCK(&osdep_callout.lock);
		}
	}
}

STATIC void *
osdep_timer_daemon(void *dummy)
{
	struct itimerval new;
	struct timeval wait;
	struct sigaction sigact;
	int rv;
	udi_queue_t *elem, *tmp;

	sigact.sa_flags = 0;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_handler = osdep_timer_handler;
	sigaction(SIGALRM, &sigact, NULL);

	timer_daemon_started = 1;

	wait.tv_sec = 0;
	wait.tv_usec = USEC_PER_SEC / POSIX_TIMER_HZ;	/* N ticks per second */

	/*
	 * If it_interval is zero, it's a nonrepeating timer 
	 */
	new.it_interval = wait;
	new.it_value = wait;

	rv = setitimer(ITIMER_REAL, &new, (struct itimerval *)NULL);

	assert(0 == rv);

	/*
	 * Block until we're signalled to quit 
	 */
	_OSDEP_EVENT_WAIT(&timer_daemon_event);

	/*
	 * Exit quickly if we were cancelled. 
	 */
	pthread_testcancel();

	/*
	 * Now disable the interval timer in our thread. 
	 */
	wait.tv_sec = wait.tv_usec = 0;
	new.it_interval = new.it_value = wait;
	setitimer(ITIMER_REAL, &new, (struct itimerval *)NULL);

	/*
	 * Release the allocated memory from the active and expired chains
	 */
	UDI_QUEUE_FOREACH(&osdep_callout.Q, elem, tmp) {
		UDI_QUEUE_REMOVE(elem);
		_OSDEP_MEM_FREE(elem);
	}

	UDI_QUEUE_FOREACH(&osdep_expired_callout, elem, tmp) {
		UDI_QUEUE_REMOVE(elem);
		_OSDEP_MEM_FREE(elem);
	}

	timer_daemon_started = 0;
	pthread_exit(POSIX_PTH_RETURN_MAGIC);
	return POSIX_PTH_RETURN_MAGIC;
}

/*
 * Initialise the timer callout mechanism
 */
STATIC void
osdep_timer_init(void)
{
	UDI_QUEUE_INIT(&osdep_callout.Q);
	UDI_QUEUE_INIT(&osdep_expired_callout);
	_OSDEP_EVENT_INIT(&timer_daemon_event);
	_OSDEP_MUTEX_INIT(&osdep_callout.lock, "Timer Callout Lock");

	/*
	 * Spawn our timeout thread 
	 */
	pthread_attr_init(&timer_daemon_attribute);
	pthread_attr_setdetachstate(&timer_daemon_attribute,
				    PTHREAD_CREATE_JOINABLE);

	pthread_create(&timer_daemon_thread, &timer_daemon_attribute,
		       osdep_timer_daemon, NULL);

}

STATIC void
osdep_prepare_pthread(void)
{
#if !defined(PTHREAD_SIGMASK_UNAVAILABLE)
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	pthread_sigmask(SIG_BLOCK, &sigset, NULL);
#else
/*
 * #warning "sleep and usleep may be interrupted by setitimer!"
 */
#endif
}

_OSDEP_TIMER_T
osdep_timer_start(void (*callback),
		  void *arg,
		  udi_ubit32_t interval,
		  udi_boolean_t repeats)
{
	_udi_time_el_t *timel;

	timel = _OSDEP_MEM_ALLOC(sizeof (_udi_time_el_t), 0, UDI_WAITOK);
	timel->callback = (void (*)())callback;
	timel->arg = arg;
	timel->repeat = repeats;
	timel->interval.seconds = interval / POSIX_TIMER_HZ;
	timel->interval.nanoseconds = (interval % POSIX_TIMER_HZ) *
		(NSEC_PER_SEC / POSIX_TIMER_HZ);
	_OSDEP_GET_CURRENT_TIME(timel->submit);
	_OSDEP_MUTEX_LOCK(&osdep_callout.lock);
	UDI_ENQUEUE_TAIL(&osdep_callout.Q, (udi_queue_t *)timel);
	_OSDEP_MUTEX_UNLOCK(&osdep_callout.lock);

	return (_OSDEP_TIMER_T) timel;
}

/*
 * Simply destroy the timer referenced by timer_id. As the timer _may_ be
 * being processed when this routine is called, we simply dequeue it (from
 * whichever queue it is currently on and then release the memory
 * Note: We have to acquire the osdep_callout lock as it _may_ be an active
 *	 repeating timer that we're trying to cancel, or it could be one that
 *	 has yet to fire. The worst case is that we take an unnecessary lock on
 *	 the queue.
 */
void
osdep_timer_cancel(void (*callback) (),
		   void *arg,
		   _OSDEP_TIMER_T timer_id)
{
	_udi_time_el_t *timel = (_udi_time_el_t *) timer_id;

	_OSDEP_MUTEX_LOCK(&osdep_callout.lock);
	UDI_QUEUE_REMOVE((udi_queue_t *)timel);
	_OSDEP_MUTEX_UNLOCK(&osdep_callout.lock);
	_OSDEP_MEM_FREE(timel);

}

/*
 * Stop the system timeout chain. Release all timers which are still pending
 * or have been serviced earlier
 */
STATIC void
osdep_timer_deinit(void)
{
	void *th_return;

	if (!timer_daemon_started) {
		return;
	}
	/*
	 * Kill the timer daemon thread
	 */
	_OSDEP_EVENT_SIGNAL(&timer_daemon_event);
	pthread_join(timer_daemon_thread, &th_return);
	assert(th_return == POSIX_PTH_RETURN_MAGIC ||
	       th_return == (void *)PTHREAD_CANCELED);
	_OSDEP_EVENT_DEINIT(&timer_daemon_event);
	_OSDEP_MUTEX_DEINIT(&osdep_callout.lock);
	pthread_attr_destroy(&timer_daemon_attribute);
}

/*
 * We can set the clock granularity pretty fine, but some POSIX OSes
 * like Linux and UnixWare only support a 10ms granularity from
 * gettimeofday().  Simple heuristic to determine how accurate it is.
 * XXX: Make it even simpler?  Single delta probably enough in practice...
 * or we could just report the _OSDEP_CLK_TCK resolution... that just
 * requires UW-posix to override the _OSDEP_CLK_TCK #define (with 100)
 * So while it was fun to play with the code, it probably isn't
 * really needed... be sure to adjust/remove the _OSDEP_CLK_TCK
 * for linux-user as well to get full advantage.
 */
unsigned int
guess_clock_res(void)
{
	struct timeval tv1, tv2, tv3;
	udi_ubit32_t usec1, usec2;
	udi_ubit32_t fudge;
	static udi_ubit32_t nsec = 0;

	if (nsec)
		return nsec;

	/*
	 * warm cache 
	 */
	gettimeofday(&tv1, NULL);

	/*
	 * We get two delta samples in case we're preempted between one set 
	 */
	gettimeofday(&tv1, NULL);
	do {
		gettimeofday(&tv2, NULL);
	} while (tv1.tv_usec == tv2.tv_usec);
	do {
		gettimeofday(&tv3, NULL);
	} while (tv2.tv_usec == tv3.tv_usec);

	usec1 = tv2.tv_usec - tv1.tv_usec;
	if (usec1 < 0)
		usec1 += USEC_PER_SEC;
	usec2 = tv3.tv_usec - tv2.tv_usec;
	if (usec2 < 0)
		usec2 += USEC_PER_SEC;

	/*
	 * Pick the smaller number 
	 */
	usec1 = (usec1 < usec2) ? usec1 : usec2;

	/*
	 * Make sure we can report this granularity 
	 */
	fudge = USEC_PER_SEC / _OSDEP_CLK_TCK;

	/*
	 * Round up to _OSDEP_CLK_TCK intervals.
	 * Can't mask because fudge not Po2 
	 */
	usec1 = ((usec1 + fudge - 1) / fudge) * fudge;
	printf("Wall clock granularity is 0.%06d seconds (%d Hz)\n", 
		usec1, USEC_PER_SEC / usec1);

	printf("Timer granularity is 0.%06d seconds (%d Hz)\n",
	       USEC_PER_SEC / POSIX_TIMER_HZ, POSIX_TIMER_HZ);

	nsec  = usec1 * 1000;
	return nsec;
}

void
posix_get_limit_t(udi_limits_t *limits)
{
	/*
	 * Most of these are, for the POSIX case, relatively arbitrary
	 * limits.   They're initialized in this way (instead of via a
	 * copy of a static buffer) to show how an environment might choose
	 * to tweak these at runtime.   For example, an OS might choose to
	 * change the allocation limits based on available host memory
	 * sizes.
	 * In general, these are intentionally picked low to be obnoxious. :-)
	 */

	limits->max_legal_alloc = UDI_MIN_ALLOC_LIMIT;
	limits->max_safe_alloc = UDI_MIN_ALLOC_LIMIT;
	limits->max_trace_log_formatted_len = _OSDEP_TRLOG_LIMIT;
	limits->max_instance_attr_len = UDI_MIN_INSTANCE_ATTR_LIMIT;
	limits->min_curtime_res = guess_clock_res();
	limits->min_timer_res = NSEC_PER_SEC / POSIX_TIMER_HZ;
}

udi_ubit32_t
udi_strtou32(const char *s,
	     char **endptr,
	     int base)
{
	return strtoul(s, endptr, base);
}

STATIC void
_invalidate_attr(_udi_dev_node_t *node,
		 const char *attr_name)
{

	long attr_start = 0;
	int retval, valid, type, name_len, val_len;
	FILE *file = node->node_osinfo.posix_dev_node.backingstore;
	char name[UDI_MAX_ATTR_NAMELEN];

	rewind(file);
	retval = fscanf(file, "%d|%d|%d|%d|", &valid, &type, &name_len,
			&val_len);
	while (retval != EOF && retval == 4) {
		if (fgets(name, name_len + 1, file) == NULL)	/* fgets stops at n-1 */
			udi_assert(0);
		if (valid && strncmp(name, attr_name, name_len) == 0) {
			fseek(file, attr_start, SEEK_SET);
			fputc('0', file);
			fflush(file);
			return;
		} else {
			fseek(file,
			      ftell(file) + 1 + name_len + 1 + val_len + 1,
			      SEEK_CUR);
			attr_start = ftell(file);
			retval = fscanf(file, "%d|%d|%d|%d|", &valid, &type,
					&name_len, &val_len);
		}
	}
}

/* information will be stored in file as follows
valid|type|name_len|name|val_len|val<cr>
*/
udi_status_t
__osdep_instance_attr_set(_udi_dev_node_t *node,
			  const char *name,
			  const void *value,
			  udi_size_t length,
			  udi_ubit8_t attr_type)
{

	udi_ubit32_t val;
	udi_ubit8_t type;
	FILE *file = node->node_osinfo.posix_dev_node.backingstore;
	int i;

	if (__osdep_instance_attr_get(node, name, (void *)&val, 0, &type) > 0)
		_invalidate_attr(node, name);
	if (attr_type == UDI_ATTR_UBIT32)
		length = 8;
	else if (attr_type == UDI_ATTR_BOOLEAN)
		length = 1;

	fseek(file, 0, SEEK_END);
	fprintf(file, "%d|%d|%ld|%ld|%s|", 1, attr_type, 
	        (signed long) udi_strlen(name),
		(signed long) length, name);

	switch (attr_type) {
	case UDI_ATTR_STRING:
	case UDI_ATTR_ARRAY8:
		for (i = 0; i < length; i++)
			fputc(*((char *)value + 1), file);
		break;
	case UDI_ATTR_BOOLEAN:
		if (*(udi_boolean_t *)value == TRUE)
			fputc('T', file);
		else
			fputc('F', file);
		break;
	case UDI_ATTR_UBIT32:
		fprintf(file, "%x", *(udi_ubit32_t *)value);
		break;
	}
	fputc('\n', file);
	return UDI_OK;
}

int
_get_val(char *val,
	 int length,
	 FILE * stream)
{
	int i;

	for (i = 0; i < length; i++) {
		int fchar = fgetc(stream);

		if (fchar == EOF)
			return EOF;
		val[i] = (char)fchar;
	}
	fgetc(stream);			/* the <cr> */
	return i;
}

udi_size_t
__osdep_instance_attr_get(_udi_dev_node_t *node,
			  const char *attr_name,
			  void *value,
			  udi_size_t length,
			  udi_ubit8_t *attr_type)
{

	FILE *file = node->node_osinfo.posix_dev_node.backingstore;
	int valid, name_len, val_len;
	udi_ubit8_t type;
	int rtype;
	char name[UDI_MAX_ATTR_NAMELEN], *val;
	int retval;

	rewind(file);
	retval = fscanf(file, "%d|%d|%d|%d|", &valid, &rtype,
			&name_len, &val_len);
	type = (udi_ubit8_t)rtype;
	while (retval != EOF && retval == 4) {
		if (fgets(name, name_len + 1, file) == NULL)	/* fgets stops at n-1 */
			udi_assert(0);
		if (fgetc(file) != '|')
			udi_assert(0);
		val = calloc(sizeof (char), val_len + 1);
		udi_assert(val);
		if (_get_val(val, val_len, file) == EOF)
			udi_assert(0);
		if (!valid || strncmp(name, attr_name, name_len) != 0) {
			free(val);
			retval = fscanf(file, "%d|%d|%d|%d|", &valid,
					&rtype, &name_len, &val_len);
			type = (udi_ubit8_t)rtype;
			continue;
		}
		*attr_type = type;
		switch (type) {
		case UDI_ATTR_ARRAY8:
		case UDI_ATTR_STRING:
			if (length < val_len)
				break;
			udi_memcpy(value, val, val_len);
			break;
		case UDI_ATTR_BOOLEAN:
			if (val[0] == 'T')
				*(udi_boolean_t *)value = TRUE;
			else
				*(udi_boolean_t *)value = FALSE;
			val_len = 1;
			break;
		case UDI_ATTR_UBIT32:
			*(udi_ubit32_t *)value = udi_strtou32(val, NULL, 16);
			val_len = sizeof (udi_ubit32_t);
			break;
		default:
			udi_assert(0);
		}
		return val_len;
	}
	return 0;
}

#if _UDI_PHYSIO_SUPPORTED

static sigjmp_buf pio_probe_err;

STATIC void
pio_probe_nack(int sig)
{
	printf("Caught signal %d\n", sig);
	siglongjmp(pio_probe_err, 1);
}


void
udi_pio_probe(udi_pio_probe_call_t *callback,
	      udi_cb_t *gcb,
	      udi_pio_handle_t pio_handle,
	      void *mem_ptr,
	      udi_ubit32_t pio_offset,
	      udi_ubit8_t tran_size_idx,
	      udi_ubit8_t direction)
{
	udi_ubit32_t tran_size = 1 << tran_size_idx;
	_udi_pio_handle_t *pio = pio_handle;
	volatile udi_status_t status = UDI_OK;
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);

	struct sigaction sigs;

	sigs.sa_handler = pio_probe_nack;
	/*
	 * Allow faults in the handler to be detected (albeit terminally) 
	 */
#ifdef SA_RESETHAND
	sigs.sa_flags = SA_RESETHAND;
#else
	sigs.sa_handler = SIG_DFL;
	sigs.sa_flags = 0;
#endif
	sigemptyset(&sigs.sa_mask);
	if (sigaction(SIGSEGV, &sigs, (struct sigaction *)NULL) != 0) {
		perror("Sigaction failed");
	}
	if (sigsetjmp(pio_probe_err, 1)) {
		status = UDI_STAT_HW_PROBLEM;
		goto do_ret;
	}

	/*
	 * Call the POPS functions for this mapping.   They'll do any
	 * needed pacing, swapping, and so on for us.
	 */
	switch (direction) {
	case UDI_PIO_IN:
		(*pio->pio_ops->std_ops.pio_read_ops[tran_size_idx])
			(pio_offset, mem_ptr, tran_size, pio);
		break;
	case UDI_PIO_OUT:
		(*pio->pio_ops->std_ops.pio_write_ops[tran_size_idx])
			(pio_offset, mem_ptr, tran_size, pio);
		break;
	default:
		_OSDEP_ASSERT(0);
	}

      do_ret:
	/*
	 * Restore signal handlers to default 
	 */
	sigs.sa_handler = SIG_DFL;
	if (sigaction(SIGSEGV, &sigs, NULL) != 0) {
		perror("Sigaction failed");
	}

	_UDI_IMMED_CALLBACK(cb, _UDI_CT_PIO_PROBE,
			    &_udi_alloc_ops_nop, callback, 1, (status));

}
#endif /* _UDI_PHYSIO_SUPPORTED */


#ifndef MALLOC_POSIX_TYPES

/*
 * Atomic Int Support:
 * Handle both Posix semaphores [optional extension] and systems where these do
 * not exist.
 */
void
_osdep_atomic_int_init(_OSDEP_ATOMIC_INT_T *atomic_intp,
		       int ival)
{
#ifdef _POSIX_SEMAPHORES
	int rval;

	rval = sem_init(&atomic_intp->sem, 0, ival);
	assert(rval == 0);
#else
	pthread_mutex_init(&(atomic_intp)->amutex, NULL);
	atomic_intp->aval = ival;
#endif /* _POSIX_SEMAPHORES */
#ifdef POSIX_TYPES_TRACKER
	posix_add_to_alloc_log(&atomic_intp->mblk);
#endif
}

void
_osdep_atomic_int_decr(_OSDEP_ATOMIC_INT_T *atomic_intp)
{
#ifdef _POSIX_SEMAPHORES
	int rval;

	rval = sem_trywait(&atomic_intp->sem);
	assert(rval == 0);
#else
	pthread_mutex_lock(&atomic_intp->amutex);
	atomic_intp->aval--;
	pthread_mutex_unlock(&atomic_intp->amutex);
#endif /* _POSIX_SEMAPHORES */
}

void
_osdep_atomic_int_incr(_OSDEP_ATOMIC_INT_T *atomic_intp)
{
#ifdef _POSIX_SEMAPHORES
	int rval;

	rval = sem_post(&atomic_intp->sem);
	assert(rval == 0);
#else
	pthread_mutex_lock(&atomic_intp->amutex);
	atomic_intp->aval++;
	pthread_mutex_unlock(&atomic_intp->amutex);
#endif /* _POSIX_SEMAPHORES */
}

void
_osdep_atomic_int_deinit(_OSDEP_ATOMIC_INT_T *atomic_intp)
{
#ifdef _POSIX_SEMAPHORES
	int rval;

	rval = sem_destroy(&atomic_intp->sem);
	assert(rval == 0);
#else
	pthread_mutex_destroy(&atomic_intp->amutex);
#endif /* _POSIX_SEMAPHORES */
#ifdef POSIX_TYPES_TRACKER
	posix_delete_from_alloc_log(&atomic_intp->mblk);
#endif
}

int
_osdep_atomic_int_read(_OSDEP_ATOMIC_INT_T *atomic_intp)
{
#ifdef _POSIX_SEMAPHORES
	int rval, value;

	rval = sem_getvalue(&atomic_intp->sem, &value);
	assert(rval == 0);
	return value;
#else
	return (atomic_intp->aval);
#endif /* _POSIX_SEMAPHORES */
}

#else

int debug_osdep_atomic_int_t = 1;
int debug_osdep_event_t = 1;
int debug_osdep_mutex_t = 1;

/*
 * Atomic Int Support:
 * Handle both Posix semaphores [optional extension] and systems where these do
 * not exist.
 */
void
_osdep_atomic_int_init(_OSDEP_ATOMIC_INT_T *atomic_inth,
		       int ival)
{
	_osdep_atomic_int *atomic_intp;

	atomic_intp = (_osdep_atomic_int *)
		_OSDEP_MEM_ALLOC(sizeof (_osdep_atomic_int), 0, UDI_WAITOK);
	*atomic_inth = atomic_intp;
#ifdef _POSIX_SEMAPHORES
	{
		int rval;

		rval = sem_init(&atomic_intp->sem, 0, ival);
		assert(rval == 0);
	}
#else
	pthread_mutex_init(&atomic_intp->amutex, NULL);
	atomic_intp->aval = ival;
#endif /* _POSIX_SEMAPHORES */
	if (debug_osdep_atomic_int_t >= 2)
		printf("_osdep_atomic_int_init(%p, %d)\n", atomic_intp, ival);
}

void
_osdep_atomic_int_decr(_OSDEP_ATOMIC_INT_T *atomic_intp)
{
#ifdef _POSIX_SEMAPHORES
	int rval;

	rval = sem_trywait(&(*atomic_intp)->sem);
	assert(rval == 0);
#else
	pthread_mutex_lock(&(*atomic_intp)->amutex);
	((*atomic_intp)->aval)--;
	pthread_mutex_unlock(&(*atomic_intp)->amutex);
#endif /* _POSIX_SEMAPHORES */
	if (debug_osdep_atomic_int_t >= 2)
		printf("_osdep_atomic_int_decr(%p)\n", atomic_intp);
}

void
_osdep_atomic_int_incr(_OSDEP_ATOMIC_INT_T *atomic_intp)
{
#ifdef _POSIX_SEMAPHORES
	int rval;

	rval = sem_post(&(*atomic_intp)->sem);
	assert(rval == 0);
#else
	pthread_mutex_lock(&(*atomic_intp)->amutex);
	((*atomic_intp)->aval)++;
	pthread_mutex_unlock(&(*atomic_intp)->amutex);
#endif /* _POSIX_SEMAPHORES */
	if (debug_osdep_atomic_int_t >= 2)
		printf("_osdep_atomic_int_incr(%p)\n", atomic_intp);
}

void
_osdep_atomic_int_deinit(_OSDEP_ATOMIC_INT_T *atomic_intp)
{
#ifdef _POSIX_SEMAPHORES
	int rval;

	rval = sem_destroy(&(*atomic_intp)->sem);
	assert(rval == 0);
#else
	pthread_mutex_destroy(&(*atomic_intp)->amutex);
#endif /* _POSIX_SEMAPHORES */
	_OSDEP_MEM_FREE(*atomic_intp);
	*atomic_intp = NULL;
	if (debug_osdep_atomic_int_t >= 2)
		printf("_osdep_atomic_int_deinit(%p)\n", atomic_intp);
}

int
_osdep_atomic_int_read(_OSDEP_ATOMIC_INT_T *atomic_intp)
{
#ifdef _POSIX_SEMAPHORES
	int rval, value;

	rval = sem_getvalue(&(*atomic_intp)->sem, &value);
	assert(rval == 0);
	if (debug_osdep_atomic_int_t >= 2)
		printf("_osdep_atomic_int_read(%p)\n", atomic_intp);
	return value;
#else
	if (debug_osdep_atomic_int_t >= 2)
		printf("_osdep_atomic_int_read(%p)\n", atomic_intp);
	return ((*atomic_intp)->aval);
#endif /* _POSIX_SEMAPHORES */
}

#endif

udi_ubit32_t
_udi_get_cur_time_in_ticks()
{
	struct timeval tv;
	udi_ubit32_t rv;

	if ((gettimeofday(&tv, NULL)) == -1) {
		printf("Unable to get time of day, errno %d\n", errno);
		exit(1);
	}

	/*
	 * Return current time in POSIX_TIMER_HZ-frequency ticks.
	 */
	rv = tv.tv_sec * POSIX_TIMER_HZ +
		tv.tv_usec / (USEC_PER_SEC / POSIX_TIMER_HZ);
	return rv;
}
