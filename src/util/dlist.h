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
 * list_for_each_entry - loop through CONTAINERs with the following structure:
 * typedef struct CONTAINER {
 * 	DLIST l0;
 * 	DLIST member;
 * 	int32_t data;
 * } CONTAINER;
 *
 * (Assume a DLIST in the CONTAINERs are connected, loop the CONTAINERs based on the DLIST.)
 * Does not loop, when array.len == 1
 * When start: pos = "the next typeof(*pos) that contains head->next""
 *
 * Usage:
 *     	CONTAINER c0 = {0};
 *     	init_container(c0); // we know the starting position
 *     	CONTAINER c1 = {0};
 *     	init_container(c1);
 *     	CONTAINER c2 = {0};
 *     	init_container(c2); // assume (c1.l0.next == &c2.l0) && (c2.l0.next == &c0.l0)
 *
 *     	CONTAINER pos*;
 *     	DLIST *head = &c0.l0;
 *
 * 	int i = 0;
 *     	list_for_each_entry(pos, head, l0)
 *     	{
 *     		if (i == 0)
 *     		{
 *     			if (pos == &c1)
 *     				return true;
 *     		}
 *     	}
 *
 *
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

