/*
 * File: env/freebsd/pio_pops.h
 *
 *  PIO OPS template for FreeBSD environment.
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
 * FIXME: Future versions may want to use bus_space_{read,write}_n instead
 * of peeking directly tinto the memh.
 */

pop_op_template_t io_rd_ops[] =
{
	{"inb(%a + pio->pio_osinfo._u.io.addr)", "io_rd"},
	{"inw(%a + pio->pio_osinfo._u.io.addr)", "io_rd"},
	{"inl(%a + pio->pio_osinfo._u.io.addr)", "io_rd"},
	{NULL}
};

pop_op_template_t mem_rd_ops[] =
{
	{"*(%t*)&pio->pio_osinfo._u.mem.mapped_addr[%a]", "mem_rd"},
	{"*(%t*)&pio->pio_osinfo._u.mem.mapped_addr[%a]", "mem_rd"},
	{"*(%t*)&pio->pio_osinfo._u.mem.mapped_addr[%a]", "mem_rd"},
	{NULL}
};

pop_op_template_t cfg_rd_ops[] =
{
	{"d = pci_read_config(pio->pio_osinfo.device, "
		"pio->pio_osinfo._u.cfg.offset + %a, 1);",
		"pci_cfg_rd"},
	{"d = pci_read_config(pio->pio_osinfo.device, "
		"pio->pio_osinfo._u.cfg.offset + %a, 2);",
		"pci_cfg_rd"},
	{"d = pci_read_config(pio->pio_osinfo.device, "
		"pio->pio_osinfo._u.cfg.offset + %a, 4);",
		"pci_cfg_rd"},
	{NULL}
};

pop_op_template_t io_wr_ops[] =
{
	{"outb(%a + pio->pio_osinfo._u.io.addr, d);", "io_wr"},
	{"outw(%a + pio->pio_osinfo._u.io.addr, d);", "io_wr"},
	{"outl(%a + pio->pio_osinfo._u.io.addr, d);", "io_wr"},
	{NULL}
};

pop_op_template_t mem_wr_ops[] =
{
  {"*(%t*) &pio->pio_osinfo._u.mem.mapped_addr[%a]  = d;", "mem_wr"},
  {"*(%t*) &pio->pio_osinfo._u.mem.mapped_addr[%a]  = d;", "mem_wr"},
  {"*(%t*) &pio->pio_osinfo._u.mem.mapped_addr[%a]  = d;", "mem_wr"},
  {NULL}
};

pop_op_template_t cfg_wr_ops[] =
{
	{"pci_write_config(pio->pio_osinfo.device, "
		"pio->pio_osinfo._u.cfg.offset + %a, d, 1);",
		"pci_cfg_wr"},
	{"pci_write_config(pio->pio_osinfo.device, "
		"pio->pio_osinfo._u.cfg.offset + %a, d, 2);",
		"pci_cfg_wr"},
	{"pci_write_config(pio->pio_osinfo.device, "
		"pio->pio_osinfo._u.cfg.offset + %a, d, 4);",
		"pci_cfg_wr"},
	{NULL}
};


pop_op_template_t read_proto[] =
{"\n\
static void \n\
%p_%t (udi_ubit32_t offset, void *data, udi_size_t write_size, _udi_pio_handle_t *pio) { \n\
	%t d; \n\
	%P \n\
	d = %s; \n\
	%d = %o (d); \n\
 } \n", "read"
};

pop_op_template_t cfg_read_proto[] =
{"\n\
static void \n\
%p_%t (udi_ubit32_t offset, void *data, udi_size_t write_size, _udi_pio_handle_t *pio) { \n\
	%t d; \n\
	%P \n\
	%s; \n\
	%d = %o (d);\n}\n", "read"
};

pop_op_template_t write_proto[] =
{"\n\
static void \n\
%p_%t (udi_ubit32_t offset, void *data, udi_size_t write_size, _udi_pio_handle_t *pio) { \n\
	%t d; \n\
	%P \n\
	d = %o(%d); \n\
	%s; \n\
} \n", "write"
};

pop_op_template_t cfg_write_proto[] =
{"\n\
static void \n\
%p_%t (udi_ubit32_t offset, void *data, udi_size_t write_size, _udi_pio_handle_t *pio) { \n\
	%t d; \n\
	%P \n\
	d = %o(%d); \n\
	%s; \n\
} \n", "write"
};


/* FIXME: There are a _lot_ of optimization opportunities here.  We could
 * delay for just the remaing time.  We could spinwait for a short period
 * and sleep for longer periods (if we could figure out if this was running
 * in interrupt context, at least.)
 */

char pause_proto[] =
"	udi_timestamp_t pt = pio->access_timestamp;\n\
	_OSDEP_GET_CURRENT_TIME(pio->access_timestamp);\n\
	if ( pio->access_timestamp - pt < pio->pace) { \n\
/* TODO: Implement pacing */ \n\
_OSDEP_ASSERT(0); \n\
	} \
";

pop_action_template_t mem_ops[] =
{
	{read_proto, mem_rd_ops},
	{write_proto, mem_wr_ops},
	{NULL, NULL}
};

pop_action_template_t io_ops[] =
{
	{read_proto, io_rd_ops},
	{write_proto, io_wr_ops},
	{NULL, NULL}
};

pop_action_template_t cfg_ops[] =
{
	{cfg_read_proto, cfg_rd_ops},
	{cfg_write_proto, cfg_wr_ops},
	{NULL, NULL}
};

all_action_template_t all_ops[] =
{
        {mem_ops, "mem", AT_FLAG_SIMPLE_MANY_ONLY },
        {io_ops, "io", AT_FLAG_SIMPLE_MANY_ONLY },
        {cfg_ops, "pci_cfg", AT_FLAG_SIMPLE_ONLY|AT_FLAG_SIMPLE_MANY_ONLY },
        {NULL, NULL, 0}
};

