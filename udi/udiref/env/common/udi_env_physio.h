
/*
 * File: env/udi_env_physio.h
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

#ifndef _UDI_ENV_PHYSIO_H
#define _UDI_ENV_PHYSIO_H

/*
 * Interrupt handling definitions.
 */
#ifndef _OSDEP_ISR_RETURN_T
#define _OSDEP_ISR_RETURN_T int
#endif
typedef _OSDEP_ISR_RETURN_T _udi_bridge_mapper_isr_t(void *context);

/*
 * DMA handle.
 *
 * O/S-private DMA handle structure.
 */
typedef struct _udi_dma_handle {
	_udi_buf_t *dma_buf;		/* Buffer mapped by udi_dma_buf_map */
	struct _udi_alloc_hdr alloc_hdr;	/* Standard allocation header. */
	udi_size_t dma_buf_base;	/* Starting offset of bound portion
					 * of source buffer */
	udi_size_t dma_buf_offset;	/* Current offset in buffer */
	udi_size_t dma_maplen;		/* Total memory/buffer mapped size */
	void *dma_mem;			/* Memory from udi_dma_mem_alloc */
	udi_scgth_t dma_scgth;		/* Scatter/gather structure */
	udi_size_t dma_scgth_size;	/* Curr size of scatter/gather array */
	udi_size_t dma_scgth_allocsize;	/* Alloc.d scatter/gather array size */
	_udi_dma_constraints_t *dma_constraints;	/* DMA constraints */
	udi_ubit8_t dma_flags;		/* DMA direction(s) */
	udi_ubit8_t dma_buf_flags;	/* Direction(s) for current buf_map */
	udi_ubit8_t dev_endianness;	/* Device's byte order */
	udi_queue_t dma_rgn_q;		/* Used for Q'ing DMA mem hdl to rgn */
	_udi_region_t *dmah_region;	/* Region owning DMA handle */
	_OSDEP_DMA_T dma_osinfo;
} _udi_dma_handle_t;

/*
 * The structure for the elements in _udi_constraints_range_attributes.
 */
typedef struct {
	udi_dma_constraints_attr_t attr_id;
	udi_ubit32_t min_value;
	udi_ubit32_t max_value;
	udi_ubit32_t least_restr;
	udi_ubit32_t most_restr;
	udi_ubit32_t default_value;
	udi_ubit8_t restr_type;
} _udi_constraints_range_types;

/*
 * Note: If these numbers are changed, make sure that
 * _udi_constraints_range_attributes in udi_constraints.c is also
 * updated appropriately.
 */
#define	_UDI_CONSTRAINTS_TYPES_MAX	UDI_DMA_SLOP_BARRIER_BITS
#define _UDI_CONSTRAINTS_INDEX_MAX	22U

/* Here's the underlying definition for udi_dma_constraints_t */
struct _udi_dma_constraints {
	struct _udi_alloc_hdr alloc_hdr;	/* Standard allocation header. */
	udi_ubit32_t attributes[_UDI_CONSTRAINTS_INDEX_MAX];	/* DMA constrs */
	udi_queue_t buf_path_q;		/* queue of buf_path's refering to me */
};

#define _UDI_CONSTRAINT_VAL(attr, attr_code) \
		(attr)[_udi_constraints_index[(attr_code) - 1] - 1]

extern udi_dma_constraints_attr_t
  _udi_constraints_index[_UDI_CONSTRAINTS_TYPES_MAX];

#define _UDI_HANDLE_TO_DMA_CONSTRAINTS(handle) \
		((_udi_dma_constraints_t *)(handle))

extern _udi_xbops_t _udi_dma_m2buf_xbops;

/*
 * Internal PIO trans register descriptor.
 */
typedef struct {
	union {
		_OSDEP_PIO_LONGEST_STYPE payload[1];
		udi_ubit8_t c_payload[32];	/* contents of this register */
	} r;
	udi_ubit8_t bytes_valid;	/* current valid width of this reg */
} _udi_pio_register_t;

/*
 * Locking primitive for serializing PIO transactions.
 */
typedef struct {
	_OSDEP_MUTEX_T lock;		/* Lock to protect contents */
	udi_queue_t pio_queue;		/* Queue of transactions if busy. */
	udi_boolean_t active;		/* True if currently running a trans */
	_udi_pio_register_t registers[9];	/* Pseudo registers during execution */
	/*
	 * With a spare for temp 
	 */
} _udi_pio_mutex_t;

/* 
 * Fast (easy to decode) table of opcodes. 
 */

typedef enum {
	_UDI_PIO_UNKNOWN_OPCODE = 0,

	_UDI_PIO_IN,
	_UDI_PIO_IN_DIRECT,
	_UDI_PIO_OUT_1,
	_UDI_PIO_OUT_2,
	_UDI_PIO_OUT_4,
	_UDI_PIO_OUT,
	_UDI_PIO_LOAD_DIR,
	_UDI_PIO_LOAD_DIR_WIDE,
	_UDI_PIO_LOAD_1,
	_UDI_PIO_LOAD_2,
	_UDI_PIO_LOAD_4,
	_UDI_PIO_LOAD,
	_UDI_PIO_STORE_DIR,
	_UDI_PIO_STORE_DIR_WIDE,
	_UDI_PIO_STORE_1,
	_UDI_PIO_STORE_2,
	_UDI_PIO_STORE_4,
	_UDI_PIO_STORE,

	_UDI_PIO_LOAD_IMM,
	_UDI_PIO_LOAD_IMM_WIDE,
	_UDI_PIO_IN_IND,
	_UDI_PIO_OUT_IND,
	_UDI_PIO_SHIFT_LEFT,
	_UDI_PIO_SHIFT_LEFT_32,
	_UDI_PIO_SHIFT_LEFT_WIDE,
	_UDI_PIO_SHIFT_RIGHT,
	_UDI_PIO_SHIFT_RIGHT_32,
	_UDI_PIO_SHIFT_RIGHT_WIDE,
	_UDI_PIO_AND,
	_UDI_PIO_AND_WIDE,
	_UDI_PIO_AND_IMM,
	_UDI_PIO_AND_IMM_WIDE,
	_UDI_PIO_OR,
	_UDI_PIO_OR_WIDE,
	_UDI_PIO_OR_IMM,
	_UDI_PIO_OR_IMM_WIDE,
	_UDI_PIO_XOR,
	_UDI_PIO_XOR_WIDE,
	_UDI_PIO_ADD,
	_UDI_PIO_ADD_WIDE,
	_UDI_PIO_ADD_IMM,
	_UDI_PIO_ADD_IMM_WIDE,
	_UDI_PIO_SUB,
	_UDI_PIO_SUB_WIDE,
	_UDI_PIO_CSKIP_Z,
	_UDI_PIO_CSKIP_Z_WIDE,
	_UDI_PIO_CSKIP_NZ,
	_UDI_PIO_CSKIP_NZ_WIDE,
	_UDI_PIO_CSKIP_NEG,
	_UDI_PIO_CSKIP_NNEG,

	_UDI_PIO_BRANCH,
	_UDI_PIO_LABEL,
	_UDI_PIO_REP_IN_IND,
	_UDI_PIO_REP_OUT_IND,
	_UDI_PIO_DELAY,
	_UDI_PIO_BARRIER,
	_UDI_PIO_SYNC,
	_UDI_PIO_SYNC_OUT,
	_UDI_PIO_DEBUG,
	_UDI_PIO_END,
	_UDI_PIO_END_IMM,

	_UDI_PIO_END_OF_OPCODES
} _udi_pio_opcode_t;

/*
 * A (non-const) version of the original udi_pio_trans_t until
 * we are able to eliminate the dependencies on it.
 */
typedef struct {
	udi_ubit8_t pio_op;
	udi_ubit8_t tran_size;
	udi_ubit16_t operand;
} _udi_env_pio_trans_t;

/*
 * Internal descriptor for a "prescanned" udi_pio_trans_t.
 * Be sensitive to the size of this structure as we allocate a single
 * array of these behind the back of the driver and there can be up to
 * 64K of them.
 */
typedef struct _udi_pio_trans {
	udi_queue_t label_list;		/* (sorted) linked list of labels */
	_udi_env_pio_trans_t opcode;	/* Original opcode */
	struct _udi_pio_trans *next_trans;	/* Next opcode, NULL if end */
	struct _udi_pio_trans *branch_target;	/* branch, to where */
	_udi_pio_opcode_t fast_opcode;	/* decoded opcode */
	udi_index_t tran_size_bytes;	/* Cache tran_size lookups */
} _udi_pio_trans_t;


/* PIO address space definition */
typedef enum {
	_UDI_PIO_INVAL_SPACE = 0,
	_UDI_PIO_CFG_SPACE,
	_UDI_PIO_IO_SPACE,
	_UDI_PIO_MEM_SPACE
} _udi_addrsp_t;


/*
 * PIO handle.
 *
 * O/S-private PIO handle structure.
 */
typedef struct {
	struct _udi_alloc_hdr alloc_hdr;	/* Standard allocation header. */
	const struct _udi_pops *pio_ops;	/* pops vector for PIO device */
	udi_pio_trans_t *trans_list;	/* transaction list */
	_udi_addrsp_t address_type;	/* Type of mapping */
	udi_size_t offset;		/* Offset of mapping */
	udi_ubit32_t regset_idx;	/* regset index */
	udi_ubit8_t atomic_sizes;	/* supported atomic sizes */
	udi_ubit16_t trans_length;	/* size of above list */
	udi_size_t length;		/* length of mapped pio memory */
	udi_ubit32_t pace;		/* pio pacing (microseconds) */
	udi_ubit32_t flags;		/* common flags (see below) */
	unsigned long access_timestamp;	/* timestamp for pio pacing */
	_udi_pio_trans_t *scanned_trans;	/* prescanneed trans list */
	_udi_pio_trans_t *trans_entry_pt[8];	/* array of entry lables */
	udi_queue_t label_list_head;	/* Head of label list. */
	udi_boolean_t movable_warning_issued;	/* TRUE if already complained */
	_udi_pio_mutex_t *pio_mutex;	/* serialization mutex */
	struct _udi_dev_node *dev_node;	/* driver instance assoc with this  */
	/*
	 * list.  Used for pio_abort_seq 
	 */
	_OSDEP_PIO_T pio_osinfo;	/* OS-specific information */
} _udi_pio_handle_t;


/* Common pio_handle flags */
#define _UDI_PIO_DO_SWAP	(1<<0)
#define _UDI_PIO_NEVERSWAP	(1<<1)
#define _UDI_PIO_STRICTORDER	(1<<2)
#define _UDI_PIO_UNALIGNED	(1<<3)

typedef void (*_udi_pop_write_t) (udi_ubit32_t dest_offset,
				  void *value,
				  udi_size_t size,
				  _udi_pio_handle_t *pio_osinfo_p);

typedef void (*_udi_pop_read_t) (udi_ubit32_t src_offset,
				 void *data,
				 udi_size_t size,
				 _udi_pio_handle_t *pio_osinfo_p);

typedef enum {
	udi_pio_1byte = 0,
	udi_pio_2byte = 1,
	udi_pio_4byte = 2,
	udi_pio_8byte = 3,
	udi_pio_16byte = 4,
	udi_pio_32byte = 5
} _udi_pio_op_idx;


typedef const struct _udi_pops {
	struct {
		_udi_pop_read_t pio_read_ops[6];
		_udi_pop_write_t pio_write_ops[6];
	} std_ops;
	struct {
		_udi_pop_read_t pio_read_ops[6];
		_udi_pop_write_t pio_write_ops[6];
	} dir_ops;
} _udi_pops_t;

udi_status_t _udi_execute_translist(_udi_pio_register_t *registers,
				    udi_ubit16_t *result,
				    _udi_pio_handle_t *piohandle,
				    udi_buf_t *buf,
				    void *mem,
				    void *scratch,
				    udi_index_t start_label);

void _udi_get_pio_mapping(_udi_pio_handle_t *pio,
			  _udi_dev_node_t *dev);

void _udi_release_pio_mapping(_udi_pio_handle_t *piohandle);

void _udi_osdep_pio_map(_udi_pio_handle_t *pio);

udi_status_t _udi_pio_trans_prescan(_udi_pio_handle_t *handle);

#endif /* _UDI_ENV_PHYSIO_H */
