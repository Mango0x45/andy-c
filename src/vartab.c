#include <stddef.h>
#include <stdlib.h>

#include <macros.h>
#include <mbstring.h>

static size_t
vartabhash(struct u8view sv)
{
	size_t h = 5381;
	for (size_t i = 0; i < sv.len; i++)
		h = ((h << 5) + h) + sv.p[i];
	return h;
}

static bool
vartabeq(struct u8view x, struct u8view y)
{
	return u8eq(x, y);
}

static void
vartabkfree(struct u8view sv)
{
	free((void *)sv.p);
}

static void
vartabvfree(struct u8view sv)
{
	free((void *)sv.p);
}

#define MAP_IMPLEMENTATION
#include "vartab.h"
