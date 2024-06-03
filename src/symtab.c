#include <stddef.h>
#include <stdlib.h>

#include <macros.h>
#include <mbstring.h>

#include "vartab.h"

static size_t
symtabhash(struct u8view sv)
{
	size_t h = 5381;
	for (size_t i = 0; i < sv.len; i++)
		h = ((h << 5) + h) + sv.p[i];
	return h;
}

static bool
symtabeq(struct u8view x, struct u8view y)
{
	return u8eq(x, y);
}

static void
symtabkfree(struct u8view sv)
{
	free((void *)sv.p);
}

static void
symtabvfree(struct vartab vt)
{
	vartabfree(vt);
}

#define MAPNAME symtab
#define KEYTYPE struct u8view
#define VALTYPE struct vartab
#include "map.h"
