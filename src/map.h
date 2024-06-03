#if !defined(MAPNAME) || !defined(KEYTYPE) || !defined(VALTYPE)                \
	|| !defined(NOTFOUND)
#	error "Missing required macro definitions"
#endif

#include <errno.h>
#include <stdckdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <alloc.h>
#include <dynarr.h>
#include <errors.h>
#include <macros.h>

#define K KEYTYPE
#define V VALTYPE

#if !MAPIMPL_MACRO_GUARD
#	define BKT     CONCAT(MAPNAME, bkt)
#	define PAIR    CONCAT(MAPNAME, pair)
#	define FUNC(f) CONCAT(MAPNAME, f)
#	define LOADF   0.75

#	define MAPIMPL_MACRO_GUARD 1
#endif

struct PAIR {
	K k;
	V v;
};

struct MAPNAME {
	struct BKT {
		dafields(struct PAIR)
	} *bkts;
	size_t len, cap;
};

static void FUNC(resz)(struct MAPNAME *, size_t);

struct MAPNAME
CONCAT(mk, MAPNAME)(void)
{
	constexpr size_t CAP = 32;
	struct MAPNAME m = {
		.cap = CAP,
		.bkts = bufalloc(nullptr, CAP, sizeof(struct BKT)),
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
}

void
FUNC(add)(struct MAPNAME *m, K k, V v)
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
			FUNC(vfree)(kv->v);
			kv->v = v;
			return;
		}
	}

	struct PAIR _kv = {k, v};
	DAPUSH(m->bkts + i, _kv);
	m->len++;
}

V
FUNC(get)(struct MAPNAME m, K k)
{
	size_t i = FUNC(hash)(k) % m.cap;
	da_foreach (m.bkts[i], kv) {
		if (FUNC(eq)(k, kv->k))
			return kv->v;
	}
	return NOTFOUND;
}

void
FUNC(del)(struct MAPNAME *m, K k)
{
	size_t i = FUNC(hash)(k) % m->cap;
	struct BKT *b = m->bkts + i;

	for (size_t j = 0; j < b->len; j++) {
		if (!FUNC(eq)(k, b->buf[j].k))
			continue;
		FUNC(kfree)(b->buf[j].k);
		FUNC(vfree)(b->buf[j].v);
		DAREMOVE(b, j);
		if (--m->len < (m->cap / 2) * LOADF)
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
	};
	memset(_m.bkts, 0, cap * sizeof(struct BKT));
	for (size_t i = 0; i < _m.cap; i++)
		_m.bkts[i].alloc = alloc_heap;
	for (size_t i = 0; i < m->cap; i++) {
		da_foreach (m->bkts[i], kv)
			FUNC(add)(&_m, kv->k, kv->v);
	}
	for (size_t i = 0; i < m->cap; i++)
		free(m->bkts->buf);
	free(m->bkts);
	*m = _m;
}

#undef K
#undef V
#undef MAPNAME
#undef KEYTYPE
#undef VALTYPE
#undef NOTFOUND
