/*
 * ========================================================================
 * FILE: guinea.h
 * ========================================================================
 *
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
 *
 */
 
typedef enum {
	DATA_TO_PROVIDER,		/* Generate pattern for host */
	DATA_FROM_PROVIDER,		/* Swallow data coming from host */
	RETURN_DATA_FROM_PROVIDER	/* simple Loopback mode */
} test_state_t; 

typedef struct {
	udi_queue_t	Q;
	udi_ubit32_t	child_id;
	udi_ubit8_t	data[200];
} guinea_child_data_t;
/*
 * Region data 
 */
typedef struct guinea_region_data {
        udi_init_context_t	init_context;
	test_state_t		testmode;
	udi_ubit32_t		testcounter;
	udi_ubit8_t		buf[1024]; /* A 'holding' place for data */
	udi_bus_bind_cb_t	*bus_bind_cb;
	udi_gio_xfer_cb_t 	*rx_cb, *tx_cb;
	udi_gio_bind_cb_t 	*rx_bind_cb, tx_bind_cb;
	udi_cb_t		*gcb1, *gcb2;
	udi_ubit8_t		enum_counter;
	udi_queue_t		child_data_list;
	udi_enumerate_cb_t	*enum_new_cb;
	udi_boolean_t 		hot_added;
} guinea_region_data_t;

