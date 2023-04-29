#ifndef DRIVERS_PS2MOUSE_H_
#define DRIVERS_PS2MOUSE_H_

#include <stdint.h>
#include "include/uapi/input-mouse-event.h"

void ps2mouse_decode(uint8_t scancode, MOUSE_DATA_BUNDLE *m);
static int32_t parse_twos_compliment(int32_t signed_bit, uint8_t tail);
static int32_t glue_twos_compliment_fragment(int32_t signed_bit, uint8_t tail);
int32_t ps2mouse_parse_three_bytes(MOUSE_DATA_BUNDLE *m);

#endif
