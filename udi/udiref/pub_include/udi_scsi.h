/*
 * File: udi_scsi.h
 *
 * UDI SCSI Metalanguage Public Definitions
 *
 * All UDI SCSI HD and SCSI PD drivers must #include this file.
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

#ifndef _UDI_SCSI_H
#define _UDI_SCSI_H
/*
 * Validate UDI_SCSI_VERSION.
 */
#if !defined(UDI_SCSI_VERSION)
#error "UDI_SCSI_VERSION must be defined."
#elif (UDI_SCSI_VERSION != 0x101)
#error "Unsupported UDI_SCSI_VERSION."
#endif

/*
 * Validate #include order.
 */
#ifndef _UDI_H
#error "udi.h must be #included before udi_scsi.h."
#endif

/*
 * Control Block Group Numbers
 */
#define UDI_SCSI_BIND_CB_NUM		1
#define UDI_SCSI_IO_CB_NUM		2
#define UDI_SCSI_CTL_CB_NUM		3
#define UDI_SCSI_EVENT_CB_NUM		4


/*
 * Control block definitions.
 */

/* 
 * udi_scsi_bind_cb_t - Control block for SCSI bind operations 
 */
typedef struct {
	udi_cb_t	gcb;
	udi_ubit16_t	events;
} udi_scsi_bind_cb_t;

/* SCSI Events */
#define UDI_SCSI_EVENT_AEN				(1U<<0)
#define UDI_SCSI_EVENT_TGT_RESET			(1U<<1)
#define UDI_SCSI_EVENT_BUS_RESET			(1U<<2)
#define UDI_SCSI_EVENT_UNSOLICITED_RESELECT		(1U<<3)


/*
 * udi_scsi_io_cb_t - Control block for SCSI I/O Operations
 */
typedef struct {
	udi_cb_t	gcb;
	udi_buf_t	*data_buf;
	udi_ubit32_t	timeout;
	udi_ubit16_t	flags;
	udi_ubit8_t	attribute;
	udi_ubit8_t	cdb_len;
	udi_ubit8_t	*cdb_ptr;  
} udi_scsi_io_cb_t;

/* I/O Request flags */
#define UDI_SCSI_DATA_IN		(1U<<0)
#define UDI_SCSI_DATA_OUT		(1U<<1)
#define UDI_SCSI_NO_DISCONNECT		(1U<<2)

/* SCSI Task Attributes */
#define UDI_SCSI_SIMPLE_TASK		1
#define UDI_SCSI_ORDERED_TASK		2
#define UDI_SCSI_HEAD_OF_Q_TASK		3
#define UDI_SCSI_ACA_TASK		4
#define UDI_SCSI_UNTAGGED_TASK		5

/* 
 * udi_scsi_ctl_cb_t - Control block for SCSI control operations
 */
typedef struct {
	udi_cb_t	gcb;
	udi_ubit8_t	ctrl_func;
	udi_ubit16_t	queue_depth;
} udi_scsi_ctl_cb_t;

/* ctrl_func codes */
#define UDI_SCSI_CTL_ABORT_TASK_SET	1
#define UDI_SCSI_CTL_CLEAR_TASK_SET	2
#define UDI_SCSI_CTL_LUN_RESET		3
#define UDI_SCSI_CTL_TGT_RESET		4
#define UDI_SCSI_CTL_BUS_RESET		5
#define UDI_SCSI_CTL_CLEAR_ACA		6
#define UDI_SCSI_CTL_SET_QUEUE_DEPTH	7

/*
 * udi_scsi_event_cb_t - Control block for SCSI event operations
 */
typedef struct {
	udi_cb_t	gcb;
	udi_ubit8_t	event;
	udi_buf_t	*aen_data_buf;
} udi_scsi_event_cb_t;

/*
 * req_status values for udi_scsi_io_nak().
 * Note that there is no SCSI Meta 'good' status; use UDI_OK.
 */
/* scsi status byte has nonzero value: (check cond'n, etc.): */
#define UDI_SCSI_STAT_NONZERO_STATUS_BYTE	(1|UDI_STAT_META_SPECIFIC)   

/* Only used when contingent allegiance was not cleared with auto sense.
	(Implies scsi status byte = 2):   */
#define UDI_SCSI_STAT_ACA_PENDING		(2|UDI_STAT_META_SPECIFIC)

#define UDI_SCSI_STAT_NOT_PRESENT		(3|UDI_STAT_META_SPECIFIC)
#define UDI_SCSI_STAT_DEVICE_PHASE_ERROR	(4|UDI_STAT_META_SPECIFIC)
#define UDI_SCSI_STAT_UNEXPECTED_BUS_FREE	(5|UDI_STAT_META_SPECIFIC)
#define UDI_SCSI_STAT_DEVICE_PARITY_ERROR	(6|UDI_STAT_META_SPECIFIC)
#define UDI_SCSI_STAT_ABORTED_HD_BUS_RESET	(7|UDI_STAT_META_SPECIFIC)
#define UDI_SCSI_STAT_ABORTED_RMT_BUS_RESET	(8|UDI_STAT_META_SPECIFIC)
#define UDI_SCSI_STAT_ABORTED_REQ_BUS_RESET	(9|UDI_STAT_META_SPECIFIC)
#define UDI_SCSI_STAT_ABORTED_REQ_TGT_RESET	(10|UDI_STAT_META_SPECIFIC)
#define UDI_SCSI_STAT_LINK_FAILURE		(11|UDI_STAT_META_SPECIFIC)
#define UDI_SCSI_STAT_SELECTION_TIMEOUT		(12|UDI_STAT_META_SPECIFIC)
#define UDI_SCSI_STAT_HD_ABORTED		(13|UDI_STAT_META_SPECIFIC)

/* SCSI Ctl Ack status codes.  most failures go here:  */
#define UDI_SCSI_CTL_STAT_FAILED		(100|UDI_STAT_META_SPECIFIC)

/*
 * Channel Ops vector types
 * Individual channel operation prototypes, 
 * then ops vector structure definition.
 */

/*
 * udi_scsi_bind_req - Request a SCSI binding (PD-to-HD)
 */
typedef void udi_scsi_bind_req_op_t(
	udi_scsi_bind_cb_t	*cb,
	udi_ubit16_t		bind_flags,
	udi_ubit16_t		queue_depth,
	udi_ubit16_t		max_sense_len,
	udi_ubit16_t		aen_buf_size );

/* Bind Flag values */
#define UDI_SCSI_BIND_EXCLUSIVE		(1U<<0)
#define UDI_SCSI_TEMP_BIND_EXCLUSIVE	(1U<<1)

/*
 * udi_scsi_bind_ack - Acknowledge a SCSI bind req (HD-to-PD)
 */

typedef void udi_scsi_bind_ack_op_t(
	udi_scsi_bind_cb_t	*cb,
	udi_ubit32_t		hd_timeout_increase,
	udi_status_t		status );

/*
 * udi_scsi_unbind_req - Request a SCSI bind (PD-to-HD)
 */
typedef void udi_scsi_unbind_req_op_t(
	udi_scsi_bind_cb_t	*cb );

/*
 * udi_scsi_unbind_ack - Acknowledge a SCSI unbind (HD-to-PD)
 */
typedef void udi_scsi_unbind_ack_op_t(
	udi_scsi_bind_cb_t	*cb );

/*
 * udi_scsi_io_req - Request a SCSI I/O operation (PD-to-HD)
 */
typedef void udi_scsi_io_req_op_t(
	udi_scsi_io_cb_t	*cb );

/*
 * udi_scsi_io_ack - Acknowledge normal completion of SCSI I/O request
 * (HD-to-PD)
 */
typedef void udi_scsi_io_ack_op_t(
	udi_scsi_io_cb_t	*cb );

/*
 * udi_scsi_io_nak - Acknowledge abnormal completion of SCSI I/O request
 * (HD-to-PD)
 */
typedef void udi_scsi_io_nak_op_t(
	udi_scsi_io_cb_t	*cb,
	udi_status_t		req_status,
	udi_ubit8_t		scsi_status,
	udi_ubit8_t		sense_status,
	udi_buf_t		*sense_buf );

/*
 * udi_scsi_ctl_req - Request a SCSI control operation (PD-to-HD)
 */
typedef void udi_scsi_ctl_req_op_t(
	udi_scsi_ctl_cb_t	*cb );

/*
 * udi_scsi_ctl_ack - Acknowledge completion of SCSI control request (HD-to-PD)
 */
typedef void udi_scsi_ctl_ack_op_t(
	udi_scsi_ctl_cb_t	*cb,
	udi_status_t		status );

/*
 * udi_scsi_event_ind - SCSI event notification (HD-to-PD)
 */
typedef void udi_scsi_event_ind_op_t(
	udi_scsi_event_cb_t	*cb );

/*
 * udi_scsi_event_res - Acknowledge a SCSI event (PD-to-HD)
 */
typedef void udi_scsi_event_res_op_t(
	udi_scsi_event_cb_t	*cb );

/* SCSI Peripheral Driver entry point ops vector */
typedef const struct
{
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_scsi_bind_ack_op_t		*bind_ack_op;
	udi_scsi_unbind_ack_op_t	*unbind_ack_op;
	udi_scsi_io_ack_op_t		*io_ack_op;
	udi_scsi_io_nak_op_t		*io_nak_op;
	udi_scsi_ctl_ack_op_t		*ctl_ack_op;
	udi_scsi_event_ind_op_t		*event_ind_op;
} udi_scsi_pd_ops_t;

/* Ops Vector Number */
#define UDI_SCSI_PD_OPS_NUM	1

/* SCSI HBA Driver entry point ops vector */
typedef const struct
{
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_scsi_bind_req_op_t		*bind_req_op;
	udi_scsi_unbind_req_op_t	*unbind_req_op;
	udi_scsi_io_req_op_t		*io_req_op;
	udi_scsi_ctl_req_op_t		*ctl_req_op;
	udi_scsi_event_res_op_t		*event_res_op;
} udi_scsi_hd_ops_t;

/* Ops Vector Number */
#define UDI_SCSI_HD_OPS_NUM	2

/*
 * Function prototypes for PD-to-HD channel  operations
 */

udi_scsi_bind_req_op_t		udi_scsi_bind_req;
udi_scsi_unbind_req_op_t	udi_scsi_unbind_req;
udi_scsi_io_req_op_t		udi_scsi_io_req;
udi_scsi_ctl_req_op_t		udi_scsi_ctl_req;
udi_scsi_event_res_op_t		udi_scsi_event_res;

/*
 * Function prototypes for HD-to-PD channel operations
 */
udi_scsi_bind_ack_op_t		udi_scsi_bind_ack;
udi_scsi_unbind_ack_op_t	udi_scsi_unbind_ack;
udi_scsi_io_ack_op_t		udi_scsi_io_ack;
udi_scsi_io_nak_op_t		udi_scsi_io_nak;
udi_scsi_ctl_ack_op_t		udi_scsi_ctl_ack;
udi_scsi_event_ind_op_t		udi_scsi_event_ind;

/* Proxis */
udi_scsi_event_ind_op_t		udi_scsi_event_ind_unused;

/* Utility Functions */
void udi_scsi_inquiry_to_string(
	const udi_ubit8_t	*inquiry_data,
	udi_size_t		inquiry_len,
	char			*str);

#endif /* _UDI_SCSI_H */
