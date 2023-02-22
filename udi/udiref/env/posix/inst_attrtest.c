
/*
 * File: env/posix/inst_attrtest.c
 *
 * Test device node instance attributes.
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
#include <errno.h>
#include <posix.h>

void
_init_node(_udi_dev_node_t *node,
	   _udi_dev_node_t *parent,
	   udi_ubit32_t child_id)
{
	UDI_QUEUE_INIT(&node->child_list);
	UDI_QUEUE_INIT(&node->sibling);
	UDI_QUEUE_INIT(&node->attribute_list);
	_OSDEP_MUTEX_INIT(&node->lock, "_udi_node_lock");
	node->parent_node = parent;
	node->read_in_progress = 0;
	node->my_child_id = child_id;
	if (parent) {
		node->my_child_id = child_id;
		UDI_ENQUEUE_TAIL(&parent->child_list, &node->sibling);
	}
	/*
	 * the osdep part 
	 */
	node->node_osinfo.posix_dev_node.backingstore = tmpfile();
	udi_assert(node->node_osinfo.posix_dev_node.backingstore);
}

void
_deinit_node(_udi_dev_node_t *node)
{
	_OSDEP_MUTEX_DEINIT(&node->lock);
}

void
_backing_store_dump(_udi_dev_node_t *node)
{

	FILE *file = node->node_osinfo.posix_dev_node.backingstore;
	int c;

	printf("*****************\n");
	rewind(file);
	while (EOF != (c = fgetc(file))) {
		printf("%c", (char)c);
	}
}

udi_size_t
udi_posix_instance_attr_get(_udi_dev_node_t *node,
			    const char *name,
			    udi_ubit32_t child_id,
			    void *value,
			    udi_size_t length,
			    udi_instance_attr_type_t *attr_type)
{

	switch (name[0]) {
	case '^':
		node = node->parent_node;
		break;
	case '@':
		node = _udi_get_child_node(node, child_id);
	}

	return _udi_instance_attr_get(node, name, value, length, attr_type);
}

udi_status_t
udi_posix_instance_attr_set(_udi_dev_node_t *node,
			    const char *name,
			    udi_ubit32_t child_id,
			    const void *value,
			    udi_size_t length,
			    udi_instance_attr_type_t attr_type)
{

	udi_boolean_t persistent = FALSE;

	switch (name[0]) {
	case '^':
		node = node->parent_node;
	case '$':
		break;
	case '%':
	case '@':
		persistent = TRUE;
		break;
	default:
		node = _udi_get_child_node(node, child_id);
	}

	return _udi_instance_attr_set(node, name, value, length, attr_type,
				      persistent);

}

int
main(int argc,
     char *argv[])
{
	_udi_dev_node_t self, child1, child2;
	udi_status_t status;

	udi_boolean_t attr_bool;
	udi_ubit32_t attr_ubit32;
	char buf[100];
	udi_size_t act_length;
	udi_instance_attr_type_t attr_type;
	int c;
	char *fname = NULL;

	memset(&self, 0, sizeof (self));
	memset(&child1, 0, sizeof (child1));
	memset(&child2, 0, sizeof (child2));

	while ((c = getopt(argc, argv, "f:d:")) != EOF) {
		switch (c) {
		case 'f':
			fname = optarg;
			break;
		case 'd':
			_udi_debug_level = atoi(optarg);
			break;
		default:
			fprintf(stderr, "%s: unknown argument\n", argv[0]);
			exit(1);
		}
	}

	posix_init(argc, argv, "", NULL);
	_init_node(&self, NULL, 1);
	_init_node(&child1, &self, 1);
	_init_node(&child2, &self, 2);

	/*
	 * enumeration attribs 
	 */
	status = udi_posix_instance_attr_set(&self, "enum_attr_string", 1,
					     "enum_val1",
					     strlen("enum_val1") + 1,
					     UDI_ATTR_STRING);
	udi_assert(status == UDI_OK);
	status = udi_posix_instance_attr_set(&self, "enum_attr_array", 1,
					     "enum_val2", strlen("enum_val2"),
					     UDI_ATTR_ARRAY8);
	udi_assert(status == UDI_OK);
	attr_ubit32 = 32;
	status = udi_posix_instance_attr_set(&self, "enum_attr_ubit32", 1,
					     &attr_ubit32,
					     sizeof (udi_ubit32_t),
					     UDI_ATTR_UBIT32);
	udi_assert(status == UDI_OK);
	attr_bool = TRUE;
	status = udi_posix_instance_attr_set(&self, "enum_attr_boolean", 1,
					     &attr_bool,
					     sizeof (udi_boolean_t),
					     UDI_ATTR_BOOLEAN);
	udi_assert(status == UDI_OK);

/* can i get it back ? */
	act_length = udi_posix_instance_attr_get(&child1, "enum_attr_string",
						 0, (void *)&buf, 100,
						 &attr_type);
	udi_assert(act_length == udi_strlen("enum_val1") + 1 && attr_type == UDI_ATTR_STRING);	/* NULL terminator is counted as part of length of attrib */
	udi_assert(udi_strncmp(buf, "enum_val1", act_length) == 0);
	act_length = udi_posix_instance_attr_get(&child1, "enum_attr_array", 0,
						 (void *)&buf, 100,
						 &attr_type);
	udi_assert(act_length == udi_strlen("enum_val2") &&
		   attr_type == UDI_ATTR_ARRAY8);
	udi_assert(udi_strncmp(buf, "enum_val2", act_length) == 0);
	act_length =
		udi_posix_instance_attr_get(&child1, "enum_attr_ubit32", 0,
					    (void *)&attr_ubit32,
					    sizeof (udi_ubit32_t), &attr_type);
	udi_assert(act_length == sizeof (udi_ubit32_t) &&
		   attr_type == UDI_ATTR_UBIT32 && attr_ubit32 == 32);

	attr_bool = FALSE;
	act_length =
		udi_posix_instance_attr_get(&child1, "enum_attr_boolean", 0,
					    (void *)&attr_bool,
					    sizeof (udi_boolean_t),
					    &attr_type);
	udi_assert(act_length == sizeof (udi_boolean_t) &&
		   attr_type == UDI_ATTR_BOOLEAN && attr_bool == TRUE);

/* change value  */
	status = udi_posix_instance_attr_set(&self, "enum_attr_string", 1,
					     "bigger enum val",
					     udi_strlen("bigger enum val") + 1,
					     UDI_ATTR_STRING);
	udi_assert(status == UDI_OK);
	act_length =
		udi_posix_instance_attr_get(&child1, "enum_attr_string", 0,
					    (void *)&buf, 100, &attr_type);
	udi_assert(act_length == udi_strlen("bigger enum val") + 1 &&
		   attr_type == UDI_ATTR_STRING &&
		   udi_strncmp("bigger enum val", buf, act_length) == 0);

/* change type */
	status = udi_posix_instance_attr_set(&self, "enum_attr_boolean", 1,
					     "boolean to string",
					     udi_strlen("boolean to string") +
					     1, UDI_ATTR_STRING);
	udi_assert(status == UDI_OK);
	act_length =
		udi_posix_instance_attr_get(&child1, "enum_attr_boolean", 0,
					    (void *)&buf, 100, &attr_type);
	udi_assert(act_length == udi_strlen("boolean to string") + 1 &&
		   attr_type == UDI_ATTR_STRING &&
		   udi_strncmp(buf, "boolean to string", act_length) == 0);

/* delete one */
	status = udi_posix_instance_attr_set(&self, "enum_attr_boolean", 1,
					     NULL, 0, UDI_ATTR_STRING);
	udi_assert(status == UDI_OK);
	act_length =
		udi_posix_instance_attr_get(&child1, "enum_attr_boolean", 0,
					    (void *)&buf, 100, &attr_type);
	udi_assert(act_length == 0);

/* child2 should not have any of these */
	act_length =
		udi_posix_instance_attr_get(&child2, "enum_attr_string", 0,
					    (void *)&buf, 100, &attr_type);
	udi_assert(act_length == 0);

/* self written attributes */
	attr_bool = TRUE;
	status = udi_posix_instance_attr_set(&self, "$self_attr_boolean", 1,
					     (void *)&attr_bool,
					     sizeof (udi_boolean_t),
					     UDI_ATTR_BOOLEAN);
	udi_assert(status == UDI_OK);
	attr_bool = FALSE;
	act_length =
		udi_posix_instance_attr_get(&self, "$self_attr_boolean", 0,
					    (void *)&attr_bool,
					    sizeof (udi_boolean_t),
					    &attr_type);
	udi_assert(act_length == sizeof (udi_boolean_t) &&
		   attr_type == UDI_ATTR_BOOLEAN && attr_bool == TRUE);

	attr_bool = TRUE;
	status = udi_posix_instance_attr_set(&self, "%self_attr_boolean", 1,
					     (void *)&attr_bool,
					     sizeof (udi_boolean_t),
					     UDI_ATTR_BOOLEAN);
	udi_assert(status == UDI_OK);
	attr_bool = FALSE;
	act_length =
		udi_posix_instance_attr_get(&self, "%self_attr_boolean", 0,
					    (void *)&attr_bool,
					    sizeof (udi_boolean_t),
					    &attr_type);
	udi_assert(act_length == sizeof (udi_boolean_t) &&
		   attr_type == UDI_ATTR_BOOLEAN && attr_bool == TRUE);

/* parent visible attributes */
	attr_bool = TRUE;
	status = udi_posix_instance_attr_set(&child1, "@pvis_attr_boolean", 1,
					     (void *)&attr_bool,
					     sizeof (udi_boolean_t),
					     UDI_ATTR_BOOLEAN);
	udi_assert(status == UDI_OK);
	attr_bool = FALSE;
	act_length =
		udi_posix_instance_attr_get(&self, "@pvis_attr_boolean", 1,
					    (void *)&attr_bool,
					    sizeof (udi_boolean_t),
					    &attr_type);
	udi_assert(act_length == sizeof (udi_boolean_t) &&
		   attr_type == UDI_ATTR_BOOLEAN && attr_bool == TRUE);

/* change a persistent value */
	attr_bool = FALSE;
	status = udi_posix_instance_attr_set(&child1, "@pvis_attr_boolean", 1,
					     (void *)&attr_bool,
					     sizeof (udi_boolean_t),
					     UDI_ATTR_BOOLEAN);
	udi_assert(status == UDI_OK);

/* sibling attributes */
	attr_bool = TRUE;
	status = udi_posix_instance_attr_set(&child1, "^sibling_attr_boolean",
					     1, (void *)&attr_bool,
					     sizeof (udi_boolean_t),
					     UDI_ATTR_BOOLEAN);
	udi_assert(status == UDI_OK);
	attr_bool = FALSE;
	act_length =
		udi_posix_instance_attr_get(&child2, "^sibling_attr_boolean",
					    1, (void *)&attr_bool,
					    sizeof (udi_boolean_t),
					    &attr_type);
	udi_assert(act_length == sizeof (udi_boolean_t) &&
		   attr_type == UDI_ATTR_BOOLEAN && attr_bool == TRUE);

	attr_bool = FALSE;
	act_length =
		udi_posix_instance_attr_get(&child1, "^sibling_attr_boolean",
					    1, (void *)&attr_bool,
					    sizeof (udi_boolean_t),
					    &attr_type);
	udi_assert(act_length == sizeof (udi_boolean_t) &&
		   attr_type == UDI_ATTR_BOOLEAN && attr_bool == TRUE);

/* simulate a reboot */
	_udi_instance_attr_release(&self);
	_udi_instance_attr_release(&child1);
	_udi_instance_attr_release(&child2);

/* dump of the backing store */
	_backing_store_dump(&self);
	_backing_store_dump(&child1);
	_backing_store_dump(&child2);

/* the persistent attributes should still be here */
	attr_bool = FALSE;
	act_length =
		udi_posix_instance_attr_get(&self, "%self_attr_boolean", 0,
					    (void *)&attr_bool,
					    sizeof (udi_boolean_t),
					    &attr_type);
	udi_assert(act_length == sizeof (udi_boolean_t) &&
		   attr_type == UDI_ATTR_BOOLEAN && attr_bool == TRUE);

	attr_bool = TRUE;
	act_length =
		udi_posix_instance_attr_get(&self, "@pvis_attr_boolean", 1,
					    (void *)&attr_bool,
					    sizeof (udi_boolean_t),
					    &attr_type);
	udi_assert(act_length == sizeof (udi_boolean_t) &&
		   attr_type == UDI_ATTR_BOOLEAN && attr_bool == FALSE);

/* the others should not */
	attr_bool = FALSE;
	act_length =
		udi_posix_instance_attr_get(&child2, "^sibling_attr_boolean",
					    1, (void *)&attr_bool,
					    sizeof (udi_boolean_t),
					    &attr_type);
	udi_assert(act_length == 0);

	if (fname) {
		FILE *f = fopen(fname, "r");
		char buf[1024] = "%";

		if (f == NULL) {
			fprintf(stderr, "Error opening %s: %s\n",
				fname, strerror(errno));
			exit(1);
		}
		while (fgets(&buf[1], sizeof (buf) - 1, f)) {
			buf[strlen(buf) - 1] = 0;
			status = udi_posix_instance_attr_set(&self, buf, 1,
							     &buf[1],
							     strlen(buf) + 1,
							     UDI_ATTR_STRING);
			printf("%d<%s>\n", status, buf);

		}
		_backing_store_dump(&self);
	}

	_deinit_node(&self);
	_deinit_node(&child1);
	_deinit_node(&child2);
	posix_deinit();
	assert(Allocd_mem == 0);

	printf("Exiting");
	return EXIT_SUCCESS;
}
