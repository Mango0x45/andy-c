#include <uchar.h>

#include "uni.h"
#include "utf8.h"

rune_t
utf8iter(const char8_t *s, size_t *di)
{
	int w;
	rune_t cp;
	char8_t b = s[*di];

	if (b < 0x80) {
		cp = b;
		w = 1;
	} else if (b < 0xE0) {
		cp = b & 0x1F;
		w = 2;
	} else if (b < 0xF0) {
		cp = b & 0x0F;
		w = 3;
	} else {
		cp = b & 0x07;
		w = 4;
	}

	for (int i = 1; i < w; i++) {
		if ((s[*di + i] & 0xC0) != 0x80)
			return UNI_REPL_CHAR;
		cp = (cp << 6) | (s[*di + i] & 0x3F);
	}

	*di += w;
	return cp;
}

char8_t *
utf8trim(char8_t *s)
{
	rune_t cp;
	size_t i = 0;

	for (size_t j = 0; (cp = utf8iter(s, &j)) && unispace(cp); i = j)
		;
	s += i;
	for (i = 0; (cp = utf8iter(s, &i));) {
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
	rune_t cp;
	size_t i = 0;

	while ((cp = utf8iter(s, &i))) {
		if (!pfn(cp))
			return false;
	}

	return true;
}

char8_t *
utf8skipf(const char8_t *s, bool (*pfn)(rune_t))
{
	rune_t ch;
	size_t i, j;

	for (i = j = 0; (ch = utf8iter(s, &i)); j = i) {
		if (!pfn(ch))
			break;
	}

	return (char8_t *)s + j;
}
