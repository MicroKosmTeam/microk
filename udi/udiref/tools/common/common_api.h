/*
 * File: tools/common/common_api.h
 *
 * Common definitions used in the UDI tools
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

#include "common.h"		/* Localisation definitions */

/* common.c functions */
extern char *optinit(void);
extern void optcpy(char **dst, char *src);
extern void optncpy(char **dst, char *src, int num);
extern void optcat(char **dst, char *src);
extern void optncat(char **dst, char *src, int num);
extern void get_cfg(void);
extern void make_args(char *str, int *cnt);
extern char *get_ccfiles(char *modname, int include_h_files);
extern char *get_ccopts(char *modname, char *file);
extern void do_remove(char *start, int fail);
extern void do_copy(char *src, char *dst);

/* udisetup.c functions */
extern void write_props_glue(FILE *fp, propline_t **data, int install_src);
extern void antiparse(FILE *fp, char *orig_str);

/* spparse.y functions */
extern void do_udiparse(char *propfile);

/* OS specific functions called from common code */
extern void osdep_mkinst_files(int install_src, propline_t **inp_data,
				    char *basedir, char *tmpdir);
extern void get_mod_type(char *modtype);

/* object manipulation functions [set_abi.c] */
extern void set_abi_sprops(char *src, char *dst);
extern void get_abi_sprops(char *src, char *dst);
extern void abi_add_section_sizes(char *objfile, char *modname,
				  unsigned int *ttl_code_size,
				  unsigned int *ttl_data_size);

extern void abi_make_string_table_sprops(char *srcfile, void **strtable_sprops,
                                        size_t *strtable_size);

extern void prepare_elf_object(const char *fname, const char *modname, 
			       const char **symbols);


#define ABI_SIZE_WARN_THRESH 0.80   /* Threshold percentage for ABI
				     * section sizes above which
				     * complaints will be
				     * generated. */

