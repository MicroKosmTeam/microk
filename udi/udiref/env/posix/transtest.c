/*
 * File: env/posix/transtest.c
 *
 * Excercise the trans-list processor.
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
 * Execute a number of hardware-independent trans_lists for sanity tests.
 * 
 * Note that this does not look very "UDI-ish" at all.   For example, we
 * use global data, abuse control blocks, and so on.   The justification for
 * this is to allow testing of the PIO engine and rely on as little of the 
 * actual UDI plumbing as we can.
 *
 * So if you're looking for sample trans lists, this is an OK example.  If
 * you're trying to pick up the Zen of UDI, you really should close your
 * eyes to a lot of bad habits in this file..
 */


#include <stdlib.h>
#include <unistd.h>

#include <udi_env.h>

#define LATER _i386

#include <posix.h>

#define N_SERIALIZATION_DOMAINS 20	/* Normally from udiprops.txt */
#define PIO_EXIT_SUCCESS 0x1234		/* "magic" number to prove we didn't
					 * abort somewhere in the translist 
					 */
STATIC void compare_very_long_word(int size_in_bytes,
				   udi_ubit32_t *expected,
				   udi_ubit32_t *got);

typedef void translist_test_fn_t(void *membuf);
typedef struct {
	translist_test_fn_t *test_fn;
	udi_ubit32_t regset_idx;
	size_t list_size;
	udi_pio_trans_t *trans_list;
	char *test_name;
	udi_ubit32_t attributes;
	udi_size_t map_length;
	udi_cb_t *gcb;
} test_translist_t;

_OSDEP_EVENT_T test_done;
_OSDEP_SIMPLE_MUTEX_T test_cnt_mutex;
STATIC volatile int test_cnt;

int verbose = 0;

udi_pio_trans_t test_load_list[] = {
	/*
	 * Should both bytes. 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0x0102},
	/*
	 * Should load both bytes, zero extended. (R1 is later
	 * * stored as a 4byte quantity.) 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0xff02},

	/*
	 * Test narrowing of a register.  
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, 0xffff},
	{UDI_PIO_AND_IMM + UDI_PIO_R2, UDI_PIO_1BYTE, 0x00ff},

	/*
	 * R7 is the offset into our storage area. 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, 0x0},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_2BYTE,
	 UDI_PIO_R0},

	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE,
	 UDI_PIO_R1},

	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_2BYTE,
	 UDI_PIO_R2},

	/*
	 * Load and store a Really Big value 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x0e0f},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x0c0d},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x0a0b},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x0809},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x0607},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x0405},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x0203},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x0001},

	/*
	 * Our destination must be suitably aligned. 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, 0x10},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_16BYTE,
	 UDI_PIO_R6},

	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 16},

	/*
	 * Tests for branching. 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0},

	{UDI_PIO_BRANCH, 0, 0x1111},	/* 0: Goto 1 */
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, (udi_ubit16_t)-1},	/* Be sure this isn't executed */

	{UDI_PIO_LABEL, 0, 0x1111},	/* 1: goto 2 */
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_4BYTE, 1},
	{UDI_PIO_BRANCH, 0, 0x2222},

	{UDI_PIO_LABEL, 0, 0x3333},	/* 3: goto 4 */
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_4BYTE, 2},
	{UDI_PIO_BRANCH, 0, 0x4444},

	{UDI_PIO_LABEL, 0, 0x2222},	/* 2: goto 3 */
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_4BYTE, 4},
	{UDI_PIO_BRANCH, 0, 0x3333},

	{UDI_PIO_LABEL, 0, 0x4444},	/* 2: goto 3 */
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_4BYTE, 8},

	/*
	 * If our goto tangle worked, R0 should hold  1+2+3+4.
	 * * Store it in our output table 
	 */
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE,
	 UDI_PIO_R0},

	/*
	 * Create a twisty tangle of relational operators 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0xff},

	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_NZ},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_Z},
	{UDI_PIO_BRANCH, 0, 0x0003},	/* Skip next */
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_LABEL, 0, 0x0003},

	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_NEG},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_NNEG},
	{UDI_PIO_BRANCH, 0, 0x0004},	/* Skip next */
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_LABEL, 0, 0x0004},

	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_2BYTE, UDI_PIO_NNEG},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_2BYTE, UDI_PIO_NEG},
	{UDI_PIO_BRANCH, 0, 0x0005},	/* Skip next */
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_LABEL, 0, 0x0005},

	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0},
	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_2BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_DELAY, 0, 1000},	/* Delay for 1000 usec */

	/*
	 * Prove that use of a register as both src and operand is OK 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, 1 * 42 * 4},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE,
	 UDI_PIO_R7},

	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, PIO_EXIT_SUCCESS},
};

void
lprintf(const char *format,
	...)
{
	va_list args;

	va_start(args, format);
	if (verbose)
		vprintf(format, args);
	va_end(args);
}

void
test_load_imm(void *buf)
{
	unsigned int i;
	udi_ubit32_t *b = (udi_ubit32_t *)buf;
	udi_ubit16_t *s = (udi_ubit16_t *)buf;

	for (i = 0; i < 44; i++) {
		lprintf("%d [%08x]\n", i, b[i]);
	}

	/*
	 * Endian independent: assert(b[0] == 0xcaca0102); 
	 */
	assert(s[0] == 0x0102);
	assert(s[1] == 0xcaca);

	assert(b[1] == 0x0000ff02);
	/*
	 * endian indepdndent: assert(b[2] == 0xcaca00ff); 
	 */
	assert(s[4] == 0x00ff);
	assert(s[5] == 0xcaca);

	{
		udi_ubit32_t expect[] = {
			0x00010203,
			0x04050607,
			0x08090a0b,
			0x0c0d0e0f
		};

		compare_very_long_word(16, expect, &b[4]);
	}

	assert(b[8] == 15);
	assert(b[42] == 42 * 4);
	udi_mem_free(buf);
}

udi_pio_trans_t test_math_list[] = {
	/*
	 * R7 is the offset into our storage area. 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, 0x0},

	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0x1234},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0x1234},
	/*
	 * 1234 << 4 == 0x12340 
	 */
	{UDI_PIO_SHIFT_LEFT + UDI_PIO_R0, UDI_PIO_4BYTE, 0x4},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	/*
	 * shift it back to the right 
	 */
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R0, UDI_PIO_4BYTE, 8},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	/*
	 * Test narrowing 
	 */
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R0, UDI_PIO_1BYTE, 0},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	/*
	 * Test add immediate one. 
	 */
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_4BYTE, 1},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	/*
	 * Test or immediate. 
	 */
	{UDI_PIO_OR_IMM + UDI_PIO_R0, UDI_PIO_4BYTE, 0x400},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	/*
	 * Test 'and'. 
	 */
	{UDI_PIO_AND + UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	/*
	 * Test a simple subtraction.  0x1234 - 0x34 = 0.  
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0x1234},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0x34},

	{UDI_PIO_SUB + UDI_PIO_R0, UDI_PIO_2BYTE, UDI_PIO_R1},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	/*
	 * Test exclusive or.  0x1200 ^0x234 = 0x1034. 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0x0234},
	{UDI_PIO_XOR + UDI_PIO_R0, UDI_PIO_2BYTE, UDI_PIO_R1},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	/*
	 * Add 0x100 back in. 0x1034 + 0x100 = 0x1134. 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0x100},
	{UDI_PIO_ADD + UDI_PIO_R0, UDI_PIO_2BYTE, UDI_PIO_R1},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	/*
	 * Subtract 0x1134 back out.   Should be zero. 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, (udi_ubit16_t)-0x1134},
	{UDI_PIO_ADD + UDI_PIO_R0, UDI_PIO_2BYTE, UDI_PIO_R1},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	/*
	 * Subtract 1 from zero.   Should go negative. 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_4BYTE, (udi_ubit16_t)-0x1},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_4BYTE, (udi_ubit16_t)-0x1},
	{UDI_PIO_ADD + UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	/*
	 * Test non-immediate 'or' 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0xc0ff},
	{UDI_PIO_SHIFT_LEFT + UDI_PIO_R0, UDI_PIO_4BYTE, 8},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0xee},
	{UDI_PIO_OR + UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	{UDI_PIO_SYNC, 0, 0},
	{UDI_PIO_BARRIER, 0, 0},
	{UDI_PIO_SYNC_OUT, 0, 0},

	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, PIO_EXIT_SUCCESS}
};


void
test_math(void *buf)
{
	unsigned int i;
	udi_ubit32_t *b = (udi_ubit32_t *)buf;

	for (i = 0; i < 12; i++) {
		lprintf("[%08x]\n", b[i]);
	}
	assert(b[0] == 0x00012340);
	assert(b[1] == 0x00000123);
	assert(b[2] == 0x00000023);
	assert(b[3] == 0x00000024);
	assert(b[4] == 0x00000424);
	assert(b[5] == 0x00000024);
	assert(b[6] == 0x00001200);	/* subtract */
	assert(b[7] == 0x00001034);	/* xor */
	assert(b[8] == 0x00001134);	/* add */
	assert(b[9] == 0x00000000);	/* add */
	assert(b[10] == 0xffffffff);	/* add */
	assert(b[11] == 0xc0ffee);	/* non-immed or */
	udi_mem_free(buf);
}

udi_pio_trans_t test_bufinc_list[] = {
	/*
	 * R0 = offset into MEM 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0x0000},
	/*
	 * R1 = mem[0] 
	 */
	{UDI_PIO_LOAD + UDI_PIO_MEM + UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1},
	/*
	 * R1 ++ 
	 */
	{UDI_PIO_ADD_IMM + UDI_PIO_R1, UDI_PIO_4BYTE, 1},
	/*
	 * mem[0] = R1 
	 */
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, PIO_EXIT_SUCCESS}
};

void
test_bufinc(void *buf)
{
	udi_ubit32_t *b = (udi_ubit32_t *)buf;

	/*
	 * The buffer is preloaded with this pattern, so be sure we got
	 * * the incremented version back.
	 */
	assert(b[0] == 0xcacacaca + 1);
	udi_mem_free(buf);
}

udi_pio_trans_t test_shift_list[] = {
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, 0},

	/*
	 * R0  = 0x0004 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 4},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	/*
	 * R0 = 0x0002 
	 */
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R0, UDI_PIO_2BYTE, 1},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	/*
	 * R0 =  0x0001  
	 */
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R0, UDI_PIO_2BYTE, 1},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	/*
	 * R0 =  0x0001  
	 */
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R0, UDI_PIO_2BYTE, 1},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	/*
	 * R0 =  0x0001  
	 */
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R0, UDI_PIO_2BYTE, 1},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	/*
	 * R0 = 0x88887777666655554444333322221111 
	 */
#if 0
	{UDI_PIO_DEBUG, 0,
	 UDI_PIO_TRACE_OPS3 | UDI_PIO_TRACE_REGS3 | UDI_PIO_TRACE_DEV3},
#endif
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0x8888},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0x7777},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0x6666},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0x5555},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0x4444},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0x3333},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0x2222},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0x1111},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0xffff},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0xeeee},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0xdddd},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0xcccc},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0xbbbb},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0xaaaa},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0x9999},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0x0d0d},

	/*
	 * r1 = r2 = r3 = .... r6 = r0;  
	 */

	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R3, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R4, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R5, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_2BYTE, 0},

	{UDI_PIO_ADD + UDI_PIO_R1, UDI_PIO_16BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD + UDI_PIO_R2, UDI_PIO_16BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD + UDI_PIO_R3, UDI_PIO_16BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD + UDI_PIO_R5, UDI_PIO_16BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD + UDI_PIO_R6, UDI_PIO_16BYTE, UDI_PIO_R0},

	{UDI_PIO_SHIFT_LEFT + UDI_PIO_R1, UDI_PIO_32BYTE, 1},
	{UDI_PIO_SHIFT_LEFT + UDI_PIO_R2, UDI_PIO_32BYTE, 4},
	{UDI_PIO_SHIFT_LEFT + UDI_PIO_R3, UDI_PIO_32BYTE, 28},
	{UDI_PIO_SHIFT_LEFT + UDI_PIO_R4, UDI_PIO_32BYTE, 32},

	{UDI_PIO_SHIFT_LEFT + UDI_PIO_R5, UDI_PIO_16BYTE, 16},

	/*
	 * Move the absolute bottom 16 bits into the absolute top 16 
	 */
	{UDI_PIO_SHIFT_LEFT + UDI_PIO_R6, UDI_PIO_16BYTE, 32},
	{UDI_PIO_SHIFT_LEFT + UDI_PIO_R6, UDI_PIO_16BYTE, 32},
	{UDI_PIO_SHIFT_LEFT + UDI_PIO_R6, UDI_PIO_16BYTE, 32},
	{UDI_PIO_SHIFT_LEFT + UDI_PIO_R6, UDI_PIO_32BYTE, 32},
	{UDI_PIO_SHIFT_LEFT + UDI_PIO_R6, UDI_PIO_32BYTE, 32},
	{UDI_PIO_SHIFT_LEFT + UDI_PIO_R6, UDI_PIO_32BYTE, 32},
	{UDI_PIO_SHIFT_LEFT + UDI_PIO_R6, UDI_PIO_32BYTE, 32},

	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, 0x40},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R1},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R2},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R3},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R4},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R5},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R6},

	/*
	 * Now shift what we can (we destroyed some bits) back to original
	 * positions.
	 */
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R1, UDI_PIO_32BYTE, 1},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R2, UDI_PIO_32BYTE, 4},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R3, UDI_PIO_32BYTE, 28},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R4, UDI_PIO_32BYTE, 32},

	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R5, UDI_PIO_16BYTE, 16},

	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R6, UDI_PIO_32BYTE, 32},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R6, UDI_PIO_32BYTE, 32},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R6, UDI_PIO_32BYTE, 32},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R6, UDI_PIO_32BYTE, 32},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R6, UDI_PIO_32BYTE, 32},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R6, UDI_PIO_32BYTE, 32},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R6, UDI_PIO_16BYTE, 32},

	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R1},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R2},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R3},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R4},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R5},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R6},

	/*
	 * Load all 1 bits into all the registers by subtracting one from zero.
	 */ 
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0}, 
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0}, 
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, 0}, 
	{UDI_PIO_LOAD_IMM + UDI_PIO_R3, UDI_PIO_2BYTE, 0}, 
	{UDI_PIO_LOAD_IMM + UDI_PIO_R4, UDI_PIO_2BYTE, 0}, 
	{UDI_PIO_LOAD_IMM + UDI_PIO_R5, UDI_PIO_2BYTE, 0}, 
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0xffff}, 
	{UDI_PIO_ADD_IMM + UDI_PIO_R1, UDI_PIO_32BYTE, 0xffff}, 
	{UDI_PIO_ADD_IMM + UDI_PIO_R2, UDI_PIO_32BYTE, 0xffff}, 
	{UDI_PIO_ADD_IMM + UDI_PIO_R3, UDI_PIO_32BYTE, 0xffff}, 
	{UDI_PIO_ADD_IMM + UDI_PIO_R4, UDI_PIO_32BYTE, 0xffff}, 
	{UDI_PIO_ADD_IMM + UDI_PIO_R5, UDI_PIO_32BYTE, 0xffff}, 
	/*
	 * Now test that a shift right of a single bit results
	 * in a zero being brought into the top bit.    We then take
	 * advantage of the property that this is -1 and that -1 + -1 == -2,
	 * thus adding "2" allows us to use the PIO "test for zero" cskip
	 * modes.
	 */
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R0, UDI_PIO_1BYTE, 1},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R1, UDI_PIO_2BYTE, 1},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R2, UDI_PIO_4BYTE, 1},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R3, UDI_PIO_8BYTE, 1},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R4, UDI_PIO_16BYTE, 1},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R5, UDI_PIO_32BYTE, 1},

	{UDI_PIO_ADD + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD + UDI_PIO_R1, UDI_PIO_2BYTE, UDI_PIO_R1},
	{UDI_PIO_ADD + UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_R2},
	{UDI_PIO_ADD + UDI_PIO_R3, UDI_PIO_8BYTE, UDI_PIO_R3},
	{UDI_PIO_ADD + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_R4},
	{UDI_PIO_ADD + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R5},

	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_1BYTE, 2},
	{UDI_PIO_ADD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 2},
	{UDI_PIO_ADD_IMM + UDI_PIO_R2, UDI_PIO_4BYTE, 2},
	{UDI_PIO_ADD_IMM + UDI_PIO_R3, UDI_PIO_8BYTE, 2},
	{UDI_PIO_ADD_IMM + UDI_PIO_R4, UDI_PIO_16BYTE, 2},
	{UDI_PIO_ADD_IMM + UDI_PIO_R5, UDI_PIO_32BYTE, 2},

	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R1, UDI_PIO_2BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R3, UDI_PIO_8BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	/* 
	 * The registers are all zero now.  Reload and start over.
	 */
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_32BYTE, 0xffff}, 
	{UDI_PIO_ADD_IMM + UDI_PIO_R1, UDI_PIO_32BYTE, 0xffff}, 
	{UDI_PIO_ADD_IMM + UDI_PIO_R2, UDI_PIO_32BYTE, 0xffff}, 
	{UDI_PIO_ADD_IMM + UDI_PIO_R3, UDI_PIO_32BYTE, 0xffff}, 
	{UDI_PIO_ADD_IMM + UDI_PIO_R4, UDI_PIO_32BYTE, 0xffff}, 
	{UDI_PIO_ADD_IMM + UDI_PIO_R5, UDI_PIO_32BYTE, 0xffff}, 

	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R0, UDI_PIO_1BYTE, 1},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R1, UDI_PIO_2BYTE, 1},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R2, UDI_PIO_4BYTE, 1},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R3, UDI_PIO_8BYTE, 1},
	{UDI_PIO_SHIFT_RIGHT + UDI_PIO_R4, UDI_PIO_16BYTE, 1},

	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, PIO_EXIT_SUCCESS}

};

void
test_shift(void *buf)
{
	unsigned int i;
	udi_ubit32_t *b = (udi_ubit32_t *)buf;

	for (i = 0; i < 10; i++) {
		lprintf("[%08x]\n", b[i]);
	}
	assert(b[0] == 0x00000004);
	assert(b[1] == 0x00000002);
	assert(b[2] == 0x00000001);
	assert(b[3] == 0x00000000);
	assert(b[4] == 0x00000000);

	{
		udi_ubit32_t expect_r0[] = {
			0x0d0d9999,
			0xaaaabbbb,
			0xccccdddd,
			0xeeeeffff,
			0x11112222,
			0x33334444,
			0x55556666,
			0x77778888
		};
		udi_ubit32_t expect_r1[] = {
			0,
			0,
			0,
			0,
			0x22224444,
			0x66668888,
			0xaaaacccc,
			0xeeef1110
		};
		udi_ubit32_t expect_r2[] = {
			0,
			0,
			0,
			0x1,
			0x11122223,
			0x33344445,
			0x55566667,
			0x77788880
		};
		udi_ubit32_t expect_r3[] = {
			0,
			0,
			0,
			0x01111222,
			0x23333444,
			0x45555666,
			0x67777888,
			0x80000000
		};
		udi_ubit32_t expect_r4[] = {
			0,
			0,
			0,
			0x11112222,
			0x33334444,
			0x55556666,
			0x77778888,
			0
		};
		udi_ubit32_t expect_r5[] = {
			0,
			0,
			0,
			0,
			0x22223333,
			0x44445555,
			0x66667777,
			0x88880000
		};
		udi_ubit32_t expect_r6[] = {
			0x77778888,
			0,
			0,
			0,
		};
		udi_ubit32_t expect_rt_r0[] = {
			0x0,
			0x0,
			0x0,
			0x0,
			0x11112222,
			0x33334444,
			0x55556666,
			0x77778888
		};
		udi_ubit32_t expect_rt_r5[] = {
			0,
			0,
			0,
			0,
			0x00002222,
			0x33334444,
			0x55556666,
			0x77778888
		};
		udi_ubit32_t expect_rt_r6[] = {
			0,
			0,
			0,
			0,
			0x0,
			0x0,
			0x0,
			0x77778888
		};

		compare_very_long_word(32, expect_r0, &b[0x10]);
		compare_very_long_word(32, expect_r1, &b[0x18]);
		compare_very_long_word(32, expect_r2, &b[0x20]);
		compare_very_long_word(32, expect_r3, &b[0x28]);
		compare_very_long_word(32, expect_r4, &b[0x30]);
		compare_very_long_word(32, expect_r5, &b[0x38]);
		compare_very_long_word(32, expect_r6, &b[0x40]);

		compare_very_long_word(32, expect_rt_r0, &b[0x48]);
		compare_very_long_word(32, expect_rt_r0, &b[0x50]);
		compare_very_long_word(32, expect_rt_r0, &b[0x58]);
		compare_very_long_word(32, expect_rt_r0, &b[0x60]);
		compare_very_long_word(32, expect_rt_r5, &b[0x68]);
		compare_very_long_word(32, expect_rt_r6, &b[0x70]);
	}

	udi_mem_free(buf);
}

udi_pio_trans_t test_size_list[] = {
#if 0
	{UDI_PIO_DEBUG, 0,
	 UDI_PIO_TRACE_OPS3 | UDI_PIO_TRACE_REGS3 | UDI_PIO_TRACE_DEV3},
#endif
	/*
	 * R7 is our index into our destination buffer of test results 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, 0},
	/*
	 * R0 is our destination test register 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0},

	/*
	 * R6 is a Really Big Number for testing operations 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1e1f},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1c1d},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1a1b},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1819},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1617},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1415},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1213},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1011},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0e0f},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0c0d},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0a0b},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0809},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0607},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0405},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0203},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0001},

	/*
	 * First, verify we can store each of the tran sizes 
	 */

	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_1BYTE, UDI_PIO_R6},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},

	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_2BYTE, UDI_PIO_R6},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},

	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R6},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},

	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_8BYTE, UDI_PIO_R6},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},

	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_16BYTE, UDI_PIO_R6},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},

	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R6},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},

	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, PIO_EXIT_SUCCESS}
};

void
test_size(void *buf)
{
	unsigned int i, j;
	udi_ubit32_t *b = (udi_ubit32_t *)buf;
	udi_ubit16_t *s = (udi_ubit16_t *)buf;
	udi_ubit8_t *c = (udi_ubit8_t *)buf;

	for (i = 0; i < 6; i++) {
		lprintf("UDI_PIO_%dBYTE\n", 1 << i);
		for (j = 0; j < 8; j++) {
			lprintf("  [%08x]\n", b[i * 8 + j]);
		}
	}

	/*
	 * Verify each result was correctly computed and that nothing
	 * was stored into memory behind it.
	 */

	/*
	 * endian independent version of assert(b[0x00] == 0xcacaca1f); 
	 */
	assert(c[0x00] == 0x1f);
	assert(c[0x01] == 0xca);
	assert(c[0x02] == 0xca);
	assert(c[0x03] == 0xca);
	assert(b[0x01] == 0xcacacaca);

	/*
	 * endian independent version of assert(b[0x08] == 0xcaca1e1f); 
	 */
	assert(s[0x10] == 0x1e1f);
	assert(s[0x11] == 0xcaca);

	assert(b[0x09] == 0xcacacaca);

	assert(b[0x10] == 0x1c1d1e1f);
	assert(b[0x11] == 0xcacacaca);

	{
		udi_ubit32_t expect_8[] = {
			0x18191a1b,
			0x1c1d1e1f
		};
		udi_ubit32_t expect_16[] = {
			0x10111213,
			0x14151617,
			0x18191a1b,
			0x1c1d1e1f
		};
		udi_ubit32_t expect_32[] = {
			0x00010203,
			0x04050607,
			0x08090a0b,
			0x0c0d0e0f,
			0x10111213,
			0x14151617,
			0x18191a1b,
			0x1c1d1e1f
		};

		compare_very_long_word(8, expect_8, &b[0x18]);
		compare_very_long_word(16, expect_16, &b[0x20]);
		compare_very_long_word(32, expect_32, &b[0x28]);
	}

	udi_mem_free(buf);
}

udi_pio_trans_t test_mem_list[] = {
#if 0
	{UDI_PIO_DEBUG, 0,
	 UDI_PIO_TRACE_OPS3 | UDI_PIO_TRACE_REGS3 | UDI_PIO_TRACE_DEV3},
#endif
	/*
	 * R7 is our index into our destination buffer of test results 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, 0},

	/*
	 * Fill the PIO buffer with test pattern. 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_4BYTE, 0x1d1d},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_4BYTE, 0x1d1d},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0x0},
	/*
	 * R2 = Count 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, 0x100},

	{UDI_PIO_REP_OUT_IND, UDI_PIO_4BYTE,
	 UDI_PIO_REP_ARGS(UDI_PIO_DIRECT,
			  UDI_PIO_R0, 0,
			  UDI_PIO_R1, 1,
			  UDI_PIO_R2)},

	/*
	 * R1 is a fixed test pattern 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_4BYTE, 0x5678},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_4BYTE, 0x1234},


	{UDI_PIO_OUT + UDI_PIO_DIRECT + UDI_PIO_R1, UDI_PIO_1BYTE, 0},
	{UDI_PIO_OUT + UDI_PIO_DIRECT + UDI_PIO_R1, UDI_PIO_2BYTE, 4},
	{UDI_PIO_OUT + UDI_PIO_DIRECT + UDI_PIO_R1, UDI_PIO_4BYTE, 8},

	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R0, UDI_PIO_1BYTE, 0},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R0, UDI_PIO_2BYTE, 4},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R0, UDI_PIO_4BYTE, 8},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R0, UDI_PIO_4BYTE, 0},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 4},

	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, PIO_EXIT_SUCCESS}
};

void
test_mem(void *buf)
{
	unsigned int i;
	udi_ubit32_t *b = (udi_ubit32_t *)buf;

	for (i = 0; i < 16; i++) {
		lprintf("  [%08x]\n", b[i]);
	}

	assert(b[0] == 0x00000078);
	assert(b[1] == 0x00005678);
	assert(b[2] == 0x12345678);
	assert(b[3] == 0x1d1d1d78);

	udi_mem_free(buf);
}

udi_pio_trans_t mem_fill_list[] = {
	/*
	 * R7 is our index into our destination buffer of test results 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, 0},

	/*
	 * R6 is our constant version of the buffer we just wrote
	 * * into PIO space and is used for comparison.
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x2120},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x2322},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x2524},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x2726},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x2928},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x2b2a},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x2d2c},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x2f2e},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x3130},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x3332},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x3534},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x3736},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x3938},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x3b3a},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x3d3c},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x3f3e},

	/*
	 * First, we fill the first 256 bytes of "PIO" space with a 
	 * test pattern.  Byte 0 = 0, byte 128 = 128, etc.
	 */
	/*
	 * R0 = the byte we're writing 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0x0},
	{UDI_PIO_LABEL, 0, 1},
	{UDI_PIO_OUT_IND + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_1BYTE, 1},
	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_Z},
	{UDI_PIO_BRANCH, 0, 1},

#if 0
	{UDI_PIO_DEBUG, 0,
	 UDI_PIO_TRACE_OPS3 | UDI_PIO_TRACE_REGS3 | UDI_PIO_TRACE_DEV3},
#endif

	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R0, UDI_PIO_1BYTE, 0x20},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R1, UDI_PIO_2BYTE, 0x20},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R2, UDI_PIO_4BYTE, 0x20},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R3, UDI_PIO_8BYTE, 0x20},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R4, UDI_PIO_16BYTE, 0x20},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R5, UDI_PIO_32BYTE, 0x20},

	{UDI_PIO_XOR + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R1, UDI_PIO_2BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R3, UDI_PIO_8BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R6},

	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R1, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R2, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R3, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R4, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_LABEL, UDI_PIO_2BYTE, __LINE__},
	/*
	 * Now, cycle through the tran sizes with ops direct to memory 
	 */
	{UDI_PIO_IN + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_1BYTE, 32},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},

	{UDI_PIO_IN + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_2BYTE, 32},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},

	{UDI_PIO_IN + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},

	{UDI_PIO_IN + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_8BYTE, 32},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},

	{UDI_PIO_IN + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_16BYTE, 32},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},

	{UDI_PIO_IN + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE, 32},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32},

	/*
	 * Tests for  PIO_IN_IND.
	 * This verifies that only the bottom 32 bits of the indirect
	 * register are used for the address calculations.
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_16BYTE, 0x0020},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_16BYTE, 0x0000},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_16BYTE, 0x1234},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_16BYTE, 0x1234},

	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_16BYTE, 0x1234},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_16BYTE, 0x1234},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_16BYTE, 0x1234},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_16BYTE, 0x1234},

#define STORE_BIG_R7 \
  { UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R0 },\
  { UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_4BYTE, 32 },

	{UDI_PIO_LABEL, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_IN_IND + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R2},
	STORE_BIG_R7 {UDI_PIO_IN_IND + UDI_PIO_R0, UDI_PIO_2BYTE, UDI_PIO_R2},
	STORE_BIG_R7 {UDI_PIO_IN_IND + UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R2},
	STORE_BIG_R7 {UDI_PIO_IN_IND + UDI_PIO_R0, UDI_PIO_8BYTE, UDI_PIO_R2},
	STORE_BIG_R7 {UDI_PIO_IN_IND + UDI_PIO_R0, UDI_PIO_16BYTE, UDI_PIO_R2},
	STORE_BIG_R7 {UDI_PIO_IN_IND + UDI_PIO_R0, UDI_PIO_32BYTE, UDI_PIO_R2},
	STORE_BIG_R7
		/*
		 * Verify PIO_REP_IN_IND 
		 */
#if 0
	{UDI_PIO_DEBUG, 0,
	 UDI_PIO_TRACE_OPS3 | UDI_PIO_TRACE_REGS3 | UDI_PIO_TRACE_DEV3},
#endif
	/*
	 * mem register base 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0x200},
	/*
	 * pio register base 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0},
	/*
	 * R2 = Count 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, 32},

	{UDI_PIO_REP_IN_IND, UDI_PIO_1BYTE,
	 UDI_PIO_REP_ARGS(UDI_PIO_MEM,
			  UDI_PIO_R0, 1,
			  UDI_PIO_R1, 1,
			  UDI_PIO_R2)},

	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, PIO_EXIT_SUCCESS}

};

void
test_mem_fill(void *buf)
{
	udi_ubit32_t *w = (udi_ubit32_t *)buf;
	udi_ubit16_t *h = (udi_ubit16_t *)buf;
	udi_ubit8_t *c = (udi_ubit8_t *)buf;

	unsigned int i;

	for (i = 0; i < 0x90; i++) {
		unsigned int ii;

		lprintf("w %02x (h %02x, b %02x): [%08x] ", i, i * 2, i * 4,
			w[i]);
		for (ii = 0; ii < 16; ii++) {
			unsigned char *foo = (unsigned char *)&w[i];

			lprintf("%02x ", foo[ii]);
		}
		lprintf("\n");
	}
	assert(c[0x00] == 0x20);
	assert(c[0x01] == 0xca);
	assert(c[0x02] == 0xca);
	assert(c[0x03] == 0xca);
	assert(w[1] == 0xcacacaca);
	assert(w[2] == 0xcacacaca);
	assert(w[3] == 0xcacacaca);
	assert(w[4] == 0xcacacaca);
	assert(w[5] == 0xcacacaca);
	assert(w[6] == 0xcacacaca);
	assert(w[7] == 0xcacacaca);

	assert(h[0x10] == 0x2120);
	assert(h[0x11] == 0xcaca);
	assert(w[0x11] == 0xcacacaca);
	assert(w[0x12] == 0xcacacaca);
	assert(w[0x13] == 0xcacacaca);
	assert(w[0x14] == 0xcacacaca);
	assert(w[0x15] == 0xcacacaca);
	assert(w[0x16] == 0xcacacaca);
	assert(w[0x17] == 0xcacacaca);

	assert(w[0x10] == 0x23222120);
	assert(w[0x11] == 0xcacacaca);
	assert(w[0x12] == 0xcacacaca);
	assert(w[0x13] == 0xcacacaca);
	assert(w[0x14] == 0xcacacaca);
	assert(w[0x15] == 0xcacacaca);
	assert(w[0x16] == 0xcacacaca);
	assert(w[0x17] == 0xcacacaca);

	{
		udi_ubit32_t expect_n8[] = {
			0x27262524, 0x23222120
		};

		udi_ubit32_t expect_n16[] = {
			0x2f2e2d2c, 0x2b2a2928,
			0x27262524, 0x23222120
		};

		udi_ubit32_t expect_n32[] = {
			0x3f3e3d3c, 0x3b3a3938,
			0x37363534, 0x33323130,
			0x2f2e2d2c, 0x2b2a2928,
			0x27262524, 0x23222120
		};

		compare_very_long_word(8, expect_n8, &w[0x18]);
		compare_very_long_word(16, expect_n16, &w[0x20]);
		compare_very_long_word(32, expect_n32, &w[0x28]);
	}

	{
		udi_ubit32_t expect_8[] = {
			0,
			0,
			0,
			0,
			0,
			0,
			0x27262524,
			0x23222120
		};
		udi_ubit32_t expect_16[] = {
			0,
			0,
			0,
			0,
			0x2f2e2d2c,
			0x2b2a2928,
			0x27262524,
			0x23222120
		};
		udi_ubit32_t expect_32[] = {
			0x3f3e3d3c,
			0x3b3a3938,
			0x37363534,
			0x33323130,
			0x2f2e2d2c,
			0x2b2a2928,
			0x27262524,
			0x23222120
		};

		compare_very_long_word(32, expect_8, &w[0x48]);
		compare_very_long_word(32, expect_16, &w[0x50]);
		compare_very_long_word(32, expect_32, &w[0x58]);
	}

	for (i = 0; i < 32; i++) {
		assert(c[0x200 + i] == i);
	}
	assert(w[0x200 / 4 + 8] == 0xcacacaca);
	assert(w[0x200 / 4 + 9] == 0xcacacaca);

	udi_mem_free(buf);
}

/*
 * Verify that UDI_PIO_IN works for all data sizes.
 * Uses only internal comparisons.   We know the test pattern
 * we generate in PIO space and we use our internal cskip operators
 * to verify that UDI_PIO_IN works.
 */
udi_pio_trans_t pio_in_list[] = {
#if 0
	{UDI_PIO_DEBUG, 0,
	 UDI_PIO_TRACE_OPS3 | UDI_PIO_TRACE_REGS3 | UDI_PIO_TRACE_DEV3},
#endif
	/*
	 * First, we fill the first 256 bytes of "PIO" space with a 
	 * test pattern.  Byte 0 = 0, byte 16 = 16, etc.
	 */
	/*
	 * R0 = the byte we're writing 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0x0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 32},
	{UDI_PIO_LABEL, 0, 1},
	{UDI_PIO_OUT_IND + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_1BYTE, 1},
	{UDI_PIO_ADD_IMM + UDI_PIO_R1, UDI_PIO_1BYTE, (udi_ubit16_t)-1},
	{UDI_PIO_CSKIP + UDI_PIO_R1, UDI_PIO_1BYTE, UDI_PIO_Z},
	{UDI_PIO_BRANCH, 0, 1},
	{UDI_PIO_LABEL, 0, __LINE__},

	/*
	 * R6 is a Mondo Register that holds the constant 
	 * bit pattern we just wrote.   There is some trickery happening
	 * here becuase we need to resynthesize the endianness of our
	 * mapping.   As masochistic as this sounds we'll determine the
	 * endianness of our mapping AT RUNTIME IN THE TRANSLIST to build
	 * and use the correct constant.
	 */

	/*
	 * This is the pattern if we have a little-endian mapping. 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0100},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0302},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0504},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0706},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0908},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0b0a},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0d0c},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0f0e},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1110},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1312},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1514},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1716},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1918},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1b1a},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1d1c},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1f1e},

	/*
	 * The first 2 bytes should be "0x0100" on LE and "0x0001" on BE.
	 * So we read a half-word and then test if the LSB is zero. If so,
	 * we're BE.
	 */
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_2BYTE, 0},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_1BYTE, UDI_PIO_NZ},
	{UDI_PIO_BRANCH, 0, 0x100},

	/*
	 * This is the pattern if we have a big-endian mapping. 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1e1f},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1c1d},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1a1b},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1819},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1617},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1415},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1213},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1011},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0e0f},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0c0d},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0a0b},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0809},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0607},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0405},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0203},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0001},

	{UDI_PIO_LABEL, 0, 0x100},
	{UDI_PIO_LABEL, 0, __LINE__},


	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R0, UDI_PIO_1BYTE, 0},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R1, UDI_PIO_2BYTE, 0},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R2, UDI_PIO_4BYTE, 0},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R3, UDI_PIO_8BYTE, 0},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R4, UDI_PIO_16BYTE, 0},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R5, UDI_PIO_32BYTE, 0},

	{UDI_PIO_XOR + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R1, UDI_PIO_2BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R3, UDI_PIO_8BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R6},

	/*
	 * Intentionally test these with "nonobvious" word sizes to
	 * exercise the CSKIP widening.
	 */
	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R1, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R2, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R3, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R4, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	/*
	 * Now that we know LOAD_IMM works, we can test STORE_IMM
	 * and then read the results with LOAD_IMM to verify.
	 */
	{UDI_PIO_OUT + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_1BYTE, 32 * 1},
	{UDI_PIO_OUT + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_2BYTE, 32 * 2},
	{UDI_PIO_OUT + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_4BYTE, 32 * 3},
	{UDI_PIO_OUT + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_8BYTE, 32 * 4},
	{UDI_PIO_OUT + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_16BYTE, 32 * 5},
	{UDI_PIO_OUT + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_32BYTE, 32 * 6},

	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R0, UDI_PIO_32BYTE, 32 * 1},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R1, UDI_PIO_32BYTE, 32 * 2},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R2, UDI_PIO_32BYTE, 32 * 3},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R3, UDI_PIO_32BYTE, 32 * 4},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R4, UDI_PIO_32BYTE, 32 * 5},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R5, UDI_PIO_32BYTE, 32 * 6},

	/*
	 * Ideally, would subtract and then test for equality, but that
	 * doesn't work yet in the PIO engine.
	 */
	{UDI_PIO_XOR + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R1, UDI_PIO_2BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R3, UDI_PIO_8BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R6},


	{UDI_PIO_CSKIP + UDI_PIO_R1, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R2, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R3, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R4, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, PIO_EXIT_SUCCESS}
};

udi_pio_trans_t pio_in_list_BE[] = {
#if 0
	{UDI_PIO_DEBUG, 0,
	 UDI_PIO_TRACE_OPS3 | UDI_PIO_TRACE_REGS3 | UDI_PIO_TRACE_DEV3},
#endif
	/*
	 * First, we fill the first 256 bytes of "PIO" space with a 
	 * test pattern.  Byte 0 = 0, byte 16 = 16, etc.
	 */
	/*
	 * R0 = the byte we're writing 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0x0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 32},
	{UDI_PIO_LABEL, 0, 1},
	{UDI_PIO_OUT_IND + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_1BYTE, 1},
	{UDI_PIO_ADD_IMM + UDI_PIO_R1, UDI_PIO_1BYTE, (udi_ubit16_t)-1},
	{UDI_PIO_CSKIP + UDI_PIO_R1, UDI_PIO_1BYTE, UDI_PIO_Z},
	{UDI_PIO_BRANCH, 0, 1},
	{UDI_PIO_LABEL, 0, __LINE__},

	/*
	 * R6 is a Mondo Register that holds the constant 
	 * bit pattern we just wrote.   There is some trickery happening
	 * here becuase we need to resynthesize the endianness of our
	 * mapping.   As masochistic as this sounds we'll determine the
	 * endianness of our mapping AT RUNTIME IN THE TRANSLIST to build
	 * and use the correct constant.
	 */

	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R0, UDI_PIO_1BYTE, 0},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R1, UDI_PIO_2BYTE, 0},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R2, UDI_PIO_4BYTE, 0},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R3, UDI_PIO_8BYTE, 0},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R4, UDI_PIO_16BYTE, 0},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R5, UDI_PIO_32BYTE, 0},

	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_2BYTE, 0},
	{UDI_PIO_XOR + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R6},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_2BYTE, 0x0001},
	{UDI_PIO_XOR + UDI_PIO_R1, UDI_PIO_2BYTE, UDI_PIO_R6},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_4BYTE, 0x0203},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_4BYTE, 0x0001},
	{UDI_PIO_XOR + UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_R6},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_8BYTE, 0x0607},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_8BYTE, 0x0405},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_8BYTE, 0x0203},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_8BYTE, 0x0001},
	{UDI_PIO_XOR + UDI_PIO_R3, UDI_PIO_8BYTE, UDI_PIO_R6},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x0e0f},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x0c0d},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x0a0b},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x0809},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x0607},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x0405},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x0203},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x0001},
	{UDI_PIO_XOR + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_R6},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1e1f},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1c1d},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1a1b},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1819},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1617},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1415},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1213},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1011},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0e0f},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0c0d},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0a0b},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0809},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0607},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0405},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0203},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0001},
	{UDI_PIO_XOR + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R6},

	/*
	 * Intentionally test these with "nonobvious" word sizes to
	 * exercise the CSKIP widening.
	 */
	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R1, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R2, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
#if BROKEN
	{UDI_PIO_CSKIP + UDI_PIO_R3, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R4, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
#endif
	/*
	 * Now that we know LOAD_IMM works, we can test STORE_IMM
	 * and then read the results with LOAD_IN to verify.
	 * Note that since we're BE, we have to read and write 
	 * with the same sizes.
	 */
	{UDI_PIO_OUT + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_1BYTE, 32 * 1},
	{UDI_PIO_OUT + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_2BYTE, 32 * 2},
	{UDI_PIO_OUT + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_4BYTE, 32 * 3},
	{UDI_PIO_OUT + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_8BYTE, 32 * 4},
	{UDI_PIO_OUT + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_16BYTE, 32 * 5},
	{UDI_PIO_OUT + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_32BYTE, 32 * 6},

	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R0, UDI_PIO_1BYTE, 32 * 1},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R1, UDI_PIO_2BYTE, 32 * 2},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R2, UDI_PIO_4BYTE, 32 * 3},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R3, UDI_PIO_8BYTE, 32 * 4},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R4, UDI_PIO_16BYTE, 32 * 5},
	{UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R5, UDI_PIO_32BYTE, 32 * 6},

	/*
	 * Ideally, would subtract and then test for equality, but that
	 * doesn't work yet in the PIO engine.
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_2BYTE, 0x001f},
	{UDI_PIO_XOR + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R6},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_2BYTE, 0x1e1f},
	{UDI_PIO_XOR + UDI_PIO_R1, UDI_PIO_2BYTE, UDI_PIO_R6},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_4BYTE, 0x1e1f},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_4BYTE, 0x1c1d},
	{UDI_PIO_XOR + UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_R6},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_8BYTE, 0x1e1f},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_8BYTE, 0x1c1d},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_8BYTE, 0x1a1b},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_8BYTE, 0x1819},
	{UDI_PIO_XOR + UDI_PIO_R3, UDI_PIO_8BYTE, UDI_PIO_R6},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x1e1f},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x1c1d},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x1a1b},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x1819},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x1617},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x1415},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x1213},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0x1011},
	{UDI_PIO_XOR + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_R6},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1e1f},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1c1d},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1a1b},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1819},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1617},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1415},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1213},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1011},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0e0f},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0c0d},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0a0b},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0809},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0607},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0405},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0203},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0001},
	{UDI_PIO_XOR + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R6},


	{UDI_PIO_LABEL, 0, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R1, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R2, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
#if BROKEN
	{UDI_PIO_CSKIP + UDI_PIO_R3, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R4, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
#endif

	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, PIO_EXIT_SUCCESS}
};

udi_pio_trans_t long_math_list[] = {
	/*
	 * Craft an addition that we "know" generates interesting
	 * boundary cases inside the environment (carries on each word
	 * plus a final carry), do an add, compare it against the known
	 * correct value.
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0xf0f0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0xf0f0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0xf0f0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0xf0f0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0xf0f0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0xf0f0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0xf0f0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0xf0f0},

	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xe1e0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xe1e1},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xe1e1},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xe1e1},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xe1e1},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xe1e1},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xe1e1},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xe1e1},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x0001},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x0000},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x0000},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x0000},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x0000},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x0000},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x0000},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x0000},
#if 0
	{UDI_PIO_DEBUG, 0,
	 UDI_PIO_TRACE_OPS3 | UDI_PIO_TRACE_REGS3 | UDI_PIO_TRACE_DEV3},
#endif

	/*
	 * Make copies of a register by adding zero to it. 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R3, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R4, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R5, UDI_PIO_2BYTE, 0},
	{UDI_PIO_ADD + UDI_PIO_R0, UDI_PIO_32BYTE, UDI_PIO_R6},
	{UDI_PIO_ADD + UDI_PIO_R1, UDI_PIO_32BYTE, UDI_PIO_R6},
	{UDI_PIO_ADD + UDI_PIO_R2, UDI_PIO_32BYTE, UDI_PIO_R6},
	{UDI_PIO_ADD + UDI_PIO_R3, UDI_PIO_32BYTE, UDI_PIO_R6},
	{UDI_PIO_ADD + UDI_PIO_R4, UDI_PIO_32BYTE, UDI_PIO_R6},
	{UDI_PIO_ADD + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R6},

	/*
	 * Add the register to itself. 
	 */
	{UDI_PIO_ADD + UDI_PIO_R6, UDI_PIO_32BYTE, UDI_PIO_R6},
	/*
	 * Flip all the bits against the known correct value 
	 */
	{UDI_PIO_XOR + UDI_PIO_R6, UDI_PIO_32BYTE, UDI_PIO_R7},
	/*
	 * Verify that no bits are left. 
	 */
	{UDI_PIO_CSKIP + UDI_PIO_R6, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_ADD + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R0},
	{UDI_PIO_ADD + UDI_PIO_R1, UDI_PIO_2BYTE, UDI_PIO_R1},
	{UDI_PIO_ADD + UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_R2},
	{UDI_PIO_ADD + UDI_PIO_R3, UDI_PIO_8BYTE, UDI_PIO_R3},
	{UDI_PIO_ADD + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_R4},
	{UDI_PIO_ADD + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R5},

	{UDI_PIO_XOR + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R1, UDI_PIO_2BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R3, UDI_PIO_8BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R7},

	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R1, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R2, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R3, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R4, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	/*
	 * Now we start the subtraction tests. 
	 */

	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0xf0f0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0xf0f0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0xf0f0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0xf0f0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0xf0f0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0xf0f0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0xf0f0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_16BYTE, 0xf0f0},
	{UDI_PIO_SUB + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R6},
	/*
	 * Flip all the bits against the known correct value 
	 */
	{UDI_PIO_XOR + UDI_PIO_R6, UDI_PIO_32BYTE, UDI_PIO_R7},
	/*
	 * Verify that no bits are left. 
	 */
	{UDI_PIO_CSKIP + UDI_PIO_R6, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, 1},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R3, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R4, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R5, UDI_PIO_2BYTE, 0},

	{UDI_PIO_SUB + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R7},
	{UDI_PIO_SUB + UDI_PIO_R1, UDI_PIO_2BYTE, UDI_PIO_R7},
	{UDI_PIO_SUB + UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_R7},
	{UDI_PIO_SUB + UDI_PIO_R3, UDI_PIO_8BYTE, UDI_PIO_R7},
	{UDI_PIO_SUB + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_R7},
	{UDI_PIO_SUB + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R7},

	/*
	 * FIXME: should test the value of the subtraction 
	 */

#if 0
	{UDI_PIO_DEBUG, 0,
	 UDI_PIO_TRACE_OPS3 | UDI_PIO_TRACE_REGS3 | UDI_PIO_TRACE_DEV3},
#endif

	/*
	 * Start the 'AND' tests 
	 */

	{UDI_PIO_LABEL, 0, __LINE__},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xffff},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xffff},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xffff},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xffff},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xffff},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xffff},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xffff},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xffff},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xffff},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xffff},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xffff},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xffff},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xffff},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xffff},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xffff},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0xffff},


	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0100},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0302},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0504},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0706},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0908},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0b0a},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0d0c},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0f0e},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1110},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1312},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1514},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1716},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1918},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1b1a},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1d1c},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1f1e},

	/*
	 * Make copies of a register by adding zero to it. 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R3, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R4, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R5, UDI_PIO_2BYTE, 0},

	{UDI_PIO_ADD + UDI_PIO_R0, UDI_PIO_32BYTE, UDI_PIO_R6},
	{UDI_PIO_ADD + UDI_PIO_R1, UDI_PIO_32BYTE, UDI_PIO_R6},
	{UDI_PIO_ADD + UDI_PIO_R2, UDI_PIO_32BYTE, UDI_PIO_R6},
	{UDI_PIO_ADD + UDI_PIO_R3, UDI_PIO_32BYTE, UDI_PIO_R6},
	{UDI_PIO_ADD + UDI_PIO_R4, UDI_PIO_32BYTE, UDI_PIO_R6},
	{UDI_PIO_ADD + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R6},

	{UDI_PIO_AND + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R7},
	{UDI_PIO_AND + UDI_PIO_R1, UDI_PIO_2BYTE, UDI_PIO_R7},
	{UDI_PIO_AND + UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_R7},
	{UDI_PIO_AND + UDI_PIO_R3, UDI_PIO_8BYTE, UDI_PIO_R7},
	{UDI_PIO_AND + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_R7},
	{UDI_PIO_AND + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R7},

	{UDI_PIO_XOR + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R1, UDI_PIO_2BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R3, UDI_PIO_8BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R6},

	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R1, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R2, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R3, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R4, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	/*
	 * And now, the 'OR' tests. 
	 */
	{UDI_PIO_LABEL, 0, __LINE__},
	{UDI_PIO_OR + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R6},
	{UDI_PIO_OR + UDI_PIO_R1, UDI_PIO_2BYTE, UDI_PIO_R6},
	{UDI_PIO_OR + UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_R6},
	{UDI_PIO_OR + UDI_PIO_R3, UDI_PIO_8BYTE, UDI_PIO_R6},
	{UDI_PIO_OR + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_R6},
	{UDI_PIO_OR + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R6},

	{UDI_PIO_XOR + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R1, UDI_PIO_2BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R3, UDI_PIO_8BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R6},

	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R1, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R2, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R3, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R4, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	/*
	 * Now some simple tests of PIO_NZ. Remember bottome byte really IS zero.
	 */
	{UDI_PIO_LABEL, 0, __LINE__},
	{UDI_PIO_CSKIP + UDI_PIO_R6, UDI_PIO_1BYTE, UDI_PIO_Z},
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 1},
	{UDI_PIO_CSKIP + UDI_PIO_R6, UDI_PIO_2BYTE, UDI_PIO_NZ},
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 1},
	{UDI_PIO_CSKIP + UDI_PIO_R6, UDI_PIO_4BYTE, UDI_PIO_NZ},
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 1},
	{UDI_PIO_CSKIP + UDI_PIO_R6, UDI_PIO_8BYTE, UDI_PIO_NZ},
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 1},
	{UDI_PIO_CSKIP + UDI_PIO_R6, UDI_PIO_16BYTE, UDI_PIO_NZ},
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 1},
	{UDI_PIO_CSKIP + UDI_PIO_R6, UDI_PIO_32BYTE, UDI_PIO_NZ},
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 1},
#if 0
	{UDI_PIO_DEBUG, 0,
	 UDI_PIO_TRACE_OPS3 | UDI_PIO_TRACE_REGS3 | UDI_PIO_TRACE_DEV3},
#endif

	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_2BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	/*
	 * Now all the registers are zero.  Let's test the sign-extension
	 * * facilities of PIO_ADD_IMM
	 */
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_1BYTE, (udi_ubit16_t)-1},
	{UDI_PIO_ADD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, (udi_ubit16_t)-1},
	{UDI_PIO_ADD_IMM + UDI_PIO_R2, UDI_PIO_4BYTE, (udi_ubit16_t)-1},
	{UDI_PIO_ADD_IMM + UDI_PIO_R3, UDI_PIO_8BYTE, (udi_ubit16_t)-1},
	{UDI_PIO_ADD_IMM + UDI_PIO_R4, UDI_PIO_16BYTE, (udi_ubit16_t)-1},
	{UDI_PIO_ADD_IMM + UDI_PIO_R5, UDI_PIO_32BYTE, (udi_ubit16_t)-1},

	{UDI_PIO_XOR + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R1, UDI_PIO_2BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R3, UDI_PIO_8BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R7},

	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R1, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R2, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R3, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R4, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},


	/*
	 * Poke all the resigters back to -1 
	 */
	{UDI_PIO_XOR + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R1, UDI_PIO_2BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R3, UDI_PIO_8BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R7},

	/*
	 * Prove that when we add zero to it, they all go zero. 
	 */
	{UDI_PIO_LABEL, 0, __LINE__},

	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_1BYTE, 1},
	{UDI_PIO_ADD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 1},
	{UDI_PIO_ADD_IMM + UDI_PIO_R2, UDI_PIO_4BYTE, 1},
	{UDI_PIO_ADD_IMM + UDI_PIO_R3, UDI_PIO_8BYTE, 1},
	{UDI_PIO_ADD_IMM + UDI_PIO_R4, UDI_PIO_16BYTE, 1},
	{UDI_PIO_ADD_IMM + UDI_PIO_R5, UDI_PIO_32BYTE, 1},

	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R1, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R2, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R3, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R4, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	/*
	 * Now clobber the top bits of R7 with AND_IMM  of zero 
	 */
	{UDI_PIO_LABEL, 0, __LINE__},
	{UDI_PIO_AND_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_OR_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, (udi_ubit16_t)-1},
	{UDI_PIO_ADD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 1},
	{UDI_PIO_CSKIP + UDI_PIO_R6, UDI_PIO_2BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, PIO_EXIT_SUCCESS}
};

void
test_long_math(void *buf)
{
	udi_mem_free(buf);
}

void
test_pio_in(void *buf)
{
	/*
	 * All testing is done in the list itself. 
	 */
	udi_mem_free(buf);
}

/*
 * Verify that PIO_LOAD works for all sizes.
 */

udi_pio_trans_t load_list[] = {
	/*
	 * R6 is a Really Big Number for testing operations 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1e1f},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1c1d},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1a1b},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1819},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1617},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1415},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1213},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x1011},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0e0f},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0c0d},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0a0b},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0809},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0607},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0405},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0203},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_32BYTE, 0x0001},

	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, 0x0},
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE,
	 UDI_PIO_R6},
#if 0
	{UDI_PIO_LABEL, 0, __LINE__},
	{UDI_PIO_DEBUG, 0,
	 UDI_PIO_TRACE_OPS3 | UDI_PIO_TRACE_REGS3 | UDI_PIO_TRACE_DEV3},
#endif

	{UDI_PIO_LOAD + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_1BYTE, UDI_PIO_R0},
	{UDI_PIO_LOAD + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_2BYTE, UDI_PIO_R1},
	{UDI_PIO_LOAD + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R2},
	{UDI_PIO_LOAD + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_8BYTE, UDI_PIO_R3},
	{UDI_PIO_LOAD + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_16BYTE, UDI_PIO_R4},
	{UDI_PIO_LOAD + UDI_PIO_MEM + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R5},

	{UDI_PIO_XOR + UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R1, UDI_PIO_2BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R2, UDI_PIO_4BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R3, UDI_PIO_8BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R4, UDI_PIO_16BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_R6},

#if LATER
	{UDI_PIO_CSKIP + UDI_PIO_R0, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R1, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R2, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R3, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R4, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_CSKIP + UDI_PIO_R5, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},
#endif

	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0x1100},
	{UDI_PIO_CSKIP + UDI_PIO_Z, UDI_PIO_1BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	/* 
	 * LOAD+DIRECT is essentially a reg->reg move.   Verify that for 
	 * all widths.
	 */
	{UDI_PIO_LOAD + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_1BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R7, UDI_PIO_1BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_1BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_LOAD + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_2BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R7, UDI_PIO_2BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_2BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_LOAD + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_4BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_LOAD + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_8BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R7, UDI_PIO_8BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_8BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_LOAD + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_16BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R7, UDI_PIO_16BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_16BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_LOAD + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_32BYTE, UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	/* 
	 * STORE+DIRECT is a reg->reg move much like LOAD+DIRECT, but
	 * the src and dest are swapped.
	 * all widths.
	 */
	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_1BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R7, UDI_PIO_1BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_1BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_2BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R7, UDI_PIO_2BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_2BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_8BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R7, UDI_PIO_8BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_8BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_16BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R7, UDI_PIO_16BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_16BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	/*
	 * Now prove that stores into narrowed registers get the 
	 * target width narrowed/extended right.
	 */
	/* R7.32 = R6 */
	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_32BYTE, 
		UDI_PIO_R6},
	/* R7.1 = R6 */
	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_1BYTE, 
		UDI_PIO_R6},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, (udi_ubit16_t) -0x1f},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	/* R7.32 = R6 */
	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_32BYTE, 
		UDI_PIO_R6},
	/* R7.2 = R6 */
	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_2BYTE, 
		UDI_PIO_R6},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, (udi_ubit16_t) -0x1e1f},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_32BYTE, 
		UDI_PIO_R6},
	/* R7.4 = R6 */
	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_4BYTE, 
		UDI_PIO_R6},
	{UDI_PIO_SUB + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_32BYTE, 
		UDI_PIO_R6},
	/* R7.8 = R6 */
	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_8BYTE, 
		UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R7, UDI_PIO_8BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_32BYTE, 
		UDI_PIO_R6},
	/* R7.16 = R6 */
	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_16BYTE, 
		UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R7, UDI_PIO_16BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_32BYTE, 
		UDI_PIO_R6},
	/* R7.32 = R6 */
	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_32BYTE, 
		UDI_PIO_R6},
	{UDI_PIO_XOR + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},


	/*
	 * Now do the same tests for PIO_LOAD + UDI_PIO_DIRECT.
 	 */
	/* R7.32 = R6 */
	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_32BYTE, 
		UDI_PIO_R6},
	/* R7.1 = R6 */
	{UDI_PIO_LOAD + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_1BYTE, 
		UDI_PIO_R7},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, (udi_ubit16_t) -0x1f},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	/* R7.32 = R6 */
	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_32BYTE, 
		UDI_PIO_R6},
	/* R7.2 = R6 */
	{UDI_PIO_LOAD + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_2BYTE, 
		UDI_PIO_R7},
	{UDI_PIO_ADD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, (udi_ubit16_t) -0x1e1f},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_32BYTE, 
		UDI_PIO_R6},
	/* R7.4 = R6 */
	{UDI_PIO_LOAD + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_4BYTE, 
		UDI_PIO_R7},
	{UDI_PIO_SUB + UDI_PIO_R7, UDI_PIO_4BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},

	{UDI_PIO_STORE + UDI_PIO_DIRECT + UDI_PIO_R7, UDI_PIO_32BYTE, 
		UDI_PIO_R6},
	/* R7.8 = R6 */
	{UDI_PIO_LOAD + UDI_PIO_DIRECT + UDI_PIO_R6, UDI_PIO_8BYTE, 
		UDI_PIO_R7},
	{UDI_PIO_XOR + UDI_PIO_R7, UDI_PIO_8BYTE, UDI_PIO_R6},
	{UDI_PIO_CSKIP + UDI_PIO_R7, UDI_PIO_32BYTE, UDI_PIO_Z},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, __LINE__},



	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, PIO_EXIT_SUCCESS}
};

void
test_load(void *buf)
{
	/*
	 * All testing is done in the list itself. 
	 */
	udi_mem_free(buf);
}

udi_pio_trans_t test_jump_list[] = {
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LABEL, 0, 0x7},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 7},
	{UDI_PIO_LABEL, 0, 0x6},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 6},
	{UDI_PIO_LABEL, 0, 0x5},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 5},
	{UDI_PIO_LABEL, 0, 0x4},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 4},
	{UDI_PIO_LABEL, 0, 0x3},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 3},
	{UDI_PIO_LABEL, 0, 0x2},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 2},
	{UDI_PIO_LABEL, 0, 0x1},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 1}
};

udi_pio_trans_t test_end_list[] = {
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_4BYTE, (udi_ubit16_t)-1},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_4BYTE, (udi_ubit16_t)-1},
	{UDI_PIO_END, UDI_PIO_2BYTE, UDI_PIO_R0},

	{UDI_PIO_LABEL, 0, 0x1},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0x1111},
	{UDI_PIO_END, UDI_PIO_2BYTE, UDI_PIO_R1},

	{UDI_PIO_LABEL, 0, 0x2},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, 0x2222},
	{UDI_PIO_END, UDI_PIO_2BYTE, UDI_PIO_R2},

	{UDI_PIO_LABEL, 0, 0x3},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R3, UDI_PIO_2BYTE, 0x3333},
	{UDI_PIO_END, UDI_PIO_2BYTE, UDI_PIO_R3},

	{UDI_PIO_LABEL, 0, 0x4},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R4, UDI_PIO_2BYTE, 0x4444},
	{UDI_PIO_END, UDI_PIO_2BYTE, UDI_PIO_R4},

	{UDI_PIO_LABEL, 0, 0x5},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R5, UDI_PIO_2BYTE, 0x5555},
	{UDI_PIO_END, UDI_PIO_2BYTE, UDI_PIO_R5},

	{UDI_PIO_LABEL, 0, 0x6},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_2BYTE, 0x6666},
	{UDI_PIO_END, UDI_PIO_2BYTE, UDI_PIO_R6},

	{UDI_PIO_LABEL, 0, 0x7},
	{UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_2BYTE, 0x7777},
	{UDI_PIO_END, UDI_PIO_2BYTE, UDI_PIO_R7},
};

udi_pio_trans_t double_skip_list[] = {
#if 0
	{UDI_PIO_DEBUG, 0,
	 UDI_PIO_TRACE_OPS3 | UDI_PIO_TRACE_REGS3 | UDI_PIO_TRACE_DEV3},
#endif
	/*
	 * Verify that code does a sensible thing when PIO_BRANCH is
	 * the last opcode.
	 */
	{UDI_PIO_BRANCH, 0, 99},
	{UDI_PIO_LABEL, 0, 3},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, PIO_EXIT_SUCCESS},
	{UDI_PIO_LABEL, 0, 99},
	/*
	 * Goto LABEL 3: next iteration 
	 */
	{UDI_PIO_BRANCH, 0, 3}
};


test_translist_t test_translist[] = {
	{test_load_imm, 0,
	 sizeof (test_load_list) / sizeof (udi_pio_trans_t),
	 test_load_list, "test_load_imm",
	 UDI_PIO_LITTLE_ENDIAN,
	 0},
	{test_math, 0,
	 sizeof (test_math_list) / sizeof (udi_pio_trans_t),
	 test_math_list, "test_math_list",
	 UDI_PIO_LITTLE_ENDIAN,
	 0},
	{test_bufinc, 0,
	 sizeof (test_bufinc_list) / sizeof (udi_pio_trans_t),
	 test_bufinc_list, "test_bufinc_list",
	 UDI_PIO_LITTLE_ENDIAN,
	 0},
	{test_shift, 0,
	 sizeof (test_shift_list) / sizeof (udi_pio_trans_t),
	 test_shift_list, "test_shift_list",
	 UDI_PIO_LITTLE_ENDIAN,
	 0},
	{test_size, 0,
	 sizeof (test_size_list) / sizeof (udi_pio_trans_t),
	 test_size_list, "test_size_list",
	 UDI_PIO_LITTLE_ENDIAN,
	 0},
	{test_mem, -UDI_SYSBUS_MEM_BUS,
	 sizeof (test_mem_list) / sizeof (udi_pio_trans_t),
	 test_mem_list, "test_mem_list",
	 UDI_PIO_LITTLE_ENDIAN,
	 0x400},
	{test_mem_fill, -UDI_SYSBUS_MEM_BUS,
	 sizeof (mem_fill_list) / sizeof (udi_pio_trans_t),
	 mem_fill_list, "mem_fill_list",
	 UDI_PIO_LITTLE_ENDIAN,
	 0x100},
	{test_pio_in, -UDI_SYSBUS_MEM_BUS,
	 sizeof (pio_in_list) / sizeof (udi_pio_trans_t),
	 pio_in_list, "pio_in_list, little endian",
	 UDI_PIO_LITTLE_ENDIAN,
	 0x100},
	{test_pio_in, -UDI_SYSBUS_MEM_BUS,
	 sizeof (pio_in_list_BE) / sizeof (udi_pio_trans_t),
	 pio_in_list_BE, "pio_in_list, big endian",
	 UDI_PIO_BIG_ENDIAN,
	 0x100},
	{test_long_math, 0,
	 sizeof (long_math_list) / sizeof (udi_pio_trans_t),
	 long_math_list, "long_math_list",
	 UDI_PIO_NEVERSWAP},
	{test_load, 0,
	 sizeof (double_skip_list) / sizeof (udi_pio_trans_t),
	 double_skip_list, "double_skip_list",
	 UDI_PIO_LITTLE_ENDIAN},
	{test_load, 0,
	 sizeof (load_list) / sizeof (udi_pio_trans_t),
	 load_list, "load_list",
	 UDI_PIO_NEVERSWAP},
};



STATIC void
trans_cb(udi_cb_t *gcb,
	 udi_buf_t *buf,
	 udi_status_t status,
	 udi_ubit16_t result)
{
	int *scratch = gcb->scratch;
	int test_num = scratch[0];

	lprintf("trans cb 0x%p.  Status 0x%x. Result %d(0x%x)\n",
		gcb, status, result, result);
	assert(result == PIO_EXIT_SUCCESS);

	/*
	 * Run the C function to verify the actions of the PIO list.
	 */
	(test_translist[test_num].test_fn) (gcb->context);

	/*
	 * Release the pio handle & CB 
	 */
	lprintf("Unmapping handle <%p>\n", gcb->initiator_context);
	udi_pio_unmap(gcb->initiator_context);
	udi_cb_free(gcb);

	/*
	 * Free a null CB just becuase we can. 
	 */
	udi_cb_free(NULL);

	_OSDEP_EVENT_SIGNAL(&test_done);
}

STATIC void
mem_alloc_cb(udi_cb_t *gcb,
	     void *new_mem)
{
	/*
	 * gcb->initiator_context is the PIO handle.   
	 * gcb->context is our 'new_mem' so the callback can free it.
	 */
	gcb->context = new_mem;

	udi_memset(new_mem, 0xca, 1000);
	udi_pio_trans(trans_cb, gcb, gcb->initiator_context, 0, NULL, new_mem);

}

STATIC void
piomap_cb(udi_cb_t *gcb,
	  udi_pio_handle_t new_handle)
{
	gcb->initiator_context = new_handle;
	udi_mem_alloc(mem_alloc_cb, gcb, 1000, UDI_MEM_MOVABLE);
}

STATIC void
cb_alloc_callback(udi_cb_t *gcb,
		  udi_cb_t *new_cb)
{
	int test_num;
	int *scratch;

	scratch = new_cb->scratch;

	_OSDEP_SIMPLE_MUTEX_LOCK(&test_cnt_mutex);
	test_num = test_cnt++;
	_OSDEP_SIMPLE_MUTEX_UNLOCK(&test_cnt_mutex);

	test_translist[test_num].gcb = new_cb;
	scratch[0] = test_num;
	udi_pio_map(piomap_cb, new_cb, test_translist[test_num].regset_idx,	/* regset_idx */
		    0,			/* pio_offset */
		    test_translist[test_num].map_length,	/* Length */
		    test_translist[test_num].trans_list,	/* trans_list */
		    test_translist[test_num].list_size,	/* list length */
		    test_translist[test_num].attributes,	/* pio_attributes */
		    0,			/* pace */
		    0			/* serialization domain */
		);
}

STATIC int entrypt_ct;
STATIC udi_pio_handle_t entrypt_handle;
STATIC udi_pio_handle_t end_handle;

STATIC void
entrypt_trans_done(udi_cb_t *gcb,
		   udi_buf_t *buf,
		   udi_status_t status,
		   udi_ubit16_t result)
{
	assert(result == entrypt_ct);
	entrypt_ct++;
	if (entrypt_ct < 8) {
		udi_pio_trans(entrypt_trans_done, gcb, entrypt_handle,
			      entrypt_ct, NULL, NULL);
	} else {
		udi_pio_unmap(entrypt_handle);
		udi_cb_free(gcb);
		udi_mem_free(buf);
		_OSDEP_EVENT_SIGNAL(&test_done);
	}
}

STATIC void
got_entrypt_map(udi_cb_t *gcb,
		udi_pio_handle_t new_handle)
{
	entrypt_handle = new_handle;

	udi_pio_trans(entrypt_trans_done, gcb, new_handle,
		      entrypt_ct, NULL, NULL);
}

STATIC void
got_entrypt_cb(udi_cb_t *gcb,
	       udi_cb_t *new_cb)
{
	udi_pio_map(got_entrypt_map, new_cb, 0, 0, 0, test_jump_list, sizeof (test_jump_list) / sizeof (udi_pio_trans_t), UDI_PIO_NEVERSWAP, 0,	/* pace */
		    0 /* serialization domain */ );

}

STATIC void
end_trans_done(udi_cb_t *gcb,
	       udi_buf_t *buf,
	       udi_status_t status,
	       udi_ubit16_t result)
{
	lprintf("END: %d Result %x\n", entrypt_ct, result);
	entrypt_ct++;
	if (entrypt_ct < 8) {
		if (entrypt_ct == 1)
			assert(result == (udi_ubit16_t)-1);
		else
			assert(result == 0x1111 * (entrypt_ct - 1));
		udi_pio_trans(end_trans_done, gcb, end_handle,
			      entrypt_ct, NULL, NULL);
	} else {
		udi_pio_unmap(end_handle);
		udi_cb_free(gcb);
		udi_mem_free(buf);
		_OSDEP_EVENT_SIGNAL(&test_done);
	}

}

STATIC void
got_end_map(udi_cb_t *gcb,
	    udi_pio_handle_t new_handle)
{
	end_handle = new_handle;
	entrypt_ct = 0;
	udi_pio_trans(end_trans_done, gcb, new_handle, 0, NULL, NULL);
}

STATIC void
got_end_cb(udi_cb_t *gcb,
	   udi_cb_t *new_cb)
{
	udi_pio_map(got_end_map, new_cb, 0, 0, 0, test_end_list, sizeof (test_end_list) / sizeof (udi_pio_trans_t), UDI_PIO_NEVERSWAP, 0,	/* pace */
		    0 /* serialization domain */ );

}

#define ABUSE(SZ) \
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x1e1f }, \
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x1c1d }, \
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x1a1b }, \
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x1819 }, \
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x1617 }, \
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x1415 }, \
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x1213 }, \
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x1011 }, \
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x0e0f }, \
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x0c0d }, \
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x0a0b }, \
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x0809 }, \
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x0607 }, \
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x0405 }, \
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x0203 }, \
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R7, UDI_PIO_32BYTE, 0x0001 }, \
	{ UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0x0 }, \
	{ UDI_PIO_OUT + UDI_PIO_DIRECT + UDI_PIO_R7, (SZ), 0 }, \
	{ UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R7,  (SZ), 0 }, \
	{ UDI_PIO_OUT_IND + UDI_PIO_R7, (SZ), UDI_PIO_R0 }, \
	{ UDI_PIO_IN_IND + UDI_PIO_R7, (SZ), UDI_PIO_R0 }, \
	{ UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R0,  (SZ), UDI_PIO_R7 }, \
	{ UDI_PIO_LOAD + UDI_PIO_MEM + UDI_PIO_R0,   (SZ), UDI_PIO_R7 }, \
	{ UDI_PIO_END_IMM, UDI_PIO_2BYTE, PIO_EXIT_SUCCESS },

udi_pio_trans_t map_sz_list[] = {
	{UDI_PIO_LABEL, 0, 1},
	ABUSE(UDI_PIO_1BYTE) {UDI_PIO_LABEL, 0, 2},
	ABUSE(UDI_PIO_2BYTE) {UDI_PIO_LABEL, 0, 3},
	ABUSE(UDI_PIO_4BYTE) {UDI_PIO_LABEL, 0, 4},
	ABUSE(UDI_PIO_8BYTE) {UDI_PIO_LABEL, 0, 5},
	ABUSE(UDI_PIO_16BYTE) {UDI_PIO_LABEL, 0, 6},
	ABUSE(UDI_PIO_32BYTE)
};

STATIC void
map_sz_trans_done(udi_cb_t *gcb,
		  udi_buf_t *buf,
		  udi_status_t status,
		  udi_ubit16_t result)
{
	static int map_sz_cnt;

	assert(result == PIO_EXIT_SUCCESS);
	udi_mem_free(gcb->context);
	udi_pio_unmap(gcb->initiator_context);
	udi_cb_free(gcb);

	map_sz_cnt++;
	if (map_sz_cnt == 6) {
		_OSDEP_EVENT_SIGNAL(&test_done);
	}
}

STATIC void
got_map_sz_pio_mapping(udi_cb_t *gcb,
		       udi_pio_handle_t pio_handle)
{
	int *scratch = gcb->scratch;

	gcb->initiator_context = pio_handle;

	udi_pio_trans(map_sz_trans_done, gcb, pio_handle,
		      scratch[0], NULL, gcb->context);
}


STATIC void
got_map_sz_mem(udi_cb_t *gcb,
	       void *new_mem)
{
	int *scratch = gcb->scratch;

	gcb->context = new_mem;

	udi_pio_map(got_map_sz_pio_mapping, gcb, -UDI_SYSBUS_MEM_BUS,	/* regset_idx */
		    0,			/* pio_offset */
		    scratch[1],		/* Length */
		    map_sz_list,	/* trans_list */
		    sizeof (map_sz_list) / sizeof (map_sz_list[0]),	/* list length */
		    UDI_PIO_BIG_ENDIAN,	/* pio_attributes */
		    0,			/* pace */
		    0			/* serialization domain */
		);
}

STATIC void
got_map_sz_cbs(udi_cb_t *gcb,
	       udi_cb_t *new_cb)
{
	int n = 1;

	for (; new_cb; new_cb = new_cb->initiator_context, n++) {
		int *scratch = new_cb->scratch;
		int sz = 1 << (n - 1);

		scratch[0] = n;
		scratch[1] = sz;
		udi_mem_alloc(got_map_sz_mem, new_cb, sz, UDI_MEM_MOVABLE);
	}
}

/*
 * The 'expected' argument is expressed in big endian.   Yes, this is 
 * somewhat counterintuitive.
 */
STATIC void
compare_very_long_word(int size_in_bytes,
		       udi_ubit32_t *expected,
		       udi_ubit32_t *got)
{
	/*
	 * test to allow 'i_am_little_endian' this to be folded to a 
	 * constant by the optimizers 
	 */
	udi_boolean_t i_am_little_endian;
	union {
		long l;
		unsigned char uc[sizeof (long)];
	} u;
	unsigned int i;

	u.l = 1;
	i_am_little_endian = u.uc[0];

	if (i_am_little_endian) {
		for (i = size_in_bytes / sizeof (int); i--;) {
			assert(expected[i] == *got);
			got++;
		}
	} else {
		for (i = 0; i < size_in_bytes / sizeof (int); i++) {
			assert(expected[i] == *got);
			got++;
		}
	}
}

void
transtest_parseargs(int c,
		    void *optarg)
{
	switch (c) {
	case 'v':
		verbose = 1;
	}
}


int
main(int argc,
     char *argv[])
{
	_udi_pio_mutex_t *pio_mutex;
	extern _udi_channel_t *_udi_MA_chan;
	static struct {
		_udi_cb_t _cb;
		udi_cb_t v;
	} _cb;
	unsigned int test_num;

	posix_init(argc, argv, "v", &transtest_parseargs);

	_OSDEP_EVENT_INIT(&test_done);
	_OSDEP_SIMPLE_MUTEX_INIT(&test_cnt_mutex, "simple mutex");

	_cb.v.channel = _udi_MA_chan;

	/*
	 * Dummy up the underlying data structures that we know will be
	 * needed.  This s needed becuase we bypass the majority of the
	 * UDI environment initialization.
	 */
	_udi_MA_chan->chan_region->reg_node = malloc(sizeof (_udi_dev_node_t));
	pio_mutex =
		malloc(N_SERIALIZATION_DOMAINS * sizeof (_udi_pio_mutex_t));
	_udi_MA_chan->chan_region->reg_node->bind_channel = calloc(1,sizeof(void*));
	_udi_MA_chan->chan_region->reg_node->node_osinfo.posix_dev_node.bus_type = bt_system;
	_OSDEP_MUTEX_INIT(&pio_mutex->lock, "trans_lock");
	_udi_MA_chan->chan_region->reg_node->pio_mutexen = pio_mutex;
	_udi_MA_chan->chan_region->reg_node->n_pio_mutexen =
		N_SERIALIZATION_DOMAINS;


	for (test_num = 0;
	     test_num < sizeof (test_translist) / sizeof (test_translist_t);
	     test_num++) {
		/*
		 * SLEAZE ALERT: "Ensure" that the MA cb is non-busy when
		 * we use it for the next allocation.
		 */

		printf("----------- test_num = %d `%s' ------------\n",
		       test_num, test_translist[test_num].test_name);


		udi_cb_alloc(cb_alloc_callback, &_cb.v, 1, _udi_MA_chan);
		_OSDEP_EVENT_WAIT(&test_done);
	}

	udi_cb_alloc(got_entrypt_cb, &_cb.v, 1, _udi_MA_chan);
	_OSDEP_EVENT_WAIT(&test_done);

	udi_cb_alloc(got_end_cb, &_cb.v, 1, _udi_MA_chan);
	_OSDEP_EVENT_WAIT(&test_done);

	udi_cb_alloc_batch(got_map_sz_cbs, &_cb.v, 1, 6, FALSE, 0, NULL);
	_OSDEP_EVENT_WAIT(&test_done);

	_OSDEP_MUTEX_DEINIT(&pio_mutex->lock);
	_OSDEP_EVENT_DEINIT(&test_done);
	_OSDEP_SIMPLE_MUTEX_DEINIT(&test_cnt_mutex);
	posix_deinit();

	printf("Exiting\n");
	return 0;
}
