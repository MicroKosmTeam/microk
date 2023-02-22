
/*
 * File: env/common/udi_env.h
 *
 * UDI Common Environment definitions.
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

#ifndef _UDI_ENV_H
#define _UDI_ENV_H

#define UDI_VERSION 0x101
#include <udi.h>			/* Standard UDI definitions */

/* Internal typedefs for forward references: */
typedef struct _udi_driver _udi_driver_t;
typedef struct _udi_region _udi_region_t;
typedef struct _udi_dev_node _udi_dev_node_t;
typedef struct _udi_channel _udi_channel_t;
typedef struct _udi_buf_path _udi_buf_path_t;

#include <udi_osdep.h>			/* OS-dependent definitions */
#include <udi_osdep_defaults.h>		/* Defaults for things OS doesn't
					 * define */

#ifdef _UDI_PHYSIO_SUPPORTED

#  define UDI_PHYSIO_VERSION 0x101
#  include <udi_physio.h>		/* Public Physical I/O definitions */

#  define UDI_PCI_VERSION 0x101
#  include <udi_pci.h>			/* PCI bus bindings */

#  define UDI_SYSBUS_VERSION 0x100
#  include <udi_sysbus.h>		/* System bus bindings */

typedef struct _udi_dma_constraints _udi_dma_constraints_t;

#endif /* _UDI_PHYSIO_SUPPORTED */

/* Values used for sleep_ok parameter in _OSDEP_MEM_ALLOC macros */
#define UDI_NOWAIT   FALSE
#define UDI_WAITOK   TRUE

/* 
 * Align an address upward to a boundary expressed as a number of bytes. 
 * Treat like a function.   It will return the 'this' rounded up to 
 * 'boundary'-aligned.   'this' is not modified, but a natural use 
 * of this function has this appearing on LHS and RHS of the expresssion.
 * NOTE: The macro evaluates 'this' a number of times so be wary of 
 * side effects!
 */

#define _udi_alignto(this, boundary) \
  ((((long) (this) + (boundary) - 1) >= (long) (this))			\
   ? (((long) (this) + ((boundary) - 1)) & (~((long)(boundary)-1)))	\
   : ~ (long) 0)

/*
 * A structure that provides a conformant way to find the largest
 * alignment of a "natural" type.  The specification says that a number
 * of items " will be aligned on the most restrictive alignment of the 
 * platform's natural alignments for long and pointer data types"
 * and this is how we determine that.   Note that floating point is 
 * intentionally ignored here since those are non-transferrable types.
 */
union {
	void * pointer;
	long scalar;
} _udi_naturally_aligned;

/*
 * Checks to see if `this' pointer is aligned to satisfy the above
 * quoted specification reference.   Returns TRUE if so.
 */
#define _UDI_IS_NATURALLY_ALIGNED(this) \
	 (0 == ((long)(this) & (sizeof(_udi_naturally_aligned) - 1))) 

/* 
 * One for each resource allocated by any region. 
 */
typedef enum {
	_UDI_ALLOC_HDR_INVALID = 0,
	_UDI_ALLOC_HDR_PIO_MAP,
	_UDI_ALLOC_HDR_MEM,
	_UDI_ALLOC_HDR_CONSTRAINT,
	_UDI_ALLOC_HDR_DMA,
	_UDI_ALLOC_HDR_BUF,
	_UDI_ALLOC_HDR_BUF_PATH
} _udi_alloc_type_t;

/*
 * This struct is prepended into each 'thing' allocated on behalf of
 * the region.   It allows per-region tracking and is meant to be 
 * searchable in an efficient manner.
 */

typedef struct _udi_alloc_hdr {
	udi_queue_t ah_link;		/* hash link list we are on. */
	_udi_alloc_type_t ah_type;	/* Type of allocation this is. */
	int ah_flags;			/* Special flags, such as movable. */
} _udi_alloc_hdr_t;


/*
 * Per-Metalanguage Library/Mapper Info Structure.
 */
typedef struct _udi_meta {
	udi_queue_t link;
	const char *meta_name;
	udi_index_t meta_idx;
	udi_index_t meta_n_ops_templates;
	udi_mei_init_t *meta_info;
} _udi_meta_t;

/*
 * Select pairs of interface/control block.
 *
 * Set by udi_cb_select.
 */
typedef struct udi_cb_select {
	udi_index_t ops_idx;
	udi_index_t cb_idx;
	struct udi_cb_select *next;
} _udi_cb_select_t;

typedef struct {
	udi_size_t inline_size;
	udi_size_t inline_offset;
	udi_ubit16_t inline_ptr_offset;
	udi_layout_t *inline_layout;
} _udi_inline_info_t;

/*
 * Control block properties definition.
 *
 * Set by udi_mei_cb_init.
 */
typedef struct _udi_mei_cb_template {
	udi_index_t meta_idx;
	udi_index_t cb_group;
	udi_index_t cb_idx;
	udi_size_t cb_alloc_size;	/* Total size for a udi_cb_alloc */
	udi_size_t cb_scratch_size;
	udi_size_t cb_visible_size;
	udi_layout_t *cb_visible_layout;
	udi_size_t cb_marshal_size;
	_udi_inline_info_t *cb_inline_info;
} _udi_mei_cb_template_t;

/*
 * Region properties structure.
 */

typedef struct _udi_rprop {
	udi_queue_t link;
	udi_size_t rdata_size;
	udi_ubit32_t reg_attr;
	udi_time_t overrun_time;
	udi_index_t region_idx;
	udi_index_t primary_ops_idx;
	udi_index_t secondary_ops_idx;
	udi_index_t parent_bind_cb_idx;
	udi_index_t internal_bind_cb_idx;
} _udi_rprop_t;

/* 
 *  Stored in the interface, this holds all per-interface op data.
 */
typedef struct {
	_udi_inline_info_t *inline_info;
	udi_size_t if_op_data_scratch_size;
	udi_ubit16_t opdata_chain_offset;
} _udi_interface_op_data_t;

/*
 * Interface definitions.
 *
 * Set by udi_mei_ops_init.
 */
typedef struct _udi_interface {
	_udi_meta_t *if_meta;
	udi_ops_vector_t *if_ops;	/* Driver ops */
	const udi_ubit8_t *op_flags;	/* flags for each op in if_ops */
	udi_ubit32_t if_num_ops;
	udi_mei_ops_vec_template_t *if_ops_template;
	/*
	 * Ops template from meta 
	 */
	udi_mei_op_template_t *if_op_template_list;
	udi_size_t chan_context_size;
	_udi_interface_op_data_t *if_op_data;
} _udi_interface_t;

/* Here's the underlying definition for udi_buf_path_t */
struct _udi_buf_path {
	udi_queue_t constraints_link;	/* link to the constraints */
	udi_queue_t node_link;		/* link to the node */
#ifdef _UDI_PHYSIO_SUPPORTED
	udi_dma_constraints_t constraints;
#endif
};

#define _UDI_HANDLE_TO_BUF_PATH(handle) \
		((_udi_buf_path_t *)(handle))

/*
 * Driver's private meta information. One for each <meta> in the udiprops.txt
 */
typedef struct _udi_driver_meta_Q {
	udi_queue_t Q;
	const char *meta_name;
	udi_mei_init_t *meta_init;
	udi_index_t num_templates;
	udi_index_t meta_idx;
} _udi_driver_meta_Q_t;

typedef struct {
	_udi_rprop_t *rprop;
	udi_index_t ops_idx;
	const char *meta_name;
	udi_index_t meta_ops_num;
} _udi_bind_ops_list_t;

/*
 * O/S-private per-driver info structure.
 */
struct _udi_driver {
	udi_queue_t link;
	_OSDEP_EVENT_T MA_event;	/* For MA synch during teardown */
	udi_status_t MA_status;
	const char *drv_name;
	_udi_cb_select_t *drv_cb_select;
	_udi_cb_select_t *drv_cb_select_tl;
	_udi_mei_cb_template_t *drv_cbtemplate[256];
	_udi_interface_t *drv_interface[256];
	udi_queue_t drv_rprops;		/* queue of region properties */
	udi_size_t drv_mgmt_scratch_size;
	udi_ubit8_t per_parent_paths;	/* number of buf_path a 
					 * child wants per parent binding */
	udi_ubit8_t enumeration_attr_list_length;
	udi_size_t child_data_size;
	_udi_bind_ops_list_t *drv_child_interfaces;	/* array ended by ops_idx == 0 */
	_udi_bind_ops_list_t *drv_parent_interfaces;	/* array ended by ops_idx == 0 */
	_OSDEP_DRIVER_T drv_osinfo;
	udi_queue_t drv_meta_Q;		/* Linked list of required metas */
	udi_queue_t node_list;		/* List of all nodes instantiated for this driver */

} /*_udi_driver_t*/ ;

/*
 * The structure associated with a udi_mem handle.
 */
typedef struct _udi_mem_handle {
	struct _udi_alloc_hdr alloc_hdr;	/* Standard allocation header. */
} _udi_mem_handle_t;


#define RGN_HASH_SIZE 17

/*
 * Region structure.
 */

struct _udi_region {
	_OSDEP_SIMPLE_MUTEX_T reg_mutex;	/* Spinlock protecting region fields */
	udi_queue_t reg_queue;		/* Operations and callbacks waiting
					 * to run */
	udi_boolean_t reg_active;	/* Region currently running an
					 * op/callback */
	udi_boolean_t reg_is_interrupt;	/* True if interrupt region. */
	udi_ubit16_t reg_callback_depth;	/* Depth of callback nesting */
	udi_queue_t reg_sched_link;	/* Link this rgn on a scheduling queue */
	udi_queue_t reg_alt_sched_link;	/* Link this rgn on an alternate scheduling queue, such as for timers. */
	_udi_driver_t *reg_driver;	/* Driver this region belongs to */
	struct _udi_dev_node *reg_node;	/* Dev node this region belongs to */
	_udi_rprop_t *reg_prop;		/* Region properties */
	udi_queue_t reg_link;		/* link to other regions belonging to the same node */
	struct _udi_init_rdata *reg_init_rdata;	/* Region data area */
	struct _udi_channel *reg_first_chan;	/* First chan for this region */
	udi_trevent_t reg_tracemask;	/* Tracemask for general events */
	udi_trevent_t reg_ml_tracemask[256];	/* Tracemask for each meta idx */
	udi_queue_t reg_chan_queue;	/* Channels bound to region */
	udi_boolean_t reg_tearing_down;	/* region currently being torn down */
	udi_boolean_t reg_self_destruct;	/* final cleanup q'ed to this rgn? */
	udi_boolean_t in_sched_q;	/* this region is already in scheduling q */
	udi_queue_t reg_alloc_hash[RGN_HASH_SIZE];	/* Hash list of allocs. */

#if UDI_DEBUG
	/*
	 * Region statistics 
	 */
	udi_ubit16_t reg_queue_depth;	/* Depth of region queue */
	udi_ubit16_t reg_max_queue_depth;	/* Max depth of region queue */
	/*
	 * % direct calls vs queued calls 
	 */
	udi_ubit32_t reg_direct_calls;	/* # region entries w/ direct calls */
	udi_ubit32_t reg_queued_calls;	/* # region entries which are queued */
	/*
	 * Average depth of the region queue 
	 */
	udi_ubit32_t reg_queue_depth_sum;	/* Queue depth sum at region exit */
	udi_ubit32_t reg_num_exits;	/* # of region exits */
#endif

#ifdef _UDI_PHYSIO_SUPPORTED
	udi_queue_t dma_mem_handle_q;	/* Queue of dma mem alloc'd mem */
#endif

} /*_udi_region_t*/ ;

/*
 * Initial region data.
 */
typedef struct _udi_init_rdata {
	_udi_region_t *ird_region;
	udi_init_context_t ird_init_context;
} _udi_init_rdata_t;

/*
 * Region macros
 */
#define REGION_IS_ACTIVE(region) ((region)->reg_active)
#define REGION_IN_SCHED_Q(region) ((region)->in_sched_q)
#define REGION_LOCK(region)	_OSDEP_SIMPLE_MUTEX_LOCK(&(region)->reg_mutex)
#define REGION_UNLOCK(region) _OSDEP_SIMPLE_MUTEX_UNLOCK(&(region)->reg_mutex)

/*
 * -------------------------------------------------------------
 * Region Initialization
 */
#if UDI_DEBUG
#define REGION_INIT_STATS(region) \
	/* Initialize Region Statistics */  \
	(region)->reg_queue_depth = 0; \
	(region)->reg_max_queue_depth = 0; \
	(region)->reg_direct_calls = 0; \
	(region)->reg_queued_calls = 0; \
	(region)->reg_queue_depth_sum = 0; \
	(region)->reg_num_exits = 0;
#else
#define REGION_INIT_STATS(region) \
					/* No stats in the non-debug version */
#endif

#define REGION_INIT(region, driver) \
  { \
	/* Can use driver to make a more unique region lock name */ \
	_OSDEP_SIMPLE_MUTEX_INIT(&(region)->reg_mutex, \
				 "_udi_region_lock"); \
	UDI_QUEUE_INIT(&(region)->reg_queue); \
	UDI_QUEUE_INIT(&(region)->reg_chan_queue); \
	(region)->reg_active = FALSE; \
	(region)->reg_callback_depth = 0; \
	REGION_INIT_STATS(region); \
  }

#define REGION_DEINIT(region) \
	_OSDEP_SIMPLE_MUTEX_DEINIT(&(region)->reg_mutex)

/*
 * -------------------------------------------------------------
 * Run the Region Queue
 */
#if UDI_DEBUG
#define RUN_REGION_QUEUE_STATS(region) \
        (region)->reg_num_exits++; \
        (region)->reg_queue_depth_sum += (region)->reg_queue_depth; \
        if ((region)->reg_queue_depth_sum > 0xfffffffd) \
    	    (region)->reg_queue_depth_sum = (region)->reg_num_exits = 0;
#else
#define RUN_REGION_QUEUE_STATS(region) \
					/* No stats in the non-debug version */
#endif

#define UDI_RUN_REGION_QUEUE(region) \
	while ( _OSDEP_CONT_RUN_THIS_REGION(region) && \
		!UDI_QUEUE_EMPTY(&(region)->reg_queue)) { \
		_udi_run_next_on_region_queue(region); \
		REGION_LOCK(region); \
		RUN_REGION_QUEUE_STATS(region); \
	} \
	(region)->reg_active = FALSE;

/* 
 * -----------------------------------------------------------
 * Macros to append or remove elements from the region queue
 */
#if UDI_DEBUG
#define REGION_Q_APPEND_STATS(region) \
	(region)->reg_queue_depth++; \
	if ((region)->reg_queue_depth > (region)->reg_max_queue_depth) \
	    (region)->reg_max_queue_depth = (region)->reg_queue_depth;
#define REGION_Q_REMOVE_FIRST_STATS(region) \
	    (region)->reg_queue_depth--;
#define REGION_Q_REMOVE_STATS(region) \
	    (region)->reg_queue_depth--;
#else
#define REGION_Q_APPEND_STATS(region)
#define REGION_Q_REMOVE_FIRST_STATS(region) (void) 0
#define REGION_Q_REMOVE_STATS(region) (void) 0
#endif

#define _UDI_REGION_Q_APPEND(region, elementp) \
	    REGION_Q_APPEND_STATS(region); \
	    UDI_ENQUEUE_TAIL(&(region)->reg_queue, elementp)
/*
 * Return and dequeue the first CB from the region queue.  Region lock
 * must be already held.
 */
#define _UDI_REGION_Q_REMOVE_FIRST(region) \
	    (REGION_Q_REMOVE_FIRST_STATS(region), \
	    UDI_DEQUEUE_HEAD(&(region)->reg_queue))
/*
 * Return and dequeue the specified CB from the region queue.  Region lock
 * must be already held.
 */
#define _UDI_REGION_Q_REMOVE(region, elementp) \
	    (REGION_Q_REMOVE_STATS(region), \
	    UDI_QUEUE_REMOVE(elementp))

/*
 * Channel endpoint structure.
 */
typedef enum {
	_UDI_CHANNEL_LOOSE = 1,
	_UDI_CHANNEL_ANCHORED,
	_UDI_CHANNEL_SPAWNING,
	_UDI_CHANNEL_CLOSED
} channel_state_t;

struct _udi_channel {
	udi_queue_t chan_queue;		/* Channels attached to region */
	_udi_region_t *chan_region;	/* Region channel is anchored in */
	void *chan_context;		/* Current channel context pointer */
	_udi_interface_t *chan_interface;	/* Interface definition */
	struct _udi_channel *other_end;	/* Other end of the channel */
	struct _udi_spawn *chan_spawns;	/* Pending spawn operations */
	channel_state_t status;		/* status of a channel as defined above */
	udi_boolean_t bind_chan;	/* env should close these channels */
};

typedef struct _udi_spawn {
	udi_index_t spawn_idx;		/* index specified by driver */
	_udi_channel_t *spawn_channel;	/* source channel for the spawn */
	struct _udi_spawn *spawn_next;	/* next spawn in the list */
} _udi_spawn_t;


/*
 * Buffer declarations are out-of-line, since they're fairly involved.
 */
#include <udi_env_buf.h>

/*
 * Similarly, Physical I/O declarations are separate.
 */
#ifdef _UDI_PHYSIO_SUPPORTED
#include <udi_env_physio.h>
#endif /* _UDI_PHYSIO_SUPPORTED */


/*
 * Device node types.
 */

/*
 * _udi_node_attr_entry_t -- a {name,value} pair for a given device node
 * attribute.
 */
typedef struct {
	udi_queue_t link;		/* Link into a _udi_dev_node_t's
					 * attribute_list */
	char *name;			/* Attribute name */
	udi_size_t name_size;		/* Name size */
	void *value;			/* Attribute value */
	udi_size_t value_size;		/* Value size */
	udi_ubit8_t attr_type;		/* Data type.  i.e. UDI_ATTR_* */
} _udi_node_attr_entry_t;

struct _udi_dev_node {
	_OSDEP_MUTEX_T lock;		/* Lock to protect contents */
	_OSDEP_EVENT_T event;		/* Synchronization event for MA. */
	struct _udi_dev_node *parent_node;	/* Link to parent node */
	udi_queue_t sibling;		/* Links to sibling nodes */
	udi_queue_t child_list;		/* List of child nodes */
	udi_queue_t drv_node_list;	/* Link->next node, this driver */
	udi_queue_t attribute_list;	/* List of node attributes */
	udi_ubit32_t instance_number;	/* For logging */
	char *attr_address_locator;	/* text of address locator inst attr */
	char *attr_physical_locator;	/* text of phys locator inst attr */
	udi_queue_t buf_path_list;	/* list of buf paths for this node 
					 * FIXME multiple parents */
	_udi_channel_t *node_mgmt_channel;	/* MA's end of node's mgmt channel */
	udi_queue_t region_list;	/* regions associated with this node */
	_udi_channel_t *bind_channel;	/* Bind channel to the parent. */
	udi_ubit32_t my_child_id;	/* Parent-relative ID for this
					 * child */
	udi_ubit8_t parent_id;		/* FIXME: multiple parents */
	udi_index_t parent_ops_idx;	/* my parent will use this ops_idx to bind
					 * with me */
	udi_boolean_t still_present;	/* used across enumeration cycles */
	udi_sbit8_t read_in_progress;	/* a semaphore to access inst attrs */

#ifdef _UDI_PHYSIO_SUPPORTED
	/*
	 * scratch area for trans list 
	 */
	udi_size_t pio_abort_scratch_size;
	udi_pio_handle_t pio_abort_handle;	/* pio handle to be executed */
	udi_ubit32_t n_pio_mutexen;	/* Count of mutexes */
	_udi_pio_mutex_t *pio_mutexen;	/* Ptr to lock structs */
#endif					/* _UDI_PHYSIO_SUPPORTED */

	_OSDEP_DEV_NODE_T node_osinfo;	/* OS specific info */
};

/*
 * The header below contains the allocation marshalling structure, which
 * is embedded in every control-block. This was separated out to improve
 * legibility.
 */

#include <udi_alloc.h>

/*
 * Tracing and Logging structures -- common environment portion.  Also
 * located in a separate header file for legibility.
 */

#include <udi_env_trlog.h>

void
  _udi_env_log_write(udi_trevent_t trace_event,
		     udi_ubit8_t severity,
		     udi_index_t meta_idx,
		     udi_status_t original_status,
		     udi_ubit16_t msgnum,
		     ...);


/*
 * Control block layout:
 *
 *	Environment private fields (_udi_cb_t)
 *	Generic visible fields (udi_cb_t)
 *	Metalanguage-defined control block type-specific fields
 *	Inline arrays, if any
 *	Scratch/marshalling space
 */

typedef void _udi_callback_func_t (void);

/*
 * callback type for _UDI_QUEUE_TO_REGION macro
 */
typedef void udi_queue_to_region_call_t(udi_cb_t *cb,
					void *param);

/*
 * Control block -- environment portion
 */
typedef struct _udi_cb {
	udi_queue_t cb_qlink;		/* Link pointers for queuing */
	/*
	 * cb_qlink MUST BE first member 
	 */
	udi_ubit8_t cb_flags;		/* Misc flags (CHECK IF BIG ENOUGH) */
	udi_index_t op_idx;		/* Operation index/callback type */
	udi_size_t cb_size;		/* Total main control block size */
	udi_size_t scratch_size;	/* Current scratch space size */
	_udi_callback_func_t *cb_func;	/* Callback function */
	_udi_inline_info_t *cb_inline_info;	/* Store the cb_inline_info during
						 * initial allocation of the CB, don't
						 * change it on realloc. This is used to
						 * get to inline info during cb_realloc */
	/*
	 * used for abortable ops 
	 */
	struct _udi_cb *new_cb;		/* realloced cb */
	struct _udi_cb *old_cb;		/* cb that new_cb was realloced from */
	udi_queue_t abort_link;		/* is part of _udi_op_abort_cb q if 
					 * this cb is to be aborted */
	_udi_channel_t *abort_chan;	/* abort cb on this channel */

	udi_index_t complete_op_idx;	/* op idx of the normal completion routine */
	udi_index_t complete_op_num;	/* op num of the normal completion routine */
	udi_index_t except_op_idx;	/* op idx of the exception routine */
	udi_index_t except_op_num;	/* op num of the exception routine */
	union {
		_udi_alloc_marshal_t allocm;	/* Marshalling space for async services */
		_udi_log_marshal_t logm;	/* udi_log_write marshalling space */
	} cb_callargs;
} _udi_cb_t;

#define cb_allocm cb_callargs.allocm

#define _UDI_CB_TO_HEADER(drv_cb)	((_udi_cb_t *)(drv_cb) - 1)
#define _UDI_HEADER_TO_CB(env_cb)	((void *)((env_cb) + 1))
#define _UDI_GCB_TO_CHAN(gcb)		((_udi_channel_t *)((gcb)->channel))
#define _UDI_CB_TO_CHAN(cb)		_UDI_GCB_TO_CHAN( \
					   ((udi_cb_t*) _UDI_HEADER_TO_CB(cb)))

/* Control block flags (others are defined in OS-specific headers) */
#define _UDI_CB_CALLBACK    (1 << 0)	/* op_idx holds callback type */
#define	_UDI_CB_ALLOC_PENDING (1 << 1)	/* CB requires async region callback */
#define _UDI_CB_ABORTABLE (1 << 2)	/* CB is still abortable */

typedef struct _udi_bind_context _udi_bind_context_t;
typedef struct _udi_unbind_context _udi_unbind_context_t;
typedef struct _udi_reg_destroy_context _udi_reg_destroy_context_t;
typedef struct _udi_devmgmt_context _udi_devmgmt_context_t;
typedef void _udi_bind_cbfn_t(_udi_bind_context_t *);
typedef void _udi_generic_call_t (void *arg);
typedef void _udi_devmgmt_call_t(_udi_devmgmt_context_t *context);


struct _udi_bind_context {
	_udi_driver_t *driver;		/* binding with this driver */
	_udi_driver_t *parent_driver;	/* parent driver */
	_udi_dev_node_t *node;		/* node that is being bound to */
	_udi_region_t *primary_reg;	/* primary region */
	_udi_region_t *parent_reg;	/* parent region */
	_udi_region_t *sec_region;	/* secdonary region */
	_udi_region_t *psec_region;	/*parent secondary region */
	udi_queue_t *current_rprop;	/* the rprop currently workin on */
	_udi_bind_cbfn_t *next_step;	/* next step in bind process */
	_udi_rprop_t *parent_prop;	/*rprop of parent */
	const char *meta_name;		/* meta of the parent */
	udi_index_t list_idx;		/* current index of list of ops */
	udi_ubit8_t count;		/*remove this debugging info */
	_udi_generic_call_t *callback;	/* call this when we are done */
	void *arg;			/* call with this arg when we are done */
	udi_status_t *status;		/* store the status here */
};

struct _udi_devmgmt_context {
	_udi_devmgmt_call_t *callback;
	_udi_unbind_context_t *unbind_context;
	udi_status_t status;
	udi_ubit8_t flags;
};


struct _udi_unbind_context {
	_udi_dev_node_t *node;		/* me */
	_udi_unbind_context_t *parent_context;	/* parent's context */
	_udi_reg_destroy_context_t *reg_dest_context;
	_udi_generic_call_t *callback;	/* callback when this node is unbound */
	void *callbackarg;		/* with this arg */
};


struct _udi_reg_destroy_context {
	_udi_generic_call_t *callback;
	_udi_reg_destroy_context_t *arg;
	_udi_region_t *region;
	void (*final_callback) (void);
};

typedef struct {
	_udi_dev_node_t *node;
	_udi_generic_call_t *callback;	/* call this when we are done */
	void *arg;			/* call with this arg when we are done */
	udi_status_t *status;		/* store the status here */
	_udi_driver_t *driver;		/* the driver this node is bound to */
	udi_enumerate_cb_t *cb;		/* use this cb to send enumerate_req */
	udi_ubit8_t enum_level;
	udi_status_t status2;		/*see _udi_enumerate_new */
} _udi_enum_context_t;


/*
 * Internal function prototypes.
 */

void _udi_add_alloc_to_tracker(_udi_region_t *region,
			       _udi_alloc_hdr_t *alloc_hdr);

char *_udi_attr_string_decode(_udi_driver_t *drv,
			      char *locale,
			      char *orig_string);

void _udi_cb_realloc(udi_cb_t **gcbp,
		     udi_size_t new_scratch_size,
		     _udi_inline_info_t *inline_info);

void _udi_channel_anchor(_udi_channel_t *channel,
			 _udi_region_t *region,
			 udi_index_t ops_idx,
			 void *channel_context);

_udi_channel_t *_udi_channel_create(_udi_region_t *region,
				    udi_index_t ops_idx,
				    void *channel_context,
				    udi_boolean_t anchor);

_udi_channel_t *_udi_bind_channel_create(_udi_region_t *region1,
					 udi_index_t op_idx1,
					 _udi_region_t *region2,
					 udi_index_t op_idx2);


void _udi_channel_destroy(_udi_channel_t *channel);

void _udi_dev_node_init(_udi_driver_t *driver,	/* New udi node */

			_udi_dev_node_t *udi_node);	/* Parent udi node */
void _udi_dev_node_deinit(_udi_dev_node_t *udi_node);

void _udi_dealloc_orphan(_udi_alloc_hdr_t *alloc_hdr,
			 const char *name);

void _udi_do_region_destroy(_udi_region_t *region);

_udi_alloc_hdr_t *_udi_find_alloc_in_tracker(_udi_region_t *region,
					     void *memblk);

_udi_dev_node_t *_udi_get_child_node(_udi_dev_node_t *parent,
				     udi_ubit32_t child_ID);

udi_size_t _udi_get_layout_size(udi_layout_t *layout,
				udi_ubit16_t *inline_ptr_offset,
				udi_ubit16_t *chain_offset);

udi_boolean_t _udi_get_layout_offset(udi_layout_t *layout,
				     udi_layout_t **end,
				     udi_size_t *offset,
				     udi_layout_t key);

udi_size_t _udi_instance_attr_get(_udi_dev_node_t *node,
				  const char *name,
				  void *value,
				  udi_size_t length,
				  udi_instance_attr_type_t *attr_type_p);

udi_status_t _udi_instance_attr_set(_udi_dev_node_t *node,
				    const char *name,
				    const void *value,
				    udi_size_t length,
				    udi_instance_attr_type_t attr_type,
				    udi_boolean_t persistent);

void _udi_instance_attr_release(_udi_dev_node_t *node);

typedef udi_boolean_t _udi_match_func_t (void *,
					 void *);

/* ------------------- Management Agent Evironment Definitions ----------- */

#include <udi_mgmt_MA.h>

udi_MA_cb_t *_udi_MA_cb_alloc(void);

extern udi_init_context_t *_udi_MA_initcontext;
extern _udi_driver_t *_udi_MA_driver;


void _udi_MA_init(void);

void _udi_MA_deinit(void (*)(void));

_udi_dev_node_t *_udi_MA_find_node_by_osinfo(_udi_match_func_t *match_func,
					     void *arg);

udi_sbit32_t _udi_MA_dev_to_instance(const char *, _udi_dev_node_t *);
void *_udi_MA_local_data(const char *, udi_ubit32_t);

void *_udi_MA_meta_load(const char *meta_name,
			       udi_mei_init_t *meta_info,
			       const _OSDEP_DRIVER_T os_driv_init);

_OSDEP_DRIVER_T _udi_MA_meta_unload(const char *meta_name);

_udi_driver_t *_udi_MA_driver_load(const char *drv_name,
				   udi_init_t *init_info,
				   const _OSDEP_DRIVER_T os_driv_init);

udi_status_t _udi_MA_install_driver(_udi_driver_t *drv);

void _udi_MA_destroy_driver(_udi_driver_t *);

void _udi_MA_destroy_driver_by_name(const char *drv_name);

_udi_driver_t *_udi_MA_match_children(_udi_dev_node_t *node);

_udi_driver_t *_udi_driver_lookup_by_name(const char *drvname);

udi_status_t _udi_MA_bind_to_parent(udi_MA_cb_t *cb,
				    _udi_dev_node_t *child_node,
				    _udi_channel_t *channel);

void _udi_MA_bind_to_primary(udi_MA_cb_t *cb,
			     _udi_channel_t *channel);

void _udi_MA_final_cleanup(_udi_dev_node_t *node,
			   _udi_unbind_context_t *context);

udi_status_t _udi_MA_bind_children(_udi_dev_node_t *parent_node,
				   _udi_region_t *parent_region);

void _udi_marshal_params(udi_layout_t *layout,
			 void *marshal_space,
			 va_list args);

_udi_mei_cb_template_t *_udi_mei_cb_init(udi_index_t meta_idx,
					 udi_index_t cb_group,
					 udi_index_t cb_idx,
					 udi_size_t scratch_requirement,
					 udi_size_t inline_size,
					 udi_layout_t *inline_layout);

_udi_interface_t *_udi_mei_ops_init(udi_index_t meta_idx,
				    const char *meta_name,
				    udi_index_t ops_num,
				    udi_index_t ops_idx,
				    udi_ops_vector_t *ops,
				    const udi_ubit8_t *op_flags,
				    udi_size_t chan_context_size,
				    udi_index_t n_ops_templates,
				    udi_mei_init_t *meta_info);

void _udi_primary_region_init(udi_primary_init_t *init_info,
			      _udi_driver_t *driver);

void _udi_post_init(udi_init_t *init_info,
		    _udi_driver_t *drv);

_udi_region_t *_udi_region_create(_udi_driver_t *driver,
				  _udi_dev_node_t *node,
				  _udi_rprop_t *rgn_prop);

_udi_rprop_t *_udi_region_prepare(udi_index_t region_idx,
				  udi_size_t rdata_size,
				  _udi_driver_t *drv);

void _udi_region_destroy(_udi_reg_destroy_context_t *context);

void _udi_rm_alloc_from_tracker(_udi_alloc_hdr_t *alloc_hdr);

void _udi_queue_callback_l(_udi_cb_t *cb);

void _udi_queue_callback(_udi_cb_t *cb);

void _udi_run_inactive_region(_udi_region_t *region);

void _udi_run_next_on_region_queue(_udi_region_t *region);

void _udi_secondary_region_init(_udi_driver_t *drv,
				udi_secondary_init_t *sec_init_list);

int _udi_tolower(int c);
int _udi_toupper(int c);

udi_boolean_t _udi_set_region_active(_udi_region_t *region);

extern udi_mei_init_t *_udi_mgmt_meta;
extern udi_mei_op_template_t _udi_channel_event_ind_template;

extern struct udi_MA_cb *_udi_op_abort_cb;
_OSDEP_SIMPLE_MUTEX_T _udi_op_abort_cb_mutex;

/* a generic function that takes an event as argument and signal that event */
void _udi_operation_done(void *arg);

/*
 * ----------------- UDIENV_CHECK ------------------------------------
 *
 * Works like _OSDEP_ASSERT except additional first argument is the
 * "name" of the thing being tested.  This is especially useful for
 * times when the driver has specified invalid parameters since a
 * description of the parameter problem is usually more useful:
 *
 * UDIENV_CHECK(udi_primary_region_init_called, rgn_prop != NULL);
 *
 * Prints the following if rgn_prop is NULL:
 *
 * .... Assertion 'udi_primary_region_init_called' failed.
 */

#ifdef NDEBUG
#define UDIENV_CHECK(Name,Eval)
#else
#define UDIENV_CHECK(Name,Eval) { udi_boolean_t Name = (Eval); \
                                  _OSDEP_ASSERT(Name); }
#endif

/*
 * ----------------- _OSDEP_ENDIAN ------------------------------------
 *
 * Default macros to swap endianness if not provided by osdep.h.
 */
#ifndef _OSDEP_ENDIAN_SWAP_16
#  define _OSDEP_ENDIAN_SWAP_16(a) UDI_ENDIAN_SWAP_16(a)
#endif

#ifndef _OSDEP_ENDIAN_SWAP_32
#  define _OSDEP_ENDIAN_SWAP_32(a) UDI_ENDIAN_SWAP_32(a)
#endif

/*
 * ----------------- _OSDEP_SPROPS -------------------------------------
 *
 * Static properties definitions
 */

/* Static Property Types */
#define UDI_SP_META		1
#define UDI_SP_CHILD_BINDOPS	2

#include <udi_sprops_ra.h>
#define UDI_SP_PARENT_BINDOPS	3
#define UDI_SP_INTERN_BINDOPS	4
#define UDI_SP_PROVIDE		5
#define UDI_SP_REGION		6
#define UDI_SP_DEVICE		7
#define UDI_SP_READFILE		8
#define UDI_SP_LOCALES		9
#define UDI_SP_READMSGFILE	10
#define UDI_SP_MESGFILE		11
#define UDI_SP_REQUIRE		12

/* BINDOPS TYPES */
#define UDI_SP_OP_METAIDX	1
#define UDI_SP_OP_REGIONIDX	2
#define UDI_SP_OP_OPSIDX	3
#define UDI_SP_OP_PRIMOPSIDX	4
#define UDI_SP_OP_SECOPSIDX	5
#define UDI_SP_OP_CBIDX		6

#include <udi_sprops_ra.h>

/* _udi_osdep_sprops_init -- locates and prepares the Static Properties for 
 *                       the specified driver (where driver = driver, mapper,
 *                       and metalanguage library).
 */
void _udi_osdep_sprops_init(_udi_driver_t *drv);

/* _udi_osdep_sprops_deinit -- called when the driver is being unloaded to reverse
 *                         any activities performed by _udi_osdep_sprops_init.
 */
void _udi_osdep_sprops_deinit(_udi_driver_t *drv);

/* _udi_osdep_sprops_count -- counts the number of Static Properties of the
 *                        specified type.
 */
udi_ubit32_t _udi_osdep_sprops_count(_udi_driver_t *drv,
				     udi_ubit8_t type);

/* _osdep_sprops_get_xxx -- obtain values for specified Static Properties */
udi_ubit32_t _udi_osdep_sprops_get_version(_udi_driver_t *drv);
char *_udi_osdep_sprops_get_shortname(_udi_driver_t *drv);

char *_udi_osdep_sprops_get_meta_if(_udi_driver_t *drv,
				    udi_ubit32_t meta_idx);
int _udi_osdep_sprops_get_bindops(_udi_driver_t *drv,
				  udi_ubit32_t inst_index,
				  udi_ubit8_t bind_type,
				  udi_ubit8_t op_type);

#ifdef _UDI_PHYSIO_SUPPORTED
udi_sbit32_t _udi_osdep_sprops_get_pio_ser_lim(_udi_driver_t *drv);
#endif /* _UDI_PHYSIO_SUPPORTED */


/* _udi_osdep_sprops_ifind_message -- obtain raw message string for the specified
 *                                message number, driver, and locale.  Use
 *                                _udi_attr_string_decode to cook this raw
 *                                string.
 */
char *_udi_osdep_sprops_ifind_message(_udi_driver_t *drv,
				      char *locale,
				      udi_ubit32_t num);

/* _OSDEP_SPROPS_GET_DEV_ATTR_VALUE -- obtain the value for a named attribute
 *                                     of the specified type.  The type found
 *                                     for the attribute must match the type
 *                                     specified; the value is copied to the
 *                                     memory pointed to by valuep.  The length
 *                                     of the value is the return value of this
 *                                     function.
 */
udi_ubit32_t _udi_osdep_sprops_get_dev_attr_value(_udi_driver_t *drv,
						  char *name,
						  udi_ubit32_t dev_index,
						  udi_instance_attr_type_t
						  attr_type,
						  void *valuep);

udi_ubit32_t _udi_osdep_sprops_get_dev_nattr(_udi_driver_t *drv,
					     udi_ubit32_t device_index);

void _udi_osdep_sprops_get_dev_attr_name(_udi_driver_t *drv,
					 udi_ubit32_t device_index,
					 udi_ubit32_t attrib_index,
					 char *namestr);

/* get info about the region declation */
udi_ubit32_t _udi_osdep_sprops_get_reg_attr(_udi_driver_t *drv,
					    udi_ubit32_t index);

udi_ubit32_t _udi_osdep_sprops_get_reg_overrun(_udi_driver_t *drv,
					       udi_ubit32_t index);

udi_ubit32_t _udi_osdep_sprops_get_reg_idx(_udi_driver_t *drv,
					   udi_ubit32_t index);

udi_boolean_t
  _udi_osdep_sprops_get_rf_data(_udi_driver_t *drv,
				const char *attr_name,
				char *buf,
				udi_size_t buf_len,
				udi_size_t *act_len);

/*
 * Categories of _udi_timer_start callers
 */
typedef enum {
	_UDI_TIMER_START,		/* Called by udi_timer_start */
	_UDI_TIMER_START_REPEATING,	/* Called by udi_timer_start_repeating */
	_UDI_TIMER_RESTART_REPEATING	/* Called to restart repeating timer */
} _udi_timer_start_caller_t;

/*
 * ----------------- Locale Management -------------------------------------
 */

/*
 * UDI Environment Locale Management
 */

char *_udi_current_locale(char *new_locale);


/*
 * ----------------- Environment Debug Management ---------------------------
 *
 * There are 5 levels defined:
 *     0 = no debug output
 *     1 = error debug output
 *     2 = major event debug output
 *     3 = detail debug output
 *     4 = unit-debugging output (should be removed after unit is functional)
 *
 * In addition the NO_UDI_DEBUG flag can be defined for the build which
 * removes ALL debug output at pre-compile time and no run-time flexibility
 * is provided.
 */

extern udi_ubit32_t _udi_debug_level;

#ifdef NO_UDI_DEBUG
#define DEBUG1(Msgnum,Argc,Args) ;
#define DEBUG2(Msgnum,Argc,Args) ;
#define DEBUG3(Msgnum,Argc,Args) ;
#define DEBUG4(Msgnum,Argc,Args) ;
#else
#define _DBGARGS_0
#define _DBGARGS_1(arg1)                          (arg1)
#define _DBGARGS_2(arg1,arg2)                     (arg1),(arg2)
#define _DBGARGS_3(arg1,arg2,arg3)                (arg1),(arg2),(arg3)
#define _DBGARGS_4(arg1,arg2,arg3,arg4)           (arg1),(arg2),(arg3),(arg4)
#define _DBGARGS_5(arg1,arg2,arg3,arg4,arg5)      (arg1),(arg2),(arg3),(arg4),\
                                                  (arg5)
#define _DBGARGS_6(arg1,arg2,arg3,arg4,arg5,arg6) (arg1),(arg2),(arg3),(arg4),\
                                                  (arg5),(arg6)

#define DEBUG1(Msgnum,Argc,Args) if (_udi_debug_level >= 1)		      \
                                   _udi_env_log_write(_UDI_TREVENT_ENVERR,    \
					              UDI_LOG_ERROR, 0,	      \
						      UDI_STAT_NOT_SUPPORTED, \
						      (Msgnum),		      \
						      _DBGARGS_##Argc Args);
#define DEBUG2(Msgnum,Argc,Args) if (_udi_debug_level >= 2)		      \
                                   _udi_env_log_write(_UDI_TREVENT_ENVINFO,   \
					              UDI_LOG_INFORMATION, 0, \
						      UDI_OK, (Msgnum),	      \
						      _DBGARGS_##Argc Args);
#define DEBUG3(Msgnum,Argc,Args) if (_udi_debug_level >= 3)		   \
                                    udi_trace_write(_udi_MA_initcontext,   \
						    _UDI_TREVENT_ENVINFO,  \
						    0, (Msgnum),	   \
						    _DBGARGS_##Argc Args);
#define DEBUG4(Msgnum,Argc,Args) if (_udi_debug_level >= 4)		   \
                                    udi_trace_write(_udi_MA_initcontext,   \
						    _UDI_TREVENT_ENVINFO,  \
						    0, (Msgnum),	   \
						    _DBGARGS_##Argc Args);
#endif

extern void _udi_timer_deinit(void);
extern void _udi_timer_init(void);
extern void _udi_alloc_deinit(void);

udi_ubit32_t _udi_timer_start(udi_timer_expired_call_t *callback,
			      _udi_cb_t *gcb,
			      udi_ubit32_t ticks,
			      _udi_timer_start_caller_t interval);

#endif /* _UDI_ENV_H */
