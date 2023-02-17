
/*
 * commands
 */
#define UDID_INSTANCE_ATTR_GET 0
#define UDID_INSTANCE_ATTR_SET 1

/*
 * status values
 */
#define UDID_OK 0
#define UDID_NODE_NOT_FOUND 1		/* probably not a udi driver */
typedef struct {
	rm_key_t key;
	char name[UDI_MAX_ATTR_NAMELEN];
	void *value;
	udi_size_t length;
	udi_size_t actual_size;
	udi_instance_attr_type_t attr_type;
	udi_ubit32_t child_id;
	udi_status_t status;
} udid_instance_attr_t;
