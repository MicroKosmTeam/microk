/*
 * File: udi_nic.c
 *
 * UDI Network Interface Metalanguage Library
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

#define UDI_VERSION 0x101
#define UDI_NIC_VERSION 0x101
#include <udi.h>	/* Public Core Environment Definitions */
#include <udi_nic.h>	/* Public Driver-to-Metalang Definitions */
#include <udi_nicP.h>	/* Private Network Metalanguage Library Definitions */

/*
 * ===========================================================================
 * Set-Interface-Ops Interfaces
 * ---------------------------------------------------------------------------
 */

static udi_mei_ops_vec_template_t net_ops_vec_template_list[] = {
	{
		UDI_ND_CTRL_OPS_NUM,
		UDI_MEI_REL_BIND | UDI_MEI_REL_EXTERNAL,
		udi_nic_nd_ctrl_op_templates
	},
	{
		UDI_ND_TX_OPS_NUM,
		UDI_MEI_REL_EXTERNAL,
		udi_nic_nd_tx_op_templates
	},
	{
		UDI_ND_RX_OPS_NUM,
		UDI_MEI_REL_EXTERNAL,
		udi_nic_nd_rx_op_templates
	},
	{
		UDI_NSR_CTRL_OPS_NUM,
		UDI_MEI_REL_BIND | UDI_MEI_REL_EXTERNAL | UDI_MEI_REL_INITIATOR,
		udi_nic_nsr_ctrl_op_templates
	},
	{
		UDI_NSR_TX_OPS_NUM,
		UDI_MEI_REL_EXTERNAL | UDI_MEI_REL_INITIATOR,
		udi_nic_nsr_tx_op_templates
	},
	{
		UDI_NSR_RX_OPS_NUM,
		UDI_MEI_REL_EXTERNAL | UDI_MEI_REL_INITIATOR,
		udi_nic_nsr_rx_op_templates
	},
	{
		0, 0, NULL	/* Terminator */
	}
};

static udi_mei_enumeration_rank_func_t udi_nic_rank_func;

static udi_ubit8_t udi_nic_rank_func(udi_ubit32_t attr_device_match,
                void **attr_value_list)
{
	udi_assert(0);
	return 0;
}


udi_mei_init_t	udi_meta_info = {
	net_ops_vec_template_list,
	udi_nic_rank_func
};


/*
 * ========================================================================
 * 
 * Unmarshalling Routines (aka "Metalanguage Stubs")
 *  
 * After dequeuing a control block from the region queue the environment
 * calls the corresponding metalanguage-specific unmarshal routine to 
 * unmarshal the extra parameters from the control block and make the 
 * associated call to the driver.
 * ------------------------------------------------------------------------
 */


/*
 * ===========================================================================
 * Interface Operations
 * ---------------------------------------------------------------------------
 */

/*
 * udi_nd_bind_req	- Network driver bind request operation
 */
UDI_MEI_STUBS(udi_nd_bind_req, udi_nic_bind_cb_t,
	      2, (tx_chan_index, rx_chan_index),
		 (udi_index_t, udi_index_t),
		 (UDI_VA_INDEX_T, UDI_VA_INDEX_T),
	      UDI_ND_CTRL_OPS_NUM, UDI_NIC_BIND_REQ)


/*
 * udi_nsr_bind_ack	- Network bind acknowledgement operation
 */
UDI_MEI_STUBS(udi_nsr_bind_ack, udi_nic_bind_cb_t,
	      1, (status), (udi_status_t), (UDI_VA_STATUS_T),
	      UDI_NSR_CTRL_OPS_NUM, UDI_NIC_BIND_ACK)


/*
 * udi_nd_unbind_req	- Network unbind request operation
 */
UDI_MEI_STUBS(udi_nd_unbind_req, udi_nic_cb_t,
	      0, (), (), (),
	      UDI_ND_CTRL_OPS_NUM, UDI_NIC_UNBIND_REQ)

/*
 * udi_nsr_unbind_ack	- Network unbind acknowledgement operation
 */
UDI_MEI_STUBS(udi_nsr_unbind_ack, udi_nic_cb_t,
	      1, (status), (udi_status_t), (UDI_VA_STATUS_T),
	      UDI_NSR_CTRL_OPS_NUM, UDI_NIC_UNBIND_ACK)

/*
 * udi_nd_enable_req	- Network link enable request operation
 */
UDI_MEI_STUBS(udi_nd_enable_req, udi_nic_cb_t,
	      0, (), (), (),
	      UDI_ND_CTRL_OPS_NUM, UDI_NIC_ENABLE_REQ)

/*
 * udi_nsr_enable_ack	- Network link enable acknowledgement operation
 */
UDI_MEI_STUBS(udi_nsr_enable_ack, udi_nic_cb_t,
	      1, (status), (udi_status_t), (UDI_VA_STATUS_T),
	      UDI_NSR_CTRL_OPS_NUM, UDI_NIC_ENABLE_ACK)

/*
 * udi_nd_disable_req	- Network link disable request operation
 */
UDI_MEI_STUBS(udi_nd_disable_req, udi_nic_cb_t,
	      0, (), (), (),
	      UDI_ND_CTRL_OPS_NUM, UDI_NIC_DISABLE_REQ)

/*
 * udi_nd_ctrl_req	- Network control operation request
 */
UDI_MEI_STUBS(udi_nd_ctrl_req, udi_nic_ctrl_cb_t,
	      0, (), (), (),
	      UDI_ND_CTRL_OPS_NUM, UDI_NIC_CTRL_REQ)

/*
 * udi_nsr_ctrl_ack	- Network control acknowledgement operation
 */
UDI_MEI_STUBS(udi_nsr_ctrl_ack, udi_nic_ctrl_cb_t,
	      1, (status), (udi_status_t), (UDI_VA_STATUS_T),
	      UDI_NSR_CTRL_OPS_NUM, UDI_NIC_CTRL_ACK)

/*
 * udi_nsr_status_ind	- Network status indication
 */
UDI_MEI_STUBS(udi_nsr_status_ind, udi_nic_status_cb_t,
	      0, (), (), (),
	      UDI_NSR_CTRL_OPS_NUM, UDI_NIC_STATUS_IND)

/*
 * udi_nd_info_req	- Network information request
 */
UDI_MEI_STUBS(udi_nd_info_req, udi_nic_info_cb_t,
	      1, (reset_statistics), (udi_boolean_t), (UDI_VA_BOOLEAN_T),
	      UDI_ND_CTRL_OPS_NUM, UDI_NIC_INFO_REQ)

/*
 * udi_nsr_info_ack	- Network information response
 */
UDI_MEI_STUBS(udi_nsr_info_ack, udi_nic_info_cb_t,
	      0, (), (), (),
	      UDI_NSR_CTRL_OPS_NUM, UDI_NIC_INFO_ACK)

/*
 * udi_nsr_tx_rdy	- Network driver ready to transmit packet
 */
UDI_MEI_STUBS(udi_nsr_tx_rdy, udi_nic_tx_cb_t,
	      0, (), (), (),
	      UDI_NSR_TX_OPS_NUM, UDI_NIC_TX_RDY)

/*
  * udi_nd_tx_req	- Network send packet
  */
UDI_MEI_STUBS(udi_nd_tx_req, udi_nic_tx_cb_t,
	      0, (), (), (),
	      UDI_ND_TX_OPS_NUM, UDI_NIC_TX_REQ)

/*
 * udi_nd_exp_tx_req	- Expedited data transmit request
 */
UDI_MEI_STUBS(udi_nd_exp_tx_req, udi_nic_tx_cb_t,
	      0, (), (), (),
	      UDI_ND_TX_OPS_NUM, UDI_NIC_EXP_TX_REQ)

/*
 * udi_nsr_rx_ind	- Network receive packet indication
 */
UDI_MEI_STUBS(udi_nsr_rx_ind, udi_nic_rx_cb_t,
	      0, (), (), (),
	      UDI_NSR_RX_OPS_NUM, UDI_NIC_RX_IND)

/*
 * udi_nsr_exp_rx_ind	- Network receive packet indication
 */
UDI_MEI_STUBS(udi_nsr_exp_rx_ind, udi_nic_rx_cb_t,
	      0, (), (), (),
	      UDI_NSR_RX_OPS_NUM, UDI_NIC_EXP_RX_IND)

/*
 * udi_nd_rx_rdy	- Network receive packet response
 */
UDI_MEI_STUBS(udi_nd_rx_rdy, udi_nic_rx_cb_t,
	      0, (), (), (),
	      UDI_ND_RX_OPS_NUM, UDI_NIC_RX_RDY)
