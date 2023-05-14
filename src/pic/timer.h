#ifndef _PIC_TIMER_H_
#define _PIC_TIMER_H_

#include "stdint.h"
#include "util/fifo.h"
#include "config.h"

/* Timer is allocated */
#define TIMER_FLAGS_ALLOCATED 1
/* Timer is counting down */
#define TIMER_FLAGS_ONCOUNTDOWN 2

typedef struct TIMER {
	/* Trigger when system ticks == "alarm" */
	uint32_t target_count;
	uint32_t flags;
	/* This uses heap, only allocated on use */
	FIFO32 *fifo;
	uint8_t data;
} TIMER;

typedef struct TIMERCTL {
	uint32_t count;
	/*
	 * The next `count` on which an alarm should be triggerred
	 * No need to look into the timer array before that.
	 */
	uint32_t next_alarm_on_count;
	/* The number of timers that are curruntly counting down */
	uint32_t total_running;
	TIMER *timers[OS_MAX_TIMER];
	/* 32 * 512 approx. 16KB */
	TIMER timer[OS_MAX_TIMER];
} TIMERCTL;

void pit_init(void);
void timer_int_handler(void);
int32_t timer_gettick(void);

void pit_init(void);
TIMER* timer_alloc_customfifobuf(FIFO32 *fifo32);
TIMER* timer_alloc(void);
void timer_settimer(TIMER *timer, uint32_t timeout, uint8_t data);
void timer_free(TIMER *timer);

#endif
