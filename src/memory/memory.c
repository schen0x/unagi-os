// memory/memory.c
#include "memory/memory.h"
#include "memory/kheap.h"
#include "util/kutil.h"
#include <stddef.h>
#include <stdint.h>
#include "config.h"


/*
 * Must check if NULL before using!
 * Return a pointer
 */
void* kzalloc(size_t size)
{

	void* ptr = kmalloc(size);
	if (!ptr)
	{
		return NULL;
	}
	kmemset(ptr, 0, size);
	return ptr;
}

void k_mm_init()
{
	k_heap_table_mm_init(); // managemend by simple heap table

	// uint8_t* memory_start = (uint8_t*) OS_HEAP_ADDRESS;
	// kmemory_init(memory_start, OS_HEAP_SIZE_BYTES); // ? -1 FIXME FIX & CONFIRM LATER
}

void* kmalloc(size_t size)
{
	return k_heap_table_mm_malloc(size);
	// return k_heap_table_mm_malloc(size);
}


void kfree(void *ptr)
{
	k_heap_table_mm_free(ptr);
	// k_dl_mm_free(ptr);
}

/*
 * Linked List Bucket Heap 2013 Goswin von Brederlow <goswin-v-b@web.de>
 *
 * COPY PASTE FROM:
 * https://wiki.osdev.org/User:Mrvn/LinkedListBucketHeapImplementation
 *
 * Usage:
 *     kmemory_init(memory, size);
 *     ptr = (<ptrtype>)kmalloc(size);
 *     kfree((void*)ptr);
 */
typedef struct DList DList;
struct DList {
    DList *next;
    DList *prev;
};

// initialize a one element *circular Doubly-Linked list*
static inline void dlist_init(DList *dlist) {
	//printf("%s(%p)\n", __FUNCTION__, dlist);
    dlist->next = dlist;
    dlist->prev = dlist;
}

// insert d2 after d1
static inline void dlist_insert_after(DList *d1, DList *d2) {
	//printf("%s(%p, %p)\n", __FUNCTION__, d1, d2);
    DList *n1 = d1->next; // save the original next node
    DList *e2 = d2->prev; // save the original "tail" node of the 2nd DList, the DList is circular

    d1->next = d2;
    d2->prev = d1;

    e2->next = n1; // end of 2nd DList, points to original next node
    n1->prev = e2; // original next node, points to original "nail" node of 2nd DList
}

// insert d2 before d1
static inline void dlist_insert_before(DList *d1, DList *d2) {
	//printf("%s(%p, %p)\n", __FUNCTION__, d1, d2);
    DList *e1 = d1->prev; // save the end of the original list
    DList *e2 = d2->prev; // save the end of the 2nd list

    e1->next = d2; // end of 1, next be the 2
    d2->prev = e1; // d2, prev be the original list
    e2->next = d1; // e2, d1
    d1->prev = e2; // d1, e2
}

// remove d from the list
static inline void dlist_remove(DList *d) {
	//printf("%s(%p)\n", __FUNCTION__, d);
    d->prev->next = d->next;
    d->next->prev = d->prev;
    d->next = d;
    d->prev = d;
}

// push d2 to the front of the d1p list
static inline void dlist_push(DList **d1p, DList *d2) {
	//printf("%s(%p, %p)\n", __FUNCTION__, d1p, d2);
    if (*d1p != NULL) {
	dlist_insert_before(*d1p, d2);
    }
    *d1p = d2;
}

// pop the front of the dp list
static inline DList * dlist_pop(DList **dp) {
	//printf("%s(%p)\n", __FUNCTION__, dp);
    DList *d1 = *dp;
    DList *d2 = d1->next;
    dlist_remove(d1);
    if (d1 == d2) {
	*dp = NULL;
    } else {
	*dp = d2;
    }
    return d1;
}

// remove d2 from the list, advancing d1p if needed
static inline void dlist_remove_from(DList **d1p, DList *d2) {
	//printf("%s(%p, %p)\n", __FUNCTION__, d1p, d2);
    if (*d1p == d2) {
	dlist_pop(d1p);
    } else {
	dlist_remove(d2);
    }
}

#define CONTAINER(C, l, v) ((C*)(((char*)v) - (intptr_t)&(((C*)0)->l)))
#define OFFSETOF(TYPE, MEMBER)  __builtin_offsetof (TYPE, MEMBER)
/* Init the DList* &v->l. Set (&v->l)->next, (&v->l)->prev = &v->l */
#define DLIST_INIT(v, l) dlist_init(&v->l)

#define DLIST_REMOVE_FROM(h, d, l)					\
    {									\
	typeof(**h) **h_ = h, *d_ = d;					\
	DList *head = &(*h_)->l;					\
	dlist_remove_from(&head, &d_->l);					\
	if (head == NULL) {						\
	    *h_ = NULL;							\
	} else {							\
	    *h_ = CONTAINER(typeof(**h), l, head);			\
	}								\
    }

#define DLIST_PUSH(h, v, l)						\
    {									\
	typeof(*v) **h_ = h, *v_ = v;					\
	DList *head = &(*h_)->l;					\
	if (*h_ == NULL) head = NULL;					\
	dlist_push(&head, &v_->l);					\
	*h_ = CONTAINER(typeof(*v), l, head);				\
    }

#define DLIST_POP(h, l)							\
    ({									\
	typeof(**h) **h_ = h;						\
	DList *head = &(*h_)->l;					\
	DList *res = dlist_pop(&head);					\
	if (head == NULL) {						\
	    *h_ = NULL;							\
	} else {							\
	    *h_ = CONTAINER(typeof(**h), l, head);			\
	}								\
	CONTAINER(typeof(**h), l, res);					\
    })

#define DLIST_ITERATOR_BEGIN(h, l, it)					\
    {									\
        typeof(*h) *h_ = h;						\
	DList *last_##it = h_->l.prev, *iter_##it = &h_->l, *next_##it;	\
	do {								\
	    if (iter_##it == last_##it) {				\
		next_##it = NULL;					\
	    } else {							\
		next_##it = iter_##it->next;				\
	    }								\
	    typeof(*h)* it = CONTAINER(typeof(*h), l, iter_##it);

#define DLIST_ITERATOR_END(it)						\
	} while((iter_##it = next_##it));				\
    }

#define DLIST_ITERATOR_REMOVE_FROM(h, it, l) DLIST_REMOVE_FROM(h, iter_##it, l)

typedef struct Chunk Chunk;
struct Chunk {
    DList all;			// track all memory chunks in the system
    int used;			// is chunk in use, if not, may try merge chunks
    union {			// a union is large enough to hold its largest member
	char data[0];
	DList free;		// track free memory chunks that can be reused
    };
};

enum {
    NUM_SIZES = OS_HEAP_SIZE_BYTES / OS_HEAP_BLOCK_SIZE,
    ALIGN = OS_HEAP_BLOCK_SIZE,
    MIN_SIZE = sizeof(DList),
    HEADER_SIZE = OFFSETOF(Chunk, data),
};

Chunk *free_chunk[NUM_SIZES] = { NULL };
size_t mem_free = 0;
size_t mem_used = 0;
size_t mem_meta = 0;
Chunk *first = NULL;
Chunk *last = NULL;

static void memory_chunk_init(Chunk *chunk) {
	//printf("%s(%p)\n", __FUNCTION__, chunk);
    DLIST_INIT(chunk, all);
    chunk->used = 0; // set the used to 0
    DLIST_INIT(chunk, free);
}

static size_t memory_chunk_size(const Chunk *chunk) {
	//printf("%s(%p)\n", __FUNCTION__, chunk);
    char *end = (char*)(chunk->all.next);
    char *start = (char*)(&chunk->all);
    return (end - start) - HEADER_SIZE;
}

static int memory_chunk_slot(size_t size) {
    int n = -1;
    while(size > 0) {
	++n;
	size /= 2;
    }
    return n;
}

void kmemory_init(void *mem, size_t size) {
    char *mem_start = (char *)(((uintptr_t)mem + ALIGN - 1) & (~(ALIGN - 1)));
    char *mem_end = (char *)(((uintptr_t)mem + size) & (~(ALIGN - 1)));
    first = (Chunk*)mem_start;
    Chunk *second = first + 1;
    last = ((Chunk*)mem_end) - 1;
    memory_chunk_init(first);
    memory_chunk_init(second);
    memory_chunk_init(last);
    dlist_insert_after(&first->all, &second->all);
    dlist_insert_after(&second->all, &last->all);
    // make first/last as used so they never get merged
    first->used = 1;
    last->used = 1;

    size_t len = memory_chunk_size(second);
    int n = memory_chunk_slot(len);
    //printf("%s(%p, %#lx) : adding chunk %#lx [%d]\n", __FUNCTION__, mem, size, len, n);
    DLIST_PUSH(&free_chunk[n], second, free);
    mem_free = len - HEADER_SIZE;
    mem_meta = sizeof(Chunk) * 2 + HEADER_SIZE;
}

void *k_dl_mm_malloc(size_t size) {
    //printf("%s(%#lx)\n", __FUNCTION__, size);
    size = (size + ALIGN - 1) & (~(ALIGN - 1));

	if (size < MIN_SIZE) size = MIN_SIZE;

	int n = memory_chunk_slot(size - 1) + 1;

	if (n >= NUM_SIZES) return NULL;

	while(!free_chunk[n]) {
		++n;
		if (n >= NUM_SIZES) return NULL;
    }

	Chunk *chunk = DLIST_POP(&free_chunk[n], free);
    size_t size2 = memory_chunk_size(chunk);
	//printf("@ %p [%#lx]\n", chunk, size2);
    size_t len = 0;

	if (size + sizeof(Chunk) <= size2) {
		Chunk *chunk2 = (Chunk*)(((char*)chunk) + HEADER_SIZE + size);
		memory_chunk_init(chunk2);
		dlist_insert_after(&chunk->all, &chunk2->all);
		len = memory_chunk_size(chunk2);
		int n = memory_chunk_slot(len);
		//printf("  adding chunk @ %p %#lx [%d]\n", chunk2, len, n);
		DLIST_PUSH(&free_chunk[n], chunk2, free);
		mem_meta += HEADER_SIZE;
		mem_free += len - HEADER_SIZE;
    }

	chunk->used = 1;
    //memset(chunk->data, 0xAA, size);
	//printf("AAAA\n");
    mem_free -= size2;
    mem_used += size2 - len - HEADER_SIZE;
    //printf("  = %p [%p]\n", chunk->data, chunk);
    return chunk->data;
}

static void remove_free(Chunk *chunk) {
    size_t len = memory_chunk_size(chunk);
    int n = memory_chunk_slot(len);
    //printf("%s(%p) : removing chunk %#lx [%d]\n", __FUNCTION__, chunk, len, n);
    DLIST_REMOVE_FROM(&free_chunk[n], chunk, free);
    mem_free -= len - HEADER_SIZE;
}

static void push_free(Chunk *chunk) {
    size_t len = memory_chunk_size(chunk);
    int n = memory_chunk_slot(len);
    //printf("%s(%p) : adding chunk %#lx [%d]\n", __FUNCTION__, chunk, len, n);
    DLIST_PUSH(&free_chunk[n], chunk, free);
    mem_free += len - HEADER_SIZE;
}

void k_dl_mm_free(void *mem) {
    Chunk *chunk = (Chunk*)((char*)mem - HEADER_SIZE);
    Chunk *next = CONTAINER(Chunk, all, chunk->all.next);
    Chunk *prev = CONTAINER(Chunk, all, chunk->all.prev);

	//printf("%s(%p): @%p %#lx [%d]\n", __FUNCTION__, mem, chunk, memory_chunk_size(chunk), memory_chunk_slot(memory_chunk_size(chunk)));
    mem_used -= memory_chunk_size(chunk);

    if (next->used == 0) {
		// merge in next
		remove_free(next);
		dlist_remove(&next->all);
		//memset(next, 0xDD, sizeof(Chunk));
		mem_meta -= HEADER_SIZE;
		mem_free += HEADER_SIZE;
    }
    if (prev->used == 0) {
		// merge to prev
		remove_free(prev);
		dlist_remove(&chunk->all);
		//memset(chunk, 0xDD, sizeof(Chunk));
		push_free(prev);
		mem_meta -= HEADER_SIZE;
		mem_free += HEADER_SIZE;
    } else {
		// make chunk as free
		chunk->used = 0;
		DLIST_INIT(chunk, free);
		push_free(chunk);
    }
}

//#define MEM_SIZE (1024*1024*256)
//char MEM[MEM_SIZE] = { 0 };
//
//#define MAX_BLOCK (1024*1024)
//#define NUM_SLOTS 1024
//void *slot[NUM_SLOTS] = { NULL };

//void check(void) {
//	int	i;
//    Chunk *t = last;
//
//	DLIST_ITERATOR_BEGIN(first, all, it) {
//		assert(CONTAINER(Chunk, all, it->all.prev) == t);
//		t = it;
//    } DLIST_ITERATOR_END(it);
//
//    for(i = 0; i < NUM_SIZES; ++i) {
//		if (free_chunk[i]) {
//			t = CONTAINER(Chunk, free, free_chunk[i]->free.prev);
//			DLIST_ITERATOR_BEGIN(free_chunk[i], free, it) {
//			assert(CONTAINER(Chunk, free, it->free.prev) == t);
//			t = it;
//			} DLIST_ITERATOR_END(it);
//		}
//    }
//}

///*
// * BASED ON HARIBOTE.OS
// * https://github.com/HariboteOS/harib27f
// * haribote/memory.c
// */
//
//
///* 2GB MEMORY in 4KB Block */
//#define TOTAL_MEM_BLOCKS	524288
//#define BLOCK_SIZE_IN_BYTES	4096
//#include <stdint.h>
//
//struct FreeMemRegionEntry
//{
//	uint32_t addr, size;
//};
//
//struct MemoryManager
//{
//	int32_t total_entries_of_free_memory, highest_entry_index, lostsize, losts;
//	struct FreeMemRegionEntry region[TOTAL_MEM_BLOCKS];
//};
//
//
//struct MemoryManager* memory_manager_init(struct MemoryManager *mm)
//{
//	mm->total_entries_of_free_memory = 0;			/* あき情報の個数 */
//	mm->highest_entry_index = 0;		/* 状況観察用：freesの最大値 */
//	mm->lostsize = 0;			/* 解放に失敗した合計サイズ */
//	mm->losts = 0;				/* 解放に失敗した回数 */
//	return mm;
//}
//
//struct MemoryManager* memory_manager_add_address_to_pool(struct MemoryManager *mm, uint32_t start_address, uint32_t size)
//{
//	// TODO memory test first
//	// TODO merge with existing
//	mm->total_entries_of_free_memory += 1;
//	mm->highest_entry_index += 1;
//	mm->region[mm->highest_entry_index].addr = start_address;
//	mm->region[mm->highest_entry_index].size = size;
//	return mm;
//}
//
//uint32_t memory_manager_calculate_total_free_memory(struct MemoryManager *man)
///* あきサイズの合計を報告 */
//{
//	int32_t i, t = 0;
//	for (i = 0; i < man->total_entries_of_free_memory; i++) {
//		t += man->region[i].size;
//	}
//	return t;
//}
//
//int32_t _kadd(struct MemoryManager *mm, uint32_t addr, uint32_t size)
//{
//
//
//}
//
//uint32_t _kmalloc(struct MemoryManager *mm, uint32_t size)
///* 確保 */
//{
//	int32_t i, a;
//	for (i = 0; i < mm->total_entries_of_free_memory; i++) {
//		if (mm->region[i].size >= size) {
//			/* 十分な広さのあきを発見 */
//			a = mm->region[i].addr;
//			mm->region[i].addr += size;
//			mm->region[i].size -= size;
//			if (mm->region[i].size == 0) {
//				/* region[i]がなくなったので前へつめる */
//				mm->total_entries_of_free_memory--;
//				for (; i < mm->total_entries_of_free_memory; i++) {
//					mm->region[i] = mm->region[i + 1]; /* 構造体の代入 */
//				}
//			}
//			return a;
//		}
//	}
//	return 0; /* あきがない */
//}
//
////FIXME Potential memory leak, when used in init
//int32_t _kfree(struct MemoryManager *mm, uint32_t addr, uint32_t size)
///* 解放 */
//{
//	int32_t i, j, k;
//	/* まとめやすさを考えると、region[]がaddr順に並んでいるほうがいい */
//	/* だからまず、どこに入れるべきかを決める */
//	for (i = 0; i < mm->total_entries_of_free_memory; i++) {
//		if (mm->region[i].addr > addr) {
//			break;
//		}
//	}
//	for (k = i; k < mm->total_entries_of_free_memory; k++) {
//		if (mm->region[k].addr  > addr) {
//			break;
//		}
//	}
//	/* region[i - 1].addr < addr < region[i].addr */
//	if (i > 0) {
//		/* 前がある */
//		if (mm->region[i - 1].addr + mm->region[i - 1].size == addr) {
//			/* 前のあき領域にまとめられる */
//			mm->region[i - 1].size += size;
//			if (i < mm->total_entries_of_free_memory) {
//				/* 後ろもある */
//				if (addr + size == mm->region[i].addr) {
//					/* なんと後ろともまとめられる */
//					mm->region[i - 1].size += mm->region[i].size;
//					/* mm->region[i]の削除 */
//					/* region[i]がなくなったので前へつめる */
//					mm->total_entries_of_free_memory--;
//					for (; i < mm->total_entries_of_free_memory; i++) {
//						mm->region[i] = mm->region[i + 1]; /* 構造体の代入 */
//					}
//				}
//			}
//			return 0; /* 成功終了 */
//		}
//	}
//	/* 前とはまとめられなかった */
//	if (i < mm->total_entries_of_free_memory) {
//		/* 後ろがある */
//		if (addr + size == mm->region[i].addr) {
//			/* 後ろとはまとめられる */
//			mm->region[i].addr = addr;
//			mm->region[i].size += size;
//			return 0; /* 成功終了 */
//		}
//	}
//	/* 前にも後ろにもまとめられない */
//	if (mm->total_entries_of_free_memory < MemoryManager_FREES) {
//		/* region[i]より後ろを、後ろへずらして、すきまを作る */
//		for (j = mm->total_entries_of_free_memory; j > i; j--) {
//			mm->region[j] = mm->region[j - 1];
//		}
//		mm->total_entries_of_free_memory++;
//		if (mm->highest_entry_index < mm->total_entries_of_free_memory) {
//			mm->highest_entry_index = mm->total_entries_of_free_memory; /* 最大値を更新 */
//		}
//		mm->region[i].addr = addr;
//		mm->region[i].size = size;
//		return 0; /* 成功終了 */
//	}
//	/* 後ろにずらせなかった */
//	mm->losts++;
//	mm->lostsize += size;
//	return -1; /* 失敗終了 */
//}
//
///* 4k aligned */
//uint32_t kmalloc(struct MemoryManager *mm, uint32_t size)
//{
//	uint32_t a;
//	size = (size + 0xfff) & 0xfffff000;
//	a = _kmalloc(mm, size);
//	return a;
//}
//
///* 4k aligned */
//int kfree(struct MemoryManager *man, unsigned int addr, unsigned int size)
//{
//	int i;
//	size = (size + 0xfff) & 0xfffff000;
//	i = _kfree(man, addr, size);
//	return i;
//}
