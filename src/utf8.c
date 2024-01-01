#include <uchar.h>

#include "uni.h"
#include "utf8.h"

rune_t
utf8iter(const char8_t *s, size_t *di)
{
	int w;
	rune_t ch;
	char8_t b = s[*di];

	if (b < 0x80) {
		ch = b;
		w = 1;
	} else if (b < 0xE0) {
		ch = b & 0x1F;
		w = 2;
	} else if (b < 0xF0) {
		ch = b & 0x0F;
		w = 3;
	} else {
		ch = b & 0x07;
		w = 4;
	}

	for (int i = 1; i < w; i++) {
		if ((s[*di + i] & 0xC0) != 0x80)
			return UNI_REPL_CHAR;
		ch = (ch << 6) | (s[*di + i] & 0x3F);
	}

	*di += w;
	return ch;
}

char8_t *
utf8trim(char8_t *s)
{
	rune_t ch;
	size_t i = 0;

	for (size_t j = 0; (ch = utf8iter(s, &j)) && unispace(ch); i = j)
		;
	s += i;
	for (i = 0; (ch = utf8iter(s, &i));) {
		if (utf8all(s + i, unispace)) {
			s[i] = 0;
			break;
		}
	}
	return s;
}

bool
utf8all(const char8_t *s, bool (*pfn)(rune_t))
{
	rune_t ch;
	size_t i = 0;

	while ((ch = utf8iter(s, &i))) {
		if (!pfn(ch))
			return false;
	}

	return true;
}

char8_t *
utf8skipf(const char8_t *s, bool (*pfn)(rune_t))
{
	rune_t ch;
	size_t i = 0;

	while ((ch = utf8iter(s, &i))) {
		if (!pfn(ch))
			break;
	}

	return (char8_t *)s + i - utf8wdth(ch);
}

int
utf8wdth(rune_t ch)
{
	if (ch <= 0x7F)
		return 1;
	if (ch <= 0x7FF)
		return 2;
	if (ch <= 0xFFFF)
		return 3;
	if (ch <= 0x10FFFF)
		return 4;
	return 0;
}
