
/*
 * File: env/posix/ndtest.c
 *
 * Test Harness for network driver. Allows driver to be bound and then have
 * Meta-specific actions applied.
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

#include <udi_env.h>
#include <posix.h>

#define UDI_NIC_VERSION	0x101
#include <udi_nic.h>
#include <netmap_posix.h>
#include <netmap.h>			/* For NSR mapper */
#define	Private_visible
#define	ND_NSR_META	1
#define	ND_GIO_META	2
#include <pseudond.h>			/* For pseudond driver */

#include <giomap_posix.h>

_OSDEP_EVENT_T ndtest_ev;

int nd_test(void *,
	    void *,
	    udi_ubit32_t,
	    char);
void ndtest_dump_MCA(nsr_mca_struct_t *);
void ndtest_dump_MAC(nsr_macaddr_t *);
extern nsrmap_Tx_region_data_t *NSR_txdata(nsrmap_region_data_t *);

#define	PSEUDO_ND_DRIVER_NAME	"pseudond"
#define	NSR_MAPPER_NAME		"netmap"
#define	GIO_MAPPER_NAME		"giomap"

void
do_binds(void)
{
	posix_module_init(udi_gio);
	posix_module_init(udi_nic);
	posix_module_init(udi_brdg);
	posix_module_init(brdg_map);
	posix_module_init(pseudond);
	posix_module_init(netmap);
}

void
do_unbinds(void)
{
	posix_module_deinit(pseudond);
	posix_module_deinit(netmap);
	posix_module_deinit(brdg_map);
	posix_module_deinit(udi_brdg);
	posix_module_deinit(udi_nic);
	posix_module_deinit(udi_gio);
}


int n_rx_packets = 0;
int n_tx_packets = 0;
int n_tx_discards = 0;
int n_rx_frees = 0;

int Debug = 0;
volatile Pseudo_Card_t *Pc;
volatile nsrmap_region_data_t *rdata;
volatile nsrmap_Tx_region_data_t *txdata;
udi_ubit32_t maxbufsize = 200;
udi_ubit32_t minpdu = 0, maxpdu = 0, mediatype = 2;
int niter = 10, nopens = 1;
udi_boolean_t Unbind = FALSE;		/*Just bind/unbind the driver, no i/o */

unsigned long long tot_transfer = 0;

int nd_test_ioctl(void);

void
ndtest_parseargs(int c,
		 void *optarg)
{
	switch (c) {
	case 'b':
		maxbufsize = udi_strtou32(optarg, NULL, 0);
		break;
	case 'D':
		Debug++;
		break;
	case 'n':
		niter = udi_strtou32(optarg, NULL, 0);
		break;
	case 'm':
		minpdu = udi_strtou32(optarg, NULL, 0);
		break;
	case 'M':
		maxpdu = udi_strtou32(optarg, NULL, 0);
		break;
	case 'c':
		mediatype = udi_strtou32(optarg, NULL, 0);
		break;
	case 'o':
		nopens = udi_strtou32(optarg, NULL, 0);
		break;
	case 'U':
		Unbind = TRUE;
		break;
	default:
		break;
	}
}

/*
 * External Function declarations
 */
extern void *_udi_MA_local_data(const char *,
				udi_ubit32_t);
extern int nsrmap_open(udi_ubit32_t);
extern int nsrmap_close(udi_ubit32_t);
extern void nsrmap_getmsg(udi_ubit32_t,
			  nsrmap_uio_t *);
extern int nsrmap_sendmsg(udi_ubit32_t,
			  nsrmap_uio_t *);
extern int nsrmap_ioctl(udi_ubit32_t,
			int,
			void *);

int
main(int ac,
     char **av)
{
	int rval;
	int i;
	struct tms start, end;
	clock_t s_clk, e_clk;
	void *databuf, *rxdatabuf;

#if 0
	pseudond_media_t Media;
	giomap_uio_t uio;
#endif
	udi_ubit32_t rbufsize;		/* Random(ish) buf size */

	srand(getpid());

	posix_init(ac, av, "b:Dn:m:M:c:o:U", &ndtest_parseargs);

	_OSDEP_EVENT_INIT(&ndtest_ev);

	do_binds();

#if 0
	/*
	 * Open the GIO interface to allow us to set the card's min/max/media 
	 */
	printf("GIO interface open...");
	rval = giomap_open(0);
	_OSDEP_ASSERT(rval == 0);
	printf(" succeeded\n");

	Media.max_pdu_size = maxpdu;
	Media.min_pdu_size = minpdu;
	Media.media_type = (udi_ubit8_t)mediatype;

	udi_memset(&uio, 0, sizeof (uio));
	uio.u_addr = &Media;
	uio.u_count = sizeof (Media);
	uio.u_op = PSEUDOND_GIO_MEDIA | UDI_GIO_DIR_WRITE;

	printf("GIO xfer ioctl...");
	rval = giomap_ioctl(0, UDI_GIO_DATA_XFER, &uio);
	_OSDEP_ASSERT(rval == 0);
	printf(" succeeded\n");

	printf("GIO interface close...");
	rval = giomap_close(0);
	_OSDEP_ASSERT(rval == 0);
	printf(" succeeded\n");
#endif

	Pc = (Pseudo_Card_t *) _udi_MA_local_data(PSEUDO_ND_DRIVER_NAME, 0);
	assert(Pc != NULL);


	if (Unbind) {
		/*
		 * Just tear down the bindings without performing any i/o
		 * This is similar to a driver load/unload with no access
		 * to /dev/pseudondX
		 */
		printf("Trying to UNBIND...");
		do_unbinds();

		printf(" succeeded\n");

		posix_deinit();
		printf("Exiting....\n");
		exit(0);
	}
	/*
	 * Open the card  - perform nopens 'pseudo'-opens first 
	 */

	printf("NIC interface open...");
	rval = nsrmap_open(0);
	_OSDEP_ASSERT(rval == 0);
	printf(" succeeded\n");

	/*
	 * Get the NSR mapper context from <r3>
	 */
	rdata = (nsrmap_region_data_t *) _udi_MA_local_data(NSR_MAPPER_NAME,
							    0);
	assert(rdata != NULL);

	if (Debug) {
		printf("NSR rdata @ %p\n", rdata);
	}

	txdata = NSR_txdata((nsrmap_region_data_t *) rdata);
	assert(txdata != NULL);

	if (Debug) {
		printf("NSR txdata @ %p\n", txdata);
	}

	for (i = 0; i < nopens; i++) {
		printf("NIC interface close...");
		rval = nsrmap_close(0);
		_OSDEP_ASSERT(rval == 0);
		printf(" succeeded\n");

		printf("NIC interface open...");
		rval = nsrmap_open(0);
		_OSDEP_ASSERT(rval == 0);
		printf(" succeeded\n");
	}

	rval = nsrmap_open(17);
	_OSDEP_ASSERT(rval != 0);

	if (Debug) {
		printf("Per Card info @ %p\n", Pc);
	}

	assert(NULL != Pc->Card_NSR_cb);
#if 0
	assert(NULL != Pc->Card_GIO_cb);
#endif

	if (Debug) {
		printf("Pc->Card_NSR_cb = %p\n", Pc->Card_NSR_cb);
	}

	databuf = _OSDEP_MEM_ALLOC(maxbufsize, 0, UDI_WAITOK);
	rxdatabuf = _OSDEP_MEM_ALLOC(maxbufsize, 0, UDI_WAITOK);

	printf("Starting %d round-trip I/O operation(s) (maxbufsize=%d)\n",
	       niter, maxbufsize);

	s_clk = times(&start);
	for (i = 0; i < niter; i++) {
		rbufsize = rand() % maxbufsize;
		if (rbufsize == 0) {
			rbufsize = 1;
		}
		rval = nd_test(databuf, rxdatabuf, rbufsize,
			       (char)((i % 10) + '0'));
		if (rval != 0) {
			printf("\n*** Test Iteration %d FAILED ***\n", i);
			return -1;
		}
	}
	e_clk = times(&end);

	_OSDEP_MEM_FREE(databuf);
	_OSDEP_MEM_FREE(rxdatabuf);

	/*
	 * Test the ioctl handling 
	 */
	rval = nd_test_ioctl();

	n_rx_packets = rdata->n_packets;
	n_tx_packets = txdata->n_packets;
	n_tx_discards = txdata->n_discards;

	/*
	 * Close the card down 
	 */
	rval = nsrmap_close(0);
	_OSDEP_ASSERT(rval == 0);

	rval = nsrmap_close(23);
	_OSDEP_ASSERT(rval != 0);

	/*
	 * Now try and unbind 
	 */

	do_unbinds();

	printf("Received %d packets\n", n_rx_packets);
	printf("Transmitted %d packets\n", n_tx_packets);
	printf("Discarded %d Tx packets\n", n_tx_discards);
	printf("\nElapsed Time = %d ticks\n", (int)(e_clk - s_clk));

	printf("Total Transfer (kbytes) = %d\n", (int)(tot_transfer / 1024));

	_OSDEP_EVENT_DEINIT(&ndtest_ev);
	posix_deinit();

	printf("Exiting\n");

	/*
	 * This looks like we're coddling a broken environment, but really 
	 * we aren't. (much)  In the real world, only one cb realloc will 
	 * happen on any given channel - either going in or going out.   
	 * Since we're forcing one on _both_ directions,  and we don't want 
	 * the env to have to test for "impossible" conditions, we allow it 
	 * to leak in this case.
	 */
	if (!posix_force_cb_realloc) {
		assert(Allocd_mem == 0);
	}

	return 0;
}

int
nd_test(void *databuf,
	void *rxdatabuf,
	udi_ubit32_t bufsize,
	char c)
{
	volatile nsrmap_uio_t uio;
	nsrmap_uio_t *uiop = (nsrmap_uio_t *) & uio;
	int rval;


	if (Debug) {
		printf("nd_test: bufsize = %d\n", bufsize);
	}

	udi_memset(databuf, c, bufsize);

	((udi_ubit8_t *)databuf)[bufsize - 1] = '\0';

	uio.u_addr = databuf;
	uio.u_size = (udi_size_t)bufsize;

	nsrmap_sendmsg(0, uiop);

	uio.u_addr = rxdatabuf;
	uio.u_size = maxbufsize;	/* Read Maximum amount of data */

	nsrmap_getmsg(0, uiop);
	if (uio.u_size == 0) {
		if (Debug) {
			printf("nsrmap_getmsg: failed!\n");
		}
		return -1;
	}

	if (Debug) {
		printf("nsrmap_getmsg: Got %d bytes\n", (udi_ubit32_t) uio.u_size);
		printf("rxdatabuf = <%s>\n", (char *)rxdatabuf);
	}
	rval = (udi_memcmp(databuf, rxdatabuf, bufsize));

	if (rval) {
		printf("*** MISMATCH *** \nExpected <%s>, got <%s>\n",
		       (char *)databuf, (char *)rxdatabuf);
		printf("Tx length = %d, Rx length = %d\n", bufsize,
		       (udi_ubit32_t) uio.u_size);
	} else {
		tot_transfer += uio.u_size;
	}
	return rval;
}

/*
 * Ugly macros to ease code testing
 */
#define	ND_TEST_MCA_IOCTL(cmd, data, datalen, mcap, retzero ) \
	{ \
		int retval; \
		(mcap)->ioc_cmd = cmd; \
		(mcap)->ioc_len = datalen; \
		(mcap)->ioc_num = datalen / 6; \
		(mcap)->ioc_data = data; \
		printf( "Trying %s...\n", #cmd ); \
		retval = nsrmap_ioctl( 0, cmd, mcap ); \
		if ( retzero ) { \
			assert( retval == 0 ); \
		} else { \
			assert( retval != 0 ); \
		} \
	}

#define	ND_TEST_MAC_IOCTL(cmd, data, datalen, macp, retzero ) \
	{ \
		int retval; \
		(macp)->ioc_cmd = cmd; \
		(macp)->ioc_len = datalen; \
		(macp)->ioc_data = data; \
		printf("Trying %s...\n", #cmd ); \
		retval = nsrmap_ioctl( 0, cmd, macp ); \
		if ( retzero ) { \
			assert( retval == 0 ); \
		} else { \
			assert( retval != 0 ); \
		} \
	}

#define	BOBOMAC	"\xB0\xB0\xB0\xB0\xB0\x0B"
#define	CACAMAC "\xCA\xCA\xCA\xCA\xCA\xCA"
#define	DEADMAC	"\xDE\xAD\xBE\xEF\xCA\xCA"

/*
 * nd_test_ioctl:
 * -------------
 * Issue series of POSIX_ND_xxx ioctls to set, delete and modify multicast
 * addresses and MAC address behaviour
 */
int
nd_test_ioctl(void)
{
	nsr_mca_struct_t mca_req;	/* multicast manipulation */
	nsr_macaddr_t MAC_req;		/* MAC address manipulation */
	void *loc_buf;			/* Buffer contents  */
	udi_size_t buf_size;

	buf_size = 16 * 6;		/* Size for 16 Multicast Addresses assuming
					 * 6-byte MAC address format
					 */
	loc_buf = _OSDEP_MEM_ALLOC(buf_size, 0, UDI_WAITOK);

	/*
	 * Ensure that the MCA table is empty.
	 */
	ND_TEST_MCA_IOCTL(POSIX_ND_RETMCA, loc_buf, buf_size, &mca_req, TRUE);

	assert(mca_req.ioc_num == 0);

	/*
	 * Add a Multicast Address to the list
	 */
	ND_TEST_MCA_IOCTL(POSIX_ND_SETMCA, BOBOMAC, 6, &mca_req, TRUE);

	/*
	 * Dump the Multicast table, should not be empty
	 */
	ND_TEST_MCA_IOCTL(POSIX_ND_RETMCA, loc_buf, buf_size, &mca_req, TRUE);

	ndtest_dump_MCA(&mca_req);

	/*
	 * Remove a non-existent MCA from the list
	 */
	ND_TEST_MCA_IOCTL(POSIX_ND_DELMCA, DEADMAC, 6, &mca_req, TRUE);

	/*
	 * Add an already existing entry to the MCA list in addition to a new
	 * one
	 */
	ND_TEST_MCA_IOCTL(POSIX_ND_SETMCA, DEADMAC BOBOMAC, 12,
			  &mca_req, TRUE);

	/*
	 * Dump new MCA table
	 */
	ND_TEST_MCA_IOCTL(POSIX_ND_RETMCA, loc_buf, buf_size, &mca_req, TRUE);

	ndtest_dump_MCA(&mca_req);


	/*
	 * Delete existing MCA address. Specify address twice in list.
	 */
	ND_TEST_MCA_IOCTL(POSIX_ND_DELMCA,
			  BOBOMAC CACAMAC BOBOMAC, 18, &mca_req, TRUE);

	/*
	 * Dump MCA table 
	 */
	ND_TEST_MCA_IOCTL(POSIX_ND_RETMCA, loc_buf, buf_size, &mca_req, TRUE);

	ndtest_dump_MCA(&mca_req);

	/*
	 * All Multicast addresses OFF: should clear the multicast table too.
	 */
	ND_TEST_MCA_IOCTL(POSIX_ND_DELALLMCA, NULL, 0, &mca_req, TRUE);

	ndtest_dump_MCA(&mca_req);

	/*
	 * Get the currently active MAC address
	 */
	ND_TEST_MAC_IOCTL(POSIX_ND_GETADDR, loc_buf, buf_size, &MAC_req, TRUE);

	ndtest_dump_MAC(&MAC_req);

	/*
	 * Set the MAC address
	 */
	ND_TEST_MAC_IOCTL(POSIX_ND_SETADDR, CACAMAC, 6, &MAC_req, TRUE);

	/*
	 * Get Current MAC and Factory MAC
	 */
	ND_TEST_MAC_IOCTL(POSIX_ND_GETRADDR, loc_buf, buf_size, &MAC_req,
			  TRUE);

	ndtest_dump_MAC(&MAC_req);

	ND_TEST_MAC_IOCTL(POSIX_ND_GETADDR, loc_buf, buf_size, &MAC_req, TRUE);

	ndtest_dump_MAC(&MAC_req);
	_OSDEP_MEM_FREE(loc_buf);
	return 0;
}

void
ndtest_dump_MCA(nsr_mca_struct_t * mcap)
{
	udi_ubit32_t i;
	udi_ubit8_t *p = (udi_ubit8_t *)mcap->ioc_data;

	printf("Got %d Multicast Address%s, %d bytes\n",
	       mcap->ioc_num, mcap->ioc_num != 1 ? "es" : "", mcap->ioc_len);

	for (i = 0; i < mcap->ioc_num; i++) {
		printf("Address[%d] = %02x.%02x.%02x.%02x.%02x.%02x\n",
		       i, p[0], p[1], p[2], p[3], p[4], p[5]);
		p += 6;
	}
}

void
ndtest_dump_MAC(nsr_macaddr_t * macp)
{
	udi_ubit8_t *p = (udi_ubit8_t *)macp->ioc_data;

	printf("Address = %02x.%02x.%02x.%02x.%02x.%02x\n",
	       p[0], p[1], p[2], p[3], p[4], p[5]);
}

void
ndtest_enabled(udi_cb_t *cb,
	       udi_status_t status)
{
	printf("ndtest_enabled( cb = %p, status = %d )\n", cb, status);

	_OSDEP_EVENT_SIGNAL(&ndtest_ev);
}

#if 0
void
osnsr_rx_packet(udi_nic_rx_cb_t *cb)
{
	n_rx_packets++;

}
#endif
