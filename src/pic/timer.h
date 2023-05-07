#ifndef _PIC_TIMER_H_
#define _PIC_TIMER_H_

#include "stdint.h"
#include "util/fifo.h"
#include "config.h"

typedef struct TIMER {
	/* Trigger when system ticks == "alarm" */
	uint32_t target_count;
	uint32_t flags;
	FIFO8 *fifo;
	uint8_t data;
} TIMER;

typedef struct TIMERCTL {
	uint32_t count;
	/*
	 * The next "count" which an alarm should be triggerred
	 * no need to check timer before that.
	 */
	uint32_t next;
	/* The number of timers that are curruntly counting down */
	uint32_t total_running;
	TIMER *TIMERS[OS_MAX_TIMER];
	TIMER timer[OS_MAX_TIMER];
} TIMERCTL;

void pit_init(void);
void timer_int_handler(void);
int32_t timer_gettick(void);

void pit_init(void);
TIMER* timer_alloc(void);
void timer_settimer(TIMER *timer, uint32_t timeout, uint8_t data);
void timer_free(TIMER *timer);

#endif
