#ifndef KERNEL_MPROCESSFIFO_H_
#define KERNEL_MPROCESSFIFO_H_

#include <stdint.h>
#include <stdbool.h>
#include "util/fifo.h"
#include "kernel/process.h"

typedef struct MPFIFO32 {
	FIFO32 fifo32;
	TASK *task;
} MPFIFO32;

void mpfifo32_init(MPFIFO32 *f, int32_t *buf, int32_t size, TASK *task);
int32_t mpfifo32_enqueue(MPFIFO32 *f, int32_t data);
int32_t mpfifo32_dequeue(MPFIFO32 *f);
int32_t mpfifo32_peek(const MPFIFO32 *f);
int32_t mpfifo32_status_getUsageB(const MPFIFO32 *f);

#endif
