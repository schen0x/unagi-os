#include "fifo.h"
#include "status.h"
#include <stdint.h>
#include <stdbool.h>

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


bool test_fifo8(void)
{
	FIFO8 f = {0};
	uint8_t _buf[8] = {0};
	fifo8_init(&f, _buf, sizeof(_buf));
	for (uint8_t i = 0x40; i < 0x48; i++)
	{
		fifo8_enqueue(&f, i);
		if (fifo8_status_getUsageB(&f) != (i - 0x40 + 1))
			return false;
	}
	for (uint8_t i = 0x40; i < 0x48; i++)
	{
		uint8_t d = fifo8_dequeue(&f);
		if (d != i)
			return false;
		if (fifo8_status_getUsageB(&f) != (0x48 - i - 1))
			return false;
	}
	// testing wrap loop
	for (uint8_t i = 0x40; i < 0x44; i++)
	{
		fifo8_enqueue(&f, i);
	}
	for (uint8_t i = 0x40; i < 0x43; i++)
	{
		fifo8_dequeue(&f);
	}
	for (uint8_t i = 0x44; i < 0x4b; i++)
	{
		fifo8_enqueue(&f, i);
		if (fifo8_status_getUsageB(&f) != (i - 0x43 + 1))
			return false;
	}
	for (uint8_t i = 0x43; i < 0x4b; i++)
	{
		uint8_t d = fifo8_dequeue(&f);
		if (d != i)
			return false;
	}
	return true;
}
