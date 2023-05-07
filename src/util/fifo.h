#ifndef UTIL_FIFO_H_
#define UTIL_FIFO_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#define FIFO8_FLAG_OVERRUN 0x1

/* FIFO buffer header */
typedef struct FIFO32 {
	int32_t *buf;
	/* size: *buf capacity in bytes */
	int32_t size;
	/* free: available bytes remaining */
	int32_t free;
	/* next_r/w: next position to read/write */
	int32_t next_r, next_w;
	/* flags: 0x1 is FIFO8_FLAG_OVERRUN  */
	int32_t flags;
} FIFO32;
void fifo32_init(FIFO32 *fifo, int32_t *buf, int32_t size);
int32_t fifo32_enqueue(FIFO32 *fifo, int32_t data);
int32_t fifo32_dequeue(FIFO32 *fifo);
int32_t fifo8_status_getUsageB(FIFO32 *fifo);
bool test_fifo8(void);

#endif
