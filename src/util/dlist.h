#ifndef UTIL_DLIST_H_
#define UTIL_DLIST_H_

#include <stdint.h>
#include <stdbool.h>
typedef struct DLIST {
    struct DLIST *next;
    struct DLIST *prev;
} DLIST;

void dlist_init(DLIST *dlist);
bool dlist_insert_after(DLIST *d0, DLIST *dnew_a);
bool dlist_insert_before(DLIST *d0, DLIST *dnew_a);
bool dlist_remove(DLIST *node);
bool test_dlist();

#endif

