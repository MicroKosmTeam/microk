
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
#include "transbench.h"

#define LATER _i386

#include <posix.h>

#define N_SERIALIZATION_DOMAINS 20	/* Normally from udiprops.txt */
#define PIO_EXIT_SUCCESS 0x1234		/* "magic" number to prove we didn't abort
					 * somewhere in the translist 
					 */

_OSDEP_EVENT_T test_done;
_OSDEP_SIMPLE_MUTEX_T test_cnt_mutex;
STATIC volatile int test_cnt;

int verbose = 0;

/*
 * Called 'rdata' in most places becuase that's sort of how we're handling
 * it but since we aren't really in a region, that's not entirely accurate.
 */
typedef struct {
	int iterations_remaining;
	int start_label;		/* First label to run, inclusive */
	int end_label;			/* Last label to run, inclusive */
	udi_pio_handle_t pio_handle;
	udi_ubit32_t pio_attributes;
	udi_timestamp_t start_time;
	udi_pio_trans_t *trans_list;
	udi_ubit16_t trans_list_size;
} trans_info_t;

long trans;

STATIC void
map_sz_trans_done(udi_cb_t *gcb,
		  udi_buf_t *buf,
		  udi_status_t status,
		  udi_ubit16_t result)
{
	static int map_sz_cnt;
	udi_time_t t;
	trans_info_t *rdata = gcb->initiator_context;

	trans++;

	if (--rdata->iterations_remaining) {
		udi_pio_trans(map_sz_trans_done, gcb, rdata->pio_handle,
			      rdata->start_label, NULL, gcb->context);
		return;
	}

	t = udi_time_since(rdata->start_time);
	printf(" %d", t.seconds * 1000 + t.nanoseconds/1000000);
	fflush(stdout);


	udi_mem_free(gcb->context);
	udi_pio_unmap(rdata->pio_handle);
	udi_cb_free(gcb);

	map_sz_cnt++;
	if (map_sz_cnt >= rdata->end_label) {
		printf("\n");
		map_sz_cnt = 0;
		_OSDEP_EVENT_SIGNAL(&test_done);
	}
}

STATIC void
got_map_sz_pio_mapping(udi_cb_t *gcb,
		       udi_pio_handle_t pio_handle)
{
	trans_info_t *rdata = gcb->initiator_context;

	rdata->pio_handle = pio_handle;

	rdata->start_time = udi_time_current();
	udi_pio_trans(map_sz_trans_done, gcb, pio_handle,
		      rdata->start_label, NULL, gcb->context);

}


STATIC void
got_map_sz_mem(udi_cb_t *gcb,
	       void *new_mem)
{
	trans_info_t *rdata = gcb->initiator_context;

	gcb->context = new_mem;

	udi_pio_map(got_map_sz_pio_mapping, gcb, 3,	/* regset_idx */
		    0x0,		/* pio_offset */
		    100,		/* Length */
		    rdata->trans_list,	/* trans_list */
		    rdata->trans_list_size, rdata->pio_attributes,	/* pio_attributes */
		    0,			/* pace */
		    0			/* serialization domain */
		);
}

STATIC void
got_cb_chain(udi_cb_t *gcb,
	     udi_cb_t *new_cb)
{
	udi_cb_t *next_cb;
	trans_info_t *parent_rdata = gcb->initiator_context;
	int n = parent_rdata->start_label;

	for (; new_cb; new_cb = next_cb, n++) {
		trans_info_t *rdata;

		next_cb = new_cb->initiator_context;

		rdata = calloc(1, sizeof (trans_info_t));
		new_cb->initiator_context = rdata;

		rdata->start_label = n;
		rdata->end_label = parent_rdata->end_label;
		rdata->trans_list = parent_rdata->trans_list;
		rdata->trans_list_size = parent_rdata->trans_list_size;
		rdata->pio_attributes = parent_rdata->pio_attributes;
		rdata->iterations_remaining =
			parent_rdata->iterations_remaining;

		udi_mem_alloc(got_map_sz_mem, new_cb, 1000, UDI_MEM_MOVABLE);
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
	trans_info_t *rdata;
	int i;

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
	_udi_MA_chan->chan_region->reg_node->bind_channel = calloc(1,sizeof(void *));
	pio_mutex =
		malloc(N_SERIALIZATION_DOMAINS * sizeof (_udi_pio_mutex_t));
	_OSDEP_MUTEX_INIT(&pio_mutex->lock, "trans_lock");
	_udi_MA_chan->chan_region->reg_node->pio_mutexen = pio_mutex;
	_udi_MA_chan->chan_region->reg_node->n_pio_mutexen =
		N_SERIALIZATION_DOMAINS;

	rdata = calloc(1, sizeof (trans_info_t));;
	_cb.v.initiator_context = rdata;

#define asizeof(ar) (sizeof(ar) / sizeof(ar[0]))
#define NITER 500
#if 1
	printf("LOAD_IMM ");
	rdata->trans_list = &load_imm_list[0];
	rdata->trans_list_size = asizeof(load_imm_list);
	rdata->iterations_remaining = NITER;
	rdata->end_label = 6;
	rdata->start_label = 1;
	rdata->pio_attributes = UDI_PIO_LITTLE_ENDIAN;
	udi_cb_alloc_batch(got_cb_chain, &_cb.v, 1, 6, FALSE, 0, NULL);
	_OSDEP_EVENT_WAIT(&test_done);

	printf("XOR ");
	rdata->trans_list = &xor_list[0];
	rdata->trans_list_size = asizeof(xor_list);
	rdata->iterations_remaining = NITER;
	rdata->end_label = 6;
	rdata->start_label = 1;
	rdata->pio_attributes = UDI_PIO_LITTLE_ENDIAN;
	udi_cb_alloc_batch(got_cb_chain, &_cb.v, 1, 6, FALSE, 0, NULL);
	_OSDEP_EVENT_WAIT(&test_done);

	printf("SHIFT_LEFT ");
	rdata->trans_list = &shift_left_list[0];
	rdata->trans_list_size = asizeof(shift_left_list);
	rdata->iterations_remaining = NITER;
	rdata->end_label = 6;
	rdata->start_label = 1;
	rdata->pio_attributes = UDI_PIO_LITTLE_ENDIAN;
	udi_cb_alloc_batch(got_cb_chain, &_cb.v, 1, 6, FALSE, 0, NULL);
	_OSDEP_EVENT_WAIT(&test_done);
#endif
	printf("PIO_IN DIRECT ");
	rdata->trans_list = &in_direct_list[0];
	rdata->trans_list_size = asizeof(in_direct_list);
	rdata->iterations_remaining = NITER;
	rdata->start_label = 1;
	rdata->end_label = 6;
	rdata->pio_attributes = UDI_PIO_LITTLE_ENDIAN;
	udi_cb_alloc_batch(got_cb_chain, &_cb.v, 1, 6, FALSE, 0, NULL);
	_OSDEP_EVENT_WAIT(&test_done);
#if 1
	printf("PIO_IN MEM ");
	rdata->trans_list = &in_mem_list[0];
	rdata->trans_list_size = asizeof(in_mem_list);
	rdata->iterations_remaining = NITER;
	rdata->start_label = 1;
	rdata->end_label = 6;
	rdata->pio_attributes = UDI_PIO_LITTLE_ENDIAN;
	udi_cb_alloc_batch(got_cb_chain, &_cb.v, 1, 6, FALSE, 0, NULL);
	_OSDEP_EVENT_WAIT(&test_done);

	printf("PIO_END list ");
	rdata->trans_list = &end_list[0];
	rdata->trans_list_size = asizeof(end_list);
	rdata->iterations_remaining = 1000000;
	rdata->start_label = 0;
	rdata->end_label = 0;
	trans = 0;
	i = 1 + rdata->end_label + rdata->start_label;
#if 0
	printf("Ops: %d\n",rdata->trans_list_size *  rdata->iterations_remaining * i);
#endif
	rdata->pio_attributes = UDI_PIO_LITTLE_ENDIAN;
	udi_cb_alloc_batch(got_cb_chain, &_cb.v, 1, i, FALSE, 0, NULL);
	_OSDEP_EVENT_WAIT(&test_done);
#if 0
	printf("Trans: %ld\n", trans);
#endif
#endif

	_OSDEP_MUTEX_DEINIT(&pio_mutex->lock);
	_OSDEP_EVENT_DEINIT(&test_done);
	_OSDEP_SIMPLE_MUTEX_DEINIT(&test_cnt_mutex);
	posix_deinit();

	printf("Exiting\n");
	return 0;
}
