/*
 * File: tools/common/param.h
 *
 * UDI Utilities Typical Parameter Handling Definitions
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

#ifndef _udi_typ_param_h
#define _udi_typ_param_h

      /* Typical parameter management for the UDI utilities involves
       * default behavior and values, environment variable overrides,
       * and command-line overrides.  This module implements a standard
       * U*ix based "-ARG" management of these parameters using getopt, etc.
       *
       * By convention, this file is compiled as part of the OS-specific
       * tool build process.  Each parameter can have an OS-specific default
       * value which may be overridden by an environment variable which may
       * be overridden in-turn by a command line option value.
       *
       * All OS-specific values are specified in the OS-specific portion of
       * the tools directory before building this file.  If OS-specific
       * defaults are not supplied, generic UDI defaults will be used.
       *
       * All OS-specific values are supplied as strings.
       *
       * If the paramtype is Integer, strtol(os-string,0,0) is used to
       * convert the string to the integer value.
       *
       * If the paramtype is Flag then the OS-specific default is
       * ignored.  The flag value is set to 0 unless the environment
       * variable is defined (value is ignored) or the flag is
       * specified on the command line, in which case the flag value
       * will be set to one.
       *
       */



/*
 * Global Variables common to all tools
 */


typedef enum {
    Flag,                    /* boolean: present or not */
    Integer,                 /* (signed) integer value */
    NamedIndex,              /* user param is a string; internal value is
			      * an integer index into the valid values array */
    String,                  /* miscellaneous string */
    ExistingFile,            /* file (must exist; string subtype) */
    ExistingFiles,           /* zero or more files (must exist;string subtype)*/
    ExistingDirectory,       /* directory (must exist; string subtype) */
    NewDirectory,            /* directory (must not exist; string subtype) */
    ExistingOrNewDirectory   /* directory (may exist; string subtype) */
} udiutil_paramtype_t;

typedef struct udiutil_param_s {
    void *p_pvar;                /* Pointer to variable to hold this param */

#define PARAM_INT(ParamPtr)            *((int *)((ParamPtr)->p_pvar))
#define PARAM_STR(ParamPtr)            *((char **)((ParamPtr)->p_pvar))
#define SET_PARAM_INT(ParamPtr,IntVal) *((int *)((ParamPtr)->p_pvar)) = IntVal;
#define SET_PARAM_STR(ParamPtr,StrVal)			\
    if (StrVal) {					\
        optcpy((char **)((ParamPtr)->p_pvar), StrVal);	\
    } else {						\
	*((char **)(ParamPtr)->p_pvar) = optinit();	\
    }
#define SET_PARAM_STRADD(ParamPtr,StrVal)			\
    if (StrVal) {						\
        optcatword((char **)((ParamPtr)->p_pvar), StrVal);	\
    } else {							\
	*((char **)(ParamPtr)->p_pvar) = optinit();		\
    }

    udiutil_paramtype_t ptype;   /* Type for this param */
    char *pdef;                  /* (OS-specific) default for this param */
    char *penvvar;               /* Name of Environment Variable override */
    char *pcmdline;              /* Name of Command Line Parameter override */
    char *phelp;                 /* Pointer to help info for this parameter */
    char **pvalid;               /* Pointer to (null-terminated) array
				  * of strings of valid values */
} udiutil_param_t;


extern udiutil_param_t udiutil_params[];


/*
 * Miscellaneous
 */

void UDI_Util_Init_Params(void);
void UDI_Util_Get_Params(int argc, char **argv);


#endif /* _udi_typ_param_h */

