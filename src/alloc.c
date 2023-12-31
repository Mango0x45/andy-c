#include <err.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "alloc.h"

void *
alloc(void *p, size_t n, size_t m)
{
	if (n && SIZE_MAX / n < m) {
		errno = EOVERFLOW;
		err(EXIT_FAILURE, __func__);
	}
	if (!(p = realloc(p, n * m)))
		err(EXIT_FAILURE, __func__);
	return p;
}
