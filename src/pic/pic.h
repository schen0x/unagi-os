#ifndef _PIC_PIC_H_
#define _PIC_PIC_H_

void PIC_sendEOI(unsigned char irq);
void PIC_remap(int offset1, int offset2);
#endif
