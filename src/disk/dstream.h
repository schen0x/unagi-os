#ifndef DISK_DSTREAM_H_
#define DISK_DSTREAM_H_

#include "disk/disk.h"
#include <stdint.h>

typedef struct DISK_STREAM
{
	/* the byte position on the disk */
	int32_t pos;
	DISK* disk;
} DISK_STREAM;

DISK_STREAM* dstream_new(int32_t disk_id);
int32_t dstream_seek(DISK_STREAM* stream, int32_t pos);
int32_t dstream_read(DISK_STREAM* dstream, void* out, int32_t total);
#endif
