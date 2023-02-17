
/*
 * File: env/common/udi_piotrans.c
 *
 * UDI PIO Trans List Routines
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
	INTERNAL REGISTER DESCRIPTION

The eight PIO registers described in the specification are internally
called pseudo-registers. Each pseudo register is a data structure that
is optimized for performance and eliminates special cases for the common
(1,2,4, and sometimes 8 byte) cases. Each pseudo register contains a
payload that is the user-visible component of the data. Internally,
the payload is an array of _OSDEP_PIO_LONGEST_TYPE along with a byte
counter describing how many bytes are currently valid in each register.
OSDEP_PIO_LONGEST_TYPE is a 'long' or a 'long long' on systems that
support it. Internally, the least significant word is always stored
first. This is so that the data that is mostly likely to be needed
(the 1,2,4 byte case) is always in the same place, payload[0]. It is
critical for the functionality of the PIO engine (esp. on big-endian
systems) that the individual payload members never be written or read
in a sub-PIO_LONGEST_TYPE access. Data is masked when written into
payload[0] and, if necessary, cast on the way out to perform potential
sign extension.

When data is displayed in a human-readable form, it is somewhat
inconvenient that we have to loop "backwards" over the payload. But the
cost of doing debugging operations "backwards" vs. potentially having to
pick data up and move it for a width extension is a good tradeoff.

*/

#if 0
#  define _UDI_SUPPRESS_PIO_DEBUGGING 1
#endif

#include <udi_env.h>

#if _UDI_PHYSIO_SUPPORTED

#include "udi_env_buf.h"

#define REGISTER(x) 	(x & 0x7)
#define ADDRESSMODE(x)  (x & 0x18)

#if  _UDI_SUPPRESS_PIO_DEBUGGING
#  define AM_TRACING(type, level) 0
#  define IS_ALIGNED(P, type) TRUE
#  define VERIFY_ALIGNED(p, type)
#  undef _OSDEP_ASSERT
#  define _OSDEP_ASSERT(expr)
#else  /* _UDI_SUPPRESS_PIO_DEBUGGING */

/*
 * For pointer P, verify that it's suitably aligned as type 
 */

#  define IS_ALIGNED(P, type) (0 == ((long)(P) & (sizeof(type) - 1)))
#  define VERIFY_ALIGNED(P, type) \
    if (!(IS_ALIGNED(P, type)))  \
  	_OSDEP_PRINTF("Alignment problem at %d: %p not %d byte aligned\n", \
			__LINE__, P, sizeof(type));


/*
 * An expression that returns true if we want to display trace events.
 * It's abstracted out to allow these to compile completely away.
 * 
 * It's unsightly that this expression "peeks out" of its arg list
 * into the current_trace_level, but adding it as an arg was aesthetically
 * unappealing.
 */
#  define AM_TRACING(type, level) \
    (current_trace_level & (type ## level))
#endif


/*
 * Unfortunately, this code does have to know the endianness of the host
 * OS as a compile-time constant to efficiently shuffle things between
 * internal register representation (which, without this has unknown 
 * endianness) and external memory representation which has a known and
 * generally constant endianness.
 */
#if !defined (_OSDEP_HOST_IS_BIG_ENDIAN)
#error Define _OSDEP_HOST_IS_BIG_ENDIAN in osdep.h as a compile-time constant.
#endif

#if _OSDEP_HOST_IS_BIG_ENDIAN
#define MEM_ITERATE \
	 for(i=tran_size_words;i--;)
#else
#define MEM_ITERATE \
	 for(i=0;i<tran_size_words;i++)
#endif


/*
 * Macros to decode UDI_PIO_REP_{OUT,IN}_IND opcodes 
 */
#define MEM_REG(x)	(x & 0x07)
#define MEM_STRIDE(x)   ((x >>  5) & 0x03)
#define PIO_REG(x)	((x >>  7) & 0x07)
#define PIO_STRIDE(x)	((x >> 10) & 0x03)
#define CNT_REG(x)	((x >> 13) & 0x07)

#define PIO_DEBUG 0

/*
 * This is the biggest "thing" we can handle as an intrinsic type.
 */
typedef _OSDEP_PIO_LONGEST_STYPE pio_s_register_t;
typedef _OSDEP_PIO_LONGEST_UTYPE pio_u_register_t;

/*
 * Because we want to store everything internally in an array of 
 * pio_register_t's, sometimes we need to mask off the bottom bits of
 * that leas significant word.  Here is a table, indexed by tran_size, 
 * to do that.
 */
static pio_s_register_t _udi_pio_mask_table[] = {
	0xff,
	0xffff,
	(pio_s_register_t)0xffffffff,
	(pio_s_register_t)-1,
	(pio_s_register_t)-1,
	(pio_s_register_t)-1,
};

#if !_UDI_SUPPRESS_PIO_DEBUGGING

/*
 * Control the format for outputting these.
 */
typedef enum {
	op_class_unknown = 0,
	op_class_immed,
	op_class_reg,
	op_class_cskip,
	op_class_branch,
	op_class_delay
} op_class_t;

typedef const struct {
	_udi_pio_opcode_t opcode;
	const char *label;
	op_class_t op_class;
} udi_pio_debug_t;


udi_pio_debug_t _udi_pio_debug_table[] = {
	{_UDI_PIO_UNKNOWN_OPCODE, "UNKNOWN_OPCODE"},
	{_UDI_PIO_IN, "IN"},
	{_UDI_PIO_IN_DIRECT, "IN_DIRECT"},
	{_UDI_PIO_OUT_1, "OUT_1"},
	{_UDI_PIO_OUT_2, "OUT_2"},
	{_UDI_PIO_OUT_4, "OUT_4"},
	{_UDI_PIO_OUT, "OUT"},
	{_UDI_PIO_LOAD_DIR, "LOAD_DIRECT"},
	{_UDI_PIO_LOAD_DIR_WIDE, "LOAD_WIDE_DIRECT"},
	{_UDI_PIO_LOAD_1, "LOAD_1"},
	{_UDI_PIO_LOAD_2, "LOAD_2"},
	{_UDI_PIO_LOAD_4, "LOAD_4"},
	{_UDI_PIO_LOAD, "LOAD"},
	{_UDI_PIO_STORE_DIR, "STORE_DIRECT"},
	{_UDI_PIO_STORE_DIR_WIDE, "STORE_DIRECT_WIDE"},
	{_UDI_PIO_STORE_1, "STORE_1"},
	{_UDI_PIO_STORE_2, "STORE_2"},
	{_UDI_PIO_STORE_4, "STORE_4"},
	{_UDI_PIO_STORE, "STORE"},
	{_UDI_PIO_LOAD_IMM, "LOAD_IMM", op_class_immed},
	{_UDI_PIO_LOAD_IMM_WIDE, "LOAD_IMM"},
	{_UDI_PIO_IN_IND, "IN_IND"},
	{_UDI_PIO_OUT_IND, "OUT_IND"},
	{_UDI_PIO_SHIFT_LEFT, "SHIFT_LEFT", op_class_immed},
	{_UDI_PIO_SHIFT_LEFT_32, "SHIFT_LEFT_32", op_class_immed},
	{_UDI_PIO_SHIFT_LEFT_WIDE, "SHIFT_LEFT_WIDE", op_class_immed},
	{_UDI_PIO_SHIFT_RIGHT, "SHIFT_RIGHT", op_class_immed},
	{_UDI_PIO_SHIFT_RIGHT_32, "SHIFT_RIGHT_32", op_class_immed},
	{_UDI_PIO_SHIFT_RIGHT_WIDE, "SHIFT_RIGHT_WIDE", op_class_immed},
	{_UDI_PIO_AND, "AND", op_class_reg},
	{_UDI_PIO_AND_WIDE, "AND_WIDE", op_class_reg},
	{_UDI_PIO_AND_IMM, "AND_IMM", op_class_immed},
	{_UDI_PIO_AND_IMM_WIDE, "AND_IMM_WIDE", op_class_immed},
	{_UDI_PIO_OR, "OR", op_class_reg},
	{_UDI_PIO_OR_WIDE, "OR_WIDE", op_class_reg},
	{_UDI_PIO_OR_IMM, "OR_IMM", op_class_immed},
	{_UDI_PIO_OR_IMM_WIDE, "OR_IMM_WIDE", op_class_immed},
	{_UDI_PIO_XOR, "XOR", op_class_reg},
	{_UDI_PIO_XOR_WIDE, "XOR_WIDE", op_class_reg},
	{_UDI_PIO_ADD, "ADD", op_class_reg},
	{_UDI_PIO_ADD_WIDE, "ADD_WIDE", op_class_reg},
	{_UDI_PIO_ADD_IMM, "ADD_IMM", op_class_immed},
	{_UDI_PIO_ADD_IMM_WIDE, "ADD_IMM_WIDE", op_class_immed},
	{_UDI_PIO_SUB, "SUB", op_class_reg},
	{_UDI_PIO_SUB_WIDE, "SUB_WIDE", op_class_reg},
	{_UDI_PIO_CSKIP_Z, "CSKIP_Z", op_class_cskip},
	{_UDI_PIO_CSKIP_Z_WIDE, "CSKIP_Z_WIDE", op_class_cskip},
	{_UDI_PIO_CSKIP_NZ, "CSKIP_NZ", op_class_cskip},
	{_UDI_PIO_CSKIP_NZ_WIDE, "CSKIP_NZ_WIDE", op_class_cskip},
	{_UDI_PIO_CSKIP_NEG, "CSKIP_NEG", op_class_cskip},
	{_UDI_PIO_CSKIP_NNEG, "CSKIP_NNEG", op_class_cskip},
	{_UDI_PIO_BRANCH, "BRANCH", op_class_branch},
	{_UDI_PIO_LABEL, "LABEL"},
	{_UDI_PIO_REP_IN_IND, "REP_IN_IND"},
	{_UDI_PIO_REP_OUT_IND, "REP_OUT_IND"},
	{_UDI_PIO_DELAY, "DELAY", op_class_delay},
	{_UDI_PIO_BARRIER, "BARRIER", op_class_immed},
	{_UDI_PIO_SYNC, "SYNC", op_class_immed},
	{_UDI_PIO_SYNC_OUT, "SYNC_OUT", op_class_immed},
	{_UDI_PIO_DEBUG, "DEBUG"},
	{_UDI_PIO_END, "END", op_class_reg},
	{_UDI_PIO_END_IMM, "END_IMM", op_class_immed},
	{_UDI_PIO_END_OF_OPCODES, "END_OF_OPCODES"},
};

STATIC void
_udi_pio_dumpmem(void *p,
		 int sz)
{
	int i;
	udi_ubit32_t *ub32 = p;

	switch (sz) {
	case 0:
		_OSDEP_PRINTF("-");
		break;
	case 1:
		_OSDEP_PRINTF("%02x", *(udi_ubit8_t *)p);
		break;
	case 2:
		_OSDEP_PRINTF("%04x", *(udi_ubit16_t *)p);
		break;
	case 4:
		_OSDEP_PRINTF("%08x", *(udi_ubit32_t *)p);
		break;
		/*
		 *  TODO: Figure out how to display a LONGEST_TYPE 
		 *        when it may or may not be available..  
		 *        Since we don't really know what a pio_u_register is,
		 *        and we don't know the real layout of it in core,
		 *        this is hard to do portably.
		 *        We do know that our OSDEP_PRINTF isn't required
		 *        to handle anything above 32 bits and we do know
		 *        that our layout doesn't have to match a 
		 *        udi_bus_addr64_t.   So we just open code a display.
		 */
	default:
		for (i = sz / sizeof (udi_ubit32_t); i;) {
			_OSDEP_PRINTF("%08x", ub32[--i]);
			if (sizeof (pio_u_register_t) ==
			    2 * sizeof (udi_ubit32_t))
				_OSDEP_PRINTF("%08x", ub32[--i]);
		}
		break;
	}
}

STATIC void
_udi_pio_dumpreg(_udi_pio_register_t *reg)
{
	unsigned int i;

	switch (reg->bytes_valid) {
	case 0:
		_OSDEP_PRINTF("-");
		break;
	case 1:
		_OSDEP_PRINTF("%02x", (int)reg->r.payload[0]);
		break;
	case 2:
		_OSDEP_PRINTF("%04x", (int)reg->r.payload[0]);
		break;
	case 4:
		_OSDEP_PRINTF("%08x", (int)reg->r.payload[0]);
		break;
	default:
		for (i = reg->bytes_valid / sizeof (pio_u_register_t); i;) {
			i--;
			if (sizeof (pio_u_register_t) == 8)
				_OSDEP_PRINTF(_OSDEP_PRINTF_LONG_LONG_SPEC,
					      reg->r.payload[i]);
			else
				_OSDEP_PRINTF("%08x", (int)reg->r.payload[i]);
		}
		break;
	}
}

STATIC void
_udi_pio_dumpregs(_udi_pio_register_t *regs)
{
	int i;

	_OSDEP_PRINTF("\n");
	for (i = 0; i < 8; i++) {
		_OSDEP_PRINTF(" %d:", i);
		_udi_pio_dumpreg(&regs[i]);
	}
	_OSDEP_PRINTF("\n");
}

static void
_udi_pio_trace_ops(int level,
		   _udi_pio_handle_t *pio,
		   _udi_pio_trans_t *scanned_trans)
{
	udi_pio_debug_t *dt;
	const char *text_desc;
	udi_ubit16_t opcode = scanned_trans->opcode.pio_op;
	udi_ubit16_t operand = scanned_trans->opcode.operand;

	dt = &_udi_pio_debug_table[scanned_trans->fast_opcode];
	text_desc = dt->label;

	/*
	 * First, output the offset and opcode name.
	 */

	if (scanned_trans->branch_target &&
	    (scanned_trans->branch_target != pio->scanned_trans)) {
		_OSDEP_PRINTF("[%d, L%d+%d] UDI_PIO_%s",
			      scanned_trans - pio->scanned_trans,
			      scanned_trans->branch_target->opcode.operand,
			      scanned_trans - scanned_trans->branch_target,
			      text_desc);
	} else {
		_OSDEP_PRINTF("[%d] UDI_PIO_%s",
			      scanned_trans - pio->scanned_trans, text_desc);
	}

	/*
	 * FOR Trace level 3, display ops symbolically when we can. 
	 */
	if (level > 2) {
		switch (dt->op_class) {
		case op_class_immed:
			_OSDEP_PRINTF(" R%d, 0x%x",
				      REGISTER(opcode),
				      scanned_trans->opcode.operand);
			break;
		case op_class_reg:
			_OSDEP_PRINTF(" R%d, R%d",
				      REGISTER(opcode),
				      (udi_ubit16_t)REGISTER(operand));
			break;
		case op_class_branch:
			_OSDEP_PRINTF(" Label %d", (udi_ubit16_t)operand);
			break;
		case op_class_cskip:
			_OSDEP_PRINTF(" Test R%d",
				      (udi_ubit16_t)REGISTER(opcode));
			break;
		case op_class_delay:
			_OSDEP_PRINTF(" %d usec", (udi_ubit16_t)operand);
			break;
		default:
			break;
		}
	}
	_OSDEP_PRINTF("\n");
}
#else
#define _udi_pio_dumpmem(p, sz)
#define _udi_pio_dumpreg(regs)
#define _udi_pio_dumpregs(regs)
#define _udi_pio_trace_ops(a,b,c)
#endif /* _UDI_SUPPRESS_PIO_DEBUGGING */

/*
 * Called for the very specific case where we need to sign-extend a pseudo
 * register that's narrower than a native word.
 */
STATIC _OSDEP_PIO_LONGEST_STYPE
_udi_pio_sign_extend(_udi_pio_register_t *reg,
		     int size)
{
	_OSDEP_PIO_LONGEST_STYPE rval;

	switch (size) {
	case 1:
		rval = (udi_sbit8_t)reg->r.payload[0];
		break;
	case 2:
		rval = (udi_sbit16_t)reg->r.payload[0];
		break;
	case 4:
		rval = (udi_sbit32_t)reg->r.payload[0];
		break;
	default:
		rval = (_OSDEP_PIO_LONGEST_STYPE) reg->r.payload[0];
	}

	return rval;
}

/*
 * Returns the appropriate address to decode a class A op.
 * I'm not happy with this calling convention.   Someday, I may collapse
 * it into the main table just as constants expanded in-line to improve
 * register allocation and to keep more things right in the optimized
 * "jump here on this opcode" path.
 */

STATIC udi_ubit8_t *
_udi_pio_get_address(int opcode,
		     _udi_pio_register_t *mem_reg,
		     void *mem,
		     void *scratch,
		     void *buf)
{
	extern void *_udi_buf_getmemaddr(udi_buf_t *src_buf);
	udi_ubit8_t *target_mem;
	udi_ubit32_t offset;

	switch (ADDRESSMODE(opcode)) {
	case UDI_PIO_DIRECT:
		target_mem = (udi_ubit8_t *)mem_reg->r.payload;
		return target_mem;
	case UDI_PIO_SCRATCH:
		target_mem = (udi_ubit8_t *)scratch;
		break;
	case UDI_PIO_BUF:
		target_mem = _udi_buf_getmemaddr(buf);
		break;
	case UDI_PIO_MEM:
		target_mem = (udi_ubit8_t *)mem;
		break;
	default:
		/*
		 * Provably impossible. 
		 */
		_OSDEP_ASSERT(0);
	}

	offset = mem_reg->r.payload[0];
	return target_mem + offset;
}

static int _udi_pio_debug_level = 0;

udi_status_t
_udi_execute_translist(_udi_pio_register_t *registers,
		       udi_ubit16_t *result,
		       _udi_pio_handle_t *pio,
		       udi_buf_t *buf,
		       void *mem,
		       void *scratch,
		       udi_index_t start_label)
{
	_udi_pio_register_t *tmpreg = &registers[8];
	_udi_pio_trans_t *scanned_trans = pio->trans_entry_pt[start_label];
	_udi_env_pio_trans_t *translist = &scanned_trans->opcode;
	udi_status_t retval = UDI_OK;
	int current_trace_level = _udi_pio_debug_level;
	_udi_pops_t *pops = pio->pio_ops;

	_OSDEP_ASSERT(start_label <= 7);

	/*
	 * The 'mem' pointer must have been returned by udi_mem_alloc.
	 * this doesn't prove it conclusively, but it's a start.
	 * We intentionally cut alignment slack on tiny allocations.
	 */
	_OSDEP_ASSERT(0 == (pio->length < sizeof (long) ? 0 :
			    ((long)mem & (sizeof (long) - 1))));
	_OSDEP_ASSERT(0 == (pio->length < sizeof (long) ? 0 :
			    ((long)mem & (sizeof (void *) - 1))));

	/*
	 * Prove that scratch in this CB is suitably aligned..
	 */
	_OSDEP_ASSERT(IS_ALIGNED(scratch, long));

	/*
	 * Unaligned accesses are currently not supported.
	 */
	_OSDEP_ASSERT(0 == (pio->flags & UDI_PIO_UNALIGNED));

#if PIO_DEBUG && 1
	/*
	 * Make it easier to spot bad register reads 
	 */
	udi_memset(registers, 0xdd, 8 * sizeof (_udi_pio_register_t));
	{
		int i;

		for (i = 8; i-- > 0;)
			registers[i].bytes_valid = 0;
	}
#endif

	/*
	 * Note any return from this function requires an UNMAP 
	 */
	_OSDEP_PIO_MAP(pio);

	/*
	 * Forcibly terminate when our "program counter" meets or 
	 * exceeds the end of the trans_list.
	 */

	for (; scanned_trans;
	     scanned_trans = scanned_trans->next_trans,
	     translist = &scanned_trans->opcode) {
		_OSDEP_PIO_LONGEST_STYPE operand;
		_udi_pio_register_t *pseudoreg, *operandreg;
		pio_s_register_t pseudo_payload, operand_payload;
		udi_ubit32_t target_addr;
		udi_index_t tran_size_idx;
		udi_ubit8_t *host_mem;
		udi_ubit8_t opcode, tran_size, tran_size_words;
		udi_boolean_t take_cskip;
		unsigned int i, overflow;	/* Misc temps */
		pio_s_register_t s_overflow; /* for overflow in 'wide' ops */
		pio_s_register_t digit;	/* tmp for 'wide' ops */
		_OSDEP_PIO_LONGEST_UTYPE word_mask;

		/*
		 * FIXME: It should ultimately be possible to hoist this 
		 * out of the loop but it's modified in the loop...
		 */
		udi_ubit32_t pio_addr = 0;

	      top:
		opcode = translist->pio_op;
		operand = translist->operand;
		tran_size_idx = translist->tran_size;
		tran_size = scanned_trans->tran_size_bytes;
		tran_size_words = tran_size / sizeof (pio_s_register_t);

		pseudoreg = &registers[REGISTER(opcode)];
		operandreg = &registers[REGISTER(operand)];
		pseudo_payload = pseudoreg->r.payload[0];
		operand_payload = operandreg->r.payload[0];
		word_mask =
			_udi_pio_mask_table[scanned_trans->opcode.tran_size];

		if (AM_TRACING(UDI_PIO_TRACE_REGS, 3)) {
			_udi_pio_dumpregs(registers);
		}

		if (AM_TRACING(UDI_PIO_TRACE_OPS, 3)) {
			_udi_pio_trace_ops(current_trace_level & 3,
					   pio, scanned_trans);
		}

		switch (scanned_trans->fast_opcode) {
			/*
			 * Branch targets are computed by 
			 * prescanner. Just reload translist and
			 * evaluate it.
			 */

		case _UDI_PIO_BRANCH:
			scanned_trans = scanned_trans->branch_target;
			translist = &scanned_trans->opcode;
			goto top;

			/*
			 * These pio_ops have very different bit encodings
			 * of operand than the rest do.
			 * Right now, I'm going to smoosh both cases together
			 * and unroll them at the end.   Maybe it's best to
			 * just duplicate them once the dust has settled...
			 */
		case _UDI_PIO_REP_IN_IND:
		case _UDI_PIO_REP_OUT_IND:{
				udi_ubit32_t phys_stride, addr_stride, count;
				_udi_pio_register_t *memreg, *pioreg, *cntreg;
				udi_ubit8_t *host_mem;
				udi_ubit32_t pio_addr;

				/*
				 * These could actually be computed by the
				 * prescanner...
				 */
				phys_stride = PIO_STRIDE(operand) == 0 ? 0 :
					tran_size *
					(1 << (PIO_STRIDE(operand) - 1));
				addr_stride = MEM_STRIDE(operand) == 0 ? 0 :
					tran_size *
					(1 << (MEM_STRIDE(operand) - 1));
				memreg = &registers[MEM_REG(operand)];
				pioreg = &registers[PIO_REG(operand)];
				cntreg = &registers[CNT_REG(operand)];

				/*
				 * Note: in this case, get addressing mode
				 * from operand rather than opcode.
				 */
				host_mem =
					_udi_pio_get_address(operand, memreg,
							     mem, scratch,
							     buf);
				if (ADDRESSMODE(opcode) == UDI_PIO_DIRECT)
					addr_stride = 0;
				pio_addr = pioreg->r.payload[0];
				count = cntreg->r.payload[0];

				if (scanned_trans->fast_opcode ==
				    _UDI_PIO_REP_OUT_IND) {
					while (count--) {
						(*pops->std_ops.
						 pio_write_ops[tran_size_idx])

							(pio_addr, host_mem,
							 tran_size, pio);
						if (AM_TRACING
						    (UDI_PIO_TRACE_DEV, 3)) {
							_udi_pio_dumpmem
								(host_mem,
								 tran_size);
							_OSDEP_PRINTF
								(" UDI_PIO_REP_OUT_IND "
								 " Written from host (0x%p)"
								 " to pio(0x%x)\n",
								 host_mem,
								 pio_addr);
						}
						host_mem += addr_stride;
						pio_addr += phys_stride;
					}
				} else {
					while (count--) {
						(*pops->dir_ops.
						 pio_read_ops[tran_size_idx])
							(pio_addr, host_mem,
							 tran_size, pio);

						if (AM_TRACING
						    (UDI_PIO_TRACE_DEV, 3)) {
							_udi_pio_dumpmem
								(host_mem,
								 tran_size);
							_OSDEP_PRINTF
								(" UDI_PIO_REP_IN_IND "
								 " Read from pio(0x%x)"
								 " to host(0x%p)\n",
								 pio_addr,
								 host_mem);
						}
						host_mem += addr_stride;
						pio_addr += phys_stride;
					}
				}

				break;
			}

		case _UDI_PIO_BARRIER:
			_OSDEP_PIO_BARRIER(pio, operand);
			break;

		case _UDI_PIO_SYNC:
			_OSDEP_PIO_SYNC(pio, tran_size_idx, operand);
			break;

		case _UDI_PIO_SYNC_OUT:
			_OSDEP_PIO_SYNC_OUT(pio, tran_size_idx, operand);
			break;

		case _UDI_PIO_END_IMM:
			/*
			 * Since operand is in native endianness, 
			 * we do not have to swap it.  
			 */
			*result = operand;
			goto end;

		case _UDI_PIO_END:
			*result = operand_payload;
			goto end;

		case _UDI_PIO_LABEL:
			/*
			 * A NOP 
			 */
			break;
		case _UDI_PIO_LOAD_IMM:
			pseudoreg->bytes_valid = tran_size;
			pseudoreg->r.payload[0] = operand;
			break;

		case _UDI_PIO_LOAD_IMM_WIDE:
			/*
			 * Slurp up pairs of continuation opcodes.
			 * Remember that we want to store things
			 * internally in natural endianness.
			 * This code is substantially complicated
			 * by LONGEST_TYPE being variably sized.
			 * this code could be improved by getting the
			 * 'am I capable of doing long long but should
			 * not becuase i'm doing a shorter load' test
			 * into its own case.
			 * This code is somewhat funny looking to 
			 * avoid warnings on 32-bit systems about
			 * shifting over the size of an int.
			 */
			for (i = 0;;) {
				_OSDEP_PIO_LONGEST_UTYPE scratch_long;
				_OSDEP_PIO_LONGEST_UTYPE scratch[4];

				scratch[0] = translist->operand;
				scanned_trans = scanned_trans->next_trans;
				translist = &scanned_trans->opcode;

				scratch[1] = translist->operand;
				scratch_long = scratch[1] << 16 | scratch[0];

				if ((tran_size > sizeof (scratch_long))
				    && sizeof (scratch_long) > 4) {

					scanned_trans =
						scanned_trans->next_trans;
					translist = &scanned_trans->opcode;

					scratch[2] = translist->operand;
					scanned_trans =
						scanned_trans->next_trans;
					translist = &scanned_trans->opcode;

					scratch[3] = translist->operand;
					scratch_long = scratch[2] |
						scratch[3] << 16;

					scratch_long = scratch_long << 16;
					scratch_long = scratch_long << 16;
					scratch_long |= scratch[0] |
						scratch[1] << 16;

				}


				pseudoreg->r.payload[i /
						     sizeof (scratch_long)] =
					scratch_long;

				if ((i += sizeof (scratch_long)) >= tran_size)
					break;
				scanned_trans = scanned_trans->next_trans;

				translist = &scanned_trans->opcode;
			}
			pseudoreg->bytes_valid = tran_size;
			break;

		case _UDI_PIO_IN_IND:
			target_addr = (udi_ubit32_t)operand_payload;

			(*pops->dir_ops.pio_read_ops[tran_size_idx])
				(target_addr, pseudoreg->r.payload, tran_size,
				 pio);
			pseudoreg->r.payload[0] &= word_mask;
			pseudoreg->bytes_valid = tran_size;
			if (AM_TRACING(UDI_PIO_TRACE_DEV, 3)) {
				_udi_pio_dumpreg(pseudoreg);
				_OSDEP_PRINTF
					(" UDI_PIO_IN_IND Read from pio(%x)\n",
					 target_addr);
			}
			break;

		case _UDI_PIO_OUT_IND:
			target_addr = (udi_ubit32_t)operand_payload;

			(*pops->std_ops.pio_write_ops[tran_size_idx])
				(target_addr, pseudoreg->r.payload, tran_size,
				 pio);
			if (AM_TRACING(UDI_PIO_TRACE_DEV, 3)) {
				_udi_pio_dumpreg(pseudoreg);
				_OSDEP_PRINTF
					(" UDI_PIO_OUT_IND Written to pio(0x%x)\n",
					 target_addr);
			}
			break;

		case _UDI_PIO_SHIFT_LEFT:
			pseudoreg->bytes_valid = tran_size;
			pseudoreg->r.payload[0] <<= operand;
			pseudoreg->r.payload[0] &= word_mask;
			break;

		case _UDI_PIO_SHIFT_LEFT_32:
			/*
			 * Carefully do an overlapping copy, shoving
			 * all the member words up one position.
			 */
			for (i = tran_size_words - 1; i > 0; i--) {
				if (i * sizeof (operand) >
				    pseudoreg->bytes_valid)
					pseudoreg->r.payload[i] = 0;
				else
					pseudoreg->r.payload[i] =
						pseudoreg->r.payload[i - 1];
			}
			/*
			 * To avoid complication (and icky tests) on systems
			 * where the shift by 32 may result in ISO undefined
			 * behaviour (register size == 32 bits) we just do it
			 * as two shifts here.   The single opcode penalty
			 * is worth the simplicity.
			 */
			pseudoreg->r.payload[0] <<= 16;
			pseudoreg->r.payload[0] <<= 16;
			pseudoreg->bytes_valid = tran_size;
			break;

		case _UDI_PIO_SHIFT_LEFT_WIDE:{
				int shift_cnt;
				_OSDEP_PIO_LONGEST_UTYPE mask;

				shift_cnt = sizeof (mask) * 8 - operand;
				mask = -((_OSDEP_PIO_LONGEST_UTYPE)1 << shift_cnt);
				i = tran_size_words;

				while (--i) {
					if (pseudoreg->bytes_valid <
					    i *
					    sizeof (pseudoreg->r.payload[0])) {
						pseudoreg->r.payload[i - 1] =
							0;
						pseudoreg->r.payload[i] = 0;
					}
					digit = pseudoreg->r.payload[i -
								     1] & mask;
					pseudoreg->r.payload[i] <<= operand;
					digit >>= shift_cnt;
					pseudoreg->r.payload[i] |= digit;
				}
				pseudoreg->r.payload[i] <<= operand;
				pseudoreg->bytes_valid = tran_size;
			}
			break;

		case _UDI_PIO_SHIFT_RIGHT:
			pseudoreg->bytes_valid = tran_size;
			pseudoreg->r.payload[0] &= word_mask;
			pseudoreg->r.payload[0] = (pio_u_register_t) 
				pseudoreg->r.payload[0] >> operand;
			break;

		case _UDI_PIO_SHIFT_RIGHT_32:
			/*
			 * Carefully do an overlapping copy, shoving
			 * all the member words up one position.
			 */
			for (i = 0; i < tran_size_words - 1; i++) {
				if (i * sizeof (operand) >
				    pseudoreg->bytes_valid)
					pseudoreg->r.payload[i] = 0;
				else
					pseudoreg->r.payload[i] =
						pseudoreg->r.payload[i + 1];
			}
			/*
			 * To avoid complication (and icky tests) on systems
			 * where the shift by 32 may result in ISO undefined
			 * behaviour (register size == 32 bits) we just do it
			 * as two shifts here.   The single opcode penalty
			 * is worth the simplicity.
			 */
			pseudoreg->r.payload[i] >>= 16;
			pseudoreg->r.payload[i] >>= 16;
			pseudoreg->bytes_valid = tran_size;
			break;

		case _UDI_PIO_SHIFT_RIGHT_WIDE:{
				int shift_cnt;
				_OSDEP_PIO_LONGEST_UTYPE mask;

				shift_cnt = sizeof (mask) * 8 - operand;
				mask = ((_OSDEP_PIO_LONGEST_UTYPE) 1 <<
					operand) - 1;

				for (i = 0; i < tran_size_words - 1; i++) {
					if (pseudoreg->bytes_valid <
					    i *
					    sizeof (pseudoreg->r.payload[0])) {
						pseudoreg->r.payload[i] = 0;
						pseudoreg->r.payload[i + 1] =
							0;
						digit = 0;
					} else {
						digit = pseudoreg->r.
							payload[i + 1] & mask;
					}
					/*
					 * Cast to be sure we don't sign-propagate
					 * (sign-preserve) since we're handling 
					 * that ourselves and our internal payloads
					 * are explictly of a signed type.
					 */
					pseudoreg->r.payload[i] =
						(_OSDEP_PIO_LONGEST_UTYPE)
						pseudoreg->r.
						payload[i] >> operand;

					digit <<= shift_cnt;
					pseudoreg->r.payload[i] |= digit;
				}
				pseudoreg->r.payload[i] = (pio_u_register_t)
					pseudoreg->r.payload[i] >> operand;
				pseudoreg->bytes_valid = tran_size;
			}
			break;

		case _UDI_PIO_AND:
			pseudoreg->bytes_valid = tran_size;
			pseudoreg->r.payload[0] &= operand_payload;
			pseudoreg->r.payload[0] &= word_mask;
			break;

		case _UDI_PIO_AND_WIDE:
			/*
			 * No icky carries needed.    Simply loop.
			 */
			for (i = 0; i < tran_size_words; i++) {
				if (operandreg->bytes_valid < tran_size)
					operandreg->r.payload[i] = 0;
				pseudoreg->r.payload[i] &=
					operandreg->r.payload[i];
			}
			pseudoreg->bytes_valid = tran_size;
			break;

		case _UDI_PIO_AND_IMM_WIDE:
			for (i = 1; i < tran_size_words; i++) {
				pseudoreg->r.payload[i] = 0;
			}
			/*
			 * FALLTHRU 
			 */
		case _UDI_PIO_AND_IMM:
			pseudoreg->bytes_valid = tran_size;
			pseudoreg->r.payload[0] &= operand;
			pseudoreg->r.payload[0] &= word_mask;
			break;

		case _UDI_PIO_OR:
			pseudoreg->bytes_valid = tran_size;
			pseudoreg->r.payload[0] |= operand_payload;
			pseudoreg->r.payload[0] &= word_mask;
			break;

		case _UDI_PIO_OR_WIDE:
			/*
			 * No icky carries needed.    Simply loop.
			 */
			for (i = 0; i < tran_size_words; i++) {
				if (operandreg->bytes_valid < tran_size)
					operandreg->r.payload[i] = 0;
				pseudoreg->r.payload[i] |=
					operandreg->r.payload[i];
			}
			pseudoreg->bytes_valid = tran_size;
			break;

		case _UDI_PIO_OR_IMM_WIDE:
			for (i = 1; i < tran_size_words; i++) {
				pseudoreg->r.payload[i] = 0;
			}
			/*
			 * FALLTHRU 
			 */

		case _UDI_PIO_OR_IMM:
			pseudoreg->bytes_valid = tran_size;
			pseudoreg->r.payload[0] |= operand;
			pseudoreg->r.payload[0] &= word_mask;
			break;

		case _UDI_PIO_XOR:
			pseudoreg->bytes_valid = tran_size;
			pseudoreg->r.payload[0] ^= operand_payload;
			pseudoreg->r.payload[0] &= word_mask;
			break;

		case _UDI_PIO_XOR_WIDE:
			/*
			 * No icky carries needed.    Simply loop.
			 */
			for (i = 0; i < tran_size_words; i++) {
				if (operandreg->bytes_valid < tran_size)
					operandreg->r.payload[i] = 0;
				pseudoreg->r.payload[i] ^=
					operandreg->r.payload[i];
			}
			pseudoreg->bytes_valid = tran_size;
			break;

		case _UDI_PIO_ADD:
			pseudoreg->bytes_valid = tran_size;
			pseudoreg->r.payload[0] += operand_payload;
			pseudoreg->r.payload[0] &= word_mask;
			break;

		case _UDI_PIO_ADD_IMM_WIDE:
			/*
			 *  Populate tmpreg with sign-extended constant, 
			 * then branch to common adder code.
			 */
			tmpreg->r.payload[0] = (udi_sbit16_t)operand;
			for (i = 1; i < tran_size_words; i++) {
				tmpreg->r.payload[i] =
					tmpreg->r.payload[0] < 0 ? -1 : 0;
			}
			tmpreg->bytes_valid = tran_size;

			operandreg = tmpreg;
			/*
			 * FALLTHRU 
			 */

		case _UDI_PIO_ADD_WIDE:
			/*
			 * We may need to zero extend either operandreg
			 * or pseudoreg in this operation.
			 */
			s_overflow = 0;
			for (i = 0; i < tran_size_words; i++) {
				if (operandreg->bytes_valid <=
				    i * sizeof (operandreg->r.payload[0]))
					operandreg->r.payload[i] = 0;
				if (pseudoreg->bytes_valid <=
				    i * sizeof (pseudoreg->r.payload[0]))
					pseudoreg->r.payload[i] = 0;
				digit = operandreg->r.payload[i] +
					pseudoreg->r.payload[i] + s_overflow;
				s_overflow = digit <
					s_overflow + operandreg->r.payload[i];
				pseudoreg->r.payload[i] = digit;
			}
			pseudoreg->bytes_valid = tran_size;
			break;

		case _UDI_PIO_ADD_IMM:
			/*
			 * This one treats operand as signed.
			 */
			pseudoreg->bytes_valid = tran_size;
			pseudoreg->r.payload[0] += (udi_sbit16_t)operand;
			pseudoreg->r.payload[0] &= word_mask;
			break;

		case _UDI_PIO_SUB:
			pseudoreg->bytes_valid = tran_size;
			pseudoreg->r.payload[0] -= operand_payload;
			pseudoreg->r.payload[0] &= word_mask;
			break;

		case _UDI_PIO_SUB_WIDE:
			/*
			 * We may need to zero extend either operandreg
			 * or pseudoreg in this operation.
			 * We're actually computing underflow here, but
			 * we call it "overflow" just to make you appreciate
			 * this code. 
			 */
			overflow = 0;
			for (i = 0; i < tran_size_words; i++) {
				if (operandreg->bytes_valid <=
				    i * sizeof (operandreg->r.payload[0]))
					operandreg->r.payload[i] = 0;
				if (pseudoreg->bytes_valid <=
				    i * sizeof (operandreg->r.payload[0]))
					pseudoreg->r.payload[i] = 0;
				digit = pseudoreg->r.payload[i] -
					operandreg->r.payload[i];

/* FIXME: THis underflow detection is wrong. */
				overflow = digit > operandreg->r.payload[i];
				digit = digit - overflow;
				pseudoreg->r.payload[i] = digit;
			}
			pseudoreg->bytes_valid = tran_size;
			break;

		case _UDI_PIO_CSKIP_Z:
			/*
			 * Simple cases are handled simply. 
			 */
			if ((pseudo_payload & word_mask) == 0) {
				scanned_trans = scanned_trans->next_trans;
			}
			break;

		case _UDI_PIO_CSKIP_Z_WIDE:

			/*
			 * A big register is zero iff all the component
			 * words are zero.
			 */
			take_cskip = TRUE;
			for (i = 0; i < tran_size_words; i++) {
				if (pseudoreg->bytes_valid <=
				    i * sizeof (pseudoreg->r.payload[0]))
					pseudoreg->r.payload[i] = 0;
				if (pseudoreg->r.payload[i] != 0) {
					take_cskip = FALSE;
					break;
				}
			}
			if (take_cskip) {
				scanned_trans = scanned_trans->next_trans;
			}
			break;

		case _UDI_PIO_CSKIP_NZ:
			if ((pseudo_payload & word_mask) != 0) {
				scanned_trans = scanned_trans->next_trans;
			}
			break;

		case _UDI_PIO_CSKIP_NZ_WIDE:
			/*
			 * A big register is non-zero iff all the component
			 * words are zero.
			 */
			take_cskip = FALSE;
			for (i = 0; i < tran_size_words; i++) {
				if (pseudoreg->bytes_valid <=
				    i * sizeof (pseudoreg->r.payload[0]))
					pseudoreg->r.payload[i] = 0;
				if (pseudoreg->r.payload[i] != 0) {
					take_cskip = TRUE;
					break;
				}
			}
			if (take_cskip) {
				scanned_trans = scanned_trans->next_trans;
			}
			break;


		case _UDI_PIO_CSKIP_NEG:
			pseudo_payload = _udi_pio_sign_extend(pseudoreg,
							      tran_size);
			if (pseudo_payload < 0) {
				scanned_trans = scanned_trans->next_trans;
			}
			break;

		case _UDI_PIO_CSKIP_NNEG:
			pseudo_payload = _udi_pio_sign_extend(pseudoreg,
							      tran_size);
			if (pseudo_payload >= 0) {
				scanned_trans = scanned_trans->next_trans;
			}
			break;
		case _UDI_PIO_IN_DIRECT:
			host_mem = _udi_pio_get_address(opcode, pseudoreg,
							mem, scratch, buf);
			pio_addr += operand;
			(*pops->std_ops.pio_read_ops[tran_size_idx])
				(pio_addr, &pseudoreg->r.payload[0],
				 tran_size, pio);
			pseudoreg->r.payload[0] &= word_mask;
			pseudoreg->bytes_valid = tran_size;
			if (AM_TRACING(UDI_PIO_TRACE_DEV, 3)) {
				_udi_pio_dumpreg(pseudoreg);
				_OSDEP_PRINTF
					(" UDI_PIO_IN DIRECT Read from pio(%x)\n",
					 pio_addr);
			}
			break;
		case _UDI_PIO_IN:
			host_mem = _udi_pio_get_address(opcode, pseudoreg,
							mem, scratch, buf);
			pio_addr += operand;

			(*pops->std_ops.pio_read_ops[tran_size_idx])
				(pio_addr, &tmpreg->r.payload[0],
				 tran_size, pio);
			switch (tran_size) {
			case 1:
				*(udi_ubit8_t *)host_mem =
					tmpreg->r.payload[0];
				break;
			case 2:
				*(udi_ubit16_t *)host_mem =
					tmpreg->r.payload[0];
				break;
			case 4:
				*(udi_ubit32_t *)host_mem =
					tmpreg->r.payload[0];
				break;
			default:{
					pio_u_register_t *hm =
						(pio_u_register_t *)host_mem;
					MEM_ITERATE {
						*hm++ = tmpreg->r.payload[i];
					}
				}
			}
			pseudoreg->bytes_valid = tran_size;
			pseudoreg->r.payload[0] &= word_mask;
			if (AM_TRACING(UDI_PIO_TRACE_DEV, 3)) {
				_udi_pio_dumpmem(host_mem, tran_size);
				_OSDEP_PRINTF
					(" UDI_PIO_IN Read from pio(%x)\n",
					 pio_addr);
			}
			break;
		case _UDI_PIO_OUT:
			host_mem = _udi_pio_get_address(opcode, pseudoreg,
							mem, scratch, buf);
			pio_addr += operand;
			(*pops->std_ops.pio_write_ops[tran_size_idx])
				(pio_addr, host_mem, tran_size, pio);
			if (AM_TRACING(UDI_PIO_TRACE_DEV, 3)) {
				_udi_pio_dumpmem(host_mem, tran_size);
				_OSDEP_PRINTF
					(" UDI_PIO_OUT Written to pio(0x%x)\n",
					 pio_addr);
			}
			break;

		case _UDI_PIO_LOAD:
			host_mem = _udi_pio_get_address(opcode, pseudoreg,
							mem, scratch, buf);
			VERIFY_ALIGNED(host_mem, pio_u_register_t);

			/*
			 * This is a direct load from local memory.  No PIO 
			 * transactions (inb/outb/pacing/delay/swapping) 
			 * can be generated but we still can't just copy 
			 * the data.  Since our internal register set is
			 * represented as an internal array of native endian
			 * words arranged in a big-endian order, and we're
			 * handling the cases that don't fit into a native
			 * word, we may have to swap them end-for-end.
			 */
			MEM_ITERATE {
				operandreg->r.payload[i] =
					*(pio_u_register_t *)host_mem;
				if (AM_TRACING(UDI_PIO_TRACE_DEV, 3)) {
					_OSDEP_PRINTF("LOADING ");
					_udi_pio_dumpreg(operandreg);
					_OSDEP_PRINTF
						(" [%d] from %p (%d bytes)\n",
						 i, host_mem, tran_size);
				}
				host_mem += sizeof (pio_u_register_t);

			}

#if PIO_DEBUG
			_OSDEP_PRINTF("LOADED **** %x from %p (%d bytes)\n",
				      operand_payload, host_mem, tran_size);
#endif
			operandreg->bytes_valid = tran_size;
			operandreg->r.payload[0] &= word_mask;

			break;

		case _UDI_PIO_LOAD_1:
			host_mem = _udi_pio_get_address(opcode, pseudoreg,
							mem, scratch, buf);
			/*
			 * No need to check alignment 
			 */
			operandreg->r.payload[0] = *(udi_ubit8_t *)host_mem;
			operandreg->bytes_valid = tran_size;
			break;

		case _UDI_PIO_LOAD_2:
			host_mem = _udi_pio_get_address(opcode, pseudoreg,
							mem, scratch, buf);
			VERIFY_ALIGNED(host_mem, udi_ubit16_t);

			operandreg->r.payload[0] = *(udi_ubit16_t *)host_mem;
			operandreg->bytes_valid = tran_size;
			break;

		case _UDI_PIO_LOAD_4:
			host_mem = _udi_pio_get_address(opcode, pseudoreg,
							mem, scratch, buf);
			VERIFY_ALIGNED(host_mem, udi_ubit32_t);

			operandreg->r.payload[0] = *(udi_ubit32_t *)host_mem;
			operandreg->bytes_valid = tran_size;
			break;

		case _UDI_PIO_LOAD_DIR:
			operandreg->r.payload[0] = pseudo_payload & word_mask;
			operandreg->bytes_valid = tran_size;
			if (AM_TRACING(UDI_PIO_TRACE_DEV, 3)) {
				_OSDEP_PRINTF("R%d = ", REGISTER(opcode));
				_udi_pio_dumpreg(operandreg);
				_OSDEP_PRINTF("\n");
			}
			break;

		case _UDI_PIO_LOAD_DIR_WIDE:
			/* Internally zero-extend source register */
			while (tran_size > pseudoreg->bytes_valid) {
				pseudoreg->r.payload[pseudoreg->bytes_valid /
						      sizeof
						      (pio_u_register_t)] = 0;
				pseudoreg->bytes_valid +=
					sizeof (pseudoreg->r.payload[0]);
			}
			
			for (i = 0; i < tran_size_words; i++) {
				operandreg->r.payload[i] = pseudoreg->r.payload[i];
			}
			if (AM_TRACING(UDI_PIO_TRACE_DEV, 3)) {
				_OSDEP_PRINTF("R%d = ", REGISTER(opcode));
				_udi_pio_dumpreg(operandreg);
				_OSDEP_PRINTF("\n");
			}
			break;


		case _UDI_PIO_STORE_DIR:
			pseudoreg->r.payload[0] = operand_payload & word_mask;
			pseudoreg->bytes_valid = tran_size;
			if (AM_TRACING(UDI_PIO_TRACE_DEV, 3)) {
				_OSDEP_PRINTF("R%d = ", REGISTER(opcode));
				_udi_pio_dumpreg(pseudoreg);
				_OSDEP_PRINTF("\n");
			}
			break;

		case _UDI_PIO_STORE_DIR_WIDE:
			/* Internally zero-extend source register */
			while (tran_size > operandreg->bytes_valid) {
				operandreg->r.payload[operandreg->bytes_valid /
						      sizeof
						      (pio_u_register_t)] = 0;
				operandreg->bytes_valid +=
					sizeof (operandreg->r.payload[0]);
			}
			
			for (i = 0; i < tran_size_words; i++) {
				pseudoreg->r.payload[i] = operandreg->r.payload[i];
			}
			if (AM_TRACING(UDI_PIO_TRACE_DEV, 3)) {
				_OSDEP_PRINTF("R%d = ", REGISTER(opcode));
				_udi_pio_dumpreg(pseudoreg);
				_OSDEP_PRINTF("\n");
			}
			break;

		case _UDI_PIO_STORE:
			host_mem = _udi_pio_get_address(opcode, pseudoreg,
							mem, scratch, buf);
			VERIFY_ALIGNED(host_mem, pio_u_register_t);

			/*
			 * Internally zero-extend register before display
			 * or calling POPS function.
			 */
			while (tran_size > operandreg->bytes_valid) {
				operandreg->r.payload[operandreg->bytes_valid /
						      sizeof
						      (pio_u_register_t)] = 0;
				operandreg->bytes_valid +=
					sizeof (operandreg->r.payload[0]);
			}

			MEM_ITERATE {
				if (AM_TRACING(UDI_PIO_TRACE_DEV, 3)) {
					_OSDEP_PRINTF("STORING ");
					_udi_pio_dumpreg(operandreg);
					_OSDEP_PRINTF
						(" [%d] to %p (%d bytes)\n", i,
						 host_mem, tran_size);
				}
				*(pio_u_register_t *)host_mem =
					operandreg->r.payload[i];
				host_mem += sizeof (pio_u_register_t);
			}

			break;

		case _UDI_PIO_STORE_1:
			host_mem = _udi_pio_get_address(opcode, pseudoreg,
							mem, scratch, buf);
			if (AM_TRACING(UDI_PIO_TRACE_DEV, 3)) {
				_OSDEP_PRINTF("Writing %02x\n",
					      (udi_ubit8_t)operand_payload);
			}
			/*
			 * No need to check alignment 
			 */
			*(udi_ubit8_t *)host_mem = operand_payload;
			break;

		case _UDI_PIO_STORE_2:
			host_mem = _udi_pio_get_address(opcode, pseudoreg,
							mem, scratch, buf);
			if (AM_TRACING(UDI_PIO_TRACE_DEV, 3)) {
				_OSDEP_PRINTF("Writing %04x\n",
					      (udi_ubit16_t)operand_payload);
			}
			VERIFY_ALIGNED(host_mem, udi_ubit16_t);

			*(udi_ubit16_t *)host_mem = operand_payload;
			break;

		case _UDI_PIO_STORE_4:
			host_mem = _udi_pio_get_address(opcode, pseudoreg,
							mem, scratch, buf);
			if (AM_TRACING(UDI_PIO_TRACE_DEV, 3)) {
				_OSDEP_PRINTF("Writing %08x\n",
					      (udi_ubit32_t)operand_payload);
			}
			VERIFY_ALIGNED(host_mem, udi_ubit32_t);

			*(udi_ubit32_t *)host_mem = operand_payload;
			break;

		case _UDI_PIO_DELAY:
			/*
			 * Entirely OS-dependent.  It can provide a way
			 * to delay for `operand' microseconds (while 
			 * preserving PIO regset locking semantics) or it
			 * can ignore this completely.
			 */
			_OSDEP_PIO_DELAY(operand);
			break;

		case _UDI_PIO_DEBUG:
			current_trace_level = operand;
			break;

		default:
			/*
			 * This can be proven to never happen. 
			 */
			_OSDEP_ASSERT(0);
			break;

		}			/*
					 * opcode switch 
					 */
	}				/*
					 * opcodes remaining 
					 */

	/*
	 * (*result) is undefined if we fell through here.   UDI_PIO_END{_IMM}
	 * sets it before getting here.
	 * Return whatever retval we currently have. 
	 */
      end:
	_OSDEP_PIO_BARRIER(pio, 0);
	_OSDEP_PIO_UNMAP(pio);
	return (retval);
}

STATIC void
_udi_pio_add_label(_udi_pio_handle_t *pio,
		   _udi_pio_trans_t *scanned_trans)
{
	udi_queue_t *listp = &pio->label_list_head;
	udi_queue_t *element, *tmp;
	unsigned int label = scanned_trans->opcode.operand;

	/*
	 * This could be written to use btree, avl lists, or other
	 * "smarter" instructions, but until proven otherwise, we'll
	 * just keep a sorted linked list of labels.  
	 */

	UDI_QUEUE_FOREACH(listp, element, tmp) {
		_udi_pio_trans_t *st = UDI_BASE_STRUCT(element,
						       _udi_pio_trans_t,
						       label_list);

		if (st->opcode.operand == label) {
			/*
			 * Duplicate label found. 
			 */
			_OSDEP_PRINTF("label = %d at offset %d;"
				      "Previous declaration at offset %d\n",
				      label,
				      scanned_trans - pio->scanned_trans,
				      st - pio->scanned_trans);
			_OSDEP_ASSERT(NULL == "Duplicate label found");
		}
		if (st->opcode.operand > label) {
			UDI_QUEUE_INSERT_BEFORE(&st->label_list,
						&scanned_trans->label_list);
			goto chk_lbl;

		}
	}

	UDI_QUEUE_INSERT_BEFORE(listp, &scanned_trans->label_list);

      chk_lbl:
	if ((label < 8) && (label > 0)) {
		pio->trans_entry_pt[label] = scanned_trans;
	}
#if 0
	UDI_QUEUE_FOREACH(listp, element, tmp) {
		_udi_pio_trans_t *st = UDI_BASE_STRUCT(element,
						       _udi_pio_trans_t,
						       label_list);

		_OSDEP_PRINTF("label list %x\n", st->opcode.operand);
	}
#endif
}

STATIC udi_status_t
_udi_pio_find_label(_udi_pio_handle_t *pio,
		    _udi_pio_trans_t *scanned_trans)
{
	udi_queue_t *listp = &pio->label_list_head;
	udi_queue_t *element, *tmp;

	UDI_QUEUE_FOREACH(listp, element, tmp) {
		_udi_pio_trans_t *st = UDI_BASE_STRUCT(element,
						       _udi_pio_trans_t,
						       label_list);

		/*
		 * On a hit, we stash the opcode after target of the branch 
		 * (we know we branch to a label) and return.
		 */
		if (st->opcode.operand == scanned_trans->opcode.operand) {
			/*
			 * NOTE: a programmer could code multiple labels
			 * in a trans list.  Though they'll be parsed as
			 * NOPs by the executor, we could eat them up 
			 * right here.
			 */
			scanned_trans->branch_target = st->next_trans;
			return UDI_OK;
		}
		/*
		 * Since it's a sorted list, if we stumble across a label
		 * that's greater than the one we're looking for, we can 
		 * bail.
		 */
		if (st->opcode.operand > scanned_trans->opcode.operand) {
			break;
		}
	}
	_OSDEP_PRINTF("Label %d:", scanned_trans->opcode.operand);
	_OSDEP_ASSERT(NULL == "Label not found");
	return UDI_STAT_INVALID_STATE;
}

udi_status_t
_udi_pio_trans_prescan(_udi_pio_handle_t *pio)
{
	unsigned int i, j;

	_udi_pio_trans_t *scanned_trans;
	_udi_pio_trans_t *last_label_seen;

	last_label_seen = pio->scanned_trans;

	/*
	 * First pass over the list.   Go for low-hanging fruit.
	 *   Store labels.
	 *   Validate tran_size.
	 *   Tag ops that require sign extension instead of zero extension.
	 *   Build an efficient decoding of the opcode so we only need to
	 *   switch on it at runtime.
	 */
	for (i = 0; i < pio->trans_length; i++) {
		/*
		 * Copy the original opcode.  (It's short) 
		 */
		udi_pio_trans_t op = pio->trans_list[i];
		udi_ubit8_t tran_size;
		udi_sbit8_t tran_size_idx;
		udi_ubit8_t opcode;
		_udi_pio_opcode_t fastop = -1;

		scanned_trans = &pio->scanned_trans[i];

		/*
		 * tran_size shall be a non-zero power of two less than 
		 * the register size.   
		 */
		tran_size_idx = op.tran_size;
		_OSDEP_ASSERT(tran_size_idx <= 5);
		tran_size = (1 << tran_size_idx);

		scanned_trans->tran_size_bytes = tran_size;
		udi_memcpy(&scanned_trans->opcode, &op, sizeof (op));
		scanned_trans->next_trans = &scanned_trans[1];

		opcode = op.pio_op;

		if (opcode >= _UDI_PIO_ENCODE_CLASS_C(0)) {
			switch (opcode) {
			case UDI_PIO_BRANCH:
				fastop = _UDI_PIO_BRANCH;
				if (op.operand == 0) {
					_udi_env_log_write(_UDI_TREVENT_ENVERR,
							   UDI_LOG_ERROR, 0,
							   UDI_STAT_NOT_UNDERSTOOD,
							   927, i);
				}
				break;
			case UDI_PIO_LABEL:
				fastop = _UDI_PIO_LABEL;
				if (op.operand == 0) {
					_udi_env_log_write(_UDI_TREVENT_ENVERR,
							   UDI_LOG_ERROR, 0,
							   UDI_STAT_NOT_UNDERSTOOD,
							   924, i);
				}
				_udi_pio_add_label(pio, scanned_trans);
				break;
			case UDI_PIO_REP_IN_IND:
				fastop = _UDI_PIO_REP_IN_IND;
				break;
			case UDI_PIO_REP_OUT_IND:
				fastop = _UDI_PIO_REP_OUT_IND;
				break;
			case UDI_PIO_DELAY:
				fastop = _UDI_PIO_DELAY;
				break;
			case UDI_PIO_BARRIER:
				fastop = _UDI_PIO_BARRIER;
				if ((op.operand != 0) &&
				    op.operand != UDI_PIO_OUT) {
					_udi_env_log_write(_UDI_TREVENT_ENVERR,
							   UDI_LOG_ERROR, 0,
							   UDI_STAT_NOT_UNDERSTOOD,
							   928, i);
				}
				break;
			case UDI_PIO_SYNC:
				fastop = _UDI_PIO_SYNC;
				break;
			case UDI_PIO_SYNC_OUT:
				fastop = _UDI_PIO_SYNC_OUT;
				break;
			case UDI_PIO_DEBUG:
				fastop = _UDI_PIO_DEBUG;
				break;

			case UDI_PIO_END:
				_OSDEP_ASSERT(UDI_PIO_2BYTE == op.tran_size);
				fastop = _UDI_PIO_END;
				break;

			case UDI_PIO_END_IMM:
				/*
				 * 1.0 says thou shalt not have a tran
				 * size other than  2 bytes. 
				 */
				_OSDEP_ASSERT(UDI_PIO_2BYTE == op.tran_size);
				fastop = _UDI_PIO_END_IMM;
				break;
			default:
				_OSDEP_ASSERT(0);
				;
			}
		} else if (opcode >= _UDI_PIO_ENCODE_CLASS_B(0)) {
			switch (opcode & 0xf8) {
			case UDI_PIO_LOAD_IMM:
				/*
				 * 1.0 says thou shalt not have a tran
				 * size under 2 bytes. 
				 */
				_OSDEP_ASSERT(tran_size > 1);
				if (tran_size == 2)
					fastop = _UDI_PIO_LOAD_IMM;
				else
					fastop = _UDI_PIO_LOAD_IMM_WIDE;
				break;
			case UDI_PIO_CSKIP:
				switch (op.operand) {
				case UDI_PIO_NEG:
					fastop = _UDI_PIO_CSKIP_NEG;
					break;
				case UDI_PIO_NNEG:
					fastop = _UDI_PIO_CSKIP_NNEG;
					break;
				case UDI_PIO_Z:
					if (tran_size <=
					    sizeof (pio_s_register_t))
						fastop = _UDI_PIO_CSKIP_Z;
					else
						fastop = _UDI_PIO_CSKIP_Z_WIDE;
					break;
				case UDI_PIO_NZ:
					if (tran_size <=
					    sizeof (pio_s_register_t))
						fastop = _UDI_PIO_CSKIP_NZ;
					else
						fastop = _UDI_PIO_CSKIP_NZ_WIDE;
					break;
				default:
					_OSDEP_ASSERT(0);
				}
				break;
			case UDI_PIO_IN_IND:
				fastop = _UDI_PIO_IN_IND;
				break;
			case UDI_PIO_OUT_IND:
				fastop = _UDI_PIO_OUT_IND;
				break;
			case UDI_PIO_SHIFT_LEFT:
				if ((op.operand == 32) &&
				    (sizeof (pio_s_register_t) == 4))
					fastop = _UDI_PIO_SHIFT_LEFT_32;
				else if (tran_size <=
					 sizeof (pio_s_register_t))
					fastop = _UDI_PIO_SHIFT_LEFT;
				else
					fastop = _UDI_PIO_SHIFT_LEFT_WIDE;
				if (op.operand > 32) {
					_udi_env_log_write(_UDI_TREVENT_ENVERR,
							   UDI_LOG_ERROR, 0,
							   UDI_STAT_NOT_UNDERSTOOD,
							   923, i);
				}
				break;
			case UDI_PIO_SHIFT_RIGHT:
				if ((op.operand == 32) &&
				    (sizeof (pio_s_register_t) == 4))
					fastop = _UDI_PIO_SHIFT_RIGHT_32;
				else if (tran_size <=
					 sizeof (pio_s_register_t))
					fastop = _UDI_PIO_SHIFT_RIGHT;
				else
					fastop = _UDI_PIO_SHIFT_RIGHT_WIDE;
				if (op.operand > 32) {
					_udi_env_log_write(_UDI_TREVENT_ENVERR,
							   UDI_LOG_ERROR, 0,
							   UDI_STAT_NOT_UNDERSTOOD,
							   923, i);
				}
				break;
			case UDI_PIO_AND:
				if (tran_size <= sizeof (pio_s_register_t))
					fastop = _UDI_PIO_AND;
				else
					fastop = _UDI_PIO_AND_WIDE;
				break;
			case UDI_PIO_AND_IMM:
				if (tran_size <= sizeof (pio_s_register_t))
					fastop = _UDI_PIO_AND_IMM;
				else
					fastop = _UDI_PIO_AND_IMM_WIDE;
				break;
			case UDI_PIO_OR:
				if (tran_size <= sizeof (pio_s_register_t))
					fastop = _UDI_PIO_OR;
				else
					fastop = _UDI_PIO_OR_WIDE;
				break;
			case UDI_PIO_OR_IMM:
				if (tran_size <= sizeof (pio_s_register_t))
					fastop = _UDI_PIO_OR_IMM;
				else
					fastop = _UDI_PIO_OR_IMM_WIDE;
				break;
			case UDI_PIO_XOR:
				if (tran_size <= sizeof (pio_s_register_t))
					fastop = _UDI_PIO_XOR;
				else
					fastop = _UDI_PIO_XOR_WIDE;
				break;
			case UDI_PIO_ADD:
				if (tran_size <= sizeof (pio_s_register_t))
					fastop = _UDI_PIO_ADD;
				else
					fastop = _UDI_PIO_ADD_WIDE;
				break;
			case UDI_PIO_ADD_IMM:
				if (tran_size <= sizeof (pio_s_register_t))
					fastop = _UDI_PIO_ADD_IMM;
				else
					fastop = _UDI_PIO_ADD_IMM_WIDE;
				break;
			case UDI_PIO_SUB:
				if (tran_size <= sizeof (pio_s_register_t))
					fastop = _UDI_PIO_SUB;
				else
					fastop = _UDI_PIO_SUB_WIDE;
				break;
			default:
				_OSDEP_ASSERT(0);
			}
		} else {
			switch (opcode & 0xe0) {
			case UDI_PIO_IN:
				if (ADDRESSMODE(opcode) == UDI_PIO_DIRECT) {
					fastop = _UDI_PIO_IN_DIRECT;
					break;
				}
				fastop = _UDI_PIO_IN;
				break;
			case UDI_PIO_OUT:
				fastop = _UDI_PIO_OUT;
				break;
			case UDI_PIO_LOAD:
				if (ADDRESSMODE(opcode) == UDI_PIO_DIRECT) {
					if (tran_size <= sizeof (pio_s_register_t))
						fastop = _UDI_PIO_LOAD_DIR;
					else 
						fastop = _UDI_PIO_LOAD_DIR_WIDE;
				} else {
					switch (tran_size) {
					case 1:
						fastop = _UDI_PIO_LOAD_1;
						break;
					case 2:
						fastop = _UDI_PIO_LOAD_2;
						break;
					case 4:
						fastop = _UDI_PIO_LOAD_4;
						break;
					default:
						fastop = _UDI_PIO_LOAD;
						break;
					}
				}
				break;
			case UDI_PIO_STORE:
				if (ADDRESSMODE(opcode) == UDI_PIO_DIRECT) {
					if (tran_size <= sizeof (pio_s_register_t))
						fastop = _UDI_PIO_STORE_DIR;
					else 
						fastop = _UDI_PIO_STORE_DIR_WIDE;
				} else {
					switch (tran_size) {
					case 1:
						fastop = _UDI_PIO_STORE_1;
						break;
					case 2:
						fastop = _UDI_PIO_STORE_2;
						break;
					case 4:
						fastop = _UDI_PIO_STORE_4;
						break;
					default:
						fastop = _UDI_PIO_STORE;
						break;
					}
				}
				break;
			default:
				fastop = -1;
			}
		}
		scanned_trans->fast_opcode = fastop;
	}

	pio->scanned_trans[pio->trans_length - 1].next_trans = NULL;

	/*
	 * Make another pass over the trans list.   
	 * Resolve branches to labels.
	 */
	for (scanned_trans = pio->scanned_trans;
	     scanned_trans; scanned_trans = scanned_trans->next_trans) {
		udi_pio_trans_t *op;

		i = scanned_trans - pio->scanned_trans;
		op = &pio->trans_list[i];

		/*
		 * We reuse the "branch target" slot for non-branch
		 * opcodes to keep a back-pointerto the most recently
		 * seen label.
		 */
		if (scanned_trans->fast_opcode != _UDI_PIO_BRANCH) {
			_OSDEP_ASSERT(scanned_trans->branch_target == NULL);
			scanned_trans->branch_target = last_label_seen;
		}

		switch (scanned_trans->fast_opcode) {
		case _UDI_PIO_BRANCH:
			_udi_pio_find_label(pio, scanned_trans);
			break;
		case _UDI_PIO_IN:
		case _UDI_PIO_OUT:
		case _UDI_PIO_IN_IND:
		case _UDI_PIO_OUT_IND:
		case _UDI_PIO_REP_IN_IND:
		case _UDI_PIO_REP_OUT_IND:
			if ((pio->flags & _UDI_PIO_NEVERSWAP) &&
			    (op->tran_size != UDI_PIO_1BYTE)) {
				_udi_env_log_write(_UDI_TREVENT_ENVERR,
						   UDI_LOG_ERROR, 0,
						   UDI_STAT_NOT_UNDERSTOOD,
						   921, i);
			}
			break;
		case _UDI_PIO_LOAD_IMM_WIDE:
			/*
			 * Walk through potential contuation immediates and
			 * validate arguments as per corrections document. 
			 */
			for (j = 1; j < scanned_trans->tran_size_bytes / 2;
			     j++) {
				if ((op->tran_size !=
				     scanned_trans[j].opcode.tran_size) ||
				    (op->pio_op !=
				     scanned_trans[j].opcode.pio_op)) {

					_udi_env_log_write(_UDI_TREVENT_ENVERR,
							   UDI_LOG_ERROR, 0,
							   UDI_STAT_NOT_UNDERSTOOD,
							   920, i + j,
							   scanned_trans[j].
							   opcode.operand);
				}
			}
			i += j - 1;
			scanned_trans += j - 1;
			break;
		case _UDI_PIO_LABEL:
			last_label_seen = scanned_trans;
			break;
		default:
			break;
		}
	}
	return UDI_OK;
}
#endif /* _UDI_PHYSIO_SUPPORTED */
