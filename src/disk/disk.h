#ifndef DISK_DISK_H_
#define DISK_DISK_H_

int32_t disk_read_sector(int32_t lba, int32_t total, void* buf);
#endif
