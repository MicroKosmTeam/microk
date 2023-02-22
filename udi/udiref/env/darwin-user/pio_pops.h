/*
 *  PIO OPS template for Darwin/POSIX environment.
 */

pop_op_template_t io_rd_ops[] =
{
	{"inb(%a + pio->pio_osinfo._u.io.addr)", "io_rd"},
	{"inw(%a + pio->pio_osinfo._u.io.addr)", "io_rd"},
	{"inl(%a + pio->pio_osinfo._u.io.addr)", "io_rd"},
	{NULL, NULL}
};

pop_op_template_t mem_rd_ops[] =
{
	{"*(%t*)&pio->pio_osinfo._u.mem.mapped_addr[%a]", "mem_rd"},
	{"*(%t*)&pio->pio_osinfo._u.mem.mapped_addr[%a]", "mem_rd"},
	{"*(%t*)&pio->pio_osinfo._u.mem.mapped_addr[%a]", "mem_rd"},
	{NULL, NULL}
};

#if 0
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
	{NULL, NULL}
};
#endif
pop_op_template_t io_wr_ops[] =
{
	{"outb(%a + pio->pio_osinfo._u.io.addr, d);", "io_wr"},
	{"outw(%a + pio->pio_osinfo._u.io.addr, d);", "io_wr"},
	{"outl(%a + pio->pio_osinfo._u.io.addr, d);", "io_wr"},
	{NULL, NULL}
};

pop_op_template_t mem_wr_ops[] =
{
  {"*(%t*) &pio->pio_osinfo._u.mem.mapped_addr[%a]  = d;", "mem_wr"},
  {"*(%t*) &pio->pio_osinfo._u.mem.mapped_addr[%a]  = d;", "mem_wr"},
  {"*(%t*) &pio->pio_osinfo._u.mem.mapped_addr[%a]  = d;", "mem_wr"},
  {NULL, NULL}
};

#if 0
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
	{NULL, NULL}
};
#endif

pop_op_template_t read_proto[] =
{
{"\n\
static void \n\
%p_%t (udi_ubit32_t offset, void *data, udi_size_t write_size, _udi_pio_handle_t *pio) { \n\
	%t d; \n\
	%P \n\
	(void)write_size; \n\
	d = %s; \n\
	%d = %o (d); \n\
 } \n", "read"
}, {NULL, NULL}
};

#if 0
pop_op_template_t cfg_read_proto[] =
{
{"\n\
static void \n\
%p_%t (udi_ubit32_t offset, void *data, udi_size_t write_size, _udi_pio_handle_t *pio) { \n\
	%t d; \n\
	%P \n\
	(void)write_size; \n\
	%s; \n\
	%d = %o (d); \n\
} \n", "read"
},
{NULL, NULL}
};
#endif

pop_op_template_t write_proto[] =
{
{"\n\
static void \n\
%p_%t (udi_ubit32_t offset, void *data, udi_size_t write_size, _udi_pio_handle_t *pio) { \n\
	%t d; \n\
	%P \n\
	(void)write_size; \n\
	d = %o(%d); \n\
	%s; \n\
} \n", "write"
},
{NULL, NULL}
};

#if 0
pop_op_template_t cfg_write_proto[] =
{
{"\n\
static void \n\
%p_%t (udi_ubit32_t offset, void *data, udi_size_t write_size, _udi_pio_handle_t *pio) { \n\
	%t d; \n\
	%P \n\
	(void)write_size; \n\
	d = %o(%d); \n\
	%s; \n\
} \n", "write"
},
{NULL, NULL}
};
#endif


/* FIXME: There are a _lot_ of optimization opportunities here.  We could
 * delay for just the remaing time.  We could spinwait for a short period
 * and sleep for longer periods (if we could figure out if this was running
 * in interrupt context, at least.)
 */

char pause_proto[] =
"	udi_timestamp_t pt = pio->access_timestamp;\n\
	_OSDEP_GET_CURRENT_TIME(pio->access_timestamp);\n\
	if ( pio->access_timestamp - pt < pio->pace) { \n\
		usleep(pio->pace/1000); \n\
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

#if 0
pop_action_template_t cfg_ops[] =
{
	{cfg_read_proto, cfg_rd_ops},
	{cfg_write_proto, cfg_wr_ops},
	{NULL, NULL}
};
#endif

all_action_template_t all_ops[] =
{
	{mem_ops, "mem", /*AT_FLAG_SIMPLE_ONLY | */ AT_FLAG_SIMPLE_MANY_ONLY },
	{io_ops, "io", /*AT_FLAG_SIMPLE_ONLY | */ AT_FLAG_SIMPLE_MANY_ONLY },
/*	{cfg_ops, "pci_cfg"},  */
	{NULL, NULL, 0}
};
