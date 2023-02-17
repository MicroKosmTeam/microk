/
/ $Copyright udi_reference:
/ 
/ 
/    Copyright (c) 1995-2001; Compaq Computer Corporation; Hewlett-Packard
/    Company; Interphase Corporation; The Santa Cruz Operation, Inc;
/    Software Technologies Group, Inc; and Sun Microsystems, Inc
/    (collectively, the "Copyright Holders").  All rights reserved.
/ 
/    Redistribution and use in source and binary forms, with or without
/    modification, are permitted provided that the conditions are met:
/ 
/            Redistributions of source code must retain the above
/            copyright notice, this list of conditions and the following
/            disclaimer.
/ 
/            Redistributions in binary form must reproduce the above
/            copyright notice, this list of conditions and the following
/            disclaimers in the documentation and/or other materials
/            provided with the distribution.
/ 
/            Neither the name of Project UDI nor the names of its
/            contributors may be used to endorse or promote products
/            derived from this software without specific prior written
/            permission.
/ 
/    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
/    "AS IS," AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
/    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
/    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
/    HOLDERS OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
/    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
/    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
/    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
/    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
/    TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
/    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
/    DAMAGE.
/ 
/    THIS SOFTWARE IS BASED ON SOURCE CODE PROVIDED AS A SAMPLE REFERENCE
/    IMPLEMENTATION FOR VERSION 1.01 OF THE UDI CORE SPECIFICATION AND/OR
/    RELATED UDI SPECIFICATIONS. USE OF THIS SOFTWARE DOES NOT IN AND OF
/    ITSELF CONSTITUTE CONFORMANCE WITH THIS OR ANY OTHER VERSION OF ANY
/    UDI SPECIFICATION.
/ 
/ 
/ $
/
/ File: osdep_pio_iout_i386.s
/
/ Purpose: Provide inb(), inw(), inl(), outb(), outw(), outl() functions
/          for Solaris 2.8 x86 using the ProWorks compiler by SunPro
/

.file "osdep_pio_iout_i386.s"
.text

.globl  inb
.globl  inw
.globl  inl
.globl  outb
.globl  outw
.globl  outl

/
/ unsigned char inb(int port);
/
.align	4
inb:
        movl 4(%esp),%edx
        subl %eax,%eax
        inb  (%dx)
		ret
.type	inb,@function
.size	inb,.-inb

/
/ unsigned short inw(int port);
/
.align	4
inw:
        movl 4(%esp),%edx
        subl %eax,%eax
        inw  (%dx)
		ret
.type	inw,@function
.size	inw,.-inw

/
/ unsigned long inl(int port);
/
.align	4
inl:
        movl 4(%esp),%edx
        inl  (%dx)
		ret
.type	inl,@function
.size	inl,.-inl

/
/     void outb(int port, unsigned char value);
/
.align	4
outb:
        movl 4(%esp),%edx
        movl 8(%esp),%eax
        outb (%dx)
		ret
.type	outb,@function
.size	outb,.-outb

/
/     void outw(int port, unsigned short value);
/
.align	4
outw:
        movl 4(%esp),%edx
        movl 8(%esp),%eax
        outw (%dx)
	    ret
.type	outw,@function
.size	outw,.-outw

/
/     void outl(int port, unsigned long value);
/
.align	4
outl:
        movl 4(%esp),%edx
        movl 8(%esp),%eax
        outl (%dx)
	    ret
.type	outl,@function
.size	outl,.-outl
