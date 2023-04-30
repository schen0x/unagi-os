#include "util/dlist.h"
#include <stdint.h>
#include <stdbool.h>

/* Initialize a one element *circular Doubly-Linked list* */
inline void dlist_init(DLIST *dlist)
{
	dlist->next = dlist;
	dlist->prev = dlist;
}

/*
 * Insert a DL `dnew_a ~ dnew_z` after d0 (between d0 and d1)
 * Assume d0 and dnew are circular (which implies all ptrs are NOT NULL)
 */
inline bool dlist_insert_after(DLIST *d0, DLIST *dnew_a)
{
	/* Backup the reference to the other involved nodes */
	DLIST *d1 = d0->next;
	DLIST *dnew_z = dnew_a->prev;

	/* Update the four nodes  */
	d0->next = dnew_a;
	dnew_a->prev = d0;
	dnew_z->next = d1;
	d1->prev = dnew_z;
	return true;
}


/*
 * Insert a DL `dnew_a ~ dnew_z` before d0 (between d0 and d999)
 * Assume d0 and dnew are circular (which implies all ptrs are NOT NULL)
 */
inline bool dlist_insert_before(DLIST *d0, DLIST *dnew_a) {
	/* Backup the reference to the other involved nodes */
	DLIST *d999 = d0->prev;
	DLIST *dnew_z = dnew_a->prev;

	/* Update the four nodes  */
	d0->prev = dnew_z;
	dnew_z->next = d0;
	dnew_a->prev = d999;
	d999->next = dnew_a;
	return true;
}


/*
 * Remove the node between d0 and d2
 * Assume the list are circular (which implies all ptrs are NOT NULL)
 */
inline bool dlist_remove(DLIST *node) {
	DLIST *d0 = node->prev;
	DLIST *d2 = node->next;

	/* Update the node */
	node->prev = node;
	node->next = node;

	/*
	 * If the DLIST has at least 3 elements, OK
	 * If only 2, then d0 == d2, OK
	 * If only 1, then d0 == node == d2, OK
	 */
	d0->next = d2;
	d2->prev = d0;
	return false;
}


bool test_dlist()
{
	DLIST *d0 = {0}, *d1 = {0}, *d2 = {0}, *d3 = {0};
	DLIST *dnew_a = {0}, *dnew_b = {0}, *dnew_c = {0}, *dnew_d = {0};

	dlist_init(d0);
	dlist_init(d1);
	dlist_init(d2);
	dlist_init(d3);
	dlist_init(dnew_a);
	dlist_init(dnew_b);
	dlist_init(dnew_c);
	dlist_init(dnew_d);

	dlist_insert_after(d0, d1);
	dlist_insert_after(d1, d2);
	dlist_insert_before(d0, d3);

	if (d0->next != d1 || d0->next->next != d2 || d0->next->next->next != d3)
		return false;
	if (d0->prev != d3 || d0->prev->prev != d2 || d0->prev->prev->prev != d1)
		return false;

	dlist_insert_before(dnew_a, dnew_b);
	dlist_insert_before(dnew_a, dnew_c);
	dlist_insert_before(dnew_a, dnew_d);

	/* Insert dnew_a...dnew_d between d0 and d1 */
	dlist_insert_after(d0, dnew_a);
	if (d0->next != dnew_a || d0->next->next != dnew_b || dnew_d->next != d1)
		return false;
	if (dnew_a->prev != d0 || d1->prev != dnew_d)
		return false;

	dlist_remove(dnew_a);
	if (d0->next != dnew_b || dnew_b->prev != d0)
		return false;
	if (dnew_a->next != dnew_a || dnew_a->prev != dnew_a)
		return false;

	return true;
}

