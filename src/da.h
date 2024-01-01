/* Lightly customized variant of the da.h library */

#ifndef MANGO_DA_H
#define MANGO_DA_H

#ifndef DA_FACTOR
#	define DA_FACTOR 2
#endif

#include "alloc.h"

#define __da_s(a) (sizeof(*(a)->buf))

#define dainit(a, n) \
	do { \
		(a)->cap = n; \
		(a)->len = 0; \
		(a)->buf = alloc(NULL, (a)->cap, __da_s(a)); \
	} while (0)

#define dapush(a, x) \
	do { \
		if ((a)->len >= (a)->cap) { \
			(a)->cap = (a)->cap ? (a)->cap * DA_FACTOR : 1; \
			(a)->buf = alloc((a)->buf, (a)->cap, __da_s(a)); \
		} \
		(a)->buf[(a)->len++] = (x); \
	} while (0)

#define daremove(a, i) da_remove_range((a), (i), (i) + 1)

#define da_remove_range(a, i, j) \
	do { \
		memmove((a)->buf + (i), (a)->buf + (j), ((a)->len - (j)) * __da_s(a)); \
		(a)->len -= j - i; \
	} while (0)

/* TODO: typeof() is used here because Clang (I use the Clangd LSP) doesn’t yet
   understand what ‘auto’ means in a C23 context.  By the time Clang catches up,
   it would read nicer to rewrite this to use auto. */
#define da_foreach(a, p) \
	for (typeof((a)->buf) p = (a)->buf; p - (a)->buf < (ptrdiff_t)(a)->len; p++)

#define datopis(a, x) ((a)->len && (a)->buf[(a)->len - 1] == (x))

#endif /* !MANGO_DA_H */
