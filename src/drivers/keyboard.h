#ifndef DRIVERS_KEYBOARD_H_
#define DRIVERS_KEYBOARD_H_

#include <stdint.h>
#include <stdbool.h>

#define BREAK_MASK (0x80)

void atakbd_interrupt(uint8_t rawscancode);
void input_report_key(uint8_t scancode, uint8_t down);
void int21h_handler(uint8_t scancode);
char input_get_char(uint8_t rawscancode, bool isShift);

#endif
