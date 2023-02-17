
/*
 * File: scsimap_uw.c
 *
 * A mapper which converts SDI specific HBA requests to 
 * UDI peripheral driver requests.
 *
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

#define _DDI 8
#include <sys/ddi.h>
#include <sys/cmn_err.h>
#include <sys/errno.h>
#include <sys/ksynch.h>
#include <sys/debug.h>
#include <sys/lwp.h>
#include <sys/conf.h>

#include <sys/sdi.h>
#include <sys/sdi_hier.h>

#include "scsimap_uw.h"

#include "scsicommon/scsimap.c"

STATIC void *scsimap_hba[SDI_MAX_HBAS];

STATIC udi_queue_t scsimap_reg_list;
STATIC udi_boolean_t scsimap_reg_daemon_kill = FALSE;
STATIC _OSDEP_MUTEX_T scsimap_reg_lock;
STATIC _OSDEP_EVENT_T scsimap_reg_ev;

STATIC int scsimap_devflag =
	HBA_MP | HBA_CALLBACK | HBA_HOT | HBA_TIMEOUT_RESET;

extern int sdi_hot_add(struct scsi_adr *sap);
extern int sdi_rm_dev(struct scsi_adr *sap,
		      int rm_flag);
extern void sdi_enable_instance(HBA_IDATA_STRUCT * idatap);

/* TODO:
 * Find out correct max_xfer (third argument to HBA_INFO) value.
 * For now we use 0x10000 until UDI implements some way of passing
 * this information from HBA driver to peripheral driver.
 */
HBA_INFO(scsimap, &scsimap_devflag, 0x10000);

HBA_IDATA_STRUCT _scsimapidata[] = {
	{HBA_UW21_IDATA | HBA_EXT_ADDRESS, "(scsimap,1) SCSI Mapper v1.00"}
};

#define MAPPER_NAME "udiMscsi"		/* O.S. specific */

/* 
 * Since we allocate sblk structures from sm_pool the size of sblk_t 
 * should not exceed 28 bytes, if possible.
 */

typedef struct scsimap_sblk {
	struct xsb *sbp;
	udi_queue_t link;
} scsimap_sblk_t;

#define SCSIMAP_QCLASS(x)	((x)->sbp->sb.sb_type)

STATIC void osdep_scsimap_init(void);
STATIC void osdep_scsimap_deinit(void);
STATIC void osdep_scsimap_reg_daemon(void *arg);

STATIC int osdep_scsimap_load(void);
STATIC int osdep_scsimap_unload(void);

STATIC void osdep_scsimap_send(scsimap_region_data_t * rdata,
			       scsimap_sblk_t * sp);
STATIC void osdep_scsimap_send_io_req(scsimap_region_data_t * rdata,
				      udi_scsi_io_cb_t *scsi_io_cb,
				      scsimap_sblk_t * sp);
STATIC void osdep_scsimap_scsi_io_req(udi_cb_t *gcb,
				      udi_buf_t *new_buf);
STATIC void osdep_scsimap_next(scsimap_region_data_t * rdata);
STATIC void osdep_scsimap_func(scsimap_region_data_t * rdata,
			       scsimap_sblk_t * sp);
STATIC void osdep_scsimap_send_ctl_req(udi_cb_t *gcb,
				       udi_cb_t *new_cb);
STATIC void osdep_scsimap_io_nak(scsimap_req_t * req,
				 udi_status_t status,
				 udi_ubit8_t scsi_status,
				 udi_ubit8_t sense_status);
STATIC void osdep_scsimap_scsi_io_nak(udi_scsi_io_cb_t *scsi_io_cb,
				      udi_status_t req_status,
				      udi_ubit8_t scsi_status,
				      udi_ubit8_t sense_status,
				      udi_buf_t *sense_buf);

#ifdef NOTYET
_OSDEP_MAPPER_INFO _osdep_mapper_info {
	osdep_scsimap_load,		/* load function */

	osdep_scsimap_unload,		/* unload function */

	NULL				/* drvinfo structure */
}
#endif					/* NOTYET */
STATIC void
osdep_scsimap_init()
{
	osdep_scsimap_load();
}

STATIC int
osdep_scsimap_load()
{
	UDI_QUEUE_INIT(&scsimap_reg_list);
	_OSDEP_EVENT_INIT(&scsimap_reg_ev);
	_OSDEP_MUTEX_INIT(&scsimap_reg_lock,
			  "scsimap registration daemon lock");

	/*
	 * start the SDI registeration daemon 
	 */
	/*
	 * Spawn registration daemon 
	 */
	if (kthread_spawn(osdep_scsimap_reg_daemon, NULL,
			  "osdep_scsimap_reg_daemon", NULL) != 0) {
		/*
		 * Could not create the UDI SCSI registration daemon LWP.
		 * This is probably due to lack of memory.
		 */
		cmn_err(CE_PANIC,
			"Failed to create osdep_scsimap_reg_daemon LWP");
		/*
		 * NOTREACHED 
		 */
	}
}

STATIC void
osdep_scsimap_deinit()
{
	osdep_scsimap_unload();
}

STATIC int
osdep_scsimap_unload()
{
	scsimap_reg_daemon_kill = TRUE;
	_OSDEP_EVENT_SIGNAL(&scsimap_reg_ev);
	_OSDEP_MUTEX_DEINIT(&scsimap_reg_lock);
	_OSDEP_EVENT_DEINIT(&scsimap_reg_ev);
}

/*
 * struct hbadata *
 * scsimapgetblk(int sleepflag)
 *
 * Description:
 * Allocate the driver specific portion of the SB structure for the caller.
 *
 * Calling/Exit State:
 * None.
 */

struct hbadata *
scsimapgetblk(int sleepflag)
{
	scsimap_sblk_t *sp;

	sp = kmem_zalloc(sizeof (scsimap_sblk_t), sleepflag);

	return ((struct hbadata *)sp);
}

/*
 * int
 * scsimapfreeblk(struct hbadata *hbap, int hba_no)
 *
 * Description:
 * Release the previously allocated driver specific portion of SB structure. 
 * A nonzero return indicates an error in pointer or type field.
 *
 * Calling/Exit State:
 * None.
 */

long
scsimapfreeblk(struct hbadata *hbap)
{
	scsimap_sblk_t *sp = (scsimap_sblk_t *) hbap;

	kmem_free(sp, sizeof (scsimap_sblk_t));

	return ((long)SDI_RET_OK);
}

/*
 * TODO - Get the driver characteristics from the HD.
 * void
 * scsimapgetinfo(struct scsi_ad *sa, struct hbagetinfo *getinfop)
 *
 * Description:
 * Return the name and iotype of the given device.  The name is copied
 * into a string pointed to by the first field of the getinfo structure.
 *
 * Calling/Exit State:
 * None.
 */

void
scsimapgetinfo(struct scsi_ad *sa,
	       struct hbagetinfo *getinfop)
{
	if (getinfop->name)
		udi_snprintf(getinfop->name, SDI_NAMESZ, "HA %d TC %d",
			     SDI_HAN(sa), SDI_TCN_32(sa));

	getinfop->iotype = F_DMA_32 | F_SCGTH | F_RESID;

	if (getinfop->bcbp) {
		bcb_t *bcbp = getinfop->bcbp;
		physreq_t *physreqp = bcbp->bcb_physreqp;

		/*
		 * initialize the bcb 
		 */
		bcbp->bcb_addrtypes = BA_KVIRT;
		bcbp->bcb_flags = 0;
		bcbp->bcb_max_xfer = scsimaphba_info.max_xfer;
		bcbp->bcb_granularity = 1;

		/*
		 * initialize the physreq 
		 */
		physreqp->phys_align = 512;
		physreqp->phys_boundary = 0;
		physreqp->phys_dmasize = 32;
		/*
		 * physreqp->phys_max_scgth = 16; 
		 */
	}
}

/*
 * int
 * scsimapxlat(struct hbadata *hbap, int flag, proc_t *procp, int sleepflag)
 *
 * Description:
 * Perform the virtual to physical translation on the SCB data pointer.
 * Since we support only kernel virtual address buffers, 
 * we don't have to do any work here.
 *
 * Calling/Exit State:
 * None.
 */

 /*ARGSUSED*/ int
scsimapxlat(struct hbadata *hbap,
	    int flag,
	    proc_t * procp,
	    int sleepflag)
{
	return (SDI_RET_OK);
}

/*
 * long
 * scsimapsend(struct hbadata *hbap)
 *
 * Description:
 *  Send a SCSI command to a controller.  Commands sent via this
 *  function are executed in the order they are received.
 *
 * Calling/Exit State:
 * None.
 */

 /*ARGSUSED*/ long
scsimapsend(struct hbadata *hbap,
	    int sleepflag)
{
	scsimap_sblk_t *sp = (scsimap_sblk_t *) hbap;
	struct sb *sbp = &sp->sbp->sb;
	struct scsi_ad *sa = &sbp->SCB.sc_dev;
	scsimap_hba_t *hba;
	scsimap_region_data_t *rdata;

	ASSERT(sbp->sb_type == SCB_TYPE);
	ASSERT(sa);

	hba = scsimap_hba[SDI_HAN_32(sa)];
	ASSERT(hba);
	rdata = SCSIMAP_GET_RDATA(hba, SDI_BUS_32(sa),
				  SDI_TCN_32(sa), SDI_LUN_32(sa));
	ASSERT(rdata);

	sbp->SCB.sc_comp_code = SDI_PROGRES;
	sbp->SCB.sc_status = 0;

	osdep_scsimap_send(rdata, sp);

	return SDI_RET_OK;
}

/*
 * long
 * scsimapicmd(struct hbadata *hbap, int sleepflag)
 *
 * Description:
 * Send an immediate command.  If the logical unit is busy, the job
 * will be queued until the unit is free.  SFB operations will take
 * priority over SCB operations.
 *
 * Calling/Exit State:
 * None.
 */

 /*ARGSUSED*/ long
scsimapicmd(struct hbadata *hbap,
	    int sleepflag)
{
	scsimap_sblk_t *sp = (scsimap_sblk_t *) hbap;
	struct sb *sbp = &sp->sbp->sb;
	struct scsi_ad *sa;
	ulong_t b, t, l;
	scsimap_hba_t *hba;
	scsimap_region_data_t *rdata;

	switch (sbp->sb_type) {

	case ISCB_TYPE:{
			struct scs *cdb;
			struct ident inq;
			int len;

			sa = &sbp->SCB.sc_dev;
			ASSERT(sa);
			hba = scsimap_hba[SDI_HAN_32(sa)];
			ASSERT(hba);

			b = SDI_BUS_32(sa);
			t = SDI_TCN_32(sa);
			l = SDI_LUN_32(sa);
			cdb = (struct scs *)sbp->SCB.sc_cmdpt;

			/*
			 * Check if the scsi command is an Inquiry command for the
			 * host itself.
			 */
			if (cdb->ss_op == SS_INQUIR &&
			    t == hba->osdep.hba_idata->ha_chan_id[b]) {
				bzero((caddr_t) & inq, sizeof (struct ident));
				strncpy(inq.id_vendor,
					hba->osdep.hba_idata->name,
					VID_LEN + PID_LEN + REV_LEN);
				if (l == 0) {
					inq.id_type = ID_PROCESOR;
					inq.id_pqual = ID_QOK;
				} else {
					inq.id_type = ID_NODEV;
					inq.id_pqual = ID_QNOLU;
				}

				len = MIN(sbp->SCB.sc_datasz, sizeof (inq));
				bcopy(&inq, sbp->SCB.sc_datapt, len);

				sbp->SCB.sc_status = 0;
				sbp->SCB.sc_comp_code = SDI_ASW;
				sbp->SCB.sc_resid = sbp->SCB.sc_datasz - len;
				sdi_callback(sbp);
				return SDI_RET_OK;
			}

			rdata = SCSIMAP_GET_RDATA(hba, b, t, l);
			if (rdata == NULL) {
				sbp->SCB.sc_comp_code = SDI_NOTEQ;
				sdi_callback(sbp);
				return SDI_RET_OK;
			}

			osdep_scsimap_send(rdata, sp);
			return SDI_RET_OK;
		}

	case SFB_TYPE:

		sa = &sbp->SFB.sf_dev;
		ASSERT(sa);
		hba = scsimap_hba[SDI_HAN_32(sa)];
		ASSERT(hba);
		rdata = SCSIMAP_GET_RDATA(hba, SDI_BUS_32(sa), SDI_TCN_32(sa),
					  SDI_LUN_32(sa));
		ASSERT(rdata);

		sbp->SFB.sf_comp_code = SDI_ASW;

		switch (sbp->SFB.sf_func) {
		case SFB_ADD_DEV:
			break;

		case SFB_RESUME:
#if 0
			/*
			 * Return error if the sp is destined to a device
			 * that does not exist.
			 */
			if (rdata == (scsimap_region_data_t *) NULL) {
				sbp->SFB.sf_comp_code = SDI_NOSELE;
				return SDI_RET_ERR;
			}
#endif
			rdata->r_flags &= ~SCSIMAP_RDATA_SUSPENDED;
			osdep_scsimap_next(rdata);
			break;

		case SFB_SUSPEND:
#if 0
			/*
			 * Return error if the sp is destined to a device
			 * that does not exist.
			 */
			if (rdata == (scsimap_region_data_t *) NULL) {
				sbp->SFB.sf_comp_code = SDI_NOSELE;
				return SDI_RET_ERR;
			}
#endif
			rdata->r_flags |= SCSIMAP_RDATA_SUSPENDED;
			break;

		case SFB_ABORTM:
		case SFB_RESETM:

			sbp->SFB.sf_comp_code = SDI_PROGRES;
			osdep_scsimap_func(rdata, sp);
			return SDI_RET_OK;

		case SFB_RESET_DEVICE:
#if 0
			/*
			 * Return error if the sp is destined to a device
			 * that does not exist.
			 */
			if (rdata == (scsimap_region_data_t *) NULL) {
				sbp->SFB.sf_comp_code = SDI_NOSELE;
				return SDI_RET_ERR;
			}
#endif
			sbp->SFB.sf_comp_code = SDI_PROGRES;
			cmn_err(CE_NOTE,
				"scsimap: Resetting SCSI Device (%d:%d,%d,%d)",
				SDI_HAN_32(sa), SDI_BUS_32(sa), SDI_TCN_32(sa),
				SDI_LUN_32(sa));
			osdep_scsimap_func(rdata, sp);
			return SDI_RET_OK;

		case SFB_TIMEOUT_ON:
			hba->hba_flags |= SCSIMAP_HBA_TIMEOUT_ON;
			break;

		case SFB_TIMEOUT_OFF:
			hba->hba_flags &= ~SCSIMAP_HBA_TIMEOUT_ON;
			break;

		case SFB_FLUSHR:
			break;

		case SFB_NOPF:
			break;

		default:
			sbp->SFB.sf_comp_code = SDI_SFBERR;
			return SDI_RET_ERR;
		}

		sdi_callback(sbp);
		return SDI_RET_OK;

	default:
		return SDI_RET_ERR;
	}
}

/*
 * Send a request
 *
 * Calling/Exit State:
 *  None
 */

STATIC void
osdep_scsimap_send(scsimap_region_data_t * rdata,
		   scsimap_sblk_t * sp)
{
	udi_scsi_io_cb_t *scsi_io_cb;

	_OSDEP_MUTEX_LOCK(&rdata->r_lock);
	scsi_io_cb = scsimap_get_io_cb(rdata);
	if (scsi_io_cb == NULL) {
		int cls = SCSIMAP_QCLASS(sp);
		scsimap_sblk_t *tsp;
		udi_queue_t *elem, *tmp;

		/*
		 * put the job on the alloc queue in a priority order 
		 */
		tsp = UDI_BASE_STRUCT(UDI_LAST_ELEMENT(&rdata->alloc_q),
				      scsimap_sblk_t, link);
		if (cls <= SCSIMAP_QCLASS(tsp))
			UDI_ENQUEUE_TAIL(&rdata->alloc_q, &sp->link);
		else {
			UDI_QUEUE_FOREACH(&rdata->alloc_q, elem, tmp) {
				tsp = UDI_BASE_STRUCT(elem, scsimap_sblk_t,
						      link);
				if (cls > SCSIMAP_QCLASS(tsp)) {
					UDI_QUEUE_INSERT_BEFORE(elem,
								&sp->link);
					break;
				}
			}
		}
		_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);
		return;
	}
	_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);

	osdep_scsimap_send_io_req(rdata, scsi_io_cb, sp);
}

/*
 * Send the next request
 */
STATIC void
osdep_scsimap_next_io_req(scsimap_region_data_t * rdata,
			  udi_scsi_io_cb_t *scsi_io_cb)
{
	scsimap_sblk_t *sp;

	_OSDEP_MUTEX_LOCK(&rdata->r_lock);
	if (UDI_QUEUE_EMPTY(&rdata->alloc_q)) {
		/*
		 * Nothing to do 
		 */
		/*
		 * Return the CB resource back to the pool 
		 */
		scsimap_free_io_cb(rdata, scsi_io_cb);
		_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);
		return;
	}

	sp = UDI_BASE_STRUCT(UDI_DEQUEUE_HEAD(&rdata->alloc_q),
			     scsimap_sblk_t, link);
	_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);

	osdep_scsimap_send_io_req(rdata, scsi_io_cb, sp);
}

STATIC void
osdep_scsimap_next(scsimap_region_data_t * rdata)
{
	udi_scsi_io_cb_t *scsi_io_cb;
	scsimap_sblk_t *sp;

	_OSDEP_MUTEX_LOCK(&rdata->r_lock);
	if (UDI_QUEUE_EMPTY(&rdata->alloc_q)) {
		_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);
		return;
	}
	scsi_io_cb = scsimap_get_io_cb(rdata);
	if (scsi_io_cb == NULL) {
		_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);
		return;
	}
	sp = UDI_BASE_STRUCT(UDI_DEQUEUE_HEAD(&rdata->alloc_q),
			     scsimap_sblk_t, link);
	_OSDEP_MUTEX_UNLOCK(&rdata->r_lock);

	osdep_scsimap_send_io_req(rdata, scsi_io_cb, sp);
}

STATIC void
osdep_scsimap_send_io_req(scsimap_region_data_t * rdata,
			  udi_scsi_io_cb_t *scsi_io_cb,
			  scsimap_sblk_t * sp)
{
	struct sb *sbp = &sp->sbp->sb;

	UDI_GCB(scsi_io_cb)->initiator_context = sp;

	/*
	 * Copy command and bytes from req->command to cdb 
	 */
	scsi_io_cb->cdb_len = sbp->SCB.sc_cmdsz;
	udi_memcpy(scsi_io_cb->cdb_ptr, sbp->SCB.sc_cmdpt,
		   scsi_io_cb->cdb_len);

	if ((sbp->SCB.sc_time != 0) &&
	    (rdata->hba->hba_flags & SCSIMAP_HBA_TIMEOUT_ON))
		/*
		 * sc_time is in seconds, convert it into milliseconds 
		 */
		scsi_io_cb->timeout = sbp->SCB.sc_time * 1000 +
			rdata->hd_timeout_increase;
	else
		scsi_io_cb->timeout = 0;

	/*
	 * (TODO) set the other flag values
	 * get the correct attribute value
	 */
	scsi_io_cb->attribute = UDI_SCSI_SIMPLE_TASK;

	/*
	 * the osdep code has to implement dma_map operations for this
	 * buffer so it can efficiently map the memory to DMA while still 
	 * satisfying the DMA constraints for this HBA
	 */
	scsi_io_cb->flags = 0;
	if (sbp->SCB.sc_datasz) {
		scsi_io_cb->flags = (sbp->SCB.sc_mode & SCB_READ) ?
			UDI_SCSI_DATA_IN : UDI_SCSI_DATA_OUT;
		_udi_buf_extern_init(osdep_scsimap_scsi_io_req,
				     UDI_GCB(scsi_io_cb),
				     sbp->SCB.sc_datapt, sbp->SCB.sc_datasz,
				     &osdep_scsimap_xbops, NULL,
				     rdata->buf_path);
	} else {
		osdep_scsimap_scsi_io_req(UDI_GCB(scsi_io_cb), NULL);
	}
}

/*
 * This routine is called by the resource allocation code after
 * resources (scsi control block and udi_buf_t) are acquired
 * for the i/o request specified by req.
 * This is also a call back function for udi_buf_t resource allocation.
 */
STATIC void
osdep_scsimap_scsi_io_req(udi_cb_t *gcb,
			  udi_buf_t *new_buf)
{
	udi_scsi_io_cb_t *scsi_io_cb = UDI_MCB(gcb, udi_scsi_io_cb_t);

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("osdep_scsimap_scsi_io_req: gcb=%p, new_buf=%p\n",
		      gcb, new_buf);
#endif
	ASSERT((new_buf == NULL) || (new_buf->buf_size == req->datasize));

	scsi_io_cb->data_buf = new_buf;
	udi_scsi_io_req(scsi_io_cb);
}

/*
 * Currently there is no passthru support.
 */
int
scsimapopen(dev_t * devp,
	    int flags,
	    int otype,
	    cred_t * cred_p)
{
	return 0;
}

/*
 * Currently there is no passthru support.
 */
int
scsimapclose(dev_t dev,
	     int flags,
	     int otype,
	     cred_t * cred_p)
{
	return 0;
}

/*
 * Currently there is no passthru support.
 */
int
scsimapioctl(dev_t dev,
	     int flags,
	     caddr_t arg,
	     int otype,
	     cred_t * cred_p,
	     int *rvalp)
{
	return ENXIO;
}

/* 
 * return the sense data to the OS
 * called in response to a REQUEST_SENSE command
 */
STATIC void
osdep_scsimap_sense(scsimap_req_t * req,
		    udi_ubit8_t *sense)
{
#if 0
	scsimap_sblk_t *sp = req->osdep;
	int len = MIN(req->rdata->sense_len, sp->sbp->sb.SCB.sc_datasz);

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("osdep_scsimap_sense: sp=%lx, sense=%lx\n", sp, sense);
#endif
	bcopy(sense, sp->sbp->sb.SCB.sc_datapt, len);
	sp->sbp->sb.SCB.sc_status = 0;
	sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
	sp->sbp->sb.SCB.sc_resid = sp->sbp->sb.SCB.sc_datasz - len;

	sdi_callback(&sp->sbp->sb);
#else
	ASSERT(0);
#endif
}

/* 
 * return inquiry data to the OS
 * called in response to an INQUIRY command
 */
STATIC void
osdep_scsimap_inquiry(scsimap_req_t * req,
		      udi_ubit8_t *inq)
{
#if 0
	scsimap_sblk_t *sp = req->osdep;
	int len = MIN(SCSIMAP_INQ_LEN, sp->sbp->sb.SCB.sc_datasz);

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("osdep_scsimap_inquiry: sp=%lx, inq=%lx\n", sp, inq);
#endif
	bcopy(inq, sp->sbp->sb.SCB.sc_datapt, len);
	sp->sbp->sb.SCB.sc_status = 0;
	sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
	sp->sbp->sb.SCB.sc_resid = sp->sbp->sb.SCB.sc_datasz - len;

	sdi_callback(&sp->sbp->sb);
#else
	ASSERT(0);
#endif
}

STATIC void
osdep_scsimap_io_ack(scsimap_req_t * req)
{
	ASSERT(0);
}

/* 
 * UDI entry point for scsi_io_ack
 */
STATIC void
osdep_scsimap_scsi_io_ack(udi_scsi_io_cb_t *scsi_io_cb)
{
	scsimap_region_data_t *rdata = UDI_GCB(scsi_io_cb)->context;
	scsimap_sblk_t *sp = UDI_GCB(scsi_io_cb)->initiator_context;
	struct sb *sbp = &sp->sbp->sb;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("osdep_scsimap_scsi_io_ack\n");
#endif
	/*
	 * Invalidate the sense data, it is no longer valid 
	 */
	rdata->r_flags &= ~SCSIMAP_RDATA_SENSE_VALID;

	if (scsi_io_cb->data_buf != NULL && sbp->SCB.sc_datapt) {
		sbp->SCB.sc_resid = sbp->SCB.sc_datasz -
			scsi_io_cb->data_buf->buf_size;
		udi_buf_free(scsi_io_cb->data_buf);
	}

	sbp->SCB.sc_comp_code = SDI_ASW;
	sdi_callback(sbp);

	/*
	 * Send the next request 
	 */
	osdep_scsimap_next_io_req(rdata, scsi_io_cb);
}

/* 
 * UDI entry point for scsi_io_nak
 */
STATIC void
osdep_scsimap_scsi_io_nak(udi_scsi_io_cb_t *scsi_io_cb,
			  udi_status_t req_status,
			  udi_ubit8_t scsi_status,
			  udi_ubit8_t sense_status,
			  udi_buf_t *sense_buf)
{
	scsimap_region_data_t *rdata = UDI_GCB(scsi_io_cb)->context;
	scsimap_sblk_t *sp = UDI_GCB(scsi_io_cb)->initiator_context;
	struct sb *sbp = &sp->sbp->sb;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("osdep_scsimap_scsi_io_nak\n");
#endif

	/*
	 * Sense data is no longer valid. 
	 */
	rdata->r_flags &= ~SCSIMAP_RDATA_SENSE_VALID;

	if (scsi_io_cb->data_buf != NULL) {
		sbp->SCB.sc_resid = sbp->SCB.sc_datasz -
			scsi_io_cb->data_buf->buf_size;
		udi_buf_free(scsi_io_cb->data_buf);
	}

	switch (req_status) {
	case UDI_OK:
		sbp->SCB.sc_comp_code = SDI_ASW;
		break;

	case UDI_SCSI_STAT_NONZERO_STATUS_BYTE:
		sbp->SCB.sc_comp_code = SDI_CKSTAT;
		sbp->SCB.sc_status = scsi_status;

		/*
		 * Check if the scsi status is CHECK CONDITION.
		 */
		if (scsi_status == S_CKCON) {
			/*
			 * Copy sense data to rdata->sense 
			 */
			rdata->sense_len = sense_buf->buf_size;
			udi_buf_read(sense_buf, 0, rdata->sense_len,
				     SENSE_AD(rdata->sense));
			rdata->r_flags |= SCSIMAP_RDATA_SENSE_VALID;

			/*
			 * HBA driver allocates sense_buf and we are supposed
			 * to free the sense_buf. 
			 */
			udi_buf_free(sense_buf);
			/*
			 * Copy sense data from rdata->sense 
			 */
			bcopy((caddr_t) rdata->sense,
			      (caddr_t) sdi_sense_ptr(sbp),
			      MIN(sizeof (struct sense), rdata->sense_len));
		}
		break;

	case UDI_SCSI_STAT_ACA_PENDING:
	case UDI_SCSI_STAT_DEVICE_PHASE_ERROR:
	case UDI_SCSI_STAT_UNEXPECTED_BUS_FREE:
	case UDI_SCSI_STAT_DEVICE_PARITY_ERROR:
		sbp->SCB.sc_comp_code = SDI_RETRY;
		break;
	case UDI_SCSI_STAT_SELECTION_TIMEOUT:
		sbp->SCB.sc_comp_code = SDI_NOSELE;
		break;
	case UDI_SCSI_STAT_ABORTED_HD_BUS_RESET:
	case UDI_SCSI_STAT_ABORTED_RMT_BUS_RESET:
	case UDI_SCSI_STAT_ABORTED_REQ_BUS_RESET:
	case UDI_SCSI_STAT_ABORTED_REQ_TGT_RESET:
		sbp->SCB.sc_comp_code = SDI_RESET;
		break;

	case UDI_SCSI_STAT_LINK_FAILURE:
		sbp->SCB.sc_comp_code = SDI_HAERR;
		break;

	case UDI_STAT_TIMEOUT:
	case UDI_STAT_ABORTED:
		sbp->SCB.sc_comp_code = SDI_TIME;
		break;

	case UDI_STAT_NOT_UNDERSTOOD:
	case UDI_STAT_HW_PROBLEM:
		sbp->SCB.sc_comp_code = SDI_HAERR;
		break;

	case UDI_STAT_DATA_OVERRUN:
	case UDI_STAT_DATA_UNDERRUN:
		sbp->SCB.sc_comp_code = SDI_HAERR;
		break;

	default:
		sbp->SCB.sc_comp_code = SDI_RETRY;
		break;
	}

	sdi_callback(sbp);

	/*
	 * Send the next request 
	 */
	osdep_scsimap_next_io_req(rdata, scsi_io_cb);
}

STATIC void
osdep_scsimap_io_nak(scsimap_req_t * req,
		     udi_status_t status,
		     udi_ubit8_t scsi_status,
		     udi_ubit8_t sense_status)
{
	ASSERT(0);
}

/* DO NOTHING */

/* This memory should not be free'd. This is external memory */
STATIC void
osdep_scsimap_dma_free(void *context,
		       udi_ubit8_t *space,
		       udi_size_t size)
{
}
STATIC void *
osdep_scsimap_dma_map(void *context,
		      udi_ubit8_t *space,
		      udi_size_t size,
		      _udi_dma_handle_t *dmah)
{
}
STATIC void
osdep_scsimap_dma_unmap(void *context,
			udi_ubit8_t *space,
			udi_size_t size,
			_udi_dma_handle_t *dmah)
{
}

STATIC void
osdep_scsimap_func(scsimap_region_data_t * rdata,
		   scsimap_sblk_t * sp)
{
	udi_cb_t *gcb = UDI_GCB(rdata->scsi_bind_cb);
	scsimap_bind_scratch_t *bind_scratch = gcb->scratch;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("osdep_scsimap_func: rdata=%p, sblk=%p\n", rdata, sp);
#endif

	/*
	 * Save the req 
	 */
	bind_scratch->req = sp;

	/*
	 * Allocate a control CB and issue the control command 
	 */
	udi_cb_alloc(osdep_scsimap_send_ctl_req, gcb, SCSI_CTL_CB_IDX,
		     gcb->channel);
}

/*
 * Call back function for allocating a control block for scsi ctl req.
 * Initialize the control block and call udi_scsi_ctl_req ().
 */
STATIC void
osdep_scsimap_send_ctl_req(udi_cb_t *gcb,
			   udi_cb_t *new_cb)
{
	udi_scsi_ctl_cb_t *scsi_ctl_cb = UDI_MCB(new_cb, udi_scsi_ctl_cb_t);
	scsimap_region_data_t *rdata = gcb->context;
	scsimap_sblk_t *sp = gcb->scratch;
	struct sb *sbp = &sp->sbp->sb;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("osdep_scsimap_send_ctl_req: rdata=%lx, sblk=%lx\n",
		      rdata, sp);
#endif
	UDI_GCB(scsi_ctl_cb)->initiator_context = sp;

	ASSERT(sbp->sb_type == SFB_TYPE);

	switch (sbp->SFB.sf_func) {
	case SFB_RESET_DEVICE:
	case SFB_RESETM:
		scsi_ctl_cb->ctrl_func = UDI_SCSI_CTL_LUN_RESET;
		udi_scsi_ctl_req(scsi_ctl_cb);
		break;
	case SFB_ABORTM:
		scsi_ctl_cb->ctrl_func = UDI_SCSI_CTL_ABORT_TASK_SET;
		udi_scsi_ctl_req(scsi_ctl_cb);
		break;

	default:
		ASSERT(0);
	}
}

/*
 * UDI entry point for scsi_ctl_ack
 */
STATIC void
osdep_scsimap_ctl_ack(scsimap_req_t * req,
		      udi_status_t status)
{
	_OSDEP_ASSERT(0);
}

/*
 * UDI entry point for scsi_ctl_ack
 */
STATIC void
osdep_scsimap_scsi_ctl_ack(udi_scsi_ctl_cb_t *scsi_ctl_cb,
			   udi_status_t status)
{
	scsimap_sblk_t *sp = UDI_GCB(scsi_ctl_cb)->initiator_context;
	struct sb *sbp = &sp->sbp->sb;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("osdep_scsimap_ctl_ack - sp=%lx status=%lx\n", sp,
		      status);
#endif

	switch (status) {
	case UDI_OK:
		sbp->SFB.sf_comp_code = SDI_ASW;
		break;

	case UDI_STAT_HW_PROBLEM:
		sbp->SFB.sf_comp_code = SDI_HAERR;
		break;

	case UDI_STAT_NOT_UNDERSTOOD:
		sbp->SFB.sf_comp_code = SDI_SFBERR;
		break;

	case UDI_STAT_NOT_SUPPORTED:
		sbp->SFB.sf_comp_code = SDI_HAERR;
		break;

	case UDI_SCSI_CTL_STAT_FAILED:
		sbp->SFB.sf_comp_code = SDI_RETRY;
		break;

	default:
		sbp->SFB.sf_comp_code = SDI_RETRY;
		break;
	}

	sdi_callback(sbp);

	/*
	 * free the ctl cb 
	 */
	udi_cb_free(UDI_GCB(scsi_ctl_cb));
}

STATIC void
osdep_scsimap_device_add(scsimap_region_data_t * rdata)
{
	_OSDEP_MUTEX_LOCK(&scsimap_reg_lock);
	UDI_ENQUEUE_TAIL(&scsimap_reg_list, &rdata->link);
	_OSDEP_MUTEX_UNLOCK(&scsimap_reg_lock);
	_OSDEP_EVENT_SIGNAL(&scsimap_reg_ev);
}

STATIC int
osdep_scsimap_hba_attach(scsimap_region_data_t * rdata)
{
	udi_queue_t *elem, *tmp;
	int rval = 0;
	uchar_t b;
	struct scsi_adr *sa;
	HBA_IDATA_STRUCT *idp, idata_template;
	char drv_descr[64];
	scsimap_hba_t *hba = rdata->hba;

	udi_snprintf(drv_descr, 64, "(%s,1) %s", hba->hba_drvname,
		     &rdata->inquiry_data[8]);
	bzero(&idata_template, sizeof (idata_template));
	idata_template.name = drv_descr;
	idata_template.version_num = HBA_UW21_IDATA | HBA_EXT_ADDRESS;

	hba->osdep.hba_rmkey = hba->parent_node->node_osinfo.rm_key;
	idp = sdi_idata_alloc(hba->osdep.hba_rmkey, &idata_template);

	udi_assert(idp);

	/*
	 * Store the idata pointer in ha structure. idata structure contains:
	 *      idata->cntlr    SDI assigned controller number 
	 *      idata->name     SDI assigned name
	 * among other things.
	 */
	hba->osdep.hba_idata = idp;

	/*
	 * Set up the number of buses, targets and LUNs supported
	 * before we register this HBA.
	 */

	/*
	 * Setup for sysdump 
	 */
	_udi_sysdump_alloc(hba->parent_node, &idp->idata_intrcookie);

	idp->idata_nbus = hba->max_buses;
	idp->idata_ntargets = hba->max_targets;
	idp->idata_nluns = hba->max_luns;

	for (b = 0; b < idp->idata_nbus; b++)
		idp->ha_chan_id[b] = hba->hba_chan[b].chan_id;
	idp->ha_id = idp->ha_chan_id[0];	/* don't forget to set this */

	scsimap_hba[idp->cntlr] = hba;

	idp->active = 1;

	/*
	 * Register the HBA devices with SDI 
	 */
	if ((rval = sdi_register(&scsimaphba_info, idp)) < 0) {
		cmn_err(CE_WARN, "%s: sdi_register() failed, status=%d",
			idp->name, rval);
		sdi_idata_free(idp);
		scsimap_hba[idp->cntlr] = NULL;
		udi_assert(0);
		/*
		 * return ENOMEM; 
		 */
	}

	_OSDEP_MUTEX_LOCK(&hba->hba_lock);
	hba->hba_flags |= SCSIMAP_HBA_ATTACHED;
	_OSDEP_MUTEX_UNLOCK(&hba->hba_lock);

	return 0;
}

/* 
 * Assign a scsimap_hba structure from the global array of scshimap_hba[]
 * structures if scsimap_hba structure is not already allocated for this HBA
 * instance. The index into the scsimap_hba[] array is our local controller
 * number
 */
STATIC void
osdep_scsimap_device_attach(scsimap_region_data_t * rdata)
{
	udi_ubit8_t found = FALSE;
	scsimap_hba_t *hba = NULL;
	_udi_dev_node_t *parent_node;
	const char *modname;
	udi_queue_t *elem, *tmp;
	udi_ubit32_t maxdevs;
	udi_ubit32_t b, t, l;
	struct scsi_adr sa;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF
		("osdep_scsimap_device_attach: rdata=%p hba=%p dev_type=0x%x addr=(%d,%d,%d)\n",
		 rdata, rdata->hba, SCSIMAP_ID_TYPE(rdata->inquiry_data),
		 rdata->scsi_addr.scsi_bus, rdata->scsi_addr.scsi_target,
		 rdata->scsi_addr.scsi_lun);
#endif

	/*
	 * The following code helps us group the devices attached to
	 * each controller. Devices attached to an HBA will have
	 * the same parent node.
	 */
	parent_node = _UDI_GCB_TO_CHAN(UDI_GCB(rdata->scsi_bind_cb))->
		chan_region->reg_node->parent_node;
	udi_assert(parent_node);

	_OSDEP_MUTEX_LOCK(&scsimap_hba_list_lock);

	UDI_QUEUE_FOREACH(&scsimap_hba_list, elem, tmp) {
		hba = UDI_BASE_STRUCT(elem, scsimap_hba_t, link);
		if (hba->parent_node == parent_node) {
			found = TRUE;
			break;
		}
	}

	_OSDEP_MUTEX_UNLOCK(&scsimap_hba_list_lock);

	if (!found) {
		if (!rdata->multi_lun) {
			_OSDEP_PRINTF
				("WARNING:Multi-lun (pseudo) device must be enumerated first\n");
			/*
			 * HBA must be the first device enumerated on a scsi bus 
			 */
			_OSDEP_ASSERT(SCSIMAP_ID_TYPE(rdata->inquiry_data) ==
				      SCSIMAP_PROCESSOR_TYPE);
			rdata->max_buses = 1;
			rdata->max_targets = 8;
			rdata->max_luns = 8;
		}

		/*
		 * Allocate and initialize an hba structure
		 */

		hba = _OSDEP_MEM_ALLOC(sizeof (scsimap_hba_t), 0, UDI_WAITOK);

		hba->parent_node = parent_node;

		/*
		 * Obtain the module name of the (parent) driver at the
		 * other end of the bind channel.
		 */
		modname = _UDI_GCB_TO_CHAN(UDI_GCB(rdata->scsi_bind_cb))->
			other_end->chan_region->reg_driver->drv_name;
		hba->hba_drvname = _OSDEP_MEM_ALLOC(udi_strlen(modname) + 1,
						    0, UDI_WAITOK);
		udi_strcpy(hba->hba_drvname, modname);

		hba->hba_flags = 0;
		hba->max_buses = rdata->max_buses;
		hba->max_targets = rdata->max_targets;
		hba->max_luns = rdata->max_luns;

		/*
		 * Initialize the device list queue 
		 */
		UDI_QUEUE_INIT(&hba->device_list);

		_OSDEP_MUTEX_INIT(&hba->hba_lock, "scsimap hba lock");

		hba->hba_rdata = rdata;	/* HBA's rdata */

		/*
		 * compute the shift values for targets and buses 
		 */
		for (l = hba->max_luns - 1, hba->tgt_shift = 0; l > 0;
		     l >>= 1, hba->tgt_shift++);
		for (t = hba->max_targets - 1, hba->bus_shift = hba->tgt_shift;
		     t > 0; t >>= 1, hba->bus_shift++);
		/*
		 * Calculate the maximum number of devices supported by this
		 * HBA and allocated  the device queue of pointers for it
		 */
		maxdevs = hba->max_buses << (hba->bus_shift + hba->tgt_shift);

		/*
		 * allocate the device array 
		 */
		hba->hba_devices =
			(scsimap_region_data_t **) _OSDEP_MEM_ALLOC(maxdevs *
								    sizeof
								    (scsimap_region_data_t
								     *), 0,
								    UDI_WAITOK);

		/*
		 * allocate the channel array 
		 */
		hba->hba_chan =
			(scsimap_channel_t *) _OSDEP_MEM_ALLOC(rdata->
							       max_buses *
							       sizeof
							       (scsimap_channel_t),
							       0, UDI_WAITOK);

		_OSDEP_MUTEX_LOCK(&scsimap_hba_list_lock);
		UDI_ENQUEUE_TAIL(&scsimap_hba_list, &hba->link);
		_OSDEP_MUTEX_UNLOCK(&scsimap_hba_list_lock);

		/*
		 * This is the multi-lun (pseudo) device, since there is
		 * nothing * else to be done for this device, complete the
		 * device add operation
		 */
		scsimap_device_add_complete(rdata);
		return;
	}

	/*
	 * Link the devices to the list of other devices attached to this hba
	 */
	_OSDEP_MUTEX_LOCK(&hba->hba_lock);

	UDI_ENQUEUE_TAIL(&hba->device_list, &rdata->device_link);
	hba->hba_num_devices++;
	rdata->hba = hba;

	/*
	 * Set up the device queues.
	 * This is an array of pointers which stores the rdata for each
	 * device. We maintain this because the os code involved is more
	 * likely to pass scsi addresses than rdatas and we want to save
	 * the time involved in searching the rdata list.
	 * Instead, we index the rdatas by (bus, target, lun) addresses.
	 */

	SCSIMAP_GET_RDATA(hba, rdata->scsi_addr.scsi_bus,
			  rdata->scsi_addr.scsi_target,
			  rdata->scsi_addr.scsi_lun) = rdata;

	_OSDEP_MUTEX_UNLOCK(&hba->hba_lock);

	/*
	 * Register the device with the OS. But first check if the
	 * HBA has been registered with the OS. If not, then register
	 * the HBA. Registering the HBA also has a side effect of
	 * registering all the devices enumerated thus far.
	 */

	if (!(rdata->hba->hba_flags & SCSIMAP_HBA_ATTACHED)) {
		/*
		 * Check if this is the HBA 
		 */
		if (SCSIMAP_ID_TYPE(rdata->inquiry_data) ==
		    SCSIMAP_PROCESSOR_TYPE) {
			hba->hba_chan[rdata->scsi_addr.scsi_bus].chan_id =
				rdata->scsi_addr.scsi_target;
			hba->hba_chan[rdata->scsi_addr.scsi_bus].chan_flag =
				SCSIMAP_CHAN_OK;
			/*
			 * Check if all the channels have been enumerated 
			 */
			for (b = 0; b < hba->hba_rdata->max_buses; b++) {
				if (hba->hba_chan[b].chan_flag !=
				    SCSIMAP_CHAN_OK) {
					/*
					 * One of the channels have not been
					 * enumerated yet. We have to wait
					 * until all the channels have been
					 * enumerated before registering this
					 * HBA. Simply, complete the device_add
					 * operation for this device.
					 */
					scsimap_device_add_complete(rdata);
					return;
				}
			}
			/*
			 * register the hba with the OS 
			 */
			osdep_scsimap_hba_attach(rdata);
		}

		scsimap_device_add_complete(rdata);
		return;
	}

	_OSDEP_ASSERT(hba->osdep.hba_idata);

	sa.scsi_ctl = hba->osdep.hba_idata->cntlr;
	sa.scsi_bus = rdata->scsi_addr.scsi_bus;
	sa.scsi_target = rdata->scsi_addr.scsi_target;
	sa.scsi_lun = rdata->scsi_addr.scsi_lun;

	sdi_hot_add(&sa);

	sdi_enable_instance(hba->osdep.hba_idata);

	scsimap_device_add_complete(rdata);
}

/* 
 * Removes a device
 */

STATIC void
osdep_scsimap_device_rm(scsimap_region_data_t * rdata)
{
	scsimap_hba_t *hba;
	HBA_IDATA_STRUCT *idp;
	struct scsi_adr sa;
	int status;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_device_rm\n");
#endif

	if (rdata->multi_lun) {
		/*
		 * If this is a multi-lun PD (pseudo device), simply return 
		 */
		scsimap_device_rm_complete(rdata);
		return;
	}

	hba = rdata->hba;

	/*
	 * Fill in the correct controller number before calling into sdi 
	 */
	sa.scsi_ctl = hba->osdep.hba_idata->cntlr;
	sa.scsi_bus = rdata->scsi_addr.scsi_bus;
	sa.scsi_target = rdata->scsi_addr.scsi_target;
	sa.scsi_lun = rdata->scsi_addr.scsi_lun;
	/*
	 * If this is the controller, we can remove it only when all
	 * the devices have been removed.
	 */
	if (sa.scsi_target != hba->osdep.hba_idata->ha_chan_id[sa.scsi_bus]) {
		/*
		 * It is a device, remove it 
		 */
		if ((status = sdi_rm_dev(&sa, 0)) != SDI_RET_OK) {
			cmn_err(CE_WARN,
				"sdi_rm_dev failed for %d,%d,%d,%d, status=0x%x",
				sa.scsi_ctl, sa.scsi_bus, sa.scsi_target,
				sa.scsi_lun, status);
		}
	}
	rdata->hba->hba_num_devices--;

	rdata->hba = NULL;

	_OSDEP_MUTEX_LOCK(&hba->hba_lock);

	SCSIMAP_GET_RDATA(hba, sa.scsi_bus, sa.scsi_target, sa.scsi_lun) =
		NULL;

	/*
	 * Remove the device from the list 
	 */
	UDI_QUEUE_REMOVE(&rdata->device_link);

	_OSDEP_MUTEX_UNLOCK(&hba->hba_lock);

	/*
	 * If all the devices have been removed, detach the driver from sdi
	 */
	if (!UDI_QUEUE_EMPTY(&hba->device_list)) {
		scsimap_device_rm_complete(rdata);
		return;
	}

	/*
	 * Detach the hba from OS
	 */

	idp = hba->osdep.hba_idata;

	if (sdi_deregister(idp)) {
		/*
		 * sdi_register() failed, let the data structures be around to
		 * avoid system panics.
		 */
		_OSDEP_PRINTF("%s:sdi_register() failed\n", idp->name);

		scsimap_device_rm_complete(rdata);
		return;
	}

	_udi_sysdump_free(&idp->idata_intrcookie);

	scsimap_hba[idp->cntlr] = NULL;
	sdi_idata_free(idp);

	/*
	 * Remove the hba from the global list 
	 */
	_OSDEP_MUTEX_LOCK(&scsimap_hba_list_lock);
	UDI_QUEUE_REMOVE(&hba->link);
	_OSDEP_MUTEX_UNLOCK(&scsimap_hba_list_lock);

	_OSDEP_MUTEX_DEINIT(&hba->hba_lock);

	_OSDEP_MEM_FREE(hba->hba_chan);
	_OSDEP_MEM_FREE(hba->hba_devices);
	_OSDEP_MEM_FREE(hba->hba_drvname);
	_OSDEP_MEM_FREE(hba);

	scsimap_device_rm_complete(rdata);
	return;
}

/*
 * STATIC void
 * osdep_scsimap_reg_daemon(void *arg)
 *
 *	Device registeration daemon
 */

STATIC void
osdep_scsimap_reg_daemon(void *arg)
{
	scsimap_region_data_t *rdata;
	udi_queue_t *elem;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("scsimap_reg_daemon_work\n");
#endif

	ASSERT(getpl() == plbase);

	for (;;) {
		if (scsimap_reg_daemon_kill)
			break;
		_OSDEP_MUTEX_LOCK(&scsimap_reg_lock);
		if (UDI_QUEUE_EMPTY(&scsimap_reg_list)) {
			_OSDEP_EVENT_CLEAR(&scsimap_reg_ev);
			_OSDEP_MUTEX_UNLOCK(&scsimap_reg_lock);
			_OSDEP_EVENT_WAIT(&scsimap_reg_ev);
			continue;	/* Jump back to the for loop to make sure
					 * that we have a real event. */
		}

		/*
		 * Dequeue the device from the list 
		 */
		elem = UDI_DEQUEUE_HEAD(&scsimap_reg_list);
		_OSDEP_MUTEX_UNLOCK(&scsimap_reg_lock);

		rdata = UDI_BASE_STRUCT(elem, scsimap_region_data_t, link);

		osdep_scsimap_device_attach(rdata);
	}

	kthread_exit();
}
