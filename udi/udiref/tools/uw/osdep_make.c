/*
 * File: tools/uw/osdep_make.c
 *
 * UDI UW-specific utility functions used during udisetup
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

#define wrapper (&udi_global.make_mapper)

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "y.tab.h"
#include "spparse.h"
#include "osdep.h"
#include <udi_sprops_ra.h>
#include "common_api.h"		/* Common utility functions */

#define UDI_VERSION 0x101
#include <udi.h>

static udi_boolean_t make_glue (char *tmpf, char *objfile);
char *pkg;
int mt_count, cb_count, pb_count, ib_count, rf_count,
			l_count, mf_count, ms_count, r_count, d_count,
			e_count, a_count;
int udi_version;
int desc_index;
char *desc;
char typedesc[80];
char name[513];
int version;
char buf2[1024];
char modtype[256];
int pioserlim;
static udi_boolean_t driver_had_node_file = FALSE;

char *udi_filedir;
udi_boolean_t installing_on_uw711 = FALSE;

#define exitloc    udi_tool_exit

typedef enum {
	attr_name_off = 1,
	scope_off = 2,
	msgnum1_off = 3,
	msgnum2_off = 4,
	msgnum3_off = 5,
	choices_off = 6,
} custom_offsets_t;

/*
 * Support routines for creating BCFG entries to match the custom device entries
 */
void
make_bcfg_head( FILE *fp, int index, int custom_num )
{
	int ind = index+choices_off;
	fprintf(fp, "CUSTOM[%d]=\"\n", custom_num);
	/*
	 * NOTE: Right now, due to resmgr implications, the
	 * parameter name needs to be limited to 15
	 * characters which includes the null, so we ahve to whack
	 * it to 14 "real" characters.  However, we need to use the
	 * first character for a type specifier, so we really only
	 * have 13 "real" characters ... for now ...
	 */
	/* LINE 1 [Name of variable] */
	fprintf(fp, "\t");
	if (strcmp(input_data[ind]->string, "string") == 0) {
		fprintf(fp, "S");
	} else if (strcmp(input_data[ind]->string, "ubit32") == 0) {
		fprintf(fp, "U");
	} else if (strcmp(input_data[ind]->string, "boolean") == 0) {
		fprintf(fp, "B");
	} else if (strcmp(input_data[ind]->string, "array") == 0) {
		fprintf(fp, "A");
	}
	fprintf(fp, "%.13s\n", input_data[index+attr_name_off]->string+1);
}

void
make_bcfg_common( FILE *fp, int index )
{
        char *title, *prompt, *basic; 

	title  = optinit();
	prompt = optinit();
	basic  = optinit();

	get_message(&title,  input_data[index+msgnum1_off]->ulong, NULL);
	get_message(&prompt, input_data[index+msgnum2_off]->ulong, NULL);
	get_message(&basic,  input_data[index+msgnum3_off]->ulong, NULL);
	
	/* LINE 4 [Title for choices] */
	fprintf(fp, "\t%s\n", title);
	/* LINE 5 [RESERVED] */
	fprintf(fp, "\tRESERVED\n");
	/* LINE 6 [Prompt text] */
	fprintf(fp, "\t%s\n", prompt);
	/* LINE 7 [BASIC/ADVANCED] */
	if (!strcmp(basic, "Basic")) {
		fprintf(fp, "\tBASIC\n");
	}
	if (!strcmp(basic, "Advanced")) {
		fprintf(fp, "\tADVANCED\n");
	}
	/* LINE 8 [TOPOLOGY] */
	fprintf(fp, "\tETHER\n");
	/* LINE 9 [Scope of parameter] */
	if (!strcmp(input_data[index+scope_off]->string, "device") 
 	 || !strcmp(input_data[index+scope_off]->string, "device_optional")) {
		fprintf(fp, "\tBOARD\n");
	} else {
		fprintf(fp, "\tDRIVER\n");
	}
	fprintf(fp, "\"\n");

	free(title);
	free(prompt);
	free(basic);
}

void
make_bcfg_mutex( FILE *fp, int index )
{
	int	i, type;
	char   *msg;

	msg = optinit();

	/*
	 * Handle 'mutex' custom parameters. This can be either a string or
	 * ubit32 type value.
	 */
	if (strcmp(input_data[index+6]->string, "string") != 0) {
		type = 1;
	} else {
		type = 0;
	}
	/* LINE 2 [Single or multi-value entry] */
	fprintf(fp, "\t");
	i = index+9;
	while (strcmp(input_data[i]->string, "end") != 0) {
		if (type) {
			fprintf(fp, " %d", input_data[i]->ulong);
		} else {
			get_message(&msg, input_data[i]->ulong, NULL);
			fprintf(fp, " %s", msg);
		}
		i++;
	}
	fprintf(fp, "\n");
	/* LINE 3 [List of choices] */
	fprintf(fp, "\t");
	i = index+9;
	while (strcmp(input_data[i]->string, "end") != 0) {
		if (!strcmp(input_data[index+2]->string, "%speed_mbps") ||
		    !strcmp(input_data[index+2]->string, "speed_bps")) {
			if (input_data[i]->ulong == 0)
				fprintf(fp, " Autodetect");
			else {
				msg[0] = '\0';
			       	parse_message(&msg,input_data[i]->string,NULL);
				fprintf(fp, " %s", msg);
			}
		} else {
			if (type) {
				msg[0] = '\0';
		       		parse_message(&msg, input_data[i]->string, NULL);
				fprintf(fp, " %s", msg);
			} else {
				get_message(&msg, input_data[i]->ulong, NULL);
				fprintf(fp, " %s", msg);
			}
		}
		i++;
	}
	fprintf(fp, "\n");
	free(msg);
}

#if OLD_BCFG_FORMAT
void
make_bcfg_range( FILE *fp, int index )
{
	udi_ubit32_t min, max, dflt, stride;
	udi_ubit32_t i;

	/*
	 * Handle ranges and values. Only valid for ubit32 type choices.
	 * For now we just provide the default value
	 * and a __string__ choice to allow free-form
	 * input from the user
	 * TODO: make this work according to the spec.
	 */

	/* LINE 2 [Single or multi-value entry] */
	fprintf(fp, "\t%p\n", input_data[index+7]->ulong);

	/* LINE 3 [List of choices] */
        fprintf(fp, "\t__STRING__\n");  /* Free-form */
}

#else

/*
 * Handle ranges and values. Only valid for ubit32 type choices.
 * We use an extension to the BCFG format: __RANGE__ which requires PTFxxxyyy
 * to be installed on the system.
 * Maybe we could determine this at run time and then produce either the new
 * fully featured BCFG, or the old incomplete version ??
 */
void
make_bcfg_range( FILE *fp, int index )
{
	udi_ubit32_t	min, max, dflt, stride;

	/* LINE 2 Range definition [default min max stride] */
	dflt   = input_data[index+7]->ulong;
	min    = input_data[index+9]->ulong;
	max    = input_data[index+10]->ulong;
	stride = input_data[index+11]->ulong;

	fprintf(fp, "\t %3d %3d %3d %3d\n", dflt, min, max, stride );

	/* LINE 3 [List of choices] */
	fprintf(fp, "\t __RANGE__\n");
}
#endif	/* OLD_BCFG_FORMAT */

void
make_bcfg_any( FILE *fp, int index )
{
	/* LINE 2 [Single or multi-value entry] */
	fprintf(fp, "\t %p\n", input_data[index+7]->ulong);

	/* LINE 3 [List of choices] */
	fprintf(fp, "\t __STRING__\n");		/* Free-form */
}

void
make_bcfg_unique( FILE *fp, int index )
{
	/* LINE 2 [Single value] */
	fprintf(fp, "\t%s\n", input_data[index+7]->string);
	/* LINE 3 [Choice] */
	fprintf(fp, "\t%s\n", input_data[index+7]->string);
}

void
make_bcfg_ubit32( FILE *fp, int index, int custom_num )
{
	char *type;
	make_bcfg_head( fp, index, custom_num );
	type = input_data[index+8]->string;
	if (!strcmp(type, "mutex")) {
		make_bcfg_mutex( fp, index );
	} else if (!strcmp(type, "range")) {
		make_bcfg_range( fp, index );
	} else if (!strcmp(type, "any")) {
		make_bcfg_any( fp, index );
	} else if (!strcmp(type, "only")) {
		make_bcfg_unique( fp, index );
	} else {
		printloc(stderr, UDI_MSG_FATAL, 2100, "Illegal choice value "
			"specified '%s'\n", type);
	}
	make_bcfg_common( fp, index );
}

void
make_bcfg_boolean( FILE *fp, int index, int custom_num )
{
	make_bcfg_head( fp, index, custom_num );

	/*
	 * Handle boolean parameters. This is the same
	 * as a mutex of F T
	 */
	/* LINE 2 [Single or multi-value entry */
	if (!strcmp(input_data[index+7]->string, "F")){
		fprintf(fp, "\tF T\n");
		/* Line 3 [List of choices] */
		fprintf(fp, "\tFalse True\n");
	} else {
		fprintf(fp, "\tT F\n");
		/* Line 3 [List of choices] */
		fprintf(fp, "\tTrue False\n");
	}

	make_bcfg_common( fp, index );
}

void
make_bcfg_string( FILE *fp, int index, int custom_num )
{
	char *str;
	char *type;
	make_bcfg_head( fp, index, custom_num );
	type = input_data[index+8]->string;
	if (!strcmp(type, "mutex")) {
		make_bcfg_mutex( fp, index );
	} else if (!strcmp(type, "any")) {
		make_bcfg_any( fp, index );
	} else if (!strcmp(type, "only")) {
	        char *only;
		only = optinit();
		get_message(&only, input_data[index+7]->ulong, NULL);
		/* LINE 2 [Single or multi-value entry */
		fprintf(fp, "\t%s\n", only);
		/*kwq: why is the following just a repeat?! */
		/* Line 3 [List of choices] */
		fprintf(fp, "\t%s\n", only);
		free(only);
	} else {
		printloc(stderr, UDI_MSG_FATAL, 2101, "Illegal choice "
			"specified '%s'\n", type);
	}
	make_bcfg_common( fp, index );
}

void
make_bcfg(char *name, char *desc, FILE *fp) 
{
	int index, sysdev, fnd_attr, i, j;
	int num_cust, custom_num;
	char	temp[14];
	int type = 0;
	char *applies_to;
	int applies_to_ulong;
#if !OLD_BCFG_FORMAT
	char	*custom_typestr = NULL;
#endif	/* !OLD_BCFG_FORMAT */

	/* Figure out the number of custom settings we will put in .bcfg */
	num_cust = 0;
	index = get_prop_type(CUSTOM, 0);
	while (index >= 0) {
		if (input_data[index+attr_name_off]->string[0] == '%') {
			num_cust++;

			/*
			 * This is somewhat cowardly.   We know that for
			 * custom declarations, that <device> is the last 
			 * thing on the line.   Rather than waiting until
			 * after we've parsed the line, we look for it now.
			 *
		  	 */
			applies_to = input_data[index]->string;
			for(i=0;input_data[index+i]->string;i++) {
				applies_to = input_data[index+i]->string;
				applies_to_ulong = input_data[index+i]->ulong;
			}
			/*
			 * FIXME: the specification says we have to 
			 * support this, but there are no current consumers.
			 */

			if (applies_to_ulong != 0) {
				printloc(stderr, UDI_MSG_FATAL, 2112,
					"non-zero <device> not yet supported "
					"in custom decl\n");
				
				abort();
			}
		}
		index = get_prop_type(CUSTOM, index+1);
	}
#if !OLD_BCFG_FORMAT
	if (num_cust > 0) {
		custom_typestr = optinit();
	}
#endif	/* !OLD_BCFG_FORMAT */

	fprintf(fp, "#$version 1\n\n");
	fprintf(fp, "#######################################\n");
	fprintf(fp, "#MANIFEST:\n\n");
	fprintf(fp, "FILES=\"Driver.o Drvmap Master Node System\"\n\n");
	fprintf(fp, "#######################################\n");
	fprintf(fp, "#DRIVER:\n\n");
	fprintf(fp, "NAME=\"%s\"\n\n", desc);
	fprintf(fp, "HELPFILE=\"HW_network ncfgN.");
	fprintf(fp, "configuring_hardware.html\"\n\n");
	fprintf(fp, "PROMISCUOUS=true\n\n");
	fprintf(fp, "FAILOVER=true\n\n");
#if OLD_BCFG_FORMAT
	fprintf(fp, "TYPE=MDI\n\n");
#else
	fprintf(fp, "TYPE=UDI\n\n");
#endif	/* OLD_BCFG_FORMAT */
	fprintf(fp, "DRIVER_NAME=%s\n\n", name);
	fprintf(fp, "CUSTOM_NUM=%d\n\n", num_cust);
	/* Search for the custom parameters */
	custom_num = 0;
	index = get_prop_type(CUSTOM, 0);
	while (index >= 0) {
		if (strncmp(input_data[index+attr_name_off]->string, "%", 1) == 0) {
			char *stype = input_data[index+6]->string;
			custom_num++;
			if (!strcmp(stype, "ubit32")) {
				type = 1;
			} else if (!strcmp(stype, "boolean")) {
				type = 2;
			} else if (!strcmp(stype, "string")) {
				type = 3;
			} else if (!strcmp(stype, "array")) {
				type = 4;
			}
			switch ( type ) {
			case 1: /* ubit32 */
				make_bcfg_ubit32( fp, index, custom_num );
#if !OLD_BCFG_FORMAT
				optcat(&custom_typestr, "ubit32 ");
#endif	/* !OLD_BCFG_FORMAT */
				break;
			case 2:	/* boolean */
				make_bcfg_boolean( fp, index, custom_num );
#if !OLD_BCFG_FORMAT
				optcat(&custom_typestr, "boolean ");
#endif	/* !OLD_BCFG_FORMAT */
				break;
			case 3:	/* string */
				make_bcfg_string( fp, index, custom_num );
#if !OLD_BCFG_FORMAT
				optcat(&custom_typestr, "string ");
#endif	/* !OLD_BCFG_FORMAT */
				break;
			case 4:	/* array */
			default: /* error */
				printloc(stderr, UDI_MSG_FATAL, 2102,
					"Unsupported custom device type '%s'\n",
					stype);
			}
		}
		index = get_prop_type(CUSTOM, index+1);
	}
#if !OLD_BCFG_FORMAT
	/*
	 * Produce a CUSTOM_TYPE entry which contains the UDI type of each
	 * custom parameter
	 */
	if (custom_num > 0) {
		fprintf(fp, "CUSTOM_TYPE=\"\n%s\n\"\n", custom_typestr);
		free(custom_typestr);
	}
#endif	/* !OLD_BCFG_FORMAT */
	fprintf(fp, "#######################################\n");
	fprintf(fp, "#ADAPTER:\n\n");
	fprintf(fp, "MAX_BD=8\n\n");
	fprintf(fp, "ACTUAL_RECEIVE_SPEED=0\n");
	fprintf(fp, "ACTUAL_SEND_SPEED=0\n\n");
	fprintf(fp, "CONFORMANCE=0x100\n\n");
	fprintf(fp, "BUS=PCI\n\n");		/* FIXME */
	fprintf(fp, "TOPOLOGY=ETHER\n\n");	/* FIXME */
	fprintf(fp, "BOARD_IDS=\"");
	/* Search for the Device lines */
	sysdev = 0;
	fnd_attr = 0;
	index = get_prop_type(DEVICE, 0);
	while (index >= 0) {
		for (j = 0; j < 4; j++) {
			i = index + 3;
			while (input_data[i] != NULL) {
			switch (j) {
			case 0:	/* bus type */
				if (strcmp(input_data[i]->string,
						"bus_type") == 0) {
					if (strcmp(input_data[i+2]->string,
							"system") == 0) {
						sysdev = 1;
					}
				}
				break;
			case 1: /* pci vendor id */
				if (sysdev) break;
				if (strcmp(input_data[i]->string,
						"pci_vendor_id") == 0) {
					if (fnd_attr)
						fprintf(fp, " ");
					fprintf(fp, "0x%04X", strtoul(
							input_data[i+2]->string,
							NULL, 0));
					fnd_attr = 1;
				}
				break;
			case 2: /* pci device id */
				if (sysdev) break;
				if (strcmp(input_data[i]->string,
						"pci_device_id") == 0) {
					fprintf(fp, "%04X", strtoul(
							input_data[i+2]->string,
							NULL, 0));
				}
				break;
			}
			i += 3;
			}
		}
		index = get_prop_type(DEVICE, index+1);
	}
	fprintf(fp, "\"\n");
}

void
make_depend(void *arg, char *name, char *meta, int type)
{
	FILE *fp;
	char *buf;
	char *ctmp = "";

	if (type == 1) {
		fp = (FILE *)arg;
	} else {
		buf = (char *)arg;
	}

	/*
	 * Temporary fix.  Really, the
	 * MA should load the mappers.
	 * However, we will do it here
	 * for now.
	 */
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

	/* Add the mapper dependency */
	if ( *ctmp != '\0' ) {
		if (type == 1)
			fprintf(fp, "$depend %s\n", ctmp);
		else
			strcpy(buf, ctmp);
	}
}

void
get_mod_type(char *modtype)
{
	int index, index2;

	index = *modtype = 0;
	while (input_data[index] != NULL) {
		if (input_data[index]->type == CHLDBOPS) {
			index2 = 0;
			while (input_data[index2] != NULL) {
				if (input_data[index2]->type == META &&
						input_data[index+1]->ulong ==
						input_data[index2+1]->ulong) {
					if (strcmp(input_data[index2+2]->string,
							"udi_scsi") == 0) {
						strcpy(modtype, "scsi");
					} else if (strcmp(input_data[index2+2]->
								string,
								"udi_nic") == 0)
								{
						if (strcmp(modtype, "scsi")
								!= 0)
							strcpy(modtype, "nic");
					} else if (strcmp(input_data[index2+2]->
								string,
								"udi_comm") ==
								0) {
						if (strcmp(modtype, "scsi")
								!= 0 && strcmp(
								modtype, "nic")
								!= 0)
							strcpy(modtype, "comm");
					} else if (strcmp(input_data[index2+2]->
								string,
								"udi_vid") ==
								0) {
						if (strcmp(modtype, "scsi")
								!= 0 && strcmp(
								modtype, "nic")
								!= 0 && strcmp(
								modtype, "comm")
								!= 0)
							strcpy(modtype, "vid");
					} else if (strcmp(input_data[index2+2]->
								string,
								"udi_snd") ==
								0) {
						if (strcmp(modtype, "scsi")
								!= 0 && strcmp(
								modtype, "nic")
								!= 0 && strcmp(
								modtype, "comm")
								!= 0 && strcmp(
								modtype, "vid")
								!= 0)
							strcpy(modtype, "snd");
					} else if (strcmp(input_data[index2+2]->
								string,
								"udi_gio") ==
								0) {
						if (strcmp(modtype, "scsi")
								!= 0 && strcmp(
								modtype, "nic")
								!= 0 && strcmp(
								modtype, "comm")
								!= 0 && strcmp(
								modtype, "vid")
								!= 0 && strcmp(
								modtype, "snd")
								!= 0)
							strcpy(modtype, "misc");
					}
					break;
				}
				while (input_data[index2] != NULL) index2++;
				index2++;
			}
		}
		while (input_data[index] != NULL) index++;
		index++;
	}
}

void
get_type_desc(char *modtype)
{
	strcpy(typedesc, "UDI ");
	if (strcmp(modtype, "scsi") == 0)
		strcat(typedesc, "SCSI Host Bus Adapters");
	else if (strcmp(modtype, "nic") == 0)
		strcat(typedesc, "Network Interface Cards");
	else if (strcmp(modtype, "comm") == 0)
		strcat(typedesc, "Communications Cards");
	else if (strcmp(modtype, "vid") == 0)
		strcat(typedesc, "Video Cards");
	else if (strcmp(modtype, "snd") == 0)
		strcat(typedesc, "Sound Boards");
	else if (strcmp(modtype, "misc") == 0)
		strcat(typedesc, "Miscellaneous");
	else
		strcat(typedesc, desc);
}

/*
 * Figure out if we're on UnixWare 7.1.1 or OpenUNIX 8 so we can
 * act differently elsewhere in this file.
 */
static 
void set_os_version(void)
{
	char sysname[100];
	char version[100];
	confstr(_CS_SYSNAME, sysname, sizeof(sysname));
	confstr(_CS_VERSION, version, sizeof(version));
	if ((strcmp(sysname, "UnixWare") == 0) &&
	    (atoi((const char *) version) == 7)) {
		installing_on_uw711 = TRUE;
	}
}

void
osdep_mkinst_files(int install_src, propline_t **inp_data, char *basedir,
		   char *tmpdir)
{
	char *drivfiles[] = {
		"disk.cfg",
		"Driver.o",
		"Drvmap",
		"Master",
		"Node",
		"System",
		NULL
	};
	FILE *fp;
	char tmpf[513], tmpf2[513];
	char pname[9];
	int modtypeindex;
	char modstr[64];
	int find_type;
	int first;
	int found;
	int index, index2;
	int sysdev;
	int i, j;
	int nmsgs, msgnum, metaidx, devmsgnum;
	char locale[256];
	char cur_loc[256];
	int ipl;
	int tmp_count;
	char *direct_install;
	char *cwd;
	char *current_dir;
	char *args[100];
	char *cptr;
	char *tfile;
#if 0
	extern int Dflg, Oflg, sflg;
#else
#define Dflg udi_global.debug
#define Oflg udi_global.optimize
#define sflg udi_global.stop_install
#endif	
	DIR *dir;
	struct dirent *entry;
	char *prov_name;
	int prov_ver, prov_sym;
	char *nmstr;
	char provv1[10], provv2[10];
	int fnd_attr;
	char *intfc;
	struct stat filestat;
	unsigned int overrun, raflag;
	int bridgemetaidx, havehw;

	char glue_obj[512], drvfile[512], loadfile[512], modfile[512], objfile[512];
	char *argv[10];
	int ii;

	char *spfname, *tstr;
	int numinp;
	extern int parsemfile;

        char    *idcmdstr;      /* idXXXXXXX command string */
        char    *idargstr;      /* argument for idXXXXX command */
        char    *intfstr = optinit();   /* /etc/conf/interface.d string */
        char    *incstr = NULL;
        char    *confstr = NULL;
        char    *rootstr= NULL; /* root of ID tree */
        int     KernelBuild = 0;        /* We're running in a kernel xenv */
	char    *wherewasi = getcwd(NULL, 513);
	udi_boolean_t glue_made;

	set_os_version();

	chdir(basedir);
	modtype[0] = '\0';

	if (install_src) {
		/* Do the udibuild, for this architecture */
		args[0] = "udibuild";
		args[1] = "-o";
		args[2] = tmpdir;
		i = 3;
		if (Dflg) {
			args[i] = "-D";
			i++;
		} else if (Oflg) {
			args[i] = "-O";
			i++;
		}
		args[i] = NULL; 
		i++;
		os_exec(basedir, args[0], args);
	}

	ipl = -1;

        /*
         * Use UnixWare Kernel Build environment if set. Otherwise default to
         * local 'root' build paths
         */
        if ( getenv("ROOT") != NULL ) {
		rootstr = optinit();
		optcpy(&rootstr, getenv("ROOT"));

                if ( NULL != (char *)getenv("MACH")) {
			optcat(&rootstr, "/");
			optcat(&rootstr, getenv("MACH"));
                        KernelBuild = 1;
                } else {
                        rootstr = NULL;
                }
        }

        confstr = (char *)getenv("CONF");
        if (confstr == NULL) {
                if ( rootstr != NULL ) {
                        confstr = optinit();
                        optcpy(&confstr, rootstr);
                        optcat(&confstr, "/etc/conf");
                } else {
                        confstr = "/etc/conf";
                }
        }

        incstr = (char *)getenv("INC");
        if ( incstr == NULL ) {
		incstr=optinit();
		if ( KernelBuild ) {
			optcpy(&incstr, rootstr);
		}
                optcat(&incstr, "/usr/include/udi");
        } else {
                strcat(incstr, "/udi");
        }

	/* Scan for various things */
	bridgemetaidx = pioserlim = -1;
	index = 0;
	while (inp_data[index] != NULL) {
		if (inp_data[index]->type == SHORTNAME) {
			strcpy(name, inp_data[index+1]->string);
		} else if (inp_data[index]->type == CATEGORY) {
			modtypeindex = inp_data[index+1]->ulong;
		} else if (inp_data[index]->type == NAME) {
			desc_index = inp_data[index+1]->ulong;
		} else if (inp_data[index]->type == PIOSERLIMIT) {
			pioserlim = inp_data[index+1]->ulong;
		} else if (inp_data[index]->type == PROPVERS) {
			version = inp_data[index+1]->ulong;
		} else if (inp_data[index]->type == REQUIRES) {
			if (strcmp(inp_data[index+1]->string, "udi") == 0)
				udi_version = inp_data[index+2]->ulong;
		} else if (inp_data[index]->type == META) {
			if (strcmp(inp_data[index+2]->string, "udi_bridge")
					== 0)
				bridgemetaidx = inp_data[index+1]->ulong;
		} else if (inp_data[index]->type == REGION) {
			if (inp_data[index+1]->ulong == 0) {
				i = index + 2;
				while (inp_data[i] != NULL) {
					if (strcmp("latency", inp_data[i]->string)
							== 0) {
						if (strcmp("powerfail_warning",
							inp_data[i+1]->string)
							== 0) {
							ipl = 7;
						} else if (strcmp("overrunable",
							inp_data[i+1]->string)
							== 0) {
							ipl = 6;
						} else if (strcmp("retryable",
							inp_data[i+1]->string)
							== 0) {
							ipl = 5;
						} else if (strcmp("non_overrunable",
							inp_data[i+1]->string)
							== 0) {
							ipl = 4;
						} else if (strcmp("non_critical",
							inp_data[i+1]->string)
							== 0) {
							ipl = 3;
						}
					}
					i += 2;
				}
			}
		}
		while (inp_data[index] != NULL) index++;
		index++;
	}

	get_message(&desc, desc_index, NULL);

	get_mod_type(modtype);

	/* For specific categories of devices, override hints.  */
	if (strcmp(modtype, "nic") == 0) {
		ipl = 6;
	}
	if (strcmp(modtype, "scsi") == 0) {
		ipl = 5;
	}

	/* If region declaration did not specify latency, use default */
	if (ipl < 0)
		ipl = 4;

	/*
	 * Find the child_bind_ops to determine the driver type.
	 * Also, check for parent_bind_ops using the udi_bridge meta.
	 */
	index = havehw = 0;
	while (inp_data[index] != NULL) {
		if (inp_data[index]->type == PARBOPS) {
			if (inp_data[index+1]->ulong == bridgemetaidx)
				havehw = 1;
		}
		while (inp_data[index] != NULL) index++;
		index++;
	}

	/*
	 * Set the readable/msg file directory
	 */
	if (! *udi_filedir) {
                if (strcmp(modtype, "nic") != 0) {
                        if ( KernelBuild ) {
                                /* Kernel source build */
                                optcpy(&udi_filedir, rootstr);
                                optcat(&udi_filedir, UDI_DRVFILEDIR);
                        } else {
                                /* normal build */
                                optcpy(&udi_filedir, UDI_DRVFILEDIR);
                        }
                } else {
                        if ( KernelBuild ) {
                                /* Kernel source build */
                                optcpy(&udi_filedir, rootstr);
                                optcat(&udi_filedir, UDI_DRVFILEDIR);
                        } else {
                                /* normal build */
                                optcpy(&udi_filedir, UDI_NICFILEDIR);
                        }
                }
	}
	optcat(&udi_filedir, "/");
	optcat(&udi_filedir, name);
	optcat(&udi_filedir, "/");
	
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

	/* Loop through the messages, and completely resolve them */
	i = 0;
	tstr = optinit();
	while (inp_data[i] != NULL) {
		if (inp_data[i]->type == MESSAGE ||
				inp_data[i]->type == DISMESG) {
			tstr[0] = 0;
			parse_message (&tstr, inp_data[i+2]->string, "\n");
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

	/*
	 * For binary distributions, copy the module into the tmp dir
	 */
	if (! install_src) {
	    os_copy(tmpdir, inp_data[module_ent+1]->string);
	}

	strcpy(objfile, tmpdir);
	strcat(objfile, "/");
	strcat(objfile, inp_data[module_ent+1]->string);

	strcpy(glue_obj, tmpdir);
	strcat(glue_obj, "/udi_glue.o");

	strcpy(drvfile, tmpdir);
	strcat(drvfile, "/Driver.o");

	strcpy(loadfile, tmpdir);
	strcat(loadfile, "/load_mod");

	strcpy(modfile, tmpdir);
	strcat(modfile, "/mod_sec.o");

	glue_made = make_glue(tmpf, objfile);

	if (glue_made) {
		strcpy(tmpf, tmpdir);
		strcat(tmpf, "/udi_glue.c");
		os_compile(tmpf, glue_obj, NULL, 0, 0);

		/*
		 * Link the glue code to main module, for a Driver.o
		 */
		strcpy(tmpf, tmpdir);
		/* We will need to hide either udi_init_info or udi_meta_info */
		ii = 0;
		argv[ii++] = udi_global.link;
		argv[ii++] = "-r";
		argv[ii++] = IS_META ? "-Bhide=udi_meta_info" : "-Bhide=udi_init_info";
		argv[ii++] = "-o";
		argv[ii++] = drvfile;
		argv[ii++] = glue_obj;
		argv[ii++] = objfile;
		argv[ii++] = NULL;
		os_exec(NULL, udi_global.link, argv);
	} else {
		ii = 0;
		argv[ii++] = "cp";
		argv[ii++] = objfile;
		argv[ii++] = drvfile;
		argv[ii++] = NULL;
		os_exec(NULL, argv[0], argv);
	}

	/*
	 * For convenience, add some optional symbols to the generated 
	 * object to make it easier to find the udiprops section and size.
	 */
	prepare_elf_object(drvfile, name, NULL);

	/*
	 * Create the Master file
	 */
	strcpy(tmpf, tmpdir);
	strcat(tmpf, "/Master");

	if ((fp = fopen(tmpf, "w+")) == NULL) {
		printloc(stderr, UDI_MSG_FATAL, 2103, "unable to open %s for "
				"writing!\n", tmpf);
	}

	/* Interfaces and Node creation. */

	fprintf(fp, "$version 2\n");
			
	index = 0;
	while (inp_data[index] != NULL) {
		switch (inp_data[index]->type) {
			case REQUIRES: {
			if (*(inp_data[index+1]->string) != '%') {
				fprintf(fp, "$interface %s ",
					inp_data[index+1]->string);
				if ((udi_global.make_mapper == 0) ||
				    ((inp_data[index+2]->string[0] == '0') &&
				    (inp_data[index+2]->string[1] == 'x'))) {
					fprintf(fp, "%x.%02x\n",
						inp_data[index+2]->ulong/256,
						inp_data[index+2]->ulong%256);
				} else {
					fprintf(fp, "%s\n", 
						inp_data[index+2]->string);
				}
				
			} else
				fprintf(fp, "$interface intern_%s %x.%02x\n",
					(inp_data[index+1]->string)+1,
					inp_data[index+2]->ulong/256,
					inp_data[index+2]->ulong%256);
			}
			break;
			/*
			 * Handle any special directives for mappers.
			 * We don't have to check for is_mapper here 
			 * as these tokens would not have made it 
			 * throught he parser otherwise.
			 */
			case MAPPER: {
				FILE *nfp;
				int ndx;

				if (inp_data[index+1] && 
				    strcmp(inp_data[index+1]->string, "node"))
					break;

				driver_had_node_file = TRUE;
				strcpy(tmpf, tmpdir);
				strcat(tmpf, "/Node");

				if ((nfp = fopen(tmpf, "a+")) == NULL) {
					printloc(stderr, UDI_MSG_FATAL, 
						2103, "unable to open %s for "
						"writing!\n", tmpf);
				}

				for (ndx = index + 2; inp_data[ndx]->string; 
				     ndx++) {
					fprintf(nfp, "%s ", 
						inp_data[ndx]->string);
				}
				fprintf(nfp, "\n");
				fclose(nfp);
			}
		}
		while (inp_data[index] != NULL) index++;
		index++;
	}

	/* NIC driver hack: netcfg on 711 requires a "ddi" line */
	if (installing_on_uw711 && (strcmp(modtype, "nic") == 0)) {
		fprintf(fp, "$interface ddi 8mp\n");
	}

	/*
	 * As a special case, let the core environment be $interface base.
	 */
	if ( strcmp(name, "udi") == 0 ) {
		fprintf(fp, "$interface base\n");
	}

	/* Dependencies */

	if (udi_global.make_mapper == 0) {
		/* Not a mapper, so use metas in [child|parent]_bind_ops only */
		index = 0;
		while (inp_data[index] != NULL) {
			if (inp_data[index]->type == CHLDBOPS ||
					inp_data[index]->type == PARBOPS) {
				index2 = 0;
				while(inp_data[index2] != NULL) {
					if (inp_data[index2]->type == META &&
						inp_data[index2+1]->ulong ==
						inp_data[index+1]->ulong) {
						make_depend(fp,
							name,
							inp_data
							[index2+2]->string,
							1);
					}
					while (inp_data[index2] != NULL)
						index2++;
					index2++;
				}
			}
			while (inp_data[index] != NULL) index++;
			index++;
		}
	}

	/* Master line */

	if (udi_global.make_mapper != 0 || IS_META) {
		/* It is a mapper or meta */
		if (driver_had_node_file) 
			fprintf(fp, "%s - R\n", name);
		else
			fprintf(fp, "%s - -\n", name);
	} else {
		if (havehw)
			/* It is a hardware driver. */
			fprintf(fp, "%s - h\n", name);
		else
			/* It is a pseudo driver.  OS creates dev nodes.  */
			fprintf(fp, "%s - R\n", name);
	}

	fclose(fp);

	/*
	 * Create the System file
	 */
	strcpy(tmpf, tmpdir);
	strcat(tmpf, "/System");

	/* If there is a system file already installed, check for $static */
	strcpy(tmpf2, confstr);
	strcat(tmpf2, "/sdevice.d/");
	strcat(tmpf2, name);
	if (access(tmpf2, R_OK) == 0) {
		FILE *sfp;
		if ((sfp = fopen(tmpf2, "r")) != 0) {
			while (fgets(tmpf2, 256, sfp) != 0) {
				if (strncmp(tmpf2, "$static", 7) == 0)
					break;
			}
			if (feof(sfp) == 0)
				ii = 1;
			else
				ii = 0;
			fclose(sfp);
		} else {
			ii = 0;
		}
	} else {
		ii = 0;
	}

	/* Create the file */
	if ((fp = fopen(tmpf, "w+")) == NULL) {
		printloc(stderr, UDI_MSG_FATAL, 2103, "unable to open %s for "
				"writing!\n", tmpf);
	}

	fprintf(fp, "$version 2\n");
	if (ii)
		fprintf(fp, "$static\n");
	fprintf(fp, "%s Y 0 %d 0 0 0 0 0 0 -1\n", name, ipl);

	fclose(fp);

	/*
	 * Create the Node file
	 */
	if (strcmp(modtype, "nic") == 0 || strcmp(modtype, "scsi") == 0 ||
	    strcmp(modtype, "misc") == 0 ) {
		strcpy(tmpf, tmpdir);
		strcat(tmpf, "/Node");

		if ((fp = fopen(tmpf, "w+")) == NULL) {
			printloc(stderr, UDI_MSG_FATAL, 2103, "unable to open "
					"%s for writing!\n", tmpf);
		}

		if (strcmp(modtype, "nic") == 0) {
			fprintf(fp, "$maxchan 1\n");
			fprintf(fp, "%s  mdi/%s%%i  c  0  0  0  600\n", name,
					name);
		} else if (strcmp(modtype, "scsi") == 0) {
			fprintf(fp, "$maxchan 0\n");
			fprintf(fp, "%s  %s%%i  c  0  0  0  600\n", name, name);
		} else if (strcmp(modtype, "misc") == 0) {
			fprintf(fp, "$maxchan 0\n");
			fprintf(fp, "%s  %s%%i  c  0  0  0  600\n", name, name);
		}

		fclose(fp);
	}

	/*
	 * Create a Drvmap file
	 */
	strcpy(tmpf, tmpdir);
	strcat(tmpf, "/Drvmap");

	if ((fp = fopen(tmpf, "w+")) == NULL) {
		printloc(stderr, UDI_MSG_FATAL, 2103, "unable to open %s for "
				"writing!\n", tmpf);
	}

	fprintf(fp, "%s|Y|N|", name);
	get_type_desc(modtype);
	fprintf(fp, "%s", typedesc);
	fprintf(fp, "|%s\n", desc);
	/* Search for the Device lines */
	sysdev = 0;
	fnd_attr = 0;

	for (index = get_prop_type(DEVICE, 0); index >= 0; 
	     index = get_prop_type(DEVICE, index + 1)) {
		unsigned long pci_class = 0UL;
		unsigned long pci_vid = 0UL;
		char *bus_type = "PCI";
		char *s;

		for(i = index + 3; inp_data[i]; i += 3) {
			char *type = inp_data[i]->string;
			char *arg = inp_data[i+2]->string;

			if (0 == strcmp(type, "bus_type")) {
				fnd_attr = 1;
				if (0 == strcmp(arg, "system"))
					bus_type = "SYSTEM";
			}
			if (0 == strcmp(type, "pci_device_id")) {
				pci_vid |= strtoul (arg, NULL, 0);
			}
			if (0 == strcmp(type, "pci_vendor_id")) {
				pci_vid |= strtoul(arg, NULL, 0) << 16;
			}
			if (0 == strcmp(type, "pci_base_class")) {
				pci_class |= strtoul(arg, NULL, 0) & 0xff;
			}
			if (0 == strcmp(type, "pci_sub_class")) {
				pci_class |= strtoul(arg, NULL, 0) << 8;
			}
		}
		fprintf(fp, "|%s|", bus_type);
		if (pci_class) {
			fprintf(fp, "CLASS0x%04lX", pci_class);
		} else {
			fprintf(fp, "0x%04lX", pci_vid);
		}

		/* look for specific device name */
		devmsgnum = inp_data[index+1]->ulong;

		s = NULL;
		if (fnd_attr) {
			fprintf(fp, "|%s",
				internal_get_message(&s,devmsgnum, NULL));
		}
		fprintf(fp, "\n");
	}
	fclose(fp);

	/*
	 * Create a bcfg file if this is a Network driver
	 */
	if (strcmp(modtype, "nic") == 0) {
		strcpy(tmpf, tmpdir);
		strcat(tmpf, "/");
		strcat(tmpf, name);
		strcat(tmpf, ".bcfg");

		if ((fp = fopen(tmpf, "w+")) == NULL) {
			printloc(stderr, UDI_MSG_FATAL, 2103, "unable to open "
					"%s for writing!\n", tmpf);
		}

		make_bcfg(name, desc, fp);

		fclose(fp);
	}

	/*
	 * Create an disk.cfg file, if this is a scsi driver
	 */
	if (strcmp(modtype, "scsi") == 0) {
		strcpy(tmpf, tmpdir);
		strcat(tmpf, "/");
		strcat(tmpf, "disk.cfg");

		if ((fp = fopen(tmpf, "w+")) == NULL) {
			printloc(stderr, UDI_MSG_FATAL, 2103, "unable to open "
					"%s for writing!\n", tmpf);
		}

		fprintf(fp, "NAMES=%s\n", name);
		fprintf(fp, "NAMEL=%s\n", desc);
		fprintf(fp, "DEVICE=SCSI\n");

		fclose(fp);
	}

	/*
	 * Create any interface files
	 */
	index = prov_sym = 0;
	prov_name = NULL;
	intfc = optinit();
	if (strcmp(name, "udi")) {
	while (inp_data[index] != NULL) {
		if (inp_data[index]->type == MODULE) {
			strcpy(tmpf, inp_data[index+1]->string);
		}
		if (inp_data[index]->type == PROVIDES) {
			if (prov_name != NULL) {
				/* Complete opened file */
				if (prov_sym) {
					/* symbols were declared, so close */
					fclose(fp);
				} else {
					/* No symbols declared, so get all */
					fclose(fp);
					nmstr = optinit();
					optcpy(&nmstr, "/usr/ccs/bin/nm -h "
							"-pl ");
					optcat(&nmstr, tmpdir);
					optcat(&nmstr, "/");
					optcat(&nmstr, tmpf);
					optcat(&nmstr, " | cut -d' ' "
						"-f2- | grep \"^[^a-zU]"
						" \" | cut -c4- >> ");
					optcat(&nmstr, intfc);
					args[0] = "/usr/bin/sh";
					args[1] = "-c";
					args[2] = nmstr;
					args[3] = NULL;
					os_exec(NULL, args[0], args);
					free(nmstr);
				}
			}
			prov_name = inp_data[index+1]->string;
			prov_ver = inp_data[index+2]->ulong;
			prov_sym = 0;
			/* Start the new one */
			optcpy(&intfc, tmpdir);
			optcat(&intfc, "/");
			if (*prov_name != '%') {
				optcat(&intfc, prov_name);
			} else {
				optcat(&intfc, "intern_");
				optcat(&intfc, prov_name+1);
			}
			optcat(&intfc, ".");
			sprintf(provv2, "%x", prov_ver/256);
			optcat(&intfc, provv2);
			optcat(&intfc, ".");
			sprintf(provv1, "%02x", prov_ver%256);
			optcat(&intfc, provv1);
			optcat(&intfc, ".intrfc");
			if ((fp = fopen(intfc, "w+")) == NULL) {
				printloc(stderr, UDI_MSG_FATAL, 2103, "unable "
					"to open %s for writing!\n", tmpf);
			}
			if (installing_on_uw711) {
				fprintf(fp, "$entrytype 1\n");
			} else { 
				fprintf(fp, "$entrytype 2\n");
			}
			/* Add the appropriate depends line(s)  */
			if (udi_global.make_mapper != 0 || !IS_META) {
				/* Only add for mappers or drivers, not metas */
				fprintf(fp, "$depend udi\n");
			}
			if (strcmp(prov_name, "udi_mei") != 0)
				fprintf(fp, "$depend %s\n", name);
		}
		if (prov_name != NULL && inp_data[index]->type == SYMBOLS) {
			if (inp_data[index+2] == NULL) {
				/* symbols provided_symbol */
				fprintf(fp, "%s\n", inp_data[index+1]->
						string);
			} else {
				/* symbols library_symbol as provided_symbol */
				fprintf(fp, "%s\t%s\n",
						inp_data[index+3]->string,
						inp_data[index+1]->string);
			}
			prov_sym = 1;
		}
		while (inp_data[index] != NULL) index++;
		index++;
	}
	}
	/* Complete last opened file */
	if (prov_name != NULL) {
		if (prov_sym) {
			/* symbols were declared, so close */
			fclose(fp);
		} else {
			/* No symbols declared, so get all */
			fclose(fp);
			nmstr = optinit();
			optcpy(&nmstr, "/usr/ccs/bin/nm -h -pl ");
			optcat(&nmstr, tmpdir);
			optcat(&nmstr, "/");
			optcat(&nmstr, tmpf);
			optcat(&nmstr, " | cut -d' ' -f2- | "
					"grep \"^[^a-zU] \""
					" | cut -c4- >> ");
			optcat(&nmstr, intfc);
			args[0] = "/usr/bin/sh";
			args[1] = "-c";
			args[2] = nmstr;
			args[3] = NULL;
			os_exec(NULL, args[0], args);
			free(nmstr);
		}
	}

	/*
	 * Now, run the commands for the actual installation
	 */

	direct_install = getenv("UDI_DIRECT_INSTALL");
	if (direct_install) {
		pkg = NULL;
	} else {
		pkg = udi_global.pkg_install;
	}

	/* If the stop before actual install flag is not set, do install */
	if ((direct_install && ! sflg) || ((pkg && *pkg != 0) && ! sflg)) {
		/* Install something from the environment package, directly */

		/*
		 * Save the current directory.
		 */
		if ((cwd = (char *)getcwd(NULL, 513)) == NULL) {
			printloc(stderr, UDI_MSG_ERROR, 1503, "unable to get "
					"current working directory!\n");
			exitloc(2);
		}
		current_dir = strdup(cwd);
		free(cwd);

		/* Go to the dir with the Architecture specific files in it */
		if (chdir(tmpdir) != 0) {
			printloc(stderr, UDI_MSG_ERROR, 2105, "unable to "
					"change to directory %s!\n", tmpdir);
			exitloc(2);
		}

		if (strcmp(name, "udi")) {
		/*
		 * First, install any interface files
                 * Use UnixWare kernel build environment if present
		 */
		if (KernelBuild) {
			optcpy(&intfstr, getenv("ROOT"));
			optcat(&intfstr, "/usr/src/");
			optcat(&intfstr, getenv("WORK"));
			optcat(&intfstr, "/uts");
		} else {
	                optcpy(&intfstr, confstr);
		}
                optcat(&intfstr, "/interface.d");
                nmstr = optinit();
                optcpy(&nmstr, "for file in $(/bin/ls *\\.intrfc 2> ");
                optcat(&nmstr, "/dev/null); do ");
                optcat(&nmstr, "dest_file=${file%%\\.intrfc}; ");
                optcat(&nmstr, "mv -f $file ");
                optcat(&nmstr, intfstr);
                optcat(&nmstr, "/$dest_file ");
                optcat(&nmstr, "|| exit 1;");
                optcat(&nmstr, "chmod 0644 ");
                optcat(&nmstr, intfstr);
                optcat(&nmstr, "/$dest_file;");

                /*
                 * Only chown to root:sys and installf if running natively
                 */
                if ( !KernelBuild ) {
                        optcat(&nmstr, "chown root:sys /etc/conf/interface.d/$dest_file;");
                        if (pkg) {
                                optcat(&nmstr, "/usr/sbin/installf ");
                                optcat(&nmstr, pkg);
                                optcat(&nmstr, " /etc/conf/interface.d/$dest_file f;");
                        }
                }
		optcat(&nmstr, "done; exit 0");
		args[0] = "/usr/bin/ksh";
		args[1] = "-c";
		args[2] = nmstr;
		args[3] = NULL;
		os_exec(NULL, args[0], args);
		free(nmstr);
		}

		/*
		 * Install any header files in /usr/include/udi/name/version.
                 * Use $(INC) if set, otherwise default to root location
		 */
                if (mkdir(incstr, 0755) == -1 && errno != EEXIST) {
                        printloc(stderr, UDI_MSG_ERROR, 2106, "unable to "
					"create the %s directory!\n", incstr);
                        exitloc(2);
                } else if (errno != EEXIST) {
                        if ( !KernelBuild ) {
                                if (chown(incstr, 2, 2) == -1) {
                        		printloc(stderr, UDI_MSG_ERROR, 2107,
                                        		"unable to set "
                                                	"permissions on %s "
                                                	"directory!\n", incstr);
                        		exitloc(2);
                                }
                        }
                }
                if (pkg && (!KernelBuild)) {
                        args[0] = "/usr/sbin/installf";
                        args[1] = pkg;
                        args[2] = incstr;
                        args[3] = "d";
                        args[4] = "0755";
                        args[5] = "bin";
                        args[6] = "bin";
                        args[7] = NULL;
                        os_exec(NULL, args[0], args);
                }

		index = 0;
		while (inp_data[index] != NULL) {
			if (inp_data[index]->type == PROVIDES &&
					inp_data[index+3] != NULL) {
				/* Create the directory(s) */
				strcpy(tmpf, "/usr/include/udi/");
				if (*(inp_data[index+1]->string) == '%') {
					strcat(tmpf, "internal/");
					/* Also create dir, if needed */
					if (mkdir(tmpf, 0755) == -1 &&
							errno != EEXIST) {
						printloc(stderr, UDI_MSG_ERROR,
							2106, "unable to create"
							" the %s directory!\n",
							tmpf);
						exitloc(2);
					} else if (errno != EEXIST) {
						if ( !KernelBuild ) {
						    if (chown(tmpf, 2, 2) == -1)
						    {
							printloc(stderr,
								UDI_MSG_ERROR,
								2107, "unable "
								"to set "
								"permissions on"
								" %s "
								"directory!\n",
								tmpf);
							exitloc(2);
						    }
						}
					}
					if (pkg && !KernelBuild) {
						args[0] = "/usr/sbin/installf";
						args[1] = pkg;
						args[2] = tmpf;
						args[3] = "d";
						args[4] = "0755";
						args[5] = "bin";
						args[6] = "bin";
						args[7] = NULL; 
						os_exec(NULL, args[0], args);
					}
				}
				if (*(inp_data[index+1]->string) != '%') {
					strcat(tmpf, inp_data[index+1]->
						string);
				} else {
					strcat(tmpf, (inp_data[index+1]->
						string)+1);
				}
				if (mkdir(tmpf, 0755) == -1 && errno != EEXIST)
				{
					printloc(stderr, UDI_MSG_ERROR, 2106,
							"unable to create the "
							"%s directory!\n",
							tmpf);
					exitloc(2);
				} else if (errno != EEXIST) {
					if ( !KernelBuild ) {
					    if (chown(tmpf, 2, 2) == -1) {
						printloc(stderr, UDI_MSG_ERROR,
							2107, "unable to set "
							"permissions on %s "
							"directory!\n", tmpf);
						exitloc(2);
					    }
					}
				}
				if (pkg && !KernelBuild) {
					args[0] = "/usr/sbin/installf";
					args[1] = pkg;
					args[2] = tmpf;
					args[3] = "d";
					args[4] = "0755";
					args[5] = "bin";
					args[6] = "bin";
					args[7] = NULL; 
					os_exec(NULL, args[0], args);
				}
				/* Create the version */
                                strcpy(tmpf, incstr);
                                strcat(tmpf, "/");
				if (*(inp_data[index+1]->string) != '%') {
					strcat(tmpf, inp_data[index+1]->
							string);
				} else {
					strcat(tmpf, "internal/");
					strcat(tmpf, (inp_data[index+1]->
							string)+1);
				}
				sprintf(provv1, "%x",
					inp_data[index+2]->ulong/256);
				sprintf(provv2, "%02x",
					inp_data[index+2]->ulong%256);
				strcat(tmpf, "/");
				strcat(tmpf, provv1);
				strcat(tmpf, ".");
				strcat(tmpf, provv2);
				if (mkdir(tmpf, 0755) == -1 && errno != EEXIST)
				{
					printloc(stderr, UDI_MSG_ERROR, 2106,
							"unable to create the "
							"%s directory!\n",
							tmpf);
					exitloc(2);
				} else if (errno != EEXIST && !KernelBuild) {
					if (chown(tmpf, 2, 2) == -1) {
						printloc(stderr, UDI_MSG_ERROR,
							2107, "unable to set "
							"permissions on %s "
							"directory!\n", tmpf);
						exitloc(2);
					}
				}
				if (pkg && !KernelBuild) {
					args[0] = "/usr/sbin/installf";
					args[1] = pkg;
					args[2] = tmpf;
					args[3] = "d";
					args[4] = "0755";
					args[5] = "bin";
					args[6] = "bin";
					args[7] = NULL; 
					os_exec(NULL, args[0], args);
				}
				/* Install each of the header files */
				j = index+3;
				while(inp_data[j] != NULL) {
                                        strcpy(tmpf, incstr);
                                        strcat(tmpf, "/");
					if (*(inp_data[index+1]->string) !=
						'%') {
						strcat(tmpf,
							inp_data[index+1]->
							string);
					} else {
						strcat(tmpf, "internal/");
						strcat(tmpf,
							(inp_data[index+1]->
							string)+1);
					}
					sprintf(provv1, "%x",
						inp_data[index+2]->ulong/256);
					sprintf(provv2, "%02x",
						inp_data[index+2]->ulong%256);
					strcat(tmpf, "/");
					strcat(tmpf, provv1);
					strcat(tmpf, ".");
					strcat(tmpf, provv2);
					strcat(tmpf, "/");
					strcat(tmpf, inp_data[j]->string);
					/*
					 * Use cp instead of mv in case
					 * same header is provided multiple
					 * times for different versions
					 */
					optcpy(&nmstr, "cp -f ");
					optcat(&nmstr, "../../../src/");
					optcat(&nmstr, inp_data[j]->string);
					optcat(&nmstr, " ");
					optcat(&nmstr, tmpf);
					optcat(&nmstr, "; chmod 0444 ");
					optcat(&nmstr, tmpf);
					if ( !KernelBuild ) {
						optcat(&nmstr, "; chown bin:bin ");
						optcat(&nmstr, tmpf);
					}
					args[0] = "/usr/bin/sh";
					args[1] = "-c";
					args[2] = nmstr;
					args[3] = NULL;
					os_exec(NULL, args[0], args);
					if (pkg && !KernelBuild) {
						args[0] = "/usr/sbin/installf";
						args[1] = pkg;
						args[2] = tmpf;
						args[3] = "f";
						args[4] = NULL; 
						os_exec(NULL, args[0], args);
					}
					j++;
				}
			}
			while (inp_data[index] != NULL) index++;
			index++;
		}

		/*
		 * Now, run the appropriate commands.
		 */
		if (strcmp(modtype, "nic") != 0 || KernelBuild) {
			/* Install a 'normal' driver */
                        idcmdstr = optinit();
                        optcpy(&idcmdstr, confstr);
                        optcat(&idcmdstr, "/bin/idinstall");
                        idargstr = optinit();
                        optcpy(&idargstr, "-R");
                        optcat(&idargstr, confstr);
			/* Do the idinstall */
			args[0] = idcmdstr;
			args[1] = idargstr;
			strcpy(tmpf, "/etc/conf/bin/idcheck -p ");
			strcat(tmpf, name);
			i = system(tmpf);
			args[2] = (i==0||i==8?"-ak":"-uk");
			i=3;
			if (pkg && !KernelBuild) {
				args[i++] = "-P";
				args[i++] = pkg;
			}
			args[i] = name;
			i++;
			args[i] = NULL;
			i++;
			os_exec(NULL, args[0], args);
			/* If this is scsi, we need the disk.cfg file also */
			if (strcmp(modtype, "scsi") == 0) {
				args[0] = "/bin/cp";
				args[1] = "disk.cfg";
				optcpy(&idargstr, confstr);
				optcat(&idargstr, "/pack.d/");
				optcat(&idargstr, name);
				optcat(&idargstr, "/");
				optcat(&idargstr, args[1]);
				args[2] = idargstr;
				args[3] = NULL;
				i = 4;
				os_exec(NULL, args[0], args);
				if (pkg && !KernelBuild) {
					args[0] = "/usr/sbin/installf";
					args[1] = pkg;
					args[2] = idargstr;
					args[3] = "f";
					args[4] = NULL; 
					os_exec(NULL, args[0], args);
				}
			}
			free( idargstr );
			free( idcmdstr );
			/* Do the idbuild */
			if ( !KernelBuild ) {
				args[0] = "/etc/conf/bin/idbuild";
				args[1] = "-KQ";
				args[2] = "-M";
				args[3] = name;
				args[4] = NULL; 
				os_exec(NULL, args[0], args);
			}
		} else {
			/* Install a nic driver in /etc/inst/nd/mdi */
			nmstr = optinit();

			/* Install dir */
			optcpy(&nmstr, udi_filedir);
			if ((i = mkdir(nmstr, 0755)) != 0) {
				if (errno != EEXIST || (stat(nmstr,
						&filestat) != 0 ||
						(filestat.st_mode&S_IFDIR) ==
						0)) {
					printloc(stderr, UDI_MSG_FATAL, 2106,
						"unable to create the %s "
						"directory!\n", nmstr);
				}
                        }
			if (pkg) {
				args[0] = "/usr/sbin/installf";
				args[1] = pkg;
				args[2] = nmstr;
				args[3] = "d";
				args[4] = "0755";
				args[5] = "root";
				args[6] = "sys";
				args[7] = NULL; 
				os_exec(NULL, args[0], args);
			}

			for (i = 0; i < 6; i++) {
				switch (i) {
					case 0:	/* Driver.o file */
						strcpy(tmpf2, "Driver.o");
						break;
					case 1:	/* Drvmap file */
						strcpy(tmpf2, "Drvmap");
						break;
					case 2: /* Master file */
						strcpy(tmpf2, "Master");
						break;
					case 3: /* Node file */
						strcpy(tmpf2, "Node");
						break;
					case 4: /* System file */
						strcpy(tmpf2, "System");
						break;
					case 5:	/* *.bcfg file */
						strcpy(tmpf2, name);
						strcat(tmpf2, ".bcfg");
						break;
				}
				optcpy(&nmstr, "mv -f ");
				optcat(&nmstr, tmpdir);
				optcat(&nmstr, "/");
				optcat(&nmstr, tmpf2);
				optcat(&nmstr, " ");
				optcat(&nmstr, udi_filedir);
				optcat(&nmstr, "; chmod 0644 ");
				optcat(&nmstr, udi_filedir);
				optcat(&nmstr, tmpf2);
				optcat(&nmstr, "; chown root:sys ");
				optcat(&nmstr, udi_filedir);
				optcat(&nmstr, tmpf2);
				args[0] = "/usr/bin/sh";
				args[1] = "-c";
				args[2] = nmstr;
				args[3] = NULL;
				os_exec(NULL, args[0], args);
				if (pkg) {
					optcpy(&nmstr, udi_filedir);
					optcat(&nmstr, tmpf2);
					args[0] = "/usr/sbin/installf";
					args[1] = pkg;
					args[2] = nmstr;
					args[3] = "f";
					args[4] = NULL; 
					os_exec(NULL, args[0], args);
				}
			}
			free(nmstr);
		}

		/*
		 * Return to the directory we started from
		 */
		if (chdir(current_dir) != 0) {
			printloc(stderr, UDI_MSG_ERROR, 2105, "unable to "
					"change to directory %s!\n",
					current_dir);
			exitloc(2);
		}
		if (chdir("..") != 0) {
			printloc(stderr, UDI_MSG_ERROR, 2108, "unable to "
					"change to directory %s%s!\n",
					current_dir, "/..");
			exitloc(2);
		}
		if (! install_src) {
			if (chdir("..") != 0) {
				printloc(stderr, UDI_MSG_ERROR, 2108, "unable "
					"to change to directory %s%s!\n",
					current_dir, "/../..");
				exitloc(2);
			}
		}

		/* Copy any msg dir files */
		if (access("msg", X_OK|R_OK) == 0) {
				nmstr = optinit();
				optcpy(&nmstr, udi_filedir);
				optcat(&nmstr, "msg");
				/* Create a directory */
				if ((i = mkdir(nmstr, 0755)) != 0) {
					if (errno != EEXIST || (stat(nmstr,
							&filestat) != 0 ||
							(filestat.st_mode&
							S_IFDIR) == 0)) {
						printloc(stderr, UDI_MSG_FATAL,
							1402, "Unable to "
							"create packaging "
							"directory %s\n",
							nmstr);
					}
				}

				/* Copy all the files */
				optcpy(&nmstr, "for file in $(/bin/ls * 2> ");
				optcat(&nmstr, "/dev/null); do ");
				optcat(&nmstr, "cp $file ");
				optcat(&nmstr, udi_filedir);
				optcat(&nmstr, "msg ");
				optcat(&nmstr, "|| exit 1;");
				optcat(&nmstr, "chmod 0644 ");
				optcat(&nmstr, udi_filedir);
				optcat(&nmstr, "msg/");
				optcat(&nmstr, "$file;");
				if (pkg && !KernelBuild) {
					optcat(&nmstr, "chown root:sys ");
					optcat(&nmstr, udi_filedir);
					optcat(&nmstr, "msg/");
					optcat(&nmstr, "$file;");
					optcat(&nmstr, "/usr/sbin/installf ");
					optcat(&nmstr, pkg);
					optcat(&nmstr, " ");
					optcat(&nmstr, udi_filedir);
					optcat(&nmstr, "msg/");
					optcat(&nmstr, "$file f;");
				}
				optcat(&nmstr, "done; exit 0");
				args[0] = "/usr/bin/ksh";
				args[1] = "-c";
				args[2] = nmstr;
				args[3] = NULL;
				os_exec("msg", args[0], args);
				free(nmstr);
		}

		/* Copy any readable file dir files */
		if (access("rfiles", X_OK|R_OK) == 0) {
				optcpy(&nmstr, udi_filedir);
				optcat(&nmstr, "rfiles");
				/* Create a directory */
				if ((i = mkdir(nmstr, 0755)) != 0) {
					if (errno != EEXIST || (stat(nmstr,
							&filestat) != 0 ||
							(filestat.st_mode&
							S_IFDIR) == 0)) {
						printloc(stderr, UDI_MSG_FATAL,
							1402, "Unable to "
							"create packaging "
							"directory %s\n",
							nmstr);
					}
				}

				/* Copy all the files */
				optcpy(&nmstr, "for file in $(/bin/ls * 2> ");
				optcat(&nmstr, "/dev/null); do ");
				optcat(&nmstr, "cp $file ");
				optcat(&nmstr, udi_filedir);
				optcat(&nmstr, "rfiles ");
				optcat(&nmstr, "|| exit 1;");
				optcat(&nmstr, "chmod 0644 ");
				optcat(&nmstr, udi_filedir);
				optcat(&nmstr, "rfiles/");
				optcat(&nmstr, "$file;");
				if (pkg && !KernelBuild) {
					optcat(&nmstr, "chown root:sys ");
					optcat(&nmstr, udi_filedir);
					optcat(&nmstr, "rfiles/");
					optcat(&nmstr, "$file;");
					optcat(&nmstr, "/usr/sbin/installf ");
					optcat(&nmstr, pkg);
					optcat(&nmstr, " ");
					optcat(&nmstr, udi_filedir);
					optcat(&nmstr, "rfiles/");
					optcat(&nmstr, "$file f;");
				}
				optcat(&nmstr, "done; exit 0");
				args[0] = "/usr/bin/ksh";
				args[1] = "-c";
				args[2] = nmstr;
				args[3] = NULL;
				os_exec("rfiles", args[0], args);
				free(nmstr);
		}

		/* Do the final installf */
		if (pkg && !KernelBuild) {
			args[0] = "/usr/sbin/installf";
			args[1] = "-f";
			args[2] = pkg;
			args[3] = NULL; 
			os_exec(NULL, args[0], args);
		}

		/*
		 * Return to the directory we started from
		 */
		if(chdir(current_dir) != 0) {
			printloc(stderr, UDI_MSG_ERROR, 2105, "unable to "
				"change to directory %s!\n", current_dir);
			exitloc(2);
		}
		free(current_dir);
	} else if ((pkg == NULL || *pkg == 0) && ! sflg) {
		/* Install a new driver, as a UW7 package */

		tfile = optinit();

		/*
		 * Save the current directory.
		 */
		if ((cwd = (char *)getcwd(NULL, 513)) == NULL) {
			printloc(stderr, UDI_MSG_ERROR, 1503, "unable to get "
					"current working directory!\n");
			exitloc(2);
		}
		current_dir = strdup(cwd);
		free(cwd);

		/* Go to the dir with the Architecture specific files in it */
		if (chdir(tmpdir) != 0) {
			printloc(stderr, UDI_MSG_ERROR, 2105, "unable to "
					"change to directory %s!\n", tmpdir);
			exitloc(2);
		}

		/* Get the architecture */
		cptr = ((char *)current_dir)+strlen(current_dir)-1;
		while (*cptr != '/') cptr--;
		cptr++;

		/*
		 * Create the package name, which can only be 8 chars.
		 * They must be alphanumeric, or have a '+' or '-'.
		 */
		/* In theory, shortname is limited to 8 chars, but ... */
		strncpy(pname, name, 8);
		pname[8] = 0;
		for (i = 0; i < 8; i++)
			if (pname[i] == '_') pname[i] = '-';

		/* Make various package directories */
		optcpy(&tfile, "uwpkg");
                if (mkdir(tfile, 0755) == -1 && errno != EEXIST) {
			printloc(stderr, UDI_MSG_ERROR, 2106, "unable "
				"to create the %s directory!\n", tfile);
                       	exitloc(2);
		}
		optcat(&tfile, "/");
		optcat(&tfile, pname);
                if (mkdir(tfile, 0755) == -1 && errno != EEXIST) {
			printloc(stderr, UDI_MSG_ERROR, 2106, "unable "
				"to create the %s directory!\n", tfile);
                       	exitloc(2);
		}
		optcat(&tfile, "/install");
                if (mkdir(tfile, 0755) == -1 && errno != EEXIST) {
			printloc(stderr, UDI_MSG_ERROR, 2106, "unable "
					"to create the %s directory!\n", tfile);
                        exitloc(2);
		}
		optcpy(&tfile, "uwpkg/");
		optcat(&tfile, pname);
		optcat(&tfile, "/root");
                if (mkdir(tfile, 0755) == -1 && errno != EEXIST) {
			printloc(stderr, UDI_MSG_ERROR, 2106, "unable "
					"to create the %s directory!\n", tfile);
                        exitloc(2);
		}

		/* Create a pkginfo file */
		optcpy(&tfile, "uwpkg/");
		optcat(&tfile, pname);
		optcat(&tfile, "/pkginfo");
		if ((fp = fopen(tfile, "w+")) == NULL) {
			printloc(stderr, UDI_MSG_FATAL, 2103, "unable "
					"to open %s for writing!\n", tfile);
		}
		fprintf(fp, "PKG=%s\n", pname);
		fprintf(fp, "VERSION=%x.%02x\n", version/256, version%256);
		fprintf(fp, "ARCH=%s\n", cptr);
		fprintf(fp, "PSTAMP=%s.%s.%x.%x\n", name, cptr,
				udi_version, version);
		fprintf(fp, "CATEGORY=system\n");
		fprintf(fp, "CLASSES=none\n");
		fprintf(fp, "NAME=%s\n", desc);
		fprintf(fp, "DESC=%s\n", desc);
		fprintf(fp, "INSTLOG=/var/sadm/install/logs/%s.log\n", pname);
		fprintf(fp, "# Compile Options :\n");
		fprintf(fp, "#    (DEBUG is %s, OPTIMIZE is %s)\n",
				Dflg?"ON":"OFF", Oflg?"ON":"OFF");
		fclose(fp);

		/* Collect all of the files to be packaged */

		/*
		 * First, install any interface files
                 * Use UnixWare kernel build environment if present
		 */
		if (KernelBuild) {
			optcpy(&intfstr, getenv("ROOT"));
			optcat(&intfstr, "/usr/src");
			optcat(&intfstr, getenv("WORK"));
			optcat(&intfstr, "/uts");
		} else {
			optcpy(&intfstr, "uwpkg/");
			optcat(&intfstr, pname);
			optcat(&intfstr, "/root/etc/conf/interface.d");
		}
                nmstr = optinit();
                optcpy(&nmstr, "for file in $(/bin/ls *\\.intrfc 2> ");
                optcat(&nmstr, "/dev/null); do ");
		optcat(&nmstr, "[ ! -d ");
		optcat(&nmstr, intfstr);
		optcat(&nmstr, " ] ");
		optcat(&nmstr, "&& { mkdir -p ");
		optcat(&nmstr, intfstr);
		optcat(&nmstr, ";}; ");
                optcat(&nmstr, "dest_file=${file%%\\.intrfc}; ");
                optcat(&nmstr, "cp $file ");
                optcat(&nmstr, intfstr);
                optcat(&nmstr, "/$dest_file ");
                optcat(&nmstr, "|| exit 1;");
		optcat(&nmstr, "done; exit 0");
		args[0] = "/usr/bin/ksh";
		args[1] = "-c";
		args[2] = nmstr;
		args[3] = NULL;
		os_exec(NULL, args[0], args);

		/*
		 * Put any header files in uwpkg/root/usr/include/udi.
		 */
		index = 0;
		while (inp_data[index] != NULL) {
			if (inp_data[index]->type == PROVIDES &&
					inp_data[index+3] != NULL) {
				/* Create the directory(s) */
				strcpy(tmpf, "uwpkg/");
				strcat(tmpf, pname);
				strcat(tmpf, "/root/usr");
				if (mkdir(tmpf, 0755) == -1 &&
						errno != EEXIST) {
					printloc(stderr, UDI_MSG_ERROR,
						2106, "unable to create"
						" the %s directory!\n",
						tmpf);
					exitloc(2);
				}
				strcat(tmpf, "/include");
				if (mkdir(tmpf, 0755) == -1 &&
						errno != EEXIST) {
					printloc(stderr, UDI_MSG_ERROR,
						2106, "unable to create"
						" the %s directory!\n",
						tmpf);
					exitloc(2);
				}
				strcat(tmpf, "/udi");
				if (mkdir(tmpf, 0755) == -1 &&
						errno != EEXIST) {
					printloc(stderr, UDI_MSG_ERROR,
						2106, "unable to create"
						" the %s directory!\n",
						tmpf);
					exitloc(2);
				}
				strcat(tmpf, "/");

				if (*(inp_data[index+1]->string) == '%') {
					strcat(tmpf, "internal/");
					/* Create dir, if needed */
					if (mkdir(tmpf, 0755) == -1 &&
							errno != EEXIST) {
						printloc(stderr, UDI_MSG_ERROR,
							2106, "unable to create"
							" the %s directory!\n",
							tmpf);
						exitloc(2);
					}
				}
				if (*(inp_data[index+1]->string) != '%') {
					strcat(tmpf, inp_data[index+1]->
						string);
				} else {
					strcat(tmpf, (inp_data[index+1]->
						string)+1);
				}
				if (mkdir(tmpf, 0755) == -1 && errno != EEXIST)
				{
					printloc(stderr, UDI_MSG_ERROR, 2106,
							"unable to create the "
							"%s directory!\n",
							tmpf);
					exitloc(2);
				}

				/* Create the version */
				sprintf(provv1, "%x",
					inp_data[index+2]->ulong/256);
				sprintf(provv2, "%02x",
					inp_data[index+2]->ulong%256);
				strcat(tmpf, "/");
				strcat(tmpf, provv1);
				strcat(tmpf, ".");
				strcat(tmpf, provv2);
				if (mkdir(tmpf, 0755) == -1 && errno != EEXIST)
				{
					printloc(stderr, UDI_MSG_ERROR, 2106,
							"unable to create the "
							"%s directory!\n",
							tmpf);
					exitloc(2);
				}

				/* Install each of the header files */
				j = index+3;
				while(inp_data[j] != NULL) {
					strcpy(tmpf2, tmpf);
					strcat(tmpf2, "/");
					strcat(tmpf2, inp_data[j]->string);
					optcpy(&nmstr, "cp ");
					optcat(&nmstr, "../../../src/");
					optcat(&nmstr, inp_data[j]->string);
					optcat(&nmstr, " ");
					optcat(&nmstr, tmpf2);
					args[0] = "/usr/bin/sh";
					args[1] = "-c";
					args[2] = nmstr;
					args[3] = NULL;
					os_exec(NULL, args[0], args);
					j++;
				}
			}
			while (inp_data[index] != NULL) index++;
			index++;
		}

		/* Create the destination directories */
		if (strcmp(modtype, "nic") != 0) {
			/*
			 * Non-nic driver, so files will go into
			 * uwpkg/pname/root/tmp/name
			 */
			strcpy(tmpf, tmpdir);
			strcat(tmpf, "/uwpkg/");
			strcat(tmpf, pname);
			strcat(tmpf, "/root/tmp/");
			if (mkdir(tmpf, 0755) == -1 && errno != EEXIST)
			{
				printloc(stderr, UDI_MSG_ERROR, 2106, "unable "
						"to create the %s directory!\n",
						tmpf);
				exitloc(2);
			}
			strcat(tmpf, name);
			strcat(tmpf, "/");
			if (mkdir(tmpf, 0755) == -1 && errno != EEXIST)
			{
				printloc(stderr, UDI_MSG_ERROR, 2106, "unable "
						"to create the %s directory!\n",
						tmpf);
				exitloc(2);
			}
		} else {
			/*
			 * Packaging for a nic driver, so files will
			 * go into uwpkg/pname/root/etc/inst/nd/mdi/name
			 */
			strcpy(tmpf, tmpdir);
			strcat(tmpf, "/uwpkg/");
			strcat(tmpf, pname);
			strcat(tmpf, "/root/etc/");
			if (mkdir(tmpf, 0755) == -1 && errno != EEXIST)
			{
				printloc(stderr, UDI_MSG_ERROR, 2106, "unable "
						"to create the %s directory!\n",
						tmpf);
				exitloc(2);
			}
			strcat(tmpf, "inst/");
			if (mkdir(tmpf, 0755) == -1 && errno != EEXIST)
			{
				printloc(stderr, UDI_MSG_ERROR, 2106, "unable "
						"to create the %s directory!\n",
						tmpf);
				exitloc(2);
			}
			strcat(tmpf, "nd/");
			if (mkdir(tmpf, 0755) == -1 && errno != EEXIST)
			{
				printloc(stderr, UDI_MSG_ERROR, 2106, "unable "
						"to create the %s directory!\n",
						tmpf);
				exitloc(2);
			}
			strcat(tmpf, "mdi/");
			if (mkdir(tmpf, 0755) == -1 && errno != EEXIST)
			{
				printloc(stderr, UDI_MSG_ERROR, 2106, "unable "
						"to create the %s directory!\n",
						tmpf);
				exitloc(2);
			}
			strcat(tmpf, name);
			strcat(tmpf, "/");
			if (mkdir(tmpf, 0755) == -1 && errno != EEXIST)
			{
				printloc(stderr, UDI_MSG_ERROR, 2106, "unable "
						"to create the %s directory!\n",
						tmpf);
				exitloc(2);
			}
		}

		/*
		 * Now, copy the various driver files
		 */
		i = 0;
		while (drivfiles[i] != NULL) {
			if (stat (drivfiles[i], &filestat) == 0) {
				optcpy(&nmstr, "cp ");
				optcat(&nmstr, drivfiles[i]);
				optcat(&nmstr, " ");
				optcat(&nmstr, tmpf);
				args[0] = "/usr/bin/sh";
				args[1] = "-c";
				args[2] = nmstr;
				args[3] = NULL;
				os_exec(NULL, args[0], args);
			}
			i++;
		}
		/* As well as the .bcfg file */
		strcpy(tmpf2, name);
		strcat(tmpf2, ".bcfg");
		if (stat (tmpf2, &filestat) == 0) {
			optcpy(&nmstr, "cp ");
			optcat(&nmstr, tmpf2);
			optcat(&nmstr, " ");
			optcat(&nmstr, tmpf);
			args[0] = "/usr/bin/sh";
			args[1] = "-c";
			args[2] = nmstr;
			args[3] = NULL;
			os_exec(NULL, args[0], args);
		}

		/*
		 * Lastly, add any readable or msg files
		 */
		/* Return to the directory we started from */
		if (chdir(current_dir) != 0) {
			printloc(stderr, UDI_MSG_ERROR, 2105, "unable to "
					"change to directory %s!\n",
					current_dir);
			exitloc(2);
		}
		if (chdir("..") != 0) {
			printloc(stderr, UDI_MSG_ERROR, 2108, "unable to "
					"change to directory %s%s!\n",
					current_dir, "/..");
			exitloc(2);
		}
		if (! install_src) {
			if (chdir("..") != 0) {
				printloc(stderr, UDI_MSG_ERROR, 2108, "unable "
					"to change to directory %s%s!\n",
					current_dir, "/../..");
				exitloc(2);
			}
		}
		/* Copy any msg dir files */
		if (access("msg", X_OK|R_OK) == 0) {
				optcpy(&nmstr, tmpf);
				optcat(&nmstr, "/msg");
				/* Create a directory */
				if ((i = mkdir(nmstr, 0755)) != 0) {
					if (errno != EEXIST || (stat(nmstr,
							&filestat) != 0 ||
							(filestat.st_mode&
							S_IFDIR) == 0)) {
						printloc(stderr, UDI_MSG_FATAL,
							1402, "Unable to "
							"create packaging "
							"directory %s\n",
							nmstr);
					}
				}

				/* Copy all the files */
				optcpy(&nmstr, "for file in $(/bin/ls * 2> ");
				optcat(&nmstr, "/dev/null); do ");
				optcat(&nmstr, "cp $file ");
				optcat(&nmstr, tmpf);
				optcat(&nmstr, "/msg");
				optcat(&nmstr, "|| exit 1; ");
				optcat(&nmstr, "done; exit 0");
				args[0] = "/usr/bin/ksh";
				args[1] = "-c";
				args[2] = nmstr;
				args[3] = NULL;
				os_exec("msg", args[0], args);
		}

		/* Copy any aux dir files */
		if (access("rfiles", X_OK|R_OK) == 0) {
				optcpy(&nmstr, tmpf);
				optcat(&nmstr, "/rfiles");
				/* Create a directory */
				if ((i = mkdir(nmstr, 0755)) != 0) {
					if (errno != EEXIST || (stat(nmstr,
							&filestat) != 0 ||
							(filestat.st_mode&
							S_IFDIR) == 0)) {
						printloc(stderr, UDI_MSG_FATAL,
							1402, "Unable to "
							"create packaging "
							"directory %s\n",
							nmstr);
					}
				}

				/* Copy all the files */
				optcpy(&nmstr, "for file in $(/bin/ls * 2> ");
				optcat(&nmstr, "/dev/null); do ");
				optcat(&nmstr, "cp $file ");
				optcpy(&nmstr, tmpf);
				optcat(&nmstr, "/rfiles");
				optcat(&nmstr, "|| exit 1; ");
				optcat(&nmstr, "done; exit 0");
				args[0] = "/usr/bin/ksh";
				args[1] = "-c";
				args[2] = nmstr;
				args[3] = NULL;
				os_exec("rfiles", args[0], args);
		}

		if (chdir(tmpdir) != 0) {
			printloc(stderr, UDI_MSG_ERROR, 2105, "unable to "
					"change to directory %s!\n",
					tmpdir);
			exitloc(2);
		}

		/* Create a postinstall script */
		optcpy(&tfile, "uwpkg/");
		optcat(&tfile, pname);
		optcat(&tfile, "/install/postinstall");
		if ((fp = fopen(tfile, "w+")) == NULL) {
			printloc(stderr, UDI_MSG_FATAL, 2103, "unable "
					"to open %s for writing!\n", tfile);
		}
		if (strcmp(modtype, "nic") != 0) {
			strcpy(tmpf, UDI_DRVFILEDIR);
			strcat(tmpf, "/");
			strcat(tmpf, name);
		} else {
			strcpy(tmpf, UDI_NICFILEDIR);
			strcat(tmpf, "/");
			strcat(tmpf, name);
		}
		fprintf(fp, "#!/usr/bin/ksh\n");
		if (strcmp(modtype, "nic") != 0) {
			fprintf(fp, "UDI_LINK=/bin/ld\n");
			fprintf(fp, "export UDI_LINK\n");
			fprintf(fp, "cd /tmp/%s\n", name);
			fprintf(fp, "/etc/conf/bin/idcheck -p %s >/dev/null 2>&1\n", name);
			fprintf(fp, "rc=$?\n", name);
			fprintf(fp, "if [ $rc -eq 0 -o $rc -eq 8 ]\n");
			fprintf(fp, "then\n");
			fprintf(fp, "echo \"Execing idinstall -a ...\" >> $INSTLOG\n");
			fprintf(fp, "/etc/conf/bin/idinstall -ak -P %s %s >> $INSTLOG 2>&1 || exit 1\n", pname, name);
			fprintf(fp, "else\n");
			fprintf(fp, "echo \"Execing idinstall -u ...\" >> $INSTLOG\n");
			fprintf(fp, "/etc/conf/bin/idinstall -uk -P %s %s >> $INSTLOG 2>&1 || exit 1\n", pname, name);
			fprintf(fp, "fi\n");
			fprintf(fp, "/etc/conf/bin/idbuild -KQ -M %s >> $INSTLOG 2>&1 || exit 1\n", name);
                        if (strcmp(modtype, "scsi") == 0) {
				fprintf(fp, "cp disk.cfg /etc/conf/pack.d/%s >> $INSTLOG 2>&1 || exit 1\n", name);
				fprintf(fp, "chown bin:bin /etc/conf/pack.d/%s/disk.cfg >> $INSTLOG 2>&1 || exit 1\n", name);
				fprintf(fp, "installf -c none %s /etc/conf/pack.d/%s/disk.cfg >> $INSTLOG 2>&1 || exit 1\n", pname, name);
                        }

			fprintf(fp, "echo \"Running idbuild ...\"\n");
			fprintf(fp, "/etc/conf/bin/idbuild >> $INSTLOG 2>&1 || exit 1\n");
		}
		fprintf(fp, "for dir in rfiles msg\n");
		fprintf(fp, "do\n");
		fprintf(fp, "  if [ -d \"$dir\" ]\n");
		fprintf(fp, "  then\n");
		fprintf(fp, "    cd $dir\n");
		fprintf(fp, "    mkdir %s/$dir\n", tmpf);
		fprintf(fp, "    chown root:sys %s/$dir\n", tmpf);
		fprintf(fp, "    chmod 755 %s/$dir\n", tmpf);
		fprintf(fp, "    /usr/sbin/installf %s %s/$dir d 755 root sys\n"				, pname,
				tmpf);
		fprintf(fp, "    ls | while read file\n");
		fprintf(fp, "    do\n");
		fprintf(fp, "      cp $file %s/$dir\n", tmpf);
		fprintf(fp, "      chown root:sys %s/$dir\n", tmpf);
		fprintf(fp, "      chmod 644 %s/$dir\n", tmpf);
		fprintf(fp, "      /usr/sbin/installf %s %s/$dir/$file f\n",
				pname, tmpf);
		fprintf(fp, "    done\n");
		fprintf(fp, "    cd ..\n");
		fprintf(fp, "  fi\n");
		fprintf(fp, "done\n");
		fprintf(fp, "/usr/sbin/installf -f %s >> $INSTLOG 2>&1 || exit 1\n", pname);
		if (strcmp(modtype, "nic") != 0) {
			fprintf(fp, "echo \"Removing temporary files and updating the contents file ...\"\n");
			fprintf(fp, "if [ -d \"/tmp/%s\" ]\n", name);
			fprintf(fp, "then\n");
			fprintf(fp, "  cd /tmp/%s\n", name);
			fprintf(fp, "  find . -print | sed 's%%^.%%/tmp/%s%%g' | removef %s - >> $INSTLOG 2>&1\n", name, pname);
			fprintf(fp, "  removef %s /tmp\n", pname);
			fprintf(fp, "  cd /tmp\n");
			fprintf(fp, "  rm -rf /tmp/%s\n", name);
			fprintf(fp, "fi\n");
			fprintf(fp, "/usr/sbin/removef -f %s\n", pname);
			fprintf(fp, "echo \"\n *** IMPORTANT NOTICE ***\"\n");
			fprintf(fp, "echo \"\tIf installation of all the desired packages is complete,\"\n");
			fprintf(fp, "echo \"\tthe machine should be rebooted in order to\"\n");
			fprintf(fp, "echo \"\tensure sane operation. Execute the shutdown\"\n");
			fprintf(fp, "echo \"\tcommand with the appropriate options to reboot.\"\n");
		}
		fclose(fp);

		/* And a postremove script */
		if (strcmp(modtype, "nic") != 0) {
			/*
			 * Only provide a postremove for non-NIC drivers.  NIC
			 * drivers have no postremove
			 */
			optcpy(&tfile, "uwpkg/");
			optcat(&tfile, pname);
			optcat(&tfile, "/install/postremove");
			if ((fp = fopen(tfile, "w+")) == NULL) {
				printloc(stderr, UDI_MSG_FATAL, 2103, "unable "
						"to open %s for writing!\n",
						tfile);
			}
			fprintf(fp, "#!/usr/bin/ksh\n");
			fprintf(fp, "echo \"Removing %s driver ...\"\n", name);
			fprintf(fp, "/etc/conf/bin/idinstall -d -P %s %s\n",
					pname, name);
			fprintf(fp, "echo \"Running idbuild -B ...\"\n");
			fprintf(fp, "/etc/conf/bin/idbuild -B\n");
			fprintf(fp, "/usr/sbin/removef -f %s\n", pname);
			fprintf(fp, "echo \"Removal of %s is complete.  The "
					"system should now be rebooted.\"\n",
					pname);
			fclose(fp);
		}

		/*
		 * Go to the directory with the package files
		 */
		strcpy(tmpf, tmpdir);
		strcat(tmpf, "/uwpkg/");
		strcat(tmpf, pname);
		if(chdir(tmpf) != 0) {
			printloc(stderr, UDI_MSG_ERROR, 2105, "unable to "
					"change to directory %s!\n",
					tmpf);
		}

		/* Create a pkgmap file */
		if ((fp = fopen("pkgmap", "w+")) == NULL) {
			printloc(stderr, UDI_MSG_FATAL, 2103, "unable "
					"to open %s for writing!\n",
					"pkgmap");
		}
		/*
		 * Make the maximum number of blocks for the package,
		 * arbitrarily large.  We only want a single piece.
		 */
		fprintf(fp, ": 1 10000\n");
		/* And add our standard install files */
		fprintf(fp, "\n");
		fclose(fp);

		/* Now, run a script to get all the driver related files */
		optcpy(&nmstr, "find root/* -print | while read inp; do ");
		optcat(&nmstr, "file=${inp#root}; type=0; ");
		optcat(&nmstr, "[ $file = \"/tmp\" ] && type=1; ");
		optcat(&nmstr, "[ $file = \"/etc\" ] && type=1; ");
		optcat(&nmstr, "[ $file = \"/etc/inst\" ] && type=1; ");
		optcat(&nmstr, "[ $file = \"/etc/inst/nd\" ] && type=1; ");
		optcat(&nmstr, "[ $file = \"/etc/inst/nd/mdi\" ] && type=1; ");
		optcat(&nmstr, "if [ $type -eq 1 ]; then ");
		optcat(&nmstr, "echo \"1 d none $file ? ? ? ? ? ?\" >> pkgmap; ");
		optcat(&nmstr, "else ");
		optcat(&nmstr, "if [ -d \"root/$file\" ]; then ");
		optcat(&nmstr, "echo \"1 d none $file 0755 root sys 1 NULL NULL\" >> pkgmap; ");
		optcat(&nmstr, "else ");
		optcat(&nmstr, "echo \"1 f none $file 0644 root sys \\c\" >> pkgmap; ");
		optcat(&nmstr, "echo \"`ls -l root/$file | sed 's/ [ ]*/ /g' | cut -d' ' -f5` \\c\" >> pkgmap; ");
		optcat(&nmstr, "echo \"`sum root/$file | cut -d' ' -f1` \\c\" >> pkgmap; ");
		optcat(&nmstr, "echo \"");
		sprintf(tmpf2, "%u", time((time_t)0));
		optcat(&nmstr, tmpf2);
		optcat(&nmstr, " \\c\" >> pkgmap;");
		optcat(&nmstr, "echo \"1 NULL NULL\" >> pkgmap; ");
		optcat(&nmstr, "fi; ");
		optcat(&nmstr, "fi; ");
		optcat(&nmstr, "done");
		args[0] = "/usr/bin/ksh";
		args[1] = "-c";
		args[2] = nmstr;
		args[3] = NULL;
		os_exec(NULL, args[0], args);

		/* And run a script to get all the pkg install related files */
		optcpy(&nmstr, "ls -d * install/* | while read inp; do ");
		optcat(&nmstr, "file=${inp#install/}; ");
		optcat(&nmstr, "if [ ! -d \"$inp\" -a \"$inp\" != \"pkgmap\" ]; then ");
		optcat(&nmstr, "echo \"1 i $file \\c\" >> pkgmap; ");
		optcat(&nmstr, "echo \"`ls -l $inp | sed 's/ [ ]*/ /g' | cut -d' ' -f5` \\c\" >> pkgmap; ");
		optcat(&nmstr, "echo \"`sum $inp | cut -d' ' -f1` \\c\" >> pkgmap; ");
		optcat(&nmstr, "echo \"");
		sprintf(tmpf2, "%u", time((time_t)0));
		optcat(&nmstr, tmpf2);
		optcat(&nmstr, "\" >> pkgmap;");
		optcat(&nmstr, "fi; ");
		optcat(&nmstr, "done");
		args[0] = "/usr/bin/ksh";
		args[1] = "-c";
		args[2] = nmstr;
		args[3] = NULL;
		os_exec(NULL, args[0], args);

		/* Finally, do the pkgadd */
		optcpy(&nmstr, "pkgadd -nq -d ");
		optcat(&nmstr, tmpdir);
		optcat(&nmstr, "/uwpkg ");
		optcat(&nmstr, pname);
		args[0] = "/usr/bin/ksh";
		args[1] = "-c";
		args[2] = nmstr;
		args[3] = NULL;
		os_exec(NULL, args[0], args);

		/*
		 * Return to the directory we started from
		 */
		if(chdir(current_dir) != 0) {
			printloc(stderr, UDI_MSG_ERROR, 2105, "unable to "
				"change to directory %s!\n", current_dir);
			exitloc(2);
		}
		free(tfile);
		free(nmstr);
		free(current_dir);
	}

	chdir(wherewasi);
	free(wherewasi);
}

udi_boolean_t
make_glue (char *tmpf, char *objfile)
{
	FILE *fp;
	char *startup_info = IS_META ? "udi_meta_info" : "udi_init_info";
	udi_boolean_t glue_made = FALSE;
	

	if ((fp = fopen(tmpf, "w+")) == NULL) {
		printloc(stderr, UDI_MSG_FATAL, 2103, "unable to open %s for "
				"writing!\n", tmpf);
	}

	if (installing_on_uw711) {
		char cmdbuf[1024];
		/*
		 * This is a crutch to support systems without the UDI-aware
		 * kernels.  Kernels with that support do not need this code.
		 */

		/*
		 * We peek into the object file.   If the file already
		 * has a symbol named _load, we don't create glue.  This
		 * is so mappers can provide their own _loads that calls
		 * drv_attach in a timely manner.
		 *
		 * nm isn't available in an ISL environment, but that's OK
		 * becuase we don't support that in UnixWare 7 with this 
		 * code anyway.
		 */
		snprintf(cmdbuf, sizeof(cmdbuf), 
			 "nm %s | grep -q '|_load$'", objfile);

		if (IS_MAPPER && (0 == system(cmdbuf))) {
			goto done;
		}

		fprintf(fp, "extern char *_udi_sprops_scn_for_%s;\n", name);
		fprintf(fp, "extern char *_udi_sprops_scn_end_for_%s;\n", name);
		fprintf(fp, "extern void *%s;\n", startup_info);
		fprintf(fp, "static void *my_module_handle;\n");
		fprintf(fp, "void * _udi_module_load(void *, char *, int);\n");
		fprintf(fp, "int _udi_module_unload(void *);\n");
		fprintf(fp, "int _load(void) {\n");
		fprintf(fp, "\tint sprops_size = (int) &_udi_sprops_scn_end_for_%s -"
				"(int) &_udi_sprops_scn_for_%s;\n", name, name);
		fprintf(fp, "\tmy_module_handle = (void *) _udi_module_load(&%s, "
				"(char *)&_udi_sprops_scn_for_%s, "
				"sprops_size);\n", startup_info, name );
		fprintf(fp, "\treturn my_module_handle == 0;\n");
		fprintf(fp, "}\n");

		/*
		 * Add a 'udi_meta_info' with weak binding for the 
	 	 * library case.   Will look like a meta but udi_meta_info
		 * will have a value of zero.
		 */
		fprintf(fp, "#pragma weak udi_meta_info\n");
		fprintf(fp, "void *udi_meta_info;\n");

		fprintf(fp, "int _unload(void) {\n");
		fprintf(fp, "\treturn _udi_module_unload(my_module_handle);\n");
		fprintf(fp, "}\n");
		glue_made = TRUE;
	}

done:
	fclose(fp);
	return(glue_made);
	
}
