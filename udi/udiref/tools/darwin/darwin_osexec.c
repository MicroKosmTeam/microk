#if 0
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "osdep.h"
#include "global.h"
#include "common_api.h"
#endif

#ifdef OS_DELETE_OVERRIDE
void os_delete(char *file)
{
    fprintf(stderr, "unlinking('%s')\n", file);
    if (unlink(file)) {
        printloc(stderr, UDI_MSG_FATAL, 1008, "unable to unlink file %s\n",
                         file);
    }
}
#endif

