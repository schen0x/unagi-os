#ifndef _PIC_TIMER_H_
#define _PIC_TIMER_H_

#include "stdint.h"
#include "util/fifo.h"
typedef struct TIMERCTL {
	/* count++ per tick from the  */
	uint32_t count;
	/* */
	uint32_t timeout;
	FIFO8 *fifo;
	uint8_t data;
} TIMERCTL;

void pit_init(void);
void timer_int_handler();
int32_t timer_gettick();
void settimer(uint32_t timeout, FIFO8 *fifo, uint8_t data);
FIFO8* timer_get_fifo8();

#endif
