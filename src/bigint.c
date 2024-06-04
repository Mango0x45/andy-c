#include "bigint.h"

bool
isbigint(struct u8view sv)
{
	if (sv.len == 0)
		return false;
	if (sv.p[0] == '0' && sv.len > 1)
		return false;
	for (size_t i = 0; i < sv.len; i++) {
		if (sv.p[i] < '0' || sv.p[i] > '9')
			return false;
	}
	return true;
}

int
bigintcmp(const void *_x, const void *_y)
{
	struct u8view *x = (struct u8view *)_x;
	struct u8view *y = (struct u8view *)_y;
	if (x->len != y->len)
		return x->len > y->len ? +1 : -1;
	for (size_t i = 0; i < x->len; i++) {
		if (x->p[i] != y->p[i])
			return x->p[i] > y->p[i] ? +1 : -1;
	}
	return 0;
}
