// SPDX-License-Identifier: MIT
/* Create one linear mapper for the extent already validated by init. */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/dm-ioctl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define NAME "thinksmart-overlay"
#define UUID "OMAMOBILE-THINKSMART-OVERLAY"

struct table {
	struct dm_ioctl header;
	struct dm_target_spec target;
	char parameters[80];
};

static void prepare(struct dm_ioctl *header, unsigned size)
{
	memset(header, 0, size);
	header->version[0] = DM_VERSION_MAJOR;
	header->data_size = size;
	header->data_start = sizeof(*header);
	strcpy(header->name, NAME);
}

static int number(const char *text, char **end, unsigned long long *value)
{
	if (!isdigit((unsigned char)*text))
		return -1;
	errno = 0;
	*value = strtoull(text, end, 10);
	return errno ? -1 : 0;
}

int main(int argc, char **argv)
{
	unsigned long long major, minor, start, sectors;
	struct dm_ioctl header;
	struct table table;
	char *end;
	int fd;

	if (argc != 4 || number(argv[1], &end, &major) || *end != ':' ||
	    number(end + 1, &end, &minor) || *end ||
	    number(argv[2], &end, &start) || *end ||
	    number(argv[3], &end, &sectors) || *end ||
	    major > 4095 || minor > 1048575 || !start || !sectors ||
	    start > 0xffffffffULL || sectors > 0xffffffffULL - start) {
		fprintf(stderr, "usage: thinksmart-map MAJOR:MINOR START SECTORS\n");
		return 1;
	}

	fd = open("/dev/mapper/control", O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		perror("/dev/mapper/control");
		return 1;
	}
	prepare(&header, sizeof(header));
	strcpy(header.uuid, UUID);
	if (ioctl(fd, DM_DEV_CREATE, &header) < 0) {
		perror("DM_DEV_CREATE");
		close(fd);
		return 1;
	}

	prepare(&table.header, sizeof(table));
	table.header.target_count = 1;
	table.header.data_start = offsetof(struct table, target);
	table.target.length = sectors;
	table.target.next = sizeof(table) - offsetof(struct table, target);
	strcpy(table.target.target_type, "linear");
	snprintf(table.parameters, sizeof(table.parameters), "%llu:%llu %llu",
		 major, minor, start);
	if (ioctl(fd, DM_TABLE_LOAD, &table) < 0) {
		perror("DM_TABLE_LOAD");
		goto remove;
	}
	prepare(&header, sizeof(header));
	if (ioctl(fd, DM_DEV_SUSPEND, &header) < 0) {
		perror("DM_DEV_SUSPEND (resume)");
		goto remove;
	}
	close(fd);
	return 0;

remove:
	prepare(&header, sizeof(header));
	if (ioctl(fd, DM_DEV_REMOVE, &header) < 0)
		perror("DM_DEV_REMOVE");
	close(fd);
	return 1;
}
