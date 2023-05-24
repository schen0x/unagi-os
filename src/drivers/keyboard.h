#ifndef DRIVERS_KEYBOARD_H_
#define DRIVERS_KEYBOARD_H_

#include <stdint.h>

void atakbd_interrupt(uint8_t rawscancode);
void input_report_key(uint8_t scancode, uint8_t down);
void int21h_handler(uint8_t scancode);
char input_get_char(uint8_t rawscancode);

#endif
