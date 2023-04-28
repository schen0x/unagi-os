#ifndef DRIVERS_PS2KBC_H_
#define DRIVERS_PS2KBC_H_

#include <stdint.h>

#define PS2KBC_PORT_DATA_RW 0x0060
#define PS2KBC_PORT_STATUS_R 0x0064
#define PS2KBC_PORT_CMD_W 0x0064
/*
 * Must be SET before attempting to read data from IO port 0x60
 * "Output buffer, 0x60", from the perspective of the PS2KBC
 */
#define PS2KBC_STATUS_FLAG_OUTPUT_BUF_FULL 1
/* Must be CLEAR before attempting to write data to IO port 0x60 or IO port 0x64 */
#define PS2KBC_STATUS_FLAG_INPUT_BUF_FULL 1 << 1
/* Read the "Configuration Byte" */
#define PS2KBC_CMD_CONFIG_READ 0x20
/* Write the "Configuration Byte" */
#define PS2KBC_CMD_CONFIG_WRITE 0x60
#define PS2KBC_CMD_KBC_SELFTEST 0xAA
/* Write next byte to second PS/2 port output buffer (makes it look like the byte written was received from the second PS/2 port) */
#define PS2KBC_CMD_REDIRECT_C2_OUTBUF 0xD3
/* Write next byte to second PS/2 port input buffer (makes it look like the byte written was received from the second PS/2 port) */
#define PS2KBC_CMD_REDIRECT_C2_INBUF 0xD4
#define PS2KBC_STATUS_KBC_SELFTEST_PASSED 0x55
#define PS2KBC_STATUS_KBC_SELFTEST_FAILED 0xFC
#define PS2MOUSE_CMD_SET_STREAM_MODE 0xEA
#define PS2MOUSE_CMD_GET_DEVICE_ID 0xF2
#define PS2MOUSE_CMD_DATA_REPORTING_ENABLE 0xF4
#define PS2MOUSE_CMD_DATA_REPORTING_DISABLE 0xF5
#define PS2MOUSE_CMD_RESET 0xFF
#define PS2MOUSE_STATUS_ACK 0xFA

void ps2kbc_KBC_init(void);
void ps2kbc_MOUSE_init(void);
#endif
