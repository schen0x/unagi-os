#ifndef _PIC_TIMER_H_
#define _PIC_TIMER_H_

#include "stdint.h"
#include "util/fifo.h"
#include "config.h"

typedef struct TIMER {
	uint32_t timeout;
	uint32_t flags;
	FIFO8 *fifo;
	uint8_t data;
} TIMER;

typedef struct TIMERCTL {
	uint32_t count;
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
