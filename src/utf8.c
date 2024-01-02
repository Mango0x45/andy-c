#include <uchar.h>

#include "uni.h"
#include "utf8.h"

rune
utf8peek(const char8_t *s)
{
	rune ch;

	if ((*s & 0x80) == 0) {
		ch = *s;
	} else if ((*s & 0xE0) == 0xC0) {
		ch = *s & 0x1F;
		ch = (ch << 6) | (s[1] & 0x3F);
	} else if ((*s & 0xF0) == 0xE0) {
		ch = *s & 0x0F;
		ch = (ch << 6) | (s[1] & 0x3F);
		ch = (ch << 6) | (s[2] & 0x3F);
	} else if ((*s & 0xF8) == 0xF0) {
		ch = *s & 0x07;
		ch = (ch << 6) | (s[1] & 0x3F);
		ch = (ch << 6) | (s[2] & 0x3F);
		ch = (ch << 6) | (s[3] & 0x3F);
	} else
		return UNI_REPL_CHAR;

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

	s = p = utf8fskip(s, unispace);

	while ((ch = utf8next((const char8_t **)&p))) {
		if (utf8all(p, unispace)) {
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
