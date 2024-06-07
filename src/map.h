#if !defined(MAPNAME) || !defined(KEYTYPE) || !defined(VALTYPE)
#	error "Missing required macro definitions"
#endif

#include <errno.h>
#include <stdckdint.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <alloc.h>
#include <dynarr.h>
#include <errors.h>
#include <macros.h>
#include <mbstring.h>

#ifdef VARTAB
#	include "bigint.h"
#endif

#define K       KEYTYPE
#define V       VALTYPE
#define BKT     CONCAT(MAPNAME, bkt)
#define PAIR    CONCAT(MAPNAME, pair)
#define FUNC(f) CONCAT(MAPNAME, f)
#ifdef MAP_IMPLEMENTATION
#	define LOADF 0.75
#endif

struct PAIR {
	K k;
	V v;
};

struct MAPNAME {
	struct BKT {
		dafields(struct PAIR);
	} *bkts;
#ifdef VARTAB
	dynarr(struct u8view) numeric;
#endif
	size_t len, cap;
};

struct MAPNAME CONCAT(mk, MAPNAME)(void);
void FUNC(free)(struct MAPNAME);
V *FUNC(add)(struct MAPNAME *, K, V, bool);
V *FUNC(get)(struct MAPNAME, K);
void FUNC(del)(struct MAPNAME *, K);

#ifdef MAP_IMPLEMENTATION

static void FUNC(resz)(struct MAPNAME *, size_t);

struct MAPNAME
CONCAT(mk, MAPNAME)(void)
{
	constexpr size_t CAP = 32;
	struct MAPNAME m = {
		.cap = CAP,
		.bkts = bufalloc(nullptr, CAP, sizeof(struct BKT)),
#	ifdef VARTAB
		.numeric.alloc = alloc_heap,
#	endif
	};
	memset(m.bkts, 0, CAP * sizeof(struct BKT));
	for (size_t i = 0; i < CAP; i++)
		m.bkts[i].alloc = alloc_heap;
	return m;
}

void
FUNC(free)(struct MAPNAME m)
{
	for (size_t i = 0; i < m.cap; i++) {
		da_foreach (m.bkts[i], kv) {
			FUNC(kfree)(kv->k);
			FUNC(vfree)(kv->v);
		}
		free(m.bkts[i].buf);
	}
	free(m.bkts);
#	ifdef VARTAB
	free(m.numeric.buf);
#	endif
}

V *
FUNC(add)(struct MAPNAME *m, K k, V v, bool kfree)
{
	if (m->len + 1 >= m->cap * LOADF) {
		size_t ncap;
		if (ckd_mul(&ncap, m->cap, 2)) {
			errno = EOVERFLOW;
			err("%s:", __func__);
		}
		FUNC(resz)(m, m->cap * 2);
	}

	size_t i = FUNC(hash)(k) % m->cap;
	da_foreach (m->bkts[i], kv) {
		if (FUNC(eq)(k, kv->k)) {
			if (kfree)
				FUNC(kfree)(k);
			FUNC(vfree)(kv->v);
			kv->v = v;
			return &kv->v;
		}
	}

	struct PAIR _kv = {k, v};
	DAPUSH(m->bkts + i, _kv);
#	ifdef VARTAB
	if (isbigint(k)) {
		DAPUSH(&m->numeric, k);
		qsort(m->numeric.buf, m->numeric.len, sizeof(*m->numeric.buf),
		      bigintcmp);
	}
#	endif
	m->len++;
	return &m->bkts[i].buf[m->bkts[i].len - 1].v;
}

V *
FUNC(get)(struct MAPNAME m, K k)
{
	size_t i = FUNC(hash)(k) % m.cap;
	da_foreach (m.bkts[i], kv) {
		if (FUNC(eq)(k, kv->k))
			return &kv->v;
	}
	return nullptr;
}

void
FUNC(del)(struct MAPNAME *m, K k)
{
	size_t i = FUNC(hash)(k) % m->cap;
	struct BKT *b = m->bkts + i;

	for (size_t j = 0; j < b->len; j++) {
		struct PAIR p = b->buf[j];
		if (!FUNC(eq)(k, p.k))
			continue;

#	ifdef VARTAB
		if (isbigint(k)) {
			/* TODO: Perform binary search */
			for (size_t i = 0; i < m->numeric.len; i++) {
				if (ucseq(m->numeric.buf[i], k)) {
					DAREMOVE(&m->numeric, i);
					break;
				}
			}
		}
#	endif

		FUNC(kfree)(p.k);
		FUNC(vfree)(p.v);
		DAREMOVE(b, j);

		if (--m->len < (m->cap / 2) * LOADF && m->cap > 32)
			FUNC(resz)(m, m->cap / 2);
		return;
	}
}

void
FUNC(resz)(struct MAPNAME *m, size_t cap)
{
	struct MAPNAME _m = {
		.cap = cap,
		.bkts = bufalloc(nullptr, cap, sizeof(struct BKT)),
#	ifdef VARTAB
		.numeric.alloc = alloc_heap,
#	endif
	};
	memset(_m.bkts, 0, cap * sizeof(struct BKT));
	for (size_t i = 0; i < _m.cap; i++)
		_m.bkts[i].alloc = alloc_heap;
	for (size_t i = 0; i < m->cap; i++) {
		struct BKT b = m->bkts[i];
		da_foreach (b, kv)
			FUNC(add)(&_m, kv->k, kv->v, true);
		free(b.buf);
	}
	free(m->bkts);
#	ifdef VARTAB
	free(m->numeric.buf);
#	endif
	*m = _m;
}

#	undef MAP_IMPLEMENTATION
#endif /* MAP_IMPLEMENTATION */

#undef K
#undef V
#undef BKT
#undef PAIR
#undef FUNC
#undef MAPNAME
#undef KEYTYPE
#undef VALTYPE

#ifdef VARTAB
#	undef VARTAB
#endif
