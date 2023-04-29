#ifndef INCLUDE_UAPI_INPUT_MOUSE_EVENT_H_
#define INCLUDE_UAPI_INPUT_MOUSE_EVENT_H_

#include <stdint.h>
typedef struct MOUSE_DATA_BUNDLE
{
	uint8_t buf[3], phase;
	int32_t x, y, btn;
} MOUSE_DATA_BUNDLE;

#endif

