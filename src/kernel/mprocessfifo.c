/**
 * A wrapper of FIFO32 used in multitasking
 *   - When data enqueue, add the task back to scheduler list, if it was asleep
 */
#include "kernel/mprocessfifo.h"
#include "util/fifo.h"

void mpfifo32_init(MPFIFO32 *f, int32_t *buf, int32_t size, TASK *task)
{
	fifo32_init(&f->fifo32, buf, size);
	f->task = task;
	return;
}
/**
 * TODO This is different from the autoswitch
 */
int32_t mpfifo32_enqueue(MPFIFO32 *mpfifo32, int32_t data)
{
	const int32_t res = fifo32_enqueue(&mpfifo32->fifo32, data);
	if (res < 0)
		return res;
	if (mpfifo32 && mpfifo32->task && mpfifo32->task->flags == MPROCESS_FLAGS_ALLOCATED)
		mprocess_task_run(mpfifo32->task);
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

