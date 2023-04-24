#ifndef _PIC_PIC_H_
#define _PIC_PIC_H_

void PIC_sendEOI(unsigned char irq);
void PIC_remap(int offset0, int offset1);
void init_pic(void);
#endif
