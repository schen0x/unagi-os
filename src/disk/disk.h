#ifndef DISK_DISK_H_
#define DISK_DISK_H_
#include <stdint.h>

int32_t disk_read_sector(int32_t lba, int32_t total, void* buf);

typedef uint32_t OS_DISK_TYPE;

#define OS_DISK_TYPE_REAL 0

typedef struct DISK
{
	OS_DISK_TYPE type;
	int32_t sector_size;
} DISK;
void disk_search_and_init();
int32_t disk_read_block(DISK* idisk, uint32_t lba, int32_t total, void* buf);
DISK* disk_get(int index);
#endif
