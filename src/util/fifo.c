#include "fifo.h"
#include "status.h"
#include <stdint.h>
#include <stdbool.h>
#include "include/uapi/graphic.h"
#include "util/printf.h"

/*
 * Consider the Lifecycle of *buf carefully
 * @buf
 * @buflen the array length of the buf[] (sizeof(buf)/ sizeof(buf[0]))
 */
void fifo32_init(FIFO32 *fifo, int32_t *buf, int32_t buflen)
{
	fifo->buflen = buflen;
	fifo->buf = buf;
	fifo->free = buflen;
	fifo->flags = 0;
	fifo->next_w = 0;
	fifo->next_r = 0;
	return;
}

/*
 * Write 1 byte to FIFO buffer
 * Return -EIO if fail, 0 if success.
 */
int32_t fifo32_enqueue(FIFO32 *fifo, int32_t data)
{
	if (!fifo)
		return -EIO;
	if (fifo->free == 0)
	{
	// 	fifo->flags |= FIFO32_FLAG_OVERRUN; // TODO is this flag really necessary?
		return -EIO;
	}
	fifo->buf[fifo->next_w] = data;
	fifo->next_w++;
	// Warp to the head of the buffer
	if (fifo->next_w == fifo->buflen)
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
int32_t fifo32_dequeue(FIFO32 *fifo)
{
	if (!fifo)
		return -EIO;
	int32_t data;
	if (fifo->free == fifo->buflen)
	{
		return -EIO;
	}
	data = fifo->buf[fifo->next_r];
	fifo->next_r++;
	// Warp to the head of the buffer
	if (fifo->next_r == fifo->buflen)
	{
		fifo->next_r = 0;
	}
	fifo->free++;
	//char b[12] = {0};
	//sprintf(b, "dq:%2x", data);
	//kfprint(b, 4);
	return data;
}

/*
 * Peek 1 byte in FIFO buffer
 * Return the same value as fifo32_dequeue but do not modify
 */
int32_t fifo32_peek(const FIFO32 *fifo)
{
	int32_t data;
	if (fifo->free == fifo->buflen)
	{
		return -EIO;
	}
	data = fifo->buf[fifo->next_r];
	return data;
}

/*
 * Return the usage of the buffer in Bytes
 */
int32_t fifo32_status_getUsageB(FIFO32 *fifo)
{
	if (!fifo)
		return -EIO;
	int32_t usage_in_bytes = 0;
	usage_in_bytes = fifo->buflen - fifo->free;
	return usage_in_bytes;
}


bool test_fifo32(void)
{
	FIFO32 f = {0};
	int32_t _buf[8] = {0};
	fifo32_init(&f, _buf, sizeof(_buf)/sizeof(_buf[0]));
	for (uint8_t i = 0x40; i < 0x48; i++)
	{
		fifo32_enqueue(&f, i);
		if (fifo32_status_getUsageB(&f) != (i - 0x40 + 1))
			return false;
	}
	for (uint8_t i = 0x40; i < 0x48; i++)
	{
		uint8_t dp = fifo32_peek(&f);
		uint8_t d = fifo32_dequeue(&f);
		if (dp != i)
			return false;
		if (d != i)
			return false;
		if (fifo32_status_getUsageB(&f) != (0x48 - i - 1))
			return false;
	}
	// test null bytes
	for (uint8_t i = 0x40; i < 0x48; i++)
	{
		fifo32_enqueue(&f, 0);
		if (fifo32_status_getUsageB(&f) != (i - 0x40 + 1))
			return false;
	}
	for (uint8_t i = 0x40; i < 0x48; i++)
	{
		int32_t d = fifo32_dequeue(&f);
		if (d != 0)
			return false;
		if (fifo32_status_getUsageB(&f) != (0x48 - i - 1))
			return false;
	}
	// test wrap loop
	for (uint8_t i = 0x40; i < 0x44; i++)
	{
		fifo32_enqueue(&f, i);
	}
	for (uint8_t i = 0x40; i < 0x43; i++)
	{
		fifo32_dequeue(&f);
	}
	for (uint8_t i = 0x44; i < 0x4b; i++)
	{
		fifo32_enqueue(&f, i);
		if (fifo32_status_getUsageB(&f) != (i - 0x43 + 1))
			return false;
	}
	for (uint8_t i = 0x43; i < 0x4b; i++)
	{
		uint8_t dp = fifo32_peek(&f);
		uint8_t d = fifo32_dequeue(&f);
		if (dp != i)
			return false;
		if (d != i)
			return false;
	}
	return true;
}
