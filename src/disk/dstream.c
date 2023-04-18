#include "disk/dstream.h"
#include "memory/memory.h"
#include <stddef.h>
#include <stdint.h>
#include "status.h"
#include "config.h"

DISK_STREAM* dstream_new(int32_t disk_id)
{
	DISK* disk = disk_get(disk_id);
	if (!disk)
	{
		return NULL;
	}

	DISK_STREAM* disk_stream = kzalloc(sizeof(DISK_STREAM));
	disk_stream->pos = 0;
	disk_stream->disk = disk;
	return disk_stream;
}

/*
 * [IN, OUT] stream
 * [IN] pos
 */
int32_t dstream_seek(DISK_STREAM* stream, int32_t pos)
{
	if (!stream)
		return -EIO;
	stream->pos = pos;
	return 0;
}

/*
 * A disk_read_block wrapper
 * FIXME Large read cause Stack Overflow
 * TODO This is not actually a streamer because a streamer should allow pipelining.
 * Only read bytes specified to the *out instead of multiplies of OS_DISK_SECTOR_SIZE
 */
int32_t dstream_read(DISK_STREAM* dstream, void* out, int32_t total)
{
	// i.e., the lba
	int32_t sector_start = dstream->pos / OS_DISK_SECTOR_SIZE;
	// O(1)
	int32_t offset_start = dstream->pos % OS_DISK_SECTOR_SIZE;
	// Read a block
	char buf[OS_DISK_SECTOR_SIZE];
	int32_t res = disk_read_block(dstream->disk, sector_start, 1, buf);
	if (res < 0)
		return -EIO;
	// If pos + total < 1 block, then all bytes are already in the buf
	// Otherwise, take the buf and next
	int32_t bytes_to_read = (offset_start + total) < OS_DISK_SECTOR_SIZE ? total : (OS_DISK_SECTOR_SIZE - offset_start);

	for(int32_t i = 0; i < bytes_to_read; i++)
	{
		*(char*)out++ = buf[offset_start + i];
	}

	dstream->pos += bytes_to_read;

	// If more is needed
	if (total > bytes_to_read)
	{
		res = dstream_read(dstream, out, total - bytes_to_read);
	}
	return 0;
}
