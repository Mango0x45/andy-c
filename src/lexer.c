#include "da.h"
#include "lexer.h"
#include "uni.h"
#include "utf8.h"

static bool
in_comment(rune_t ch)
{
	return ch != '\n' && ch != '\0';
}

void
lexstr(const char *s, struct lextoks *toks)
{
	dainit(toks, 64);

	while (*s) {
		rune_t ch;
		size_t i = 0;

		s = utf8skipf(s, unispace);
		ch = utf8iter(s, &i);

		/* Comments */
		if (ch == '#') {
			s = utf8skipf(s, in_comment);
			continue;
		}

		s += i;
	}
}
