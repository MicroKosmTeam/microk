#
# File: mapper/README.txt
#
# Implementation Notes for External Mappers
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


		Implementation Notes for External Mappers
		-----------------------------------------

External mappers are used in the I/O path to bridge between the UDI
execution environment and a surrounding "native" environment. These
mappers act in some ways like UDI drivers, but in other ways they act
like native device drivers.

While there are a number of alternatives, the recommended design is to
have the mappers be as "UDI-ish" as possible and only access native
structures and APIs where absolutely necessary. This has three
advantages.

The first advantage is performance. Since requests/responses are sent
from the UDI side as channel operations, it is best to have the UDI side
of the mapper run as a UDI region, so the IMC code that sends operations
over channels doesn't need to take the performance cost of making a test
to special-case mappers. Further, while it would work to have a single
region for a mapper regardless of how many device instances on behalf of
which it is acting, better scalability will be achieved by having a
separate region per instance, as with a real UDI driver.

The second advantage is portability. The less "native" code in a mapper,
the more the remaining code can be leveraged for ports of the mapper to
other operating systems.

The third advantage is reusability. If it is subsequently decided to
move the "UDI line" further up the I/O stack, including the next layer
of drivers as full UDI drivers, then much of the mapper code can be
reused as part of such drivers.
