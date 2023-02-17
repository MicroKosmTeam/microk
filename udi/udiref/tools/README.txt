#
# File: tools/README.txt
#
# Notes for UDI Tools
#

#
# $Copyright udi_reference:
# 
# 
#    Copyright (c) 1995-2001; Compaq Computer Corporation; Hewlett-Packard
#    Company; Interphase Corporation; The Santa Cruz Operation, Inc;
#    Software Technologies Group, Inc; and Sun Microsystems, Inc
#    (collectively, the "Copyright Holders").  All rights reserved.
# 
#    Redistribution and use in source and binary forms, with or without
#    modification, are permitted provided that the conditions are met:
# 
#            Redistributions of source code must retain the above
#            copyright notice, this list of conditions and the following
#            disclaimer.
# 
#            Redistributions in binary form must reproduce the above
#            copyright notice, this list of conditions and the following
#            disclaimers in the documentation and/or other materials
#            provided with the distribution.
# 
#            Neither the name of Project UDI nor the names of its
#            contributors may be used to endorse or promote products
#            derived from this software without specific prior written
#            permission.
# 
#    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#    "AS IS," AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#    HOLDERS OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
#    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
#    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
#    TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
#    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
#    DAMAGE.
# 
#    THIS SOFTWARE IS BASED ON SOURCE CODE PROVIDED AS A SAMPLE REFERENCE
#    IMPLEMENTATION FOR VERSION 1.01 OF THE UDI CORE SPECIFICATION AND/OR
#    RELATED UDI SPECIFICATIONS. USE OF THIS SOFTWARE DOES NOT IN AND OF
#    ITSELF CONSTITUTE CONFORMANCE WITH THIS OR ANY OTHER VERSION OF ANY
#    UDI SPECIFICATION.
# 
# 
# $
#

			Notes for UDI Tools
			-------------------

What follows, is a explanation of the UDI Tools, the options, and
configuration files.

By default, the tools look in /usr/include/udi for the UDI public
include files.  If you are working from a local development tree, use
UDI_PUB_INCLUDE to specify an alternate directory to be used to locate
this information.

The tools know pretty much everything to operate in your particular
environment.  There aren't many ways to modify the normal operation of
the tools other than the following:

  * You can use the UDI_PUB_INCLUDE to modify where the UDI public
    include files are found
  * You can set define/undefine macros via the compile_options
    declaration in the static properties file (udiprops.txt)
  * You can specify command line arguments to the tools
  * You can change your path to determine where invoked tools such as
    the compiler are located.
  * Some of the tools will use environment variables to obtain
    parameters: these are all documented by the tools usage
    information.

Issue a -h to any of the tools to get help and usage information.

The udibuild tool builds driver modules from source files.
This command is typically run from a source file directory, or
run by udisetup to build a source distributed driver.


The udimkpkg tool is used to assemble the driver source code and
related files into a conforming UDI package directory structure.


The udisetup tool is used to install a udimkpkg packaged driver onto
the native system.

