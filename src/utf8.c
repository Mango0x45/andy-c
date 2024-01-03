#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <uchar.h>

#include "utf8.h"

rune
utf8peek(const char8_t *s)
{
	int cb;
	rune ch;

	if ((*s & 0x80) == 0) {
		cb = 0;
		ch = *s;
	} else if ((*s & 0xE0) == 0xC0) {
		cb = 1;
		ch = *s & 0x1F;
	} else if ((*s & 0xF0) == 0xE0) {
		cb = 2;
		ch = *s & 0x0F;
	} else if ((*s & 0xF8) == 0xF0) {
		cb = 3;
		ch = *s & 0x07;
	} else
		return REPL_CHAR;

	for (int i = 0; i < cb; i++) {
		if ((s[i + 1] & 0xC0) != 0x80)
			return REPL_CHAR;
		ch = (ch << 6) | (s[i + 1] & 0x3F);
	}

	return ch;
}

rune
utf8next(const char8_t **s)
{
	rune ch = utf8peek(*s);
	*s += utf8wdth(ch);
	return ch;
}

char8_t *
utf8trim(char8_t *s)
{
	rune ch;
	char8_t *p;

	s = p = utf8fskip(s, risblank);

	while ((ch = utf8next((const char8_t **)&p))) {
		if (utf8all(p, risblank)) {
			*p = 0;
			break;
		}
	}

	return s;
}

bool
utf8all(const char8_t *s, bool (*pfn)(rune))
{
	rune ch;

	while ((ch = utf8next(&s))) {
		if (!pfn(ch))
			return false;
	}

	return true;
}

char8_t *
utf8fskip(const char8_t *s, bool (*pfn)(rune))
{
	rune ch;

	while ((ch = utf8next(&s))) {
		if (!pfn(ch))
			break;
	}

	return (char8_t *)s - utf8wdth(ch);
}

int
utf8wdth(rune ch)
{
	return ch <= 0x7F     ? 1
	     : ch <= 0x7FF    ? 2
	     : ch <= 0xFFFF   ? 3
	     : ch <= 0x10FFFF ? 4
	                      : 0;
}

char8_t *
utf8chr(const char8_t *haystack, rune needle)
{
	if (needle <= UCHAR_MAX)
		return (char8_t *)strchr((char *)haystack, needle);

	for (rune ch; (ch = utf8next(&haystack));) {
		if (ch == needle)
			return (char8_t *)haystack;
	}

	return nullptr;
}

size_t
utf8pfx(const char8_t *s, rune ch)
{
	return utf8npfx(s, ch, SIZE_MAX);
}

size_t
utf8npfx(const char8_t *s, rune ch, size_t mx)
{
	size_t n = 0;

	while (n < mx && (utf8next(&s) == ch))
		n += utf8wdth(ch);

	return n;
}

bool
risblank(rune ch)
{
	return ch == ' ' || ch == '\t';
}
