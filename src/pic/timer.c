#include "pic/timer.h"
#include "config.h"
#include "io/io.h"
#include "memory/memory.h"
#include "util/printf.h"
#include "kernel/process.h"
/**
 * Want a structure of timer that can easily be searched based on something (Easy Reset: pointer is known; Insert: need to search based on time; Most performance required at each INT handler, which implies that if a timer is frequently triggered, the time should be quite close to the head)
 *
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
	timerctl_init();
	if (!isCli)
		_io_sti();

	return;
}

static void timerctl_init(void)
{
	timerctl.tick = 0;
	timerctl.next_alarm_on_tick = UINT32_MAX;

	TIMER *timerTail = &timerctl.timer[OS_MAX_TIMER - 1];
	timerctl.listtail = timerTail;

	/* Should never be used */
	timerTail->flags = 0xffff;
	timerTail->data = 0;
	timerTail->fifo = NULL;
	timerTail->target_tick = UINT32_MAX;
	dlist_init(&timerTail->timerDL);

	DLIST *prev = &timerTail->timerDL;
	for (int32_t i = 0; i < OS_MAX_TIMER - 1; i++)
	{
		TIMER *t = &timerctl.timer[i];
		/* Flag: Unused */
		t->flags = 0;
		t->data = 0;
		t->fifo = NULL;
		t->target_tick = UINT32_MAX;
		/* Connect all into a loop */
		dlist_init(&t->timerDL);
		dlist_insert_after(prev, &t->timerDL);
		prev = &t->timerDL;
	}
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
			t->data = 0;
			t->target_tick = UINT32_MAX;
			dlist_remove(&t->timerDL);
			return t;
		}
	}
	return NULL;
}

/**
 * FIXME This is broken AF
 *   - data == 0 is reserved, means no change to prev data
 *   - when a timer is still running?
 */
void timer_settimer(TIMER *timer, uint32_t timeout, uint8_t data)
{
	if (!timer)
		return;
	bool isCli = io_get_is_cli();
	if (!isCli)
		_io_cli();

	if (data == 0)
		data = timer->data;

	/* If the timer is still running */
	if (timer->flags == TIMER_FLAGS_ONCOUNTDOWN)
	{
		DLIST *prevDL = timer->timerDL.prev;
		dlist_remove(&timer->timerDL);

		timer->target_tick = timerctl.tick + timeout;
		timer->flags = TIMER_FLAGS_ONCOUNTDOWN;
		timer->data = data;

		TIMER *pos;
		DLIST *head = prevDL;
		list_for_each_entry(pos, head, timerDL)
		{
			if (pos->target_tick >= timer->target_tick)
				dlist_insert_before(&pos->timerDL, &timer->timerDL);
			break;
		}
	}

		// TODO

	if (timer->flags == TIMER_FLAGS_ALLOCATED)
	{

		/* This implementation does not need timeout--, thus faster */
		timer->target_tick = timerctl.tick + timeout;

		/* Update `next` */
		if (timerctl.next_alarm_on_tick > timer->target_tick)
		{
			timerctl.next_alarm_on_tick = timer->target_tick;
		}

		timer->flags = TIMER_FLAGS_ONCOUNTDOWN;
		timer->data = data;

		/* Insert the timer based on tick order, note that the first pos is the TIMER contains head->next */
		TIMER *pos;
		DLIST *head = &timerctl.listtail->timerDL;

		list_for_each_entry(pos, head, timerDL)
		{
			if (pos->target_tick >= timer->target_tick)
				dlist_insert_before(&pos->timerDL, &timer->timerDL);
			break;
		}
	}

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
	if (!timer)
		return;
	timer->flags = 0;
	timer->data = 0;
	timer->target_tick = 0;
	if (timer->fifo)
	{
		kfree(timer->fifo->buf);
		kfree(timer->fifo);
	}
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
	timerctl.tick++;
	if (timerctl.next_alarm_on_tick > timerctl.tick)
		return;
	timerctl.next_alarm_on_tick = UINT32_MAX;
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
			if (t->target_count <= timerctl.tick)
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

	TIMER *tss_timer = process_get_tss_timer();
	if (tss_timer)
	{
		int32_t dt = fifo32_dequeue(tss_timer->fifo);
		if (dt > 0)
			process_autotaskswitch(100);
	}
	return;
}


int32_t timer_gettick()
{
	return timerctl.tick;
}




