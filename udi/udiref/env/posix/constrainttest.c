
/*
 * File: env/posix/constrainttest.c
 *
 * Verify that simple constraints management plumbing works.   
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

#define FAILURE 0
#define SUCCESS 1

udi_dma_constraints_t constraints, orig_constraints;
_udi_dma_constraints_t chkconstr;

udi_dma_constraints_attr_spec_t attr_list[] = {
	{110, 32},
	{111, 1},
	{120, 20},
	{123, 64},
	{124, 128}
};

#define N_ATTR	(sizeof(attr_list) / sizeof(attr_list[0]))

void start_test0(udi_cb_t *),
  start_test1(udi_cb_t *),
  start_test2(udi_cb_t *),
  start_test3(udi_cb_t *),
  start_test4(udi_cb_t *);
void dflt_constr(void),
  modified_constr(void);

struct test {
	void (*startf) (udi_cb_t *);
	void (*resultf) (void);
} tests[] = {
	{
	start_test0, dflt_constr}, {
	start_test1, dflt_constr}, {
	start_test2, modified_constr}, {
	start_test3, dflt_constr}, {
	start_test4, modified_constr}
};

#define N_TESTS	(sizeof(tests) / sizeof(tests[0]))

int test_num;

extern _udi_constraints_range_types _udi_constraints_range_attributes[];
extern udi_dma_constraints_attr_t _udi_constraints_index[];

typedef struct {
	char *attr_type;
	udi_ubit32_t attr_value;
	udi_ubit32_t attr_intend;
} result_t;

char *constr_type[] = {

/*	"UDI_XFER_MAX",
	"UDI_XFER_TYPICAL",
	"UDI_XFER_GRANULARITY",
	"UDI_XFER_ONE_PIECE",
	"UDI_XFER_EXACT_SIZE",
	"UDI_XFER_NO_REORDER",
	"UDI_BUF_HEADER_SIZE",
	"UDI_BUF_TRAILER_SIZE",
*/
	"UDI_DMA_DATA_ADDRESSABLE_BITS",
	"UDI_DMA_NO_PARTIAL",
	"UDI_DMA_SCGTH_MAX_ELEMENTS",
	"UDI_DMA_SCGTH_FORMAT",
	"UDI_DMA_SCGTH_ENDIANNESS",
	"UDI_DMA_SCGTH_ADDRESSABLE_BITS",
	"UDI_DMA_SCGTH_MAX_SEGMENTS",
	"UDI_DMA_SCGTH_ALIGNMENT_BITS",
	"UDI_DMA_SCGTH_MAX_EL_PER_SEG",
	"UDI_DMA_SCGTH_PREFIX_BYTES",
	"UDI_DMA_ELEMENT_ALIGNMENT_BITS",
	"UDI_DMA_ELEMENT_LENGTH_BITS",
	"UDI_DMA_ELEMENT_GRANULARITY_BITS",
	"UDI_DMA_ADDR_FIXED_BITS",
	"UDI_DMA_ADDR_FIXED_TYPE",
	"UDI_DMA_ADDR_FIXED_VALUE_LO",
	"UDI_DMA_ADDR_FIXED_VALUE_HI",
	"UDI_DMA_SEQUENTIAL",
	"UDI_DMA_SLOP_IN_BITS",
	"UDI_DMA_SLOP_OUT_BITS",
	"UDI_DMA_SLOP_OUT_EXTRA",
	"UDI_DMA_SLOP_BARRIER_BITS",
};

/* Build a set of dummy data until we have more environmental 
 * services up.
 */
_udi_region_t *myregion;

_OSDEP_DEV_NODE_T osinfo;

udi_cb_t *volatile test_gcb;
volatile udi_boolean_t test_done;
udi_status_t test_status;

void
printconstr(udi_dma_constraints_t constraints)
{
	udi_dma_constraints_attr_t attr_type;
	_udi_dma_constraints_t *constr = (_udi_dma_constraints_t *)constraints;

	printf("    udi_dma_constraints_t (0x%p)\n", constr);

	for (attr_type = 0; attr_type < _UDI_CONSTRAINTS_INDEX_MAX;
	     attr_type++) {
		printf("      %32s = %u (%u %u %u %u %u %d)\n",
		       constr_type[attr_type],
		       *(((udi_ubit32_t *)constr->attributes)
			 + attr_type),
		       _udi_constraints_range_attributes
		       [attr_type].min_value,
		       _udi_constraints_range_attributes
		       [attr_type].max_value,
		       _udi_constraints_range_attributes
		       [attr_type].least_restr,
		       _udi_constraints_range_attributes
		       [attr_type].most_restr,
		       _udi_constraints_range_attributes
		       [attr_type].default_value,
		       _udi_constraints_range_attributes
		       [attr_type].restr_type);
	}
}

void
constr_check(udi_dma_constraints_t new_constraints,
	     _udi_dma_constraints_t *chk_constr,
	     result_t result[])
{
	udi_dma_constraints_attr_t index;
	int fails;

	_udi_dma_constraints_t *new_constr =
		(_udi_dma_constraints_t *)new_constraints;

	fails = 0;

	for (index = 0; index < _UDI_CONSTRAINTS_INDEX_MAX; index++) {
		if (new_constr->attributes[index] !=
		    chk_constr->attributes[index]) {
			result[fails].attr_type = constr_type[index];
			result[fails].attr_value =
				new_constr->attributes[index];
			result[fails].attr_intend =
				chk_constr->attributes[index];
			fails++;
		}
	}

	if (fails < _UDI_CONSTRAINTS_INDEX_MAX - 1) {
		result[fails].attr_type = NULL;
	}
}

void
dflt_constr(void)
{
	int count;

	for (count = 0; count < _UDI_CONSTRAINTS_INDEX_MAX; count++) {
		chkconstr.attributes[count] =
			_udi_constraints_range_attributes[count].default_value;
	}
}

void
modified_constr(void)
{
	int count;

	dflt_constr();

	for (count = 0; count < N_ATTR; count++) {
		chkconstr.attributes[_udi_constraints_index
				     [attr_list[count].attr_type - 1] - 1] =
			attr_list[count].attr_value;
	}
}

void
compare_results(void)
{
	result_t chkresult[_UDI_CONSTRAINTS_INDEX_MAX];

	chkresult[0].attr_type = "";

	constr_check(constraints, &chkconstr, chkresult);

	if (test_status != UDI_OK) {
		printf("TEST %d: FAILED -- udi_status 0x%x\n",
		       test_num, test_status);
		abort();
	} else if (chkresult[0].attr_type == NULL) {
		printf("TEST %d: SUCCEEDED\n", test_num);
	} else {
		int count = 0;

		printf("TEST %d: FAILED!\n", test_num);
		printf("  Intended/Actual:\n");
		while (count < _UDI_CONSTRAINTS_INDEX_MAX &&
		       chkresult[count].attr_type != NULL) {
			printf("    %32s = %u / %u\n",
			       chkresult[count].attr_type,
			       chkresult[count].attr_intend,
			       chkresult[count].attr_value);
			count++;
		}
		abort();
	}
}

void
constr_create_done(udi_cb_t *gcb,
		   udi_dma_constraints_t new_constraints,
		   udi_status_t status)
{
	constraints = new_constraints;
	test_status = status;
	test_done = TRUE;
}

void
start_test0(udi_cb_t *gcb)
{
	printf("  TEST 0: initialize constraints\n");
	udi_dma_constraints_attr_set(constr_create_done, gcb,
				     UDI_NULL_DMA_CONSTRAINTS, NULL, 0, 0);
}

/* ARGSUSED */
void
start_test1(udi_cb_t *gcb)
{
	udi_dma_constraints_attr_t index;
	_udi_dma_constraints_t *c;

	printf("  TEST 1: reset all constraints\n");
	constraints = (udi_dma_constraints_t)
		_OSDEP_MEM_ALLOC(sizeof (_udi_dma_constraints_t), 0,
				 UDI_WAITOK);
	c = (_udi_dma_constraints_t *)constraints;
	UDI_QUEUE_INIT(&c->buf_path_q);
	c->alloc_hdr.ah_type = _UDI_ALLOC_HDR_CONSTRAINT;
	_udi_add_alloc_to_tracker(_UDI_GCB_TO_CHAN(gcb)->chan_region,
				  &c->alloc_hdr);

	for (index = 0; index < _UDI_CONSTRAINTS_TYPES_MAX; index++) {
		if (_udi_constraints_index[index]) {
			udi_dma_constraints_attr_reset(constraints, index + 1);
		}
	}

	test_done = TRUE;
}

void
constr_set(udi_cb_t *gcb,
	   udi_dma_constraints_t new_constraints,
	   udi_status_t status)
{
	constraints = new_constraints;
	test_status = status;
	test_done = TRUE;
}

void
start_test2(udi_cb_t *gcb)
{
	printf("  TEST 2: set constraints\n");
	udi_dma_constraints_attr_set(constr_set, gcb,
				     UDI_NULL_DMA_CONSTRAINTS,
				     attr_list, N_ATTR, 0);
}

void
t3_constr_set(udi_cb_t *gcb,
	      udi_dma_constraints_t new_constraints,
	      udi_status_t status)
{
	test_status = status;
	test_done = TRUE;

	udi_dma_constraints_free(new_constraints);
}

void
t3_constr_created(udi_cb_t *gcb,
		  udi_dma_constraints_t new_constraints,
		  udi_status_t status)
{
	constraints = new_constraints;	/* Remember version before copy */

	udi_dma_constraints_attr_set(t3_constr_set, gcb,
				     new_constraints,
				     attr_list, N_ATTR,
				     UDI_DMA_CONSTRAINTS_COPY);
}

void
start_test3(udi_cb_t *gcb)
{
	printf("  TEST 3: copy constraints -- check original unmodified\n");
	udi_dma_constraints_attr_set(t3_constr_created, gcb,
				     UDI_NULL_DMA_CONSTRAINTS, NULL, 0, 0);
}

void
t4_constr_set(udi_cb_t *gcb,
	      udi_dma_constraints_t new_constraints,
	      udi_status_t status)
{
	constraints = new_constraints;
	test_status = status;
	test_done = TRUE;

	udi_dma_constraints_free(orig_constraints);
}

void
t4_constr_created(udi_cb_t *gcb,
		  udi_dma_constraints_t new_constraints,
		  udi_status_t status)
{
	orig_constraints = new_constraints;	/* Remember version before copy */

	udi_dma_constraints_attr_set(t4_constr_set, gcb,
				     new_constraints,
				     attr_list, N_ATTR,
				     UDI_DMA_CONSTRAINTS_COPY);
}

void
start_test4(udi_cb_t *gcb)
{
	printf("  TEST 4: copy constraints -- check new copy\n");
	udi_dma_constraints_attr_set(t4_constr_created, gcb,
				     UDI_NULL_DMA_CONSTRAINTS, NULL, 0, 0);
}

void
cballoc(udi_cb_t *gcb,
	udi_cb_t *new_cb)
{
	printf("New cb allocated %p\n", new_cb);
	fflush(stdout);
	test_gcb = new_cb;
}

int
main(int argc,
     char **argv)
{
	extern _udi_channel_t *_udi_MA_chan;
	struct {
		_udi_cb_t _cb;
		udi_cb_t v;
	} _cb;

	posix_init(argc, argv, "", NULL);

	/*
	 * Allocate a control block by bootstrapping off of the MA channel. 
	 */
	_cb.v.channel = _udi_MA_chan;
	_cb._cb.cb_allocm.alloc_state = _UDI_ALLOC_IDLE;
	udi_cb_alloc(cballoc, &_cb.v, 1, _udi_MA_chan);
	while (test_gcb == NULL)
		sleep(1);

	for (test_num = 0; test_num < N_TESTS; test_num++) {
		test_status = UDI_OK;
		test_done = FALSE;

		(*tests[test_num].startf) (test_gcb);
		while (!test_done)
			sleep(1);

		printconstr(constraints);

		/*
		 * Check results. First, create expectations. 
		 */
		(*tests[test_num].resultf) ();
		compare_results();

		/*
		 * Free the constraints structure the test created. 
		 */
		udi_dma_constraints_free(constraints);
	}

	udi_cb_free(test_gcb);

	posix_deinit();

	assert(Allocd_mem == 0);
	printf("Exiting\n");
	return 0;
}
