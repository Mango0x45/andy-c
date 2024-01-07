#ifndef ANDY_ALLOC_H
#define ANDY_ALLOC_H

#include <stddef.h>

/* Allocate space for n items of size m.  If p is null, a newly allocated buffer
   is returned.  If p is non-null then it is resized to fit the given size.

   On error this function terminates the process. */
[[nodiscard, gnu::returns_nonnull]] void *alloc(void *p, size_t n, size_t m);

#endif /* !ANDY_ALLOC_H */
