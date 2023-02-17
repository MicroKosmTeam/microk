#include 	<stdio.h>

#include <udi_env.h>
#include "obj_osdep.h"                  /* from tools */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <udi_std_sprops.h>
  

#if !OVERRIDE_ELF_SPROPS
#include	<libelf/libelf.h>
#include	"obj_osdep.h"

char	elf_class;

/*
 * Operations related to the elf object file header
 */

/* Return a ptr to the file header */
void *
elf_getehdr (Elf *elf)
{
	if (elf_class == ELFCLASS32) {
		return(elf32_getehdr(elf));
	} else if (elf_class == ELFCLASS64) {
		return(elf64_getehdr(elf));
	}
	return (0);
}

/* Return the section index of the section header string table */
int
elf_geteshstrndx (void *ehdr)
{
	if (elf_class == ELFCLASS32) {
		return(((Elf32_Ehdr *)(ehdr))->e_shstrndx);
	} else if (elf_class == ELFCLASS64) {
		return(((Elf64_Ehdr *)(ehdr))->e_shstrndx);
	}
	return (0);
}


/*
 * Operations related to the elf section headers
 */

/* Return a pointer to the elf section header */
void *
elf_getshdr (Elf_Scn *scn)
{
	if (elf_class == ELFCLASS32) {
		return(elf32_getshdr(scn));
	} else if (elf_class == ELFCLASS64) {
		return(elf64_getshdr(scn));
	}
	return (0);
}

/* Return the index in the section header string table, for the section name */
size_t
elf_getsname (void *shdr)
{
	if (elf_class == ELFCLASS32) {
		return(((Elf32_Shdr *)(shdr))->sh_name);
	} else if (elf_class == ELFCLASS64) {
		return(((Elf64_Shdr *)(shdr))->sh_name);
	}
	return (0);
}

/* Set the section name, an index into the section header string table */
void
elf_setsname (void *shdr, int ndx)
{
	if (elf_class == ELFCLASS32) {
		((Elf32_Shdr *)(shdr))->sh_name = ndx;
	} else if (elf_class == ELFCLASS64) {
		((Elf64_Shdr *)(shdr))->sh_name = ndx;
	}
}

#endif

#if 1 /*OVERRIDE_ELF_SPROPS*/

/*
 * Given the name of an object file (either the Posix executable or
 * the name of the module) and the driver object name this routine
 * attempts to find the associated properties block for that module
 * by getting the .udiprops section from the object file and then
 * finding the properties section for the specified driver.
 *
 * Note that the linking stage for the various modules in the Posix
 * target will concatenate the .udiprops sections together.  This code
 * assumes:
 * 1) The udibuild has pre-processed the udiprops.txt file to create
 *    the .udiprops section so that the .udiprops is well formatted and
 *    highly regular (e.g. no comments or extraneous whitespace, etc.).
 * 2) Each property is a null terminated string within the block
 * 3) All properties for a module are locally clustered and the
 *    "properties_version" property is the separator between modules.
 * 4) The correct properties for the specified driver can be determined
 *    by comparing the driver name to the module or shortname properties.
 */

#include <mach-o/loader.h>
#include <mach-o/getsect.h>
#include <sys/stat.h>

/* These defs had better mirror what's in tools/darwin/set_abi_mach_o.c */
#define SEG_UDIPROPS "__UDIPROPS"
#define SECT_UDIPROPS "__udiprops"

_udi_std_sprops_t *   
_udi_get_elf_sprops(const char *exec_img_file,
                    const char *driver_obj_file,
                    const char *driver_name);

#define propeq_start if (*cp == 0) { continue 
#define propeq(S) } else if (udi_strncmp(cp,(S),udi_strlen(S)) == 0) { \
                                cp += udi_strlen(S);
#define propeq_end }

char *get_shortname_from_props(_udi_std_sprops_t *sprops)
{
	char *cp;
	for (cp = sprops->props_text;
		cp < sprops->props_text + sprops->props_len;
		cp += udi_strlen(cp) + 1) {
		propeq_start;
		propeq("shortname ");
		return cp;
		propeq_end;
	}
	return NULL;
}

int match_shortname(_udi_std_sprops_t *sprops, char *shortname)
{
	char *cp;
	cp = get_shortname_from_props(sprops);
	if (!cp)
		return 0;
	if (udi_strcmp(cp + udi_strlen("shortname "), shortname) == 0)
		return 1;
	return 0;
}

void convert_sprops_to_omfified(const char *sprops_orig, const char *sprops_omfified)
{
}

_udi_std_sprops_t *   
_udi_get_elf_sprops(const char *exec_img_file,
                    const char *driver_obj_file,
                    const char *driver_name)
{
	_udi_std_sprops_t *sprops;
        FILE *objfile;
        struct stat sb;
        char *srcdata, *sectdata;
        struct section thissect;
        unsigned long size;
        const char *ofname;
	int len;

        ofname = driver_obj_file;       
        if (ofname) {
                objfile = fopen(driver_obj_file, "rb");
        } else {  
                objfile = NULL;
        }
        if (objfile == NULL) {
                ofname = exec_img_file;
                objfile = fopen(exec_img_file, "rb");
                if (objfile == NULL) {
                        fprintf(stderr,
                                "Error: couldn't open %s or %s to obtain "
                                "static properties for %s\n", exec_img_file,
                                driver_obj_file, driver_name);   
                        exit(ENOENT); 
                }
        }

        /* Read in the entire binary */ 
        if (stat(ofname, &sb) != 0) 
                perror("Failed stat ofname");
        srcdata = malloc(sb.st_size);
        assert(srcdata);
        size = fread(srcdata, 1, sb.st_size, objfile);
        assert(size == sb.st_size);

        /* Copy out the section info we want. */
        memcpy((void*)&thissect,
                        getsectbynamefromheader((struct mach_header*)srcdata,
                                                 SEG_UDIPROPS, SECT_UDIPROPS),
                        sizeof(thissect));
        if (0)
        {
                fprintf(stderr, "MACH-O SECTION INFO:   addr:%ld   size:%ld  offset: %ld\n",
                        thissect.addr,
                        thissect.size,
                        thissect.offset);
        }
        sectdata = ((char*)srcdata) + thissect.offset;
	/*fwrite(sectdata, 1, thissect.size, stderr);*/

	len = thissect.size;
	sprops = udi_std_sprops_parse(sectdata,
					thissect.size,
					driver_name);
	free(srcdata);
	fclose(objfile);
	return sprops;
}

#endif

