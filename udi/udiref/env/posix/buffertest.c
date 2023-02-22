
/*
 * File: env/posix/buffertest.c
 *
 * Verify that simple buffer management plumbing works.   
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

#include <stdlib.h>
#include <unistd.h>

#include <udi_env.h>
#include <posix.h>

/* #define SINGLE_THREADED_TESTS 1 */

#ifndef SINGLE_THREADED_TESTS
#define	SINGLE_THREADED_TESTS 0
#endif /* SINGLE_THREADED_TESTS */

#define MAXBUFS 32
#define NUMTAGS 2
#define BUF_TAGS_USED	2

#define SUCCESS  0
#define FAILMEM  1
#define FAILSIZ  2

char memory[4166];
char chkmem[4096];
char bufmem[4096];
udi_buf_t *buffers[MAXBUFS];

int n_buf_allocs = 0;
int n_cb_alloc = 0;
int n_buftag_tests = 0;
int n_buf_tests = MAXBUFS;
_udi_buf_path_t buf_path;

void cballoc(udi_cb_t *gcb,
	     udi_cb_t *new_cb);

udi_ubit32_t
calc_checksum(udi_ubit8_t *mem,
	      int len)
{
	int last;
	int i;
	udi_ubit32_t sum;

	if (len % 2)
		last = len - 1;
	else
		last = len;

	sum = 0;
	for (i = 0; i < last; i += 2) {
		sum += ((udi_ubit16_t)0x100 * (udi_ubit16_t)(*(mem + i)) +
			(udi_ubit16_t)(*(mem + i + 1)));
		if (sum > 0xffff)
			sum -= 0xffff;
	}
	if (last != len) {
		sum += (udi_ubit16_t)0x100 *(udi_ubit16_t)(*(mem + len - 1));

		if (sum > 0xffff)
			sum -= 0xffff;
	}
	sum = (~sum & 0x0000ffff);
	return (sum);
}

void
printmemory(unsigned char *mem,
	    int length,
	    int num,
	    int seg,
	    int offset)
{
	unsigned char *ptr = mem;
	int count = 0;

	printf("    %p:(offset = %d)", mem, offset);
	while (count < length) {
		if (count % 70 == 0)
			printf("\n    %d:%d ", num, seg);
		if (*ptr >= ' ' && *ptr <= '~')
			printf("%c", *ptr);
		else {
			if (*ptr == 0xca)
				printf("?");
			else
				printf(".");
		}
		ptr++;
		count++;
	}
	printf("\n");
}

int
my_memcmp(char *bufmem,
	  char *chkmem,
	  int chkbytes)
{
	int i;

	for (i = 0; i < chkbytes; i++) {
		if (*(bufmem + i) != *(chkmem + i))
			return 0;
	}
	return 1;
}

void
dumpbuffer(int num)
{
	_udi_buf_dataseg_t *dataseg;
	_udi_buf_memblk_t *memblk;
	udi_size_t bytes, pbytes;
	int seg, offset, status;
	static int buffercheck = 0;
	udi_size_t chkbytes = 0;
	udi_size_t chkpbytes = 0;
	udi_buf_tag_t *buftag;
	_udi_buf_tag_t *tag;
	int ntag;

	_udi_buf_t *buffer = (_udi_buf_t *)buffers[num];
	void *bc;

	pbytes = buffer->buf_total_size;
	printf("Buffers[%d]: %p\n", num, buffer);
	printf("  %d: buf.buf_size     = %d\n", num, (udi_ubit32_t) buffer->buf.buf_size);
	printf("  %d: *buf_contents    = %p\n", num, buffer->buf_contents);
	printf("  %d: *buf_ops         = %p\n", num, buffer->buf_ops);
	printf("  %d: *buf_path        = %p\n", num, buffer->buf_path);
	printf("  %d: buf_total_size   = %d\n", num, (udi_ubit32_t) buffer->buf_total_size);
	printf("  %d: *buf_tags        = %p\n", num, buffer->buf_tags);
	ntag = 0;
	tag = buffer->buf_tags;
	while (tag != NULL) {
		buftag = (udi_buf_tag_t *)tag;
		printf("    tag[%d] = %p\n", ntag, buffer->buf_tags);
		printf("      bt_v.tag_type   = 0x%x\n", buftag->tag_type);
		printf("      bt_v.tag_value  = 0x%x\n", buftag->tag_value);
		printf("      bt_v.tag_off    = %d\n", (udi_ubit32_t) buftag->tag_off);
		printf("      bt_v.tag_len    = %d\n", (udi_ubit32_t) buftag->tag_len);
		printf("      bt_drv_instance = %p\n", tag->bt_drv_instance);
		printf("      bt_next         = %p\n", tag->bt_next);
		tag = tag->bt_next;
		ntag++;
	}
	printf("  %d: buf_refcnt       = %p (%d)\n", num, &buffer->buf_refcnt,
	       _OSDEP_ATOMIC_INT_READ(&buffer->buf_refcnt));
	dataseg = (_udi_buf_dataseg_t *)buffer->buf_contents;
	/*
	 * Dump the data for each data segment 
	 */
	seg = offset = 0;
	while (dataseg != NULL) {
		printf("  %d:%d %p\n", num, seg, dataseg);
		printf("  %d:%d *bd_start  = %p\n", num, seg,
		       dataseg->bd_start);
		printf("  %d:%d *bd_end    = %p\n", num, seg, dataseg->bd_end);
		printf("  %d:%d (end-start)= %d\n", num, seg,
		       dataseg->bd_end - dataseg->bd_start);
		printf("  %d:%d *bd_memblk = %p\n", num, seg,
		       dataseg->bd_memblk);
		bc = dataseg->bd_container;
		printf("  %d:%d *bd_container = %p ", num, seg, bc);
		if (&((_udi_buf_container1_t *) bc)->bc_dataseg == dataseg) {
			printf("(container1)\n");
		} else if (&((_udi_buf_container2_t *) bc)->bc_dataseg ==
			   dataseg) {
			printf("(container2)\n");
		} else if (&((_udi_buf_container3_t *) bc)->bc_dataseg ==
			   dataseg) {
			printf("(container3)\n");
		} else {
			printf("(unknown container type)\n");
		}
		printf("  %d:%d *bd_next   = %p\n", num, seg,
		       dataseg->bd_next);
		memblk = dataseg->bd_memblk;
		printf("  %d:%d:m %p\n", num, seg, memblk);
		printf("  %d:%d:m bm_refcnt     = %p (%d)\n", num, seg,
		       &memblk->bm_refcnt,
		       _OSDEP_ATOMIC_INT_READ(&memblk->bm_refcnt));
		printf("  %d:%d:m *bm_space     = %p\n", num, seg,
		       memblk->bm_space);
		printf("  %d:%d:m bm_size       = %d\n", num, seg,
		       (udi_ubit32_t) memblk->bm_size);
		printf("  %d:%d:m *bm_external  = %p\n", num, seg,
		       memblk->bm_external);
		if (memblk->bm_external)
			printf("  %d:%d:+ *bc_xmem_context = %p\n", num, seg,
			       (void *)(memblk + 1));
		printmemory((unsigned char *)dataseg->bd_start,
			    (int)(dataseg->bd_end - dataseg->bd_start), num,
			    seg, offset);
		seg++;
		offset += (dataseg->bd_end - dataseg->bd_start);
		dataseg = dataseg->bd_next;
	}
	status = SUCCESS;
	udi_memset(&chkmem[0], 0, 4096);
	udi_memset(&bufmem[0], 0, 4096);

	/*
	 * Set the number of bytes to read in 
	 */
	bytes = buffers[num]->buf_size;

	switch (buffercheck) {
	case 0:
		/*
		 * TEST: allocate an empty buffer 
		 */
		udi_buf_read(buffers[num], 0, bytes, &bufmem);
		chkbytes = 210;
		chkpbytes = 0;
		break;
	case 1:
		/*
		 * TEST: allocate an initialized buffer 
		 */
		udi_memcpy(&chkmem[0], &memory[0], 140);
		udi_buf_read(buffers[num], 0, bytes, &bufmem);
		chkbytes = 140;
		chkpbytes = 140;
		if (my_memcmp(bufmem, chkmem, chkbytes) == 0)
			status = FAILMEM;
		break;
	case 2:
		/*
		 * TEST: allocate an empty buffer 
		 */
		udi_buf_read(buffers[num], 0, bytes, &bufmem);
		chkbytes = 210;
		chkpbytes = 0;
		break;
	case 3:
		/*
		 * TEST: INSERT at beginning of an inited buffer 
		 */
		udi_memcpy(&chkmem[0], &memory[140], 70);
		udi_memcpy(&chkmem[70], &memory[0], 70);
		udi_memcpy(&chkmem[140], &memory[70], 70);
		udi_buf_read(buffers[num], 0, bytes, &bufmem);
		chkbytes = 210;
		chkpbytes = 210;
		if (my_memcmp(bufmem, chkmem, chkbytes) == 0)
			status = FAILMEM;
		break;
	case 4:
		/*
		 * TEST: INSERT at beginning of an uninited buffer 
		 */
		udi_memcpy(&chkmem[0], &memory[140], 70);
		udi_buf_read(buffers[num], 0, bytes, &bufmem);
		chkbytes = 280;
		chkpbytes = 70;
		if (my_memcmp(bufmem, chkmem, chkpbytes) == 0)
			status = FAILMEM;
		break;
	case 5:
		/*
		 * TEST: INSERT in middle of buffer at end of segment 
		 */
		udi_memcpy(&chkmem[0], &memory[140], 70);
		udi_memcpy(&chkmem[70], &memory[0], 70);
		udi_memcpy(&chkmem[140], &memory[210], 70);
		udi_memcpy(&chkmem[210], &memory[70], 70);
		udi_buf_read(buffers[num], 0, bytes, &bufmem);
		chkbytes = 280;
		chkpbytes = 280;
		if (my_memcmp(bufmem, chkmem, chkbytes) == 0)
			status = FAILMEM;
		break;
	case 6:
		/*
		 * TEST: INSERT in middle of buffer at middle of segment 
		 */
		udi_memcpy(&chkmem[0], &memory[140], 70);
		udi_memcpy(&chkmem[70], &memory[0], 35);
		udi_memcpy(&chkmem[105], &memory[280], 70);
		udi_memcpy(&chkmem[175], &memory[0], 35);
		udi_memcpy(&chkmem[210], &memory[210], 70);
		udi_memcpy(&chkmem[280], &memory[70], 70);
		udi_buf_read(buffers[num], 0, bytes, &bufmem);
		chkbytes = 350;
		chkpbytes = 350;
		if (my_memcmp(bufmem, chkmem, chkbytes) == 0)
			status = FAILMEM;
		break;
	case 7:
		/*
		 * TEST: INSERT in middle of buffer at beginning of segment 
		 */
		udi_memcpy(&chkmem[0], &memory[140], 70);
		udi_memcpy(&chkmem[70], &memory[350], 70);
		udi_memcpy(&chkmem[140], &memory[0], 35);
		udi_memcpy(&chkmem[175], &memory[280], 70);
		udi_memcpy(&chkmem[245], &memory[0], 35);
		udi_memcpy(&chkmem[280], &memory[210], 70);
		udi_memcpy(&chkmem[350], &memory[70], 70);
		udi_buf_read(buffers[num], 0, bytes, &bufmem);
		chkbytes = 420;
		chkpbytes = 420;
		if (my_memcmp(bufmem, chkmem, chkbytes) == 0)
			status = FAILMEM;
		break;
	case 8:
		/*
		 * TEST: INSERT in middle of buffer at end of segment spanning segment 
		 */
		udi_memcpy(&chkmem[0], &memory[140], 70);
		udi_memcpy(&chkmem[70], &memory[350], 70);
		udi_memcpy(&chkmem[140], &memory[0], 35);
		udi_memcpy(&chkmem[175], &memory[280], 70);
		udi_memcpy(&chkmem[245], &memory[0], 35);
		udi_memcpy(&chkmem[280], &memory[210], 70);
		udi_memcpy(&chkmem[350], &memory[70], 50);
		udi_memcpy(&chkmem[400], &memory[420], 70);
		udi_memcpy(&chkmem[470], &memory[70], 20);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 490;
		chkpbytes = 490;
		if (my_memcmp(bufmem, chkmem, chkbytes) == 0)
			status = FAILMEM;
		break;
	case 9:
		/*
		 * TEST: OVERWRITE at beginning of buffer at beginning of segment 
		 */
		udi_memcpy(&chkmem[0], &memory[0], 10);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 210;
		chkpbytes = 10;
		if (my_memcmp(bufmem, chkmem, 10) == 0)
			status = FAILMEM;
		break;
	case 10:
		/*
		 * TEST: OVERWRITE at beginning of buffer at middle of segment 
		 */
		udi_memcpy(&chkmem[0], &memory[0], 10);
		udi_memcpy(&chkmem[10], &memory[70], 10);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 210;
		chkpbytes = 20;
		if (my_memcmp(bufmem, chkmem, 20) == 0)
			status = FAILMEM;
		break;
	case 11:
		/*
		 * TEST: OVERWRITE at end of buffer at end of segment 
		 */
		udi_memcpy(&chkmem[0], &memory[0], 10);
		udi_memcpy(&chkmem[10], &memory[70], 10);
		udi_memcpy(&chkmem[200], &memory[140], 10);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 210;
		chkpbytes = 210;
		if (my_memcmp(bufmem, chkmem, 20) == 0 ||
		    my_memcmp(&bufmem[200], &chkmem[200], 10) == 0)
			status = FAILMEM;
		break;
	case 12:
		/*
		 * TEST: OVERWRITE at end of buffer at end of segment, extending buffer 15 bytes 
		 */
		udi_memcpy(&chkmem[0], &memory[0], 10);
		udi_memcpy(&chkmem[10], &memory[70], 10);
		udi_memcpy(&chkmem[200], &memory[140], 5);
		udi_memcpy(&chkmem[205], &memory[210], 20);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 225;
		chkpbytes = 225;
		if (my_memcmp(bufmem, chkmem, 20) == 0 ||
		    my_memcmp(&bufmem[200], &chkmem[200], 25) == 0)
			status = FAILMEM;
		break;
	case 13:
		/*
		 * TEST: DELETE at end of buffer at end of segment 
		 */
		udi_memcpy(&chkmem[0], &memory[140], 70);
		udi_memcpy(&chkmem[70], &memory[350], 70);
		udi_memcpy(&chkmem[140], &memory[0], 35);
		udi_memcpy(&chkmem[175], &memory[280], 70);
		udi_memcpy(&chkmem[245], &memory[0], 35);
		udi_memcpy(&chkmem[280], &memory[210], 70);
		udi_memcpy(&chkmem[350], &memory[70], 50);
		udi_memcpy(&chkmem[400], &memory[420], 70);
		udi_memcpy(&chkmem[470], &memory[70], 10);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 480;
		chkpbytes = 480;
		if (my_memcmp(bufmem, chkmem, chkbytes) == 0)
			status = FAILMEM;
		break;
	case 14:
		/*
		 * TEST: DELETE at middle of buffer at middle of segment 
		 */
		udi_memcpy(&chkmem[0], &memory[140], 70);
		udi_memcpy(&chkmem[70], &memory[350], 70);
		udi_memcpy(&chkmem[140], &memory[0], 35);
		udi_memcpy(&chkmem[175], &memory[280], 70);
		udi_memcpy(&chkmem[245], &memory[0], 35);
		udi_memcpy(&chkmem[280], &memory[210], 70);
		udi_memcpy(&chkmem[350], &memory[70], 40);
		udi_memcpy(&chkmem[390], &memory[420], 60);
		udi_memcpy(&chkmem[450], &memory[70], 10);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 460;
		chkpbytes = 460;
		if (my_memcmp(bufmem, chkmem, chkbytes) == 0)
			status = FAILMEM;
		break;
	case 15:
		/*
		 * TEST: DELETE at middle of buffer across segments 
		 */
		udi_memcpy(&chkmem[0], &memory[140], 70);
		udi_memcpy(&chkmem[70], &memory[350], 70);
		udi_memcpy(&chkmem[140], &memory[0], 35);
		udi_memcpy(&chkmem[175], &memory[280], 70);
		udi_memcpy(&chkmem[245], &memory[0], 35);
		udi_memcpy(&chkmem[280], &memory[210], 60);
		udi_memcpy(&chkmem[340], &memory[420], 40);
		udi_memcpy(&chkmem[380], &memory[70], 10);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 390;
		chkpbytes = 390;
		if (my_memcmp(bufmem, chkmem, chkbytes) == 0)
			status = FAILMEM;
		break;
	case 16:
		/*
		 * TEST: DELETE a segment at start of buffer 
		 */
		udi_memcpy(&chkmem[0], &memory[350], 70);
		udi_memcpy(&chkmem[70], &memory[0], 35);
		udi_memcpy(&chkmem[105], &memory[280], 70);
		udi_memcpy(&chkmem[175], &memory[0], 35);
		udi_memcpy(&chkmem[210], &memory[210], 60);
		udi_memcpy(&chkmem[270], &memory[420], 40);
		udi_memcpy(&chkmem[310], &memory[70], 10);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 320;
		chkpbytes = 320;
		if (my_memcmp(bufmem, chkmem, chkbytes) == 0)
			status = FAILMEM;
		break;
	case 17:
		/*
		 * TEST: Copy buffer 1 to new buffer 15 
		 */
		udi_memcpy(&chkmem[0], &memory[350], 70);
		udi_memcpy(&chkmem[70], &memory[0], 35);
		udi_memcpy(&chkmem[105], &memory[280], 35);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 140;
		chkpbytes = 140;
		if (my_memcmp(bufmem, chkmem, chkbytes) == 0)
			status = FAILMEM;
		break;
	case 18:
		/*
		 * TEST: Insert part of buffer 1 into buffer 15 
		 */
		udi_memcpy(&chkmem[0], &memory[350], 70);
		udi_memcpy(&chkmem[70], &memory[210], 40);
		udi_memcpy(&chkmem[110], &memory[420], 30);
		udi_memcpy(&chkmem[140], &memory[0], 35);
		udi_memcpy(&chkmem[175], &memory[280], 35);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 210;
		chkpbytes = 210;
		if (my_memcmp(bufmem, chkmem, chkbytes) == 0)
			status = FAILMEM;
		break;
	case 19:
		/*
		 * TEST: Overwrite part of buffer 1 into buffer 15 
		 */
		udi_memcpy(&chkmem[0], &memory[350], 70);
		udi_memcpy(&chkmem[70], &memory[0], 5);
		udi_memcpy(&chkmem[75], &memory[280], 70);
		udi_memcpy(&chkmem[145], &memory[0], 35);
		udi_memcpy(&chkmem[180], &memory[210], 30);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 210;
		chkpbytes = 210;
		if (my_memcmp(bufmem, chkmem, chkbytes) == 0)
			status = FAILMEM;
		break;
	case 20:
		/*
		 * TEST: Allocate empty buffer into buffer 15 
		 */
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 0;
		chkpbytes = 0;
		break;
	case 21:
		/*
		 * TEST: INSERT at beginning of an empty buffer 
		 */
		udi_memcpy(&chkmem[0], &memory[140], 70);
		udi_buf_read(buffers[num], 0, bytes, &bufmem);
		chkbytes = 70;
		chkpbytes = 70;
		if (my_memcmp(bufmem, chkmem, chkbytes) == 0)
			status = FAILMEM;
		break;
	case 22:
		/*
		 * TEST: DELETE whole buffer from start 
		 */
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 0;
		chkpbytes = 0;
		break;
	case 23:
		/*
		 * TEST: Set larger buf_size 
		 */
		udi_memcpy(&chkmem[0], &memory[0], 10);
		udi_memcpy(&chkmem[10], &memory[70], 10);
		udi_memcpy(&chkmem[200], &memory[140], 5);
		udi_memcpy(&chkmem[205], &memory[210], 20);
		udi_memcpy(&chkmem[245], &memory[140], 25);
		udi_memcpy(&chkmem[270], &memory[4096], 35);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 305;
		chkpbytes = 270;
		if (my_memcmp(bufmem, chkmem, 20) == 0
		    || my_memcmp(&bufmem[200], &chkmem[200], 25) == 0
		    || my_memcmp(&bufmem[245], &chkmem[245], 25) == 0
		    || my_memcmp(&bufmem[270], &chkmem[270], 35) == 0)
			status = FAILMEM;
		break;
	case 24:
		/*
		 * TEST: Set smaller buf_size 
		 */
		udi_memcpy(&chkmem[0], &memory[140], 20);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 35;
		chkpbytes = 35;
		if (my_memcmp(bufmem, chkmem, 20) == 0)
			status = FAILMEM;
		break;
	case 25:
		/*
		 * TEST: Set a larger buf_size, do a DELETE 
		 */
		udi_memcpy(&chkmem[0], &memory[140], 20);
		udi_memcpy(&chkmem[200], &memory[140], 5);
		udi_memcpy(&chkmem[205], &memory[210], 15);
		udi_memcpy(&chkmem[220], &memory[140], 20);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 240;
		chkpbytes = 35;
		if (my_memcmp(bufmem, chkmem, 20) == 0 ||
		    my_memcmp(&bufmem[200], &chkmem[200], 40) == 0)
			status = FAILMEM;
		break;
	case 26:
		/*
		 * TEST: Copy expanded buffer 15 to new buffer 24 
		 */
		udi_memcpy(&chkmem[0], &memory[350], 70);
		udi_memcpy(&chkmem[70], &memory[0], 5);
		udi_memcpy(&chkmem[75], &memory[280], 70);
		udi_memcpy(&chkmem[145], &memory[0], 35);
		udi_memcpy(&chkmem[180], &memory[210], 30);
		udi_memcpy(&chkmem[210], &memory[4096], 70);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 280;
		chkpbytes = 280;
		if (my_memcmp(bufmem, chkmem, chkbytes) == 0)
			status = FAILMEM;
		break;
	case 27:
		/*
		 * TEST: Copy expanded buffer 15 to expanded buffer 19 
		 */
		udi_memcpy(&chkmem[5], &memory[350], 70);
		udi_memcpy(&chkmem[75], &memory[0], 5);
		udi_memcpy(&chkmem[80], &memory[280], 70);
		udi_memcpy(&chkmem[150], &memory[0], 35);
		udi_memcpy(&chkmem[185], &memory[210], 30);
		udi_memcpy(&chkmem[215], &memory[4096], 70);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 285;
		chkpbytes = 285;
		if (my_memcmp(&bufmem[5], &chkmem[5], chkbytes - 5) == 0)
			status = FAILMEM;
		break;
	case 28:
		/*
		 * TEST: Insert expanded buffer 15 to buffer 25 
		 */
		udi_memcpy(&chkmem[5], &memory[350], 70);
		udi_memcpy(&chkmem[75], &memory[0], 5);
		udi_memcpy(&chkmem[80], &memory[280], 60);
		udi_memcpy(&chkmem[140], &memory[210], 10);
		udi_memcpy(&chkmem[150], &memory[4096], 60);
		udi_memcpy(&chkmem[210], &memory[280], 10);
		udi_memcpy(&chkmem[220], &memory[0], 35);
		udi_memcpy(&chkmem[255], &memory[210], 30);
		udi_memcpy(&chkmem[285], &memory[4096], 70);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 355;
		chkpbytes = 355;
		if (my_memcmp(&bufmem[5], &chkmem[5], chkbytes - 5) == 0)
			status = FAILMEM;
		break;
	case 29:
		/*
		 * TEST: Insert 10 bytes at the end of buffer 25 
		 */
		udi_memcpy(&chkmem[5], &memory[350], 70);
		udi_memcpy(&chkmem[75], &memory[0], 5);
		udi_memcpy(&chkmem[80], &memory[280], 60);
		udi_memcpy(&chkmem[140], &memory[210], 10);
		udi_memcpy(&chkmem[150], &memory[4096], 60);
		udi_memcpy(&chkmem[210], &memory[280], 10);
		udi_memcpy(&chkmem[220], &memory[0], 35);
		udi_memcpy(&chkmem[255], &memory[210], 30);
		udi_memcpy(&chkmem[285], &memory[4096], 70);
		udi_memcpy(&chkmem[355], &memory[0], 10);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 365;
		chkpbytes = 365;
		if (my_memcmp(&bufmem[5], &chkmem[5], chkbytes - 5) == 0)
			status = FAILMEM;
		break;
	case 30:
		/*
		 * TEST: Replace 10 bytes in beginning of buffer 2 with
		 * 20 bytes 
		 */
		udi_memcpy(&chkmem[0], &memory[0], 20);
		udi_memcpy(&chkmem[20], &memory[140], 60);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 280;
		chkpbytes = 80;
		if (my_memcmp(bufmem, chkmem, chkpbytes) == 0)
			status = FAILMEM;
		break;
	case 31:
		/*
		 * TEST: Replace 10 bytes in middle of buffer 2 with
		 * 20 bytes
		 */
		udi_memcpy(&chkmem[0], &memory[0], 15);
		udi_memcpy(&chkmem[15], &memory[70], 20);
		udi_memcpy(&chkmem[35], &memory[140], 55);
		udi_buf_read(buffers[num], 0, bytes, bufmem);
		chkbytes = 280;
		chkpbytes = 90;
		if (my_memcmp(bufmem, chkmem, chkpbytes) == 0)
			status = FAILMEM;
		break;
	}
	if (buffercheck < n_buf_tests) {
		if (bytes != chkbytes || pbytes != chkpbytes)
			status |= FAILSIZ;
		printf("TEST %d: %s\n", buffercheck,
		       status == SUCCESS ? "SUCCEEDED" : "FAILED!");
		printf("Buffer size is ");
		if (bytes != chkbytes) {
			printf("incorrect!\n");
			printf("  Should be %d, was %d\n", 
			        (udi_ubit32_t) chkbytes, (udi_ubit32_t) bytes);
		} else {
			printf("correct (%d)\n", (udi_ubit32_t) bytes);
		}
		printf("Buffer phys size is ");
		if (pbytes != chkpbytes) {
			printf("incorrect!\n");
			printf("  Should be %d, was %d\n", 
				(udi_ubit32_t)chkpbytes, (udi_ubit32_t) pbytes);
		} else {
			printf("correct (%d)\n", (udi_ubit32_t) pbytes);
		}
		printf("Content is %s\n",
		       status & FAILMEM ? "incorrect!" : "correct");
		if (status & FAILMEM) {
			printf("  Intended:\n");
			printmemory((unsigned char *)chkmem, chkbytes, 0, 0,
				    0);
			printf("  Actual:\n");
			printmemory((unsigned char *)bufmem, chkbytes, 0, 0,
				    0);
		}
		if (status)
			abort();

		buffercheck++;
	}
}

void
buf_alloc(udi_cb_t *gcb,
	  udi_buf_t *new_dst_buf)
{
	int num;

	buffers[n_buf_allocs] = new_dst_buf;
	num = n_buf_allocs;
	dumpbuffer(num);
	n_buf_allocs++;
	if (n_buf_allocs < n_buf_tests && SINGLE_THREADED_TESTS) {
		/*
		 * This is the path to take if we are single-threading the
		 * tests via one CB (see the if statement in the main
		 * function).  Valid to pass NULL as first arg of cballoc
		 * since it never uses it. 
		 */
		cballoc(NULL, gcb);
	} else
		udi_cb_free(gcb);
}

void
cballoc(udi_cb_t *gcb,
	udi_cb_t *new_cb)
{
	int num = n_cb_alloc;

	n_cb_alloc++;

	printf("New cb allocated %p\n", new_cb);

/* For udi_buf_write args:
 *   *callback, *gcb, *src_mem, src_len, dst_buf, dst_off, dst_len, constraints
 */
	switch (num) {
	case 0:
		printf("  TEST: allocate an empty buffer\n");
		printf("  Alloc new buf, src=NULL src_len=210\n");
		udi_buf_write(buf_alloc, new_cb, NULL, 210,
			      NULL, 0, 0, (udi_buf_path_t *)&buf_path);
		break;
	case 1:
		printf("  TEST: allocate an initialized buffer\n");
		printf("  Alloc new buf, src=mem[0] (AB) src_len=140\n");
		udi_buf_write(buf_alloc, new_cb, &memory[0], 140,
			      NULL, 0, 0, (udi_buf_path_t *)&buf_path);
		break;
	case 2:
		printf("  TEST: allocate an empty buffer (again)\n");
		printf("  Alloc new buf, src=NULL src_len=210\n");
		udi_buf_write(buf_alloc, new_cb, NULL, 210,
			      NULL, 0, 0, (udi_buf_path_t *)&buf_path);
		break;
	case 3:
		printf("  TEST: INSERT at beginning of an initialized buffer\n");
		printf("  Insert into buf 1, src_mem[140] (C) src_len=70 dst_buf=buf[1], dst_off=0\n");
		udi_buf_write(buf_alloc, new_cb, &memory[140], 70,
			      buffers[1], 0, 0, UDI_NULL_BUF_PATH);
		break;
	case 4:
		printf("  TEST: INSERT at beginning of an uninitialized buffer\n");
		printf("  Insert into buf 0, src_mem[140] (C) src_len=70 dst_buf=buf[0] dst_off=0\n");
		udi_buf_write(buf_alloc, new_cb, &memory[140], 70,
			      buffers[2], 0, 0, UDI_NULL_BUF_PATH);
		break;
	case 5:
		printf("  TEST: INSERT in middle of buffer at end of segment\n");
		printf("  Insert into buf 1, src_mem[210] (D) src_len=70 dst_buf=buf[1], dst_off=140\n");
		udi_buf_write(buf_alloc, new_cb, &memory[210], 70,
			      buffers[1], 140, 0, UDI_NULL_BUF_PATH);
		break;
	case 6:
		printf("  TEST: INSERT in middle of buffer at middle of segment\n");
		printf("  Insert into buf 1, src_mem[280] (E) src_len=70 dst_buf=buf[1], dst_off=105\n");
		udi_buf_write(buf_alloc, new_cb, &memory[280], 70,
			      buffers[1], 105, 0, UDI_NULL_BUF_PATH);
		break;
	case 7:
		printf("  TEST: INSERT in middle of buffer at beginning of segment\n");
		printf("  Insert into buf 1, src_mem[350] (F) src_len=70 dst_buf=buf[1], dst_off=70\n");
		udi_buf_write(buf_alloc, new_cb, &memory[350], 70,
			      buffers[1], 70, 0, UDI_NULL_BUF_PATH);
		break;
	case 8:
		printf("  TEST: INSERT in middle of buffer at end of segment spanning segment\n");
		printf("  Insert into buf 1, src_mem[420] (G) src_len=70 dst_buf=buf[1], dst_off=400\n");
		udi_buf_write(buf_alloc, new_cb, &memory[420], 70,
			      buffers[1], 400, 0, UDI_NULL_BUF_PATH);
		break;
	case 9:
		printf("  TEST: OVERWRITE at beginning of buffer at beginning of segment\n");
		printf("  Overwrite buf 0, src_mem[0] (A) src_len=10 dst_buf=buf[0] dst_off=0 dst_len=10\n");
		udi_buf_write(buf_alloc, new_cb, &memory[0], 10,
			      buffers[0], 0, 10, UDI_NULL_BUF_PATH);
		break;
	case 10:
		printf("  TEST: OVERWRITE at beginning of buffer at middle of segment\n");
		printf("  Overwrite buf 0, src_mem[70] (B) src_len=10 dst_buf=buf[0] dst_off=10 dst_len=10\n");
		udi_buf_write(buf_alloc, new_cb, &memory[70], 10,
			      buffers[0], 10, 10, UDI_NULL_BUF_PATH);
		break;
	case 11:
		printf("  TEST: OVERWRITE at end of buffer at end of segment\n");
		printf("  Overwrite buf 0, src_mem[140] (C) src_len=10 dst_buf=buf[0] dst_off=200 dst_len=10\n");
		udi_buf_write(buf_alloc, new_cb, &memory[140], 10,
			      buffers[0], 200, 10, UDI_NULL_BUF_PATH);
		break;
	case 12:
		printf("  TEST: OVERWRITE at end of buffer at end of segment, extending buffer 15 bytes\n");
		printf("  Overwrite buf 0, src_mem[210] (E) src_len=20 dst_buf=buf[0] dst_off=205 dst_len=5\n");
		udi_buf_write(buf_alloc, new_cb, &memory[210], 20,
			      buffers[0], 205, 5, UDI_NULL_BUF_PATH);
		break;
	case 13:
		printf("  TEST: DELETE at end of buffer at end of segment\n");
		printf("  Delete buf 1, dst_buf=buf[1] dst_off=480 dst_len=10\n");
		udi_buf_write(buf_alloc, new_cb, NULL, 0,
			      buffers[1], 480, 10, UDI_NULL_BUF_PATH);
		break;
	case 14:
		printf("  TEST: DELETE at middle of buffer at middle of segment\n");
		printf("  Delete buf 1, dst_buf=buf[1] dst_off=390 dst_len=20\n");
		udi_buf_write(buf_alloc, new_cb, NULL, 0,
			      buffers[1], 390, 20, UDI_NULL_BUF_PATH);
		break;
	case 15:
		printf("  TEST: DELETE at middle of buffer across segments\n");
		printf("  Delete buf 1, dst_buf=buf[1] dst_off=340 dst_len=70\n");
		udi_buf_write(buf_alloc, new_cb, NULL, 0,
			      buffers[1], 340, 70, UDI_NULL_BUF_PATH);
		break;
	case 16:
		printf("  TEST: DELETE a segment at start of buffer\n");
		printf("  Delete buf 1, dst_buf=buf[1] dst_off=0 dst_len=70\n");
		udi_buf_write(buf_alloc, new_cb, NULL, 0,
			      buffers[1], 0, 70, UDI_NULL_BUF_PATH);
		break;
	case 17:
		printf("  TEST: Copy buffer 1 to new buffer 17\n");
		printf("  Copy to new buf, src_buf=buf[1] src_off=0 src_len=140, dst_buf=NULL dst_off=0 dst_len=0\n");
		udi_buf_copy(buf_alloc, new_cb, buffers[1], 0, 140,
			     NULL, 0, 0, (udi_buf_path_t *)&buf_path);
		break;
	case 18:
		printf("  TEST: Insert part of buffer 1 into buffer 17\n");
		printf("  Copy to buf 17, src_buf=buf[1] src_off=230 src_len=70, dst_buf=buf[17] dst_off=70 dst_len=0\n");
		udi_buf_copy(buf_alloc, new_cb, buffers[1], 230, 70,
			     buffers[17], 70, 0, UDI_NULL_BUF_PATH);
		break;
	case 19:
		printf("  TEST: Overwrite part of buffer 1 into buffer 17\n");
		printf("  Overwrite buf 17, src_buf=buf[1] src_off=100 src_len=140, dst_buf=buf[17] dst_off=70 dst_len=140\n");
		udi_buf_copy(buf_alloc, new_cb, buffers[1], 100, 140,
			     buffers[17], 70, 140, UDI_NULL_BUF_PATH);
		break;
	case 20:
		printf("  TEST: Allocate a zero size buffer into buffer 20\n");
		printf("  Write buf 20, src_mem[0] src_len=0, dst_buf=NULL dst_off=0 dst_len=0\n");
		udi_buf_write(buf_alloc, new_cb, &memory[0], 0, NULL,
			      0, 0, (udi_buf_path_t *)&buf_path);
		break;
	case 21:
		printf("  TEST: INSERT at beginning of an empty buffer 21\n");
		printf("  Insert into buf 21, src_mem[140] (C) src_len=70 dst_buf=buf[18], dst_off=0\n");
		/*
		 * Note: buffers[21] hasn't been written to yet and is NULL 
		 */
		udi_buf_write(buf_alloc, new_cb, &memory[140], 70,
			      buffers[21], 0, 0, (udi_buf_path_t *)&buf_path);
		break;
	case 22:
		printf("  TEST: DELETE whole buffer from start\n");
		printf("  Delete buf 21, dst_buf=buf[22], dst_off=0, dst_len=%d\n", (udi_ubit32_t) buffers[21]->buf_size);
		udi_buf_write(buf_alloc, new_cb, NULL, 0,
			      buffers[21], 0, buffers[21]->buf_size,
			      UDI_NULL_BUF_PATH);
		break;
	case 23:
		printf("  TEST: Set larger buf_size\n");
		printf("  Increase buf 0 logically to 280 bytes\n");
		printf("  Insert into buf 0, src_mem[140] (C) src_len=25 dst_buf=buf[0], dst_off=245\n");
		buffers[0]->buf_size = 280;
		udi_buf_write(buf_alloc, new_cb, &memory[140], 25,
			      buffers[0], 245, 0, UDI_NULL_BUF_PATH);
		break;
	case 24:
		printf("  TEST: Set smaller buf_size\n");
		printf("  Decrease buf 0 logically to 35 bytes\n");
		printf("  Overwrite into buf 0, src_mem[140] (C) src_len=20 dst_buf=buf[0], dst_off=0\n");
		buffers[0]->buf_size = 35;
		udi_buf_write(buf_alloc, new_cb, &memory[140], 20,
			      buffers[0], 0, 20, UDI_NULL_BUF_PATH);
		break;
	case 25:
		printf("  TEST: Set a larger buf_size, do a DELETE\n");
		printf("  Increase buf 0 logically to 265 bytes\n");
		printf("  Delete from buf 0, dst_buf=buf[0], dst_off=220 dst_len=25\n");
		buffers[0]->buf_size = 265;
		udi_buf_write(buf_alloc, new_cb, NULL, 0,
			      buffers[0], 220, 25, UDI_NULL_BUF_PATH);
		break;
	case 26:
		printf("  TEST: Copy expanded buffer 17 to new buffer\n");
		printf("  Increate buf 15 logically to 280 bytes\n");
		printf("  Copy to new buf, src_buf=buf[17] src_off=0 src_len=280, dst_buf=NULL dst_off=0 dst_len=0\n");
		buffers[17]->buf_size = 280;
		udi_buf_copy(buf_alloc, new_cb, buffers[17], 0, 280,
			     NULL, 0, 0, (udi_buf_path_t *)&buf_path);
		break;
	case 27:
		printf("  TEST: Copy expanded buffer 17 to expanded buffer 21\n");
		printf("  Increase buf 21 logically to 280 bytes\n");
		printf("  Copy to buf[21], src_buf=buf[17] src_off=0 src_len=280, dst_buf=buf[21] dst_off=5 dst_len=275\n");
		buffers[21]->buf_size = 280;
		udi_buf_copy(buf_alloc, new_cb, buffers[17], 0, 280,
			     buffers[21], 5, 275, UDI_NULL_BUF_PATH);
		break;
	case 28:
		printf("  TEST: Insert expanded buffer 17 to buffer 27\n");
		printf("  Copy to buf[27], src_buf=buf[17] src_off=200 src_len=70, dst_buf=buf[27] dst_off=140 dst_len=0\n");
		udi_buf_copy(buf_alloc, new_cb, buffers[17], 200, 70,
			     buffers[27], 140, 0, UDI_NULL_BUF_PATH);
		break;
	case 29:
		printf("  TEST: Insert 10 bytes at the end of buffer 27\n");
		printf("  Insert into buf[27], src_mem[0], src_len=10, dst_buf=buf[27] dst_off=355 dst_len=0\n");
		udi_buf_write(buf_alloc, new_cb, &memory[0], 10,
			      buffers[27], 355, 0, UDI_NULL_BUF_PATH);
		break;
	case 30:
		printf("  TEST: Replace 10 bytes at beginning of buffer 2 with 20 bytes (A)\n");
		printf("  Overwrite buf[2], src_mem[0] (A), src_len=20, dst_buf=buf[2] dst_off=0, dst_len=10\n");
		udi_buf_write(buf_alloc, new_cb, &memory[0], 20,
			      buffers[2], 0, 10, UDI_NULL_BUF_PATH);
		break;
	case 31:
		printf("  TEST: Replace 10 bytes in the middle of buffer 2 with 20 bytes (B)\n");
		printf("  Overwrite buf[2], src_mem[70] (B), src_len=20, dst_buf=buf[2] dst_off=15, dst_len=10\n");
		udi_buf_write(buf_alloc, new_cb, &memory[70], 20,
			      buffers[2], 15, 10, UDI_NULL_BUF_PATH);
		break;
	}
}

void
cbtags(udi_cb_t *gcb,
       udi_buf_t *buf)
{
	udi_buf_tag_t tags[2];
	udi_ubit16_t r;

	switch (n_buftag_tests) {
	case 0:
		dumpbuffer(0);
		printf("  TEST: udi_buf_tag_set on buffer 0\n");
		r = udi_buf_tag_get(buf, UDI_BUFTAG_SET_TCP_CHECKSUM,
				    tags, 2, 0);
		_OSDEP_ASSERT(r == 2);
		_OSDEP_ASSERT(tags[0].tag_type == UDI_BUFTAG_SET_TCP_CHECKSUM);
		_OSDEP_ASSERT(tags[0].tag_value == 0);
		_OSDEP_ASSERT(tags[0].tag_off == 5);
		_OSDEP_ASSERT(tags[0].tag_len == 10);
		_OSDEP_ASSERT(tags[1].tag_type == UDI_BUFTAG_SET_TCP_CHECKSUM);
		_OSDEP_ASSERT(tags[1].tag_value == 0);
		_OSDEP_ASSERT(tags[1].tag_off == 0);
		_OSDEP_ASSERT(tags[1].tag_len == 10);
		break;
	}
	printf("TEST %d: SUCCEEDED (TAG)\n", n_buftag_tests);

	n_buftag_tests++;
}

int
main(int argc,
     char *argv[])
{
	int i, j, prev;
	char *nbt;

	nbt = getenv("NUM_BUF_TESTS");
	if (nbt)
		n_buf_tests = atoi(nbt) > MAXBUFS ? MAXBUFS : atoi(nbt);

	for (i = 0; i < 4096; i++)
		memory[i] = 'A' + i / 70;
	for (i = 0; i < 70; i++)
		memory[4096 + i] = 0xca;

	posix_init(argc, argv, "", NULL);
	posix_module_init(udi_gio);

	if (SINGLE_THREADED_TESTS) {
		extern _udi_channel_t *_udi_MA_chan;
		struct {
			_udi_cb_t _cb;
			udi_cb_t v;
		} _cb;

		_cb.v.channel = _udi_MA_chan;
		_cb._cb.cb_allocm.alloc_state = _UDI_ALLOC_IDLE;
		/*
		 * Single CB to single-thread the tests: preserves interdependencies 
		 */
		udi_cb_alloc(cballoc, &_cb.v, 1, _udi_MA_chan);
	} else {
		extern _udi_channel_t *_udi_MA_chan;
		struct {
			_udi_cb_t _cb;
			udi_cb_t v;
		} _cb[MAXBUFS];

		for (i = 0; i < n_buf_tests; i++) {
			_cb[i].v.channel = _udi_MA_chan;
			_cb[i]._cb.cb_allocm.alloc_state = _UDI_ALLOC_IDLE;
			udi_cb_alloc(cballoc, &_cb[i].v, 1, _udi_MA_chan);
			/*
			 * This alloc's a CB for each buffer test and runs them all
			 * * individually.  Unfortunately, there are dependencies
			 * * between the tests so any rescheduling (via callback limits
			 * * or POSIX_FAIL_MEM settings) will cause buffertest to fail.
			 * * You can either use the single-threaded version above or
			 * * depend on the while statement following (i.e. removing the
			 * * while will cause sequencing problems). 
			 */
			while (n_buf_allocs == i);
		}
	}

	while (n_buf_allocs < n_buf_tests)
		sleep(1);		/* allow alloc thread to run, we hope [if needed] */

	if (1) {
		udi_ubit32_t val32;
		udi_ubit32_t tmp32;
		udi_buf_tag_t *tag =
			malloc(sizeof (udi_buf_tag_t) * BUF_TAGS_USED);
		extern _udi_channel_t *_udi_MA_chan;
		struct {
			_udi_cb_t _cb;
			udi_cb_t v;
		} _cb;

		printf("\nBUFFER TAG TESTS ...\n\n");
		for (i = 0; i < NUMTAGS; i++) {
			_cb.v.channel = _udi_MA_chan;
			_cb._cb.cb_allocm.alloc_state = _UDI_ALLOC_IDLE;
			switch (i) {
			case 0:
				tag[0].tag_type = UDI_BUFTAG_SET_TCP_CHECKSUM;
				tag[0].tag_value = 0;
				tag[0].tag_off = 0;
				tag[0].tag_len = 10;
				tag[1].tag_type = UDI_BUFTAG_SET_TCP_CHECKSUM;
				tag[1].tag_value = 0;
				tag[1].tag_off = 5;
				tag[1].tag_len = 10;
				udi_buf_tag_set(cbtags, &_cb.v, buffers[0],
						tag, 2);
				break;
			case 1:
				dumpbuffer(0);
				printf("  TEST: udi_buf_tag_compute on buffer 0, " "offset = 5, len = 10\n");
				udi_memcpy(&chkmem, "CCCCCCCCCC", 10);
				val32 = udi_buf_tag_compute(buffers[0], 5, 10,
							    UDI_BUFTAG_BE16_CHECKSUM);
				tmp32 = calc_checksum((udi_ubit8_t *)chkmem,
						      10);
				if (val32 != tmp32) {
					printf("  Intended: %x\n", tmp32);
					printf("  Actual: %x\n", val32);
					_OSDEP_ASSERT(val32 == tmp32);
				} else {
					printf("TEST %d: SUCCEEDED\n",
					       n_buftag_tests);
				}
				n_buftag_tests++;
				break;
			}

			/*
			 * Wait until each one is completed 
			 */
			while (i == n_buftag_tests);
		}
	}

	printf("\nFREE ALLOCATED BUFFERS, TO MAKE SURE FREE DOES NOT CORE DUMP"
	       " ...\n\n");
	/*
	 * We can not free some of the buffers since we used a static
	 * array of target buffers for some of the operations.  Here
	 * we will free all the unique buffers used 
	 */
	for (i = 0; i < n_buf_tests; i++) {
		prev = 0;
		for (j = 0; j <= i; j++) {
			if (j == i) {
				dumpbuffer(i);
				udi_buf_free(buffers[i]);
				printf("Buffer %d freed.\n", i);
				break;
			} else if (buffers[j] == buffers[i]) {
				prev = 1;
				break;
			}
		}
		if (prev)
			printf("Buffer %d = Buffer %d (already freed %p)\n",
			       i, j, buffers[i]);
	}

	posix_module_deinit(udi_gio);
	posix_deinit();

	printf("Exiting\n");
	return 0;
}
