#include "io.h"

/* Wait for but a moment (1 to 4 microseconds, generally). Useful for implementing a small delay for PIC remapping on old hardware or generally as a simple but imprecise wait.
 * You can do an IO operation on any unused port: the Linux kernel by default uses port 0x80, which is often used during POST to log information on the motherboard's hex display but almost always unused after boot. */
inline void io_wait(void)
{
    outb(0x80, 0);
}
