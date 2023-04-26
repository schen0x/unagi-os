#include "fifo.h"
#include "status.h"
#include <stdint.h>

void fifo8_init(FIFO8 *fifo, uint8_t *buf, int32_t size)
{
	fifo->size = size;
	fifo->buf = buf;
	fifo->free = size;
	fifo->flags = 0;
	fifo->next_w = 0;
	fifo->next_r = 0;
	return;
}

/*
 * Write 1 byte to FIFO buffer
 * Return -EIO if fail, 0 if success.
 */
int32_t fifo8_enqueue(FIFO8 *fifo, uint8_t data)
{
	if (fifo->free == 0)
	{
		fifo->flags |= FIFO8_FLAG_OVERRUN; // TODO is this flag really necessary?
		return -EIO;
	}
	fifo->buf[fifo->next_w] = data;
	fifo->next_w++;
	// Warp to the head of the buffer
	if (fifo->next_w == fifo->size)
	{
		fifo->next_w = 0;
	}
	fifo->free--;
	return 0;
}

/*
 * Read 1 byte to FIFO buffer
 * Return -EIO if fail (if success data is uint8_t, must be positive)
 */
int32_t fifo8_dequeue(FIFO8 *fifo)
{
	int32_t data;
	if (fifo->free == fifo->size)
	{
		return -EIO;
	}
	data = fifo->buf[fifo->next_r];
	fifo->next_r++;
	// Warp to the head of the buffer
	if (fifo->next_r == fifo->size)
	{
		fifo->next_r = 0;
	}
	fifo->free++;
	return data;
}

/*
 * Return the usage of the buffer in Bytes
 */
int32_t fifo8_status_getUsageB(FIFO8 *fifo)
{
	int32_t usage_in_bytes = 0;
	usage_in_bytes = fifo->size - fifo->free;
	return usage_in_bytes;
}

