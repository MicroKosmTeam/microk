
/*
 * File: env/posix/piotest.c
 *
 * Test harness for PIO.
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

#include <stdlib.h>
#include <unistd.h>

#include <udi_env.h>
#include <posix.h>

static _OSDEP_EVENT_T test_done;

#define N_SERIALIZATION_DOMAINS 1

void
mem_alloc_cb(udi_cb_t *gcb,
	     void *new_mem)
{
	printf("udi_mem_alloc completed\n");
	udi_mem_free(new_mem);
	udi_cb_free(gcb);
	_OSDEP_EVENT_SIGNAL(&test_done);
}


void
piomap_cb(udi_cb_t *gcb,
	  udi_pio_handle_t new_handle)
{
	printf("pio map cb%p.  Handle %p\n", gcb, new_handle);

	udi_pio_unmap(new_handle);

	/*
	 * call udi_mem_alloc just to prove that we can 
	 */
	udi_mem_alloc(mem_alloc_cb, gcb, UDI_MIN_ALLOC_LIMIT, 0);

}

void
cancel_cb(udi_cb_t *gcb)
{
	printf("op cancelled\n");
}

udi_pio_trans_t test_trans[] = {
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0x0000}
};

void
cballoc(udi_cb_t *gcb,
	udi_cb_t *new_cb)
{
	printf("New cb allocated %p\n", new_cb);

	udi_pio_map(piomap_cb, new_cb, 0,	/* regset_idx */
		    0,			/* pio_offset */
		    0x0,		/* map_length */
		    test_trans,		/* trans_list */
		    sizeof (test_trans) / sizeof (udi_pio_trans_t),	/* list_length */
		    0,			/* pio_attributes */
		    0,			/* pace */
		    0			/* serialization domain */
		);

}

void
printtest(void)
{
	udi_busaddr64_t x = { 0x01234567, 0x89abcdef };
	udi_busaddr64_t x2 = { 0x00004567, 0x0000cdef };
	static char testbuf[1024];
	static const char lexpected[] = "123456789abcdef";
	static const char uexpected[] = "123456789ABCDEF";
	static const char x2lbuf[] = "45670000cdef";
	static const char x2ubuf[] = "45670000CDEF";
	static const char sbuf[] = "abcdef";
	static const char lbuf[] = "1 00000000000123456789abcdef 2";
	static const char rbuf[] = "           123456789abcdef";
	static const char vbuf[] = "12345678901234567890abz";
	static const char v2buf[] = "1, 1, 1\n";
	static const char samplebuf[] = "Register #  0: 0x0000c093 "
		"<Active,DMA Ready,RCV,Mode=Sync FDX,"
		"TX Threshold=2,RX Threshold=1>";
	void *ptr;
	int blah = 0x4321;		/* Value isn't used */
	char *mbuf = malloc(8);

	assert(mbuf);

	assert(15 == udi_snprintf(testbuf, sizeof (testbuf), "%a", x));
	assert(0 == strcmp(testbuf, lexpected));

	assert(15 == udi_snprintf(testbuf, sizeof (testbuf), "%A", x));
	assert(0 == strcmp(testbuf, uexpected));

	assert(6 == udi_snprintf(testbuf, sizeof (testbuf), "%06a", x));
	assert(0 == strcmp(testbuf, sbuf));

	assert(6 == udi_snprintf(testbuf, sizeof (testbuf), "%6a", x));
	assert(0 == strcmp(testbuf, sbuf));

	assert(6 == udi_snprintf(testbuf, sizeof (testbuf), "%-6a", x));
	assert(0 == strcmp(testbuf, sbuf));

	assert(12 == udi_snprintf(testbuf, sizeof (testbuf), "%a", x2));
	assert(0 == strcmp(testbuf, x2lbuf));

	assert(12 == udi_snprintf(testbuf, sizeof (testbuf), "%A", x2));
	assert(0 == strcmp(testbuf, x2ubuf));

	/*
	 * The tests for %[pP] are written to avoid introducing implicit 
	 * dependencies of the size of pointers on the host, yet test that
	 * all the bits are preserved.   This does test both a global 
	 * function and a local in the hopes that one of them will have 
	 * some of the "interesting"  (>32) bits set.
	 */
	udi_snprintf(testbuf, sizeof (testbuf), "%p", printtest);
	sscanf(testbuf, "%p", &ptr);
	assert(ptr == (void *)printtest);

	udi_snprintf(testbuf, sizeof (testbuf), "%P", printtest);
	sscanf(testbuf, "%p", &ptr);
	assert(ptr == (void *)printtest);

	udi_snprintf(testbuf, sizeof (testbuf), "%p", &blah);
	sscanf(testbuf, "%p", &ptr);
	assert(ptr == (void *)&blah);

	udi_snprintf(testbuf, sizeof (testbuf), "%P", &blah);
	sscanf(testbuf, "%p", &ptr);
	assert(ptr == (void *)&blah);

	/*
	 * Prove varargs handling is right 
	 */
	assert(30 == udi_snprintf(testbuf, sizeof (testbuf),
				  "%x %026a %x", 1, x, 2));
	assert(0 == strcmp(testbuf, lbuf));
	assert(23 == udi_snprintf(testbuf, sizeof (testbuf),
				  "%c%c%c%c%c%c%c%c%c%c%04x%08x%c",
				  '1', '2', '3', '4', '5', '6', '7', '8', '9',
				  '0', 0x1234, 0x567890ab, 'z'));
	assert(0 == strcmp(testbuf, vbuf));

	assert(8 == udi_snprintf(testbuf, sizeof (testbuf),
				 "%bx, %hx, %x\n", 0x1, 0x1, 0x1));
	assert(0 == strcmp(testbuf, v2buf));

	assert(26 == udi_snprintf(testbuf, sizeof (testbuf), "%26a", x));
	assert(0 == strcmp(testbuf, rbuf));

	/*
	 * Prove that truncated writes don't overwrite.  Rely on 
	 * * bounds-checked malloc to catch this.
	 */
	assert(7 == udi_snprintf(mbuf, 8, "%s", "Hello, world."));
	assert(0 == strcmp(mbuf, "Hello, "));

	/*
	 *  And the example from the udi_snprint man page.
	 */
	assert(92 == udi_snprintf(testbuf, sizeof (testbuf),
				  "%s %-2c %d: 0x%08x "
				  "%<15=Active,14=DMA Ready,13=XMIT,"
				  "~13=RCV,0-2=Mode:0=HDX:1=FDX"
				  ":2=Sync HDX:3=Sync FDX:4=Coded,"
				  "3-6=TX Threshold,7-10=RX Threshold>",
				  "Register", '#', 0, 0xc093, 0xc093));
	assert(0 == strcmp(testbuf, samplebuf));
}

void
endiantest(void)
{
	/*
	 * This is pretty grubby, I know... 
	 */

	int i;
	udi_ubit8_t *cbuf;
	udi_ubit8_t *tbuf;
	udi_ubit16_t *ub16p;
	udi_ubit32_t *ub32p;
	udi_ubit32_t magic = 0x01020304;
	udi_boolean_t host_big_endian = *(char *)&magic == 1;
	udi_ubit16_t tmp16;
	udi_ubit32_t tmp32;

#define CBUF_SIZE 256

	cbuf = malloc(CBUF_SIZE);
	tbuf = malloc(CBUF_SIZE);
	assert(cbuf);
	assert(tbuf);

	for (i = 0; i < CBUF_SIZE; i++)
		cbuf[i] = i;

	udi_memcpy(tbuf, cbuf, CBUF_SIZE);

	/*
	 * Prove the macros work in their simplest form. 
	 */
	tmp16 = UDI_ENDIAN_SWAP_16(0x0102);
	tmp32 = UDI_ENDIAN_SWAP_32(0x01020304);
	assert(tmp16 == 0x0201);
	assert(tmp32 == 0x04030201);

	/*
	 * Prove udi_endian_swap(swap_size=1)  is a NOP 
	 */
	udi_endian_swap(tbuf, tbuf, 1, 2, CBUF_SIZE);
	assert(0 == memcmp(tbuf, cbuf, CBUF_SIZE));

	/*
	 * Fill the test buffer, swap as 16's 
	 */
	udi_memcpy(tbuf, cbuf, CBUF_SIZE);
	udi_endian_swap(tbuf, tbuf, 2, 2, CBUF_SIZE / 2);

	ub16p = (udi_ubit16_t *)tbuf;
	for (i = 0; i < CBUF_SIZE / 2; i += 2, ub16p++) {
		udi_ubit16_t expected = (i << 8) | (i + 1);

		if (host_big_endian)
			expected = UDI_ENDIAN_SWAP_16(expected);
		assert(expected == *ub16p);
	}

	/*
	 * Fill test buffer, swap as 32's 
	 */
	udi_memcpy(tbuf, cbuf, CBUF_SIZE);
	udi_endian_swap(tbuf, tbuf, 4, 4, CBUF_SIZE / 4);

	ub32p = (udi_ubit32_t *)tbuf;
	for (i = 0; i < CBUF_SIZE / 4; i += 4, ub32p++) {
		udi_ubit32_t expected = (i << 24) |
			((i + 1) << 16) | ((i + 2) << 8) | ((i + 3));
		if (host_big_endian)
			expected = UDI_ENDIAN_SWAP_32(expected);
		assert(expected == *ub32p);
	}

	/*
	 * Fill test buffer, swap as 64 BYTES (not bits) 
	 */
	udi_memcpy(tbuf, cbuf, CBUF_SIZE);
	udi_endian_swap(tbuf, tbuf, 64, 64, 1);
	/*
	 * the first 64 bytes should be "mirrored", the remainder untouched 
	 */
	for (i = 0; i < CBUF_SIZE; i++) {
		if (i < 64) {
			assert(tbuf[i] == 63 - i);
			continue;
		};
		assert(tbuf[i] == i);
	}

	/*
	 * Fill test buffer, swap as 64 BYTES (not bits) 
	 */
	udi_memcpy(tbuf, cbuf, CBUF_SIZE);
	udi_endian_swap(tbuf, tbuf, 32, 64, 2);
	/*
	 * the first 32 bytes should be "mirrored", the next 32 untouched,
	 * the next 32 mirrored, the remainder  untouched.
	 */
	for (i = 0; i < CBUF_SIZE; i++) {
		if (i < 32) {
			assert(tbuf[i] == 31 - i);
			continue;
		};
		if (i < 64) {
			assert(tbuf[i] == i);
			continue;
		};
		if (i < 96) {
			assert(tbuf[i] == 95 - i + 64);
			continue;
		};
		assert(tbuf[i] == i);
	}

	/*
	 * Test the "stride" cases 
	 */
	udi_memcpy(tbuf, cbuf, CBUF_SIZE);
	udi_endian_swap(tbuf, tbuf, 4, 8, 32);

	ub32p = (udi_ubit32_t *)tbuf;
	for (i = 0; i < CBUF_SIZE; i += 4, ub32p++) {
		udi_ubit32_t expected = ((i + 0) << 24) |
			((i + 1) << 16) | ((i + 2) << 8) | ((i + 3));
		if (host_big_endian)
			expected = UDI_ENDIAN_SWAP_32(expected);
		if (i % 8 == 0)
			assert(expected == *ub32p);
		else
			assert(UDI_ENDIAN_SWAP_32(expected) == *ub32p);

	}

	/*
	 * And a few simple calls to with non-overlapping src/dst.  Really,
	 * we tested these cases implictly above...
	 */
	udi_memset(tbuf, 0xca, CBUF_SIZE);
	udi_endian_swap(cbuf, tbuf, 4, 8, 32);

	ub32p = (udi_ubit32_t *)tbuf;
	for (i = 0; i < CBUF_SIZE; i += 4, ub32p++) {
		udi_ubit32_t expected = ((i + 0) << 24) |
			((i + 1) << 16) | ((i + 2) << 8) | ((i + 3));
		if (host_big_endian)
			expected = UDI_ENDIAN_SWAP_32(expected);
		if (i % 8 == 0)
			assert(expected == *ub32p);
		else
			assert(0xcacacaca == *ub32p);
	}


	/*
	 * Fill test buffer, swap as 64 BYTES (not bits) 
	 */
	udi_memset(tbuf, 0xca, CBUF_SIZE);
	udi_endian_swap(cbuf, tbuf, 64, 64, 1);
	/*
	 * the first 64 bytes should be "mirrored", the remainder untouched 
	 */
	for (i = 0; i < CBUF_SIZE; i++) {
		if (i < 64) {
			assert(tbuf[i] == 63 - i);
			continue;
		};
		assert(tbuf[i] == 0xca);
	}

}

int
main(int argc,
     char *argv[])
{
	extern _udi_channel_t *_udi_MA_chan;
	_udi_pio_mutex_t *pio_mutex;
	static struct {
		_udi_cb_t _cb;
		udi_cb_t v;
	} _cb;

	posix_init(argc, argv, "", NULL);

	_OSDEP_EVENT_INIT(&test_done);

	printtest();

	endiantest();

	_cb.v.channel = _udi_MA_chan;
	/*
	 * Dummy up the underlying data structures that we know will be
	 * needed.  This is needed becuase we bypass the majority of the
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

	udi_cb_alloc(cballoc, &_cb.v, 1, _udi_MA_chan);

	_OSDEP_EVENT_WAIT(&test_done);
	_OSDEP_MUTEX_DEINIT(&pio_mutex->lock);
	_OSDEP_EVENT_DEINIT(&test_done);

	posix_deinit();

	printf("Exiting\n");
	return 0;
}
