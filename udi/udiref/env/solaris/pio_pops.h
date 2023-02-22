/*
 *  PIO OPS template for UnixWare.
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
	{"cm_read_devconfig8 (pio->pio_osinfo.cfg_key, "
		"pio->pio_osinfo._u.cfg.offset + %a, &d);", 
		"pci_cfg_rd"},
	{"cm_read_devconfig16(pio->pio_osinfo.cfg_key, "
		"pio->pio_osinfo._u.cfg.offset + %a, &d);", 
		"pci_cfg_rd"},
	{"cm_read_devconfig32(pio->pio_osinfo.cfg_key, "
		"pio->pio_osinfo._u.cfg.offset + %a, &d);", 
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
	{"cm_write_devconfig8 (pio->pio_osinfo.cfg_key, "
		"pio->pio_osinfo._u.cfg.offset + %a, &d);", 
		"pci_cfg_wr"},
	{"cm_write_devconfig16(pio->pio_osinfo.cfg_key, "
		"pio->pio_osinfo._u.cfg.offset + %a, &d);", 
		"pci_cfg_wr"},
	{"cm_write_devconfig32(pio->pio_osinfo.cfg_key, "
		"pio->pio_osinfo._u.cfg.offset + %a, &d);", 
		"pci_cfg_wr"},
	{NULL}
};


pop_op_template_t read_proto[] =
{"\n\
static void \n\
%p_%t (udi_ubit32_t offset, void *data, udi_size_t write_size, _udi_pio_handle_t *pio) { \n\
	%t d; \n\
	d = %s; \n\
	%d = %o (d); \n\
	%P \n\
 } \n", "read"
};

pop_op_template_t cfg_read_proto[] =
{"\n\
static void \n\
%p_%t (udi_ubit32_t offset, void *data, udi_size_t write_size, _udi_pio_handle_t *pio) \n{ \n\
	%t d; \n\
	%s; \n\
	%d = %o (d);\n\
	%P \n\
	}\n", "read"
};

pop_op_template_t write_proto[] =
{"\n\
static void \n\
%p_%t (udi_ubit32_t offset, void *data, udi_size_t write_size, _udi_pio_handle_t *pio) \n{ \n\
	%t d; \n\
	d = %o(%d); \n\
	%s; \n\
	%P \n\
} \n", "write"
};

pop_op_template_t cfg_write_proto[] =
{"\n\
static void \n\
%p_%t (udi_ubit32_t offset, void *data, udi_size_t write_size, _udi_pio_handle_t *pio) \n{ \n\
	%t d; \n\
	d = %o(%d); \n\
	%s; \n\
	%P \n\
} \n", "write"
};

/* FIXME: There are a _lot_ of optimization opportunities here.  We could
 * delay for just the remaing time.  We could spinwait for a short period
 * and sleep for longer periods (if we could figure out if this was running
 * in anything but interrupt context, at least.)
 */

/*
 * This code knows that a udi_timestamp_t is actually kernel clock_t
 * and that access_timestamp is stored in TICKS units.
 * It currently doesn't take advantage of an opportunity to sleep
 * for less than the pace time if we had PIO computations happening
 * that added up to an interesting amount of time.
 */

char pause_proto[] =
"	{\n\
		clock_t dt = TICKS_SINCE(pio->access_timestamp); \n\
		if (drv_hztousec(dt) < pio->pace)\n\
			drv_usecwait(pio->pace); \n\
		pio->access_timestamp = TICKS();\n\
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
	NULL
};
