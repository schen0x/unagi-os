#include "fs/pathparser.h"
#include <stdint.h>
#include "config.h"
#include "util/kutil.h"
#include "memory/memory.h"
#include "status.h"

static int32_t path_valid_format(const char* filename)
{
	int32_t len = kstrnlen(filename, OS_PATH_MAX_LENGTH);
	return (len >= 3 && is_digit(filename[0] && kmemcmp((void*)&filename[1], ":/", 2) == 0));
}

/*
 * Get and remove the drive part in a path
 */
static int path_get_drive_by_path(const char** path)
{
	if(!path_valid_format(*path))
	{
		return -EBADPATH;
	}
	int32_t driver_no = to_digit(*path[0]);

	// Skip 3 bytes, "0:/"
	*path += 3;
	return driver_no;
}


static PATH_ROOT* path_parse_root(int32_t driver_number)
{
	PATH_ROOT* pr = kzalloc(sizeof(PATH_ROOT));
	pr->drive_no = driver_number;
	pr->first = 0;
	return pr;
}

/* Get the "foo" from "///foo/bar" */
static const char* path_get_path_part(const char** path)
{
	char* path_part = kzalloc(OS_PATH_MAX_LENGTH);
	int32_t i = 0;
	for (i = 0; i < OS_PATH_MAX_LENGTH; i++)
	{
		if (**path == 0)
			break;
		if (**path == '/' && kstrnlen(path_part, OS_PATH_MAX_LENGTH) == 0)
		{
			(*path)++; // have not found anything yet, proceed
			continue;
		}
		path_part[i] = **path;
		(*path)++;
	}
	if (kstrnlen(path_part, OS_PATH_MAX_LENGTH) == 0)
	{
		kfree(path_part);
		return 0;
	}
	return path_part;
}

PATH_PART* path_parse_part(PATH_PART* prev, const char** path_str)
{
	const char* path_part_str = path_get_path_part(path_str);
	if (!path_part_str)
		return 0;
	PATH_PART* pp = kzalloc(sizeof(PATH_PART));
	pp->part = path_part_str;
	pp->next = 0;
	if (prev)
	{
		prev->next = pp;
	}
	return pp;
}

uint32_t path_free(PATH_ROOT* root)
{
	struct PATH_PART* pt_ptr[OS_PATH_MAX_LENGTH] = {0}; // all PATH_PART ptr
	PATH_PART* pt = root->first;
	int32_t i = 0;
	for (i = 0; i < OS_PATH_MAX_LENGTH; i++)
	{
		if (!pt)
			break;
		pt_ptr[i] = pt;
		pt = pt->next;
	}
	if (pt)
		return -EBADPATH;
	for (; i >= 0; i--)
	{
		kfree((void *)pt_ptr[i]->part); // the string buffer
		kfree(pt_ptr[i]);
	}
	kfree(root);
	return 0;
}

//PATH_ROOT* path_parse(const char* path, const char* current_dir_path)
//{
//	//TODO
//
//}
