#ifndef UTIL_FIFO_H_
#define UTIL_FIFO_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#define FIFO8_FLAG_OVERRUN 0x1

/* FIFO buffer header */
typedef struct FIFO8 {
	uint8_t *buf;
	/* size: *buf capacity in bytes */
	int32_t size;
	/* free: available bytes remaining */
	int32_t free;
	/* next_r/w: next position to read/write */
	int32_t next_r, next_w;
	/* flags: 0x1 is FIFO8_FLAG_OVERRUN  */
	int32_t flags;
} FIFO8;
void fifo8_init(FIFO8 *fifo, uint8_t *buf, int32_t size);
int32_t fifo8_enqueue(FIFO8 *fifo, uint8_t data);
int32_t fifo8_dequeue(FIFO8 *fifo);
int32_t fifo8_status_getUsageB(FIFO8 *fifo);

#endif
