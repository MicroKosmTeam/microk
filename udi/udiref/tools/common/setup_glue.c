/*
 * File: tools/common/setup_glue.c
 *
 * UDI Setup Tool support for creating glue code to provide static
 * properties and OS interface hooks attachments to the distributed
 * objects.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include "y.tab.h"
#include "osdep.h"
#include "global.h"
#include "param.h"
#include "spparse.h"
#include "common_api.h"

#define UDI_VERSION 0x101
#include <udi.h>
#include "../../env/common/udi_sprops_ra.h"


#ifdef OS_GLUE_PROPS_STRUCT

char name[513];
int udi_version;
int desc_index;
int version;
int pioserlim;
int mt_count, cb_count, pb_count, ib_count, pv_count, rf_count,
            l_count, mf_count, ms_count, r_count, d_count,
            e_count, a_count;
char modtype[256];
char *desc;


#define NUM_FIND 13 /* Things for sprops to loop through and find */


void write_props_glue (FILE *fp, propline_t **data, int install_src)
{
    int i, index, find_type, first, found, tmp_count = 0;
    char *ptr1;
    char buf[1024];
    char locale[256];
    char cur_loc[256];
    unsigned int raflag, overrun;
    int nmsgs = 0, msgnum, metaidx;
    int bridgemetaidx, havehw, modtypeindex;
    int fd, nr;
    struct stat fst;

    /*
     * #define UDI_VERSION ... and #include <udi.h> should already be
     * written to the file.
     */

    /* Scan for various things */
    bridgemetaidx = pioserlim = -1;
    index = 0;
    while (udi_global.propdata[index] != NULL) {
        if (udi_global.propdata[index]->type == SHORTNAME) {
            strcpy(name, udi_global.propdata[index+1]->string);
        } else if (udi_global.propdata[index]->type == CATEGORY) {
            modtypeindex = udi_global.propdata[index+1]->ulong;
        } else if (udi_global.propdata[index]->type == NAME) {
            desc_index = udi_global.propdata[index+1]->ulong;
        } else if (udi_global.propdata[index]->type == PROPVERS) {
            version = udi_global.propdata[index+1]->ulong;
        } else if (udi_global.propdata[index]->type == PIOSERLIMIT) {
            pioserlim = udi_global.propdata[index+1]->ulong;
        } else if (udi_global.propdata[index]->type == REQUIRES) {
            if (strcmp(udi_global.propdata[index+1]->string, "udi") == 0)
                udi_version = udi_global.propdata[index+2]->ulong;
        } else if (udi_global.propdata[index]->type == META) {
            if (strcmp(udi_global.propdata[index+2]->string, "udi_bridge")
                    == 0)
                bridgemetaidx = udi_global.propdata[index+1]->ulong;
        } else if (udi_global.propdata[index]->type == REGION) {
            if (udi_global.propdata[index+1]->ulong == 0) {
                i = index + 2;
            }
        }
        while (udi_global.propdata[index] != NULL) index++;
        index++;
    }

/*kwq: modtype is not used...     get_mod_type(modtype); */

    /*
     * Find the child_bind_ops to determine the driver type.
     * Also, check for parent_bind_ops using the udi_bridge meta.
     */
    index = havehw = 0;
    while (udi_global.propdata[index] != NULL) {
        if (udi_global.propdata[index]->type == PARBOPS) {
            if (udi_global.propdata[index+1]->ulong == bridgemetaidx)
                havehw = 1;
        }
        while (udi_global.propdata[index] != NULL) index++;
        index++;
    }

    /* Create the various typedefs */
    fprintf(fp, "typedef struct {\n");
    fprintf(fp, "\tint\tnum;\n");
    fprintf(fp, "\tvoid\t*off;\n");
    fprintf(fp, "} _udi_sprops_idx_t;\n\n");
    fprintf(fp, "typedef struct {\n");
    fprintf(fp, "\tint\tmeta_idx;\n");
    fprintf(fp, "\tchar\t*if_name;\n");
    fprintf(fp, "} _udi_sp_meta_t;\n\n");
    fprintf(fp, "typedef struct {\n");
    fprintf(fp, "\tint\tmeta_idx;\n");
    fprintf(fp, "\tint\tops_idx;\n");
    fprintf(fp, "\tint\tregion_idx;\n");
    fprintf(fp, "\tint\tcb_idx;\n");
    fprintf(fp, "} _udi_sp_bindops_t;\n\n");
    fprintf(fp, "typedef struct {\n");
    fprintf(fp, "\tint\tmeta_idx;\n");
    fprintf(fp, "\tint\tregion_idx;\n");
    fprintf(fp, "\tint\tprimops_idx;\n");
    fprintf(fp, "\tint\tsecops_idx;\n");
    fprintf(fp, "\tint\tcb_idx;\n");
    fprintf(fp, "} _udi_sp_ibindops_t;\n\n");
    fprintf(fp, "typedef struct {\n");
    fprintf(fp, "\tchar\t*if_name;\n");
    fprintf(fp, "\tint\tver;\n");
    fprintf(fp, "} _udi_sp_provide_t;\n\n");
    fprintf(fp, "typedef struct {\n");
    fprintf(fp, "\tint\t\tregion_idx;\n");
    fprintf(fp, "\tunsigned int\tregion_attr;\n");
    fprintf(fp, "\tunsigned int\tover_time;\n");
    fprintf(fp, "} _udi_sp_region_t;\n\n");
    fprintf(fp, "typedef struct {\n");
    fprintf(fp, "\tint\tmeta_idx;\n");
    fprintf(fp, "\tint\tindex;\n");
    fprintf(fp, "\tint\tnum;\n");
    fprintf(fp, "} _udi_sp_dev_t;\n\n");
    fprintf(fp, "typedef struct {\n");
    fprintf(fp, "\tint\tmeta_idx;\n");
    fprintf(fp, "\tunsigned int\tmin_num;\n");
    fprintf(fp, "\tunsigned int\tmax_num;\n");
    fprintf(fp, "\tint\tindex;\n");
    fprintf(fp, "\tint\tnum;\n");
    fprintf(fp, "} _udi_sp_enum_t;\n\n");
    fprintf(fp, "typedef struct {\n");
    fprintf(fp, "\tchar\t*name;\n");
    fprintf(fp, "\tint\ttype;\n");
    fprintf(fp, "\tunion _udi_sp_attr_value {\n");
    fprintf(fp, "\t\tchar\t*string;\n");
    fprintf(fp, "\t\tint\tinteg;\n");
    fprintf(fp, "\t} value;\n");
    fprintf(fp, "} _udi_sp_attr_t;\n\n");
    fprintf(fp, "typedef struct {\n");
    fprintf(fp, "\tchar\t*file;\n");
    fprintf(fp, "\tchar\t*filename;\n");
    fprintf(fp, "\tunsigned int\tlen;\n");
    fprintf(fp, "\tunsigned char\t*data;\n");
    fprintf(fp, "} _udi_sp_readfile_t;\n\n");
    fprintf(fp, "typedef struct {\n");
    fprintf(fp, "\tchar\t*loc_str;\n");
    fprintf(fp, "\tint\tindex;\n");
    fprintf(fp, "\tint\tnum;\n");
    fprintf(fp, "} _udi_sp_locale_t;\n\n");
    fprintf(fp, "typedef struct {\n");
    fprintf(fp, "\tchar\t*loc_str;\n");
    fprintf(fp, "\tchar\t*filename;\n");
    fprintf(fp, "} _udi_sp_msgfile_t;\n\n");
    fprintf(fp, "typedef struct {\n");
    fprintf(fp, "\tint\tmsgnum;\n");
    fprintf(fp, "\tchar\t*text;\n");
    fprintf(fp, "} _udi_sp_message_t;\n\n");

    /* Create the udi_xxx_sprops_t structure */
    mt_count = cb_count = pb_count = ib_count = pv_count = rf_count =
            l_count = mf_count = ms_count = r_count = d_count =
            e_count = a_count = 0;
    fprintf(fp, "typedef struct {\n");
    fprintf(fp, "\tchar\t\t\t*shortname;\n");
    fprintf(fp, "\tint\t\t\tversion;\n");
    fprintf(fp, "\tint\t\t\trelease;\n");
    fprintf(fp, "\tint\t\t\tpioserlim;\n");
    for (find_type = 0; find_type < NUM_FIND; find_type++) {
        first = 1;
        index = 0;

        while (udi_global.propdata[index] != NULL) {
            switch(find_type) {
            case 0: /* metas */
                if (udi_global.propdata[index]->type == META)
                    mt_count++;
                break;
            case 1: /* child_bindops */
                if (first) {
                    if (mt_count == 0)
                        mt_count++;
                    fprintf(fp, "\t_udi_sp_meta_t"
                            "\t\tmetas"
                            "[%d];\n", mt_count);
                    first = 0;
                }
                if (udi_global.propdata[index]->type == CHLDBOPS)
                    cb_count++;
                break;
            case 2: /* parent_bindops */
                if (first) {
                    if (cb_count == 0)
                        cb_count++;
                    fprintf(fp, "\t_udi_sp_bindops_t"
                            "\tchild_bindops"
                            "[%d];\n", cb_count);
                    first = 0;
                }
                if (udi_global.propdata[index]->type == PARBOPS)
                    pb_count++;
                break;
            case 3: /* intern_bindops */
                if (first) {
                    if (pb_count == 0)
                        pb_count++;
                    fprintf(fp, "\t_udi_sp_bindops_t"
                            "\tparent_bindops"
                            "[%d];\n", pb_count);
                    first = 0;
                }
                if (udi_global.propdata[index]->type == INTBOPS)
                    ib_count++;
                break;
            case 4: /* provides */
                if (first) {
                    if (ib_count == 0)
                        ib_count++;
                    fprintf(fp, "\t_udi_sp_ibindops_t"
                            "\tintern_bindops"
                            "[%d];\n", ib_count);
                    first = 0;
                }
                if (udi_global.propdata[index]->type == PROVIDES)
                    pv_count++;
                break;
            case 5: /* regions */
                if (first) {
                    if (pv_count == 0)
                        pv_count++;
                    fprintf(fp, "\t_udi_sp_provide_t"
                            "\tprovides"
                            "[%d];\n", pv_count);
                    first = 0;
                }
                if (udi_global.propdata[index]->type == REGION)
                    r_count++;
                break;
            case 6: /* devices */
                if (first) {
                    if (r_count == 0)
                        r_count++;
                    fprintf(fp, "\t_udi_sp_region_t\t"
                            "regions[%d];\n",
                            r_count);
                    first = 0;
                }
                if (udi_global.propdata[index]->type == DEVICE)
                    d_count++;
                break;
            case 7: /* enumerates */
                if (first) {
                    if (d_count == 0)
                        d_count++;
                    fprintf(fp, "\t_udi_sp_dev_t\t"
                            "devices[%d];\n",
                            d_count);
                    first = 0;
                }
                if (udi_global.propdata[index]->type == ENUMERATES)
                    e_count++;
                break;
            case 8: /* attributes */
                if (first) {
                    if (e_count == 0)
                        e_count++;
                    fprintf(fp, "\t_udi_sp_enum_t\t\t"
                            "enumerates[%d];\n",
                            e_count);
                    first = 0;
                }
                if (udi_global.propdata[index]->type == DEVICE) {
                    i = index+3;
                    while (udi_global.propdata[i] != NULL) {
                        i+=3;
                    }
                    i -= index+3;
                    i /= 3;
                    a_count += i;
                }
                if (udi_global.propdata[index]->type == ENUMERATES) {
                    i = index+5;
                    while (udi_global.propdata[i] != NULL) {
                        i+=3;
                    }
                    i -= index+3;
                    i /= 3;
                    a_count += i;
                }
                break;
            case 9: /* readable_files */
                if (first) {
                    if (a_count == 0) {
                        a_count++;
                    }
                    fprintf(fp, "\t_udi_sp_attr_t\t\t"
                            "attribs[%d];\n",
                            a_count);
                    first = 0;
                }
                if (udi_global.propdata[index]->type == READFILE)
                    rf_count++;
                break;
            case 10: /* locale */
                if (first) {
                    if (rf_count == 0)
                        rf_count++;
                    fprintf(fp, "\t_udi_sp_readfile_t\t"
                            "read_files[%d];\n",
                            rf_count);
                    first = 0;
                    l_count = 1;
                }
                if (udi_global.propdata[index]->type == LOCALE)
                    l_count++;
                break;
            case 11: /* message files */
                if (first) {
                    fprintf(fp, "\t_udi_sp_locale_t\t"
                            "locale[%d];\n",
                            l_count);
                    first = 0;
                }
                if (udi_global.propdata[index]->type == MESGFILE) {
                    /*
                     * Read in, and count the number of
                     * locales.
                     */
                    FILE *mfp;
                    int msgf = 0;
                    if (install_src) {
                        sprintf(buf, "../msg/%s",
                            udi_global.propdata[index+1]->
                            string);
                    } else {
                        sprintf(buf, "../../msg/%s",
                            udi_global.propdata[index+1]->
                            string);
                    }
                    if ((mfp = fopen(buf, "r")) == NULL) {
                        printloc(stderr, UDI_MSG_FATAL, 1515, "Error: unable "
                            "to open message file "
                            "%s for reading!\n",
                            buf);
                    }
                    while(fgets(buf, 1023, mfp) != NULL) {
                        if (strncmp("locale ", buf, 7)
                                == 0) {
                            if (mf_count == 0 &&
                                msgf != 0)
                                mf_count++;
                            mf_count++;
                        } else if (strncmp("message ",
                                buf, 8) == 0 ||
                                strncmp("disast"
                                "er_message ",
                                buf,17) == 0) {
                            if (mf_count == 0) mf_count++;
                            msgf++;
                        }
                    }
                    if (mf_count == 0 && msgf != 0)
                        mf_count++;
                    fclose(mfp);
                }
                break;
            case 12: /* messages */
                if (first) {
                    if (mf_count == 0)
                        mf_count++;
                    fprintf(fp, "\t_udi_sp_msgfile_t\t"
                            "msg_files[%d];\n",
                            mf_count);
                    first = 0;
                }
                if (udi_global.propdata[index]->type == MESSAGE ||
                        udi_global.propdata[index]->type == DISMESG) {
                    ms_count++;
                    if (udi_global.propdata[index+1]->ulong ==
                            desc_index) {
			desc = optinit();
			parse_message(&desc,
				      udi_global.propdata[index+2]->string,
				      NULL);
                    }
                }
                break;
            }
            while(udi_global.propdata[index] != NULL) index++;
            index++;
        }
    }
    fprintf(fp, "\t_udi_sp_message_t\tmessages[%d];\n", ms_count+1);
    fprintf(fp, "} udi_%s_sprops_t;\n\n", name);

    /* Create the udi_???_sprops structure */
    mt_count = cb_count = pb_count = ib_count = pv_count = rf_count =
            l_count = mf_count = ms_count = r_count = d_count =
            e_count = a_count = 0;
    fprintf(fp, "udi_%s_sprops_t udi_%s_sprops = {\n", name, name);
    fprintf(fp, "\t/* Static Properties Shortname */\n");
    fprintf(fp, "\t\"%s\",\n", name);
    fprintf(fp, "\t/* Static Properties Version */\n");
    fprintf(fp, "\t0x%x,\n", version);
    fprintf(fp, "\t/* Static Properties Structure Release */\n");
    fprintf(fp, "\t1,\n");
    fprintf(fp, "\t/* Static Properties PIO Serialization Limit */\n");
    fprintf(fp, "\t%d,\n", pioserlim);

    for (find_type = 0; find_type < NUM_FIND; find_type++) {
        first = 1;
        index = 0;

        while (udi_global.propdata[index] != NULL) {
            switch(find_type) {
            case 0: /* metas */
                if (first) {
                    fprintf(fp, "\t/* metas */\n");
                    fprintf(fp, "\t{");
                    first = 0;
                }
                if (udi_global.propdata[index]->type == META) {
                    mt_count++;
                    fprintf(fp, "\n\t\t{%ld, \"%s\"},",
                        udi_global.propdata[index+1]->ulong,
                        udi_global.propdata[index+2]->string);
                }
                break;
            case 1: /* child_bind_ops */
                if (first) {
                    if (mt_count == 0) {
                        fprintf(fp, "\n\t\t{-1, \"\"}");
                    }
                    else {
                        fseek(fp, -1, SEEK_CUR); /* delete last comma */
                    }
                    fprintf(fp, "\n\t},\n");
                    fprintf(fp, "\t/* child_bind_ops */\n");
                    fprintf(fp, "\t{");
                    first = 0;
                }
                if (udi_global.propdata[index]->type == CHLDBOPS) {
                    cb_count++;
                    fprintf(fp, "\n\t\t{%ld, %ld, %ld, -1},",
                        udi_global.propdata[index+1]->ulong,
                        udi_global.propdata[index+2]->ulong,
                        udi_global.propdata[index+3]->ulong);
                }
                break;
            case 2: /* parent_bind_ops */
                if (first) {
                    if (cb_count == 0) {
                        fprintf(fp, "\n\t\t{-1, -1, -1, -1}");
                    }
                    else {
                        fseek(fp, -1, SEEK_CUR); /* delete last comma */
                    }
                    fprintf(fp, "\n\t},\n");
                    fprintf(fp, "\t/* parent_bind_ops */\n");
                    fprintf(fp, "\t{");
                    first = 0;
                }
                if (udi_global.propdata[index]->type == PARBOPS) {
                    pb_count++;
                    fprintf(fp, "\n\t\t{%ld, %ld, %ld, %ld},",
                        udi_global.propdata[index+1]->ulong,
                        udi_global.propdata[index+2]->ulong,
                        udi_global.propdata[index+3]->ulong,
                        udi_global.propdata[index+4]->ulong);
                }
                break;
            case 3: /* internal_bind_ops */
                if (first) {
                    if (pb_count == 0) {
                        fprintf(fp, "\n\t\t{-1, -1, -1, -1}");
                    }
                    else {
                        fseek(fp, -1, SEEK_CUR); /* delete last comma */
                    }
                    fprintf(fp, "\n\t},\n");
                    fprintf(fp, "\t/* internal_bind_ops */\n");
                    fprintf(fp, "\t{");
                    first = 0;
                }
                if (udi_global.propdata[index]->type == INTBOPS) {
                    ib_count++;
                    fprintf(fp, "\n\t\t{%ld, %ld, %ld, %ld, %ld},",
                        udi_global.propdata[index+1]->ulong,
                        udi_global.propdata[index+2]->ulong,
                        udi_global.propdata[index+3]->ulong,
                        udi_global.propdata[index+4]->ulong,
                        udi_global.propdata[index+5]->ulong);
                }
                break;
            case 4: /* provides */
                if (first) {
                    if (ib_count == 0) {
                        fprintf(fp, "\n\t\t{-1, -1, -1, -1, -1}");
                    }
                    else {
                        fseek(fp, -1, SEEK_CUR); /* delete last comma */
                    }
                    fprintf(fp, "\n\t},\n");
                    fprintf(fp, "\t/* provides */\n");
                    fprintf(fp, "\t{");
                    first = 0;
                }
                if (udi_global.propdata[index]->type == PROVIDES) {
                    pv_count++;
                    fprintf(fp, "\n\t\t{\"%s\", %ld},",
                        udi_global.propdata[index+1]->string,
                        udi_global.propdata[index+2]->ulong);
                }
                break;
            case 5: /* regions */
                if (first) {
                    if (pv_count == 0) {
                        fprintf(fp, "\n\t\t{\"\", -1}");
                    }
                    else {
                        fseek(fp, -1, SEEK_CUR); /* delete last comma */
                    }
                    fprintf(fp, "\n\t},\n");
                    fprintf(fp, "\t/* regions */\n");
                    fprintf(fp, "\t{");
                    first = 0;
                }
                if (udi_global.propdata[index]->type == REGION) {
                    r_count++;
                    fprintf(fp, "\n\t\t{%ld, ",
                        udi_global.propdata[index+1]->ulong);
                    raflag = overrun = 0;
                    i = index+2;
                    while (udi_global.propdata[i] != NULL) {
                        if (strcmp(udi_global.propdata[i]->
                            string, "overrun_time")
                                != 0) {
                            raflag |= udi_global.propdata
                                [i+1]->type;
                        } else {
                            overrun = udi_global.propdata
                                [i+1]->ulong;
                        }
                        i+=2;
                    }
                    /*
                     * And set any defaults, if a
                     * particular set was not mentioned
                     */
                    if(! (raflag&_UDI_RA_TYPE_MASK))
                        raflag |= _UDI_RA_NORMAL;
                    if(! (raflag&_UDI_RA_BINDING_MASK))
                        raflag |= _UDI_RA_STATIC;
                    if(! (raflag&_UDI_RA_PRIORITY_MASK))
                        raflag |= _UDI_RA_MED;
                    if(! (raflag&_UDI_RA_LATENCY_MASK))
                        raflag |= _UDI_RA_NON_OVERRUN;
                    /* Set the values */
                    fprintf(fp, "0x%x, 0x%x},", raflag,
                            overrun);
                }
                break;
            case 6: /* devices */
                if (first) {
                    if (r_count == 0) {
                        fprintf(fp, "\n\t\t{-1, 0, 0}");
                    }
                    else {
                        fseek(fp, -1, SEEK_CUR); /* delete last comma */
                    }
                    fprintf(fp, "\n\t},\n");
                    fprintf(fp, "\t/* devices */\n");
                    fprintf(fp, "\t{");
                    first = tmp_count = 0;
                }
                if (udi_global.propdata[index]->type == DEVICE) {
                    d_count++;
                    fprintf(fp, "\n\t\t{%ld, ",
                        udi_global.propdata[index+2]->ulong);
                    i = index+3;
                    found = 0;
                    while (udi_global.propdata[i] != NULL) {
                        i++;
                    }
                    i -= index+3;
                    i /= 3;
                    fprintf(fp, "%d, %d},", tmp_count, i);
                    a_count += i;
                    tmp_count += i;
                } else if (udi_global.propdata[index]->type ==
                        ENUMERATES) {
                    i = index+5;
                    while (udi_global.propdata[i] != NULL)
                        i++;
                    i -= index+5;
                    i /= 3;
                    tmp_count += i;
                }
                break;
            case 7: /* enumerates */
                if (first) {
                    if (d_count == 0) {
                        fprintf(fp, "\n\t\t{-1, -1, -1}");
                    }
                    else {
                        fseek(fp, -1, SEEK_CUR); /* delete last comma */
                    }
                    fprintf(fp, "\n\t},\n");
                    fprintf(fp, "\t/* enumerations */\n");
                    fprintf(fp, "\t{");
                    first = tmp_count = 0;
                }
                if (udi_global.propdata[index]->type == ENUMERATES) {
                    d_count++;
                    fprintf(fp, "\n\t\t{%ld, %ld, %ld, ",
                        udi_global.propdata[index+4]->ulong,
                        udi_global.propdata[index+2]->ulong,
                        udi_global.propdata[index+3]->ulong);
                    i = index+5;
                    found = 0;
                    while (udi_global.propdata[i] != NULL) {
                        i++;
                    }
                    i -= index+3;
                    i /= 3;
                    fprintf(fp, "%d, %d},", tmp_count, i);
                    a_count += i;
                } else if (udi_global.propdata[index]->type ==
                        DEVICE) {
                    i = index+3;
                    while (udi_global.propdata[i] != NULL)
                        i++;
                    i -= index+3;
                    i /= 3;
                    tmp_count += i;
                }
                break;
            case 8: /* attributes */
                if (first) {
                    if (e_count == 0) {
                        fprintf(fp, "\n\t\t{-1, 0, 0,"
                                " -1, -1}");
                    }
                    else {
                        fseek(fp, -1, SEEK_CUR); /* delete last comma */
                    }
                    fprintf(fp, "\n\t},\n");
                    a_count = d_count = e_count = 0;
                    fprintf(fp, "\t/* attributes */\n");
                    fprintf(fp, "\t{");
                    first = 0;
                }
                if (udi_global.propdata[index]->type == DEVICE ||
                        udi_global.propdata[index]->type ==
                        ENUMERATES) {
                    if (udi_global.propdata[index]->type == DEVICE) {
                        msgnum = udi_global.propdata[index+1]->
                                ulong;
                        metaidx = udi_global.propdata[index+2]->
                                ulong;
                        d_count++;
                        i = index+3;
                    } else {
                        msgnum = udi_global.propdata[index+1]->
                                ulong;
                        metaidx = udi_global.propdata[index+4]->
                                ulong;
                        d_count++;
                        i = index+5;
                    }
                    while (udi_global.propdata[i] != NULL) {
                        fprintf(fp, "\n\t\t{\"%s\", %d, ",
                            udi_global.propdata[i]->string,
                            udi_global.propdata[i+1]->type);
                        switch(udi_global.propdata[i+1]->type) {
                        case UDI_ATTR_STRING:
                                fprintf(fp, "{\"");
                                antiparse(fp,
                                  udi_global.propdata[i+2]->string);
                            fprintf(fp, "\"}}");
                            break;
                        case UDI_ATTR_UBIT32:
                            fprintf(fp, "{(char *)%s"
                                "}}", udi_global.propdata[i+2]
                                ->string);
                            break;
                        case UDI_ATTR_BOOLEAN:
                            fprintf(fp, "{\"%s\"}}",
                                udi_global.propdata[i+2]
                                ->string);
                            break;
                        case UDI_ATTR_ARRAY8:
                           ptr1 = udi_global.propdata[i+2]->string;
                           fprintf(fp, "{\"");
                           fprintf(fp, "\\x%02x", strlen(ptr1)/2);
                           while (*ptr1) {
                               fprintf(fp, "\\x%c", tolower(*ptr1));
                               ptr1++;
                               fprintf(fp, "%c", tolower(*ptr1));
                               ptr1++;
                           }
                           fprintf(fp, "\"}}");
                            break;
                        }
                        fprintf(fp, " /* type=");
                        if (udi_global.propdata[i+1]->type == UDI_ATTR_STRING)
                            fprintf(fp, "STRING");
                        else if (udi_global.propdata[i+1]->type == UDI_ATTR_UBIT32)
                            fprintf(fp, "UBIT32");
                        else if (udi_global.propdata[i+1]->type == UDI_ATTR_BOOLEAN)
                            fprintf(fp, "BOOLEAN");
                        else if (udi_global.propdata[i+1]->type == UDI_ATTR_ARRAY8)
                            fprintf(fp, "ARRAY8");
                        fprintf(fp, ", msgnum=%d, "
                            "metaidx=%d */,",
                            msgnum, metaidx);
                        i+=3;
                    }
                    i -= index+3;
                    i /= 3;
                    a_count += i;
                }
                break;
            case 9: /* readable_files */
                if (first) {
                    if (a_count == 0) {
                        fprintf(fp, "\n\t\t{\"\", 0, {\"\"}}");
                    }
                    else {
                        fseek(fp, -1, SEEK_CUR); /* delete last comma */
                    }
                    fprintf(fp, "\n\t},\n");
                    fprintf(fp, "\t/* readable_files */\n");
                    fprintf(fp, "\t{");
                    first = 0;
                }
                if (udi_global.propdata[index]->type == READFILE) {
                    rf_count++;
		    /* The file name */
                    fprintf(fp, "\n\t\t{\"%s\", ",
                        udi_global.propdata[index+1]->string);
		    /* The full path to the actual file itself */
                    fprintf(fp, "\"%srfiles/%s\", ",
                        udi_filedir,
                        udi_global.propdata[index+1]->string);
		    /* Include the file size */
		    getcwd(buf, 1024);
                    if (install_src) {
		    	strcat(buf, "/../extra/");
		    } else {
		    	strcat(buf, "/../../extra/");
		    }
		    strcat(buf, udi_global.propdata[index+1]->string);
		    if (stat (buf, &fst) != 0)
                    	printloc(stderr, UDI_MSG_FATAL, 1535, "Error: unable "
                                "to stat readable file %s!\n", buf);
		    fprintf(fp, "0x%lx, (unsigned char *)\n", fst.st_size);
		    /* Include the file itself */
		    fd = open(buf, O_RDONLY);
		    while ((nr = read(fd, buf, 10)) != 0) {
			int ii;
			fprintf(fp, "\t\t\t\"");
			for (ii = 0; ii < nr; ii++)
				fprintf(fp, "\\x%02x", buf[ii]);
			fprintf(fp, "\"");
				fprintf(fp, "\n");
		    }
		    fprintf(fp, "\t\t},\n");
                }
                break;
            case 10: /* locale */
                if (first) {
                    if (rf_count == 0) {
                        fprintf(fp, "\n\t\t{\"\", \"\",\n\t\t\t0, (unsigned char *)\"\"}");
                    }
                    else {
                        fseek(fp, -1, SEEK_CUR); /* delete last comma */
                    }
                    fprintf(fp, "\n\t},\n");
                    fprintf(fp, "\t/* locale */\n");
                    fprintf(fp, "\t{\n");
                    first = 0;
                    l_count = 1;
                    nmsgs = 0;
                    fprintf(fp, "\t\t{\"C\", 0, ");
                }
                if (udi_global.propdata[index]->type == LOCALE) {
                    fprintf(fp, " %d},\n", nmsgs);
                    nmsgs = 0;
                    l_count++;
                    fprintf(fp, "\t\t{\"%s\", %d,",
                        udi_global.propdata[index+1]->string,
                        ms_count);
                }
                if (udi_global.propdata[index]->type == MESSAGE ||
                    udi_global.propdata[index]->type == DISMESG) {
                    ms_count++;
                    nmsgs++;
                }
                break;
            case 11: /* msg_files */
                if (first) {
                    fprintf(fp, " %d}\n", nmsgs);
                    fprintf(fp, "\t},\n");
                    fprintf(fp, "\t/* msg_files */\n");
                    fprintf(fp, "\t{");
                    strcpy(cur_loc, "C");
                    first = 0;
                }
                if (udi_global.propdata[index]->type == MESGFILE) {
                    /*
                     * Read in, and process the message files.
                     */
                    FILE *mfp;
                    int msgf = 0;
                    if (install_src) {
                        sprintf(buf, "../msg/%s",
                            udi_global.propdata[index+1]->
                            string);
                    } else {
                        sprintf(buf, "../../msg/%s",
                            udi_global.propdata[index+1]->
                            string);
                    }
                    if ((mfp = fopen(buf, "r")) == NULL) {
                        printloc(stderr, UDI_MSG_FATAL, 1515, "Error: unable "
                            "to open message file "
                            "%s for reading!\n",
                            buf);
                    }
                    while(fgets(buf, 1023, mfp) != NULL) {
                        if (strncmp("locale ", buf, 7)
                                == 0) {
                            if (mf_count == 0 &&
                                    msgf != 0) {
                                mf_count++;
                                fprintf(fp, "\t"
                                    "\"%s\", ",
                                    cur_loc);
                            }
                            strcpy(cur_loc,
                                &buf[7]);
                            if (cur_loc[strlen(
                                cur_loc)-1] ==
                                '\n')
                                cur_loc[strlen(
                                cur_loc)-1] =
                                '\0';

                            mf_count++;
                            fprintf(fp, "\n\t\t{\"%s\", "
                                , cur_loc);
                            fprintf(fp, "\"%smsg/"
                                "%s\"},",
                                udi_filedir,
                                udi_global.propdata
                                [index+1]->
                                string);
                        } else if (strncmp("message ",
                                buf, 8) == 0 ||
                                strncmp("disast"
                                "er_message ",
                                buf,17) == 0) {
                            msgf++;
                        }
                    }
                    if (mf_count == 0 && msgf != 0) {
                        mf_count++;
                        fprintf(fp, "\n\t\t{\"%s\", ",
                            cur_loc);
                        fprintf(fp, "\"%smsg/%s\"},",
                            udi_filedir,
                            udi_global.propdata[index+1]->
                            string);
                    }
                    fclose(mfp);
                }
                break;
            case 12: /* messages */
                if (first) {
                    if (mf_count == 0) {
                        fprintf(fp, "\n\t\t{\"\", \"\"}");
                    }
                    else {
                        fseek(fp, -1, SEEK_CUR); /* delete last comma */
                    }
                    fprintf(fp, "\n\t},\n");
                    fprintf(fp, "\t/* %d messages */\n",
                        nmsgs);
                    fprintf(fp, "\t{");
                    first = ms_count = 0;
                    l_count = 1;
                    strcpy(locale, "C");
                }
                if (udi_global.propdata[index]->type == LOCALE) {
                    l_count++;
                    strcpy(locale, udi_global.propdata[index+1]->string);
                }
                if (udi_global.propdata[index]->type == MESSAGE ||
                    udi_global.propdata[index]->type == DISMESG) {
                        fprintf(fp, "\n\t\t{%ld, \"",
                        udi_global.propdata[index+1]->ulong);
                    antiparse(fp,
                          udi_global.propdata[index+2]->string);
                    fprintf(fp, "\"}, /* %s */",
                        locale);
                    ms_count++;
                }
                break;
            }
            while(udi_global.propdata[index] != NULL) index++;
            index++;
        }
    }
    fprintf(fp, "\n\t\t{-1, 0}\n\t}\n};\n");
    ms_count++;

    /* Create the udi_???_sprops_idx structure */
    fprintf(fp, "\n_udi_sprops_idx_t udi_%s_sprops_idx[] "
            "= {\n", name);
    fprintf(fp, "\t{0, &udi_%s_sprops.shortname},\n", name);
    fprintf(fp, "\t{0, &udi_%s_sprops.version},\n", name);
    fprintf(fp, "\t{0, &udi_%s_sprops.release},\n", name);
    fprintf(fp, "\t{0, &udi_%s_sprops.pioserlim},\n", name);
    fprintf(fp, "\t{%d, &udi_%s_sprops.metas},\n", mt_count, name);
    fprintf(fp, "\t{%d, &udi_%s_sprops.child_bindops},\n", cb_count, name);
    fprintf(fp, "\t{%d, &udi_%s_sprops.parent_bindops},\n", pb_count, name);
    fprintf(fp, "\t{%d, &udi_%s_sprops.intern_bindops},\n", ib_count, name);
    fprintf(fp, "\t{%d, &udi_%s_sprops.provides},\n", pv_count, name);
    fprintf(fp, "\t{%d, &udi_%s_sprops.regions},\n", r_count, name);
    fprintf(fp, "\t{%d, &udi_%s_sprops.devices},\n", d_count, name);
    fprintf(fp, "\t{%d, &udi_%s_sprops.enumerates},\n", e_count, name);
    fprintf(fp, "\t{%d, &udi_%s_sprops.attribs},\n", a_count, name);
    fprintf(fp, "\t{%d, &udi_%s_sprops.read_files},\n", rf_count, name);
    fprintf(fp, "\t{%d, &udi_%s_sprops.locale},\n", l_count, name);
    fprintf(fp, "\t{%d, &udi_%s_sprops.msg_files},\n", mf_count, name);
    fprintf(fp, "\t{%d, &udi_%s_sprops.messages}\n", 0, name);
    fprintf(fp, "};\n\n");

    fprintf(fp, "static void * sprops = &udi_%s_sprops_idx;\n\n", name);
}

#endif /* OS_GLUE_PROPS_STRUCT */
