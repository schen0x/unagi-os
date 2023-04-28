/*
 * PS2 keyboard init process
 * But the proper way is to use ACPI
 * TODO ACPI
 * TODO PS2KBC selftest. Assumed PS2MOUSE support, so kernel may likely crash
 * on a platform without dual channel PS2 support
 *
 * To send a command to the controller, write the command byte to IO port 0x64.
 * If there is a "next byte" (the command is 2 bytes) then the next byte needs
 * to be written to IO Port 0x60 after making sure that the controller is ready
 * for it (by making sure bit 1 of the Status Register is clear
 * (PS2KBC_STATUS_FLAG_INPUT_BUF_FULL is 0)). If there is a response byte, then
 * the response byte needs to be read from IO Port 0x60 after making sure it
 * has arrived (by making sure bit 0 of the Status Register is set
 * (PS2KBC_CMD_CONFIG_READ is 1)).
 */
#include "drivers/ps2kbc.h"
#include "io/io.h"
#include "util/kutil.h"
#include "util/printf.h"
#include <stdbool.h>

void ps2kbc_wait_KBC_writeReady()
{
	// Wait until flag bit is CLEAR (which indicate writable)
	while(!isMaskBitsAllClear(_io_in8(PS2KBC_PORT_STATUS_R), PS2KBC_STATUS_FLAG_INPUT_BUF_FULL))
		asm("pause");
	return;
}

void ps2kbc_wait_KBC_readReady()
{
	// Wait until flag bit is SET (which indicate readable)
	while(!isMaskBitsAllSet(_io_in8(PS2KBC_PORT_STATUS_R), PS2KBC_STATUS_FLAG_OUTPUT_BUF_FULL))
		asm("pause");
	return;
}

/*
 * "PS/2 Controller Configuration Byte"
 * The straight goal should be to set the "Second PS/2 port interrupt"
 */
void ps2kbc_KBC_init(void)
{
	/*
	 * Ensure the output buffer is not blocked before init.
	 * If bit 0 is set -> buffer is full -> flush it
	 */
	if ((_io_in8(PS2KBC_PORT_STATUS_R) & PS2KBC_STATUS_FLAG_OUTPUT_BUF_FULL) == 1)
		_io_in8(PS2KBC_PORT_DATA_RW);

	/* Get the existing flags */
	ps2kbc_wait_KBC_writeReady();
	_io_out8(PS2KBC_PORT_CMD_W, PS2KBC_CMD_CONFIG_READ);
	/* Read the response */
	ps2kbc_wait_KBC_readReady();
	uint8_t kbc_conf0 = _io_in8(PS2KBC_PORT_DATA_RW);

	/* Test the bit 5 (0-indexed) (Second PS/2 port clock), if 0 (enabled), the dual channel controller is probably not supported, because by default it should be 1 (disabled)  */
	if (((kbc_conf0 >> 5) & 1) == 0)
		return;

	/* Enable mouse (IRQ12) by setting the bit 1 (0-indexed) (Second PS/2 port interrupt (1 = enabled, 0 = disabled, only if 2 PS/2 ports supported)) */
	kbc_conf0 |= 1 << 1;
	/* Write back the flags (the "next byte" after PS2KBC_CMD_CONFIG_WRITE) */
	ps2kbc_wait_KBC_writeReady();
	_io_out8(PS2KBC_PORT_CMD_W, PS2KBC_CMD_CONFIG_WRITE);
	ps2kbc_wait_KBC_writeReady();
	_io_out8(PS2KBC_PORT_DATA_RW, kbc_conf0);
}

/*
 * Enable the Second PS/2 Controller (mouse)
 * Which is by default disabled.
 * Because enabling it in an unsupported device may cause crash.
 * TODO add timeout in case only 1 byte is sent from the device after ACK(0xfa)
 */
void ps2kbc_MOUSE_init(void)
{
	ps2kbc_wait_KBC_writeReady();
	_io_out8(PS2KBC_PORT_CMD_W, PS2KBC_CMD_REDIRECT_C2_INBUF);
	ps2kbc_wait_KBC_writeReady();
	_io_out8(PS2KBC_PORT_DATA_RW, PS2MOUSE_CMD_RESET);
	ps2kbc_wait_KBC_readReady();
	_io_in8(PS2KBC_PORT_DATA_RW); // 0xfa

	/* In emulation, ~500 milliseconds after powerup, transmit "0xAA, 0x00" */
	ps2kbc_wait_KBC_readReady();
	_io_in8(PS2KBC_PORT_DATA_RW); // 0xaa
	ps2kbc_wait_KBC_readReady();
	_io_in8(PS2KBC_PORT_DATA_RW); // 0x00?

	//ps2kbc_wait_KBC_writeReady();
	//_io_out8(PS2KBC_PORT_CMD_W, PS2KBC_CMD_REDIRECT_C2_INBUF);
	//ps2kbc_wait_KBC_writeReady();
	//_io_out8(PS2KBC_PORT_DATA_RW, PS2MOUSE_CMD_GET_DEVICE_ID);
	//ps2kbc_wait_KBC_readReady();
	//_io_in8(PS2KBC_PORT_DATA_RW); // 0xfa
	//ps2kbc_wait_KBC_readReady();
	//_io_in8(PS2KBC_PORT_DATA_RW); // 0x00

	//ps2kbc_wait_KBC_writeReady();
	//_io_out8(PS2KBC_PORT_CMD_W, PS2KBC_CMD_REDIRECT_C2_INBUF);
	//ps2kbc_wait_KBC_writeReady();
	//_io_out8(PS2KBC_PORT_DATA_RW, PS2MOUSE_CMD_SET_STREAM_MODE);

	ps2kbc_wait_KBC_writeReady();
	_io_out8(PS2KBC_PORT_CMD_W, PS2KBC_CMD_REDIRECT_C2_INBUF);
	ps2kbc_wait_KBC_writeReady();
	_io_out8(PS2KBC_PORT_DATA_RW, PS2MOUSE_CMD_DATA_REPORTING_ENABLE);
	return;
}
