#include "bigint.h"

bool
isbigint(struct u8view sv)
{
	if (sv.len == 0)
		return false;

	bool neg = false;
	if (sv.p[0] == '-') {
		if (sv.len == 1)
			return false;
		VSHFT(&sv, 1);
		neg = true;
	}

	if (sv.p[0] == '0' && (neg || sv.len > 1))
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

	int ret;
	bool neg = false;

	if (x->p[0] == '-') {
		if (y->p[0] != '-')
			return -1;
		neg = true;
	} else if (y->p[0] == '-')
		return +1;

	if (x->len != y->len)
		ret = x->len > y->len ? +1 : -1;
	else for (size_t i = 0; i < x->len; i++) {
		if (x->p[i] != y->p[i]) {
			ret = x->p[i] > y->p[i] ? +1 : -1;
			break;
		}
	}

	return neg ? -ret : ret;
}
