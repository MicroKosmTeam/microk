
/*
 * File: env/common/inst_attr.c
 *
 * UDI Instance Attribute Management.
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

_udi_dev_node_t *
_udi_get_child_node(_udi_dev_node_t *parent,
		    udi_ubit32_t child_ID)
{

	udi_queue_t *head, *elem, *tmp;
	_udi_dev_node_t *child;

	child = NULL;
	head = &(parent->child_list);
	UDI_QUEUE_FOREACH(head, elem, tmp) {
		child = UDI_BASE_STRUCT(elem, _udi_dev_node_t,
					sibling);

		if (child->my_child_id == child_ID) {
			return child;
		}
	}
	_OSDEP_ASSERT(0);
}

/**********************************************************************
 * Search a node attribute list to match a name.  If found, set
 * *found_it to TRUE and return the entry's address.  If not found,
 * set *found_it to FALSE and return the address of the next entry
 * in the sorted list beyond where the desired one would have been
 * (or NULL if it would have been at the end of the list).
 * This is predcated upon items being sorted when added to the list.
 *********************************************************************/

STATIC _udi_node_attr_entry_t *
_udi_node_attr_find_name(_udi_dev_node_t *node,
			 const char *name,
			 udi_boolean_t *found_it)
{
	extern int _udi_toupper(int c);
	udi_queue_t *listp = &(node->attribute_list);
	udi_queue_t *element, *tmp;
	_udi_node_attr_entry_t *entry;
	udi_sbit32_t cmpval;
	char *buf1;
	const char *buf2;

	UDI_QUEUE_FOREACH(listp, element, tmp) {

		entry = (_udi_node_attr_entry_t *) element;
		buf1 = entry->name;
		buf2 = name;
		while (((cmpval = _udi_toupper(*buf1) - _udi_toupper(*buf2)) ==
			0) && *buf1 != 0) {
			buf1++;
			buf2++;
		}

		*found_it = (cmpval == 0);
		if (cmpval == 0)
			return entry;
	}

	*found_it = FALSE;
	return NULL;
}


/**********************************************************************
 * Get an attribute from the UDI node.
 *********************************************************************/

STATIC void
_udi_instance_attr_async_get(_udi_cb_t *cb)
{

	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	_udi_dev_node_t *node = allocm->_u.instance_attr_get_request.node;
	udi_instance_attr_get_call_t *callback =
		allocm->_u.instance_attr_get_request.callback;
	const char *name = allocm->_u.instance_attr_get_request.name;
	void *value = allocm->_u.instance_attr_get_request.value;
	udi_size_t length = allocm->_u.instance_attr_get_request.length;
	udi_boolean_t found_it;
	_udi_node_attr_entry_t *attr_entry;
	udi_size_t attr_length = 0;
	udi_time_t interval;

	if (udi_strchr(name, ':') != NULL) {
		_udi_osdep_sprops_get_rf_data(node->node_mgmt_channel->
					      other_end->chan_region->
					      reg_driver, name, value, length,
					      &attr_length);
		SET_RESULT_UDI_CT_INSTANCE_ATTR_GET(allocm, UDI_ATTR_FILE,
						    attr_length);
		_UDI_IMMED_CALLBACK(cb, _UDI_CT_INSTANCE_ATTR_GET,
				    &_udi_alloc_ops_nop, callback, 2,
				    (UDI_ATTR_FILE, attr_length));
		return;
	}

	_OSDEP_MUTEX_LOCK(&node->lock);
	if (node->read_in_progress < 0) {
		/*
		 * write in progress or pending try again later
		 */
		_OSDEP_MUTEX_UNLOCK(&node->lock);
		interval.nanoseconds = 0;
		interval.seconds = 1;
		_OSDEP_TIMER_START(_udi_instance_attr_async_get,
				   cb, _OSDEP_TIME_T_TO_TICKS(interval),
				   FALSE);
		return;
	}
	node->read_in_progress++;
	_OSDEP_MUTEX_UNLOCK(&node->lock);
	/*
	 * someone is reading it or no one is touching it
	 * either case we a ahead and read
	 */
	attr_entry = _udi_node_attr_find_name(node, name, &found_it);

	/*
	 * If we found it, copy as much as we can to the dest buffer.
	 */
	if (found_it) {
		udi_size_t len = attr_entry->value_size < length ?
			attr_entry->value_size : length;

		udi_memcpy(value, attr_entry->value, len);
		SET_RESULT_UDI_CT_INSTANCE_ATTR_GET(allocm,
						    attr_entry->attr_type,
						    attr_entry->value_size);
	} else if (name[0] == '%' || name[0] == '@') {
		udi_instance_attr_type_t attr_type;

		/*
		 * try the backing store 
		 */
		attr_length =
			_OSDEP_INSTANCE_ATTR_GET(node, name, value, length,
						 &attr_type);
		SET_RESULT_UDI_CT_INSTANCE_ATTR_GET(allocm,
						    attr_type, attr_length);
	} else {
		SET_RESULT_UDI_CT_INSTANCE_ATTR_GET(allocm, UDI_ATTR_NONE, 0);
	}
	_OSDEP_MUTEX_LOCK(&node->lock);
	UDIENV_CHECK(instance_attr_invalid_state, node->read_in_progress != 0);
	if (node->read_in_progress < 0) {
		/*
		 * write is pending
		 */
		node->read_in_progress++;
	} else {
		/*
		 * no write pending
		 */
		node->read_in_progress--;
	}
	_OSDEP_MUTEX_UNLOCK(&node->lock);
	/*
	 * By this point, we're a little late to detect memory overwrites,
	 * but at least we've rendevoused back into common code.  Validate
	 * that the sizes returned by the OS-specific get attr for this type
	 * and the size of the buffer that the user handed us are sensible.
	 */
	switch(allocm->_u.instance_attr_get_result.attr_type) {
		case UDI_ATTR_UBIT32:
			_OSDEP_ASSERT(allocm->_u.instance_attr_get_result.
				actual_length == sizeof(udi_ubit32_t));
			_OSDEP_ASSERT(length >= sizeof(udi_ubit32_t));
			break;
		case UDI_ATTR_BOOLEAN:
			_OSDEP_ASSERT(allocm->_u.instance_attr_get_result.
				actual_length == sizeof(udi_boolean_t));
			_OSDEP_ASSERT(length >= sizeof(udi_boolean_t));
			break;
		case UDI_ATTR_NONE:
			_OSDEP_ASSERT(allocm->_u.instance_attr_get_result.
				actual_length == 0);
			break;
		default:
			break;

	}
	_UDI_IMMED_CALLBACK(cb, _UDI_CT_INSTANCE_ATTR_GET,
			    &_udi_alloc_ops_nop, callback, 2,
			    (allocm->_u.instance_attr_get_result.attr_type,
			     allocm->_u.instance_attr_get_result.
			     actual_length));
}

/*
 * env interface to obtain instance attr.  this is a sync call.
 * this call also bypasses "name redirection" the attritbute is
 * search on the given node regardless of name prefix.
 * ie: a search on "^sibling_attr" which normally should be searched
 * on the parent node, but this function will search on the node
 * being passed in.
 */
STATIC void
_udi_inst_call_get_back(udi_cb_t *gcb,
			udi_instance_attr_type_t type,
			udi_size_t size)
{

	_OSDEP_EVENT_SIGNAL(&(*(_OSDEP_EVENT_T *)gcb->initiator_context));
}

/*
 * Instance attribute values are returned in native endian.
 */
udi_size_t
_udi_instance_attr_get(_udi_dev_node_t *node,
		       const char *name,
		       void *value,
		       udi_size_t length,
		       udi_instance_attr_type_t *attr_type)
{

	udi_MA_cb_t *MA_cb = _udi_MA_cb_alloc();
	udi_cb_t *gcb = UDI_GCB(&MA_cb->v.m);
	_udi_cb_t *new_cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &new_cb->cb_allocm;
	_OSDEP_EVENT_T event;
	udi_size_t actual_size;

	_OSDEP_EVENT_INIT(&event);
	allocm->_u.instance_attr_get_request.node = node;
	allocm->_u.instance_attr_get_request.callback =
		_udi_inst_call_get_back;
	allocm->_u.instance_attr_get_request.name = name;
	allocm->_u.instance_attr_get_request.value = value;
	allocm->_u.instance_attr_get_request.length = length;
	gcb->initiator_context = (void *)&event;
	_udi_instance_attr_async_get(new_cb);
	_OSDEP_EVENT_WAIT(&event);
	_OSDEP_EVENT_DEINIT(&event);
	*attr_type = allocm->_u.instance_attr_get_result.attr_type;
	actual_size = allocm->_u.instance_attr_get_result.actual_length;
	udi_cb_free(gcb);
	return actual_size;
}

/* udi_instance_attr_get
 *
 * Public interface to obtain an instance attr value for the
 * corresponding node.
 */

void
udi_instance_attr_get(udi_instance_attr_get_call_t *callback,
		      udi_cb_t *gcb,
		      const char *attr_name,
		      udi_ubit32_t child_ID,
		      void *attr_value,
		      udi_size_t attr_length)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_dev_node_t *node = _UDI_GCB_TO_CHAN(gcb)->chan_region->reg_node;
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;

	switch (attr_name[0]) {
	case '^':
		node = node->parent_node;
		break;
	case '@':
		node = _udi_get_child_node(node, child_ID);
	}

	allocm->_u.instance_attr_get_request.node = node;
	allocm->_u.instance_attr_get_request.callback = callback;
	allocm->_u.instance_attr_get_request.name = attr_name;
	allocm->_u.instance_attr_get_request.value = attr_value;
	allocm->_u.instance_attr_get_request.length = attr_length;

	_udi_instance_attr_async_get(cb);
}

/* 
 * actually alloc mem and insert attr into attribute list 
 * read_in_progress is to be set properly prior to entering this function
 */
STATIC udi_status_t
_udi_inst_attr_set(_udi_cb_t *cb)
{

	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	_udi_dev_node_t *node = allocm->_u.instance_attr_set_request.node;
	const char *name = allocm->_u.instance_attr_set_request.name;
	const void *value = allocm->_u.instance_attr_set_request.value;
	udi_size_t length = allocm->_u.instance_attr_set_request.length;
	udi_instance_attr_type_t attr_type =
		allocm->_u.instance_attr_set_request.attr_type;
	udi_boolean_t persistent =
		allocm->_u.instance_attr_set_request.persistent;
	udi_boolean_t found_it;
	_udi_node_attr_entry_t *attr_entry;
	udi_status_t ret_val = UDI_OK;

	/*
	 * Try to find the named attribute.
	 * Lock the node for the duration of the search.
	 */
	attr_entry = _udi_node_attr_find_name(node, name, &found_it);

	/*
	 * let's try backing store first, if it fails, then don't
	 * change the info in memory so that things are consistent
	 */
	if (persistent &&
	    (ret_val = _OSDEP_INSTANCE_ATTR_SET(node, name, value,
						length,
						attr_type)) != UDI_OK) {
		return ret_val;
	}
	/*
	 * Found it.
	 */
	if (found_it) {
		if (length == 0) {
			UDI_QUEUE_REMOVE(&attr_entry->link);
			_OSDEP_MEM_FREE(attr_entry->value);
			_OSDEP_MEM_FREE(attr_entry);
			return UDI_OK;
		}
		/*
		 * allocate the new size 
		 */
		_OSDEP_MEM_FREE(attr_entry->value);
		attr_entry->value = _OSDEP_MEM_ALLOC(length, 0, UDI_WAITOK);
		udi_memcpy(attr_entry->value, value, length);
		attr_entry->value_size = length;
		attr_entry->attr_type = attr_type;
	}
	/*
	 * Attribute doesn't exist.  Create attribute entry and INSERT it
	 * in the attribute list before 'attr_entry'.  This will keep the
	 * attribute list lexigraphically sorted by "name".  If 'attr_entry'
	 * is NULL, append the new attribute to the end of the list.
	 */
	else {
		_udi_node_attr_entry_t *new_entry;
		udi_size_t name_size = udi_strlen(name) + 1;

		/*
		 * First allocate storage for the _udi_node_attr_entry_t 
		 * and the 'name'.
		 */
		new_entry = _OSDEP_MEM_ALLOC(sizeof (_udi_node_attr_entry_t) +
					     name_size, 0, UDI_WAITOK);
		new_entry->value = _OSDEP_MEM_ALLOC(length, 0, UDI_WAITOK);
		new_entry->name = (char *)(new_entry + 1);

		/*
		 * Initialize the new attribute structure.
		 */
		udi_memcpy(new_entry->name, name, name_size);
		new_entry->name_size = name_size;
		udi_memcpy(new_entry->value, value, length);
		new_entry->value_size = length;
		new_entry->attr_type = attr_type;

		/*
		 * Add the new entry to the list, keeping the list 
		 * lexigraphically sorted.
		 */
		if (attr_entry) {
			UDI_QUEUE_INSERT_BEFORE(&(attr_entry->link),
						&(new_entry->link));
		} else {
			UDI_ENQUEUE_TAIL(&(node->attribute_list),
					 &(new_entry->link));
		}
	}
	return UDI_OK;
}


/* serialize the operation */
void
_udi_instance_attr_async_set(_udi_cb_t *cb)
{

	udi_time_t interval;
	udi_status_t status;
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	_udi_dev_node_t *node = allocm->_u.instance_attr_set_request.node;

	_OSDEP_MUTEX_LOCK(&node->lock);
	if (node->read_in_progress == 0) {
		node->read_in_progress = -1;
		_OSDEP_MUTEX_UNLOCK(&node->lock);
		status = _udi_inst_attr_set(cb);
		_OSDEP_MUTEX_LOCK(&node->lock);
		UDIENV_CHECK(instance_attribute_invalid_state,
			     node->read_in_progress == -1);
		node->read_in_progress = 0;
		_OSDEP_MUTEX_UNLOCK(&node->lock);
		SET_RESULT_UDI_CT_INSTANCE_ATTR_SET(allocm, status);
		_UDI_IMMED_CALLBACK(cb, _UDI_CT_INSTANCE_ATTR_SET,
				    &_udi_alloc_ops_nop,
				    allocm->_u.instance_attr_set_request.
				    callback, 1, (status))
			return;
	}
	/*
	 * > 0  means read(s) in progress so negate and wait for it to 
	 * get down to 0.  < 0 means write in progress so just wait for
	 * it to get to 0
	 */
	if (node->read_in_progress > 0) {
		node->read_in_progress *= -1;
	}
	interval.nanoseconds = 0;
	interval.seconds = 1;
	_OSDEP_TIMER_START(_udi_instance_attr_async_set,
			   cb, _OSDEP_TIME_T_TO_TICKS(interval), FALSE);
}


/*
 * env interface to obtain instance attr.  this is a sync call.
 * this call also bypasses "name redirection" the attritbute is
 * search on the given node regardless of name prefix.
 * ie: a search on "^sibling_attr" which normally should be searched
 * on the parent node, but this function will search on the node
 * being passed in.
 */
void
_udi_inst_call_set_back(udi_cb_t *gcb,
			udi_status_t status)
{

	_OSDEP_EVENT_SIGNAL(&(*(_OSDEP_EVENT_T *)gcb->initiator_context));
}

udi_status_t
_udi_instance_attr_set(_udi_dev_node_t *node,
		       const char *name,
		       const void *value,
		       udi_size_t length,
		       udi_instance_attr_type_t attr_type,
		       udi_boolean_t persistent)
{

	udi_MA_cb_t *MA_cb = _udi_MA_cb_alloc();
	udi_cb_t *gcb = UDI_GCB(&MA_cb->v.m);
	_udi_cb_t *new_cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &new_cb->cb_allocm;
	_OSDEP_EVENT_T event;
	udi_status_t status;

	_OSDEP_EVENT_INIT(&event);
	allocm->_u.instance_attr_set_request.node = node;
	allocm->_u.instance_attr_set_request.callback =
		_udi_inst_call_set_back;
	allocm->_u.instance_attr_set_request.name = name;
	allocm->_u.instance_attr_set_request.value = value;
	allocm->_u.instance_attr_set_request.length = length;
	allocm->_u.instance_attr_set_request.attr_type = attr_type;
	allocm->_u.instance_attr_set_request.persistent = persistent;
	gcb->initiator_context = (void *)&event;
	_udi_instance_attr_async_set(new_cb);

	/*
	 * Copy some of the cornerstone generic attributes into the dev node
	 * for rapid access by the tracing/logging code.
	 * Since we're about to fall into  _udi_inst_attr_set() and it 
	 * calls _OSDEP_MEM_ALLOC with WAITOK, we're hoping that we're in
	 * an appropriate context.
	 */
	if (udi_strcmp(name, "physical_locator") == 0) {
		if (node->attr_physical_locator) {
			_OSDEP_MEM_FREE(node->attr_physical_locator);
		}
		node->attr_physical_locator = _OSDEP_MEM_ALLOC(length + 1, 0,
							       UDI_WAITOK);
		udi_strcpy(node->attr_physical_locator, value);
	} else if (udi_strcmp(name, "address_locator") == 0) {
		if (node->attr_address_locator) {
			_OSDEP_MEM_FREE(node->attr_address_locator);
		}
		node->attr_address_locator = _OSDEP_MEM_ALLOC(length + 1, 0,
							      UDI_WAITOK);
		udi_strcpy(node->attr_address_locator, value);
	}
	_OSDEP_EVENT_WAIT(&event);
	_OSDEP_EVENT_DEINIT(&event);
	status = allocm->_u.instance_attr_set_result.status;
	udi_cb_free(gcb);
	return status;
}

void
udi_instance_attr_set(udi_instance_attr_set_call_t *callback,
		      udi_cb_t *gcb,
		      const char *attr_name,
		      udi_ubit32_t child_ID,
		      const void *attr_value,
		      udi_size_t attr_length,
		      udi_ubit8_t attr_type)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_dev_node_t *node = _UDI_GCB_TO_CHAN(gcb)->chan_region->reg_node;
	udi_boolean_t persistent = FALSE;
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;

	/*
	 * Validate 'length' argument against spec.
	 */
	switch (attr_type) {
		case UDI_ATTR_NONE: 
			_OSDEP_ASSERT(attr_length == 0);
			break;
		case UDI_ATTR_FILE:
			/*
			 * ATTR_FILE is flatly illegal for attr_set. 
 			 */
			_OSDEP_ASSERT(attr_type != UDI_ATTR_FILE);
			break;
		case UDI_ATTR_BOOLEAN:
			_OSDEP_ASSERT(attr_length == sizeof(udi_boolean_t));
			break;
		case UDI_ATTR_UBIT32:
			_OSDEP_ASSERT(attr_length == sizeof(udi_ubit32_t));
			break;
		default:
			_OSDEP_ASSERT(attr_length > 0);
			break;
	}

	switch (attr_name[0]) {
	case '^':
		node = node->parent_node;
	case '$':
		break;
	case '%':
	case '@':
		persistent = TRUE;
		break;
	default:
		node = _udi_get_child_node(node, child_ID);
	}
	allocm->_u.instance_attr_set_request.node = node;
	allocm->_u.instance_attr_set_request.callback = callback;
	allocm->_u.instance_attr_set_request.name = attr_name;
	allocm->_u.instance_attr_set_request.value = attr_value;
	allocm->_u.instance_attr_set_request.length = attr_length;
	allocm->_u.instance_attr_set_request.attr_type = attr_type;
	allocm->_u.instance_attr_set_request.persistent = persistent;

	_udi_instance_attr_async_set(cb);
}


/* _udi_instance_attr_release
 *
 * Internal routine to call when cleaning up a node to release all
 * local instance attribute resources associated with that node
 * (typically reverses all _udi_instance_attr_set operations).
 */

void
_udi_instance_attr_release(_udi_dev_node_t *node)
{
	_udi_node_attr_entry_t *attr;
	udi_queue_t *elem, *tmp;

	UDI_QUEUE_FOREACH(&node->attribute_list, elem, tmp) {
		UDI_QUEUE_REMOVE(elem);
		attr = UDI_BASE_STRUCT(elem, _udi_node_attr_entry_t, link);
		_OSDEP_MEM_FREE(attr->value);
		_OSDEP_MEM_FREE(attr);
	}
}




/* _udi_attr_string_decode
 * 
 * This routine is used to translate the UDI_ATTR_STRING Escape
 * Sequences described in Table 31.2.
 *
 * This routine allocates the decode memory internally and passes it
 * back to the caller; the caller is responsible for deallocating the
 * memory.  When used with udi_instance_attr_get, the user already has
 * a buffer with a specified length, but since we are required to pass
 * back a valid attr_length regardless of the length of the user's
 * buffer we have to do the full translation anyhow, so we'll choose
 * to build it internally here and udi_instance_attr_get will have to
 * do a udi_memcpy/udi_strncpy on the result.
 */

#define MAX_DECODE 2000			/* see Table 31-2 UDI_ATTR_STRING Escape Seq. */

STATIC void
_udi_decode_escapes(_udi_driver_t *drv,
		    char *locale,
		    char *srcptr,
		    char *dstptr,
		    udi_size_t *dstlen,
		    int recurse_level)
{
	char *sp, *dp, *tp;

	dp = dstptr;
	for (sp = srcptr; *sp && *dstlen <= MAX_DECODE; sp++, (*dstlen)++) {
		if (*sp == '\\') {
			switch (*(++sp)) {
			case '_':
				*(dp++) = ' ';
				break;
			case 'H':
				*(dp++) = '#';
				break;
			case '\\':
				*(dp++) = *sp;
				break;
			case 'p':	/*OSDEP-specific... */

/* 		*(dp++) = '\n'; (*dstlen)++; */
				*(dp++) = '\n';
				break;
			case 'm':
				if (recurse_level >= 3) {
					*(dp++) = '\\';
					*(dp++) = 'm';
					*dstlen += 1;
				} else {
					/* 
					 * sp points to the 'm'  Advance past
					 * that to get the message #
					 */
					sp++;
					tp = _udi_osdep_sprops_ifind_message
						(drv, locale,
						 udi_strtou32(sp, &sp, 0));
					_udi_decode_escapes(drv, locale, tp,
							    dp, dstlen,
							    recurse_level + 1);
					_OSDEP_MEM_FREE(tp);
					dp = (dstptr + (*dstlen)--);
					sp--;
					continue;
				}
				break;
			default:
				*(dp++) = '\\';
				*(dp++) = *sp;
				(*dstlen)++;
				break;
			}
		} else
			*(dp++) = *sp;
	}
	*dp = 0;
}


/* _udi_attr_string_decode
 *
 * This routine may be called by the UDI environment to translate any
 * embedded escape sequences and other niceties into actual ASCII strings.
 *
 * Note that this routine allocates memory for the return string with
 * UDI_WAITOK.  The caller must not be holding any locks.  The caller
 * is also responsible for issuing _OSDEP_MEM_FREE on the returned
 * string when it's done with the string.
 */

char *
_udi_attr_string_decode(_udi_driver_t *drv,
			char *locale,
			char *orig_string)
{
	char *rstr;
	udi_size_t rlen;

	rstr = _OSDEP_MEM_ALLOC(MAX_DECODE, 0, UDI_WAITOK);
	rlen = 0;
	_udi_decode_escapes(drv, locale, orig_string, rstr, &rlen, 0);
	return rstr;
}
