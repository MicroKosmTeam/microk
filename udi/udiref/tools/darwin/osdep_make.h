const static char *glueincl = "\

#define UDI_VERSION 0x101
#include <udi.h>

";

const static char *gluehead = "\
#define DEBUGPRINT(x) kprintf x

/* Module exported entry points the OS looks for... */
int init_module(void);
void cleanup_module(void);

int init_module (void)
{
    extern udi_init_t udi_init_info;
    extern udi_mei_init_t udi_meta_info;

    DEBUGPRINT(\"udi: loading module \" UDI_MOD_NAME \"\\n\");
";

const static char *gluefoot = "\

void cleanup_module (void)
{
    DEBUGPRINT(\"udi: unloading module \" UDI_MOD_NAME \"\\n\");
";

const static char *driver_glue_head = "\
    return _udi_driver_load (UDI_MOD_NAME, &udi_init_info, sprops);
}
";

const static char *driver_glue_foot = "\
    DEBUGPRINT((\"Calling _udi_driver_unload('%%s')\\n\", UDI_MOD_NAME));
    _udi_driver_unload (UDI_MOD_NAME);
}
";

const static char *meta_glue_head = "\
    return _udi_meta_load (UDI_MOD_NAME, &udi_meta_info, sprops);
}
";

const static char *meta_glue_foot = "\
    DEBUGPRINT((\"Calling _udi_meta_unload('%%s')\\n\", UDI_MOD_NAME));
    _udi_meta_unload (UDI_MOD_NAME);
}
";

const static char *mapper_glue_head = "\
    return _udi_mapper_load (UDI_MOD_NAME, &udi_init_info, sprops);
}
";

const static char *mapper_glue_foot = "\
    DEBUGPRINT((\"Calling _udi_mapper_unload('%%s')\\n\", UDI_MOD_NAME));
    _udi_mapper_unload (UDI_MOD_NAME);
}
";
