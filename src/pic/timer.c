#include "pic/timer.h"
#include "config.h"
#include "io/io.h"
#include "memory/memory.h"

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
	_io_out8(PIT_CTRL, 0x34); // channel 0, lobyte/hibyte, rate generator
	_io_out8(PIT_CNT0, 0x9c); // Set low byte of PIT reload value
	_io_out8(PIT_CNT0, 0x2e); // Set high byte of PIT reload value
	timerctl.count = 0;
	for (int32_t i = 0; i < OS_MAX_TIMER; i++)
	{
		/* Flag: Unused */
		timerctl.timer[i].flags = 0;
	}
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

			FIFO8 *timer_fifo = (FIFO8 *)kzalloc(sizeof(FIFO8));
			uint8_t *timer_buf = (uint8_t *)kzalloc(512);
			fifo8_init(timer_fifo, timer_buf, 512);
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
	timer->timeout = timeout;
	timer->flags = TIMER_FLAGS_ONCOUNTDOWN;
	timer->data = data;
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


void timer_int_handler()
{
	timerctl.count++;
	for (int32_t i = 0; i < OS_MAX_TIMER; i++)
	{
		TIMER *t = &timerctl.timer[i];
		if (t->flags == TIMER_FLAGS_ONCOUNTDOWN)
		{
			t->timeout--;
			if (t->timeout == 0)
			{
				fifo8_enqueue(t->fifo, t->data);
			}
		}
	}
	return;
}


int32_t timer_gettick()
{
	return timerctl.count;
}


