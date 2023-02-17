
/*
 * File: env/posix/sptest.c
 * 
 * Static properties exercises.
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
#include <udi_osdep.h>

/*
 * The following fuction declaration are not used in a
 * typical driver environment.
 */
char *_udi_osdep_sprops_get_shortname(_udi_driver_t *drv);
void _udi_osdep_sprops_set(_udi_driver_t *drv,
			   void *sprops_idx);
udi_ubit32_t _udi_osdep_sprops_get_version(_udi_driver_t *drv);
udi_ubit32_t _udi_osdep_sprops_get_release(_udi_driver_t *drv);
udi_ubit32_t _osdep_sprops_get_num(_udi_driver_t *drv,
				   udi_ubit8_t type);
udi_ubit32_t _udi_osdep_sprops_get_meta_idx(_udi_driver_t *drv,
					    udi_ubit32_t index);
char *_udi_osdep_sprops_get_prov_if(_udi_driver_t *drv,
				    udi_ubit32_t index);
udi_ubit32_t _udi_osdep_sprops_get_prov_ver(_udi_driver_t *drv,
					    udi_ubit32_t index);
udi_ubit32_t _udi_osdep_sprops_get_reg_idx(_udi_driver_t *drv,
					   udi_ubit32_t index);
udi_ubit32_t _udi_osdep_sprops_get_reg_attr(_udi_driver_t *drv,
					    udi_ubit32_t index);
udi_ubit32_t _udi_osdep_sprops_get_reg_overrun(_udi_driver_t *drv,
					       udi_ubit32_t index);
udi_ubit32_t _udi_osdep_sprops_get_dev_meta_idx(_udi_driver_t *drv,
						udi_ubit32_t index);
udi_ubit32_t _udi_osdep_sprops_get_dev_nattr(_udi_driver_t *drv,
					     udi_ubit32_t index);
void *_udi_osdep_sprops_get_dev_attr(_udi_driver_t *drv,
				     udi_ubit32_t dev_index,
				     udi_ubit32_t att_index);
char *_udi_osdep_sprops_get_readfile(_udi_driver_t *drv,
				     udi_ubit32_t index);
udi_boolean_t _udi_osdep_sprops_get_rf_data(_udi_driver_t *drv,
					    const char *attr_name,
					    char *buf,
					    udi_size_t buf_len,
					    udi_size_t *act_len);

/*
 * Create bogus definitions here because including other unnecssary stuff
 * beyond posix.c requires LOTS of other stuff.  This way, we keep things
 * to a minimal.
 */
void *udi_giomap_sprops_idx;
void *udi_udi_MA_sprops_idx;
void *udi_gio_meta_sprops_idx;
udi_mei_init_t udi_mgmt_meta_info = { NULL };
udi_init_context_t *_udi_MA_initcontext;
udi_trevent_t _udi_trlog_initial;
udi_trevent_t _udi_trlog_ml_initial[256];
void
_udi_env_log_write(udi_trevent_t trace_event,
		   udi_ubit8_t severity,
		   udi_index_t meta_idx,
		   udi_status_t original_status,
		   udi_ubit16_t msgnum,
		   ...)
{
}

void
udi_trace_write(udi_init_context_t *init_context,
		udi_trevent_t trace_event,
		udi_index_t meta_idx,
		udi_ubit32_t msgnum,
		...)
{
}
udi_queue_t *
udi_dequeue(udi_queue_t *element)
{
	return (udi_queue_t *)NULL;
}

void
_udi_MA_init(void)
{
}
void
_udi_alloc_init(void)
{
}
void
_udi_alloc_deinit(void)
{
}
udi_boolean_t
_udi_alloc_daemon_work(void)
{
	return TRUE;
}

void
udi_enqueue(udi_queue_t *new_el,
	    udi_queue_t *old_el)
{
}
void
_udi_MA_deinit(void (*cbfn) ())
{
}
char *hard_coded_locale = "C";
char *
_udi_current_locale(char *new_locale)
{
	return (hard_coded_locale);
}

udi_ubit32_t _udi_debug_level = 0;
void
_udi_queue_callback(_udi_cb_t *cb)
{
}
_udi_alloc_ops_t _udi_alloc_ops_nop = { NULL };

_OSDEP_MUTEX_T _udi_alloc_lock;
_udi_meta_t *
_udi_MA_meta_load(const char *arg1,
		  udi_mei_init_t *arg2,
		  const _OSDEP_DRIVER_T os_drv_init)
{
	return (_udi_meta_t *) NULL;
}

udi_size_t
_udi_instance_attr_get(_udi_dev_node_t *node,
		       const char *name,
		       void *value,
		       udi_size_t length,
		       udi_instance_attr_type_t *attr_type)
{
	return 0;
}

void
_udi_run_inactive_region(_udi_region_t *rgn)
{
}

void
check_iresult(int *num,
	      unsigned long num1,
	      unsigned long num2)
{
	if (num1 != num2) {
		printf("TEST %d FAILED!\n", *num);
		printf("\tExpected: 0x%lx, Got: 0x%lx\n", num2, num1);
		abort();
	}

	(*num)++;
}

void
check_sresult(int *num,
	      unsigned char *str1,
	      unsigned char *str2)
{
	if (udi_strcmp((char *)str1, (char *)str2) != 0) {
		printf("TEST %d FAILED!\n", *num);
		printf("\tExpected: \"%s\", Got: \"%s\"\n", str2, str1);
		abort();
	}

	(*num)++;
}

int
main(void)
{
	udi_ubit32_t index1, index2, index3, index4;
	void **attr;
	_udi_driver_t driver, *drv = &driver;

	/*
	 * _udi_sprops_idx_t *sprops_idx = udi_sp_drv_sprops_idx; 
	 */
	int check;
	unsigned long num1, num2, num3, num4;
	unsigned char *str1;
	udi_size_t act_len;
	char buf[50];

	check = 0;
	drv->drv_name = "sp_drv";

	printf("STATIC PROPERTIES TESTS RUNNING ...\n");
	_udi_osdep_sprops_init(drv);

	str1 = (unsigned char *)_udi_osdep_sprops_get_shortname(drv);
	printf("\tShortname = %s\n", str1);
	/*
	 * Test 0 
	 */
	check_sresult(&check, str1, (unsigned char *)"sp_drv");

	num1 = _udi_osdep_sprops_get_version(drv);
	printf("\tVersion = 0x%lX\n", num1);
	/*
	 * Test 1 
	 */
	check_iresult(&check, num1, 0x100);

	num1 = _udi_osdep_sprops_get_release(drv);
	printf("\tRelease = %ld\n", num1);
	/*
	 * Test 2 
	 */
	check_iresult(&check, num1, 1);

	index1 = _udi_osdep_sprops_count(drv, UDI_SP_META);
	printf("\n\tNumber of metas: %d\n", index1);
	/*
	 * Test 3 
	 */
	check_iresult(&check, index1, 2);
	for (index2 = 0; index2 < index1; index2++) {
		num1 = _udi_osdep_sprops_get_meta_idx(drv, index2);
		str1 = (unsigned char *)_udi_osdep_sprops_get_meta_if(drv,
								      num1);
		printf("\t\tMeta Index: %ld \"%s\"\n", num1, str1);
		if (index2 == 0) {
			num2 = 1;
			/*
			 * Test 4 
			 */
			check_iresult(&check, num1, num2);
		} else {
			num2 = 2;
			/*
			 * Test 6 
			 */
			check_iresult(&check, num1, num2);
		}
		if (index2 == 0) {
			/*
			 * Test 5 
			 */
			check_sresult(&check, str1,
				      (unsigned char *)"udi_scsi");
		} else {
			num2 = 2;
			/*
			 * Test 7 
			 */
			check_sresult(&check, str1,
				      (unsigned char *)"udi_bridge");
		}
	}

	index1 = _udi_osdep_sprops_count(drv, UDI_SP_CHILD_BINDOPS);
	printf("\n\tNumber of child_bind_ops: %d (meta_idx, region_idx, "
	       "ops_idx)\n", index1);
	/*
	 * Test 8 
	 */
	check_iresult(&check, index1, 1);
	for (index2 = 0; index2 < index1; index2++) {
		num1 = _udi_osdep_sprops_get_bindops(drv, index2,
						     UDI_SP_CHILD_BINDOPS,
						     UDI_SP_OP_METAIDX);
		num2 = _udi_osdep_sprops_get_bindops(drv, index2,
						     UDI_SP_CHILD_BINDOPS,
						     UDI_SP_OP_REGIONIDX);
		num3 = _udi_osdep_sprops_get_bindops(drv, index2,
						     UDI_SP_CHILD_BINDOPS,
						     UDI_SP_OP_OPSIDX);
		printf("\t\tops: %ld, %ld, %ld\n", num1, num2, num3);
		/*
		 * Test 9 
		 */
		num4 = 1;
		check_iresult(&check, num1, num4);
		/*
		 * Test 10 
		 */
		num4 = 0;
		check_iresult(&check, num2, num4);
		/*
		 * Test 11 
		 */
		num4 = 2;
		check_iresult(&check, num3, num4);
	}

	index1 = _udi_osdep_sprops_count(drv, UDI_SP_PARENT_BINDOPS);
	printf("\n\tNumber of parent_bind_ops: %d (meta_idx, region_idx, "
	       "ops_idx)\n", index1);
	/*
	 * Test 12 
	 */
	check_iresult(&check, index1, 1);
	for (index2 = 0; index2 < index1; index2++) {
		num1 = _udi_osdep_sprops_get_bindops(drv, index2,
						     UDI_SP_PARENT_BINDOPS,
						     UDI_SP_OP_METAIDX);
		num2 = _udi_osdep_sprops_get_bindops(drv, index2,
						     UDI_SP_PARENT_BINDOPS,
						     UDI_SP_OP_REGIONIDX);
		num3 = _udi_osdep_sprops_get_bindops(drv, index2,
						     UDI_SP_PARENT_BINDOPS,
						     UDI_SP_OP_OPSIDX);
		printf("\t\tops: %ld, %ld, %ld\n", num1, num2, num3);
		/*
		 * Test 13 
		 */
		num4 = 2;
		check_iresult(&check, num1, num4);
		/*
		 * Test 14 
		 */
		num4 = 0;
		check_iresult(&check, num2, num4);
		/*
		 * Test 15 
		 */
		num4 = 1;
		check_iresult(&check, num3, num4);
	}

	index1 = _udi_osdep_sprops_count(drv, UDI_SP_INTERN_BINDOPS);
	printf("\n\tNumber of internal_bind_ops: %d (meta_idx, region_idx, "
	       "primary_ops_idx, secondary_ops_idx)\n", index1);
	/*
	 * Test 16 
	 */
	check_iresult(&check, index1, 2);
	for (index2 = 0; index2 < index1; index2++) {
		num1 = _udi_osdep_sprops_get_bindops(drv, index2,
						     UDI_SP_INTERN_BINDOPS,
						     UDI_SP_OP_METAIDX);
		num2 = _udi_osdep_sprops_get_bindops(drv, index2,
						     UDI_SP_INTERN_BINDOPS,
						     UDI_SP_OP_REGIONIDX);
		num3 = _udi_osdep_sprops_get_bindops(drv, index2,
						     UDI_SP_INTERN_BINDOPS,
						     UDI_SP_OP_PRIMOPSIDX);
		num4 = _udi_osdep_sprops_get_bindops(drv, index2,
						     UDI_SP_INTERN_BINDOPS,
						     UDI_SP_OP_SECOPSIDX);
		printf("\t\tops: %ld, %ld, %ld, %ld\n", num1, num2, num3,
		       num4);
		/*
		 * No tests for this one 
		 */
	}

	index1 = _udi_osdep_sprops_count(drv, UDI_SP_PROVIDE);
	printf("\n\tNumber of provides: %d\n", index1);
	/*
	 * Test 17 
	 */
	check_iresult(&check, index1, 1);
	for (index2 = 0; index2 < index1; index2++) {
		str1 = (unsigned char *)_udi_osdep_sprops_get_prov_if(drv,
								      index2);
		num1 = _udi_osdep_sprops_get_prov_ver(drv, index2);
		printf("\t\tProvides: %s 0x%lx\n", str1, num1);
		/*
		 * Test 18 
		 */
		check_sresult(&check, str1, (unsigned char *)"some_interface");
		/*
		 * Test 19 
		 */
		check_iresult(&check, num1, 0x95);
	}

	index1 = _udi_osdep_sprops_count(drv, UDI_SP_REGION);
	printf("\n\tNumber of regions: %d\n", index1);
	/*
	 * Test 20 
	 */
	check_iresult(&check, index1, 1);
	for (index2 = 0; index2 < index1; index2++) {
		index3 = _udi_osdep_sprops_get_reg_idx(drv, index2);
		printf("\t\tRegion Index: %d\n", index3);
		/*
		 * Test 21 
		 */
		if (index2 == 0)
			check_iresult(&check, index3, 0);
		num1 = _udi_osdep_sprops_get_reg_attr(drv, index2);
		printf("\t\t\tattributes = 0x%lx\n", num1);
		/*
		 * Test 22 
		 */
		check_iresult(&check, num1, 0x80211);
		num1 = _udi_osdep_sprops_get_reg_overrun(drv, index2);
		printf("\t\t\toverrun_time = 0x%lx\n", num1);
		/*
		 * Test 23 
		 */
		check_iresult(&check, num1, 0xdead);
	}

	index1 = _udi_osdep_sprops_count(drv, UDI_SP_DEVICE);
	printf("\n\tNumber of devices: %d\n", index1);
	/*
	 * Test 23 
	 */
	check_iresult(&check, index1, 1);
	for (index2 = 0; index2 < index1; index2++) {
		index4 = _udi_osdep_sprops_get_dev_meta_idx(drv, index2);
		index3 = _udi_osdep_sprops_get_dev_nattr(drv, index2);
		printf("\t\tMeta Index: %d has %d attributes\n",
		       index4, index3);
		/*
		 * Test 24 
		 */
		check_iresult(&check, index4, 2);
		/*
		 * Test 25 
		 */
		check_iresult(&check, index3, 4);
		for (index4 = 0; index4 < index3; index4++) {
			attr = (void **)_udi_osdep_sprops_get_dev_attr(drv,
								       index2,
								       index4);
			printf("\t\t\tAttribute: %s ", (char *)*attr);
			if (index4 == 0)
				/*
				 * Test 26 
				 */
				check_sresult(&check, (unsigned char *)*attr,
					      (unsigned char *)"bus_type");
			else if (index4 == 1)
				/*
				 * Test 28 
				 */
				check_sresult(&check, (unsigned char *)*attr,
					      (unsigned char *)
					      "pci_vendor_id");
			else if (index4 == 2)
				/*
				 * Test 30 
				 */
				check_sresult(&check, (unsigned char *)*attr,
					      (unsigned char *)
					      "pci_device_id");
			else if (index4 == 3)
				/*
				 * Test ?? 
				 */
				check_sresult(&check, (unsigned char *)*attr,
					      (unsigned char *)"init_code");
			switch (*(udi_ubit32_t *)(attr + 1)) {
			case UDI_ATTR_STRING:
				printf("UDI_ATTR_STRING ");
				printf("\"%s\"\n", (char *)*(attr + 2));
				/*
				 * Test 27 
				 */
				str1 = *(attr + 2);
				check_sresult(&check,
					      (unsigned char *)str1,
					      (unsigned char *)"pci");
				break;
			case UDI_ATTR_UBIT32:
				printf("UDI_ATTR_UBIT32 ");
				if (index4 == 1) {
					/*
					 * Test 29 
					 */
					num1 = *(udi_ubit32_t *)
						(attr + 2);
					printf("0x%lx\n", num1);
					check_iresult(&check, num1, 0x1044);
				} else {
					/*
					 * Test 31 
					 */
					num1 = *(udi_ubit32_t *)
						(attr + 2);
					printf("0x%lx\n", num1);
					check_iresult(&check, num1, 0xA400);
				}
				break;
			case UDI_ATTR_BOOLEAN:
				printf("UDI_ATTR_BOOLEAN ");
				if (*(char *)(attr + 2) == 'T')
					printf("TRUE\n");
				else
					printf("FALSE\n");
				break;
			case UDI_ATTR_ARRAY8:
				printf("UDI_ATTR_ARRAY8\n");
				break;
			default:
				printf("UNKNOWN!\n");
				break;
			}
		}
	}

	index1 = _udi_osdep_sprops_count(drv, UDI_SP_READFILE);
	printf("\n\tNumber of readable files: %d\n", index1);
	/*
	 * Test 32 
	 */
	check_iresult(&check, index1, 1);
	for (index2 = 0; index2 < index1; index2++) {
		str1 = (unsigned char *)_udi_osdep_sprops_get_readfile(drv,
								       index2);
		printf("\t\tReadable File: %s\n", str1);
		/*
		 * Test 33 
		 */
		check_sresult(&check, str1, (unsigned char *)"read_file");
	}

	printf("\tGet string number 4, in the C locale, using an integer "
	       "index:\n");
	str1 = (unsigned char *)_udi_osdep_sprops_ifind_message(drv, "C", 4);
	printf("\t\t\"%s\"\n", str1);
	/*
	 * Test 49 
	 */
	check_sresult(&check, str1, (unsigned char *)"Sprops Test Driver");
	printf("\tGet string 3 in an invalid locale, to return the C string:"
	       "\n");
	str1 = (unsigned char *)_udi_osdep_sprops_ifind_message(drv, "X", 3);
	printf("\t\t\"%s\"\n", str1);
	/*
	 * Test 50 
	 */
	check_sresult(&check, str1,
		      (unsigned char *)"POSIX static properties test driver");
	printf("\tGet string 7 in an invalid locale, to return not found "
	       "string:\n");
	str1 = (unsigned char *)_udi_osdep_sprops_ifind_message(drv, "X", 7);
	printf("\t\t\"%s\"\n", str1);
	/*
	 * Test 51 
	 */
	check_sresult(&check, str1, (unsigned char *)"[Unknown message number "
		      "7.]");
	printf("\tGet string number 1, in the gibberish locale, using an "
	       "integer index, reading it from the file:\n");
	str1 = (unsigned char *)_udi_osdep_sprops_ifind_message(drv,
								"gibberish",
								1);
	printf("\t\t\"%s\"\n", str1);
	/*
	 * Test 52 
	 */
	check_sresult(&check, str1, (unsigned char *)"Gibberish: This is a "
		      "message in a defined message_file.");

	printf("\tGet 20 bytes, at offset 12, from the readable file:\n");
	_udi_osdep_sprops_get_rf_data(drv, "read_file:12", buf, 20, &act_len);
	buf[20] = '\0';
	printf("\t\t\"%s\"\n", buf);
	/*
	 * Test 52 
	 */
	check_sresult(&check, (unsigned char *)buf,
		      (unsigned char *)"internal text of a d");

	printf("\tGet an array definition for init_code for device 0:\n");
	index1 = _udi_osdep_sprops_get_dev_attr_value(drv, "init_code", 0,
						      UDI_ATTR_ARRAY8, buf);
	printf("\t\tdevice 0 init_code array = %s (%d bytes)\n", buf, index1);
	check_sresult(&check, (unsigned char *)buf,
		      (unsigned char *)"HELP ME!");
	printf("STATIC PROPERTIES TESTS COMPLETED SUCCESSFULLY!\n");
	return EXIT_SUCCESS;
}
