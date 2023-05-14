#include "pic/timer.h"
#include "config.h"
#include "io/io.h"
#include "memory/memory.h"
#include "util/printf.h"
/**
 * TODO use a double linked list etc. so that on setting timer or on triggering etc.
 * Keep a list in order so that no need to loop through all elements.
 *
 * The major Performance difference on the user side is when an interruption happens.
 * So by using a sorted linked list, theoretically next->next->next until all is triggered, no need to traverse all structures
 * But what if by using the simple list, cpu auto fetch and cached the whole list, so that IO cost is drastically reduced? (limit/architecture/implementation of caching)
 */

#define PIT_CNT0 0x0040
#define PIT_CTRL 0x0043

TIMERCTL timerctl;

/**
 * Get a 100Hz clock (10ms per tick)
 * Setup the Programmable Interval Timer (PIT) chip (Intel 8353/8254)
 * Set counter to 0x2e9c (11932) => (1.1931816666 MHz / 11932 = 99.99846 Hz)
 */
void pit_init(void)
{
	bool isCli = io_get_is_cli();
	if (!isCli)
		_io_cli();

	_io_out8(PIT_CTRL, 0x34); // channel 0, lobyte/hibyte, rate generator
	_io_out8(PIT_CNT0, 0x9c); // Set low byte of PIT reload value
	_io_out8(PIT_CNT0, 0x2e); // Set high byte of PIT reload value
	timerctl.count = 0;
	timerctl.next_alarm_on_count = UINT32_MAX;
	timerctl.total_running = 0;
	for (int32_t i = 0; i < OS_MAX_TIMER; i++)
	{
		/* Flag: Unused */
		timerctl.timer[i].flags = 0;
		timerctl.timers[i] = NULL;
	}

	if (!isCli)
		_io_sti();

	return;
}

/**
 * First, allocate a timer
 * By default use a unique fifo buffer
 */
TIMER* timer_alloc(void)
{
	int32_t *fifo32buf = (int32_t *)kzalloc(512);
	FIFO32 *timer_fifo = (FIFO32 *)kzalloc(sizeof(FIFO32));
	fifo32_init(timer_fifo, fifo32buf, 512 / sizeof(fifo32buf[0]));
	return timer_alloc_customfifobuf(timer_fifo);
}

/**
 * Allocate a timer, allow specifying a custom fifo buffer
 */
TIMER* timer_alloc_customfifobuf(FIFO32 *fifo32)
{
	for (int32_t i = 0; i< OS_MAX_TIMER; i++)
	{
		TIMER *t = &timerctl.timer[i];
		if (t->flags == 0)
		{
			t->flags = TIMER_FLAGS_ALLOCATED;
			t->fifo = fifo32;
			return &timerctl.timer[i];
		}
	}
	return NULL;
}

/**
 * FIXME This is broken
 */
void timer_settimer(TIMER *timer, uint32_t timeout, uint8_t data)
{
	bool isCli = io_get_is_cli();
	if (!isCli)
		_io_cli();
	if (timer->flags != TIMER_FLAGS_ALLOCATED)
		return;

	/* This implementation does not need timeout--, thus slightly faster */
	timer->target_count = timerctl.count + timeout;
	/* Update `next` */
	if (timerctl.next_alarm_on_count > timer->target_count)
	{
		timerctl.next_alarm_on_count = timer->target_count;
	}
	timer->flags = TIMER_FLAGS_ONCOUNTDOWN;
	timer->data = data;
	timerctl.timers[timerctl.total_running++] = timer;

	if (!isCli)
		_io_sti();

	return;
}

/**
 * At last, free a timer
 * The TIMER *timer always exists and is 0 initialized
 */
void timer_free(TIMER *timer)
{
	timer->flags = 0;
	timer->data = 0;
	timer->target_count = 0;
	kfree(timer->fifo->buf);
	kfree(timer->fifo);
	return;
}

static void timer_arr_remove_element_u32(TIMER *arr[], uint32_t index_to_remove[], uint32_t arr_size, uint32_t index_to_remove_size)
{
	uint32_t oldi = 0, di = 0, newi = 0;

	for (oldi = 0; oldi < arr_size; oldi++)
	{
		if (di < index_to_remove_size && oldi == index_to_remove[di])
		{
			di++;
			continue;
		}
		arr[newi++] = arr[oldi];
	}
	return;
}


void timer_int_handler()
{
	timerctl.count++;
	if (timerctl.next_alarm_on_count > timerctl.count)
		return;
	timerctl.next_alarm_on_count = UINT32_MAX;
	/**
	 * Sorted array
	 * Timer has been triggerred (thus should be removed):
	 * TIMER *t = timerctl.timers[triggerred[x]];
	 */
	uint32_t triggerred[OS_MAX_TIMER] = {0};
	/* Temporary, array index */
	uint32_t __triggerred_arr_len = 0;
	for (uint32_t i = 0; i < timerctl.total_running; i++)
	{
		TIMER *t = timerctl.timers[i];
		/* Timers that are in use */
		if (t->flags == TIMER_FLAGS_ONCOUNTDOWN)
		{
			/* Trigger */
			if (t->target_count <= timerctl.count)
			{
				t->flags = TIMER_FLAGS_ALLOCATED;
				fifo32_enqueue(t->fifo, t->data);
				/* Push the triggered index, meanwhile this ensures the ascending order */
				triggerred[__triggerred_arr_len++] = i;
				continue;
			}
			/* Update `next` */
			if (timerctl.next_alarm_on_count > t->target_count)
			{
				timerctl.next_alarm_on_count = t->target_count;
			}
		}
	}

	/**
	 * Remove the triggerred timers from timerctl.timers
	 * Assume the triggered[OS_MAX_TIMER] is sorted by value in ascending order
	 * (which it should because it is the timers that are triggered on the
	 * same tick, and the loop was by index order)
	 */
	timer_arr_remove_element_u32(timerctl.timers, triggerred, timerctl.total_running, __triggerred_arr_len);
	timerctl.total_running = timerctl.total_running - __triggerred_arr_len;
	return;
}


int32_t timer_gettick()
{
	return timerctl.count;
}




