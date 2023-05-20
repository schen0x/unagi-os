#include "pic/timer.h"
#include "config.h"
#include "io/io.h"
#include "memory/memory.h"
#include "util/printf.h"
#include "kernel/process.h"
#include "util/containerof.h"
#include "kernel/mprocessfifo.h"
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
TIMER *tssTimer;

/**
 * Get a 100Hz clock (10ms per tick)
 * Setup the Programmable Interval Timer (PIT) chip (Intel 8353/8254)
 * Set counter to 0x2e9c (11932) => (1.1931816666 MHz / 11932 = 99.99846 Hz)
 */
void PIT_init(void)
{
	bool isCli = io_get_is_cli();
	if (!isCli)
		_io_cli();

	_io_out8(PIT_CTRL, 0x34); // channel 0, lobyte/hibyte, rate generator
	_io_out8(PIT_CNT0, 0x9c); // Set low byte of PIT reload value
	_io_out8(PIT_CNT0, 0x2e); // Set high byte of PIT reload value
	timerctl_init();
	tssTimer = timer_alloc_customfifo(NULL);
	if (!isCli)
		_io_sti();

	return;
}

TIMER* timer_get_tssTimer(void)
{
	return tssTimer;
}

/**
 * WARN: free the FIFO before resetting the parameters
 */
static void __timer_set_default_params(TIMER *t)
{
	if (!t)
		return;
	t->flags = TIMER_FLAGS_FREE;
	t->data = 0;
	t->fifo = NULL;
	t->target_tick = UINT32_MAX;
	dlist_init(&t->timerDL);
	return;
}


static void timerctl_init(void)
{
	timerctl.tick = 0;
	timerctl.next_alarm_on_tick = UINT32_MAX;

	TIMER *timerTail = &timerctl.timer[OS_MAX_TIMER - 1];
	timerctl.listtail = timerTail;

	__timer_set_default_params(timerTail);
	/* The guard node hould never be modified */
	timerTail->flags = TIMER_FLAGS_GUARDNODE;

	DLIST *prev = &timerTail->timerDL;
	for (int32_t i = 0; i < OS_MAX_TIMER - 1; i++)
	{
		TIMER *t = &timerctl.timer[i];
		__timer_set_default_params(t);
		/* Connect all into a loop */
		dlist_insert_after(prev, &t->timerDL);
		prev = &t->timerDL;
	}
}

/**
 * Find a free timer and "allocate"
 *   - Auto allocate an FIFO
 * Return NULL when all timers are occupied
 */
TIMER* timer_alloc(void)
{
	int32_t *fifo32buf = (int32_t *)kzalloc(512);
	MPFIFO32 *timer_fifo = kzalloc(sizeof(FIFO32));
	mpfifo32_init(timer_fifo, fifo32buf, 512 / sizeof(fifo32buf[0]), NULL);
	return timer_alloc_customfifo(timer_fifo);
}


/**
 * WARN: Timer must exist
 */
static TIMER* __get_timer_prev(TIMER *timer)
{
	if (!timer)
		return NULL;
	return container_of(timer->timerDL.prev, TIMER, timerDL);
}

/**
 * WARN: Timer must exist
 */
static TIMER* __get_timer_next(TIMER *timer)
{
	if (!timer)
		return NULL;
	return container_of(timer->timerDL.next, TIMER, timerDL);
}

/**
 * Find a free timer and "allocate"
 *   - Attach FIFO (May be NULL)
 *   - Change its flag to TIMER_FLAGS_ALLOCATED
 *   - Remove the timer from the DL list
 * Return NULL when all timers are occupied
 */
TIMER* timer_alloc_customfifo(MPFIFO32 *fifo32)
{
	for (int32_t i = 0; i< OS_MAX_TIMER; i++)
	{
		TIMER *t = &timerctl.timer[i];
		if (t->flags == TIMER_FLAGS_FREE)
		{
			t->flags = TIMER_FLAGS_ALLOCATED;
			t->fifo = fifo32;
			/**
			 * The DL is for timers that are running or free
			 * Because the DL is in tick order, while the position is yet to be known
			 */
			dlist_remove(&t->timerDL);
			return t;
		}
	}
	return NULL;
}

/**
 * Set a timeout and start an ALLOCATED or RUNNING timer
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

	/**
	 * If the timer is still running:
	 *   - Check if timerctl.next_alarm_on_tick == timer.target_tick; update if necessary
	 *   - Remove the timer from the timerDL
	 *   - Set parameters
	 *   - Update `timerctl.next_alarm_on_tick`
	 *   - Insert the timer to the timerDL (with optimisation)
	 */
	if (timer->flags == TIMER_FLAGS_ONCOUNTDOWN)
	{

		if (timerctl.next_alarm_on_tick == timer->target_tick)
		{
			TIMER *nextTimer = __get_timer_next(timer);
			timerctl.next_alarm_on_tick = nextTimer->target_tick;
		}

		TIMER *prevTimer = __get_timer_prev(timer);
		DLIST *prevDL = &prevTimer->timerDL;
		/* Remove from the timerDL */
		dlist_remove(&timer->timerDL);

		/* Set parameters */
		timer->target_tick = timerctl.tick + timeout;
		timer->flags = TIMER_FLAGS_ONCOUNTDOWN;
		timer->data = data;
		/* Update `timerctl.next_alarm_on_tick` */
		if (timerctl.next_alarm_on_tick > timer->target_tick)
		{
			timerctl.next_alarm_on_tick = timer->target_tick;
		}

		/**
	 	 * Insert the new timer back to the correct position in timeDL
		 *   - By default, listhead = timerctl.listtail
		 *   - To optimise, compare the `target_tick` with the previous timer,
		 *   if is postponing, listhead = prevDL
		 */
		TIMER *pos;
		DLIST *head = &timerctl.listtail->timerDL;
		if (prevTimer->target_tick < timer->target_tick)
			head = prevDL;
		list_for_each_entry(pos, head, timerDL)
		{
			if (pos->target_tick >= timer->target_tick)
			{
				dlist_insert_before(&pos->timerDL, &timer->timerDL);
				break;
			}
		}
	}

	/**
	 * Normally
	 *   - Set parameters
	 *   - Update `timerctl.next_alarm_on_tick`
	 *   - Insert the timer to the timerDL
	 */
	if (timer->flags == TIMER_FLAGS_ALLOCATED)
	{
		/* Set parameters */
		timer->target_tick = timerctl.tick + timeout;
		timer->flags = TIMER_FLAGS_ONCOUNTDOWN;
		timer->data = data;
		/* Update `timerctl.next_alarm_on_tick` */
		if (timerctl.next_alarm_on_tick > timer->target_tick)
		{
			timerctl.next_alarm_on_tick = timer->target_tick;
		}

		/* Insert the timer based on the tick order */
		TIMER *pos;
		DLIST *head = &timerctl.listtail->timerDL;
		list_for_each_entry(pos, head, timerDL)
		{
			if (pos->target_tick >= timer->target_tick)
			{
				dlist_insert_before(&pos->timerDL, &timer->timerDL);
				break;
			}
		}
	}

	if (!isCli)
		_io_sti();

	return;
}

/**
 * Free a TIMER
 *   - if not exist, or guardnode(listtail), return
 *   - if was still RUNNING, cleanup timerctl.next_alarm_on_tick
 *
 *   - Remove the TIMER from the list
 *   - Reset its parameters, free the FIFO32
 *   - Insert the TIMER back to the listtail
 */
void timer_free(TIMER *timer)
{
	if (!timer)
		return;
	if (timer->flags == TIMER_FLAGS_GUARDNODE)
		return;
	if (timer->flags == TIMER_FLAGS_ONCOUNTDOWN)
	{
		if (timerctl.next_alarm_on_tick == timer->target_tick)
		{
			TIMER *nextTimer = __get_timer_next(timer);
			timerctl.next_alarm_on_tick = nextTimer->target_tick;
		}
	}
	/**
	 * This do no harm (changes nothing), when the node is not in the main timerDL circle
	 */
	dlist_remove(&timer->timerDL);
	if (timer->fifo)
	{
		kfree(timer->fifo->fifo32.buf);
		kfree(timer->fifo);
	}
	__timer_set_default_params(timer);

	dlist_insert_before(&timerctl.listtail->timerDL, &timer->timerDL);
	return;
}

void timer_int_handler()
{
	bool isCli = io_get_is_cli();
	if (!isCli)
		_io_cli();

	timerctl.tick++;
	if (timerctl.next_alarm_on_tick > timerctl.tick)
		return;

	/* mProcess, tss */
	bool isTssTriggerred = false;

	TIMER *pos;
	DLIST *head = &timerctl.listtail->timerDL;
	list_for_each_entry(pos, head, timerDL)
	{
		if (pos->target_tick > timerctl.tick)
			break;
		/**
		 * On trigger,
		 *   - push data to FIFO (does nothing if FIFO == NULL)
		 *   - revert a timer back to ALLOCATED (alter the flag, remove from timerDL)
		 */

		/* mProcess, tss */
		if (isTssTriggerred == false && tssTimer && pos == tssTimer)
			isTssTriggerred = true;

		mpfifo32_enqueue(pos->fifo, pos->data);

		/**
		 * WARN: Mutating the DList itself
		 *   - set the pos to the original DList after detaching
		 */
		TIMER *prevTimer = __get_timer_prev(pos);
		pos->flags = TIMER_FLAGS_ALLOCATED;
		dlist_remove(&pos->timerDL);
		pos = prevTimer;

	}
	/* Update `timerctl.next_alarm_on_tick` */
	TIMER *firstTimer = __get_timer_next(timerctl.listtail);
	timerctl.next_alarm_on_tick = firstTimer->target_tick;

	if (!isCli)
		_io_sti();

	/* mProcess, tss */
	if (isTssTriggerred == true)
	{
		mprocess_task_autoswitch();
	}
	return;
}


int32_t timer_gettick()
{
	return timerctl.tick;
}

