#include "pic/timer.h"
#include "io/io.h"

#define PIT_CNT0 0x0040
#define PIT_CTRL 0x0043

TIMERCTL timerctl;
FIFO8 __timer_fifo = {0};
uint8_t __timer_buf[256] = {0};

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
	fifo8_init(&__timer_fifo, __timer_buf, sizeof(__timer_buf));
	return;
}


void timer_int_handler()
{
	timerctl.count++;
	if (timerctl.timeout > 0)
	{
		timerctl.timeout--;
		if (timerctl.timeout == 0)
		{
			fifo8_enqueue(timerctl.fifo, timerctl.data);
		}
	}
	return;
}


int32_t timer_gettick()
{
	return timerctl.count;
}


void settimer(uint32_t timeout, FIFO8 *fifo, uint8_t data)
{
	int32_t eflags;
	eflags = _io_get_eflags();
	_io_cli();
	timerctl.timeout = timeout;
	timerctl.fifo = fifo;
	timerctl.data = data;
	_io_set_eflags(eflags);
	_io_sti();
	return;
}

FIFO8* timer_get_fifo8()
{
	return &__timer_fifo;
}

