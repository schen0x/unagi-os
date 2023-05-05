#ifndef _PIC_TIMER_H_
#define _PIC_TIMER_H_

#include "stdint.h"
typedef struct TIMERCTL {
	uint32_t count;
} TIMERCTL;

void pit_init(void);
void timer_int_handler();
int32_t timer_gettick();

#endif
