#include "pic/timer.h"
#include "config.h"
#include "io/io.h"
#include "memory/memory.h"
#include "util/printf.h"

#define PIT_CNT0 0x0040
#define PIT_CTRL 0x0043
/* Timer is allocated */
#define TIMER_FLAGS_ALLOCATED 1
/* Timer is counting down */
#define TIMER_FLAGS_ONCOUNTDOWN 2


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
	timerctl.next = UINT32_MAX;
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
 */
TIMER* timer_alloc(void)
{
	for (int32_t i = 0; i< OS_MAX_TIMER; i++)
	{
		TIMER *t = &timerctl.timer[i];
		if (t->flags == 0)
		{
			t->flags = TIMER_FLAGS_ALLOCATED;

			FIFO32 *timer_fifo = (FIFO32 *)kzalloc(sizeof(FIFO32));
			int32_t *timer_buf = (int32_t *)kzalloc(512);
			fifo32_init(timer_fifo, timer_buf, 512);
			t->fifo = timer_fifo;
			return &timerctl.timer[i];
		}
	}
	return NULL;
}

/**
 * Second, set a timer
 */
void timer_settimer(TIMER *timer, uint32_t timeout, uint8_t data)
{
	bool isCli = io_get_is_cli();
	if (!isCli)
		_io_cli();

	/* This implementation does not need timeout--, thus slightly faster */
	timer->target_count = timerctl.count + timeout;
	/* Update `next` */
	if (timerctl.next > timer->target_count)
	{
		timerctl.next = timer->target_count;
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
 */
void timer_free(TIMER *timer)
{
	timer->flags = 0;
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
	if (timerctl.next > timerctl.count)
		return;
	timerctl.next = UINT32_MAX;
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
				fifo8_enqueue(t->fifo, t->data);
				/* Push the triggered index, meanwhile this ensures the ascending order */
				triggerred[__triggerred_arr_len++] = i;
				continue;
			}
			/* Update `next` */
			if (timerctl.next > t->target_count)
			{
				timerctl.next = t->target_count;
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




