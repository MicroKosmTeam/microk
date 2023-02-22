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
 *  $
 */
/*
 * Written for SCO by: Rob Tarte
 * At: Pacific CodeWorks, Inc.
 * Contact: http://www.pacificcodeworks.com
 */


#define wrapper (&udi_global.make_mapper)
#define input_data udi_global.propdata
#define exitloc    udi_tool_exit

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
#include "obj_osdep.h"		/* Common utility functions */

#define UDI_VERSION 0x101
#include <udi.h>

#define NUM_FIND 14	/* Things for sprops to loop through and find */

typedef enum {
	attr_name_off = 1,
	scope_off = 2,
	msgnum1_off = 3,
	msgnum2_off = 4,
	msgnum3_off = 5,
	choices_off = 6
} custom_offsets_t;

#define FOUND_VENDOR_ID			0x01
#define FOUND_DEVICE_ID			0x02
#define FOUND_SUBSYSTEM_VENDOR_ID	0x04
#define FOUND_SUBSYSTEM_ID		0x08

struct selection_list {
	struct selection_list	*next;
	struct selection_list	*prev;
	char			*list_name;
	char			*default_value;
	unsigned		type;
};

struct id_list {
	struct id_list	*next;
	unsigned	vendor_id;
	unsigned	device_id;
};

struct mkinst_info {
	char		*name;
	char		*shortname;
	char		*module;
	char		*modtype;
	int		ipl;
	char		*tmpdir;
	int		havehw;
	int		install_src;
	propline_t	**inp_data;
        char    	*confstr;
	char		*desc;
};

struct selection_list	advanced_list;
struct selection_list	basic_list;

int			pv_count;
int			desc_index;
int			version;
int			pioserlim;
char			*udi_filedir;


extern char		*optinit();

void make_glue(char *tmpf, struct mkinst_info *);
void output_net_space_dot_c(FILE *, char *);
void output_net_space_dot_h(FILE *, char *);
void output_lkcfg(FILE *);



unsigned
find_device_id(
struct id_list	*id_listp,
unsigned	vendor_id,
unsigned	device_id
)
{
	while (id_listp != NULL)
	{
		if ((id_listp->vendor_id == vendor_id) &&
			(id_listp->device_id == device_id))
		{
			return 1;
		}

		id_listp = id_listp->next;
	}

	return 0;
}

struct id_list *
generate_ID_list()
{
	struct id_list	*id_listp;
	struct id_list	*id_list_head;
	unsigned	pci_vendor_id;
	unsigned	pci_device_id;
	unsigned	pci_subsystem_vendor_id;
	unsigned	pci_subsystem_id;
	unsigned	search_flag;
	int		i;
	int		index;

	id_list_head = NULL;

	index = get_prop_type(DEVICE, 0);

	while (index >= 0)
	{
		pci_vendor_id = 0;
		pci_device_id = 0;
		pci_subsystem_vendor_id = 0;
		pci_subsystem_id = 0;

		search_flag = 0;

		for (i = index + 3 ; input_data[i] != NULL ; i += 3)
		{
			if (strcmp(input_data[i]->string, "bus_type") == 0)
			{
				if (strcmp(input_data[i+2]->string, "system")
					== 0)
				{
					return NULL;
				}
			}

			if (strcmp(input_data[i]->string, "pci_vendor_id") == 0)
			{
				pci_vendor_id = strtoul(input_data[i+2]->string,
						NULL, 0);

				search_flag |= FOUND_VENDOR_ID;
			}

			if (strcmp(input_data[i]->string, "pci_device_id") == 0)
			{
				pci_device_id = strtoul(input_data[i+2]->string,
						NULL, 0);

				search_flag |= FOUND_DEVICE_ID;
			}

			if (strcmp(input_data[i]->string,
				"pci_subsystem_vendor_id") == 0)
			{
				pci_subsystem_vendor_id =
					strtoul(input_data[i+2]->string,
						NULL, 0);

				search_flag |= FOUND_SUBSYSTEM_VENDOR_ID;
			}

			if (strcmp(input_data[i]->string,
				"pci_subsystem_id") == 0)
			{
				pci_subsystem_id =
					strtoul(input_data[i+2]->string,
						NULL, 0);

				search_flag |= FOUND_SUBSYSTEM_ID;
			}
		}

		/*
		 * We didn't find anything.  Go on to the next.
		 */
		if (search_flag == 0)
		{
			continue;
		}

		if ((search_flag & (FOUND_VENDOR_ID | FOUND_DEVICE_ID)) ==
			(FOUND_VENDOR_ID | FOUND_DEVICE_ID))
		{
			if (find_device_id(id_list_head, pci_vendor_id,
				pci_device_id) == 0)
			{
				id_listp = (struct id_list *)malloc(
					sizeof(struct id_list));

				id_listp->vendor_id = pci_vendor_id;

				id_listp->device_id = pci_device_id;

				id_listp->next = id_list_head;

				id_list_head = id_listp;
			}
		}

		if ((search_flag & 
			(FOUND_SUBSYSTEM_VENDOR_ID | FOUND_SUBSYSTEM_ID)) ==
			(FOUND_SUBSYSTEM_VENDOR_ID | FOUND_SUBSYSTEM_ID))
		{
			if (find_device_id(id_list_head,
				pci_subsystem_vendor_id, pci_subsystem_id) == 0)
			{
				id_listp = (struct id_list *)malloc(
					sizeof(struct id_list));

				id_listp->vendor_id = pci_subsystem_vendor_id;

				id_listp->device_id = pci_subsystem_id;

				id_listp->next = id_list_head;

				id_list_head = id_listp;
			}
		}

		index = get_prop_type(DEVICE, index+1);
	}

	return id_list_head;
}

void
destroy_id_list(
struct id_list	*id_listp
)
{
	struct id_list	*id_listp_next;

	while (id_listp != NULL)
	{
		id_listp_next = id_listp->next;

		free(id_listp);

		id_listp = id_listp_next;
	}
}

/*
 * Support routines for creating AOF entries to match the custom device entries
 */
struct selection_list *
make_AOF_head(
int	index
)
{
	struct selection_list	*slistp;
        char			*basic; 

	basic = optinit();

	get_message(&basic, input_data[index+msgnum3_off]->ulong, NULL);

	slistp = (struct selection_list *)malloc(sizeof(struct selection_list));

	if (slistp == NULL)
	{
		fprintf(stderr, "Error: Unable to alloc select_list\n");

		exit(1);
	}

	if (!strcmp(basic, "Basic"))
	{
		slistp->prev = basic_list.prev;
		slistp->next = &basic_list;

		basic_list.prev->next = slistp;
		basic_list.prev = slistp;
	}

	if (!strcmp(basic, "Advanced"))
	{
		slistp->prev = advanced_list.prev;
		slistp->next = &advanced_list;

		advanced_list.prev->next = slistp;
		advanced_list.prev = slistp;

	}

	slistp->default_value = NULL;

	slistp->list_name =
		strdup(input_data[index + attr_name_off]->string + 1);

	free(basic);

	return slistp;
}

void
make_AOF_common(
FILE	*fp,
int	index
)
{
        char		*prompt; 

	prompt = optinit();

	get_message(&prompt, input_data[index+msgnum2_off]->ulong, NULL);
	
	fprintf(fp, "\tPROMPT=\"%s\"\n", prompt);
	fprintf(fp, "\tTYPE=link-kit\n\n");

	free(prompt);
}

void
make_AOF_mutex(
FILE			*fp,
int			index,
struct selection_list	*slistp
)
{
	int		i;
	int		type;
	char		*msg;
	int		first;
	char		buffer[14];

	msg = optinit();

	/*
	 * Handle 'mutex' custom parameters. This can be either a string or
	 * ubit32 type value.
	 */
	if (strcmp(input_data[index+6]->string, "string") != 0)
	{
		type = 1;

		sprintf(buffer, "%d", input_data[index+7]->ulong);

		slistp->default_value = strdup(buffer);
	}
	else
	{
		type = 0;

		get_message(&msg, input_data[index+7]->ulong, NULL);

		slistp->default_value = strdup(msg);
	}


	fprintf(fp, "\tDEFAULTS=%s\n", slistp->default_value);

	fprintf(fp, "\tVALUES=");

	i = index + 9;

	first = 0;

	while (strcmp(input_data[i]->string, "end") != 0)
	{
		if (first)
		{
			fprintf(fp, ",");
		}

		if (type)
		{
			fprintf(fp, "%d", input_data[i]->ulong);
		}
		else
		{
			get_message(&msg, input_data[i]->ulong, NULL);

			fprintf(fp, "%s", msg);
		}

		first = 1;

		i++;
	}

	fprintf(fp, "\n");

	free(msg);
}

/*
 * Handle ranges and values. Only valid for ubit32 type choices.
 * We use an extension to the BCFG format: __RANGE__ which requires PTFxxxyyy
 * to be installed on the system.
 * Maybe we could determine this at run time and then produce either the new
 * fully featured BCFG, or the old incomplete version ??
 */
void
make_AOF_range(
FILE	*fp,
int	index
)
{
	udi_ubit32_t	min, max, dflt, stride;
	udi_ubit32_t	i;

	/* LINE 2 Range definition [default min max stride] */
	dflt   = input_data[index+7]->ulong;
	min    = input_data[index+9]->ulong;
	max    = input_data[index+10]->ulong;
	stride = input_data[index+11]->ulong;

	fprintf(fp, "\tDEFAULTS=%d\n", dflt);

	fprintf(fp, "\tVALUES=");

	for (i = min ; i < max ; i += stride)
	{
		fprintf(fp, "%d", i);

		i += stride;

		if (i < max)
		{
			fprintf(fp, ",");
		}
	}

	fprintf(fp, "\n");
}

void
make_AOF_any(
FILE	*fp,
int	index
)
{
	/*
	 * AOF's can't do this.  Not supported.
	 */
}

void
make_AOF_unique(
FILE	*fp,
int	index
)
{
	fprintf(fp, "\tDEFAULTS=%s\n", input_data[index+7]->string);

	fprintf(fp, "\tVALUES=%s\n", input_data[index+7]->string);
}

void
make_AOF_ubit32(
FILE			*fp,
int			index,
struct selection_list	*slistp
)
{
	char			*type;

	type = input_data[index+8]->string;

	if (!strcmp(type, "mutex"))
	{
		make_AOF_mutex(fp, index, slistp);
	}
	else if (!strcmp(type, "range"))
	{
		make_AOF_range(fp, index);
	}
	else if (!strcmp(type, "any"))
	{
		make_AOF_any(fp, index);
	}
	else if (!strcmp(type, "only"))
	{
		make_AOF_unique(fp, index);
	}
	else
	{
		printloc(stderr, UDI_MSG_FATAL, 2100, "Illegal choice value "
			"specified '%s'\n", type);
	}

	make_AOF_common(fp, index);
}

void
make_AOF_boolean(
FILE			*fp,
int			index,
struct selection_list	*slistp
)
{
	fprintf(fp, "\tDEFAULTS=%s\n", input_data[index+7]->string);

	fprintf(fp, "\tVALUES=T,F\n");

	make_AOF_common(fp, index);
}

void
make_AOF_string(
FILE			*fp,
int			index,
struct selection_list	*slistp
)
{
	char			*type;

	type = input_data[index+8]->string;

	if (!strcmp(type, "mutex"))
	{
		make_AOF_mutex(fp, index, slistp);
	}
	else if (!strcmp(type, "any"))
	{
		make_AOF_any(fp, index);
	}
	else if (!strcmp(type, "only"))
	{
	        char *only;

		only = optinit();

		get_message(&only, input_data[index+7]->ulong, NULL);

		fprintf(fp, "\tVALUES=");

		/*
		 * Line 3 [List of choices]
		 */
		fprintf(fp, "\t%s\n", only);

		free(only);
	}
	else
	{
		printloc(stderr, UDI_MSG_FATAL, 2101, "Illegal choice "
			"specified '%s'\n", type);
	}

	make_AOF_common(fp, index);
}

void
make_AOF_custom(
FILE	*fp
)
{
	int			custom_num;
	int			index;
	int			type;
	char			*stype;
	struct selection_list	*slistp;

	custom_num = 0;

	index = get_prop_type(CUSTOM, 0);

	while (index >= 0)
	{
		if (strncmp(input_data[index+attr_name_off]->string, "%", 1) 
			!= 0)
		{
			index = get_prop_type(CUSTOM, index + 1);

			continue;
		}

		stype = input_data[index+6]->string;

		custom_num++;

		if (!strcmp(stype, "ubit32"))
		{
			type = UDI_ATTR_UBIT32;
		}
		else if (!strcmp(stype, "boolean"))
		{
			type = UDI_ATTR_BOOLEAN;
		}
		else if (!strcmp(stype, "string"))
		{
			type = UDI_ATTR_STRING;
		}
		else
		{
			/* could be an array */
			/* error */
			printloc(stderr, UDI_MSG_FATAL, 2102,
				"Unsupported custom device type '%s'\n",
				stype);
		}

		slistp = make_AOF_head(index);

		fprintf(fp, "%s:\n", slistp->list_name);

		slistp->type = type;

		switch (type)
		{
		case UDI_ATTR_UBIT32:
			/* ubit32 */
			make_AOF_ubit32(fp, index, slistp);
			break;

		case UDI_ATTR_BOOLEAN:
			/* boolean */
			make_AOF_boolean(fp, index, slistp);
			break;

		case UDI_ATTR_STRING:
			/* string */
			make_AOF_string(fp, index, slistp);
			break;

		}

		index = get_prop_type(CUSTOM, index + 1);
	}

	fprintf(fp, "\n");
}

void
make_AOF(
struct mkinst_info	*mkinst_infop,
FILE			*fp
)
{
	int			i;
	struct id_list		*id_listp;
	struct id_list		*id_list_head;
	struct selection_list	*slistp;

	advanced_list.next = &advanced_list;
	advanced_list.prev = &advanced_list;

	basic_list.next = &basic_list;
	basic_list.prev = &basic_list;

	fprintf(fp, "#\n");

	/*
	 * PCI_BUS: ---------------------------------------
	 */
	fprintf(fp, "PCI_BUS:\n");
	fprintf(fp, "\tPROMPT=\"PCI Bus#\"\n");
	fprintf(fp, "\tVALUES=0");
	for (i = 1 ; i < 256 ; i++)
	{
		fprintf(fp, ",%d", i);
	}
	fprintf(fp, "\n");
	fprintf(fp, "\tTYPE=link-kit\n\n");

	/*
	 * PCI_DEV: ---------------------------------------
	 */
	fprintf(fp, "PCI_DEV:\n");
	fprintf(fp, "\tPROMPT=\"Device#\"\n");
	fprintf(fp, "\tVALUES=0");
	for (i = 1 ; i < 32 ; i++)
	{
		fprintf(fp, ",%d", i);
	}
	fprintf(fp, "\n");
	fprintf(fp, "\tTYPE=link-kit\n\n");

	/*
	 * PCI_FUNC: ---------------------------------------
	 */
	fprintf(fp, "PCI_FUNC:\n");
	fprintf(fp, "\tPROMPT=\"Function#\"\n");
	fprintf(fp, "\tVALUES=0");
	for (i = 1 ; i < 8 ; i++)
	{
		fprintf(fp, ",%d", i);
	}
	fprintf(fp, "\n");
	fprintf(fp, "\tTYPE=link-kit\n\n");

	/*
	 * custom: ----------------------------------------
	 */
	make_AOF_custom(fp);

	/*
	 * ADAPTER: ---------------------------------------
	 */
	fprintf(fp, "ADAPTER:\n");
	fprintf(fp, "\tDESCRIPTION=\"%s\"\n", mkinst_infop->desc);
	fprintf(fp, "\tBUS=PCI\n");
	fprintf(fp, "\tMEDIA_TYPE=ethernet\n");
	fprintf(fp, "\tMAX_BD=3\n");
	fprintf(fp, "\tKEY=PCI_BUS,PCI_DEV,PCI_FUNC\n");

	fprintf(fp, "\tREQUIRED=");

	for (slistp = basic_list.next ; slistp != &basic_list ; )
	{
		fprintf(fp, "%s", slistp->list_name);

		slistp = slistp->next;

		if (slistp != &basic_list)
		{
			fprintf(fp, ",");
		}
	}

	fprintf(fp, "\n");

	fprintf(fp, "\tADVANCED=");

	for (slistp = advanced_list.next ; slistp != &advanced_list ; )
	{
		fprintf(fp, "%s", slistp->list_name);

		slistp = slistp->next;

		if (slistp != &advanced_list)
		{
			fprintf(fp, ",");
		}
	}

	fprintf(fp, "\n");

	fprintf(fp, "\tSPEED=fast\n");

	/*
	 * Generate a list of vendor_id/device_id or subystem_vendor_id/
	 * subsystem_id's that we will add to the ADAPTER: section.
	 */
	id_list_head = generate_ID_list();

	/*
	 * Output the ID= string 
	 */
	fprintf(fp, "\tID=");

	for (id_listp = id_list_head ; id_listp != NULL ; )
	{
		fprintf(fp, "%04x%04x",
			id_listp->vendor_id, id_listp->device_id);

		id_listp = id_listp->next;

		if (id_listp == NULL)
		{
			fprintf(fp, "\n");
		}
		else
		{
			fprintf(fp, ",");
		}
	}

	for (id_listp = id_list_head ; id_listp != NULL ; )
	{
		fprintf(fp, "\t%04x%04x=\"%s\"\n",
			id_listp->vendor_id, id_listp->device_id,
				mkinst_infop->desc);

		id_listp = id_listp->next;
	}

	destroy_id_list(id_list_head);
}

void
get_mod_type(char *modtype)
{
	int index, index2;

	index = 0;
	while (input_data[index] != NULL)
	{
		if (input_data[index]->type != CHLDBOPS)
		{
			while (input_data[index] != NULL)
			{
				index++;
			}

			index++;
			continue;
		}

		index2 = 0;

		while (input_data[index2] != NULL)
		{
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

			while (input_data[index2] != NULL)
			{
				index2++;
			}

			index2++;
		}

		while (input_data[index] != NULL)
		{
			index++;
		}

		index++;
	}
}

void
create_directory(
char	*dirname
)
{
	int		i;
	struct stat	filestat;

	i = mkdir(dirname, 0755);

	if (i != 0)
	{
		if (errno != EEXIST || (stat(dirname, &filestat) != 0 ||
				(filestat.st_mode&S_IFDIR) == 0))
		{
			fprintf(stderr, "Error: Unable to create directory "
				"%s\n", dirname);

			exit(1);
		}
	}
}

void
create_master_file(
struct mkinst_info	*mkinst_infop
)
{
	FILE	*fp;
	char	tmpf[513];

	/*
	 * Create the Master file
	 */
	strcpy(tmpf, mkinst_infop->tmpdir);

	strcat(tmpf, "/usr");
	create_directory(tmpf);
	strcat(tmpf, "/lib");
	create_directory(tmpf);
	strcat(tmpf, "/udirt");
	create_directory(tmpf);
	strcat(tmpf, "/ID/");
	create_directory(tmpf);
	strcat(tmpf, mkinst_infop->name);
	create_directory(tmpf);

	strcat(tmpf, "/Master");

	fp = fopen(tmpf, "w+");

	if (fp == NULL)
	{
		fprintf(stderr, "Error: unable to open %s for writing!\n",
				tmpf);

		exit(1);
	}

	/*
	 * The udi driver has a halt routine.
	 */
	if (strcmp(mkinst_infop->name, "udi") == 0)
	{
		fprintf(fp, "%.8s\tIh\tBi\t%5.5s\t0\t0\t0\t1\t-1\n",
			mkinst_infop->name, mkinst_infop->shortname);

		fclose(fp);

		return;
	}

	/*
	 * NIC drivers have character interfaces also
	 */
	if (strcmp(mkinst_infop->modtype, "nic") == 0)
	{
		fprintf(fp, "%.8s\tI\tBic\t%5.5s\t0\t0\t0\t1\t-1\n",
			mkinst_infop->name, mkinst_infop->shortname);

		fclose(fp);

		return;
	}

	/*
	 * SCSI drivers have extra _entry & intr routines
	 */
	if (strcmp(mkinst_infop->modtype, "scsi") == 0)
	{
		fprintf(fp, "%.8s\tI\tBih\t%5.5s\t0\t0\t0\t1\t-1\n",
			mkinst_infop->name, mkinst_infop->shortname);

		fclose(fp);

		return;
	}

	if (*wrapper != 0 || pv_count)
	{
		/*
		 * It is a mapper or meta
		 */
		fprintf(fp, "%.8s\tI\tBi\t%5.5s\t0\t0\t1\t1\t-1\n",
			mkinst_infop->name, mkinst_infop->shortname);

		fclose(fp);

		return;
	}

	if (mkinst_infop->havehw)
	{

		if (strcmp(mkinst_infop->modtype, "misc") == 0)
		{
			fprintf(fp, "%.8s\tI\tBHic\t%5.5s\t0\t0\t1\t1\t-1\n",
				mkinst_infop->name, mkinst_infop->shortname);
		}
		else
		{
			fprintf(fp, "%.8s\tI\tBi\t%5.5s\t0\t0\t1\t1\t-1\n",
				mkinst_infop->name, mkinst_infop->shortname);
		}
	}
	else
	{
		/*
		 * It is a pseudo driver
		 */
		fprintf(fp, "%.8s\tI\tBi\t%5.5s\t0\t0\t1\t1\t-1\n",
			mkinst_infop->name, mkinst_infop->shortname);
	}

	fclose(fp);

}

void
create_system_file(
struct mkinst_info	*mkinst_infop
)
{
	FILE	*fp;
	char	tmpf[513];

	/*
	 * Create the System file
	 */
	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/System");

	fp = fopen(tmpf, "w+");

	if (fp == NULL)
	{
		fprintf(stderr, "Error: unable to open %s for writing!\n",
				tmpf);
		exit(1);
	}

	if (strcmp(mkinst_infop->modtype, "nic") == 0)
	{
		fprintf(fp, "%.8s\tN\t0\t0\t0\t0\t0\t0\t0\t0\n",
			mkinst_infop->name);

		fprintf(fp, "%.8s\tN\t0\t0\t0\t0\t0\t0\t0\t0\n",
			mkinst_infop->name);

		fprintf(fp, "%.8s\tN\t0\t0\t0\t0\t0\t0\t0\t0\n",
			mkinst_infop->name);

		fprintf(fp, "%.8s\tN\t0\t0\t0\t0\t0\t0\t0\t0\n",
			mkinst_infop->name);
	}
	else
	{
		fprintf(fp, "%.8s\tY\t1\t%d\t0\t0\t0\t0\t0\t0\n",
			mkinst_infop->name, mkinst_infop->ipl);
	}

	fclose(fp);
}

void
create_node_file(
struct mkinst_info	*mkinst_infop
)
{
	FILE		*fp;
	char		tmpf[513];
	int		i;

	/*
	 * Create the Node file
	 */
	if (strcmp(mkinst_infop->modtype, "nic") != 0 &&
		strcmp(mkinst_infop->modtype, "scsi") != 0 &&
		strcmp(mkinst_infop->modtype, "misc") != 0)
	{
		return;
	}

	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/Node");

	fp = fopen(tmpf, "w+");

	if (fp == NULL)
	{
		fprintf(stderr, "Error: unable to open %s for writing!\n",
			tmpf);

		exit(1);
	}

	if (strcmp(mkinst_infop->modtype, "nic") == 0)
	{
		for (i = 0 ; i < 4 ; i++)
		{
			fprintf(fp, "%s\tmdi/%s%d\tc\t%d\troot\troot\t600\n",
				mkinst_infop->name, mkinst_infop->name, i, i);
		}
	}
	else if (strcmp(mkinst_infop->modtype, "misc") == 0)
	{
		fprintf(fp, "%s\t%s%d\tc\t%d\troot\troot\t600\n",
			mkinst_infop->name, mkinst_infop->name, i, i);
	}

	fclose(fp);
}

void
create_AOF(
struct mkinst_info	*mkinst_infop
)
{
	FILE		*fp;
	char		tmpf[513];
	int		i;
	struct stat	filestat;

	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/AOF");

	i = mkdir(tmpf, 0755);

	if (i != 0)
	{
		if (errno != EEXIST || (stat(tmpf, &filestat) != 0 ||
				(filestat.st_mode&S_IFDIR) == 0))
		{
			fprintf(stderr, "Error: Unable to "
				"create directory "
				"%s\n", tmpf);

			exit(1);
		}
	}

	strcat(tmpf, "/");
	strcat(tmpf, mkinst_infop->name);

	fp = fopen(tmpf, "w+");

	if (fp == NULL)
	{
		fprintf(stderr, "Error: unable to open %s for ", tmpf);

		fprintf(stderr, "writing!\n");
	}

	make_AOF(mkinst_infop, fp);

	fclose(fp);

	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/lkcfg");

	fp = fopen(tmpf, "w+");

	if (fp == NULL)
	{
		fprintf(stderr, "Error: unable to open %s for writing!\n",
				tmpf);
		exit(1);
	}

	output_lkcfg(fp);

	fclose(fp);

	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/space.h");

	fp = fopen(tmpf, "w+");

	if (fp == NULL)
	{
		fprintf(stderr, "Error: unable to open %s for writing!\n",
				tmpf);
		exit(1);
	}

	output_net_space_dot_h(fp, mkinst_infop->name);

	fclose(fp);

	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/Space.c");

	fp = fopen(tmpf, "w+");

	if (fp == NULL)
	{
		fprintf(stderr, "Error: unable to open %s for writing!\n",
				tmpf);
		exit(1);
	}

	output_net_space_dot_c(fp, mkinst_infop->name);

	fclose(fp);
}

void
output_scsi_space_dot_c(
struct mkinst_info	*mkinst_infop
)
{
	FILE		*fp;
	char		tmpf[513];

	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/Space.c");

	fp = fopen(tmpf, "w+");

	if (fp == NULL)
	{
		fprintf(stderr, "Error: unable to open %s for writing!\n",
				tmpf);
		exit(1);
	}

	fprintf(fp, "int %s_entry(reqp)\n", mkinst_infop->shortname);
	fprintf(fp, "int reqp;\n");
	fprintf(fp, "{\n\treturn uscsi_entry(\"%s\", reqp);\n}\n\n",
		mkinst_infop->name);
	fprintf(fp, "int %sintr() { }\n", mkinst_infop->shortname);

	fclose(fp);
}


void
create_driver_file(
struct mkinst_info	*mkinst_infop
)
{
	FILE		*fp;
	char		*args[20];
	char		tmpf[513];
	char		tmpf2[513];
	char		tmpf3[513];
	char		*current_dir;

	/*
	 * Create the udi_glue.c and udi_glue.o stubs
	 */
	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/udi_glue.c");

	make_glue(tmpf, mkinst_infop);

	if (strcmp(mkinst_infop->name, "udi_nic") == 0)
	{
		strcpy(tmpf, mkinst_infop->tmpdir);
		strcat(tmpf, "/usr/lib/udirt/ID/");
		strcat(tmpf, mkinst_infop->name);
		strcat(tmpf, "/Space.c");

		fp = fopen(tmpf, "w+");

		if (fp == NULL)
		{
			fprintf(stderr,
				"Error: unable to open %s for writing!\n",
					tmpf);
			exit(1);
		}

		fprintf(fp, "#include \"config.h\"\n\n");
		fprintf(fp, "#if ");
		fprintf(fp, "!defined(NET0_0) ");
		fprintf(fp, "&& !defined(NET1_0) ");
		fprintf(fp, "&& !defined(NET2_0) ");
		fprintf(fp, "&& !defined(NET3_0)\n");
		fprintf(fp, "mdi_macokack()\n{\n}\n\n");
		fprintf(fp, "mdi_macerrorack()\n{\n}\n\n");
		fprintf(fp, "mdi_do_loopback()\n{\n}\n\n");
		fprintf(fp, "mdi_addrs_equal()\n{\n}\n\n");
		fprintf(fp, "#endif\n");

		fclose(fp);
	}

	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/Driver.o.elf");

	os_copy(tmpf, mkinst_infop->inp_data[module_ent+1]->string);

	/*
	 * Compile the glue code
	 */
	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/udi_glue.c");

	strcpy(tmpf2, mkinst_infop->tmpdir);
	strcat(tmpf2, "/usr/lib/udirt/ID/");
	strcat(tmpf2, mkinst_infop->name);
	strcat(tmpf2, "/udi_glue.s");

	args[0] = "/lib/idcomp";
	args[1] = "-i";
	args[2] = tmpf;
	args[3] = "-o";
	args[4] = tmpf2;
	args[5] = NULL;
	os_exec(NULL, "/lib/idcomp", args);

	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/udi_glue.o");

	args[0] = "/bin/idas";
	args[1] = "-dl";
	args[2] = "-o";
	args[3] = tmpf;
	args[4] = tmpf2;
	args[5] = NULL;
	os_exec(NULL, "idas", args);

	/*
	 * Save the current directory.
	 */
	current_dir = (char *)getcwd(NULL, 513);

	if (current_dir == NULL)
	{
		fprintf(stderr, "Error: unable to get current working "
				"directory!\n");
		exit(2);
	}

	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);

	/*
	 * Go to the dir with the Architecture specific files in it
	 */
	if (chdir(tmpf) != 0)
	{
		fprintf(stderr, "Error: unable to change to directory "
				"%s!\n", mkinst_infop->tmpdir);
		exit(2);
	}

	if (elf2coff("Driver.o.elf", "Driver.o.coff") == 0)
	{
		fprintf(stderr, "elf2coff failed\n");
		exit(2);
	}

	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/Driver.o");

	strcpy(tmpf2, mkinst_infop->tmpdir);
	strcat(tmpf2, "/usr/lib/udirt/ID/");
	strcat(tmpf2, mkinst_infop->name);
	strcat(tmpf2, "/Driver.o.coff");

	strcpy(tmpf3, mkinst_infop->tmpdir);
	strcat(tmpf3, "/usr/lib/udirt/ID/");
	strcat(tmpf3, mkinst_infop->name);
	strcat(tmpf3, "/udi_glue.o");

	args[0] = "/bin/idld";
	args[1] = "-r";
	args[2] = "-o";
	args[3] = tmpf;
	args[4] = tmpf2;
	args[5] = tmpf3;
	args[6] = NULL;
	os_exec(NULL, "idld", args);


	fix_static(tmpf);

	/*
	 * Return to the directory we started from
	 */
	if (chdir(current_dir) != 0)
	{
		fprintf(stderr, "Error: unable to change to directory %s!\n",
			current_dir);

		exit(2);
	}
}

#define INSTDIR	"/usr/lib/udirt/ID"

void
create_package_file(
struct mkinst_info	*mkinst_infop
)
{
	FILE		*fp;
	char		tmpf[513];
	char		*s;
	char		*s2;
	char		*s3;
	char		*args[100];
	char		*current_dir;

	fprintf(stderr, "Creating files in %s\n", mkinst_infop->tmpdir);
	fflush(stderr);

	/*
	 * --------------------------------------------------------------
	 * Create prototype
	 */
	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/prototype");

	fp = fopen(tmpf, "w+");

	if (fp == NULL)
	{
		fprintf(stderr, "Error: unable to open %s for writing!\n",
				tmpf);
		exit(1);
	}

	fprintf(fp, "i copyright\n");
	fprintf(fp, "i pkginfo\n");
	fprintf(fp, "i postinstall\n");
	fprintf(fp, "i preremove\n");
	fprintf(fp, "i admin\n");

	fprintf(fp, "d none /usr\t0755\troot\tsys\n");
	fprintf(fp, "d none /usr/lib\t0755\troot\tsys\n");
	fprintf(fp, "d none /usr/lib/udirt\t0755\troot\tsys\n");
	fprintf(fp, "d none /usr/lib/udirt/ID\t0755\troot\tsys\n");

	fprintf(fp, "f none %s/%s/Driver.o\t0644\troot\tsys\n", INSTDIR,
		mkinst_infop->name);
	fprintf(fp, "f none %s/%s/Master\t0644\troot\tsys\n", INSTDIR,
		mkinst_infop->name);
	fprintf(fp, "f none %s/%s/System\t0644\troot\tsys\n", INSTDIR,
		mkinst_infop->name);

	if (strcmp(mkinst_infop->name, "udi_nic") == 0)
	{
		fprintf(fp, "f none %s/%s/Space.c\t0644\troot\tsys\n", INSTDIR,
			mkinst_infop->name);
	}

	if (strcmp(mkinst_infop->modtype, "scsi") == 0)
	{
		fprintf(fp, "f none %s/%s/Space.c\t0644\troot\tsys\n", INSTDIR,
			mkinst_infop->name);
	}

	if (strcmp(mkinst_infop->modtype, "nic") == 0)
	{
		fprintf(fp, "f none %s/%s/Space.c\t0644\troot\tsys\n", INSTDIR,
			mkinst_infop->name);
		fprintf(fp, "f none %s/%s/space.h\t0644\troot\tsys\n", INSTDIR,
			mkinst_infop->name);
		fprintf(fp, "f none %s/%s/Node\t0644\troot\tsys\n", INSTDIR,
			mkinst_infop->name);
		fprintf(fp, "f none %s/%s/lkcfg\t0744\troot\tsys\n", INSTDIR,
			mkinst_infop->name);
		fprintf(fp, "d none %s/%s/AOF\t0644\troot\tsys\n", INSTDIR,
			mkinst_infop->name);
		fprintf(fp, "f none %s/%s/AOF/%s\t0644\troot\tsys\n", INSTDIR,
			mkinst_infop->name, mkinst_infop->name);
	}

	fclose(fp);

	/*
	 * --------------------------------------------------------------
	 * Create pkginfo
	 */
	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/pkginfo");

	fp = fopen(tmpf, "w+");

	if (fp == NULL)
	{
		fprintf(stderr, "Error: unable to open %s for writing!\n",
				tmpf);
		exit(1);
	}

	s = strdup(mkinst_infop->name);

	for (s2 = s, s3 = mkinst_infop->name ; *s3 != '\0' ; s3++)
	{
		if (*s3 == '_')
		{
			continue;
		}

		if ((*s3 >= 'a') && (*s3 <= 'z'))
		{
			*s2 = *s3 - 'a' + 'A';
		}

		s2++;
	}

	*s2 = '\0';

	fprintf(fp, "PKG=%s\n", s);
	fprintf(fp, "NAME=%s UDI driver\n", mkinst_infop->name);
	fprintf(fp, "VERSION=1.0\n");
	fprintf(fp, "CATEGORY=system\n");
	fprintf(fp, "ARCH=i86\n");
	fprintf(fp, "VENDOR=SCO\n");
	fprintf(fp, "BASEDIR=/\n");
	fprintf(fp, "MAXINST=1\n");
	fprintf(fp, "PSTAMP=scohackStamp\n");
	fprintf(fp, "CLASSES=none\n");

	fclose(fp);

	/*
	 * --------------------------------------------------------------
	 * Create copyright
	 */
	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/copyright");

	fp = fopen(tmpf, "w+");

	if (fp == NULL)
	{
		fprintf(stderr, "Error: unable to open %s for writing!\n",
				tmpf);
		exit(1);
	}

	fprintf(fp, "Copyright The Santa Cruz Operation, Inc., All "
		"Rights Reserved\n");

	fclose(fp);

	/*
	 * --------------------------------------------------------------
	 * Create admin
	 */
	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/admin");

	fp = fopen(tmpf, "w+");

	if (fp == NULL)
	{
		fprintf(stderr, "Error: unable to open %s for writing!\n",
				tmpf);
		exit(1);
	}


	fprintf(fp, "mail=\n");
	fprintf(fp, "instance=overwrite\n");
	fprintf(fp, "partial=nocheck\n");
	fprintf(fp, "runlevel=ask\n");
	fprintf(fp, "idepend=ask\n");
	fprintf(fp, "rdepend=ask\n");
	fprintf(fp, "space=ask\n");
	fprintf(fp, "setuid=nochec\n");
	fprintf(fp, "conflict=nocheck\n");
	fprintf(fp, "action=nocheck\n");
	fprintf(fp, "basedir=default\n");

	fclose(fp);

	/*
	 * --------------------------------------------------------------
	 * Create postinstall
	 */
	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/postinstall");

	fp = fopen(tmpf, "w+");

	if (fp == NULL)
	{
		fprintf(stderr, "Error: unable to open %s for writing!\n",
				tmpf);
		exit(1);
	}


	fprintf(fp, "#!/bin/ksh\n\n");
	fprintf(fp, "cd /usr/lib/udirt/ID/%s\n", mkinst_infop->name);

	if (strcmp(mkinst_infop->modtype, "nic") == 0)
	{
		fprintf(fp, "if [ ! -d %sAOF ] ; then mkdir -p %sAOF > /dev/null 2>&1 ; fi\n", udi_filedir, udi_filedir);
		fprintf(fp, "for i in Driver.o Master Node System Space.c "
			"AOF/* lkcfg space.h\n");

		fprintf(fp, "do\n");

		fprintf(fp, "\tif [ -f ");
		fprintf(fp, "$i ] ; then ");

		fprintf(fp, "cp ");
		fprintf(fp, "$i ");
		fprintf(fp, udi_filedir);
		fprintf(fp, "$i ; ");
		fprintf(fp, "chown root:root ");
		fprintf(fp, udi_filedir);
		fprintf(fp, "$i ; fi\n");
		fprintf(fp, "done\n");
	}
	else
	{
		if (strcmp(mkinst_infop->modtype, "scsi") == 0)
		{
			pid_t		process_pid;

			process_pid = getpid();

			fprintf(fp, "sed 's/udi_scsi/%s/g' "
				"/etc/conf/cf.d/mscsi > "
				"/etc/conf/cf.d/mscsi.%d\n",
					mkinst_infop->name, process_pid);

			fprintf(fp, "mv /etc/conf/cf.d/mscsi.%d "
				"/etc/conf/cf.d/mscsi\n", process_pid);

			fprintf(fp, "grep -v \"^%s[ \t]\" "
				"/etc/default/scsihas > "
				"/tmp/scsihas.%d\n", mkinst_infop->name,
					process_pid);

			fprintf(fp, "echo \"%-15.15s\" \"\\\"%s\\\"\" >> "
				"/tmp/scsihas.%d\n",
				mkinst_infop->name, mkinst_infop->desc,
				process_pid);

			fprintf(fp, "cp /tmp/scsihas.%d "
				"/etc/default/scsihas\n", process_pid);

			fprintf(fp, "rm -f /tmp/scsihas.%d\n", process_pid);

		}

		fprintf(fp, "/etc/conf/bin/idinstall -d %s\n",
			mkinst_infop->name);
		fprintf(fp, "/etc/conf/bin/idinstall -a -k %s\n",
			mkinst_infop->name);
	}

	fprintf(fp, "\nexit 0\n");

	fclose(fp);

	/*
	 * --------------------------------------------------------------
	 * Create preremove
	 */
	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/preremove");

	fp = fopen(tmpf, "w+");

	if (fp == NULL)
	{
		fprintf(stderr, "Error: unable to open %s for writing!\n",
				tmpf);
		exit(1);
	}


	fprintf(fp, "#!/bin/ksh\n\n");

	if (strcmp(mkinst_infop->modtype, "nic") == 0)
	{
		fprintf(fp, "rm -rf /var`/etc/llipathmap`/ID/%s\n",
			mkinst_infop->name);
	}
	else
	{
		fprintf(fp, "/etc/conf/bin/idinstall -d %s\n",
			mkinst_infop->name);
	}

	fprintf(fp, "\nexit 0\n");

	fclose(fp);

	/*
	 * --------------------------------------------------------------
	 * Create package: /usr/bin/pkgmk -o -d $TMPDIR -r $TMPDIR
	 */

	/*
	 * Save the current directory.
	 */
	current_dir = (char *)getcwd(NULL, 513);

	if (current_dir == NULL)
	{
		fprintf(stderr, "Error: unable to get current working "
				"directory!\n");
		exit(2);
	}

	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);

	/*
	 * Go to the dir with the Architecture specific files in it
	 */
	if (chdir(tmpf) != 0)
	{
		fprintf(stderr, "Error: unable to change to directory "
				"%s!\n", mkinst_infop->tmpdir);
		exit(2);
	}


	args[0] = "/usr/bin/pkgmk";
	args[1] = "-o";
	args[2] = "-d";
	args[3] = mkinst_infop->tmpdir;
	args[4] = "-r";
	args[5] = mkinst_infop->tmpdir;
	args[6] = NULL;
	os_exec(NULL, "/usr/bin/pkgmk", args);

	/*
	 * --------------------------------------------------------------
	 * Translate package: /usr/bin/pkgtrans -s 
	 * 	$TMPDIR $TMPDIR/package package-name
	 */
	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, "/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, ".udi.pkg");

	args[0] = "/usr/bin/pkgtrans";
	args[1] = "-s";
	args[2] = mkinst_infop->tmpdir;
	args[3] = tmpf;
	args[4] = s;
	args[5] = NULL;
	os_exec(NULL, "/usr/bin/pkgtrans", args);

	/*
	 * Return to the directory we started from
	 */
	if (chdir(current_dir) != 0)
	{
		fprintf(stderr, "Error: unable to change to directory "
				"%s!\n", current_dir);
		exit(2);
	}


	free(s);
}

void
install_driver(
struct mkinst_info	*mkinst_infop
)
{
	char		tmpf[513];
	char		*args[100];
	char		*current_dir;

	/*
	 * Save the current directory.
	 */
	current_dir = (char *)getcwd(NULL, 513);

	if (current_dir == NULL)
	{
		fprintf(stderr, "Error: unable to get current working "
				"directory!\n");
		exit(2);
	}

	strcpy(tmpf, mkinst_infop->tmpdir);
	strcat(tmpf, "/usr/lib/udirt/ID/");
	strcat(tmpf, mkinst_infop->name);

	/*
	 * Go to the dir with the Architecture specific files in it
	 */
	if (chdir(tmpf) != 0)
	{
		fprintf(stderr, "Error: unable to change to directory "
				"%s!\n", mkinst_infop->tmpdir);
		exit(2);
	}

	strcat(tmpf, "/");
	strcat(tmpf, mkinst_infop->name);
	strcat(tmpf, ".udi.pkg");

	args[0] = "/usr/bin/pkgadd";
	args[1] = "-d";
	args[2] = tmpf;
	args[3] = "all";
	args[4] = NULL;
	os_exec(NULL, "/usr/bin/pkgadd", args);

	/*
	 * Return to the directory we started from
	 */
	if (chdir(current_dir) != 0) {
		fprintf(stderr, "Error: unable to change to directory "
				"%s!\n", current_dir);
		exit(2);
	}

	free(current_dir);
}


#undef input_data
#define Dflg udi_global.debug
#define Oflg udi_global.optimize
#define sflg udi_global.stop_install


void
osdep_mkinst_files(
int		install_src,
propline_t	**inp_data,
char		*basedir,
char		*tmpdir
)
{
	int			modtypeindex;
	int			index;
	int			i;
	char			*wherewasi;
	int			bridgemetaidx;
        char    		*incstr;
	char			*s;
	char			*spfname;
	int			numinp;
	extern int		parsemfile;
	struct mkinst_info	*mkinst_infop;
	char			name[513];
	char			module[513];
	char			shortname[513];
	char			modtype[256];
	int			udi_version;

        incstr = NULL;

	mkinst_infop = (struct mkinst_info *)malloc(
		sizeof(struct mkinst_info));

	if (mkinst_infop == NULL)
	{
		fprintf(stderr, "Malloc failed\n");

		exit(2);
	}

	mkinst_infop->name = name;
	mkinst_infop->module = module;
	mkinst_infop->shortname = shortname;
	mkinst_infop->tmpdir = tmpdir;
	mkinst_infop->install_src = install_src;

	/*
	 * Original directory location
	 */
	wherewasi = getcwd(NULL, 513);

        if (chdir(basedir) != 0)
	{
                fprintf(stderr, "Error: unable to change to directory "
                                "%s!\n", basedir);
                exit(2);
        }

	modtype[0] = '\0';

	mkinst_infop->ipl = -1;

        /*
         * Use OpenServer Kernel Build environment if set. Otherwise default to
         * local 'root' build paths
         */
        mkinst_infop->confstr = (char *)getenv("CONF");

        if (mkinst_infop->confstr == NULL)
	{
		mkinst_infop->confstr = "/etc/conf";
        }

        incstr = (char *)getenv("INC");

        if (incstr == NULL)
	{
		incstr = optinit();
                optcat(&incstr, "/usr/include/udi");
        }
	else
	{
                strcat(incstr, "/udi");
        }

	/* Scan for various things */
	bridgemetaidx = pioserlim = -1;
	index = 0;
	while (inp_data[index] != NULL)
	{
		if (inp_data[index]->type == SHORTNAME)
		{
			strcpy(name, inp_data[index+1]->string);
		}
		else if (inp_data[index]->type == MODULE)
		{
			strcpy(module, inp_data[index+1]->string);
		}
		else if (inp_data[index]->type == CATEGORY)
		{
			modtypeindex = inp_data[index+1]->ulong;
		}
		else if (inp_data[index]->type == NAME)
		{
			desc_index = inp_data[index+1]->ulong;
		}
		else if (inp_data[index]->type == PIOSERLIMIT)
		{
			pioserlim = inp_data[index+1]->ulong;
		}
		else if (inp_data[index]->type == PROPVERS)
		{
			version = inp_data[index+1]->ulong;
		}
		else if (inp_data[index]->type == REQUIRES)
		{
			if (strcmp(inp_data[index+1]->string, "udi") == 0)
			{
				udi_version = inp_data[index+2]->ulong;
			}
		} else if (inp_data[index]->type == META)
		{
			if (strcmp(inp_data[index+2]->string, "udi_bridge")
					== 0)
			{
				bridgemetaidx = inp_data[index+1]->ulong;
			}
		}
		else if (inp_data[index]->type == REGION)
		{
			if (inp_data[index+1]->ulong == 0)
			{
				i = index + 2;

				while (inp_data[i] != NULL)
				{
					if (strcmp("latency", inp_data[i]->string)
							== 0)
					{
						if (strcmp("powerfail_warning",
							inp_data[i+1]->string)
							== 0)
						{
							mkinst_infop->ipl = 7;
						}
						else if (strcmp("overrunable",
							inp_data[i+1]->string)
							== 0)
						{
							mkinst_infop->ipl = 6;
						}
						else if (strcmp("retryable",
							inp_data[i+1]->string)
							== 0)
						{
							mkinst_infop->ipl = 5;
						}
						else if (strcmp("non_overrunable",
							inp_data[i+1]->string)
							== 0)
						{
							mkinst_infop->ipl = 4;
						}
						else if (strcmp("non_critical",
							inp_data[i+1]->string)
							== 0)
						{
							mkinst_infop->ipl = 3;
						}
					}
					i += 2;
				}
			}
		}
		while (inp_data[index] != NULL) index++;
		index++;
	}

	if (strlen(name) > 5)
	{
		if (strncmp(name, "udi_", 4) == 0)
		{
			strcpy(shortname, "u");
			strncat(shortname, name + 4, 4);
		}
		else if (strncmp(name, "udi", 3) == 0)
		{
			strcpy(shortname, "u");
			strncat(shortname, name + 3, 4);
		}
		else
		{
			strncpy(shortname, name, 5);
		}
	} else {
		strcpy (shortname, name);
	}

	get_mod_type(modtype);

	/*
	 * For specific categories of devices, override hints.
	 */
	if (strcmp(modtype, "nic") == 0) {
		mkinst_infop->ipl = 6;
	}
	if (strcmp(modtype, "scsi") == 0) {
		mkinst_infop->ipl = 5;
	}

	/*
	 * If region declaration did not specify latency, use default
	 */
	if (mkinst_infop->ipl < 0)
		mkinst_infop->ipl = 5;

	/*
	 * Find the child_bind_ops to determine the driver type.
	 * Also, check for parent_bind_ops using the udi_bridge meta.
	 */
	index = mkinst_infop->havehw = 0;

	while (inp_data[index] != NULL)
	{
		if (inp_data[index]->type == PARBOPS)
		{
			if (inp_data[index+1]->ulong == bridgemetaidx)
				mkinst_infop->havehw = 1;
		}

		while (inp_data[index] != NULL)
		{
			index++;
		}

		index++;
	}

	/*
	 * Set the aux/msg file directory
	 */
	if (strcmp(modtype, "nic") != 0)
	{
		/* normal build */
		optcpy(&udi_filedir, UDI_DRVFILEDIR);
	}
	else
	{
#define MAXPATHLEN	1024
		FILE	*llipathfp;
		char	*llipathmap;
		
		
		llipathmap = (char *)malloc(MAXPATHLEN);

		if (llipathmap == NULL)
		{
			fprintf(stderr, "Error: Unable to alloc "
				"llipathmap\n");

			exit(1);
		}

		llipathfp = popen("/etc/llipathmap", "r");

		if (llipathfp == NULL)
		{
			fprintf(stderr, "Error: Unable to run "
				"/etc/llipathmap\n");

			exit(1);
		}

		if (fgets(llipathmap, MAXPATHLEN, llipathfp) == NULL)
		{
			fprintf(stderr, "Error: Unable to run "
				"/etc/llipathmap\n");

			exit(1);
		}

		s = strchr(llipathmap, '\n');

		if (s != NULL)
		{
			*s = '\0';
		}

		pclose(llipathfp);

		/* normal build */
		optcpy(&udi_filedir, "/var");
		optcat(&udi_filedir, llipathmap);
		optcat(&udi_filedir, "/ID");

		free(llipathmap);
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

	while (i < numinp)
	{
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
			yyparse();
			inp_data = udi_global.propdata;
		}
		while (inp_data[i] != NULL) i++; i++;
	}

	mkinst_infop->inp_data = inp_data;
	mkinst_infop->modtype = modtype;

	create_master_file(mkinst_infop);

	create_system_file(mkinst_infop);

	create_node_file(mkinst_infop);

	create_driver_file(mkinst_infop);

	/*
	 * Create a AOF file if this is a network driver
	 */
	if (strcmp(modtype, "nic") == 0)
	{
		create_AOF(mkinst_infop);
	}

	if (strcmp(modtype, "scsi") == 0)
	{
		output_scsi_space_dot_c(mkinst_infop);
	}

	create_package_file(mkinst_infop);

	/*
	 * Now, run the commands for the actual installation.
	 * If the stop before actual install flag is not set, do install
	 */
	if (!sflg)
	{
		install_driver(mkinst_infop);
	}

        /*
         * Return to original 'wherewasi' path
         */
        if (chdir(wherewasi) != 0) {
                fprintf(stderr, "Error: unable to change to directory "
                                "%s!\n", wherewasi);
                exit(2);
        }

        free(wherewasi);
}

#define input_data udi_global.propdata

struct readable_file_data {
	struct readable_file_data	*next;
	char				*name;
	int				size;
};

struct readable_file_data	*rfile_data_head;
struct readable_file_data	*rfile_data_tail;

void
add_readable_file(
char *name
)
{
	struct readable_file_data	*rfile_datap;

	rfile_datap = (struct readable_file_data *)
		malloc(sizeof(struct readable_file_data));

	if (rfile_datap == NULL)
	{
		return;
	}

	rfile_datap->next = NULL;

	if (rfile_data_head == NULL)
	{
		rfile_data_head = rfile_datap;
		rfile_data_tail = rfile_datap;
	}
	else
	{
		rfile_data_tail->next = rfile_datap;
		rfile_data_tail = rfile_datap;
	}

	rfile_datap->name = name;
}

void
process_readfiles(
FILE			*fp,
struct mkinst_info	*mkinst_infop
)
{
	struct readable_file_data	*rfile_datap;
	int				fd;
	unsigned			i;
	unsigned			j;
	char				buf2[1024];
	char				*buffer;

	for (i = 0, rfile_datap = rfile_data_head ; rfile_datap != NULL ;
		rfile_datap = rfile_datap->next, i++)
	{
		getcwd(buf2, 1024);

		if (mkinst_infop->install_src)
		{
			strcat(buf2, "/../extra/");
		}
		else
		{
			strcat(buf2, "/../../extra/");
		}

		strcat(buf2, rfile_datap->name);

		fd = open(buf2, O_RDONLY);

		if (fd < 0)
		{
			fprintf(fp, "/* open failed %s */\n", buf2);

			continue;
		}

		lseek(fd, 0, SEEK_END);

		rfile_datap->size = lseek(fd, 0, SEEK_CUR);

		lseek(fd, 0, SEEK_SET);

		if (rfile_datap->size <= 0)
		{
			close(fd);
			continue;
		}

		buffer = (char *)malloc(rfile_datap->size);

		if (buffer == NULL)
		{
			close(fd);
			continue;
		}

		fprintf(fp, "unsigned char\t%s_readfile%d[] = {\n",
			mkinst_infop->name, i);


		read(fd, buffer, rfile_datap->size);

		for (j = 0 ; j < rfile_datap->size ; j++)
		{
			fprintf(fp, "0x%02x", *(buffer + j));

			if (j != (rfile_datap->size - 1))
			{
				fprintf(fp, ", ");
			}

			if (((j + 1) % 8) == 0)
			{
				fprintf(fp, "\n");
			}
		}

		fprintf(fp, "\n};\n");

		free(buffer);

		close(fd);
	}

	fprintf(fp, "\n");
}


void
make_glue(
char			*tmpf,
struct mkinst_info	*mkinst_infop
)
{
	FILE		*fp;
	int		i;
	int		index;
	int		find_type;
	int		first;
	int		tmp_count;
	char		buf[1024];
	char		buf2[1024];
	char		locale[256];
	char		cur_loc[256];
	unsigned int	raflag;
	unsigned int	overrun;
	int		nmsgs;
	int		msgnum;
	int		metaidx;
	int		mt_count;
	int		rq_count;
	int		cb_count;
	int		pb_count;
	int		ib_count;
	int		rf_count;
	int		l_count;
	int		mf_count;
	int		ms_count;
	int		r_count;
	int		d_count;
	int		e_count;
	int		a_count;
	struct readable_file_data	*rfile_datap;

	/* Are we doing this in the Posix environment? */
	char *posix = getenv("DO_POSIX");

	fp = fopen(tmpf, "w+");

	if (fp == NULL)
	{
		fprintf(stderr, "Error: unable to open %s for writing!\n",
				tmpf);
		exit(1);
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
	fprintf(fp, "\tchar\t*if_name;\n");
	fprintf(fp, "\tint\tver;\n");
	fprintf(fp, "} _udi_sp_require_t;\n\n");
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
	fprintf(fp, "\tchar\t*string;\n");
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

	/* Create the xxx_udi_sprops_t structure */
	mt_count = cb_count = pb_count = ib_count = pv_count = rf_count =
			l_count = mf_count = ms_count = r_count = d_count =
			e_count = a_count = rq_count = 0;

	fprintf(fp, "typedef struct {\n");
	fprintf(fp, "\tchar\t\t\t*shortname;\n");
	fprintf(fp, "\tint\t\t\tversion;\n");
	fprintf(fp, "\tint\t\t\trelease;\n");
	fprintf(fp, "\tint\t\t\tpioserlim;\n");

	for (find_type = 0; find_type < NUM_FIND; find_type++) {
		first = 1;
		index = 0;

		while (input_data[index] != NULL) {
			switch(find_type) {
			case 0: /* metas */
				if (input_data[index]->type == META)
					mt_count++;
				break;
			case 1:	/* child_bindops */
				if (first) {
					if (mt_count == 0)
						mt_count++;
					fprintf(fp, "\t_udi_sp_meta_t"
							"\t\tmetas"
							"[%d];\n", mt_count);
					first = 0;
				}
				if (input_data[index]->type == CHLDBOPS)
					cb_count++;
				break;
			case 2:	/* parent_bindops */
				if (first) {
					if (cb_count == 0)
						cb_count++;
					fprintf(fp, "\t_udi_sp_bindops_t"
							"\tchild_bindops"
							"[%d];\n", cb_count);
					first = 0;
				}
				if (input_data[index]->type == PARBOPS)
					pb_count++;
				break;
			case 3:	/* intern_bindops */
				if (first) {
					if (pb_count == 0)
						pb_count++;
					fprintf(fp, "\t_udi_sp_bindops_t"
							"\tparent_bindops"
							"[%d];\n", pb_count);
					first = 0;
				}
				if (input_data[index]->type == INTBOPS)
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
				if (input_data[index]->type == PROVIDES)
					pv_count++;
				break;
			case 5: /* requires */
				if (first) {
					if (pv_count == 0)
						pv_count++;
					fprintf(fp, "\t_udi_sp_provide_t"
							"\tprovides"
							"[%d];\n", pv_count);
					first = 0;
				}
				if (input_data[index]->type == REQUIRES)
					rq_count++;
				break;
			case 6: /* regions */
				if (first) {
					if (rq_count == 0)
						rq_count++;
					fprintf(fp, "\t_udi_sp_require_t"
							"\trequires"
							"[%d];\n", rq_count);
					first = 0;
				}
				if (input_data[index]->type == REGION)
					r_count++;
				break;
			case 7: /* devices */
				if (first) {
					if (r_count == 0)
						r_count++;
					fprintf(fp, "\t_udi_sp_region_t\t"
							"regions[%d];\n",
							r_count);
					first = 0;
				}
				if (input_data[index]->type == DEVICE)
					d_count++;
				break;
			case 8: /* enumerates */
				if (first) {
					if (d_count == 0)
						d_count++;
					fprintf(fp, "\t_udi_sp_dev_t\t"
							"devices[%d];\n",
							d_count);
					first = 0;
				}
				if (input_data[index]->type == ENUMERATES)
					e_count++;
				break;
			case 9: /* attributes */
				if (first) {
					if (e_count == 0)
						e_count++;
					fprintf(fp, "\t_udi_sp_enum_t\t\t"
							"enumerates[%d];\n",
							e_count);
					first = 0;
				}
				if (input_data[index]->type == DEVICE) {
					i = index+3;
					while (input_data[i] != NULL) {
						i+=3;
					}
					i -= index+3;
					i /= 3;
					a_count += i;
				}
				if (input_data[index]->type == ENUMERATES) {
					i = index+5;
					while (input_data[i] != NULL) {
						i+=3;
					}
					i -= index+3;
					i /= 3;
					a_count += i;
				}
				break;
			case 10: /* readable_files */
				if (first) {
					if (a_count == 0) {
						a_count++;
					}
					fprintf(fp, "\t_udi_sp_attr_t\t\t"
							"attribs[%d];\n",
							a_count);
					first = 0;
				}
				if (input_data[index]->type == READFILE)
				{
					add_readable_file(
						input_data[index+1]->string);
					rf_count++;
				}
				break;
			case 11: /* locale */
				if (first) {
					if (rf_count == 0)
						rf_count++;
					fprintf(fp, "\t_udi_sp_readfile_t\t"
							"read_files[%d];\n",
							rf_count);
					first = 0;
					l_count = 1;
				}
				if (input_data[index]->type == LOCALE)
					l_count++;
				break;
			case 12: /* message files */
				if (first) {
					fprintf(fp, "\t_udi_sp_locale_t\t"
							"locale[%d];\n",
							l_count);
					first = 0;
				}
				if (input_data[index]->type == MESGFILE) {
					/*
					 * Read in, and count the number of
					 * locales.
					 */
					FILE *mfp;
					int msgf = 0;
					if (mkinst_infop->install_src) {
						sprintf(buf, "../msg/%s",
							input_data[index+1]->
							string);
					} else {
						sprintf(buf, "../../msg/%s",
							input_data[index+1]->
							string);
					}
					mfp = fopen(buf, "r");

					if (mfp == NULL)
					{
						fprintf(stderr, "Error: unable "
							"to open message file "
							"%s for reading!\n",
							buf);
						exit(1);
					}
					while (fgets(buf, 1023, mfp) != NULL) {
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
							msgf++;
						}
					}
					if (mf_count == 0 && msgf != 0)
						mf_count++;
					fclose(mfp);
				}
				break;
			case 13: /* messages */
				if (first) {
					if (mf_count == 0)
						mf_count++;
					fprintf(fp, "\t_udi_sp_msgfile_t\t"
							"msg_files[%d];\n",
							mf_count);
					first = 0;
				}
				if (input_data[index]->type == MESSAGE ||
						input_data[index]->type == DISMESG) {
					ms_count++;
					if (input_data[index+1]->ulong ==
							desc_index) {
						mkinst_infop->desc =
							input_data[index+2]->
								string;
					}
				}
				break;
			}
			while (input_data[index] != NULL) index++;
			index++;
		}
	}
	fprintf(fp, "\t_udi_sp_message_t\tmessages[%d];\n", ms_count+1);
	fprintf(fp, "} %s_udi_sprops_t;\n\n", mkinst_infop->name);

	process_readfiles(fp, mkinst_infop);

	rfile_datap = rfile_data_head;

	/* Create the xxx_udi_sprops structure */
	mt_count = cb_count = pb_count = ib_count = pv_count = rf_count =
			l_count = mf_count = ms_count = r_count = d_count =
			e_count = a_count = rq_count = 0;
	fprintf(fp, "%s_udi_sprops_t %s_udi_sprops = {\n",
		mkinst_infop->name, mkinst_infop->name);
	fprintf(fp, "\t/* Static Properties Shortname */\n");
	fprintf(fp, "\t\"%s\",\n", mkinst_infop->name);
	fprintf(fp, "\t/* Static Properties Version */\n");
	fprintf(fp, "\t0x%x,\n", version);
	fprintf(fp, "\t/* Static Properties Structure Release */\n");
	fprintf(fp, "\t1,\n");
	fprintf(fp, "\t/* PIO SERLIMIT */\n");
	fprintf(fp, "\t%d,\n", pioserlim);

	for (find_type = 0; find_type < NUM_FIND; find_type++) {
		first = 1;
		index = 0;

		while (input_data[index] != NULL) {
			switch(find_type) {
			case 0:	/* metas */
				if (first) {
					fprintf(fp, "\t/* metas */\n");
					first = 0;
				}
				if (input_data[index]->type == META) {
					mt_count++;
					fprintf(fp, "\t%d, \"%s\",\n",
						input_data[index+1]->ulong,
						input_data[index+2]->string);
				}
				break;
			case 1:	/* child_bind_ops */
				if (first) {
					if (mt_count == 0) {
						fprintf(fp, "\t-1, \"\",\n");
					}
					fprintf(fp, "\t/* child_bind_ops */\n");
					first = 0;
				}
				if (input_data[index]->type == CHLDBOPS) {
					cb_count++;
					fprintf(fp, "\t%d, %d, %d, %d,\n",
						input_data[index+1]->ulong,
						input_data[index+2]->ulong,
						input_data[index+3]->ulong,
						input_data[index+4]->ulong);
				}
				break;
			case 2:	/* parent_bind_ops */
				if (first) {
					if (cb_count == 0) {
						fprintf(fp, "\t-1, -1, -1, -1,\n");
					}
					fprintf(fp, "\t/* parent_bind_ops */\n");
					first = 0;
				}
				if (input_data[index]->type == PARBOPS) {
					pb_count++;
					fprintf(fp, "\t%d, %d, %d, %d,\n",
						input_data[index+1]->ulong,
						input_data[index+2]->ulong,
						input_data[index+3]->ulong,
						input_data[index+4]->ulong);
				}
				break;
			case 3:	/* internal_metas */
				if (first) {
					if (pb_count == 0) {
						fprintf(fp, "\t-1, -1, -1, -1,\n");
					}
					fprintf(fp, "\t/* internal_bind_ops */\n");
					first = 0;
				}
				if (input_data[index]->type == INTBOPS) {
					ib_count++;
					fprintf(fp, "\t%d, %d, %d, %d, %d,\n",
						input_data[index+1]->ulong,
						input_data[index+2]->ulong,
						input_data[index+3]->ulong,
						input_data[index+4]->ulong,
						input_data[index+5]->ulong);
				}
				break;
			case 4: /* provides */
				if (first) {
					if (ib_count == 0) {
						fprintf(fp, "\t-1, -1, -1, -1, -1,"
								"\n");
					}
					fprintf(fp, "\t/* provides */\n");
					first = 0;
				}
				if (input_data[index]->type == PROVIDES) {
					pv_count++;
					fprintf(fp, "\t\"%s\", %d,\n",
						input_data[index+1]->string,
						input_data[index+2]->ulong);
				}
				break;
			case 5: /* requires */
				if (first) {
					if (pv_count == 0) {
						fprintf(fp, "\t\"\", -1,\n");
					}
					fprintf(fp, "\t/* requires */\n");
					first = 0;
				}
				if (input_data[index]->type == REQUIRES) {
					rq_count++;
					fprintf(fp, "\t\"%s\", %d,\n",
						input_data[index+1]->string,
						input_data[index+2]->ulong);
				}
				break;
			case 6: /* regions */
				if (first) {
					if (rq_count == 0) {
						fprintf(fp, "\t\"\", -1,\n");
					}
					fprintf(fp, "\t/* regions */\n");
					first = 0;
				}
				if (input_data[index]->type == REGION) {
					r_count++;
					fprintf(fp, "\t%d, ",
						input_data[index+1]->ulong);
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
					if (! (raflag&_UDI_RA_TYPE_MASK))
						raflag |= _UDI_RA_NORMAL;
					if (! (raflag&_UDI_RA_BINDING_MASK))
						raflag |= _UDI_RA_STATIC;
					if (! (raflag&_UDI_RA_PRIORITY_MASK))
						raflag |= _UDI_RA_MED;
					if (! (raflag&_UDI_RA_LATENCY_MASK))
						raflag |= _UDI_RA_NON_OVERRUN;
					/* Set the values */
					fprintf(fp, "0x%x, 0x%x,\n", raflag,
							overrun);
				}
				break;
			case 7: /* devices */
				if (first) {
					if (r_count == 0) {
						fprintf(fp, "\t-1, 0, 0,\n");
					}
					fprintf(fp, "\t/* devices */\n");
					first = tmp_count = 0;
				}
				if (input_data[index]->type == DEVICE) {
					d_count++;
					fprintf(fp, "\t%d, ",
						input_data[index+2]->ulong);
					i = index+3;
					while (input_data[i] != NULL) {
						i++;
					}
					i -= index+3;
					i /= 3;
					fprintf(fp, "%d, %d,\n", tmp_count, i);
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
			case 8: /* enumerates */
				if (first) {
					if (d_count == 0) {
						fprintf(fp, "\t-1, -1, -1,\n");
					}
					fprintf(fp, "\t/* enumerations */\n");
					first = tmp_count = 0;
				}
				if (input_data[index]->type == ENUMERATES) {
					d_count++;
					fprintf(fp, "\t%d, %d, %d, ",
						input_data[index+4]->ulong,
						input_data[index+2]->ulong,
						input_data[index+3]->ulong);
					i = index+5;
					while (input_data[i] != NULL) {
						i++;
					}
					i -= index+3;
					i /= 3;
					fprintf(fp, "%d, %d,\n", tmp_count, i);
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
			case 9: /* attributes */
				if (first) {
					if (e_count == 0) {
						fprintf(fp, "\t-1, 0, 0,"
								" -1, -1,\n");
					}
					a_count = d_count = e_count = 0;
					fprintf(fp, "\t/* attributes */\n");
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
						fprintf(fp, "\t\"%s\", %d, ",
							input_data[i]->string,
							input_data[i+1]->type);
						switch(input_data[i+1]->type) {
						case UDI_ATTR_STRING:
						        fprintf(fp, "\"");
						        antiparse(fp,
								  input_data[i+2]->string);
							fprintf(fp, "\",");
							break;
						case UDI_ATTR_UBIT32:
							fprintf(fp, "(char *)%s"
								",", input_data[i+2]
								->string);
							break;
						case UDI_ATTR_BOOLEAN:
							fprintf(fp, "\"%s\",",
								input_data[i+2]
								->string);
							break;
						case UDI_ATTR_ARRAY8:
							/*
							 * TODO: convert byte
							 * pairs to a string
							 */
							break;
						}
						fprintf(fp, " /* type=");
						if (input_data[i+1]->type == UDI_ATTR_STRING)
							fprintf(fp, "STRING");
						else if (input_data[i+1]->type == UDI_ATTR_UBIT32)
							fprintf(fp, "UBIT32");
						else if (input_data[i+1]->type == UDI_ATTR_BOOLEAN)
							fprintf(fp, "BOOLEAN");
						else if (input_data[i+1]->type == UDI_ATTR_ARRAY8)
							fprintf(fp, "ARRAY8");
						fprintf(fp, ", msgnum=%d, "
							"metaidx=%d */\n",
							msgnum, metaidx);
						i+=3;
					}
					i -= index+3;
					i /= 3;
					a_count += i;
				}
				break;
			case 10: /* readable_files */
				if (first) {
					if (a_count == 0) {
						fprintf(fp, "\t\"\", 0, \"\","
								"\n");
					}
					fprintf(fp, "\t/* readable_files */\n");
					first = 0;
				}
				if (input_data[index]->type == READFILE) {

					fprintf(fp, "\t\"%s\", ",
						input_data[index+1]->string);
					fprintf(fp, "\"%saux/%s\", ",
						udi_filedir,
						input_data[index+1]->string);
					fprintf(fp, "%d,", rfile_datap->size);
					fprintf(fp, " %s_readfile%d,\n",
						mkinst_infop->name, rf_count);

					rf_count++;

					rfile_datap = rfile_datap->next;
				}
				break;
			case 11: /* locale */
				if (first) {
					if (rf_count == 0) {
						fprintf(fp, "\t\"\", \"\", 0, (unsigned char *)\"\",\n");
					}
					fprintf(fp, "\t/* locale */\n");
					first = 0;
					l_count = 1;
					nmsgs = 0;
					fprintf(fp, "\t\"C\", 0, ");
				}
				if (input_data[index]->type == LOCALE) {
					fprintf(fp, " %d,\n", nmsgs);
					nmsgs = 0;
					l_count++;
					fprintf(fp, "\t\"%s\", %d,",
						input_data[index+1]->string,
						ms_count);
				}
				if (input_data[index]->type == MESSAGE ||
					input_data[index]->type == DISMESG) {
					ms_count++;
					nmsgs++;
				}
				break;
			case 12: /* msg_files */
				if (first) {
					fprintf(fp, " %d,\n", nmsgs);
					fprintf(fp, "\t/* msg_files */\n");
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
					if (mkinst_infop->install_src) {
						sprintf(buf, "../msg/%s",
							input_data[index+1]->
							string);
					} else {
						sprintf(buf, "../../msg/%s",
							input_data[index+1]->
							string);
					}
					mfp = fopen(buf, "r");

					if (mfp == NULL)
					{
						fprintf(stderr, "Error: unable "
							"to open message file "
							"%s for reading!\n",
							buf);
						exit(1);
					}
					while (fgets(buf, 1023, mfp) != NULL) {
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
							fprintf(fp, "\t\"%s\", "
								, cur_loc);
							fprintf(fp, "\"%smsg/"
								"%s\",\n",
								udi_filedir,
								input_data
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
						fprintf(fp, "\t\"%s\", ",
							cur_loc);
						fprintf(fp, "\"%smsg/%s\",\n",
							udi_filedir,
							input_data[index+1]->
							string);
					}
					fclose(mfp);
				}
				break;
			case 13: /* messages */
				if (first) {
					if (mf_count == 0) {
						fprintf(fp, "\t\"\", \"\",\n");
					}
					fprintf(fp, "\t/* %d messages */\n", 
						nmsgs);
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
				        fprintf(fp, "\t%d, \"", 
						input_data[index+1]->ulong);
					antiparse(fp,
						  input_data[index+2]->string);
					fprintf(fp, "\", /* %s */\n",
						locale);
					ms_count++;
				}
				break;
			}
			while (input_data[index] != NULL) index++;
			index++;
		}
	}
	fprintf(fp, "\t-1, 0\n};\n");
	ms_count++;

	/* Create the xxx_udi_sprops_idx structure */
	fprintf(fp, "\n_udi_sprops_idx_t %s_udi_sprops_idx[] "
			"= {\n", mkinst_infop->name);

	fprintf(fp, "\t0, (void *)&%s_udi_sprops.shortname,\n",
		mkinst_infop->name);

	fprintf(fp, "\t0, (void *)&%s_udi_sprops.version,\n",
		mkinst_infop->name);

	fprintf(fp, "\t0, (void *)&%s_udi_sprops.release,\n",
		mkinst_infop->name);

	fprintf(fp, "\t0, (void *)&%s_udi_sprops.pioserlim,\n",
		mkinst_infop->name);

	fprintf(fp, "\t%d, (void *)&%s_udi_sprops.metas[0],\n",
		mt_count, mkinst_infop->name);

	fprintf(fp, "\t%d, (void *)&%s_udi_sprops.child_bindops[0],\n",
		cb_count, mkinst_infop->name);

	fprintf(fp, "\t%d, (void *)&%s_udi_sprops.parent_bindops[0],\n",
		pb_count, mkinst_infop->name);

	fprintf(fp, "\t%d, (void *)&%s_udi_sprops.intern_bindops[0],\n",
		ib_count, mkinst_infop->name);

	fprintf(fp, "\t%d, (void *)&%s_udi_sprops.provides[0],\n",
		pv_count, mkinst_infop->name);

	fprintf(fp, "\t%d, (void *)&%s_udi_sprops.requires[0],\n",
		rq_count, mkinst_infop->name);

	fprintf(fp, "\t%d, (void *)&%s_udi_sprops.regions[0],\n",
		r_count, mkinst_infop->name);

	fprintf(fp, "\t%d, (void *)&%s_udi_sprops.devices[0],\n",
		d_count, mkinst_infop->name);

	fprintf(fp, "\t%d, (void *)&%s_udi_sprops.enumerates[0],\n",
		e_count, mkinst_infop->name);

	fprintf(fp, "\t%d, (void *)&%s_udi_sprops.attribs[0],\n",
		a_count, mkinst_infop->name);

	fprintf(fp, "\t%d, (void *)&%s_udi_sprops.read_files[0],\n",
		rf_count, mkinst_infop->name);

	fprintf(fp, "\t%d, (void *)&%s_udi_sprops.locale[0],\n",
		l_count, mkinst_infop->name);

	fprintf(fp, "\t%d, (void *)&%s_udi_sprops.msg_files[0],\n",
		mf_count, mkinst_infop->name);

	fprintf(fp, "\t%d, (void *)&%s_udi_sprops.messages[0]\n",
		0, mkinst_infop->name);

	fprintf(fp, "};\n\n");

	if (*wrapper == 0) {
		/* Not a mapper */
		if (! pv_count) {
			/* For a driver */
			if (! posix) {
				strcpy(buf2, "udi_init_info");
			} else {
				strcpy(buf2, posix);
				strcat(buf2, "_init_info");
			}
			fprintf(fp, "extern char * %s;\n\n", buf2);

			if (strcmp(mkinst_infop->modtype, "nic") == 0)
			{
				fprintf(fp, "extern char * "
					"%s_instance_table;\n",
					mkinst_infop->name);
			}

			if (strcmp(mkinst_infop->modtype, "scsi") == 0)
			{
				fprintf(fp, "extern int %s_entry();\n",
					mkinst_infop->shortname);
			}

			fprintf(fp, "%5.5sinit()\n", mkinst_infop->shortname);

			fprintf(fp, "{\n");

			if (strcmp(mkinst_infop->modtype, "nic") == 0)
			{
				fprintf(fp, "_udi_register_instance_attributes("
					"&%s_instance_table);\n", mkinst_infop->name);
			}

			if (strcmp(mkinst_infop->modtype, "scsi") == 0)
			{
				fprintf(fp, "\tudi_register_scsi_entry(\"%s\", "
					"%s_entry);\n",
					mkinst_infop->name,
					mkinst_infop->shortname);
			}

			fprintf(fp, "\treturn _udi_driver_load(\"%s\", "
				"&%s, (void *)&%s_udi_sprops_idx[0]);\n",
					mkinst_infop->module, buf2, mkinst_infop->name);
			fprintf(fp, "}\n\n");
		} else {
			/* For a metalanguage */
			if (! posix) {
				if (strcmp(mkinst_infop->name, "udi_mgmt") == 0)
				{
					strcpy(buf2, mkinst_infop->name);
					strcat(buf2, "_meta_info");
				}
				else
				{
					strcpy(buf2, "udi_meta_info");
				}
			} else {
				strcpy(buf2, posix);
				strcat(buf2, "_meta_info");
			}
			fprintf(fp, "extern char * %s;\n", buf2);
			fprintf(fp, "int\n");
			fprintf(fp, "%5.5sinit()\n", mkinst_infop->shortname);
			fprintf(fp, "{\n");
			fprintf(fp, "\treturn _udi_meta_load(\"%s\", "
					"&%s, (void *)&%s_udi_sprops_idx[0]);\n",
					mkinst_infop->module, buf2, mkinst_infop->name);
			fprintf(fp, "}\n\n");
		}
	} else {
		/* Wrapper for a mapper */
		if (! posix) {
			strcpy(buf2, "udi_init_info");
		} else {
			strcpy(buf2, posix);
			strcat(buf2, "_init_info");
		}
		fprintf(fp, "extern char * %s;\n", buf2);
		fprintf(fp, "int\n");
		fprintf(fp, "%5.5sinit()\n", mkinst_infop->shortname);
		fprintf(fp, "{\n");
		fprintf(fp, "\treturn _udi_mapper_load(\"%s\", "
				"&%s, (void *)&%s_udi_sprops_idx[0]);\n",
				mkinst_infop->module, buf2, mkinst_infop->name);
		fprintf(fp, "}\n\n");
	}

	fclose(fp);
}

void
output_lkcfg(
FILE	*fp
)
{
	struct selection_list	*slistp;

	fprintf(fp, "#\n");
	fprintf(fp, "#       @(#) lkcfg 75.1 98/06/26 SCOINC - modified by UDI\n");
	fprintf(fp, "#\n");
	fprintf(fp, "#	Copyright (C) The Santa Cruz Operation, 1993-1998\n");
	fprintf(fp, "#	This Module contains Proprietary Information of\n");
	fprintf(fp, "#	The Santa Cruz Operation, and should be treated as Confidential.\n\n");
	fprintf(fp, "[ \"$#\" != \"5\" ] && {\n");
	fprintf(fp, "	echo \"Usage:\\n\\t$0 <-ird> INIT_DB_RECORD MDI_driver Board_number\"\n");
	fprintf(fp, "	exit $FAIL\n");
	fprintf(fp, "}\n\n");
	fprintf(fp, "LLI_ROOT=$MKMOKROOT`llipathmap`\n");
	fprintf(fp, ". $LLI_ROOT/lib/libcfg.sh\n\n");
	fprintf(fp, "PATH=$PATH:$LLI_ROOT/bin\n");
	fprintf(fp, "OPTION=$1\n");
	fprintf(fp, "IN_FILE=$2\n");
	fprintf(fp, "AOFFILE=$3\n");
	fprintf(fp, "DRV_ID=$4\n");
	fprintf(fp, "BRD_NUM=$5\n");
	fprintf(fp, "SYSTEM_FILE=$MKMOKROOT/etc/conf/sdevice.d/$DRV_ID\n");
	fprintf(fp, "MASTER_FILE=$MKMOKROOT/etc/conf/cf.d/mdevice\n");
	fprintf(fp, "DRIVER_DIR=$MKMOKROOT/etc/conf/pack.d/$DRV_ID\n\n");
	fprintf(fp, "# de-configure driver\n");
	fprintf(fp, "[ \"$OPTION\" = \"-d\" ] && {\n");
	fprintf(fp, "    [ -f $SYSTEM_FILE ] && {\n");
	fprintf(fp, "	set_system_info $SYSTEM_FILE $BRD_NUM \"N\" 0 0 0 0 0 0 0\n");
	fprintf(fp, "	awk '{ if ( $2 == \"Y\" ) exit 1 }' $SYSTEM_FILE && idinstall -d $DRV_ID\n");
	fprintf(fp, "    }\n");
	fprintf(fp, "    exit $OK\n");
	fprintf(fp, "}\n\n");
	fprintf(fp, "# source INIT DB RECORD\n");
	fprintf(fp, "[ -f \"$IN_FILE\" ] || {\n");
	fprintf(fp, "    echo \"No INIT DB RECORD, \\\"$IN_FILE\\\" found\"\n");
	fprintf(fp, "    exit $FAIL\n");
	fprintf(fp, "}\n\n");
	fprintf(fp, "cd $LLI_ROOT/ID/$DRV_ID\n");
	fprintf(fp, "for i in System Master Node Driver.o\n");
	fprintf(fp, "do\n");
	fprintf(fp, "    [ -f \"$i\" ] || {\n");
	fprintf(fp, "	echo \"$DRV_ID: File, \\\"$LLI_ROOT/ID/$DRV_ID/$i\\\" not found\"\n");
	fprintf(fp, "	exit $FAIL\n");
	fprintf(fp, "    }\n");
	fprintf(fp, "done\n\n");
	fprintf(fp, "# set the system file fields\n");
	fprintf(fp, "PCI_DEV=`stzget $IN_FILE PCI_DEV SELECT`\n");
	fprintf(fp, "if [ \"$?\" != \"0\" ]\n");
	fprintf(fp, "then\n");
	fprintf(fp, "	echo \"$DRV_ID: PCI_DEV: SELECT does not exist in stanza file\"\n");
	fprintf(fp, "	exit $FAIL\n");
	fprintf(fp, "fi\n\n");
	fprintf(fp, "PCI_BUS=`stzget $IN_FILE PCI_BUS SELECT`\n");
	fprintf(fp, "if [ \"$?\" != \"0\" ]\n");
	fprintf(fp, "then\n");
	fprintf(fp, "	echo \"$DRV_ID: PCI_BUS: SELECT does not exist in stanza file\"\n");
	fprintf(fp, "	exit $FAIL\n");
	fprintf(fp, "fi\n\n");
	fprintf(fp, "PCI_FUNC=`stzget $IN_FILE PCI_FUNC SELECT`\n");
	fprintf(fp, "if [ \"$?\" != \"0\" ]\n");
	fprintf(fp, "then\n");
	fprintf(fp, "	echo \"$DRV_ID: PCI_FUNC: SELECT does not exist in stanza file\"\n");
	fprintf(fp, "	exit $FAIL\n");
	fprintf(fp, "fi\n\n");

	/* ----------------------------------------------------------------- */

	for (slistp = basic_list.next ; slistp != &basic_list ; )
	{
		fprintf(fp, "%s=`stzget $IN_FILE %s SELECT` || %s=\"%s\"\n",
			slistp->list_name,
			slistp->list_name,
			slistp->list_name,
			(slistp->default_value != NULL) ? 
				slistp->default_value : "unknown");

		slistp = slistp->next;
	}

	for (slistp = advanced_list.next ; slistp != &advanced_list ; )
	{
		fprintf(fp, "%s=`stzget $IN_FILE %s SELECT` || %s=\"%s\"\n",
			slistp->list_name,
			slistp->list_name,
			slistp->list_name,
			(slistp->default_value != NULL) ? 
				slistp->default_value : "unknown");

		slistp = slistp->next;
	}

	fprintf(fp, "\n");

	/* ----------------------------------------------------------------- */

	fprintf(fp, "# check to see if the driver is in the link-kit\n");
	fprintf(fp, "[ -f $SYSTEM_FILE ] || {\n");
	fprintf(fp, "	idinstall -a -e -k $DRV_ID\n");
	fprintf(fp, "	idinstall -u -k -H $DRV_ID\n");
	fprintf(fp, "}\n\n");

	/* ----------------------------------------------------------------- */

	fprintf(fp, "set_header_info $DRV_ID $BRD_NUM PCI_DEV	$PCI_DEV\n");
	fprintf(fp, "set_header_info $DRV_ID $BRD_NUM PCI_BUS	$PCI_BUS\n");
	fprintf(fp, "set_header_info $DRV_ID $BRD_NUM PCI_FUNC	$PCI_FUNC\n");

	/* ----------------------------------------------------------------- */

	for (slistp = basic_list.next ; slistp != &basic_list ; )
	{
		fprintf(fp, "set_header_info $DRV_ID $BRD_NUM %s     ",
			slistp->list_name);

		switch (slistp->type)
		{
		case UDI_ATTR_UBIT32:
			fprintf(fp, " \"$%s\"\n", slistp->list_name);
			break;

		case UDI_ATTR_BOOLEAN:
			fprintf(fp, " \\\\\\\"\"$%s\"\\\\\\\\\n",
				slistp->list_name);
			break;

		case UDI_ATTR_STRING:
			fprintf(fp, " \\\\\\\"\"$%s\"\\\\\\\"\n",
				slistp->list_name);

			break;
		}

		slistp = slistp->next;
	}

	for (slistp = advanced_list.next ; slistp != &advanced_list ; )
	{
		fprintf(fp, "set_header_info $DRV_ID $BRD_NUM %s     ",
			slistp->list_name);

		switch (slistp->type)
		{
		case UDI_ATTR_UBIT32:
			fprintf(fp, " \"$%s\"\n", slistp->list_name);
			break;

		case UDI_ATTR_BOOLEAN:
			fprintf(fp, " \\\\\\\"\"$%s\"\\\\\\\"\n",
				slistp->list_name);
			break;

		case UDI_ATTR_STRING:
			fprintf(fp, " \\\\\\\"\"$%s\"\\\\\\\"\n",
				slistp->list_name);
			break;
		}

		slistp = slistp->next;
	}

	fprintf(fp, "\n");

	/* ----------------------------------------------------------------- */

	fprintf(fp, "# Set up system file information (sdevice) fields:\n");
	fprintf(fp, "set_system_info $SYSTEM_FILE $BRD_NUM \"Y\" ${SLOT:-0} 0 0 0 0 0 0\n\n");
	fprintf(fp, "exit $OK\n");
}

void
output_net_space_dot_c(
FILE	*fp,
char	*driver_name
)
{
	int			i;
	int			j;
	char			*upper_name;
	struct selection_list	*slistp;
	char			*s;

	upper_name = strdup(driver_name);

	for (s = upper_name ; *s != '\0' ; s++)
	{
		if ((*s >= 'a') && (*s <= 'z'))
		{
			*s = (*s - 'a') + 'A';
		}
	}

	fprintf(fp, "#include \"config.h\"\n\n");
	fprintf(fp, "#include \"space.h\"\n\n");
	fprintf(fp, "struct instance_attribute_entry {\n");
	fprintf(fp, "	char				*attr;\n");
	fprintf(fp, "	unsigned			type;\n");
	fprintf(fp, "	char				*value;\n");
	fprintf(fp, "};\n\n");
	fprintf(fp, "struct instance_info {\n");
	fprintf(fp, "	unsigned			busnum;\n");
	fprintf(fp, "	unsigned			slotnum;\n");
	fprintf(fp, "	unsigned			funcnum;\n");
	fprintf(fp, "	struct instance_attribute_entry	*attr_entry;\n");
	fprintf(fp, "};\n\n");
	fprintf(fp, "struct instance_table {\n");
	fprintf(fp, "	struct instance_info		**instance_info;\n");
	fprintf(fp, "	char				*next;\n");
	fprintf(fp, "};\n\n");

	for (i = 0 ; i < 5 ; i++)
	{
		fprintf(fp, "#ifdef ");

		for (j = 0, s = upper_name ; (j < 5) && (*s != '\0') ; j++, s++)
		{
			fprintf(fp, "%c", *s);
		}

		fprintf(fp, "_%d\n", i);

		fprintf(fp, "struct instance_attribute_entry "
			"%s_%d_instance_attribute_list[] = {\n",
			driver_name, i);

		for (slistp = basic_list.next ; slistp != &basic_list ; )
		{
			fprintf(fp, "	{ ");

			fprintf(fp, "\"%s\", ", slistp->list_name);
			fprintf(fp, "%d, ", slistp->type);

			fprintf(fp, "(char *)%s_%d_%s },\n", upper_name, i,
				slistp->list_name);

			slistp = slistp->next;
		}

		for (slistp = advanced_list.next ; slistp != &advanced_list ; )
		{
			fprintf(fp, "	{ ");

			fprintf(fp, "\"%s\", ", slistp->list_name);
			fprintf(fp, "%d, ", slistp->type);

			fprintf(fp, "(char *)%s_%d_%s },", upper_name, i,
				slistp->list_name);

			slistp = slistp->next;
		}

		fprintf(fp, "	{ (char *)0, 0, (char *)0 }\n");

		fprintf(fp, "};\n");

		fprintf(fp, "struct instance_info %s_%d_instance_info = {\n",
			driver_name, i);
		fprintf(fp, "\t%s_%d_PCI_BUS, ", upper_name, i);
		fprintf(fp, "%s_%d_PCI_DEV, ", upper_name, i);
		fprintf(fp, "%s_%d_PCI_FUNC, ", upper_name, i);
		fprintf(fp, "%s_%d_instance_attribute_list\n", driver_name, i);
		fprintf(fp, "};\n");

		fprintf(fp, "#endif\n\n");
	}

	fprintf(fp, "struct instance_info *%s_instance_info[] = {\n",
		driver_name);

	for (i = 0 ; i < 5 ; i++)
	{
		fprintf(fp, "#ifdef ");

		for (j = 0, s = upper_name ; (j < 5) && (*s != '\0') ; j++, s++)
		{
			fprintf(fp, "%c", *s);
		}

		fprintf(fp, "_%d\n", i);

		fprintf(fp, "\t&%s_%d_instance_info,\n", driver_name, i);

		fprintf(fp, "#endif\n");
	}

	fprintf(fp, "\t(struct instance_info *)0\n");

	fprintf(fp, "};\n");

	fprintf(fp, "struct instance_table %s_instance_table= {\n",
		driver_name);

	fprintf(fp, "\t%s_instance_info, ", driver_name);

	fprintf(fp, "(char *)0\n");

	fprintf(fp, "};\n");

	free(upper_name);
}

void
output_net_space_dot_h(
FILE	*fp,
char	*driver_name
)
{
	char			*upper_name;
	struct selection_list	*slistp;
	char			*s;
	int			i;

	upper_name = strdup(driver_name);

	for (s = upper_name ; *s != '\0' ; s++)
	{
		if ((*s >= 'a') && (*s <= 'z'))
		{
			*s = (*s - 'a') + 'A';
		}
	}

	for (i = 0 ; i < 5 ; i++)
	{
		fprintf(fp, "#define %s_%d_PCI_DEV 0\n", upper_name, i);
		fprintf(fp, "#define %s_%d_PCI_BUS 0\n", upper_name, i);
		fprintf(fp, "#define %s_%d_PCI_FUNC 0\n", upper_name, i);

		for (slistp = basic_list.next ; slistp != &basic_list ; )
		{
			fprintf(fp, "#define %s_%d_%s",
				upper_name,
				i,
				slistp->list_name);

			switch (slistp->type)
			{
			case UDI_ATTR_UBIT32:
				fprintf(fp, " %s\n",
					(slistp->default_value != NULL) ? 
					slistp->default_value : "0");
				break;

			case UDI_ATTR_BOOLEAN:
				fprintf(fp, " \"%s\"\n",
					(slistp->default_value != NULL) ? 
					slistp->default_value : "0");
				break;

			case UDI_ATTR_STRING:
				fprintf(fp, " \"%s\"\n",
					(slistp->default_value != NULL) ? 
					slistp->default_value : "unknown");
				break;
			}

			slistp = slistp->next;
		}

		for (slistp = advanced_list.next ; slistp != &advanced_list ; )
		{
			fprintf(fp, "#define %s_%d_%s",
				upper_name,
				i,
				slistp->list_name);

			switch (slistp->type)
			{
			case UDI_ATTR_UBIT32:
				fprintf(fp, " %s\n",
					(slistp->default_value != NULL) ? 
					slistp->default_value : "0");
				break;

			case UDI_ATTR_BOOLEAN:
				fprintf(fp, " \"%s\"\n",
					(slistp->default_value != NULL) ? 
					slistp->default_value : "0");
				break;

			case UDI_ATTR_STRING:
				fprintf(fp, " \"%s\"\n",
					(slistp->default_value != NULL) ? 
					slistp->default_value : "unknown");
				break;
			}

			slistp = slistp->next;
		}

		fprintf(fp, "\n");
	}

	free(upper_name);
}
