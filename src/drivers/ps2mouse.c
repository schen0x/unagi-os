#include "drivers/ps2mouse.h"
#include "include/uapi/graphic.h"
#include "include/uapi/input-mouse-event.h"
#include "util/kutil.h"
#include "util/printf.h"
#include "status.h"
#include <stdint.h>

/**
 * Decode mouse data packets
 * Assume 3 packets protocol
 *
 * [IN] uint8_t scancode
 * [IN, OUT] MOUSE_DATA_BUNDLE *m
 * FIXME This is broken AF;
 * Many packets are discarded.
 * Because sometimes there may be only 1~2 bytes (QEMU bug?)
 * i.e. when a change is tiny, the 0x08 is not followed by 0x00 0x00
 * Need a Timer
 */
void ps2mouse_decode(uint8_t scancode, MOUSE_DATA_BUNDLE *m)
{
	if (scancode == 0xfa)
		m->phase = 1;
	if (m->phase == 0)
		return;
	/* xy overflow bits not set, bit 3 is set */
	if (m->phase == 1 && ((scancode & 0xc8) == 0x08))
	/*
	 * xy overflow bits not set, bit 3 is set
	 * And reset the phase on 0x08, 0x18, 0x28... packets
	 * Because previous packet is likely not completed
	 */
	// if ((scancode & 0xc8) == 0x08)
	{
		m->buf[0] = scancode;
		m->phase = 2;
		return;
	}

	if (m->phase == 2)
	{
		m->buf[1] = scancode;
		m->phase = 3;
		return;
	}
	if (m->phase == 3)
	{
		m->buf[2] = scancode;
		ps2mouse_parse_three_bytes(m);
		m->phase = 1;
		// printf("%02d %02d %02x %02x %02x \n", m->x, m->y, m->buf[0], m->buf[1], m->buf[2]);
		return;
	}
}

static int32_t parse_twos_compliment(int32_t signed_bit, uint8_t tail)
{
	int32_t res = tail;
	if (signed_bit)
	{
		res = ~res + 1;
	}
	// char buf[20]={0};
	// sprintf(buf, "sign:%x tail:%02x res:%02d \n", signed_bit, tail, res);
	// kfprint(buf, 4);
	return res;
}

/*
 * Assume a "second_byte" is already in two's compliment format.
 * i.e. signed == 1, second_byte = 0xfe means -2
 */

static int32_t glue_twos_compliment_fragment(int32_t signed_bit, uint8_t tail)
{
	int32_t res = 0;
	if (signed_bit)
	{
		res = ~res;
		res &= 0xffffff00;
		res |= tail;
	} else
	{
		res = tail;
	}
	return res;
}

int32_t ps2mouse_parse_three_bytes(MOUSE_DATA_BUNDLE *m)
{
	if (!m)
		return -EIO;
	uint8_t state = m->buf[0];
	uint8_t second_byte = m->buf[1];
	uint8_t third_byte = m->buf[2];

	int32_t rel_x = glue_twos_compliment_fragment(isMaskBitsAllSet(state, 0x10), second_byte);
	int32_t rel_y = glue_twos_compliment_fragment(isMaskBitsAllSet(state, 0x20), third_byte);
	// int32_t rel_x = parse_twos_compliment(isMaskBitsAllSet(state, 0x10), second_byte);
	// int32_t rel_y = parse_twos_compliment(isMaskBitsAllSet(state, 0x20), third_byte);
	/*
	 * This only works for int8_t rel_x, rel_y. Because 2 - 0x100 is -254, which is then not properly casted(? it should work?)
	 *
	 * ((state << 4) & 0x100) equals 0x100 only if the signed
	 * bit (9'th bit stored in the first byte) is set. If the 9'th bit is
	 * set then the value is deemed negative, but the value in second_byte
	 * is not stored in one or two's complement form. It is instead stored
	 * as a positive 8-bit value. So, if second_byte is say a 2 then it
	 * will become 2 minus 0 since the negative (9'th bit) is off. But, if
	 * it is on then it will become 2 minus 0x100 which will produce the
	 * twos complement, or -2. It will also cause the register to be
	 * correctly sign extended no matter its size.
	 */
	// int32_t rel_x = second_byte - ((state << 4) & 0x100);
	// int32_t rel_y = third_byte - ((state << 3) & 0x100);
	m->x = rel_x;
	m->y = rel_y;
	/* Bit 0:2 is button state (bl, br, bm) */
	m->btn = m->buf[0] & 0b111;
	return 0;
}

