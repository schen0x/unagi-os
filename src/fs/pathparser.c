#include "fs/pathparser.h"
#include <stdint.h>
#include <stddef.h>
#include "config.h"
#include "util/kutil.h"
#include "memory/memory.h"
#include "status.h"

static int32_t path_valid_format(const char* filename)
{
	if (!filename)
	{
		return 0;
	}
	int32_t len = kstrnlen(filename, OS_PATH_MAX_LENGTH);
	return (len >= 3 && is_digit(filename[0]) && (kmemcmp((void*)&filename[1], ":/", 2) == 0));
}

/*
 * Get and remove the drive part in a path
 */
static int32_t path_get_drive_number_by_path(const char** path)
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


static PATH_ROOT* path_create_root(int32_t driver_number)
{
	PATH_ROOT* pr = kzalloc(sizeof(PATH_ROOT));
	pr->drive_no = driver_number;
	pr->first = NULL;
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
		// If already found some path_part, on encountering the '/', stop and return
		if (**path == '/' && kstrnlen(path_part, OS_PATH_MAX_LENGTH) != 0)
		{
			(*path)++;
			return path_part;
		}
		path_part[i] = **path;
		(*path)++;
	}
	if (kstrnlen(path_part, OS_PATH_MAX_LENGTH) == 0)
	{
		kfree(path_part);
		return NULL;
	}
	return path_part;
}

/*
 * Append prev->next.
 * Return prev->next on success, otherwise NULL.
 * If the path_get_path_part(path_str) returns NULL, make no
 * change because prev->next is already NULL by default.
 */
PATH_PART* path_create_part(PATH_PART* prev, const char** path_str)
{
	const char* path_part_str = path_get_path_part(path_str);
	if (!path_part_str)
		return NULL;
	PATH_PART* pp = kzalloc(sizeof(PATH_PART));
	pp->part = path_part_str;
	pp->next = NULL;
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

/*
 * Parse a path, given a "path_str", e.g., "0:/opt/bin"
 * Return PATH_ROOT* or NULL;
 */
PATH_ROOT* path_parse(const char* path_str, const char* current_dir_path)
{
	(void)current_dir_path;
	int32_t drive_number = 0;
	const char* tmp_p_ptr = path_str;
	PATH_ROOT* path_root = 0;

	if (kstrlen(path_str) > OS_PATH_MAX_LENGTH)
		return NULL;
	drive_number = path_get_drive_number_by_path(&tmp_p_ptr);
	if (drive_number < 0)
		return NULL;
	path_root = path_create_root(drive_number);
	if (!path_root)
		return NULL;
	PATH_PART* part = path_create_part(NULL, &tmp_p_ptr);
	if (!part)
		return NULL;
	path_root->first = part;

	while(part)
	{
		part = path_create_part(part, &tmp_p_ptr);
	}
	return path_root;
}

