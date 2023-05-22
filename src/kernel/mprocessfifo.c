/**
 * A wrapper of FIFO32 used in multitasking
 *   - When data enqueue, add the task back to scheduler list, if it was asleep
 */
#include "kernel/mprocessfifo.h"
#include "util/fifo.h"
#include "status.h"

void mpfifo32_init(MPFIFO32 *f, int32_t *buf, int32_t size, TASK *task)
{
	if (!f)
		return;
	if (!buf)
		return;
	fifo32_init(&f->fifo32, buf, size);
	f->task = task;
	return;
}

int32_t mpfifo32_enqueue(MPFIFO32 *f, int32_t data)
{
	if (!f)
		return -EIO;
	const int32_t res = fifo32_enqueue(&f->fifo32, data);
	TASK *t = f->task;
	if (!t)
		return res;
	/* Should be asleep, awaiting awakening */
	if (t->flags == MPROCESS_FLAGS_ALLOCATED)
		mprocess_task_run(t, -1, 0); /* Keep the priority unchanged */
	return res;
}

int32_t mpfifo32_dequeue(MPFIFO32 *f)
{
	return fifo32_dequeue(&f->fifo32);
}

int32_t mpfifo32_peek(const MPFIFO32 *f)
{
	return fifo32_peek(&f->fifo32);
}

int32_t mpfifo32_status_getUsageB(const MPFIFO32 *f)
{
	return fifo32_status_getUsageB(&f->fifo32);
}

