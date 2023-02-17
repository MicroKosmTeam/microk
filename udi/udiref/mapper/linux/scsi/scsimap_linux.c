/* 
 * File: mapper/linux/scsi/scsimap_linux.c
 *
 * Linux SCSI mapper.
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

#include "udi_osdep_opaquedefs.h"
#include <linux/module.h>
#include <linux/blkdev.h>
#include "linuxscsi.h"
#include <linux/proc_fs.h>
#include <linux/module.h>

#include <scsimap_linux.h>

#include "scsicommon/scsimap.c"

#define MAPPER_NAME "udiMscsi"

/* 
 * get the scsimap_hba_t pointer from a Scsi_Host pointer
 */
#define HOST2HBA(sh) \
  (*((scsimap_hba_t**)(&sh->hostdata)))

STATIC kmem_cache_t *scsi_req_cache;

/* 
 * exported by the common code
 */
STATIC int scsimap_send_req(scsimap_req_t *);

extern void __udi_kthread_common_init(char*);

typedef struct hba_template {
    Scsi_Host_Template  hba_template;
    udi_queue_t         link;
} hba_template_t;

STATIC udi_queue_t  hba_template_list;

extern udi_queue_t scsimap_hba_list;
extern _OSDEP_MUTEX_T scsimap_hba_list_lock;

STATIC udi_queue_t  scsimap_reg_list;
STATIC udi_boolean_t scsimap_reg_done = TRUE;
STATIC udi_boolean_t scsimap_reg_daemon_kill = FALSE;
STATIC _OSDEP_MUTEX_T scsimap_reg_lock;
STATIC _OSDEP_MUTEX_T scsimap_reg_done_lock;
STATIC _OSDEP_EVENT_T scsimap_reg_ev;

STATIC _OSDEP_SEM_T param_sem;

STATIC scsimap_hba_t *cur_hba;

STATIC void osdep_scsimap_dma_free(void*, udi_ubit8_t*, udi_size_t);
STATIC void* osdep_scsimap_dma_map(void*, udi_ubit8_t*, udi_size_t, 
				   _udi_dma_handle_t*);
STATIC void osdep_scsimap_dma_unmap(void*, udi_ubit8_t*, udi_size_t, 
				   _udi_dma_handle_t *);

STATIC void osdep_scsimap_hba_attach(scsimap_hba_t * hba);
void osdep_scsimap_device_attach(scsimap_region_data_t* rdata);

/* 
 * convert the SCSI type code into a human-readable format
 * (TODO) use Linux array
 */
#define TYPE(t) (t == 0x00 ? "disk" : \
                (t == 0x01 ? "tape" : \
                (t == 0x02 ? "printer" : \
                (t == 0x03 ? "processor" : \
                (t == 0x04 ? "WORM" : \
                (t == 0x05 ? "CD-ROM" : \
                (t == 0x06 ? "scanner" : \
                (t == 0x07 ? "optical" : \
                (t == 0x08 ? "media changer" : \
                (t == 0x09 ? "communication" : \
                (t == 0x0a || t == 0x0b ? "print preprocessing" : "unknown")))))))))))

/* 
 * get the direction mode for a SCSI command
 * (FIXME) make sure this is right, add missing ones
 */
#define CMD2MODE(c) (c) == WRITE_6 || (c) == WRITE_10 || \
                    (c) == SEND_DIAGNOSTIC || (c) == WRITE_FILEMARKS || \
                    (c) == FORMAT_UNIT || (c) == MODE_SELECT || \
                    (c) == COPY || (c) == SEND_DIAGNOSTIC || \
                    (c) == SET_WINDOW || (c) == SEEK_6 || (c) == SEEK_10 || \
                    (c) == WRITE_VERIFY || (c) == VERIFY || \
                    (c) == SET_LIMITS || (c) == COPY_VERIFY || \
                    (c) == COMPARE || (c) == WRITE_BUFFER || \
                    (c) == UPDATE_BLOCK || (c) == WRITE_LONG || \
                    (c) == CHANGE_DEFINITION || (c) == WRITE_SAME || \
                    (c) == MODE_SELECT_10 || (c) == WRITE_12 || \
                    (c) == WRITE_VERIFY_12 || (c) == WRITE_LONG_2 || \
                    (c) == RESERVE || (c) == RELEASE || \
                    (c) == MEDIUM_SCAN || (c) == 0x51 ? \
                         UDI_SCSI_DATA_OUT : \
		    (c) == TEST_UNIT_READY ? 0 : UDI_SCSI_DATA_IN

/* 
 * OS driver-level functions
 */

/* 
 * notify SCSI mid-layer of completion of async SCSI command
 */
STATIC void
osdep_scsimap_done(Scsi_Cmnd * cmd)
{
#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("osdep_scsimap_done: cmd=%p\n", cmd);
#endif
    if (cmd->scsi_done) {
	cmd->scsi_done(cmd);
    } else {
	_OSDEP_EVENT_SIGNAL((_OSDEP_EVENT_T *) cmd->SCp.ptr);
    }
    return;
}

/* 
 * generate the file for the HBA in the /proc/scsi/<hba> directory
 */
int
osdep_scsimap_proc_info(char *buffer, char **start, off_t offset,
			int length, int hostno, int inout)
{
    udi_queue_t        *elem,
                       *tmp;
    scsimap_hba_t      *hba = NULL; 
    scsimap_region_data_t *rdata;
    udi_ubit8_t         found = FALSE;
    int                 len;

#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("osdep_scsimap_proc_info\n");
#endif
    if (inout)
	return (-ENOSYS);

    _OSDEP_MUTEX_LOCK(&scsimap_hba_list_lock);

    UDI_QUEUE_FOREACH(&scsimap_hba_list, elem, tmp) {
	hba = UDI_BASE_STRUCT(elem, scsimap_hba_t, link);
	if (hba->osdep->host_no == hostno) {
	    found = TRUE;
	    break;
	}
    }

    if (found) {
	len =
	    udi_snprintf(buffer, length,
		    "UDI SCSI driver %s is scsi%d at (%02d, %02d, %02d).\n\n",
		    hba->hba_drvname, hostno, 
		    hba->hba_rdata->scsi_addr.scsi_bus,
		    hba->hba_rdata->scsi_addr.scsi_target, 
		    hba->hba_rdata->scsi_addr.scsi_lun);
	UDI_QUEUE_FOREACH(&hba->device_list, elem, tmp) {
	    rdata = UDI_BASE_STRUCT(elem, scsimap_region_data_t, device_link);
	    len += udi_snprintf(&buffer[len], length - len, 
			   "Channel: %02d Id: %02d Lun: %02d:\n" \
			   "  Vendor: %.8s Model: %.16s Rev: %.4s\n" \
			   "  Type:   %s\n\n",
			   rdata->scsi_addr.scsi_bus,
			   rdata->scsi_addr.scsi_target,
			   rdata->scsi_addr.scsi_lun,
			   SCSIMAP_VENDOR(rdata->inquiry_data),
			   SCSIMAP_PRODUCT(rdata->inquiry_data),
			   SCSIMAP_REVISION(rdata->inquiry_data),
			   TYPE(SCSIMAP_ID_TYPE(rdata->inquiry_data)));
	}

    } else {
	len =
	    udi_snprintf(buffer, length, 
			 "host scsi%d is not serviced by %s.\n", hostno,
		    hba->hba_drvname);
    }

    _OSDEP_MUTEX_UNLOCK(&scsimap_hba_list_lock);

    if (offset > len) {
	*start = buffer;
	return 0;
    }

    *start = buffer + offset;
    len -= offset;

    if (len > length)
	len = length;

    return len;
}

/* 
 * register the host adapter(s) for a given template
 */
int
osdep_scsimap_detect(Scsi_Host_Template * sht)
{
    scsimap_hba_t      *hba;
    struct Scsi_Host   *sh;

#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF
	("osdep_scsimap_detect: sht=%p, hba=%p\n", sht, cur_hba);
#endif

    /* 
     * get the hba from the static variable, it is protected by the param_sem
     */
    hba = cur_hba;

    sh = scsi_register(sht, sizeof(scsimap_hba_t *));

    HOST2HBA(sh) = hba;

    hba->osdep = sh;

    sh->max_id = hba->max_targets;
    sh->max_lun = hba->max_luns;
    sh->max_channel = hba->max_buses;
    sh->this_id = hba->hba_rdata->scsi_addr.scsi_target;
    sh->unique_id = sht->present;

    _OSDEP_SEM_V(&param_sem);

    return sht->present + 1;
}

/* 
 * return the disk geometry for the given disk
 * dummy implementation
 * (FIXME) Do we need to make this work right?
 */
int
osdep_scsimap_biosparam(Disk * disk, kdev_t dev, int geom[])
{
    int                 size = disk->capacity;

#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("osdep_scsimap_biosparam: disk=%p, dev=%d, geom=%p\n",
		  disk, dev, geom);
#endif

    if (geom[2] > 1024) {
	geom[0] = 255;
	geom[1] = 63;
	geom[2] = size / (255 * 63);
    } else {
	geom[0] = 64;
	geom[1] = 32;
	geom[2] = size >> 11;
    }
    return 0;
}

/* 
 * handle a SCSI command (queued)
 */
int
osdep_scsimap_queue_command(Scsi_Cmnd * cmd, void (*done) (Scsi_Cmnd *))
{
    scsimap_req_t      *req;
    scsimap_hba_t      *hba = HOST2HBA(cmd->host);
    udi_ubit8_t         ccmd = *cmd->cmnd;

#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF
	("osdep_scsimap_queue_command: cmd=%p, done=%p, cmd_code=%d, cmd_string=%p, buf=%p, address=(%d,%d,%d)\n",
	 cmd, done, ccmd, cmd->cmnd, cmd->request_buffer, cmd->channel, cmd->target, cmd->lun);
#endif
#ifdef SCSIMAP_EXTRA_DEBUG
   _OSDEP_PRINTF("*cmd= \n");
   {
 	int i;
	for (i=0; i<12; i++) _OSDEP_PRINTF("%02x ", cmd->cmnd[i]);
	_OSDEP_PRINTF("\n\n*data= \n");
	for (i=0; i<cmd->request_bufflen; i++) 
		_OSDEP_PRINTF("%02x ", ((unsigned char*)cmd->request_buffer)[i]);
	_OSDEP_PRINTF("\n\n*data= \n");
    }
#endif

    req = kmem_cache_alloc(scsi_req_cache, 
			   in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);

    req->addr.scsi_target = cmd->target;
    req->addr.scsi_lun = cmd->lun;
    req->addr.scsi_bus = cmd->channel;
    req->hba = hba;
    req->datasize = cmd->request_bufflen;
    req->dataptr = cmd->request_buffer;
    req->residual = 0;
    req->command = cmd->cmnd;
    req->cmdsize = cmd->cmd_len;

    /*
     * The CMD2MODE macro sets the direction of data flow based on the SCSI 
     * command to be sent. Because the Linux SCSI stack associates a data 
     * buffer with all commands (including TEST UNIT READY, which involves
     * no data transfer), we must specifically check for TEST UNIT READY and
     * set the data flow direction flag to 0. Thus, we prevent a buffer from
     * being passed with the scsi_io_cb_t, which fixes a problem with udi_mega.
     */ 
    req->mode = CMD2MODE(ccmd);

    if (!req->mode) {
	req->datasize = 0;
	req->dataptr = NULL;
    }

    req->type = SCSIMAP_IO_TYPE;       /* this is an io request */
    req->timeout = (udi_ubit32_t) (cmd->timeout);	/* (FIXME) is this
							 * correct? */
    req->osdep = cmd;

    /* 
     * find out if we want to call the os-provided done function
     * or if we want to signal an event
     */

    if (done) {
	cmd->scsi_done = done;
	cmd->SCp.ptr = NULL;
    }

    /* send the request */
    if (scsimap_send_req(req) == SCSIMAP_NO_SUCH_DEVICE) {
#if SCSIMAP_DEBUG	
	_OSDEP_PRINTF("no device for (%d,%d,%d)\n", cmd->channel, cmd->target,
		      cmd->lun);
#endif
	/* there is no such device, tell the OS right away */
	cmd->result = DID_NO_CONNECT << 16;

   	kmem_cache_free(scsi_req_cache, req);
	
	osdep_scsimap_done(cmd);
    }
    return 0;
}

/* 
 * handle a SCSI command (non-queued)
 */
int
osdep_scsimap_command(Scsi_Cmnd * cmd)
{
    _OSDEP_EVENT_T      scsimap_cmd_queue_done_ev;

#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("osdep_scsimap_command: cmd=%p\n", cmd);
#endif

    /* 
     * create a new event, queue the command, and wait for the event to
     * fire before returning
     * The event goes into cmd->SCp.ptr because this filed is set aside
     * for drivers by the OS and we are not passing a done function
     */
    _OSDEP_EVENT_INIT(&scsimap_cmd_queue_done_ev);
    _OSDEP_EVENT_CLEAR(&scsimap_cmd_queue_done_ev);
    cmd->SCp.ptr = (char *)&scsimap_cmd_queue_done_ev;
    osdep_scsimap_queue_command(cmd, NULL);
    _OSDEP_EVENT_WAIT(&scsimap_cmd_queue_done_ev);
    _OSDEP_EVENT_DEINIT(&scsimap_cmd_queue_done_ev);
    return cmd->result;
}

/* 
 * send a control request to the SCSI HBA driver
 */
int
osdep_scsimap_send_ctl_req(Scsi_Cmnd * cmd, udi_ubit8_t func)
{
    scsimap_req_t      *req;
    scsimap_hba_t      *hba = HOST2HBA(cmd->host);
    _OSDEP_EVENT_T      scsimap_ctl_req_done_ev;

#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("osdep_scsimap_send_ctl_req: cmd=%p, func=%d\n", cmd, func);
#endif

    req = kmem_cache_alloc(scsi_req_cache, 
			   in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);

    req->osdep = cmd;

    req->addr.scsi_target = cmd->target;
    req->addr.scsi_lun = cmd->lun;
    req->addr.scsi_bus = cmd->channel;
    req->hba = hba;
    req->type = SCSIMAP_CTL_TYPE;
    req->timeout = (udi_ubit32_t) cmd->timeout;	/* (FIXME) is this correct? * */
    req->func = func;

    /* 
     * always use an event for control requests,
     * as they are always synchronous in Linux
     */
    _OSDEP_EVENT_INIT(&scsimap_ctl_req_done_ev);
    _OSDEP_EVENT_CLEAR(&scsimap_ctl_req_done_ev);
    cmd->SCp.ptr = (char *)&scsimap_ctl_req_done_ev;
    cmd->scsi_done = NULL;

    /* 
     * if there is no such device, tell the OS right away
     */
    if (scsimap_send_req(req) == SCSIMAP_NO_SUCH_DEVICE) {
	cmd->result = DID_NO_CONNECT << 16;
    } else {
	_OSDEP_EVENT_WAIT(&scsimap_ctl_req_done_ev);
    }

    kmem_cache_free(scsi_req_cache, req);
    _OSDEP_EVENT_DEINIT(&scsimap_ctl_req_done_ev);
#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("leaving osdep_scsimap_send_ctl_req.\n");
#endif
    return cmd->result;
}

/* 
 * abort the pending SCSI request
 * maps into a UDI_SCSI_CTL_ABORT_TASK_SET control request
 */
int
osdep_scsimap_abort(Scsi_Cmnd * cmd)
{
    return osdep_scsimap_send_ctl_req(cmd, UDI_SCSI_CTL_ABORT_TASK_SET);
}

/* 
 * reset the device
 * maps into a UDI_SCSI_CTL_LUN_RESET control request
 */
int
osdep_scsimap_device_reset(Scsi_Cmnd * cmd)
{
    return osdep_scsimap_send_ctl_req(cmd, UDI_SCSI_CTL_LUN_RESET);
}

/* 
 * reset the SCSI bus
 * maps into a UDI_SCSI_CTL_BUS_RESET control request
 */
int
osdep_scsimap_bus_reset(Scsi_Cmnd * cmd)
{
    return osdep_scsimap_send_ctl_req(cmd, UDI_SCSI_CTL_BUS_RESET);
}

/* 
 * reset the host adapter
 * maps into a UDI_SCSI_CTL_ABORT_TASK_SET control request
 * (FIXME) is this correct?
 */
int
osdep_scsimap_host_reset(Scsi_Cmnd * cmd)
{
    return osdep_scsimap_send_ctl_req(cmd, UDI_SCSI_CTL_BUS_RESET);
}


/* 
 * UDI osdep functions
 */

extern int _udi_osdep_event_debug;

STATIC void
scsimap_reg_daemon_work()
{
    scsimap_region_data_t *rdata;
    udi_queue_t        *elem;

#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("scsimap_reg_daemon_work\n");
#endif

    __udi_kthread_common_init("udi_scsid");
    for (;;) {
#ifdef SCSIMAP_DEBUG
	    _OSDEP_PRINTF("registration daemon running.\n");
	    if (scsimap_reg_daemon_kill)
		_OSDEP_PRINTF("registration daemon dying.\n");
#endif
	    if (scsimap_reg_daemon_kill)
	        return;
	    _OSDEP_MUTEX_LOCK(&scsimap_reg_lock);
	    if (UDI_QUEUE_EMPTY(&scsimap_reg_list)) {
	        _OSDEP_MUTEX_UNLOCK(&scsimap_reg_lock);
#ifdef SCSIMAP_DEBUG
		_OSDEP_PRINTF("registration daemon waiting...\n");
#endif
	        _OSDEP_EVENT_WAIT(&scsimap_reg_ev);
	    continue; /* Jump back to the for loop to make sure
			 that we have a real event. */
	    }

	    /* Dequeue the device from the list */
	    elem = UDI_DEQUEUE_HEAD(&scsimap_reg_list);
	    _OSDEP_MUTEX_UNLOCK(&scsimap_reg_lock);

	    rdata = UDI_BASE_STRUCT(elem, scsimap_region_data_t, link);

	    osdep_scsimap_device_attach(rdata);
    }
}

/*
 * initialize static data
 */
STATIC void 
osdep_scsimap_init()
{
#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("osdep_scsimap_init\n");
#endif
}

/*
 * deinitialize static data
 */
STATIC void
osdep_scsimap_deinit()
{
#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("osdep_scsimap_deinit\n");
#endif
}

/* 
 * detach the HBA
 */
STATIC udi_boolean_t 
osdep_scsimap_hba_detach(scsimap_hba_t * hba)
{
    Scsi_Host_Template *sht;
    hba_template_t     *ht = NULL; 
    udi_queue_t        *elem,
                       *tmp;
    udi_boolean_t      found = FALSE;

#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("osdep_scsimap_hba_detach: hba=%p\n", hba);
#endif

    /* get the template for this hba */
    sht = hba->osdep->hostt;
    
    /* remove this instance */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
    proc_scsi_unregister(sht->proc_dir, hba->osdep->host_no + PROC_SCSI_FILE);
#endif
    scsi_unregister(hba->osdep);
    sht->present--;
        
    /* decrement module usage count */
    MOD_DEC_USE_COUNT;

    /*
     * only remove the associated template if there are no more devices 
     * attached
     */
    if (sht->present == 0) {
	scsi_unregister_module(MODULE_SCSI_HA, sht);
	UDI_QUEUE_FOREACH(&hba_template_list, elem, tmp) {
	    ht = UDI_BASE_STRUCT(elem, hba_template_t, link);
	    if (&ht->hba_template == sht) {
		found = TRUE;
		break;
	    }
	}

	if (found) {
	    UDI_QUEUE_REMOVE(elem);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
	    _OSDEP_MEM_FREE(sht->proc_dir);
#endif
	    _OSDEP_MEM_FREE(ht);
	} else {
	    _OSDEP_PRINTF("sht %p not found!\n", sht);
	}

    }

    return TRUE;
}

/* 
 * attach an HBA
 */
extern int _udi_osdep_event_debug;

STATIC void
osdep_scsimap_hba_attach(scsimap_hba_t * hba)
{
    udi_queue_t        *elem,
                       *tmp;
    Scsi_Host_Template *sht;
    int                 found = FALSE;
    hba_template_t     *ht = NULL; 

#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("osdep_scsimap_hba_attach: hba=%p, rdata=%p\n", hba,
		  hba->hba_rdata);

    _udi_osdep_event_debug = 1;
#endif
    /* increment module usage count */
    MOD_INC_USE_COUNT;

    /* 
     * This semaphore protects the static variable cur_hba, which is
     * needed to communicate the hba parameter to osdep_scsimap_detect and
     * there is no way to pass it through the Scsi_Host_Template structure.
     * Note that it is safe to do that here because we are always called from
     * the scsi daemon.
     */
    _OSDEP_SEM_P(&param_sem);

    /* 
     * check if we have already registered an HBA of this type
     * "type" here stands for the driver used to drive this HBA
     */
    UDI_QUEUE_FOREACH(&hba_template_list, elem, tmp) {
	ht = UDI_BASE_STRUCT(elem, hba_template_t, link);
	if (ht->hba_template.name == hba->hba_drvname) {
	    found = TRUE;
	    break;
	}
    }
    /* 
     * if we didn't find such a template, we have to create a new one
     */
    if (!found) {
	ht = _OSDEP_MEM_ALLOC(sizeof(hba_template_t), 0, in_interrupt() ?
							 UDI_NOWAIT : 
							 UDI_WAITOK);

	_OSDEP_ASSERT(ht);

	sht = &ht->hba_template;
	sht->module = _UDI_GCB_TO_CHAN(UDI_GCB(hba->hba_rdata->scsi_bind_cb))->
			    other_end->chan_region->reg_driver->drv_osinfo.mod;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
	sht->proc_dir = _OSDEP_MEM_ALLOC(sizeof(struct proc_dir_entry), 0, 
					 in_interrupt() ? UDI_NOWAIT : 
					 UDI_WAITOK);

	_OSDEP_ASSERT(sht->proc_dir);
	sht->proc_dir->low_ino = PROC_SCSI_SCSI;
	sht->proc_dir->namelen = strlen(hba->hba_drvname);
	sht->proc_dir->name = hba->hba_drvname;
	sht->proc_dir->mode = S_IFDIR | S_IRUGO | S_IXUGO;	/* (FIXME)
								 * what's this? 
								 */
	sht->proc_dir->nlink = 2;      /* (FIXME) what's this? */
	sht->proc_dir->name = hba->hba_drvname;
#else
	sht->proc_name = hba->hba_drvname;
#endif
	sht->proc_info = osdep_scsimap_proc_info;
	sht->name = hba->hba_drvname;
	sht->detect = osdep_scsimap_detect;
	sht->command = osdep_scsimap_command;
	sht->queuecommand = osdep_scsimap_queue_command;
	sht->bios_param = osdep_scsimap_biosparam;
	sht->eh_abort_handler = osdep_scsimap_abort;
	sht->eh_device_reset_handler = osdep_scsimap_device_reset;
	sht->eh_bus_reset_handler = osdep_scsimap_bus_reset;
	sht->eh_host_reset_handler = osdep_scsimap_host_reset;
	sht->can_queue = OSDEP_SCSIMAP_MAX_CMD_Q;
	sht->this_id = -1;
	sht->sg_tablesize = 0;	       /* (FIXME) make scatter/gather work */
	sht->cmd_per_lun = OSDEP_SCSIMAP_MAX_CMD_PER_LUN;	/* (FIXME)
								 * choose
								 * better value 
								 * here */
	sht->use_new_eh_code = 1;

#ifdef SCSIMAP_DEBUG
	_OSDEP_PRINTF("sht->module=%p, modname=%s, count=%d\n", 
		     sht->module, sht->module ? sht->module->name : "(NULL)", 
		     sht->module ? atomic_read(&sht->module->uc.usecount) : 
		     0);
#endif
	UDI_ENQUEUE_HEAD(&hba_template_list, &ht->link);

	cur_hba = hba;
	scsi_register_module(MODULE_SCSI_HA, sht);

	/* 
	 * the detect function will take over from here and set up the HBA
	 * with the OS
	 */
    } else {

	/* 
	 * we already have such a device, have osdep_scsimap_detect
	 * add the new one
	 */
	cur_hba = hba;
	ht->hba_template.present = osdep_scsimap_detect(&ht->hba_template);
    }

    _OSDEP_MUTEX_LOCK(&hba->hba_lock);
    hba->hba_flags |= SCSIMAP_HBA_ATTACHED;
    hba->hba_flags &= ~SCSIMAP_HBA_ATTACH_PENDING;
    _OSDEP_MUTEX_UNLOCK(&hba->hba_lock);

#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("leaving osdep_scsimap_hba_attach\n");
    _udi_osdep_event_debug = 0;
#endif
    return;
}

/* 
 * communicate the successful completion of a SCSI command to the OS
 */
STATIC void
osdep_scsimap_io_ack(scsimap_req_t * req)
{
    Scsi_Cmnd          *cmd = req->osdep;

#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("osdep_scsimap_io_ack: req=%p\n", req);
#endif
#ifdef SCSIMAP_EXTRA_DEBUG
   _OSDEP_PRINTF("*cmd= \n");
   {
 	int i;
	for (i=0; i<12; i++) _OSDEP_PRINTF("%02x ", cmd->cmnd[i]);
	_OSDEP_PRINTF("\n\n*data= \n");
	for (i=0; i<cmd->request_bufflen; i++) 
		_OSDEP_PRINTF("%02x ", ((unsigned char*)cmd->request_buffer)[i]);
	_OSDEP_PRINTF("\n\n*data= \n");
    }
#endif

    /* just in case, copy the sense data if it is valid */
    if (req->rdata && (req->rdata->r_flags & SCSIMAP_RDATA_SENSE_VALID)) {
	udi_memcpy(cmd->sense_buffer, req->rdata->sense,
		   MIN(sizeof(cmd->sense_buffer), req->rdata->sense_len));
    }

    /* successful operation, no scsi status */
    cmd->result = DID_OK << 16;

    /* either call the OS-provided done function, or signal the event */
    kmem_cache_free(scsi_req_cache, req);
    
    osdep_scsimap_done(cmd);

}

/* 
 * communicate an error to the OS
 */
STATIC void
osdep_scsimap_io_nak(scsimap_req_t * req, udi_status_t req_status,
		     udi_ubit8_t scsi_status, udi_ubit8_t sense_status)
{
    Scsi_Cmnd          *cmd = req->osdep;
    udi_ubit8_t         status;

#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF
	("osdep_scsimap_io_nak: req=%p, req_status=%d, scsi_status=%d, sense_status=%d, rdata=%p\n",
	 req, req_status, scsi_status, sense_status, req->rdata);
#endif

#ifdef _SCSIMAP_DEBUG
	if (req->rdata->r_flags & SCSIMAP_RDATA_SENSE_VALID)
	{
		int i;
		_OSDEP_PRINTF("sense data:\n");
		for (i=0; i<req->rdata->sense_len; i++)
			_OSDEP_PRINTF("%02x ", req->rdata->sense[i]);
		_OSDEP_PRINTF("\n");
	}
	{
		int i;
		_OSDEP_PRINTF("original command:\n");
		for (i=0; i<req->osdep->cmd_len; i++)
			_OSDEP_PRINTF("%02x ", req->osdep->cmnd[i]);
		_OSDEP_PRINTF("\n");
	}
#endif

    /* translate error codes */
    switch (req_status) {
	case UDI_SCSI_STAT_ABORTED_HD_BUS_RESET:
	case UDI_SCSI_STAT_ABORTED_RMT_BUS_RESET:
	case UDI_SCSI_STAT_ABORTED_REQ_BUS_RESET:
	case UDI_SCSI_STAT_ABORTED_REQ_TGT_RESET:
	    status = DID_RESET;
	    break;

	case UDI_STAT_ABORTED:
	    status = DID_ABORT;
	    break;

	case UDI_STAT_TIMEOUT:
	    status = DID_TIME_OUT;
	    break;

	case UDI_SCSI_STAT_SELECTION_TIMEOUT:
	    status = DID_NO_CONNECT;
	    break;

	case UDI_STAT_NOT_UNDERSTOOD:
	    status = DID_BAD_TARGET;

	case UDI_SCSI_STAT_DEVICE_PARITY_ERROR:
	    status = DID_PARITY;
	    break;

	case UDI_SCSI_STAT_ACA_PENDING:
	case UDI_SCSI_STAT_DEVICE_PHASE_ERROR:
	case UDI_SCSI_STAT_UNEXPECTED_BUS_FREE:
	    status = DID_SOFT_ERROR;
	    break;

	case UDI_SCSI_STAT_NONZERO_STATUS_BYTE:
	    status = DID_PASSTHROUGH;
	    break;

	case UDI_STAT_DATA_OVERRUN:
	case UDI_STAT_DATA_UNDERRUN:
	case UDI_STAT_HW_PROBLEM:
	case UDI_SCSI_STAT_LINK_FAILURE:
	    status = DID_ERROR;
	    break;

	case UDI_OK:
	    status = DID_OK;
	    break;

	default:
	    status = DID_SOFT_ERROR;
	    break;
    }
    /* mix in the acutal scsi status */
    cmd->result = status << 16 | scsi_status;

    /* also return the sense data, just in case */
    if (req->rdata && (req->rdata->r_flags & SCSIMAP_RDATA_SENSE_VALID)) {
	udi_memcpy(cmd->sense_buffer, req->rdata->sense,
		   MIN(sizeof(cmd->sense_buffer), req->rdata->sense_len));
    }
    kmem_cache_free(scsi_req_cache, req);

    osdep_scsimap_done(cmd);
}

/* 
 * communicate completion of a SCSI control request to the OS
 */
STATIC void
osdep_scsimap_ctl_ack(scsimap_req_t * req, udi_status_t status)
{
    Scsi_Cmnd          *cmd = req->osdep;

#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("osdep_scsimap_ctl_ack: req=%p, status=%d\n", req, status);
#endif

    /* translate completion status codes */
    switch (status) {
	case UDI_OK:
	    cmd->result = DID_OK << 16;
	    break;

	case UDI_STAT_HW_PROBLEM:
	    cmd->result = DID_SOFT_ERROR << 16;
	    break;

	case UDI_STAT_NOT_UNDERSTOOD:
	case UDI_STAT_NOT_SUPPORTED:
	    cmd->result = DID_BAD_TARGET;
	    break;

	case UDI_SCSI_CTL_STAT_FAILED:
	    cmd->result = DID_SOFT_ERROR;
	    break;

	default:
	    cmd->result = DID_SOFT_ERROR;
	    break;
    }

    /* copy the sense data, just in case */
    if (req->rdata && (req->rdata->r_flags & SCSIMAP_RDATA_SENSE_VALID)) {
	udi_memcpy(cmd->sense_buffer, req->rdata->sense,
		   MIN(sizeof(cmd->sense_buffer), req->rdata->sense_len));
    }

    osdep_scsimap_done(cmd);
}

/* 
 * return the sense data to the OS
 * called in response to a REQUEST_SENSE command
 */
STATIC void
osdep_scsimap_sense(scsimap_req_t * req, udi_ubit8_t * sense)
{
    Scsi_Cmnd          *cmd = req->osdep;

#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("osdep_scsimap_sense: req=%p, sense=%p\n", cmd, sense);
#endif

    udi_memcpy(cmd->sense_buffer, sense,
	       MIN(sizeof(cmd->sense_buffer), req->rdata->sense_len));
    cmd->result = DID_OK << 16;

    kmem_cache_free(scsi_req_cache, req);

    osdep_scsimap_done(cmd);
}

/* 
 * return inquiry data to the OS
 * called in response to an INQUIRY command
 */
STATIC void
osdep_scsimap_inquiry(scsimap_req_t * req, udi_ubit8_t * inq)
{
    Scsi_Cmnd          *cmd = req->osdep;

#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("osdep_scsimap_inquiry: req=%p, inq=%p\n", req, inq);
#endif

    udi_memcpy(cmd->request_buffer, inq,
	       MIN(SCSIMAP_INQ_LEN, cmd->request_bufflen));

    cmd->result = DID_OK << 16;

    kmem_cache_free(scsi_req_cache, req);

    osdep_scsimap_done(cmd);
}

void
osdep_scsimap_device_add(scsimap_region_data_t *rdata)
{
#ifdef SCSIMAP_DEBUG
    	_OSDEP_PRINTF("osdep_scsimap_device_add: rdata=%p (%d,%d,%d)\n", 
		      rdata, rdata->scsi_addr.scsi_bus, 
		      rdata->scsi_addr.scsi_target, rdata->scsi_addr.scsi_lun);
#endif
	_OSDEP_MUTEX_LOCK(&scsimap_reg_lock);
	UDI_ENQUEUE_TAIL(&scsimap_reg_list, &rdata->link);
	_OSDEP_MUTEX_UNLOCK(&scsimap_reg_lock);
	_OSDEP_EVENT_SIGNAL(&scsimap_reg_ev);
#ifdef SCSIMAP_DEBUG
    	_OSDEP_PRINTF("leaving osdep_scsimap_device_add\n");
#endif
}

void 
osdep_scsimap_device_attach(scsimap_region_data_t* rdata)
{
	udi_boolean_t found = FALSE;
	scsimap_hba_t *hba = NULL;
	_udi_dev_node_t *parent_node = NULL;
	const char *modname;
	udi_queue_t *elem, *tmp;
	udi_ubit32_t maxdevs, b = 0, t = 0, l = 0;
    
#ifdef SCSIMAP_DEBUG
    	_OSDEP_PRINTF("osdep_scsimap_device_attach: rdata=%p\n", rdata);
#endif

	/* Paranoid sanity check */
	_OSDEP_ASSERT(!in_interrupt());

	/*
	 * The following code helps us group devices to their respective
	 * hbas
	 */
	parent_node = _UDI_GCB_TO_CHAN(UDI_GCB(rdata->scsi_bind_cb))->
			chan_region->reg_node->parent_node;

	_OSDEP_ASSERT(parent_node);

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
			/*
		 	 * Multi-Lun HBA must be enumerated first
			 */
			_OSDEP_PRINTF("WARNING: Multi-lun (pseudo-)device "
				      "must be enumerated first!\n");
			_OSDEP_ASSERT(SCSIMAP_ID_TYPE(rdata->inquiry_data) ==
				SCSIMAP_PROCESSOR_TYPE);
			rdata->max_luns = rdata->max_targets = 8;
			rdata->max_buses = 1;
		}

		/*	
		 * Allocate and initialize hba structure 
		 */
		hba = _OSDEP_MEM_ALLOC(sizeof(*hba), 0, UDI_WAITOK);

		hba->parent_node = parent_node;
		
		/*
	         * obtain the module name of the hba driver
		 * this happens to be the driver name of the driver at the
	         * other end of this rdata's bind channel
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
		 * Initialize hba's device list and lock
		 */
		UDI_QUEUE_INIT(&hba->device_list);
		_OSDEP_MUTEX_INIT(&hba->hba_lock, "hba device lock");

		rdata->hba = hba;

		/*
		 * compute the bitshift values to be used to calculate the
		 * offset into the device/channel array when accessing an
		 * rdata for a given hba
		 */
		for (l = hba->max_luns - 1, hba->tgt_shift = 0; l > 0;
		     l >>= 1, hba->tgt_shift++);

		for (t = hba->max_targets - 1, hba->bus_shift = hba->tgt_shift;
		     t > 0; t >>= 1, hba->bus_shift++);
		
		/*
	 	 * Calculate the theoretically maximum number of devices
		 * supported by this hba
		 */
		maxdevs = hba->max_buses << (hba->bus_shift + hba->tgt_shift);

		/*
		 * allocate the device array
		 */
		hba->hba_devices = _OSDEP_MEM_ALLOC(maxdevs * 
					sizeof(scsimap_region_data_t *),
					0, UDI_WAITOK);

		/*
		 * allocate the channel array
		 */
		hba->hba_chan = _OSDEP_MEM_ALLOC(hba->max_buses * 
					sizeof(scsimap_channel_t), 
					0, UDI_WAITOK);

		_OSDEP_MUTEX_LOCK(&scsimap_hba_list_lock);
		UDI_ENQUEUE_TAIL(&scsimap_hba_list, &hba->link);
		_OSDEP_MUTEX_UNLOCK(&scsimap_hba_list_lock);
	
		/*
		 * We're done adding the hba
		 * There's nothing more to do here, go back to common scsi
		 * mapper and have it do whatever else it needs to do.
		 */
		scsimap_device_add_complete(rdata);
		return;
	}

	/*
	 * Link the device to the list of other devices attached to the hba
	 */
	_OSDEP_MUTEX_LOCK(&hba->hba_lock);
	UDI_ENQUEUE_TAIL(&hba->device_list, &rdata->device_link);
	hba->hba_num_devices++;
	rdata->hba = hba;

	SCSIMAP_GET_RDATA(hba, rdata->scsi_addr.scsi_bus, 
			  rdata->scsi_addr.scsi_target, 
			  rdata->scsi_addr.scsi_lun) = rdata;
	_OSDEP_MUTEX_UNLOCK(&hba->hba_lock);

	if (!(hba->hba_flags & SCSIMAP_HBA_ATTACHED)) {
		if (SCSIMAP_ID_TYPE(rdata->inquiry_data) ==
			SCSIMAP_PROCESSOR_TYPE) {
			hba->hba_rdata = rdata;
			hba->hba_chan[rdata->scsi_addr.scsi_bus].chan_id =
                                                rdata->scsi_addr.scsi_target;
                        hba->hba_chan[rdata->scsi_addr.scsi_bus].chan_flag =
                                                SCSIMAP_CHAN_OK;
                        /* Check if all the channels have been enumerated */
                        for (b = 0; b < hba->hba_rdata->max_buses; b++) {
                                if (hba->hba_chan[b].chan_flag !=
                                                        SCSIMAP_CHAN_OK) {
                                        /* One of the channels has not been
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
			osdep_scsimap_hba_attach(rdata->hba);
		}
		
		scsimap_device_add_complete(rdata);
		return;
	} 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
	{
		struct proc_dir_entry *dp;
		extern struct proc_dir_entry *proc_scsi;
		for (dp = proc_scsi->subdir; dp; dp = dp->next) {
			if (memcmp(dp->name, "scsi", 4) == 0) {
				char buf[80];
				udi_snprintf(buf, sizeof(buf), 
					     "scsi add-single-device "
					     "%d %d %d %d", 
			                     rdata->hba->osdep->host_no,
				    	     rdata->scsi_addr.scsi_bus,
		                             rdata->scsi_addr.scsi_target, 
					     rdata->scsi_addr.scsi_lun);
		
				dp->write_proc(NULL, buf, 80, NULL);
		}
	}
    }
#else
    if (dispatch_scsi_info_ptr) {
	char buf[80];

	udi_snprintf(buf, sizeof(buf), "scsi add-single-device %d %d %d %d",
		    rdata->hba->osdep->host_no, rdata->scsi_addr.scsi_bus,
		    rdata->scsi_addr.scsi_target, rdata->scsi_addr.scsi_lun);
	dispatch_scsi_info_ptr(PROC_SCSI_SCSI, buf, NULL, 0, udi_strlen(buf), 
			       1);
    }
#endif
    scsimap_device_add_complete(rdata);

}

void
osdep_scsimap_device_rm(scsimap_region_data_t *rdata)
{
#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("osdep_scsimap_device_rm: rdata=%p\n", rdata);
#endif
    if (rdata->multi_lun) {
	scsimap_device_rm_complete(rdata);
	return;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
	{
		struct proc_dir_entry *dp;
		extern struct proc_dir_entry *proc_scsi;
		for (dp = proc_scsi->subdir; dp; dp = dp->next) {
			if (memcmp(dp->name, "scsi", 4) == 0) {
				char buf[80];
				udi_snprintf(buf, sizeof(buf), "scsi "
					     "remove-single-device %d %d %d %d",
			                     rdata->hba->osdep->host_no,
				    	     rdata->scsi_addr.scsi_bus,
		                             rdata->scsi_addr.scsi_target, 
					     rdata->scsi_addr.scsi_lun);
				dp->write_proc(NULL, buf, 80, NULL);
		}
	}
    }
#else
    if (dispatch_scsi_info_ptr) {
	char buf[80];

	udi_snprintf(buf, sizeof(buf), "scsi remove-single-device %d %d %d %d",
		    rdata->hba->osdep->host_no, rdata->scsi_addr.scsi_bus,
		    rdata->scsi_addr.scsi_target, rdata->scsi_addr.scsi_lun);
	dispatch_scsi_info_ptr(PROC_SCSI_SCSI, buf, NULL, 0, udi_strlen(buf), 
			       1);
    }
#endif

    rdata->hba->hba_num_devices--;
    _OSDEP_MUTEX_LOCK(&rdata->hba->hba_lock);
    SCSIMAP_GET_RDATA(rdata->hba, rdata->scsi_addr.scsi_bus, 
                      rdata->scsi_addr.scsi_target, 
		      rdata->scsi_addr.scsi_lun) = NULL;
    _OSDEP_MUTEX_UNLOCK(&rdata->hba->hba_lock);
    UDI_QUEUE_REMOVE(&rdata->device_link);
   
  	/*
         * If all the devices have been removed, detach the driver from the OS 
         */
        if (!UDI_QUEUE_EMPTY(&rdata->hba->device_list)) {
                scsimap_device_rm_complete(rdata);
                return;
        }

        /*
         * Detach the hba from OS
         */
	osdep_scsimap_hba_detach(rdata->hba);

         /* Remove the hba from the global list */
        _OSDEP_MUTEX_LOCK(&scsimap_hba_list_lock);
        UDI_QUEUE_REMOVE(&rdata->hba->link);
        _OSDEP_MUTEX_UNLOCK(&scsimap_hba_list_lock);

        _OSDEP_MUTEX_DEINIT(&rdata->hba->hba_lock);

        _OSDEP_MEM_FREE(rdata->hba->hba_chan);
        _OSDEP_MEM_FREE(rdata->hba->hba_devices);
        _OSDEP_MEM_FREE(rdata->hba->hba_drvname);
        _OSDEP_MEM_FREE(rdata->hba);

        scsimap_device_rm_complete(rdata);
	return; 
}

STATIC void
osdep_scsimap_dma_free(void* context, udi_ubit8_t* space, udi_size_t size)
{
    /*
     * prevent "unused parameters" compiler warning
     */
    context = context;
    space = space;
    size = size;
    /*
     * do nothing
     * the memory we mapped was provided by the OS
     */
}
#ifdef _UDI_PHYSIO_SUPPORTED
extern udi_boolean_t
_osdep_dma_build_scgth(_udi_dma_handle_t* dmah, void* mem, udi_size_t size,
		       int flags);

STATIC void*
osdep_scsimap_dma_map(void* context, udi_ubit8_t* space, udi_size_t size, _udi_dma_handle_t* dmah)
{
    context = context;
    size = size;
    dmah = dmah;
    
#ifdef SCSIMAP_DEBUG
    _OSDEP_PRINTF("osdep_scsimap_dma_map: context=%p space=%p size=%ld dmah=%p\n",
		  context, space, size, dmah);
#endif
    /*
     * For now, rely upon the OS providing dma-able memory and do nothing
     * but create the scatter/gather list here
     * later, we will want to use Linux's scatter/gather capabilities to
     * advantage here by converting Linux's own scatter/gather format into
     * UDI's
     */
     return _osdep_dma_build_scgth(dmah, (void*)space, size, 0) ? (void*)space :
                                   NULL;
}

STATIC void
osdep_scsimap_dma_unmap(void* context, udi_ubit8_t* space, udi_size_t size, 
			_udi_dma_handle_t *dmah)
{
    (void)context;
    (void)space;
    (void)size;
    /*
     * Do nothing
     * the scatter/gather lists will be freed by the common env code
     */
    return;
}
#endif
STATIC void 
mapper_init()
{
#ifdef SCSIMAP_DEBUG
   _OSDEP_PRINTF("mapper_init: scsi\n");
#endif
    scsi_req_cache = kmem_cache_create("scsi_req_cache", sizeof(scsimap_req_t),
				       0, 0, NULL, NULL);
    UDI_QUEUE_INIT(&scsimap_reg_list);
    _OSDEP_EVENT_INIT(&scsimap_reg_ev);
    _OSDEP_MUTEX_INIT(&scsimap_reg_lock, "scsimap registration daemon lock");
    UDI_QUEUE_INIT(&hba_template_list);
    _OSDEP_SEM_INIT(&param_sem, 1);

    osdep_scsimap_reg_daemon_init();
}

STATIC void
mapper_deinit()
{
#ifdef SCSIMAP_DEBUG
   _OSDEP_PRINTF("mapper_deinit: scsi\n");
#endif
    kmem_cache_destroy(scsi_req_cache);
    scsimap_reg_daemon_kill = TRUE;
    _OSDEP_EVENT_SIGNAL(&scsimap_reg_ev);
    _OSDEP_EVENT_DEINIT(&scsimap_reg_ev);
    _OSDEP_MUTEX_DEINIT(&scsimap_reg_done_lock);
    _OSDEP_MUTEX_DEINIT(&scsimap_reg_lock);

    _OSDEP_SEM_DEINIT(&param_sem);
}
