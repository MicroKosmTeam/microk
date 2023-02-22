/*
 * File: tools/solaris/osdep_make.c
 *
 * Solaris-specific target file installation.
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
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "y.tab.h"
#include "spparse.h"
#include "osdep.h"
#include <udi_sprops_ra.h>
#include "common_api.h"			/* Common utility functions */

#define	UDI_VERSION 0x101
#include <udi.h>

void make_glue (char *tmpf, int install_src, char *posix, char *depend);
char *pkg;
int mt_count, cb_count, pb_count, ib_count, pv_count, rf_count,
			l_count, mf_count, ms_count, r_count, d_count,
			e_count, a_count;
int udi_version;
int desc_index = -1;
char *desc;
char typedesc[80];
char name[513];
int version;
char buf2[1024];
char modtype[256];
int pioserlim;
int made_conf = 0;

#define	NUM_FIND	13	/* Number of properties to scan */

#define input_data udi_global.propdata
#define Dflg	   udi_global.debug
#define Oflg	   udi_global.optimize
#define	sflg	   udi_global.stop_install

/* Custom parameter offsets for 'device' lines in udiprops.txt */
typedef enum {
	attr_name_off = 1,
	scope_off = 2,
	msgnum1_off = 3,
	msgnum2_off = 4,
	msgnum3_off = 5,
	choices_off = 6,
} custom_offsets_t;

/*
 * make_driver(fp, init_info, name, full_name)
 * -----------
 * Create a udi_glue.c for the given driver
 *	fp 		- File reference to udi_glue.c
 *	init_info	- name of udi_init_info for driver
 *	name		- driver name
 *	full_name	- full description of driver
 */
static void
make_driver(FILE *fp, char *init_info, char *name, char *full_name)
{
	/* Driver declarations */
	fprintf(fp, "\n#include <sys/conf.h>\n"
		    "#include <sys/modctl.h>\n"
		    "#include <sys/ddi.h>\n"
		    "#include <sys/sunddi.h>\n\n"
		    "typedef struct soldrv_state {\n"
		    "dev_info_t\t*dip;\t/* Set by xxattach */\n"
		    "void\t*sprops;\t/* Static properties */\n"
		    "void\t*init;\t/* udi_init_info reference */\n"
		    "} soldrv_state_t;\n\n"
		    "static void *soldrv_state;\n\n"
		    "/* device ops for driver */\n"
		    "extern int\t_udi_drv_attach(dev_info_t *, "
		    "ddi_attach_cmd_t); \n"
		    "extern int\t_udi_drv_detach(dev_info_t *, "
		    "ddi_detach_cmd_t);\n"
		    "extern int\t_udi_driver_load(const char *, void *, "
		    "void *); \n"
		    "extern int\t_udi_driver_unload(const char *); \n\n"
		    "extern int\t%s;\n", init_info);
	/* Device Ops vectors */
	fprintf(fp, "\nstatic int\t%s_getinfo(dev_info_t *, ddi_info_cmd_t, "
		    "void *, void **);\n"
		    "int %s_open(dev_t *, int, int, cred_t *);\n"
		    "/* Character/Block device ops */\n"
		    "static struct cb_ops %s_cb_ops = {\n"
		    "\t%s_open,\t\t\t/* open		*/\n"
		    "\tnodev,\t\t\t/* close		*/\n"
		    "\tnodev,\t\t\t/* strategy		*/\n"
		    "\tnodev,\t\t\t/* print		*/\n"
		    "\tnodev,\t\t\t/* dump		*/\n"
		    "\tnodev,\t\t\t/* read		*/\n"
		    "\tnodev,\t\t\t/* write		*/\n"
		    "\tnodev,\t\t\t/* ioctl		*/\n"
		    "\tnodev,\t\t\t/* devmap		*/\n"
		    "\tnodev,\t\t\t/* mmap		*/\n"
		    "\tnodev,\t\t\t/* segmap		*/\n"
		    "\tnochpoll,\t\t\t/* chpoll		*/\n"
		    "\tnodev,\t\t\t/* prop_op		*/\n"
		    "\tNULL,\t\t\t/* streamtab		*/\n"
		    "\tD_MP | D_64BIT,\t\t/* cb_flag		*/\n"
		    "\tCB_REV,\t\t\t/* cb_rev		*/\n"
		    "\tnodev,\t\t\t/* cb_aread		*/\n"
		    "\tnodev\t\t\t/* cb_awrite		*/\n};\n"
		    "/* Device ops vector */\n"
		    "static struct dev_ops %s_dev_ops = {\n"
		    "\tDEVO_REV,\t\t/* devo_rev		*/\n"
		    "\t0,\t\t/* devo_refcnt		*/\n"
		    "\tnodev,\t\t/* devo_getinfo		*/\n"
		    "\t%s_getinfo,\t/* devo_identify	*/\n"
		    "\tnulldev,\t/* devo_probe		*/\n"
		    "\t_udi_drv_attach,\t/* devo_attach		*/\n"
		    "\t_udi_drv_detach,\t/* devo_detach		*/\n"
		    "\tnodev,\t\t/* devo_reset		*/\n"
		    "\t&%s_cb_ops,\t\t/* devo_cb_ops		*/\n"
		    "\tNULL,\t\t/* devo_bus_ops		*/\n"
		    "\tnodev\t\t/* devo_power		*/\n};\n\n"
		    "/* modlinkage for driver */\n"
		    "static struct modldrv %s_modldrv = {\n"
		    "\t&mod_driverops,\n"
		    "\t\"UDI: %s\",\n"
		    "\t&%s_dev_ops \n};\n", 
		    name, name, name, name, name, name, name, name, 
		    full_name, name);
	/* Create modlinkage */
	fprintf(fp, "\nstatic struct modlinkage %smodlinkage = {\n"
		    "\tMODREV_1,\n"
		    "\t&%s_modldrv,\n"
		    "\tNULL\n};", name, name);
	/* Create driver */
	fprintf(fp, "\nint\n_init(void)\n"
		    "{\n"
		    "\tint error;\n"
		    "\tddi_soft_state_init(&soldrv_state, "
		    "sizeof(soldrv_state_t), 0);\n"
		    "\tif (ddi_soft_state_zalloc(soldrv_state,0) != "
		    "DDI_SUCCESS) {\n"
		    "\t\tddi_soft_state_fini(&soldrv_state);\n"
		    "\t\treturn DDI_FAILURE;\n"
		    "\t} else {\n"
		    "\t\t/* Create sprops link in _first_ state field */\n"
		    "\t\t((soldrv_state_t *)ddi_get_soft_state(soldrv_state,"
		    "0))->sprops = \n\t\t\tsprops; \n"
		    "\t\t((soldrv_state_t *)ddi_get_soft_state(soldrv_state,"
		    "0))->init = \n\t\t\t(void *)&udi_init_info;\n\t}\n"
		    "\terror = mod_install(&%smodlinkage);\n"
		    "\tif (error != 0) {\n"
		    "\t\t/* Cleanup after failure */\n"
		    "\t\tddi_soft_state_free(soldrv_state, 0);\n"
		    "\t\tddi_soft_state_fini(&soldrv_state);\n"
		    "\t} else {\n"
		    "\t\t/* Load the driver into the UDI namespace */\n"
		    "\t\terror = _udi_driver_load(\"%s\" , &%s, sprops);\n"
		    "\t}\n"
		    "\treturn (error);\n}\n"
		    "int\n"
		    "_info(struct modinfo *mip)\n"
		    "{\n"
		    "\treturn mod_info(&%smodlinkage, mip);\n"
		    "}\n\n"
		    "int\n"
		    "_fini(void)\n"
		    "{\n"
		    "\tint	error;\n"
		    "\terror = mod_remove(&%smodlinkage);\n"
		    "\tif (error == 0) {\n"
		    "\t\tddi_soft_state_fini(&soldrv_state);\n"
		    "\t\terror = _udi_driver_unload(\"%s\");\n"
		    "\t}\n"
		    "\treturn (error);\n"
		    "}\n\n"
		    "/*\n"
 		    " * xx_getinfo:\n"
 		    " * ----------\n"
 		    " * Return information for the specified dev_info_t "
		    "instance\n"
 		    " */\n"
		    "static int\n"
		    "%s_getinfo(dev_info_t *dip, ddi_info_cmd_t cmd, "
		    "void *arg, void**result) \n{\n"
		    "\tint	error = 0;\n"
		    "\tcmn_err(CE_WARN, \"%s_getinfo called\");\n"
		    "\tswitch(cmd) {\n"
		    "\tcase DDI_INFO_DEVT2DEVINFO:\n"
		    "\tcase DDI_INFO_DEVT2INSTANCE:\n"
		    "\tdefault:\n"
		    "\t\terror = DDI_FAILURE;\n"
		    "\t}\n"
		    "\treturn (error);\n}\n"
		    "\n/*\n"
		    " * open stub -- TESTING __ONLY__\n"
		    " */\n"
		    "int\n%s_open( dev_t *devp, int flag, int otyp, cred_t "
		    "*credp)\n"
		    "{\n"
		    "\tcmn_err(CE_WARN, \"%s_open Unexpected CALL\");\n"
		    "\treturn ENXIO;\n}\n",
		    name, name, init_info, name, name, name, name, name, name, 
		    name );
}

/*
 * make_meta(fp, init_info, name, full_name)
 * ---------
 * Create a udi_glue.c for the given meta
 *	fp 		- File reference to udi_glue.c
 *	init_info	- name of udi_mei_init_t for meta
 *	name		- meta name
 *	full_name	- full description of meta
 */
static void
make_meta(FILE *fp, char *init_info, char *name, char *full_name)
{
	/* Create declarations for metalanguage */
	fprintf(fp, "\n#include <sys/conf.h>\n"
	"#include <sys/modctl.h>\n"
	"#include <sys/ddi.h>\n"
	"#include <sys/sunddi.h>\n\n"
	"typedef struct soldrv_state {\n"
	"\tdev_info_t *dip;\n"
	"\tvoid	*sprops;\n"
	"\tvoid *init;\n"
	"} soldrv_state_t;\n\n"
	"static void *soldrv_state;\n\n"
	"/* device ops for meta */\n"
	"static int\n"
	"_udi_meta_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)\n{\n"
	"\treturn DDI_SUCCESS;\n}\n"
	"static int\n"
	"_udi_meta_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)\n{\n"
	"\treturn DDI_SUCCESS;\n}\n"
	"extern int	_udi_meta_load(const char *, void *, void *);\n"
	"extern int	_udi_meta_unload(const char *);\n\n"
	"extern int %s;\n", init_info);

	/* Device ops vector */
	fprintf(fp, "\n"
	"static int\t%s_getinfo(dev_info_t *, ddi_info_cmd_t, void *, "
	"void **);\n"
	"/* Character/Block device ops */\n"
	"static struct cb_ops %s_cb_ops = {\n"
	"\tnodev,			/* open			*/\n"
	"\tnodev,			/* close		*/\n"
	"\tnodev,			/* strategy		*/\n"
	"\tnodev,			/* print		*/\n"
	"\tnodev,			/* dump			*/\n"
	"\tnodev,			/* read			*/\n"
	"\tnodev,			/* write		*/\n"
	"\tnodev,			/* ioctl		*/\n"
	"\tnodev,			/* devmap		*/\n"
	"\tnodev,			/* mmap			*/\n"
	"\tnodev,			/* segmap		*/\n"
	"\tnochpoll,			/* chpoll		*/\n"
	"\tnodev,			/* prop_op		*/\n"
	"\tNULL,			/* streamtab		*/\n"
	"\tD_MP | D_64BIT,		/* cb_flag		*/\n"
	"\tCB_REV,			/* cb_rev		*/\n"
	"\tnodev,			/* cb_aread		*/\n"
	"\tnodev			/* cb_awrite		*/\n};\n"
	"/* Device ops vector */\n"
	"static struct dev_ops %s_dev_ops = {\n"
	"\tDEVO_REV,		/* devo_rev		*/\n"
	"\t0,			/* devo_refcnt		*/\n"
	"\t%s_getinfo,	/* devo_identify	*/\n"
	"\tnulldev,		/* devo_identify	*/\n"
	"\tnulldev,		/* devo_probe		*/\n"
	"\t_udi_meta_attach,	/* devo_attach		*/\n"
	"\t_udi_meta_detach,	/* devo_detach		*/\n"
	"\tnodev,			/* devo_reset		*/\n"
	"\tNULL,			/* devo_cb_ops		*/\n"
	"\tNULL,			/* devo_bus_ops		*/\n"
	"\tnodev			/* devo_power		*/\n};\n"
	"/* modlinkage for driver */\n"
	"static struct modldrv %s_modldrv = { \n"
	"\t&mod_driverops, \n"
	"\t\"UDI: %s\", \n"
	"\t&%s_dev_ops\n};\n", name, name, name, name, name, full_name, name);
	/* Create modlinkage */
	fprintf(fp, "\nstatic struct modlinkage %smodlinkage = {\n"
		    "\tMODREV_1,\n"
		    "\t&%s_modldrv,\n"
		    "\tNULL\n};", name, name);
	/* Create meta driver */
	fprintf(fp, "\n"
	"int\n"
	"_init(void)\n"
	"{\n"
	"\tint error;\n"
	"\terror = mod_install(&%smodlinkage);\n"
	"\tif (error == 0) {\n"
	"\t\t/* Load the driver into the UDI namespace */\n"
	"\t\terror = _udi_meta_load(\"%s\" , (void *)&%s, sprops);\n"
	"\t}\n"
	"\treturn (error); \n"
	"}\n\n"
	"int\n"
	"_info(struct modinfo *mip)\n"
	"{\n"
	"\treturn mod_info(&%smodlinkage, mip);\n"
	"}\n\n"
	"int\n"
	"_fini(void)\n"
	"{\n"
	"\tint	error;\n"
	"\terror = mod_remove(&%smodlinkage);\n"
	"\tif (error == 0) {\n"
	"\t\terror = _udi_meta_unload(\"%s\");\n"
	"\t}\n"
	"\treturn (error);\n"
	"}\n\n"
	"/*\n"
 	"* xx_getinfo:\n"
 	"* ----------\n"
 	"* Return information for the specified dev_info_t instance\n"
 	"*/\n"
	"static int\n"
	"%s_getinfo(dev_info_t *dip, ddi_info_cmd_t cmd, void *arg, "
	"void**result)\n"
	"{\n"
	"\tint	error = 0;\n\n"
	"\tswitch(cmd) {\n"
	"\t\tcase DDI_INFO_DEVT2DEVINFO:\n"
	"\t\tcase DDI_INFO_DEVT2INSTANCE:\n"
	"\t\tdefault:\n"
	"\t\t\terror = DDI_FAILURE;\n"
	"\t}\n"
	"\treturn (error);\n}\n" , 
	name, name, init_info, name, name, name, name );
}

/*
 * make_conf_ubit32:
 * ----------------
 */
static void
make_conf_ubit32(FILE *fp, int index, int custom_num)
{
	fprintf(fp, "%s=0x%x;\n", input_data[index+attr_name_off]->string+1,
		input_data[index+7]->ulong);
	fprintf(stderr, "UBIT32 <%s> Value <%d>\n",
		input_data[index+attr_name_off]->string+1,
		input_data[index+7]->ulong);
}

/*
 * make_conf_boolean:
 * -----------------
 */
static void
make_conf_boolean(FILE *fp, int index, int custom_num)
{
	int	val;

	if (!strcmp(input_data[index+7]->string, "F")) {
		val = 0;
	} else {
		val = 1;
	}
	
	fprintf(stderr, "Boolean <%s> Value <%s>\n",
			input_data[index+attr_name_off]->string+1,
			val ? "True" : "False");
	fprintf(fp, "%s=%d;\n", input_data[index+attr_name_off]->string+1, val);
}

/*
 * make_conf_string:
 * ----------------
 */
static void
make_conf_string(FILE *fp, int index, int custom_num)
{
	char	*sval;

	sval = optinit();
	get_message(&sval, input_data[index+7]->ulong, NULL);
	fprintf(stderr, "String <%s> Value <%s>\n",
		input_data[index+attr_name_off]->string+1,
		sval);
	fprintf(fp, "%s=\"%s\";\n", input_data[index+attr_name_off]->string+1,
		sval);
}

/*
 * make_conf:
 * ---------
 * Create the driver.conf file which contains this nodes' custom parameters.
 * These can be queried and modified by udicfg [in a future version]
 */
static void
make_conf(char *name, FILE *fp)
{
	int	index;
	int	num_cust;
	int	type;

	num_cust = 0;
	index = get_prop_type(CUSTOM, 0);
	while (index >= 0) {
		type = 0;
		if (input_data[index+attr_name_off]->string[0] == '%') {
			char	*stype = input_data[index+choices_off]->string;
			num_cust++;
			if (!strcmp(stype, "ubit32")) {
				type = 1;
			} else if (!strcmp(stype, "boolean")) {
				type = 2;
			} else if (!strcmp(stype, "string")) {
				type = 3;
			} else if (!strcmp(stype, "array")) {
				type = 4;
			}
			switch (type) {
			case 1:	/* ubit32 */
				make_conf_ubit32(fp, index, num_cust);
				break;
			case 2: /* boolean */
				make_conf_boolean(fp, index, num_cust);
				break;
			case 3:	/* string */
				make_conf_string(fp, index, num_cust);
				break;
			case 4:	/* array */
			default:/* error */
				printloc(stderr, UDI_MSG_FATAL, 2102,
				        "Unsupported custom device type '%s'\n",
					stype);
			}
		}
		index = get_prop_type(CUSTOM, index+1);
	}

	if (num_cust > 0) {
		made_conf = 1;	/* We have a .conf file to associate */
		fprintf(stderr, "%d parameters to store\n", num_cust);
	}
}

#if 0
/*
 * make_mapper_depend:
 * ------------------
 * Generate the list of dependencies that this driver requires. These need to
 * appear in the _depends_on[] array (in udi_glue.c)
 */
static void
make_mapper_depend(char *buf, char *name, char *meta)
{
	char	*ctmp = "";

	if (strcmp(meta, "udi_bridge") == 0 &&
			strcmp(name, "udiMbrdg") != 0)
		ctmp = "udiMbrdg";
	if (strcmp(meta, "udi_gio") == 0 &&
			strcmp(name, "udiMgio") != 0 &&
			strcmp(name, "tstMgio") != 0 &&
			strcmp(name, "tstIgio") != 0)
		ctmp = "udiMgio";
	if (strcmp(meta, "udi_gio") == 0 &&
			strcmp(name, "tstIgio") == 0)
		ctmp = "tstMgio";
	if (strcmp(meta, "udi_scsi") == 0 &&
			strcmp(name, "udiMscsi") != 0 &&
			strcmp(name, "tstIgio") != 0)
		ctmp = "udiMscsi";
	if (strcmp(meta, "udi_nic") == 0 &&
			strcmp(name, "udiMnic") != 0 &&
			strcmp(name, "tstIgio") != 0)
		ctmp = "udiMnic";
	if (*meta == '%') {
		ctmp = meta+1;
	}

	/* Generate -N <dependency> for linker */
	strcpy(buf, "misc/");
	strcat(buf, ctmp);
}

/*
 * make_meta_depend:
 * ----------------
 * Generate meta dependencies for given name
 */
static void
make_meta_depend(char *buf, char *name, char *meta)
{
	char *ctmp = "";

	if (strcmp(name, meta) != 0) {
		if (!strcmp(meta, "udi_bridge")) {
			ctmp = "udi_brdg";
		} else {
			ctmp = meta;
		}
		strcpy(buf, "misc/");
		strcat(buf, ctmp);
	} else {
		strcpy(buf, "");
	}
}
#endif

/*
 * strcompare(v1, v2)
 * ----------
 * Compare the strings referenced by v1, v2 and return a comparison value:
 *
 * Returns:
 *	< 0 	if v1 lexicographically < v2
 *	== 0 	if v1 lexicographically == v2
 *	> 0	if v1 lexicographically > v2
 */
static int
strcompare(const void *v1, const void *v2)
{
	return strcmp((const char *)v1, (const char *)v2);
}

/*
 * make_alias(name, tmpdir)
 * ----------
 * Create a list of driver_aliases entries for the given driver <name>. These
 * will be stored in <tmpdir/name.alias> for subsequent editing into 
 * /etc/driver_aliases
 */
static void
make_alias(char *name, char *tmpdir)
{
	FILE	*fp, *uniqfp;
	int	fd, uniqfd;
	int	index, ii, jj, num_entries = 0, new_entry;
	char	*entry, *tmp, *p, *end, *aliasname, *uniqname;
	size_t	entry_len = 0;
	void	*pa;
	struct stat	stbuf;
	char	cwd[1025];

	entry = optinit();
	tmp   = optinit();
	aliasname = optinit();
	uniqname = optinit();

	optcpy(&aliasname, tmpdir);
	optcat(&aliasname, "/");
	optcat(&aliasname, name);
	optcat(&aliasname, ".alias");

	if ((fd = open(aliasname, O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0) {
		printloc(stderr, UDI_MSG_FATAL, 2103, "unable to open "
		 	"%s for writing!\n", aliasname);
	}
	/* Obtain a FILE reference to fd */
	if ((fp = fdopen(fd, "w+")) == NULL) {
		printloc(stderr, UDI_MSG_FATAL, 2104, "unable to open "
			 	"%s.alias for writing!\n", name);
	}
	index = get_prop_type(DEVICE, 0);
	while (index > 0) {
		propline_t	**pp;
		pp = udi_global.propdata + index;

		for (jj = 3; pp[jj]; jj++) {
			if (strcmp(pp[jj]->string, "bus_type") == 0 &&
			    strcmp(pp[jj+2]->string, "pci") == 0) {
				optcpy(&entry, name);
				optcat(&entry, " \"pci");
				for (ii = 3; pp[ii]; ii++) {
					if (!strcmp(pp[ii]->string,
						    "pci_vendor_id")) {
						sprintf(tmp, "%04lx",
							pp[ii+2]->ulong);
						optcat(&entry, tmp);
					}
				}
				for (ii = 3; pp[ii]; ii++) {
					if (!strcmp(pp[ii]->string,
						    "pci_device_id")) {
						sprintf(tmp, ",%04lx\"",
							pp[ii+2]->ulong);
						optcat(&entry, tmp);
					}
				}
				break;
			}
		}
		fprintf(fp, "%s\n", entry);
		entry_len = strlen(entry) + 1;
		num_entries++;
		index = get_prop_type(DEVICE, index+1);
	}

	fflush(fp);

	/*
	 * Don't generate an alias file if we have no devices
	 */
	if (num_entries == 0) {
		fclose(fp);
		goto bail;
	}

	/*
	 * We've now written out [entry_len] * [num_entries] bytes to fp.
	 * We must now sort this and remove duplicate entries. Use qsort() to
	 * do the grunt work once we've mmap'd the file
	 */
	if (fstat(fd, &stbuf) < 0) {
		printloc(stderr, UDI_MSG_FATAL, 2105, "unable to access "
			 	"%s.alias for fstat()!\nError: %d\n", name,
				errno);
	}
	pa = mmap(0, stbuf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (pa == (void *)-1) {
		printloc(stderr, UDI_MSG_FATAL, 2106, "unable to memory map "
				"%s.alias!\n", name);
	}

	/*
	 * OK, we now have the alias file memory-mapped, so we can simply use
	 * qsort to sort the entries into increasing order. We know that all
	 * entries are of the same length as the pci_vendor_id and pci_device_id
	 * are mandatory device attributes and we've taken steps to forcibly
	 * format these as 4 digit quantities. Each entry will look like:
	 *   <driver> "pciVVVV,DDDD"
	 * where VVVV is the pci_vendor_id value [hex], and DDDD is the
	 * pci_device_id value [hex].
	 *
	 * NOTE: We'll need to be more creative when supporting other bus types
	 */

	for (p = (char *)pa, ii=0; ii < num_entries; ii++, p+=entry_len) {
		p[entry_len-1] = '\0';
	}

	qsort(pa, num_entries, entry_len, strcompare);

	/*
	 * Now we 'uniq' the file contents into 'name.uniq'
	 */
	optcpy(&uniqname, tmpdir);
	optcat(&uniqname, "/");
	optcat(&uniqname, name);
	optcat(&uniqname, ".uniq");
	if ((uniqfd = open(uniqname, O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0) {
		printloc(stderr, UDI_MSG_FATAL, 2103, "unable to open "
			 	"%s for writing!\n", uniqname);
	}
	if ((uniqfp = fdopen(uniqfd, "w+")) == NULL) {
		printloc(stderr, UDI_MSG_FATAL, 2103, "unable to open "
			 	"%s for writing!\n", tmp);
	}
	optcpy(&entry, "");
	end = (char *)pa + stbuf.st_size;
	new_entry = 0;
	for (p = (char *)pa; p < end; p += entry_len) {
		optncpy(&tmp, p, entry_len-1);
		if (strncmp(entry, tmp, entry_len-1)) {
			/* 
			 * new entry different from existing one, save old one
			 * if its non-NULL
			 */
			if (strlen(entry) > 0) {
				fprintf(stderr, "[%s]\n", entry);
				fprintf(uniqfp, "%s\n", entry);
			}
			optncpy(&entry, tmp, entry_len-1);
			new_entry = 1;
		}
	}
	/* exhausted entries, if new_entry is set we have to output <entry> */
	if (new_entry) {
		fprintf(stderr, "[%s]\n", entry);
		fprintf(uniqfp, "%s\n", entry);
	}
	fflush(uniqfp);

	/*
	 * Now unmap the alias file and copy the new unique file over it
	 */
	munmap(pa, stbuf.st_size);
	close(fd);
	close(uniqfd);

	os_copy(aliasname, uniqname);
	
bail:
	free(entry);
	free(tmp);
	free(aliasname);
	free(uniqname);
}

void
osdep_mkinst_files(int install_src, propline_t **inp_data, char *basedir,
		   char *tmpdir)
{
	char	tmpf[513];
	char	glue_obj[513], drvfile[513], objfile[513], depend[513]; 
	char	tmpbuf[513], mapfile[513], metadepend[513];
	/* Are we doing this in the Posix environment? */
	char 	*posix = getenv("DO_POSIX");
	char	*spfname, *tstr;
	char	*wherewasi = getcwd(NULL, 513);
	int	i, ii, index, index2, numinp;
	char	*argv[15];
	FILE	*fp;
	int	alias_fd;
	char	*prov_name;
	ulong	prov_ver;
	int	prov_sym = 0;
	int	depends = 0;
	char	*drvdir;
	int	udi_depend = 0;
	struct utsname	uts;
	extern int parsemfile;

	drvdir = optinit();
	optcpy(&drvdir, "/kernel/misc/");

	/*
	 * If we're running on a sun4u, we need to put drivers into
	 * /kernel/misc/sparcv9
	 */
	if (uname(&uts) < 0) {
		printloc(stderr, UDI_MSG_FATAL, 2107, 
				"Unable to determine machine type\n");
	}

	if (!strcmp(uts.machine, "sun4u")) {
		optcat(&drvdir, "sparcv9/");
	}

	chdir(basedir);
	/*
	 * Parse and bring in any message files
	 */
	spfname = optinit();
	input_num--;	/* Remove the last terminator */
	numinp = input_num;
	i = 0;
	while (i < numinp) {
		parsemfile = 1;
		if (inp_data[i]->type == MESGFILE) {
			entries[0].type = MSGFSTART;
			entries[0].string = "message_file_start";
			entries[1].type = STRING;
			entries[1].string = strdup(inp_data[i+1]->string);
			entindex = 2;
			saveinput();
			inp_data = udi_global.propdata;

			fclose(yyin);
			if (install_src)
				optcpy(&spfname, "../msg/");
			else
				optcpy(&spfname, "../../msg/");
			optcat(&spfname, inp_data[i+1]->string);
			yyin = fopen(spfname, "r");
			if (yyin == NULL) {
				printloc(stderr, UDI_MSG_FATAL, 1202, "Cannot "
						"open %s!\n", spfname);
			}
			optcpy(&propfile, spfname);
			prop_seen = 0 ;
			yyparse();
			inp_data = udi_global.propdata;
		}
		while (inp_data[i] != NULL) i++;
		i++;
	}

	free(spfname);
	entindex = parsemfile = 0;
	saveinput();
	inp_data = udi_global.propdata;

	saveinput();
	inp_data = udi_global.propdata;
	
	/* Loop through the messages, and completely resolve them */
	i = 0;
	tstr = optinit();
	while (inp_data[i] != NULL) {
		if (inp_data[i]->type == MESSAGE ||
				inp_data[i]->type == DISMESG) {
			tstr[0] = 0;
			parse_message (&tstr, inp_data[i+2]->string, "");
			if (strcmp(tstr, inp_data[i+2]->string) != 0) {
				free(inp_data[i+2]->string);
				inp_data[i+2]->string = strdup(tstr);
			}
		}
		while (inp_data[i] != NULL) i++;
		i++;
	}
	/*
	 * Create the udi_glue.c and udi_glue.o stubs
	 */
	strcpy(tmpf, tmpdir);
	strcat(tmpf, "/udi_glue.c");

	ii = get_prop_type(SHORTNAME, 0);
	if (ii > -1) {
		strcpy(name, udi_global.propdata[ii+1]->string);
	}

	/*
	 * Generate the dependency list for the driver
	 */
	depend[0]='\0';

	/*
	 * Scan the REQUIRES entries to determine which metas are needed.
	 * Special-case udi_mei as this is provided by the 'udi' driver
	 */
	index = get_prop_type(REQUIRES,0);
	metadepend[0]='\0';
	while (index >= 0) {
		if (*(inp_data[index+1]->string) != '%') {
			if (strcmp(name, "udi")) {
				if (!strcmp(inp_data[index+1]->string,
				    "udi_mei")) {
					if (!udi_depend) {
				    		strcat(metadepend, "misc/udi ");
						udi_depend = 1;
					}
				} else if (!strcmp(inp_data[index+1]->string,
					   "udi_physio")) {
					if (!udi_depend) {
						strcat(metadepend, "misc/udi ");
						udi_depend = 1;
					}
				} else {
					strcat(metadepend, "misc/");
					if (!strcmp("udi_bridge",
						    inp_data[index+1]->
						    string)) {
						strcat(metadepend,
						       "udi_brdg");
					} else {
						strcat(metadepend,
					       	     inp_data[index+1]->
					             string);
						if (!strcmp(inp_data[index+1]->
							    string, "udi")){
							udi_depend=1;
						}
					}
					strcat(metadepend, " ");
				}
			}
		}
		index = get_prop_type(REQUIRES, index+1);
	}
	if (strlen(metadepend) > 0) {
		strcpy(depend, "\"");
		strcat(depend, metadepend);
		depends=1;
	}

	if (depends) {
		depend[strlen(depend)-1] = '\"';
		make_glue(tmpf, install_src, posix, depend);
	} else {
		make_glue(tmpf, install_src, posix, "");
	}

	/*
 	 * Create the <driver>.conf file for the custom parameters
	 */
	strcpy(tmpf, tmpdir);
	strcat(tmpf, "/");
	strcat(tmpf, name);
	strcat(tmpf, ".conf");

	if ((fp = fopen(tmpf, "w+")) == NULL) {
		printloc(stderr, UDI_MSG_FATAL, 2103, "unable to open "
			 "%s for writing!\n", tmpf);
	}
	make_conf(name, fp);
	fclose(fp);

	/*
	 * Copy .conf file into driver directory for add_drv to make use of
	 */
	if (made_conf && !sflg) {
		os_copy(drvdir, tmpf);
	}

	/*
	 * Create the driver alias file if needed
	 */
	if (udi_global.make_mapper == 0 && !pv_count) {
		strcpy(tmpf, tmpdir);
		strcat(tmpf, "/");
		strcat(tmpf, name);
		strcat(tmpf, ".alias");
		make_alias(name, tmpdir);

		if (!sflg) {
			os_copy(drvdir, tmpf);
		}
	}
		
	/*
	 * For binary distributions, copy the module into the tmp dir
	 */
	if (!install_src) {
		os_copy(tmpdir, inp_data[module_ent+1]->string);
	}
	
	strcpy(objfile, tmpdir);
	strcat(objfile, "/");
	strcat(objfile, inp_data[module_ent+1]->string);

	/* Rename the driver to <driver>.o */
	strcpy(drvfile, objfile);
	strcat(drvfile, ".o");
	os_copy(drvfile, objfile);

	/* Make sure the objfile is <SHORTNAME> */
	strcpy(objfile, tmpdir);
	strcat(objfile, "/");
	strcat(objfile, name);

	strcpy(glue_obj, tmpdir);
	strcat(glue_obj, "/udi_glue.o");

	/*
	 * Compile the glue code
	 */
	strcpy(tmpf, tmpdir);
	strcat(tmpf, "/udi_glue.c");

	os_compile(tmpf, glue_obj, UDI_CCOPTS_MAPPER, Oflg, Dflg);

	/*
	 * Parse the udiprops.txt to see if we provide an external interface
	 * (provides udi_xxxx) and if there are any explicit symbols. If so
	 * we add these to the mapfile of exported symbols
	 * Default behaviour is to globally export _init, _fini and _info only
	 */
	
	strcpy(mapfile, tmpdir);
	strcat(mapfile, "/Mapfile");
	fp = fopen(mapfile, "w+");
	if (fp == NULL) {
		printloc(stderr, UDI_MSG_FATAL, 2103, "unable to open "
			 "%s for writing!\n", mapfile);
	}
	/* Just make _init, _fini, _info and _depends_on globally available */
	fprintf(fp, "SUNW_driver_%s_1.1 {\n\tglobal:\n", name);
	fprintf(fp, "\t\t_init;\n\t\t_info;\n\t\t_fini;\n");
	if (depends) {
		fprintf(fp, "\t\t_depends_on;\n");
	}
	fprintf(fp, "\tlocal:\n\t\t*;\n};\n");

	/* Now parse for any "symbols" entries */
	index = 0;
	prov_name = NULL;
	while (inp_data[index] != NULL) {
		if (inp_data[index]->type == PROVIDES) {
			if (prov_name != NULL) {
				/* Close the existing mapfile entry */
				fprintf(fp, "\tlocal:\n\t\t*;\n};\n");
			}
			prov_name = inp_data[index+1]->string;
			prov_ver  = inp_data[index+2]->ulong;
			fprintf(fp, "%s_%x.%02x {\n", prov_name,
				prov_ver / 256, prov_ver % 256);
			fprintf(fp, "\tglobal:\n");
		}
		if (inp_data[index]->type == SYMBOLS) {
			if (inp_data[index+2] == NULL) {
				/* SYMBOLS provided_symbol */
				fprintf(fp, "\t\t%s;\n", 
					inp_data[index+1]->string);
			} else {
				/* SYMBOLS library_symbol AS provided_symbol */
				fprintf(fp, "\t\t%s;\n",
					inp_data[index+1]->string);
			}
			prov_sym = 1;
		}
		while (inp_data[index] != NULL) index++;
		index++;
	}
	if (prov_name != NULL) {
		fprintf(fp, "\tlocal:\n\t\t*;\n};\n");
	} 
	fclose(fp);
					    	
	/*
	 * Link the glue code to the main module for a Driver
	 */
	strcpy(tmpf, tmpdir);
	ii = 0;
	argv[ii++] = udi_global.link;
#if 0 || NEED_DYNAMIC
	argv[ii++] = "-dy";
#endif	/* 0 || NEED_DYNAMIC */
	argv[ii++] = "-r";
#if 0 || OLDWAY
	if (depends) {
		argv[ii++] = depend;
	}
#endif
	argv[ii++] = "-M";
	argv[ii++] = mapfile;
	argv[ii++] = "-Beliminate";
	argv[ii++] = "-o";
	argv[ii++] = objfile;
	argv[ii++] = glue_obj;
	argv[ii++] = drvfile;
	argv[ii++] = NULL;
	
	os_exec(NULL, udi_global.link, argv);

	/*
	 * Copy the resulting driver to the driver directory
	 */
	if (!sflg) {
		os_copy(drvdir, objfile);

		/*
		 * Issue the appropriate add_drv magic to make the driver
		 * available to the system. We don't force a load (yet)
		 */
	}
	free(drvdir);
	chdir(wherewasi);
	free(wherewasi);
}

void
make_glue (char *tmpf, int install_src, char *posix, char *depend)
{
	FILE *fp;
	struct stat fst;
	char *ptr1, *ptr2;
	int i, index, index2, find_type, first, found, tmp_count;
	char buf[1024];
	char locale[256];
	char cur_loc[256];
	unsigned int raflag, overrun;
	int nmsgs, msgnum, metaidx;
	int fd, mfd, fdr, nr;
	long udi_ver;
	char lfunc[40], ulfunc[40];

	if ((fp = fopen(tmpf, "w+")) == NULL) {
		printloc(stderr, UDI_MSG_FATAL, 2103, "unable to open %s for "
				"writing!\n", tmpf);
	}

	/* Get the udi version number */
	index = 0;
	while (input_data[index] != NULL) {
		if (input_data[index]->type == REQUIRES) {
			if (!strcmp(input_data[index+1]->string, "udi")) {
				udi_ver = (long)input_data[index+2]->ulong;
			}
		}
		while (input_data[index] != NULL) index++;
		index++;
	}
	fprintf(fp, "#define _KERNEL\n");

	write_props_glue (fp, input_data, install_src);

	/*
	 * Add the char _depends_on[] = ..... line to udi_glue.c. We should be
	 * able to use the '-N' flag to the linker, but this seems not to work
	 */
	if (strlen(depend) > 0) {
		fprintf(fp, "\nchar _depends_on[] = %s;\n", depend);
	}

	mt_count = cb_count = pb_count = ib_count = pv_count = rf_count =
			l_count = mf_count = ms_count = r_count = d_count =
			e_count = a_count = 0;

	for (find_type = 0; find_type < NUM_FIND; find_type++) {
		first = 1;
		index = 0;

		while (input_data[index] != NULL) {
			switch(find_type) {
			case 0:	/* metas */
				if (first) {
					first = 0;
				}
				if (input_data[index]->type == META) {
					mt_count++;
				}
				break;
			case 1:	/* child_bind_ops */
				if (first) {
					if (mt_count == 0) {
					}
					first = 0;
				}
				if (input_data[index]->type == CHLDBOPS) {
					cb_count++;
				}
				break;
			case 2:	/* parent_bind_ops */
				if (first) {
					if (cb_count == 0) {
					}
					first = 0;
				}
				if (input_data[index]->type == PARBOPS) {
					pb_count++;
				}
				break;
			case 3:	/* internal_bind_ops */
				if (first) {
					if (pb_count == 0) {
					}
					first = 0;
				}
				if (input_data[index]->type == INTBOPS) {
					ib_count++;
				}
				break;
			case 4: /* provides */
				if (first) {
					if (ib_count == 0) {
					}
					first = 0;
				}
				if (input_data[index]->type == PROVIDES) {
					pv_count++;
				}
				break;
			case 5: /* regions */
				if (first) {
					if (pv_count == 0) {
					}
					first = 0;
				}
				if (input_data[index]->type == REGION) {
					r_count++;
					raflag = overrun = 0;
					i = index+2;
					while (input_data[i] != NULL) {
						if (strcmp(input_data[i]->
							string, "overrun_time")
								!= 0) {
							raflag |= input_data
								[i+1]->type;
						} else {
							overrun = input_data
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
				}
				break;
			case 6: /* devices */
				if (first) {
					if (r_count == 0) {
					}
					first = tmp_count = 0;
				}
				if (input_data[index]->type == DEVICE) {
					d_count++;
					i = index+3;
					found = 0;
					while (input_data[i] != NULL) {
						i++;
					}
					i -= index+3;
					i /= 3;
					a_count += i;
					tmp_count += i;
				} else if (input_data[index]->type ==
						ENUMERATES) {
					i = index+5;
					while (input_data[i] != NULL)
						i++;
					i -= index+5;
					i /= 3;
					tmp_count += i;
				}
				break;
			case 7: /* enumerates */
				if (first) {
					if (d_count == 0) {
					}
					first = tmp_count = 0;
				}
				if (input_data[index]->type == ENUMERATES) {
					d_count++;
					i = index+5;
					found = 0;
					while (input_data[i] != NULL) {
						i++;
					}
					i -= index+3;
					i /= 3;
					a_count += i;
				} else if (input_data[index]->type ==
						DEVICE) {
					i = index+3;
					while (input_data[i] != NULL)
						i++;
					i -= index+3;
					i /= 3;
					tmp_count += i;
				}
				break;
			case 8: /* attributes */
				if (first) {
					if (e_count == 0) {
					}
					a_count = d_count = e_count = 0;
					first = 0;
				}
				if (input_data[index]->type == DEVICE ||
						input_data[index]->type ==
						ENUMERATES) {
					if (input_data[index]->type == DEVICE) {
						msgnum = input_data[index+1]->
								ulong;
						metaidx = input_data[index+2]->
								ulong;
						d_count++;
						i = index+3;
					} else {
						msgnum = input_data[index+1]->
								ulong;
						metaidx = input_data[index+4]->
								ulong;
						d_count++;
						i = index+5;
					}
					while (input_data[i] != NULL) {
						switch(input_data[i+1]->type) {
						case UDI_ATTR_STRING:
						        antiparse(NULL,
								  input_data[i+2]->string);
							break;
						case UDI_ATTR_UBIT32:
							break;
						case UDI_ATTR_BOOLEAN:
							break;
						case UDI_ATTR_ARRAY8:
							break;
						}
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
					}
					first = 0;
				}
				if (input_data[index]->type == READFILE) {
					rf_count++;
				}
				break;
			case 10: /* locale */
				if (first) {
					if (rf_count == 0) {
					}
					first = 0;
					l_count = 1;
					nmsgs = 0;
				}
				if (input_data[index]->type == LOCALE) {
					nmsgs = 0;
					l_count++;
				}
				if (input_data[index]->type == MESSAGE ||
					input_data[index]->type == DISMESG) {
					ms_count++;
					nmsgs++;
				}
				break;
			case 11: /* msg_files */
				if (first) {
					strcpy(cur_loc, "C");
					first = 0;
				}
				if (input_data[index]->type == MESGFILE) {
					/*
					 * Read in, and count the number of
					 * locales.
					 */
					FILE *mfp;
					int msgf = 0;
					if (install_src) {
						sprintf(buf, "../msg/%s",
							input_data[index+1]->
							string);
					} else {
						sprintf(buf, "../../msg/%s",
							input_data[index+1]->
							string);
					}
					if ((mfp = fopen(buf, "r")) == NULL) {
						exit(1);
					}
					while(fgets(buf, 1023, mfp) != NULL) {
						if (strncmp("locale ", buf, 7)
								== 0) {
							if (mf_count == 0 &&
								    msgf != 0) {
								mf_count++;
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
					}
					fclose(mfp);
				}
				break;
			case 12: /* messages */
				if (first) {
					if (mf_count == 0) {
					}
					first = ms_count = 0;
					l_count = 1;
					strcpy(locale, "C");
				}
				if (input_data[index]->type == LOCALE) {
					l_count++;
					strcpy(locale, input_data[index+1]->string);
				}
				if (input_data[index]->type == MESSAGE ||
					input_data[index]->type == DISMESG) {
					antiparse(NULL,
						  input_data[index+2]->string);
					ms_count++;
					if (ms_count == desc_index) {
						desc = optinit();
						parse_message(&desc,
						    input_data[index+2]->string,
						    NULL);
					}
				}
				if (input_data[index]->type == NAME) {
					desc_index = input_data[index+1]->ulong;
				}
				break;
			}
			while(input_data[index] != NULL) index++;
			index++;
		}
	}
	ms_count++;

	if (desc_index == -1) {
		/* Error: We must have one NAME entry */
		fprintf(stderr, "FATAL: desc_index not found\n");
		exit(1);
	} 

	/* Create the udi_???_sprops_idx structure */

	if (udi_global.make_mapper == 0) {
		/* Not a mapper */
		if (! pv_count) {
			/* For a driver */
			if (! posix) {
				strcpy(buf2, "udi_init_info");
			} else {
				strcpy(buf2, posix);
				strcat(buf2, "_init_info");
			}
			/* Create declarations for driver */
			make_driver(fp, buf2, name, desc);
		} else {
			/* For a metalanguage */
			if (! posix) {
				strcpy(buf2, "udi_meta_info");
			} else {
				strcpy(buf2, posix);
				strcat(buf2, "_meta_info");
			}

			/*
			 * Make the meta if we're not the 'udi' meta. This is
			 * coded in env/solaris/udi_MA.c
			 */
			if (strcmp(name, "udi")) {
				make_meta(fp, buf2, name, desc);
			}

		}
	} else {
		/*
		 * Mapper - we only produce the sprops, the mapper itself holds
		 * the _init, _fini, _info etc. routines and is responsible for
		 * calling the appropriate MA functions as necessary
		 */
	}

	fclose(fp);

	/* Set the offset for the section headers */
}
