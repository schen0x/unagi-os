#ifndef UTIL_DLIST_H_
#define UTIL_DLIST_H_

#include <stdint.h>
#include <stdbool.h>
#include "util/containerof.h"
typedef struct DLIST {
    struct DLIST *next;
    struct DLIST *prev;
} DLIST;

void dlist_init(DLIST *dlist);
bool dlist_insert_after(DLIST *d0, DLIST *dnew_a);
bool dlist_insert_before(DLIST *d0, DLIST *dnew_a);
bool dlist_remove(DLIST *node);
bool test_dlist();


#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

/*
 * list_for_each_entry - cast a member of a structure out to the containing structure
 * typedef struct CONTAINER {
 * 	DLIST l0;
 * 	DLIST member;
 * 	int32_t data;
 * } CONTAINER;
 * @pos:	CONTAINER *pos, that the member is in
 * @head:	DLIST *head
 * @member:	the name of the DLIST within the struct.
 *
 * First, get the CONTAINER address of the (head->next; member)
 * COTAINER *pos;
 * for (pos = addr; &pos->member != head; pos = container_of(pos->member.next, CONTAINER, member)
 *
 * WARNING: any const qualifier of @head->next is lost.
 */
#define list_for_each_entry(pos, head, member) \
	for (pos = list_entry((head)->next, typeof(*pos), member); \
	&pos->member != (head); \
	pos = list_entry(pos->member.next, typeof(*pos), member))

#endif

