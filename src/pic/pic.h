#ifndef _PIC_PIC_H_
#define _PIC_PIC_H_

#include <stdint.h>
void PIC_sendEOI(uint8_t irq);
void PIC_remap(uint8_t offset0, uint8_t offset1);
#endif
