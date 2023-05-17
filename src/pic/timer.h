#ifndef _PIC_TIMER_H_
#define _PIC_TIMER_H_

#include "stdint.h"
#include "util/fifo.h"
#include "config.h"
#include "util/dlist.h"

#define TIMER_FLAGS_FREE 0
/* Timer is allocated */
#define TIMER_FLAGS_ALLOCATED 1
/* Timer is counting down */
#define TIMER_FLAGS_ONCOUNTDOWN 2
#define TIMER_FLAGS_GUARDNODE 0xffff

/**
 * A basic solution:
 *   - all timers are allocated on boot
 *   - all timers are connected, in the order of `target_tick`
 *   - on trigger/on flag == 1, remove timer from the list
 *   - on free, insert timer before the listtail
 */
typedef struct TIMER {
	/* Either running or free, does not include "allocated" timer */
	DLIST timerDL;
	/* Trigger when system ticks == "alarm" */
	uint32_t target_tick;
	uint32_t flags;
	/* This uses heap, only allocated on use */
	FIFO32 *fifo;
	uint8_t data;
} TIMER;

typedef struct TIMERCTL {
	uint32_t tick;
	/* listtail->target_tick == UINT32_MAX, tail of the DLIST, connected to the head */
	TIMER *listtail;
	/*
	 * The next `count` on which an alarm should be triggerred
	 * No need to look into the timer array before that.
	 */
	uint32_t next_alarm_on_tick;
	// /* The number of timers that are curruntly counting down */
	//uint32_t total_running;
	//TIMER *timers[OS_MAX_TIMER];
	/* 32 * 512 approx. 16KB. Want easy initialization */
	TIMER timer[OS_MAX_TIMER];
} TIMERCTL;

void pit_init(void);
static void timerctl_init(void);
void timer_int_handler(void);
int32_t timer_gettick(void);
static void __timer_set_default_params(TIMER *t);

void pit_init(void);
TIMER* timer_alloc_customfifobuf(FIFO32 *fifo32);
TIMER* timer_alloc(void);
void timer_settimer(TIMER *timer, uint32_t timeout, uint8_t data);
void timer_free(TIMER *timer);

#endif
