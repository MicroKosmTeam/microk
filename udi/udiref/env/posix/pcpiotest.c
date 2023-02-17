
/*
 * File: env/posix/pcpiotest.c
 *
 * Test PC PIO.
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
 * Execute a number of PC architecture trans_lists for exercising actual
 * PIO backends, funky addressing modes, etc.
 * *** NOTE: these tests aren't very automated right now.   If they don't
 * drop core, they pass. (sigh.)
 */


#include <udi_env.h>
#include <posix.h>

#define PIO_EXIT_SUCCESS 0x2345		/* "magic" number to prove we didn't
					 * abort somewhere in the translist 
					 */
#define N_SERIALIZATION_DOMAINS 2

/*
 * This seriously violates the spirit of UDI having global data...
 */

typedef void translist_test_fn_t(void *membuf);
typedef struct {
	translist_test_fn_t *test_fn;
	udi_index_t regset_idx;
	size_t list_size;
	udi_pio_trans_t *trans_list;
} test_translist_t;

_OSDEP_EVENT_T test_ev;

char read_data[32];

/* 
 * 1. Read first word of (mem).   Xor it with 0xffff.  Write back to mem[0].
 * 2. Fill rest of *mem with 
 */

static udi_pio_trans_t iotrans_list[] = {
	/*
	 * R0 <- zero 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0},
	/*
	 * R2:2 = 0xffff 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, 0xffff},
	/*
	 * R1:4 <- mem[r0] 
	 */
	{UDI_PIO_LOAD + UDI_PIO_MEM + UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1},

	/*
	 * r1 ^= r2 
	 */
	{UDI_PIO_XOR + UDI_PIO_R1, UDI_PIO_4BYTE, UDI_PIO_R2},
	/*
	 * mem[r0] = R1 
	 */
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R1},

	/*
	 * R6  = offset into the destination buffer 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R6, UDI_PIO_2BYTE, 4},
	/*
	 * R0  = cmos data register 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0x71},
	/*
	 * R2 = 0.   Byte in CMOS to read. 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, 0x00},
	/*
	 * R3 = CMOS address register 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R3, UDI_PIO_2BYTE, 0x70},
	/*
	 * R4 = remaining count. 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R4, UDI_PIO_2BYTE, 200},
	/*
	 * R4 -= R6.  (Offset into dest buffer) 
	 */
	{UDI_PIO_SUB + UDI_PIO_R4, UDI_PIO_4BYTE, UDI_PIO_R6},

	/*
	 * TOP OF LOOP 
	 */
	{UDI_PIO_LABEL, 0, 0x234},
	/*
	 * pio[R3]:1 <- R2 
	 */
	{UDI_PIO_OUT_IND + UDI_PIO_R2, UDI_PIO_1BYTE, UDI_PIO_R3},
	/*
	 * R1:1 <- pio[R0] 
	 */
	{UDI_PIO_IN_IND + UDI_PIO_R1, UDI_PIO_1BYTE, UDI_PIO_R0},
	/*
	 * mem[R6]:1 <- R1 
	 */
	{UDI_PIO_STORE + UDI_PIO_MEM + UDI_PIO_R6, UDI_PIO_1BYTE, UDI_PIO_R1},
	/*
	 * R6 = R6 + 1.  (Inc dest buffer offset)  
	 */
	{UDI_PIO_ADD_IMM + UDI_PIO_R6, UDI_PIO_1BYTE, 1},
	/*
	 * R4 -- (decrement bytes remaining) 
	 */
	{UDI_PIO_ADD_IMM + UDI_PIO_R4, UDI_PIO_2BYTE, (udi_ubit16_t)-1},

	/*
	 * If R4 != 0, goto top of loop 
	 */
	{UDI_PIO_CSKIP + UDI_PIO_R4, UDI_PIO_4BYTE, UDI_PIO_Z},
	{UDI_PIO_BRANCH, 0, 0x234},

	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, PIO_EXIT_SUCCESS},
};

#define SCRATCH_ADDR 1
#define SCRATCH_COUNT 2
#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

static udi_pio_trans_t cmos_trans_read_byte[] = {
	/*
	 * R0 <- SCRATCH_ADDR {offset into scratch of address} 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, SCRATCH_ADDR},
	/*
	 * R1 <- address 
	 */
#if 0
	{UDI_PIO_LOAD + UDI_PIO_SCRATCH + UDI_PIO_R0, UDI_PIO_1BYTE,
	 UDI_PIO_R1},
#else
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 0x00},
#endif
	/*
	 * R0 <- SCRATCH_COUNT {offset into scratch of count} 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, SCRATCH_COUNT},
	/*
	 * R2 <- count 
	 */
#if 0
	{UDI_PIO_LOAD + UDI_PIO_SCRATCH + UDI_PIO_R0, UDI_PIO_1BYTE,
	 UDI_PIO_R2},
#else
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, 0x10},
#endif
	/*
	 * R0 <- 0 {current buffer offset} 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0},
	/*
	 * begin main loop 
	 */
	{UDI_PIO_LABEL, 0, 1},
	/*
	 * output address from R1 to address register 
	 */
	{UDI_PIO_OUT + UDI_PIO_DIRECT + UDI_PIO_R1, UDI_PIO_1BYTE, CMOS_ADDR},
#if 0
	/*
	 * input value from data register into next buffer location 
	 */
	{UDI_PIO_IN + UDI_PIO_BUF + UDI_PIO_R0, UDI_PIO_1BYTE, CMOS_DATA},
#else
	{UDI_PIO_IN + UDI_PIO_MEM + UDI_PIO_R0, UDI_PIO_1BYTE, CMOS_DATA},
#endif
	/*
	 * decrement count (in R2) 
	 */
	{UDI_PIO_ADD_IMM + UDI_PIO_R2, UDI_PIO_1BYTE, (udi_ubit16_t)-1},
	/*
	 * if count is zero, we're done 
	 */
	{UDI_PIO_CSKIP + UDI_PIO_R2, UDI_PIO_1BYTE, UDI_PIO_NZ},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, PIO_EXIT_SUCCESS},
	/*
	 * increment address and buffer offset 
	 */
	{UDI_PIO_ADD_IMM + UDI_PIO_R1, UDI_PIO_1BYTE, 1},
	{UDI_PIO_ADD_IMM + UDI_PIO_R0, UDI_PIO_1BYTE, 1},
	/*
	 * go back to main loop 
	 */
	{UDI_PIO_BRANCH, 0, 1},
};

static udi_pio_trans_t test_rep_list[] = {
	/*
	 * R0 <- {mem buffer offset} 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 8},
	/*
	 * R1 <- {ROM offset} 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R1, UDI_PIO_2BYTE, 4},
	/*
	 * R2 <- {count} 
	 */
	{UDI_PIO_LOAD_IMM + UDI_PIO_R2, UDI_PIO_2BYTE, 0x20},
	/*
	 * repeated input 
	 */
	{UDI_PIO_REP_IN_IND, UDI_PIO_4BYTE,
	 UDI_PIO_REP_ARGS(UDI_PIO_MEM, UDI_PIO_R0, 1,
			  UDI_PIO_R1, 1, UDI_PIO_R2)},

	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, PIO_EXIT_SUCCESS},
};

void
test_io(void *buf)
{
	int i;
	unsigned char *c = (unsigned char *)buf;
	udi_ubit32_t *u = (udi_ubit32_t *)buf;

	assert(u[0] == (0xcacacaca ^ 0xffff));

	for (i = 0; i < 256; i++) {
		printf("%x ", c[i]);
	}
	free(buf);
}

void
test_cmos(void *buf)
{
	int i;
	unsigned char *c = (unsigned char *)buf;

	for (i = 0; i < 256; i++) {
		printf("%x ", c[i]);
	}
	free(buf);

}

void
test_rep(void *buf)
{
	int i;
	unsigned char *c = (unsigned char *)buf;

	for (i = 0; i < 256; i++) {
		printf("%x ", c[i]);
	}
	free(buf);
}

test_translist_t test_translist[] = {
	{test_io, UDI_SYSBUS_MEM_BUS,
	 sizeof (iotrans_list) / sizeof (udi_pio_trans_t),
	 iotrans_list},
	{test_cmos, UDI_SYSBUS_MEM_BUS,
	 sizeof (cmos_trans_read_byte) / sizeof (udi_pio_trans_t),
	 cmos_trans_read_byte},
	{test_rep, UDI_SYSBUS_MEM_BUS,
	 sizeof (test_rep_list) / sizeof (udi_pio_trans_t),
	 test_rep_list}
};



static void
trans_cb(udi_cb_t *gcb,
	 udi_buf_t *buf,
	 udi_status_t status,
	 udi_ubit16_t result)
{
	printf("trans cb %p.  Status 0x%x. Result 0x%x\n",
	       gcb, status, result);
	assert(result == PIO_EXIT_SUCCESS);
	udi_pio_unmap(gcb->initiator_context);
}

static void
piomap_cb(udi_cb_t *gcb,
	  udi_pio_handle_t new_handle)
{
	char *memb;

	/*
	 * By divine knowledge, we "know" there are two words in the scratch
	 * * space of these cb's.  We'll cheat and use them.
	 */
	int *scratch = gcb->scratch;
	int test_num = scratch[0];

	memb = malloc(1000);
	if (memb == NULL) {
		fprintf(stderr,
			"piomapcb malloc of temp buffer failed.  Exiting.\n");
		abort();
	}
	udi_memset(memb, 0xca, 1000);
	gcb->initiator_context = new_handle;
	udi_pio_trans(trans_cb, gcb, new_handle, 0, NULL, memb);
	(test_translist[test_num].test_fn) (memb);
}

STATIC void
probe_rd_ok_cb(udi_cb_t *gcb,
	       udi_status_t status)
{
	assert(status == UDI_OK);
	udi_pio_unmap(gcb->initiator_context);
	udi_cb_free(gcb);
}

STATIC void
probe_rd_fail_cb(udi_cb_t *gcb,
		 udi_status_t status)
{
	assert(status == UDI_STAT_HW_PROBLEM);
	udi_pio_unmap(gcb->initiator_context);
	udi_cb_free(gcb);
}

STATIC void
probemap_cb(udi_cb_t *gcb,
	    udi_pio_handle_t new_handle)
{
	gcb->initiator_context = new_handle;
	udi_pio_probe(probe_rd_fail_cb, gcb, new_handle, &read_data, 10000,	/* known bad offset */
		      UDI_PIO_4BYTE, UDI_PIO_IN);
}

STATIC void
probemap_ok_cb(udi_cb_t *gcb,
	       udi_pio_handle_t new_handle)
{
	gcb->initiator_context = new_handle;
	udi_pio_probe(probe_rd_ok_cb, gcb, new_handle, &read_data, 0,	/* good offset */
		      UDI_PIO_4BYTE, UDI_PIO_IN);
}

static void
cballoc(udi_cb_t *gcb,
	udi_cb_t *new_cb)
{
	int test_num;
	int *scratch;

	scratch = new_cb->scratch;

	for (test_num = 0;
	     test_num < sizeof (test_translist) / sizeof (test_translist_t);
	     test_num++) {
		scratch[0] = test_num;
		udi_pio_map(piomap_cb, new_cb, test_translist[test_num].regset_idx,	/* regset_idx */
			    0,		/* pio_offset */
			    0x4321,	/* Length */
			    test_translist[test_num].trans_list,	/* trans_list */
			    test_translist[test_num].list_size,	/* list length */
			    UDI_PIO_LITTLE_ENDIAN,	/* pio_attributes */
			    0,		/* pace */
			    0		/* serialization domain */
			);

/* FIXME: This is to get forgiveness becuase I'm scribbling into the MEI
 * scratch space.
 */
		sleep(1);
	}
}


static void
probe_cb(udi_cb_t *gcb,
	 udi_cb_t *new_cb)
{
	udi_pio_map(probemap_cb, new_cb, 2,	/* pick a regset_idx where we know we can fault. */
		    0,			/* pio_offset */
		    0x20,		/* Length */
		    NULL,		/* trans_list */
		    0,			/* list length */
		    UDI_PIO_NEVERSWAP,	/* pio_attributes */
		    0,			/* pace */
		    0			/* serialization domain */
		);
}

static void
probe_ok_cb(udi_cb_t *gcb,
	    udi_cb_t *new_cb)
{
	udi_pio_map(probemap_ok_cb, new_cb, 2,	/* pick a regset_idx where we know we can fault. */
		    0,			/* pio_offset */
		    0x20,		/* Length */
		    NULL,		/* trans_list */
		    0,			/* list length */
		    UDI_PIO_NEVERSWAP,	/* pio_attributes */
		    0,			/* pace */
		    0			/* serialization domain */
		);
}

int
main(int argc,
     char *argv[])
{
	extern _udi_channel_t *_udi_MA_chan;
	static struct {
		_udi_cb_t _cb;
		udi_cb_t v;
	} _cb;
	_udi_pio_mutex_t *pio_mutex;


	_OSDEP_EVENT_INIT(&test_ev);

	posix_init(argc, argv, "", NULL);

	_cb.v.channel = _udi_MA_chan;

	/*
	 * Dummy up the underlying data structures that we know will be
	 * needed.  This s needed becuase we bypass the majority of the
	 * UDI environment initialization.
	 */
	_udi_MA_chan->chan_region->reg_node = malloc(sizeof (_udi_dev_node_t));
	pio_mutex =
		malloc(N_SERIALIZATION_DOMAINS * sizeof (_udi_pio_mutex_t));
	_udi_MA_chan->chan_region->reg_node->bind_channel = calloc(1,sizeof(void *));
	_OSDEP_MUTEX_INIT(&pio_mutex->lock, "trans_lock");
	_udi_MA_chan->chan_region->reg_node->pio_mutexen = pio_mutex;
	_udi_MA_chan->chan_region->reg_node->n_pio_mutexen =
		N_SERIALIZATION_DOMAINS;


	udi_cb_alloc(cballoc, &_cb.v, 1, _udi_MA_chan);
	sleep(1);
	udi_cb_alloc(probe_cb, &_cb.v, 1, _udi_MA_chan);
	sleep(1);
	udi_cb_alloc(probe_ok_cb, &_cb.v, 1, _udi_MA_chan);
	sleep(1);

	posix_deinit();

	printf("Exiting\n");
	return 0;
}
