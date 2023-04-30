/* SPDX-License-Identifier: GPL-2.0 */
#ifndef UTIL_CONTAINER_OF_H_
#define UTIL_CONTAINER_OF_H_


#include <stddef.h>

/* Are two types/vars the same type (ignoring qualifiers)? */
#ifndef __same_type
# define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#endif

#undef offsetof
#define offsetof(TYPE, MEMBER)	__builtin_offsetof(TYPE, MEMBER)

#define typeof_member(T, m)	typeof(((T*)0)->m)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 * WARNING: any const qualifier of @ptr is lost.
 */
#define container_of(ptr, type, member) ({				\
	void *__mptr = (void *)(ptr);					\
	_Static_assert(__same_type(*(ptr), ((type *)0)->member) ||	\
		      __same_type(*(ptr), void),			\
		      "pointer type mismatch in container_of()");	\
	((type *)(__mptr - offsetof(type, member))); })

/**
 * container_of_const - cast a member of a structure out to the containing
 *			structure and preserve the const-ness of the pointer
 * @ptr:		the pointer to the member
 * @type:		the type of the container struct this is embedded in.
 * @member:		the name of the member within the struct.
 */
#define container_of_const(ptr, type, member)				\
	_Generic(ptr,							\
		const typeof(*(ptr)) *: ((const type *)container_of(ptr, type, member)),\
		default: ((type *)container_of(ptr, type, member))	\
	)

#endif	/* _LINUX_CONTAINER_OF_H */
