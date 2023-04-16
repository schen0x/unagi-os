#ifndef FS_PATHPARSER_H_
#define FS_PATHPARSER_H_

#include <stdint.h>

typedef struct PATH_ROOT
{
	int32_t drive_no;
	struct PATH_PART* first;
} PATH_ROOT;

typedef struct PATH_PART
{
	const char* part;
	struct PATH_PART* next;
} PATH_PART;
uint32_t path_free(PATH_ROOT* root);
PATH_ROOT* path_parse(const char* path_str, const char* current_dir_path);

#endif

