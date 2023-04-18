#include "io/io.h"
#include "disk/disk.h"
#include <stdint.h>
#include <stddef.h>
#include "memory/memory.h"
#include "status.h"
#include "config.h"

#define ATA_PRIMARY_CTRL_BASE_ADDR 0x1f0
#define ATA_DATA_RW ATA_PRIMARY_CTRL_BASE_ADDR + 0
#define ATA_ERR_R ATA_PRIMARY_CTRL_BASE_ADDR + 1
#define ATA_FEATURE_W ATA_PRIMARY_CTRL_BASE_ADDR + 1
#define ATA_SECTOR_COUNT_RW ATA_PRIMARY_CTRL_BASE_ADDR + 2
#define ATA_LBA_LOW_RW ATA_PRIMARY_CTRL_BASE_ADDR + 3
#define ATA_LBA_MID_RW ATA_PRIMARY_CTRL_BASE_ADDR + 4
#define ATA_LBA_HIGH_RW ATA_PRIMARY_CTRL_BASE_ADDR + 5
#define ATA_DEVICE_OR_HEAD_RW ATA_PRIMARY_CTRL_BASE_ADDR + 6
#define ATA_STATUS_R ATA_PRIMARY_CTRL_BASE_ADDR + 7
#define ATA_CMD_W ATA_PRIMARY_CTRL_BASE_ADDR + 7

struct DISK disk;

/* Read `total_sectors` (max 0xff) sectors starting from the 28 bits `lba`, store to `buf`  */
int32_t disk_read_sector(int32_t lba, int32_t total_sectors, void* buf)
{
	lba &= 0x0fffffff; // 0-27 bits is the 28-bit-long lba
	outb(ATA_SECTOR_COUNT_RW, total_sectors);
	outb(ATA_LBA_LOW_RW, (lba & 0xff));
	outb(ATA_LBA_MID_RW, ((lba >> 8) & 0xff));
	outb(ATA_LBA_HIGH_RW, ((lba >> 16) & 0xff));
	outb(ATA_DEVICE_OR_HEAD_RW, ((lba >> 24) | 0b01000000)); // bits 24-27 + 0b?1?0 Select Primary Master Drive, bit 30 (6, 0b?1) indicate lbs, bit 28 indicate the DEVICE number.
	outb(ATA_CMD_W, 0x20);

	uint16_t *inbuf = (uint16_t*) buf;
	for (int32_t i = 0; i < total_sectors; i++)
	{
		// Wait for the controller to be ready
		uint8_t s = 0;
		while (!(s & 0x08))
		{
			s = insb(ATA_STATUS_R);
		}
		// Copy from hard disk to memory, 2 bytes a time
		// OS_DISK_SECTOR_SIZE must be a multiple of 512
		for (int j = 0; j < OS_DISK_SECTOR_SIZE / 2; j++)
		{
			*inbuf = insw(ATA_DATA_RW);
			inbuf++;
		}
	}
	return 0;
}


/* Search the disk and init */
void disk_search_and_init()
{
	kmemset(&disk, 0, sizeof(disk)); // init the struct DISK (metadata)
	disk.type = OS_DISK_TYPE_REAL;
	disk.sector_size = OS_DISK_SECTOR_SIZE;
}

DISK* disk_get(int index)
{
	if (index != 0)
	{
		return NULL;
	}
	return &disk;
}

/*
 * Read from a disk location
 * Example: disk_read_block(disk_get(0), 0, 2, buf);
 */
int32_t disk_read_block(DISK* idisk, uint32_t lba, int32_t total_sectors, void* buf)
{
	if (idisk != &disk)
	{
		return -EIO;
	}
	return disk_read_sector(lba, total_sectors, buf);
}

