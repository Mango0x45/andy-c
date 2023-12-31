#include <stddef.h>
#include <stdint.h>

#include "utf8.h"

rune_t
utf8iter(const char *s, size_t *di)
{
	int w;
	rune_t cp;
	uint8_t b = s[*di];

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

	for (int i = 1; i < w; ++i) {
		if ((s[*di + i] & 0xC0) != 0x80)
			return UNI_REPL_CHAR;
		cp = (cp << 6) | (s[*di + i] & 0x3F);
	}

	*di += w;
	return cp;
}
