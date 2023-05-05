#include "pic/timer.h"
#include "io/io.h"

#define PIT_CNT0 0x0040
#define PIT_CTRL 0x0043

TIMERCTL timerctl;

/**
 * Get a 100Hz clock
 * Set counter to 0x2e9c (11932) => (1.1931816666 MHz / 11932 = 99.99846 Hz)
 */
void pit_init(void)
{
	_io_out8(PIT_CTRL, 0x34); // channel 0, lobyte/hibyte, rate generator
	_io_out8(PIT_CNT0, 0x9c); // Set low byte of PIT reload value
	_io_out8(PIT_CNT0, 0x2e); // Set high byte of PIT reload value
	timerctl.count = 0;
	return;
}


void timer_int_handler()
{
	timerctl.count++;
	return;
}


int32_t timer_gettick()
{
	return timerctl.count;
}
