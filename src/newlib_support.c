#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

void _exit(void)
{
  while (1)
    __asm__("hlt");
}

/**
 * As an example, this starts at around 0x1413a0 (some .bss offset);
 * then the original .data section may be 1.5 MB
 */
// caddr_t sbrk(int incr) {
//   /**
//    * @_end Defined by the linker, `man 3 end`,
//    * the the first address past the end of the uninitialized data segment (BSS)
//    */
//   extern char _end;
//   static char *heap_end;
//   char *prev_heap_end;
//
//   if (heap_end == 0) {
//     heap_end = &_end;
//   }
//   prev_heap_end = heap_end;
//   if (heap_end + incr > stack_ptr) {
//     write (1, "Heap and stack collision\n", 25);
//     abort ();
//   }
//
//   heap_end += incr;
//   return (caddr_t) prev_heap_end;
// }

/**
 * TODO mordern linux reImpl
 * The impl in the book, head start && end need to be initialized by us
 * typedef	char *	caddr_t;
 * @program_break heap_end; // the head of heap after the allocation
 * @program_break_end _end; // Some _end we assign in memory_manager.cpp
 * @return -1 when fail;
 */
caddr_t program_break, program_break_end;
caddr_t sbrk(int incr)
{
  if (program_break == 0 || program_break + incr >= program_break_end)
  {
    errno = ENOMEM;
    return (caddr_t)-1;
  }

  caddr_t prev_break = program_break;
  program_break += incr;
  return prev_break;
}

int getpid(void)
{
  return 1;
}

int kill(int pid, int sig)
{
  errno = EINVAL;
  return -1;
}

int close(int fd)
{
  errno = EBADF;
  return -1;
}

off_t lseek(int fd, off_t offset, int whence)
{
  errno = EBADF;
  return -1;
}

int open(const char *path, int flags)
{
  errno = ENOENT;
  return -1;
}

ssize_t read(int fd, void *buf, size_t count)
{
  errno = EBADF;
  return -1;
}

ssize_t write(int fd, const void *buf, size_t count)
{
  errno = EBADF;
  return -1;
}

int fstat(int fd, struct stat *buf)
{
  errno = EBADF;
  return -1;
}

int isatty(int fd)
{
  errno = EBADF;
  return -1;
}
