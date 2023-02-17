/*
 * File: mapper/posix/net/netmap_posix.c
 *
 * POSIX/OS specific network mapper code
 * Provides the osnsr_xxxxx routines called out of the netmap driver
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
 *
 */
#define UDI_NIC_VERSION 0x101
#include <udi_env.h>
#include <udi_nic.h>
#include "netmap_posix.h"
#include "netmap.h"

#ifndef STATIC
#define	STATIC static
#endif	/* STATIC */

#define	MY_MIN(a, b)	((a) < (b) ? (a) : (b))

/*
 * Constants
 */
static const char *mapper_name = "netmap";	/* Test-frame specific */

/*
 * Forward declarations
 */

typedef struct _osnsr_data {
	_nsr_q_head	Rx_data;		/* Data pending from link */
	_nsr_q_head	Rx_avail;		/*Available queues for Rx data*/
	_nsr_q_head	Reqq;			/* Queue for requests */
	udi_ubit32_t	os_resource;		/* Allocated resources */
	udi_ubit32_t	os_resource_rqst;	/* Resources requested */
} _osnsr_data_t;

STATIC void NSR_enable( udi_cb_t *, void (*)(udi_cb_t *, udi_status_t ) );
STATIC void NSR_disable( udi_cb_t *, void(*)(udi_cb_t *, udi_status_t ) );
STATIC void NSR_return_packet( udi_nic_rx_cb_t * );
STATIC void osnsr_tx_packet( nsrmap_Tx_region_data_t *, void *, udi_size_t );
STATIC void NSR_ctrl_req( udi_nic_ctrl_cb_t * );
nsrmap_Tx_region_data_t *NSR_txdata( nsrmap_region_data_t * );

/*
 * -----------------------------------------------------------------------------
 * OS-specific Interface routines [called by nsrmap.c]
 * -----------------------------------------------------------------------------
 */
STATIC void osnsr_init( nsrmap_region_data_t * );
STATIC void osnsr_bind_done( udi_channel_event_cb_t *, udi_status_t );
STATIC void osnsr_unbind_done( udi_nic_cb_t * );
STATIC void osnsr_status_ind( udi_nic_status_cb_t * );
STATIC udi_ubit32_t osnsr_max_xfer_size( udi_ubit8_t );
STATIC udi_ubit32_t osnsr_min_xfer_size( udi_ubit8_t );
STATIC void osnsr_rx_packet( udi_nic_rx_cb_t *, udi_boolean_t );
STATIC void osnsr_ctrl_ack( udi_nic_ctrl_cb_t *, udi_status_t );
STATIC udi_ubit32_t osnsr_mac_addr_size( udi_ubit8_t );
STATIC void osnsr_final_cleanup( udi_mgmt_cb_t * );

/*
 * -----------------------------------------------------------------------------
 * OS-specific mapper routines [not called from netmap.c]
 * -----------------------------------------------------------------------------
 */
STATIC void l_osnsr_txbuf_alloc( udi_cb_t *, udi_buf_t * );
STATIC void l_osnsr_open_complete( udi_cb_t *, udi_status_t );
STATIC void l_nsrmap_ioctl_cb1( udi_cb_t *, udi_cb_t * );
STATIC void l_nsrmap_ioctl_cb2( udi_cb_t *, udi_buf_t * );
STATIC void l_nsrmap_ioctl_cb3( udi_cb_t *, udi_buf_t * );
STATIC udi_boolean_t NETMAP_MCA_MATCH( udi_nic_ctrl_cb_t * );
STATIC udi_boolean_t l_netmap_mca_found( udi_ubit8_t *, 
					netmap_osdep_t *, 
					udi_size_t );
STATIC void l_netmap_mca_add( udi_ubit8_t *, netmap_osdep_t *, udi_size_t );
STATIC void l_netmap_mca_del( udi_ubit8_t *, netmap_osdep_t *, udi_size_t );
STATIC void l_netmap_req_remove( udi_ubit8_t *, netmap_osdep_t *, udi_size_t );

STATIC void l_osnsr_enqueue_callback( udi_cb_t *, void (*)(udi_cb_t *) );
STATIC void l_osnsr_tx_data( udi_cb_t * );

/*
 * OS specific code:
 */

/*
 * =============================================================================
 * 		O P E N   /   C L O S E	   H A N D L E R S
 * =============================================================================
 */

/*
 * nsrmap_open:
 * -----------
 * Initiate link connection for given channel. Once we return the underlying
 * link is ready for data transfers.
 */
int
nsrmap_open( udi_ubit32_t devnum )
{
	nsrmap_region_data_t	*rdata;

	rdata = _udi_MA_local_data( mapper_name, devnum );

	if ( rdata == NULL ) {
		return -1;
	} else {
		/* Enforce single open() access */
		_OSDEP_MUTEX_LOCK( &rdata->osdep_ptr->op_mutex );
		if ( rdata->nopens > 0 ) {
			_OSDEP_MUTEX_UNLOCK( &rdata->osdep_ptr->op_mutex );
			return -1;	/* EBUSY */
		} else {
			rdata->nopens++;
		}
		_OSDEP_MUTEX_UNLOCK( &rdata->osdep_ptr->op_mutex );

		if ( rdata->bind_status != UDI_OK ) {
			return -1;	/* ENXIO */
		}
#if 0
		NSR_enable( rdata->tx_cb, l_osnsr_open_complete );
#else
		NSR_enable( rdata->bind_cb, l_osnsr_open_complete );
#endif
		_OSDEP_EVENT_WAIT( &rdata->osdep_ptr->op_close_sv );
#ifdef NET_DEBUG
		_OSDEP_PRINTF("\007OPEN!\n");
#endif

		return -(rdata->open_status);
	}
}

STATIC void
l_osnsr_open_complete( udi_cb_t *cb, udi_status_t status )
{
	nsrmap_region_data_t	*rdata = cb->context;

	rdata->open_status = status;

	_OSDEP_EVENT_SIGNAL( &rdata->osdep_ptr->op_close_sv );
	_OSDEP_PRINTF("Waking up open/close process\n");
}

/*
 * nsrmap_close:
 * ------------
 * Tear down link for given channel. Once we return the link is no longer
 * available for data transfer
 */
int
nsrmap_close( udi_ubit32_t devnum )
{
	nsrmap_region_data_t	*rdata;

	rdata = _udi_MA_local_data( mapper_name, devnum );

	if ( rdata == NULL ) {
		return -1;
	} else {

		NSR_disable( rdata->bind_cb, l_osnsr_open_complete );

		/* Wait for NSR_disable to complete */
		_OSDEP_EVENT_WAIT( &rdata->osdep_ptr->op_close_sv );
#ifdef NET_DEBUG
		_OSDEP_PRINTF("\007CLOSED!\n");
#endif

		if ( BTST(rdata->osdep_ptr->status, OSNsrmapTx_Waiting) ) {
			_OSDEP_EVENT_SIGNAL( &rdata->osdep_ptr->sv1 );
		}
		if ( BTST(rdata->osdep_ptr->status, OSNsrmapRx_Waiting) ) {
			_OSDEP_EVENT_SIGNAL( &rdata->osdep_ptr->sv2 );
		}

		_OSDEP_MUTEX_LOCK( &rdata->osdep_ptr->op_mutex );
		rdata->nopens = 0;
		_OSDEP_MUTEX_UNLOCK( &rdata->osdep_ptr->op_mutex );
	}
	return 0;
}

/*
 * =============================================================================
 *	I O C T L     H A N D L E R
 * =============================================================================
 */
/*
 * nsrmap_ioctl:
 * ------------
 * Handle user supplied requests to the underlying network driver. These will
 * typically include setting MAC addresses, querying link state etc. etc.
 * This routine needs to allocate a udi_nic_ctrl_cb_t to contain the request
 * and a buffer to hold the data. This is done using the 
 * rdata->osdep_ptr->ioc_sv synchronisation variable to synchronise the
 * allocation and despatch of the NSR_ctrl_req().
 * On completion of the acknowledgement (osnsr_ctrl_ack) this routine will be
 * unblocked and will return the ioctl status to the user, along with any
 * appropriate data.
 */

int
nsrmap_ioctl( udi_ubit32_t devnum, int cmd, void *arg )
{
	nsrmap_region_data_t	*rdata;
	nsr_mca_struct_t	l_mca;
	nsr_macaddr_t		l_macaddr;
	void			*l_datap = NULL;
	void			*ret_locn = NULL;
	udi_size_t		ret_len = 0;
	netmap_osdep_t		*osdp;

	rdata = _udi_MA_local_data( mapper_name, devnum );

	if ( rdata == NULL ) {
		return -1;
	}

	osdp = rdata->osdep_ptr;

	osdp->ioc_cmd = ( udi_ubit32_t )cmd;
	/*
 	 * Obtain the user-specified commands and data from the <arg> parameter
	 * These will be stored in allocated memory and referenced by the
	 * rdata osdep_ptr field to allow the UDI allocation routines to also
	 * access them.
	 */
	switch( cmd ) {

	case POSIX_ND_SETMCA:
	case POSIX_ND_DELMCA:
	case POSIX_ND_DELALLMCA:
		/* Obtain list of multicast addresses */
		NETMAP_COPYIN(arg, &l_mca, sizeof(l_mca));
		/* Allocate sufficient space for address list */
		if ( cmd != POSIX_ND_DELALLMCA ) {
			l_datap=_OSDEP_MEM_ALLOC(l_mca.ioc_len, 0, UDI_WAITOK);
			NETMAP_COPYIN(l_mca.ioc_data, l_datap, l_mca.ioc_len);
		}
		osdp->ioc_datap = l_datap;
		osdp->ioc_cmdp  = &l_mca;
		break;
	
	case POSIX_ND_GETADDR:
	case POSIX_ND_SETADDR:
	case POSIX_ND_GETRADDR:
		/* Obtain user data buffer */
		NETMAP_COPYIN(arg, &l_macaddr, sizeof(l_macaddr));
		/* Allocate sufficient space for MAC address */
		l_datap = _OSDEP_MEM_ALLOC(l_macaddr.ioc_len, 0, UDI_WAITOK);
		NETMAP_COPYIN(l_macaddr.ioc_data, l_datap, l_macaddr.ioc_len);
		osdp->ioc_datap = l_datap;
		osdp->ioc_cmdp  = &l_macaddr;
		if ( cmd  != POSIX_ND_SETADDR ) {
			ret_locn = l_macaddr.ioc_data;
			ret_len  = l_macaddr.ioc_len;
		}
		break;

	case POSIX_ND_SETALLMCA:
	case POSIX_ND_PROMISC_ON:
	case POSIX_ND_PROMISC_OFF:
	case POSIX_ND_HWRESET:
		/* No data needed for this operation */
		osdp->ioc_datap = NULL;
		osdp->ioc_cmdp  = NULL;
		break;
	
	case POSIX_ND_BAD_RXPKT:
		/* Need to pass the number of bytes to be returned to NSR */
		osdp->ioc_datap = NULL;
		osdp->ioc_cmdp  = arg;	/* Sleaze alert */
		break;
	
	case POSIX_ND_RETMCA:
		/* Return list of active multicast addresses */
		{
			udi_size_t	addr_size, amount, MCA_size;

			addr_size = osnsr_mac_addr_size(
					rdata->link_info.media_type);
			MCA_size = osdp->MCA_list.n_entries * addr_size;
			
			NETMAP_COPYIN(arg, &l_mca, sizeof(l_mca));

			amount = MY_MIN( MCA_size, l_mca.ioc_len);
			NETMAP_COPYOUT(osdp->MCA_list.mca_table, l_mca.ioc_data,
					amount);
			
			l_mca.ioc_num = osdp->MCA_list.n_entries;
			l_mca.ioc_len = MCA_size;
			NETMAP_COPYOUT(&l_mca, arg, sizeof(l_mca));
		}
		return 0;
	default:
		/* Unsupported ioctl request */
		return -1;
	}

	/*
	 * Allocate the ctrl_cb
	 */
	udi_cb_alloc( l_nsrmap_ioctl_cb1, rdata->bind_cb, NSRMAP_CTRL_CB_IDX,
		      rdata->bind_info.bind_channel );
	
	/*
	 * Wait for the osnsr_ctrl_ack to complete. At this point we will know
	 * if we have data to return.
	 */
	_OSDEP_EVENT_WAIT( &osdp->ioc_sv );


	/*
	 * Get the updated data reference from the command. This may have been
	 * changed during processing of the request so we cannot make any
	 * assumptions about the state of l_datap.
	 */
	l_datap = osdp->ioc_datap;

	if ( osdp->ioc_copyout && 
	    (osdp->ioc_retval == UDI_OK) ) {
		/* Return data to the application in the appropriate location */
		NETMAP_COPYOUT( l_datap, ret_locn, ret_len );
	}
	
	/*
	 * Release buffer [if any]
	 */
	udi_buf_free( UDI_MCB(osdp->ioc_cb, udi_nic_ctrl_cb_t)->data_buf );

	/*
	 * Return the CB storage that we allocated
	 */
	udi_cb_free( osdp->ioc_cb );
	
	/*
	 * Return the local data block that we allocated
	 */
	if ( l_datap ) {
		_OSDEP_MEM_FREE( l_datap );
	}

	return osdp->ioc_retval;
}

/*
 * l_nsrmap_ioctl_cb1:
 * -----------------
 * Called on completion of the udi_cb_alloc for the udi_nic_ctrl_cb_t to be
 * used for the NSR_ctrl_req()
 * This routine is responsible for filling in the necessary fields of the new
 * cb and for updating the ND-specific multicast address lists where necessary.
 */
STATIC void
l_nsrmap_ioctl_cb1(
	udi_cb_t	*gcb,
	udi_cb_t	*new_cb )
{
	nsrmap_region_data_t	*rdata = gcb->context;
	udi_nic_ctrl_cb_t	*ctrl_cb = UDI_MCB( new_cb, udi_nic_ctrl_cb_t );
	nsr_mca_struct_t	*mcap;
	nsr_macaddr_t		*macaddrp;
	netmap_osdep_t		*osdp = rdata->osdep_ptr;
	udi_size_t		addr_size;

	/*
	 * Construct the appropriate udi_nic_ctrl_cb_t for the command.
	 */
	
	/* Get OS dependent MAC address size for DELMCA, SETMCA cmds */
	addr_size = osnsr_mac_addr_size( rdata->link_info.media_type );

	/* Save reference to ctrl_cb for completion */
	osdp->ioc_cb = new_cb;

	osdp->ioc_copyout = FALSE;
	osdp->ioc_retval = UDI_OK;

	switch ( osdp->ioc_cmd ) {

	case POSIX_ND_SETMCA:		/* Set MCA to values held in arg */
		mcap = (nsr_mca_struct_t *)osdp->ioc_cmdp;
		ctrl_cb->command = UDI_NIC_ADD_MULTI;	/* command	*/
		ctrl_cb->indicator = mcap->ioc_num;	/* # of entries */

		/*
		 * TODO: Verify that the specified Multicast Address is not
		 *	 already in the list of assigned MCAs. If it is, we
		 *	 don't set it.
		 *	 We also need to construct the list of active MCAs so
		 *	 that the ND can simply set these if it cannot use the
		 *	 list of new addresses to be added.
		 *	 The buffer contents will look like this:
		 *
		 *   <New MCA1><New MCA2>|<existing MCA...><New MCA1><New MCA2>
		 *
		 *	Each of the MCAs will be in media-specific format as
		 *	determined by the OS. If the ND has a _different_
		 *	expectation we have to fail the request.
		 */

		if (NETMAP_MCA_MATCH( ctrl_cb )){
			/*
			 * Found a new entry to add, the private active list
			 * has been updated and the ctrl_cb fields too.
			 */
			UDI_BUF_ALLOC( l_nsrmap_ioctl_cb2, UDI_GCB(ctrl_cb),
				       osdp->ioc_datap, 
				       mcap->ioc_num * addr_size,
				       rdata->buf_path );
		} else {
			/*
			 * No new entry to add. Simply flag a completion so
			 * that the ioctl will return to the user
			 */
			osdp->ioc_retval = UDI_OK;
			osdp->ioc_copyout = FALSE;
			_OSDEP_EVENT_SIGNAL( &osdp->ioc_sv );
		}
		break;

	case POSIX_ND_DELMCA:		/* Remove MCA(s) held in arg	 */
		mcap = (nsr_mca_struct_t *)osdp->ioc_cmdp;
		ctrl_cb->command = UDI_NIC_DEL_MULTI;
		ctrl_cb->indicator = mcap->ioc_num;	/* # to delete	*/

		if (NETMAP_MCA_MATCH( ctrl_cb )){
			/*
			 * Entry was found (and deleted) from our active list
			 * Send the new list to the ND
			 */
			UDI_BUF_ALLOC( l_nsrmap_ioctl_cb2, UDI_GCB(ctrl_cb),
					osdp->ioc_datap, 
					mcap->ioc_num * addr_size,
					rdata->buf_path );
		} else {
			/*
			 * Entry not in list, so nothing to delete.
			 */
			osdp->ioc_retval = UDI_OK;
			osdp->ioc_copyout = FALSE;
			_OSDEP_EVENT_SIGNAL( &osdp->ioc_sv );
		}
		break;
		
	case POSIX_ND_GETADDR:		/* Get current MAC address	 */
	case POSIX_ND_GETRADDR:		/* Get Factory (real) MAC address*/
		macaddrp = (nsr_macaddr_t *)osdp->ioc_cmdp;
		if ( osdp->ioc_cmd == POSIX_ND_GETADDR ) {
			ctrl_cb->command = UDI_NIC_GET_CURR_MAC;
		} else {
			ctrl_cb->command = UDI_NIC_GET_FACT_MAC;
		}
		osdp->ioc_copyout = TRUE;

		/* Allocate a buffer large enough for UDI_NIC_MAC_ADDRESS_SIZE*/
		UDI_BUF_ALLOC( l_nsrmap_ioctl_cb3, UDI_GCB(ctrl_cb), NULL,
				UDI_NIC_MAC_ADDRESS_SIZE,
				rdata->buf_path );
		break;

	case POSIX_ND_SETADDR:		/* Set current MAC address	 */
		macaddrp = (nsr_macaddr_t *)osdp->ioc_cmdp;

		ctrl_cb->command = UDI_NIC_SET_CURR_MAC;
		ctrl_cb->indicator = macaddrp->ioc_len;

		/* Allocate a data buffer containing the MAC address */
		UDI_BUF_ALLOC( l_nsrmap_ioctl_cb3, UDI_GCB(ctrl_cb),
				osdp->ioc_datap, macaddrp->ioc_len,
				rdata->buf_path );
		break;

	case POSIX_ND_SETALLMCA:	/* Enable ND to receive all MCA	 */
		ctrl_cb->command = UDI_NIC_ALLMULTI_ON;

		NSR_ctrl_req( ctrl_cb );
		break;

	case POSIX_ND_DELALLMCA:	/* Disable reception of all MCA	 *
					 * except those specified in arg */
		mcap = (nsr_mca_struct_t *)osdp->ioc_cmdp;

		ctrl_cb->command = UDI_NIC_ALLMULTI_OFF;
		ctrl_cb->indicator = mcap->ioc_num;
		(void)NETMAP_MCA_MATCH( ctrl_cb );

		/* Allocate a data buffer to contain the new MCAs (if any) */
		if ( mcap->ioc_num > 0 ) {
			UDI_BUF_ALLOC( l_nsrmap_ioctl_cb2, UDI_GCB(ctrl_cb),
					osdp->ioc_datap,
					mcap->ioc_num * addr_size,
					rdata->buf_path );
		} else {
			/* No MCAs will be active after this command */
			NSR_ctrl_req( ctrl_cb );
		}
		break;

	case POSIX_ND_PROMISC_ON:	/* Set promiscuous mode ON	 */
		ctrl_cb->command = UDI_NIC_PROMISC_ON;

		NSR_ctrl_req( ctrl_cb );
		break;

	case POSIX_ND_PROMISC_OFF:	/* Set promiscuous node OFF	 */
		ctrl_cb->command = UDI_NIC_PROMISC_OFF;

		NSR_ctrl_req( ctrl_cb );
		break;

	case POSIX_ND_HWRESET:		/* Reset ND			 */
		ctrl_cb->command = UDI_NIC_HW_RESET;

		NSR_ctrl_req( ctrl_cb );
		break;

	case POSIX_ND_BAD_RXPKT:	/* Specify bad Rx packet handling*/
		ctrl_cb->command = UDI_NIC_BAD_RXPKT;
		ctrl_cb->indicator = ( udi_ubit32_t )osdp->ioc_datap;

		NSR_ctrl_req( ctrl_cb );
		break;
	default:
		/* Unsupported ioctl operation, fail the request */
		osdp->ioc_retval = UDI_STAT_NOT_UNDERSTOOD;
		_OSDEP_EVENT_SIGNAL( &osdp->ioc_sv );
		break;
	}

	return;
}

/*
 * l_nsrmap_ioctl_cb2:
 * -----------------
 * Callback function for buffer allocation when using Multicast addresses.
 * This routine will append the full multicast table to the end of the new_buf
 * and then schedule the NSR_ctrl_req()
 */
STATIC void
l_nsrmap_ioctl_cb2(
	udi_cb_t	*gcb,
	udi_buf_t	*new_buf )
{
	nsrmap_region_data_t	*rdata = gcb->context;
	udi_nic_ctrl_cb_t	*ctrl_cb = UDI_MCB( gcb, udi_nic_ctrl_cb_t );
	udi_size_t		addr_size;
	netmap_osdep_t		*osdp = rdata->osdep_ptr;

	ctrl_cb->data_buf = new_buf;

	addr_size = osnsr_mac_addr_size( rdata->link_info.media_type );

	/*
	 * Copy the new multicast address table to the end of the buffer.
	 * This will have been updated by the NETMAP_MCA_MATCH routine before
	 * we are called
	 */
	udi_buf_write( l_nsrmap_ioctl_cb3, gcb, osdp->MCA_list.mca_table,
			osdp->MCA_list.n_entries * addr_size,
			new_buf,
			new_buf->buf_size,	/* Append */
			0,			/* Don't replace */
			UDI_NULL_BUF_PATH );
}

/*
 * l_nsrmap_ioctl_cb3:
 * -----------------
 * Callback function to send the completed udi_nic_ctrl_cb_t down to the ND.
 * The buffer has been completely filled with all applicable data by this time.
 */
STATIC void
l_nsrmap_ioctl_cb3(
	udi_cb_t	*gcb,
	udi_buf_t	*new_buf )
{
	udi_nic_ctrl_cb_t	*ctrl_cb = UDI_MCB( gcb, udi_nic_ctrl_cb_t );

	/*
	 * Release old (stale) buffer if it's been used
	 */
	if ( ctrl_cb->data_buf != new_buf ) {
		udi_buf_free( ctrl_cb->data_buf );
	}

	ctrl_cb->data_buf = new_buf;

	NSR_ctrl_req( ctrl_cb );
}

/*
 * NETMAP_MCA_MATCH:
 * ----------------
 * Maintain the list of active multicast addresses which are updated by 
 * UDI_NIC_ADD_MULTI, UDI_NIC_DEL_MULTI, and UDI_NIC_ALLMULTI_OFF
 * Each of these commands needs to update the table in some manner.
 *
 * UDI_NIC_ADD_MULTI:
 *	If the given MCA is already in the active list, remove it from the
 *	request.
 *	Return TRUE if there is a new MCA to add, and update the MCA_list with
 *	it
 *
 * UDI_NIC_DEL_MULTI:
 *	If the given MCA is not in the table, remove it from the request.
 *	Return TRUE if the MCA was in the table, and update the MCA_list by
 *	deleting the entry.
 *
 * UDI_NIC_ALLMULTI_OFF:
 *	Clear the MCA_list table and replace the entries by those specified in
 *	the request.
 *	Returns TRUE.
 */
STATIC udi_boolean_t 
NETMAP_MCA_MATCH(
	udi_nic_ctrl_cb_t	*cb )
	
{
	nsrmap_region_data_t	*rdata = UDI_GCB(cb)->context;
	udi_boolean_t		retval = FALSE;
	netmap_osdep_t		*osdp = rdata->osdep_ptr;
	nsr_mca_struct_t	*mcap = osdp->ioc_cmdp;	/* Original request */
	udi_size_t		addr_size;
	udi_ubit8_t		*p, *req_end;
	udi_ubit32_t		req_len;

	addr_size = osnsr_mac_addr_size( rdata->link_info.media_type );
	req_len = mcap->ioc_len;

restart:
	switch( cb->command ) {
	case UDI_NIC_ADD_MULTI:
		/*
		 * Check for existing multicast entry in MCA_list. If found,
		 * remove the entry from the request's list. Otherwise add the
		 * entry to the new list.
		 * Continue until all entries have been processed.
		 */
		req_end = (udi_ubit8_t *)osdp->ioc_datap;
		req_end += req_len;
		for ( p = osdp->ioc_datap; p < req_end; p += addr_size ) {
			if ( l_netmap_mca_found( p, osdp, addr_size ) ) {
				/* remove entry from request */
				l_netmap_req_remove( p, osdp, addr_size );
				req_len -= addr_size;
				goto restart;
			} else {
				/* add request entry to MCA list */
				l_netmap_mca_add( p, osdp, addr_size );
				retval = TRUE;
			}
		}
		break;
	case UDI_NIC_DEL_MULTI:
		/*
		 * Check that the multicast entry exists in the MCA_list. If not
		 * remove it from the request. Otherwise, update the MCA_list by
		 * deleting the entry.
		 * Continue until all request entries have been processed.
		 */
		req_end = (udi_ubit8_t *)osdp->ioc_datap;
		req_end += req_len;
		for ( p = osdp->ioc_datap; p < req_end; p += addr_size ) {
			if ( l_netmap_mca_found( p, osdp, addr_size ) ) {
				/* remove request entry from MCA list */
				l_netmap_mca_del( p, osdp, addr_size );
				retval = TRUE;
			} else {
				/* remove entry from request */
				l_netmap_req_remove( p, osdp, addr_size );
				req_len -= addr_size;
				goto restart;
			}
		}
		break;
	case UDI_NIC_ALLMULTI_OFF:
		/*
		 * Clear the MCA_list table and initialise to the request's
		 * list.
		 */
		if ( osdp->MCA_list.n_entries ) {
			osdp->MCA_list.n_entries = 0;
			_OSDEP_MEM_FREE( osdp->MCA_list.mca_table );
		}
		req_end = (udi_ubit8_t *)osdp->ioc_datap;
		req_end += req_len;
		for ( p = osdp->ioc_datap; p < req_end; p+= addr_size ) {
			l_netmap_mca_add(p, osdp, addr_size);
		}
		retval = TRUE;
		break;
	default:
		break;
	}

	return retval;
}

/*
 * l_netmap_mca_found:
 * -----------------
 * Scan the MCA list held in <osdp> looking for a match against the MAC address
 * referenced by <p>
 */
STATIC udi_boolean_t
l_netmap_mca_found(
	udi_ubit8_t	*p,		/* MAC address */
	netmap_osdep_t	*osdp,		/* OS-dependent MCA list reference */
	udi_size_t	size )		/* OS-dependent MAC address size   */
{
	udi_ubit8_t	*q, *q_end;

	q = (udi_ubit8_t *)osdp->MCA_list.mca_table;
	q_end = q + osdp->MCA_list.n_entries * size;

	for ( ; q < q_end; q += size ) {
		if ( udi_memcmp(p, q, size) == 0 ) {
			return TRUE;
		}
	}
	return FALSE;
}

/*
 * l_netmap_mca_add:
 * ---------------
 * Add a new MAC address to the list of multicast addresses already active.
 * The existing list will be freed.
 */
STATIC void
l_netmap_mca_add(
	udi_ubit8_t	*p,		/* MAC address */
	netmap_osdep_t	*osdp,		/* MCA list reference */
	udi_size_t	size )		/* MAC address size */
{
	void		*new_list;
	udi_ubit8_t	*new_q;
	udi_size_t	old_size;

	old_size = osdp->MCA_list.n_entries * size;

	osdp->MCA_list.n_entries++;
	new_list = _OSDEP_MEM_ALLOC( size * osdp->MCA_list.n_entries, 0, 
					UDI_WAITOK );
	
	udi_memcpy(new_list, osdp->MCA_list.mca_table, old_size);

	new_q = (udi_ubit8_t *)new_list;
	new_q += old_size;

	udi_memcpy(new_q, p, size);

	if ( old_size ) {
		_OSDEP_MEM_FREE( osdp->MCA_list.mca_table);
	}
	osdp->MCA_list.mca_table = new_list;
}

/*
 * l_netmap_mca_del:
 * ---------------
 * Delete the specified multicast address from the MCA_list. The storage
 * associated will be released.
 */
STATIC void
l_netmap_mca_del(
	udi_ubit8_t	*p,		/* MAC address */
	netmap_osdep_t	*osdp,		/* MCA list reference */
	udi_size_t	size )		/* MAC address size */
{
	void		*new_list;
	udi_ubit8_t	*q, *q_start, *q_end, *listp;
	udi_size_t	new_size;

	new_size = osdp->MCA_list.n_entries * size;

	q_start = (udi_ubit8_t *)osdp->MCA_list.mca_table;
	q_end = q_start + new_size;

	new_size -= size;


	/* Find address in old list, copy start..address to new list */

	for ( q = q_start; q < q_end; q += size ) {
		if ( udi_memcmp(q, p, size) == 0 ) {
			if ( new_size ) {
				new_list = _OSDEP_MEM_ALLOC( new_size, 0, 
								UDI_WAITOK );
				/* Found the entry, copy start..q to new list */
				if ( q > q_start ) {
					udi_memcpy( new_list, q_start, 
							q - q_start );
				}
				/* Copy remaining entries skipping entry <q> */
				if ( q < q_end - size ) {
					listp = (udi_ubit8_t *)new_list;
					listp += q - q_start;	/* End */
					udi_memcpy( listp, q + size, 
							q_end - q - size );
				}
			} else {
				new_list = NULL;
			}
			_OSDEP_MEM_FREE( osdp->MCA_list.mca_table );
			osdp->MCA_list.n_entries--;
			osdp->MCA_list.mca_table = new_list;
			break;
		}
	}
}

/*
 * l_netmap_req_remove:
 * ------------------
 * Remove the specified MAC address from the user-supplied request. The 
 * associated data area will be updated and reallocated to remove the address.
 */
STATIC void
l_netmap_req_remove(
	udi_ubit8_t	*p,		/* MAC address */
	netmap_osdep_t	*osdp,		/* Originating request reference */
	udi_size_t	size )		/* MAC address size */
{
	nsr_mca_struct_t	*mcap;
	udi_ubit8_t		*q, *q_start, *q_end, *listp;
	void			*new_list;
	udi_size_t		new_size;

	mcap = osdp->ioc_cmdp;				/* Command header */
	q_start = (udi_ubit8_t *)osdp->ioc_datap;	/* Command data   */
	q_end = q_start + mcap->ioc_len;

	new_size = mcap->ioc_len - size;

	if ( new_size ) {
		/* Reallocation needed */
		for ( q = q_start; q < q_end; q += size ) {
			if ( udi_memcmp(q, p, size) == 0 ) {
				/* Found the entry, copy start of area */
				new_list = _OSDEP_MEM_ALLOC( new_size, 0,
								UDI_WAITOK );
				if ( q > q_start ) {
					udi_memcpy( new_list, q_start,
								q - q_start );
				}
				/* Copy remaining entries */
				if ( q < q_end - size ) {
					listp = (udi_ubit8_t *)new_list;
					listp += q - q_start;	/* End */
					udi_memcpy( listp, q + size, 
							q_end - q - size );
				}
				_OSDEP_MEM_FREE( osdp->ioc_datap );
				mcap->ioc_num--;
				mcap->ioc_len = new_size;
				osdp->ioc_datap = new_list;
				break;
			}
		}
	} else {
		/* Simply clear the data */
		mcap->ioc_num--;
		mcap->ioc_len -= size;
		_OSDEP_MEM_FREE( osdp->ioc_datap );
		osdp->ioc_datap = NULL;
	}
}

/*
 * =============================================================================
 *		D A T A   T R A N S F E R   H A N D L E R S
 * =============================================================================
 */
/*
 * nsrmap_sendmsg:
 * --------------
 * Send a network data packet out over the link. Blocks until there is a
 * transmit request control block available, or the link is brought down.
 *
 * Return Value:
 *	-1	=> no resources available
 *	 0	=> successful transmission
 */
int
nsrmap_sendmsg( udi_ubit32_t devnum, nsrmap_uio_t *arg )
{
	nsrmap_region_data_t	*rdata;
	nsrmap_Tx_region_data_t	*txdata;

	rdata = _udi_MA_local_data( mapper_name, devnum );

	txdata = NSR_txdata( rdata );

	if ( txdata == NULL ) {
		return -1;	/* ENXIO */
	} else {
		/* Wait until Tx control block is available */
		while ( NSR_UDI_QUEUE_EMPTY( &txdata->cbq ) && 
			BTST(txdata->NSR_state, NsrmapS_Enabled) ) {
			BSET(rdata->osdep_ptr->status, OSNsrmapTx_Waiting);
			_OSDEP_EVENT_WAIT( &rdata->osdep_ptr->sv1 );
			BCLR(rdata->osdep_ptr->status, OSNsrmapTx_Waiting);
		}
		/* Check that link is still available */
		if ( !BTST(txdata->NSR_state, NsrmapS_Enabled) ) {
			return -1;	/* EIO */
		}

		/* Send the packet */
		osnsr_tx_packet( txdata, arg->u_addr, arg->u_size );
	}
	return 0;
}

/* 
 * nsrmap_data_avail:
 * -----------------
 * Returns TRUE if data is available to be read, FALSE otherwise
 */
udi_boolean_t 
nsrmap_data_avail( udi_ubit32_t devnum )
{
	nsrmap_region_data_t	*rdata;
	

	rdata = _udi_MA_local_data( mapper_name, devnum );
	if ( rdata == NULL ) {
		return FALSE;
	} else {
		return !NSR_UDI_QUEUE_EMPTY( &rdata->rxcbq );
	}
}

/*
 * nsrmap_getmsg:
 * -------------
 * Get an incoming data message from the link. Blocks until data is available
 * or the link is brought down.
 * The u_size member of <arg> is updated to reflect the amount of data present
 * in the incoming packet. If <u_size> is less than the amount of data in the
 * packet, only <u_size> bytes are transferred to the user.
 * The originating Rx packet is returned to the NSR.
 */
void
nsrmap_getmsg( udi_ubit32_t devnum, nsrmap_uio_t *arg )
{
	nsrmap_region_data_t	*rdata;
	nsr_cb_t		*pcb;
	udi_nic_rx_cb_t		*rxcb;
	udi_ubit32_t		amount;

	rdata = _udi_MA_local_data( mapper_name, devnum );
	if ( rdata == NULL ) {
		arg->u_size = 0;
	} else {

		/* Wait until Rx control block is returned from ND */
		while ( NSR_UDI_QUEUE_EMPTY( &rdata->rxcbq ) &&
			BTST(rdata->NSR_state, NsrmapS_Enabled) ) {
			BSET(rdata->osdep_ptr->status, OSNsrmapRx_Waiting);
			_OSDEP_EVENT_WAIT( &rdata->osdep_ptr->sv2 );
			BCLR(rdata->osdep_ptr->status, OSNsrmapRx_Waiting);
		}
		if ( !BTST(rdata->NSR_state, NsrmapS_Enabled) ) {
			arg->u_size = 0;
			return;
		}

		/*
		 * Copy the data from the Rx CB to the user buffer and return
		 * the CB to the ND
		 */
		pcb = (nsr_cb_t *)NSR_UDI_DEQUEUE_HEAD( &rdata->rxcbq );

		rxcb = UDI_MCB( pcb->cbp, udi_nic_rx_cb_t );

		/*
		 * Break the chain of received messages as we only want to
		 * see one at a time
		 */
		rxcb->chain = NULL;

		/* Only copy data out if an error has not been encountered */
		if ( rxcb->rx_status == 0 ) {
			amount = MY_MIN(arg->u_size, 
					rxcb->rx_buf->buf_size);
			arg->u_size = amount;
			udi_buf_read( rxcb->rx_buf, 0, amount, arg->u_addr );
		} else {
			/* TODO: Update error status counts */
			arg->u_size = 0;
		}

		/* Return the Rx CB to the underlying ND */
		NSR_return_packet( rxcb );
	}
	return;
}


/*
 * -----------------------------------------------------------------------------
 * OS-specific routines called by nsrmap.c
 * -----------------------------------------------------------------------------
 */
/*
 * osnsr_init:
 * ----------
 * Perform OS-specific initialisation of region-local data. Must not block.
 *
 * Calling Context:
 *	NSR Primary region
 */
STATIC void
osnsr_init( nsrmap_region_data_t *rdata )
{
	netmap_osdep_t	*osdp;

	/* Initialise the osdep_ptr field */
	rdata->osdep_ptr = (netmap_osdep_t *)&rdata->osdep[0];
	osdp = rdata->osdep_ptr;

	_OSDEP_EVENT_INIT( &osdp->sv1 );
	_OSDEP_EVENT_INIT( &osdp->sv2 );
	_OSDEP_EVENT_INIT( &osdp->op_close_sv );
	_OSDEP_MUTEX_INIT( &osdp->op_mutex, "NSR Open Lock" );

	_OSDEP_EVENT_INIT( &osdp->ioc_sv );

	/* Setup to use RX checksum offload */
	rdata->capabilities = UDI_NIC_CAP_USE_RX_CKSUM;
}

/*
 * osnsr_final_cleanup:
 * -------------------
 * Remove any OS-specific region allocated data.
 */
STATIC void
osnsr_final_cleanup( udi_mgmt_cb_t *cb )
{
	nsrmap_region_data_t	*rdata = UDI_GCB(cb)->context;
	netmap_osdep_t		*osdp = rdata->osdep_ptr;

	_OSDEP_EVENT_DEINIT( &osdp->sv1 );
	_OSDEP_EVENT_DEINIT( &osdp->sv2 );
	_OSDEP_EVENT_DEINIT( &osdp->op_close_sv );
	_OSDEP_MUTEX_DEINIT( &osdp->op_mutex );
	_OSDEP_EVENT_DEINIT( &osdp->ioc_sv );

	if ( osdp->MCA_list.n_entries > 0 ) {
		_OSDEP_MEM_FREE( osdp->MCA_list.mca_table );
	}

	return;
}

/*
 * osnsr_status_ind:
 * ----------------
 * Called whenever a udi_nsr_status_ind() is issued by the ND. This routine can
 * propagate the state change to the OS as appropriate.
 */
STATIC void
osnsr_status_ind( udi_nic_status_cb_t *cb )
{
}

/*
 * osnsr_max_xfer_size:
 * -------------------
 * Return the OS specific maximum transfer size for a given media type.
 * This will depend on the configuration layout of the network stack +
 * demux modules.
 */
STATIC udi_ubit32_t 
osnsr_max_xfer_size( 
	udi_ubit8_t media_type )
{
	switch( media_type ) {
	case UDI_NIC_FDDI:
		return 4500;
	case UDI_NIC_ETHER:
	case UDI_NIC_TOKEN:
	case UDI_NIC_FASTETHER:
	case UDI_NIC_GIGETHER:
	case UDI_NIC_VGANYLAN:
	case UDI_NIC_ATM:
	case UDI_NIC_FC:
	default:
		return 1514;			/* Ethernet II frame size */
	}
}

/*
 * osnsr_min_xfer_size:
 * -------------------
 * Return the OS specific minimum transfer size for a given media type.
 */
STATIC udi_ubit32_t
osnsr_min_xfer_size(
	udi_ubit8_t media_type )
{
	switch( media_type ) {
		case UDI_NIC_FDDI:
		case UDI_NIC_ETHER:
		case UDI_NIC_TOKEN:
		case UDI_NIC_FASTETHER:
		case UDI_NIC_GIGETHER:
		case UDI_NIC_VGANYLAN:
		case UDI_NIC_ATM:
		case UDI_NIC_FC:
		default:
			return 14;		/* Link-layer header */
	}
}

/*
 * osnsr_mac_addr_size:
 * -------------------
 * Return the OS specific MAC address size for the given media type.
 */
STATIC udi_ubit32_t
osnsr_mac_addr_size(
	udi_ubit8_t media_type )
{
	switch( media_type ) {
	default:
		return 6;
	}
}


/*
 * osnsr_rx_packet:
 * ---------------
 * Wake up any sleeping user getmsg() to signal that there's some data to
 * read. Transfer a container from rxonq to rxcbq so that the sleeping
 * nsrmap_getmsg() can pick up the data.
 *
 * Calling Context:
 *	NSR Receive region
 */
void
osnsr_rx_packet(
	udi_nic_rx_cb_t	*rxcb,
	udi_boolean_t	expedited )
{
	nsrmap_region_data_t *rdata = UDI_GCB(rxcb)->context;
	nsr_cb_t		*pcb;

	rdata->n_packets++;

	pcb = (nsr_cb_t *)NSR_UDI_DEQUEUE_HEAD( &rdata->rxonq );
	pcb->cbp = UDI_GCB(rxcb);

	NSR_SET_SCRATCH( pcb->cbp->scratch, pcb );

	NSR_UDI_ENQUEUE_TAIL( &rdata->rxcbq, &pcb->q );
	
	if ( BTST(rdata->osdep_ptr->status, NsrmapRx_Waiting) ) {
		_OSDEP_EVENT_SIGNAL( &rdata->osdep_ptr->sv2 );
	}
}

/*
 * osnsr_tx_packet:
 * ---------------
 * Allocate a CB for the user-supplied write request and schedule a callback
 * into the NSR to complete the allocation / udi_nd_tx_req processing.
 * If the size of the request is too large for the underlying media, or we
 * have exhausted the Tx CB queue we drop the request.
 */
STATIC void
osnsr_tx_packet(
	nsrmap_Tx_region_data_t	*rdata,
	void			*userbuf,
	udi_size_t		size )
{
	nsr_cb_t		*pcb;
	udi_cb_t		*gcb;
	void			*l_userbuf;	/* Local copy */

	rdata->n_packets++;

	if ( rdata->buf_size < size ) {
		rdata->n_discards++;
		return;
	}

	if ( NSR_UDI_QUEUE_EMPTY( &rdata->cbq ) ) {
		rdata->n_discards++;
		return;
	} else {
		pcb = (nsr_cb_t *)NSR_UDI_DEQUEUE_HEAD( &rdata->cbq );

		/* Allocate a copy of the user data so that we don't have to
		 * block the user write() call
		 */
		l_userbuf = (void *)_OSDEP_MEM_ALLOC(size, 
			UDI_MEM_NOZERO|UDI_MEM_MOVABLE, UDI_WAITOK);
		udi_memcpy( l_userbuf, userbuf, size );

		gcb = pcb->cbp;
		NSR_SET_SCRATCH( gcb->scratch, pcb );	/* Reverse link */
		NSR_SET_SCRATCH_1( gcb->scratch, l_userbuf );
		NSR_SET_SCRATCH_2( gcb->scratch, (void *)size );

		/* 
		 * Enqueue callback to NSR
		 */
#ifdef NSR_USE_SERIALISATION
		l_osnsr_enqueue_callback( gcb, l_osnsr_tx_data );
#else
		l_osnsr_tx_data( gcb );
#endif	/* NSR_USE_SERIALISATION */
	}
}

/*
 * l_osnsr_enqueue_callback:
 * -----------------------
 * Schedule a callback to the NSR region. This serialises all access to the
 * region data structures and simplifies the locking requirements for the
 * structures
 *
 * Calling Context:
 *	Kernel
 */
STATIC void
l_osnsr_enqueue_callback(
	udi_cb_t	*gcb,
	void		(*cbfn)(udi_cb_t *) )
{
	_udi_channel_t	*chan = (_udi_channel_t *)gcb->channel;
	_udi_region_t *region = chan->chan_region;
	_udi_cb_t	*_cb = _UDI_CB_TO_HEADER(gcb);

	_cb->cb_flags |= _UDI_CB_CALLBACK;
	_cb->cb_func = (_udi_callback_func_t *)cbfn;
	/*
	 * Sleaze alert: we only need a callback that takes one arg.
	 * so we hijack the _UDI_CT_CANCEL op_idx. This should result
	 * in a delayed call to (*cbfn)( gcb )
	 */
	_cb->op_idx = _UDI_CT_CANCEL;	

	/* Lock region as we now may contend with already active thread */
	REGION_LOCK(region);
	/* Append the callback to the end of the region's requests */
	_UDI_REGION_Q_APPEND( region, &_cb->cb_qlink );

	if ( !REGION_IS_ACTIVE(region) && !REGION_IN_SCHED_Q(region)) {
		/* Schedule this region to become active */
		_OSDEP_REGION_SCHEDULE( region );
		return;
	} else {
		/* Region already scheduled */
		REGION_UNLOCK( region );
	}
}

/*
 * l_osnsr_tx_data:
 * --------------
 * Allocate a buffer to contain the user-data supplied by an earlier call to
 * osnsr_tx_packet. The user buffer and size are obtained from the scratch data
 * area of the CB
 */
STATIC void
l_osnsr_tx_data( udi_cb_t *gcb )
{
	void		*userbuf;
	udi_size_t	size;
	nsrmap_Tx_region_data_t	*rdata = gcb->context;

	NSR_GET_SCRATCH_1( gcb->scratch, userbuf );
	NSR_GET_SCRATCH_2( gcb->scratch, size );

	UDI_BUF_ALLOC( l_osnsr_txbuf_alloc, gcb, userbuf,
			size, rdata->buf_path );
}

/*
 * l_osnsr_txbuf_alloc:
 * ------------------
 * Callback for transmit buffer allocation. The data has been put in the
 * buffer. We need to obtain a Tx CB from our list and link the buffer to it.
 * Then call the NSR to transmit it.
 */
STATIC void
l_osnsr_txbuf_alloc(
	udi_cb_t	*gcb,
	udi_buf_t	*new_buf )
{
	nsrmap_Tx_region_data_t	*rdata = gcb->context;
	nsr_cb_t		*pcb;
	udi_nic_tx_cb_t		*txcb;
	void			*userbuf;

	NSR_GET_SCRATCH( gcb->scratch, pcb );
	NSR_GET_SCRATCH_1( gcb->scratch, userbuf );

	_OSDEP_MEM_FREE( userbuf );

	txcb = UDI_MCB( gcb, udi_nic_tx_cb_t );

	txcb->tx_buf = new_buf;

	NSR_UDI_ENQUEUE_TAIL(&rdata->onq, (udi_queue_t *)pcb);

	udi_nd_tx_req( txcb );
}

/*
 * osnsr_bind_done:
 * ---------------
 * Called when meta-specific bind has completed. This routine must call
 * udi_channel_event_complete after updating any OS specific data structures
 * to reflect the new binding instance.
 */
STATIC void
osnsr_bind_done(
	udi_channel_event_cb_t *cb,
	udi_status_t		status )
{
	udi_channel_event_complete(cb, status);
}

/*
 * osnsr_unbind_done:
 * -----------------
 * Called on completion of udi_nd_unbind_req and before the common code responds
 * to the UDI_DMGMT_UNBIND request.
 */
STATIC void
osnsr_unbind_done(
	udi_nic_cb_t	*cb )
{
	return;
}

/*
 * osnsr_ctrl_ack:
 * --------------
 * Called on completion of an earlier NSR_ctrl_req(). This needs to complete
 * the pending request and return control to the user application.
 */
STATIC void
osnsr_ctrl_ack(
	udi_nic_ctrl_cb_t	*cb,
	udi_status_t		status )
{
	nsrmap_region_data_t	*rdata = UDI_GCB(cb)->context;
	nsr_macaddr_t		*macaddrp;
	udi_size_t		xfer_sz;

	rdata->osdep_ptr->ioc_copyout = FALSE;
	rdata->osdep_ptr->ioc_retval = status;

	rdata->osdep_ptr->ioc_cb = UDI_GCB(cb);

	if ( status != UDI_OK ) {
		/* Return failure case */
		_OSDEP_EVENT_SIGNAL( &rdata->osdep_ptr->ioc_sv );
		return;
	}

	switch( cb->command ) {
	case UDI_NIC_ADD_MULTI:
	case UDI_NIC_DEL_MULTI:
	case UDI_NIC_ALLMULTI_ON:
	case UDI_NIC_ALLMULTI_OFF:
	case UDI_NIC_SET_CURR_MAC:
	case UDI_NIC_PROMISC_ON:
	case UDI_NIC_PROMISC_OFF:
	case UDI_NIC_HW_RESET:
	case UDI_NIC_BAD_RXPKT:
		/* No data to transfer to user */
		break;
	case UDI_NIC_GET_CURR_MAC:
	case UDI_NIC_GET_FACT_MAC:
		/* Return a MAC address to the user-supplied buffer */
		macaddrp = rdata->osdep_ptr->ioc_cmdp;

		/* Write the buffer data to the pre-allocated data area */
		xfer_sz = MY_MIN(cb->indicator, macaddrp->ioc_len);
		udi_buf_read( cb->data_buf, 0, xfer_sz, 
			      rdata->osdep_ptr->ioc_datap);
		
		rdata->osdep_ptr->ioc_copyout = TRUE;
		break;
	default:
		udi_assert( 0 );
		/* NOTREACHED */
	}

#if 0
	/*
	 * Release allocated buffer (if any)
	 */
	udi_buf_free( cb->data_buf );
#endif

	/*
	 * Awaken blocked ioctl(): it will complete the data copyout to the
	 * user buffer
	 */
	_OSDEP_EVENT_SIGNAL( &rdata->osdep_ptr->ioc_sv );
	return;
}

/* Function that gets called when we receive more xmit cb's */
#define osnsr_tx_rdy(x)

/*
 * Common Code:
 */
#include "netmap.c"
