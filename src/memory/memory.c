// memory/memory.c
#include "memory/memory.h"
#include "memory/kheap.h"
#include "config.h"
#include "util/kutil.h"
#include "io/io.h"
#include "status.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

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

/* Memory Test */
uintptr_t kmemtest_subtest(uintptr_t mem_start, uintptr_t mem_end)
{
	uintptr_t addr_current;
	volatile uint32_t *p;
	uint32_t original_data, pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;
	const uint32_t step = 0x1000; // 4KB
	for (addr_current = mem_start; addr_current < mem_end; addr_current += step)
	{
		p = (uint32_t *) addr_current;
		original_data = *p;
		*p = pat0;
		*p = ~*p;
		if (*p != pat1)
		{
// not_memory:
			*p = original_data;
			return addr_current;
		}
		*p = ~*p;
		if (*p != pat0)
		{
			*p = original_data;
			// goto not_memory;
			return addr_current;
		}
		*p = original_data;
	}
	return addr_current - step;
}

/*
 * Memory Test.
 * If 486 or above, the function make sure cache is disabled during the test.
 */
uintptr_t kmemtest(uintptr_t mem_start, uintptr_t mem_end)
{
	bool is_486_or_above = false;
	/* CPU >= 486? */
	const uint32_t eflagsbk = _io_get_eflags();
	uint32_t eflags = eflagsbk;
	eflags |= EFLAGS_MASK_AC;
	_io_set_eflags(eflags);
	eflags = _io_get_eflags();
	/* 386 will auto reset the EFLAGS_MASK_AC bit to 0 */
	if (isMaskBitsAllSet(eflags, EFLAGS_MASK_AC))
		is_486_or_above = true;
	_io_set_eflags(eflagsbk);

	const uint32_t cr0bk = _io_get_cr0();
	if (is_486_or_above)
	{
		/* Disable cache during the memory test */
		_io_set_cr0(cr0bk | CR0_MASK_CD);
	}

	int32_t res = kmemtest_subtest(mem_start, mem_end);

	if (is_486_or_above)
	{
		/* Cleanup, reset the original cr0 flags */
		_io_set_cr0(cr0bk);
	}
	return res;
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
typedef struct DLIST {
    struct DLIST *next;
    struct DLIST *prev;
} DLIST;

/* Initialize a one element *circular Doubly-Linked list* */
static inline void dlist_init(DLIST *dlist) {
	//printf("%s(%p)\n", __FUNCTION__, dlist);
    dlist->next = dlist;
    dlist->prev = dlist;
}

/* Insert dnew after d0 */
static inline void dlist_insert_after(DLIST *d0, DLIST *dnew) {
	//printf("%s(%p, %p)\n", __FUNCTION__, d0, dnew);
    DLIST *n1 = d0->next; // save the original next node
    DLIST *e2 = dnew->prev; // save the original "tail" node of the 2nd DLIST, the DLIST is circular

    d0->next = dnew;
    dnew->prev = d0;

    e2->next = n1; // end of 2nd DLIST, points to original next node
    n1->prev = e2; // original next node, points to original "nail" node of 2nd DLIST
}

/* Insert dnew before d0 */
static inline void dlist_insert_before(DLIST *d0, DLIST *dnew) {
	//printf("%s(%p, %p)\n", __FUNCTION__, d0, dnew);
    DLIST *e1 = d0->prev; // save the end of the original list
    DLIST *e2 = dnew->prev; // save the end of the 2nd list

    e1->next = dnew; // end of 1, next be the 2
    dnew->prev = e1; // dnew, prev be the original list
    e2->next = d0; // e2, d0
    d0->prev = e2; // d0, e2
}

/* Remove d from the list */
static inline void dlist_remove(DLIST *d) {
	//printf("%s(%p)\n", __FUNCTION__, d);
    d->prev->next = d->next;
    d->next->prev = d->prev;
    d->next = d;
    d->prev = d;
}

/* push dnew to the front of the d0p list */
static inline void dlist_push(DLIST **d0p, DLIST *dnew) {
	//printf("%s(%p, %p)\n", __FUNCTION__, d0p, dnew);
    if (*d0p != NULL) {
	dlist_insert_before(*d0p, dnew);
    }
    *d0p = dnew;
}

// pop the front of the dp list
static inline DLIST * dlist_pop(DLIST **d0p) {
	//printf("%s(%p)\n", __FUNCTION__, d0p);
    DLIST *d0 = *d0p;
    DLIST *dnew = d0->next;
    dlist_remove(d0);
    if (d0 == dnew) {
	*d0p = NULL;
    } else {
	*d0p = dnew;
    }
    return d0;
}

// remove dnew from the list, advancing d0p if needed
static inline void dlist_remove_from(DLIST **d0p, DLIST *dnew) {
	//printf("%s(%p, %p)\n", __FUNCTION__, d0p, dnew);
    if (*d0p == dnew) {
	dlist_pop(d0p);
    } else {
	dlist_remove(dnew);
    }
}


// #define CONTAINER(C, l, v) ((C*)(((char*)v) - (intptr_t)&(((C*)0)->l)))
/* gcc extension, returns the member's position in type (e.g. a struct) in bytes */
/*
* #define OFFSETOF(TYPE, MEMBER)  __builtin_offsetof (TYPE, MEMBER)
* #define DLIST_INIT(v, l) dlist_init(&v->l)
* 
* #define DLIST_REMOVE_FROM(h, d, l)					\
*     {									\
* 	typeof(**h) **h_ = h, *d_ = d;					\
* 	DLIST *head = &(*h_)->l;					\
* 	dlist_remove_from(&head, &d_->l);					\
* 	if (head == NULL) {						\
* 	    *h_ = NULL;							\
* 	} else {							\
* 	    *h_ = CONTAINER(typeof(**h), l, head);			\
* 	}								\
*     }
* 
* // DLIST_PUSH(header=(CHUNK **)&free_chunk[n], val= (CHUNK *)second, dlist="free");
* #define DLIST_PUSH(header, val, dl_name)				\
*     {									\
* 	typeof(*val) **h_ = header, *v_ = val;				\
* 	DLIST *head = &(*h_)->dl_name;					\
* 	if (*h_ == NULL) head = NULL;					\
* 	dlist_insert_before(head, &v_->dl_name)			\
* 	*h_ = CONTAINER(typeof(*val), dl_name, head);			\
*     }
* 
* #define DLIST_POP(h, l)							\
*     ({									\
* 	typeof(**h) **h_ = h;						\
* 	DLIST *head = &(*h_)->l;					\
* 	DLIST *res = dlist_pop(&head);					\
* 	if (head == NULL) {						\
* 	    *h_ = NULL;							\
* 	} else {							\
* 	    *h_ = CONTAINER(typeof(**h), l, head);			\
* 	}								\
* 	CONTAINER(typeof(**h), l, res);					\
*     })
* 
* #define DLIST_ITERATOR_BEGIN(h, l, it)					\
*     {									\
*         typeof(*h) *h_ = h;						\
* 	DLIST *last_##it = h_->l.prev, *iter_##it = &h_->l, *next_##it;	\
* 	do {								\
* 	    if (iter_##it == last_##it) {				\
* 		next_##it = NULL;					\
* 	    } else {							\
* 		next_##it = iter_##it->next;				\
* 	    }								\
* 	    typeof(*h)* it = CONTAINER(typeof(*h), l, iter_##it);
* 
* #define DLIST_ITERATOR_END(it)						\
* 	} while((iter_##it = next_##it));				\
*     }
* 
* #define DLIST_ITERATOR_REMOVE_FROM(h, it, l) DLIST_REMOVE_FROM(h, iter_##it, l)
*/
//  /*
//   * The heap.
//   * HEADER + HEAP_DATA
//   * DLIST all, is not a circle, connected to next CHUNK->DLIST
//   */
//  typedef struct CHUNK {
//      DLIST all;			// track all memory chunks in the system
//      bool isUsed;		// is chunk in use, if not, may try merge chunks
//      union {			// a union is large enough to hold its largest member
//  	char data[0];
//  	DLIST free;		// track free memory chunks that can be reused
//      };
//  } CHUNK;
//  
//  enum {
//  	/* 112MB / 4096 = Max CHUNK numbers */
//  	MAX_TOTAL_CHUNKS = OS_HEAP_SIZE_BYTES / OS_HEAP_BLOCK_SIZE,
//      	ALIGN = OS_HEAP_BLOCK_SIZE,
//      	MIN_SIZE = sizeof(DLIST),
//      	/*  sizeof(DLIST) + sizeof(uint32_t) ? */
//      	HEADER_SIZE = OFFSETOF(CHUNK, data),
//  };
//  
//  CHUNK *free_chunk[MAX_TOTAL_CHUNKS] = { NULL };
//  size_t mem_free = 0;
//  size_t mem_used = 0;
//  size_t mem_meta = 0;
//  CHUNK *first = NULL;
//  CHUNK *last = NULL;
//  
//  /*
//   * Init a CHUNK
//   * chunk->all = &chunk->all;
//   * chunk->free = &chunk->free;
//   * isused = false;
//   * Everything is supposed to be available and unused.
//   */
//  static void memory_chunk_init(CHUNK *chunk) {
//  	//printf("%s(%p)\n", __FUNCTION__, chunk);
//      DLIST_INIT(chunk, all);
//      chunk->isUsed = false;
//      DLIST_INIT(chunk, free);
//  }
//  
//  /* Return the available size of a CHUNK */
//  static size_t memory_chunk_size(const CHUNK *chunk) {
//  	//printf("%s(%p)\n", __FUNCTION__, chunk);
//      char *end = (char*)(chunk->all.next);
//      char *start = (char*)(&chunk->all);
//      return (end - start) - HEADER_SIZE;
//  }
//  
//  static int32_t memory_chunk_slot(size_t size) {
//      int n = -1;
//      while(size > 0) {
//  	++n;
//  	size /= 2;
//      }
//      return n;
//  }
//  
//  /*
//   * Return the closest upper value that align.
//   * e.g., 50->4096; 4097 -> 8192;
//   */
//  static uintptr_t align_address_to_upper(uintptr_t val)
//  {
//  	uintptr_t residual = val % ALIGN;
//      	if (residual == 0)
//      	        return val;
//  	val += (ALIGN - residual); // ALIGN >= residual
//      	return val;
//  }
//  
//  /*
//   * Return the closest lower value that align.
//   * e.g., 4097 -> 4096; 8193 -> 8192;
//   */
//  static uintptr_t align_address_to_lower(uintptr_t val)
//  {
//  	uintptr_t residual = val % ALIGN;
//      	if (residual == 0)
//      	{
//      	        return val;
//      	}
//      	val -= residual; // val >= residual
//      	return val;
//  }
//  
//  void kmemory_init(void *mem, size_t size) {
//  	uint8_t *mem_start = (uint8_t *)align_address_to_upper((uintptr_t)mem);
//  	uint8_t *mem_end = (uint8_t *)align_address_to_lower((uintptr_t)mem + size);
//     	first = (CHUNK*)mem_start;
//     	CHUNK *second = first + 1;
//     	last = ((CHUNK*)mem_end) - 1;
//     	memory_chunk_init(first);
//     	memory_chunk_init(second);
//     	memory_chunk_init(last);
//  	/* &Chunk->DLIST, link `DLIST all` */
//     	dlist_insert_after(&first->all, &second->all);
//     	dlist_insert_after(&second->all, &last->all);
//     	/* Make first/last as used so they never get merged */
//     	first->isUsed = true;
//     	last->isUsed = true;
//  
//     	size_t free_bytes_in_chunk_2 = memory_chunk_size(second);
//  	/* log(2) */
//     	int32_t n = memory_chunk_slot(free_bytes_in_chunk_2);
//  	/*
//  	 * CHUNK *free_chunk[MAX_TOTAL_CHUNKS] = { NULL };
//  	 *
//  	 * DLIST_PUSH(header=&free_chunk[n], val=second, dlist=free);
//  	 * {
//  	 * 	typeof(*second) **h_ = &free_chunk[19] // CHUNK **h_ = (CHUNK **) free_chunk
//  	 * 	DLIST *head = &((CHUNK *)(*free_chunk))->free
//  	 * 	if ((CHUNK *)(*free_chunk) == NULL) head = NULL; // after using null ptr?
//  	 * 	// dlist_push() // ? why not using pointer directly?
//  	 *	// dlist_insert_before(DLIST *d0, DLIST *dnew)
//  	 *	CHUNK *v_= (CHUNK *)second;
//  	 *	dlist_insert_before(head, &second->free)
//  	 *
//  	 * }
//  	 *
//  	 */
//     	DLIST_PUSH(&free_chunk[n], second, free);
//     	mem_free = free_bytes_in_chunk_2 - HEADER_SIZE;
//     	mem_meta = sizeof(CHUNK) * 2 + HEADER_SIZE;
//  }
//  
//  void *k_dl_mm_malloc(size_t size) {
//      //printf("%s(%#lx)\n", __FUNCTION__, size);
//      size = align_address_to_upper(size);
//  
//  	if (size < MIN_SIZE) size = MIN_SIZE;
//  
//  	int32_t n = memory_chunk_slot(size - 1) + 1;
//  
//  	if (n >= MAX_TOTAL_CHUNKS) return NULL;
//  
//  	while(!free_chunk[n]) {
//  		++n;
//  		if (n >= MAX_TOTAL_CHUNKS) return NULL;
//      }
//  
//  	CHUNK *chunk = DLIST_POP(&free_chunk[n], free);
//      size_t size2 = memory_chunk_size(chunk);
//  	//printf("@ %p [%#lx]\n", chunk, size2);
//      size_t len = 0;
//  
//  	if (size + sizeof(CHUNK) <= size2) {
//  		CHUNK *chunk2 = (CHUNK*)(((char*)chunk) + HEADER_SIZE + size);
//  		memory_chunk_init(chunk2);
//  		dlist_insert_after(&chunk->all, &chunk2->all);
//  		len = memory_chunk_size(chunk2);
//  		int n = memory_chunk_slot(len);
//  		//printf("  adding chunk @ %p %#lx [%d]\n", chunk2, len, n);
//  		DLIST_PUSH(&free_chunk[n], chunk2, free);
//  		mem_meta += HEADER_SIZE;
//  		mem_free += len - HEADER_SIZE;
//      }
//  
//  	chunk->isUsed = 1;
//      //memset(chunk->data, 0xAA, size);
//  	//printf("AAAA\n");
//      mem_free -= size2;
//      mem_used += size2 - len - HEADER_SIZE;
//      //printf("  = %p [%p]\n", chunk->data, chunk);
//      return chunk->data;
//  }
//  
//  static void remove_free(CHUNK *chunk) {
//      size_t len = memory_chunk_size(chunk);
//      int n = memory_chunk_slot(len);
//      //printf("%s(%p) : removing chunk %#lx [%d]\n", __FUNCTION__, chunk, len, n);
//      DLIST_REMOVE_FROM(&free_chunk[n], chunk, free);
//      mem_free -= len - HEADER_SIZE;
//  }
//  
//  static void push_free(CHUNK *chunk) {
//      size_t len = memory_chunk_size(chunk);
//      int32_t n = memory_chunk_slot(len);
//      //printf("%s(%p) : adding chunk %#lx [%d]\n", __FUNCTION__, chunk, len, n);
//      DLIST_PUSH(&free_chunk[n], chunk, free);
//      mem_free += len - HEADER_SIZE;
//  }
//  
//  void k_dl_mm_free(void *mem) {
//      CHUNK *chunk = (CHUNK*)((char*)mem - HEADER_SIZE);
//      CHUNK *next = CONTAINER(CHUNK, all, chunk->all.next);
//      CHUNK *prev = CONTAINER(CHUNK, all, chunk->all.prev);
//  
//  	//printf("%s(%p): @%p %#lx [%d]\n", __FUNCTION__, mem, chunk, memory_chunk_size(chunk), memory_chunk_slot(memory_chunk_size(chunk)));
//      mem_used -= memory_chunk_size(chunk);
//  
//      if (next->isUsed == 0) {
//  		// merge in next
//  		remove_free(next);
//  		dlist_remove(&next->all);
//  		//memset(next, 0xDD, sizeof(CHUNK));
//  		mem_meta -= HEADER_SIZE;
//  		mem_free += HEADER_SIZE;
//      }
//      if (prev->isUsed == 0) {
//  		// merge to prev
//  		remove_free(prev);
//  		dlist_remove(&chunk->all);
//  		//memset(chunk, 0xDD, sizeof(CHUNK));
//  		push_free(prev);
//  		mem_meta -= HEADER_SIZE;
//  		mem_free += HEADER_SIZE;
//      } else {
//  		// make chunk as free
//  		chunk->isUsed = 0;
//  		DLIST_INIT(chunk, free);
//  		push_free(chunk);
//      }
//  }
//  
//#define MEM_SIZE (1024*1024*256)
//char MEM[MEM_SIZE] = { 0 };
//
//#define MAX_BLOCK (1024*1024)
//#define NUM_SLOTS 1024
//void *slot[NUM_SLOTS] = { NULL };

//void check(void) {
//	int	i;
//    CHUNK *t = last;
//
//	DLIST_ITERATOR_BEGIN(first, all, it) {
//		assert(CONTAINER(CHUNK, all, it->all.prev) == t);
//		t = it;
//    } DLIST_ITERATOR_END(it);
//
//    for(i = 0; i < NUM_SIZES; ++i) {
//		if (free_chunk[i]) {
//			t = CONTAINER(CHUNK, free, free_chunk[i]->free.prev);
//			DLIST_ITERATOR_BEGIN(free_chunk[i], free, it) {
//			assert(CONTAINER(CHUNK, free, it->free.prev) == t);
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
