// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * COPY AND PASTED FROM LINUX KERNEL v6.2.10
 * drivers/input/keyboard/atakbd.c
 */

/*
 *  atakbd.c
 *
 *  Copyright (c) 2005 Michael Schmitz
 *
 * Based on amikbd.c, which is
 *
 *  Copyright (c) 2000-2001 Vojtech Pavlik
 *
 *  Based on the work of:
 *	Hamish Macdonald
 */

#include "include/uapi/input-event-code.h"
#include "drivers/keyboard.h"
#include "idt/idt.h"
#include "util/kutil.h"
#include "include/uapi/graphic.h"
#define BREAK_MASK (0x80)

static unsigned char atakbd_keycode[0x73] = {	/* American layout */
	[1]	 = KEY_ESC,
	[2]	 = KEY_1,
	[3]	 = KEY_2,
	[4]	 = KEY_3,
	[5]	 = KEY_4,
	[6]	 = KEY_5,
	[7]	 = KEY_6,
	[8]	 = KEY_7,
	[9]	 = KEY_8,
	[10]	 = KEY_9,
	[11]	 = KEY_0,
	[12]	 = KEY_MINUS,
	[13]	 = KEY_EQUAL,
	[14]	 = KEY_BACKSPACE,
	[15]	 = KEY_TAB,
	[16]	 = KEY_Q,
	[17]	 = KEY_W,
	[18]	 = KEY_E,
	[19]	 = KEY_R,
	[20]	 = KEY_T,
	[21]	 = KEY_Y,
	[22]	 = KEY_U,
	[23]	 = KEY_I,
	[24]	 = KEY_O,
	[25]	 = KEY_P,
	[26]	 = KEY_LEFTBRACE,
	[27]	 = KEY_RIGHTBRACE,
	[28]	 = KEY_ENTER,
	[29]	 = KEY_LEFTCTRL,
	[30]	 = KEY_A,
	[31]	 = KEY_S,
	[32]	 = KEY_D,
	[33]	 = KEY_F,
	[34]	 = KEY_G,
	[35]	 = KEY_H,
	[36]	 = KEY_J,
	[37]	 = KEY_K,
	[38]	 = KEY_L,
	[39]	 = KEY_SEMICOLON,
	[40]	 = KEY_APOSTROPHE,
	[41]	 = KEY_GRAVE,
	[42]	 = KEY_LEFTSHIFT,
	[43]	 = KEY_BACKSLASH,
	[44]	 = KEY_Z,
	[45]	 = KEY_X,
	[46]	 = KEY_C,
	[47]	 = KEY_V,
	[48]	 = KEY_B,
	[49]	 = KEY_N,
	[50]	 = KEY_M,
	[51]	 = KEY_COMMA,
	[52]	 = KEY_DOT,
	[53]	 = KEY_SLASH,
	[54]	 = KEY_RIGHTSHIFT,
	[55]	 = KEY_KPASTERISK,
	[56]	 = KEY_LEFTALT,
	[57]	 = KEY_SPACE,
	[58]	 = KEY_CAPSLOCK,
	[59]	 = KEY_F1,
	[60]	 = KEY_F2,
	[61]	 = KEY_F3,
	[62]	 = KEY_F4,
	[63]	 = KEY_F5,
	[64]	 = KEY_F6,
	[65]	 = KEY_F7,
	[66]	 = KEY_F8,
	[67]	 = KEY_F9,
	[68]	 = KEY_F10,
	[71]	 = KEY_HOME,
	[72]	 = KEY_UP,
	[74]	 = KEY_KPMINUS,
	[75]	 = KEY_LEFT,
	[77]	 = KEY_RIGHT,
	[78]	 = KEY_KPPLUS,
	[80]	 = KEY_DOWN,
	[82]	 = KEY_INSERT,
	[83]	 = KEY_DELETE,
	[96]	 = KEY_102ND,
	[97]	 = KEY_UNDO,
	[98]	 = KEY_HELP,
	[99]	 = KEY_KPLEFTPAREN,
	[100]	 = KEY_KPRIGHTPAREN,
	[101]	 = KEY_KPSLASH,
	[102]	 = KEY_KPASTERISK,
	[103]	 = KEY_KP7,
	[104]	 = KEY_KP8,
	[105]	 = KEY_KP9,
	[106]	 = KEY_KP4,
	[107]	 = KEY_KP5,
	[108]	 = KEY_KP6,
	[109]	 = KEY_KP1,
	[110]	 = KEY_KP2,
	[111]	 = KEY_KP3,
	[112]	 = KEY_KP0,
	[113]	 = KEY_KPDOT,
	[114]	 = KEY_KPENTER,
};

// static void atakbd_interrupt(unsigned char scancode, char down)
void atakbd_interrupt(uint8_t rawscancode)
{
	uint8_t keydown = 1;
	// if (rawscancode < 0x73) {		/* scancodes < 0xf3 are keys */
	if ((rawscancode | BREAK_MASK) < (0x73 | BREAK_MASK)) {		/* scancodes for keys, on press and on release */
		if (rawscancode > BREAK_MASK)
		{
			keydown = 0;
		}
		uint8_t uscancode = atakbd_keycode[rawscancode & ~BREAK_MASK];

		input_report_key(uscancode, !!keydown);
		// input_sync(atakbd_dev);
	}
	else				/* scancodes >= 0xf3 are mouse data, most likely */
	{

		kfprint("KERN_INFO atakbd_interrupt: unhandled scancode ", 4);
		char msg[8] = {0};
		kfprint(hex_to_ascii(msg, &rawscancode, sizeof(rawscancode)), 4);
	}

	return;
}

// TODO https://elixir.bootlin.com/linux/latest/source/drivers/input/input.c#L424
void input_report_key(uint8_t scancode, uint8_t down)
{
	if(!down)
	{
		return;
	}
	switch(scancode)
	{
		case 2:
			kfprint("1", 4);
			break;
		case 3:
			kfprint("2", 4);
			break;
		case 4:
			kfprint("3", 4);
			break;
		case 30:
			kfprint("a", 4);
			break;
		case 31:
			kfprint("s", 4);
			break;
		case 32:
			kfprint("d", 4);
			break;

	}
	return;
}


/*
 * Keyboard interrupt handler
 * scancode: uint8_t _scancode
 */
void int21h_handler(uint8_t scancode)
{
	//char buf[20]={0};
	//sprintf(buf, "21h_handler:%02x ", scancode);
	//kfprint(buf, 4);
	atakbd_interrupt(scancode);
}

