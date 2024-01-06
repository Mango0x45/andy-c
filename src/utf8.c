#include <sys/types.h>

#include <stdint.h>
#if HAS_STRCHRNUL
#	include <string.h>
#endif
#include <uchar.h>

#include "utf8.h"
#include "util.h"

#include "gen/xid.h"

#define ASCII_MAX (0x7F)

#define U1(x) (((x)&0b1000'0000) == 0b0000'0000)
#define U2(x) (((x)&0b1110'0000) == 0b1100'0000)
#define U3(x) (((x)&0b1111'0000) == 0b1110'0000)
#define U4(x) (((x)&0b1111'1000) == 0b1111'0000)

static bool rtbl_bsearch(size_t n, const rune[n][2], rune);
#if !HAS_STRCHRNUL
static char *strchrnul(const char *, int);
#endif

char8_t *
c8chk(const char8_t *s)
{
	/* B == ‘Between’, L == ‘Literal’, T == ‘Tail’ */
#define B(lo, i, hi) ((0x##lo) <= s[(i)] && s[(i)] <= (0x##hi))
#define L(i, x)      (s[(i)] == (0x##x))
#define T(i)         (B(80, i, BF))

	/* https://www.rfc-editor.org/rfc/rfc3629#section-4 */
	for (size_t i = 0; s[i];) {
		if (B(00, i, 7F))
			i += 1;
		else if (B(C2, i, DF) && T(i + 1))
			i += 2;
		else if (L(i, E0) && B(A0, i + 1, BF) && T(i + 2))
			i += 3;
		else if (L(i, ED) && B(80, i + 1, 9F) && T(i + 2))
			i += 3;
		else if (B(E1, i, EC) && T(i + 1) && T(i + 2))
			i += 3;
		else if (B(EE, i, EF) && T(i + 1) && T(i + 2))
			i += 3;
		else if (L(i, F0) && B(90, i + 1, BF) && T(i + 2) && T(i + 3))
			i += 4;
		else if (L(i, F4) && B(80, i + 1, 8F) && T(i + 2) && T(i + 3))
			i += 4;
		else if (B(F1, i, F3) && T(i + 1) && T(i + 2) && T(i + 3))
			i += 4;
		else
			return (char8_t *)s + i;
	}

	return nullptr;

#undef T
#undef L
#undef B
}

char8_t *
c8fwd(const char8_t *s)
{
	if (U1(*s))
		return (char8_t *)s + 1;
	if (U2(*s))
		return (char8_t *)s + 2;
	if (U3(*s))
		return (char8_t *)s + 3;
	if (U4(*s))
		return (char8_t *)s + 4;
	unreachable();
}

char8_t *
c8chrnul(const char8_t *s, rune ᚱ)
{
	if (ᚱ <= ASCII_MAX)
		return (char8_t *)strchrnul((char *)s, ᚱ);
	for (rune ᚹ; (ᚹ = c8tor(s)) && ᚹ != ᚱ; s = c8fwd(s))
		;
	return (char8_t *)s;
}

char8_t *
c8pbrknul(const char8_t *s, const char8_t *accept)
{
	for (rune ᚱ; (ᚱ = c8tor(s)) && !*c8chrnul(accept, ᚱ); s = c8fwd(s))
		;
	return (char8_t *)s;
}

char8_t *
c8pcbrknul(const char8_t *s, const char8_t *reject)
{
	for (rune ᚱ; (ᚱ = c8tor(s)) && *c8chrnul(reject, ᚱ); s = c8fwd(s))
		;
	return (char8_t *)s;
}

size_t
c8rspn(const char8_t *s, rune ᚱ)
{
	return c8nrspn(s, ᚱ, SIZE_MAX);
}

size_t
c8nrspn(const char8_t *s, rune ᚱ, size_t n)
{
	const char8_t *p = s;
	while ((size_t)(p - s) < n && c8tor(p) == ᚱ)
		p = c8fwd(p);
	return p - s;
}

rune
c8tor(const char8_t *s)
{
	if (U1(*s))
		return *s;
	if (U2(*s))
		return ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
	if (U3(*s))
		return ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
	if (U4(*s))
		return ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12)
		     | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
	unreachable();
}

size_t
rwdth(rune ᚱ)
{
	if (ᚱ <= 0x7F)
		return 1;
	if (ᚱ <= 0x7FF)
		return 2;
	if (ᚱ <= 0xFFFF)
		return 3;
	if (ᚱ <= 0x10FFFF)
		return 4;
	unreachable();
}

bool
risbndry(rune ᚱ)
{
	return ᚱ == ' ' || ᚱ == '\t' || ᚱ == '\n' || ᚱ == '\0';
}

bool
rtbl_bsearch(size_t n, const rune tbl[n][2], rune ᚱ)
{
	ssize_t lo, hi;
	lo = 0;
	hi = n - 1;

	while (lo <= hi) {
		ssize_t i = (lo + hi) / 2;

		if (ᚱ < tbl[i][0])
			hi = i - 1;
		else if (ᚱ > tbl[i][1])
			lo = i + 1;
		else
			return true;
	}

	return false;
}

bool
risstart(rune ᚱ)
{
	return rtbl_bsearch(lengthof(xid_start_tbl), xid_start_tbl, ᚱ);
}

bool
riscont(rune ᚱ)
{
	return rtbl_bsearch(lengthof(xid_cont_tbl), xid_cont_tbl, ᚱ);
}

#if !HAS_STRCHRNUL
char *
strchrnul(const char *p, int c)
{
	while (*p && *p != c)
		p++;
	return (char *)p;
}
#endif
