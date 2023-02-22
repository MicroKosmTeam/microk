#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <udi_env.h>

#if 0
void _udi_driver_load(){}
void _udi_driver_unload(){}
void _udi_meta_load(){}
void _udi_meta_unload(){}
void _udi_mapper_load(){}
void _udi_mapper_unload(){}
#endif

void
_udi_dev_node_init(
	_udi_driver_t *driver,	   /* New udi node */
	_udi_dev_node_t *udi_node)      /* Parent udi node */
{
	_OSDEP_DEV_NODE_T *osinfo = &udi_node->node_osinfo;
	unsigned int vendor_id, device_id, svendor_id , sdevice_id;
	udi_instance_attr_type_t attr_type;

#if 0
	if (driver == NULL) {
		osinfo->key = 0;;
		return;
	}
#endif

	if (posix_dev_node_init(driver, udi_node)) return;

	vendor_id = device_id = svendor_id = sdevice_id = 0;

        _udi_instance_attr_get(udi_node, "pci_device_id",
			&device_id, sizeof(udi_ubit32_t), &attr_type);

        _udi_instance_attr_get(udi_node, "pci_vendor_id",
			&vendor_id, sizeof(udi_ubit32_t), &attr_type);

        _udi_instance_attr_get(udi_node, "pci_subsystem_vendor_id",
			&svendor_id, sizeof(udi_ubit32_t), &attr_type);

        _udi_instance_attr_get(udi_node, "pci_subsystem_id",
			&sdevice_id, sizeof(udi_ubit32_t), &attr_type);

	return;
}

char *
_udi_msgfile_search (const char *filename, udi_ubit32_t msgnum,
			const char *locale)
{
	int fd, found;
	struct stat msgstat;
	char *buf, *ptr;
	udi_size_t buf_size;
	char *offset1, *offset2;
	int stage;

	/* Open up the message file, and read it into memory */
	if ((fd = open(filename, O_RDONLY)) < 0) {
		printf("Unable to open message file: %s!\n", filename);
		return (NULL);
	}

	fstat(fd, &msgstat);

	buf = _OSDEP_MEM_ALLOC(msgstat.st_size+1, UDI_MEM_NOZERO, UDI_WAITOK);

	if ((buf_size = read(fd, buf, msgstat.st_size)) < 0) {
		printf("Unable to read message file: %s!\n", filename);
		close(fd);
		return (NULL);
	}
	close(fd);
	if (buf_size == 0) return (NULL);
	*(buf+buf_size) = '\0';

	/* Parse the buffer for a match */
	found = stage = 0;
	ptr = buf;
	while(ptr-buf < buf_size) {
		if (udi_strncmp(ptr, "locale ", 7) == 0) {
			/* Found a locale */
			offset1 = ptr+7;
			offset2 = offset1;
			while (*offset2 != '\0' && *offset2 != '\n')
				offset2++;
			if (offset2-offset1 == udi_strlen(locale) &&
					udi_strncmp(locale, offset1,
						udi_strlen(locale)) == 0)
				stage = 1;
			else
				stage = 0;
		}
		if ((stage == 1 || udi_strcmp(locale, "C") == 0) &&
				udi_strncmp(ptr, "message ", 8) == 0 ||
				udi_strncmp(ptr, "disaster_message ", 17)
				== 0) {
			/* Found a message */
			/* Point to a message number */
			if (*ptr == 'm')
				offset1 = ptr+8;
			else
				offset1 = ptr+17;
			/* Try to match the message number */
			if (msgnum == udi_strtou32(offset1, NULL, 10)) {
				/* Go to the start of the message */
				while (*offset1 != ' ') offset1++;
				offset1++;
				/* Find the end of the message */
				offset2 = offset1;
				while (*offset2 != '\n' && *offset2 != '\0')
					offset2++;
				found = 1;
				break;
			}
		}
		while (*ptr != '\n' && ptr < buf+buf_size) ptr++;
		if (*ptr == '\n') ptr++;
	}


	if (! found) {
		_OSDEP_MEM_FREE(buf);
		return (NULL);
	}

	/* Allocate some memory, and copy the string to return it */
	ptr = _OSDEP_MEM_ALLOC(offset2-(offset1-1), UDI_MEM_NOZERO, UDI_WAITOK);
	*(ptr+(offset2-offset1)) = '\0';
	udi_memcpy(ptr, offset1, offset2-offset1);

	/* Free the buffer memory */
	_OSDEP_MEM_FREE(buf);

	return (ptr);
}

void
_udi_readfile_getdata (const char *filename, udi_size_t offset, char *buf,
			udi_size_t buf_len, udi_size_t *act_len)
{
	int fd, buf_size;
	struct stat msgstat;

	/* Open up the readable file, and read it into memory */
	if ((fd = open(filename, O_RDONLY)) < 0) {
		printf("Unable to open readable file: %s!\n", filename);
		*act_len = 0;
		return;
	}

	fstat(fd, &msgstat);

	if (msgstat.st_size < offset) {
		*act_len = 0;
		close(fd);
		return;
	}

	/* Read in data */
	if (buf_len > msgstat.st_size - offset)
		buf_len = (msgstat.st_size - offset);
	if (lseek(fd, offset, SEEK_SET) == -1) {
		*act_len = 0;
		close(fd);
		return;
	}
	if ((buf_size = read(fd, buf, buf_len)) < 0) {
		printf("Unable to read readable file: %s!\n", filename);
		*act_len = 0;
		close(fd);
		return;
	}

	*act_len = msgstat.st_size - offset;
	return;
}


int pthread_mutexattr_gettype(pthread_mutexattr_t *attr, int *type)
{
	return -1;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
	return -1;
}
