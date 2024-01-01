#include "lexer.h"
#include "da.h"
#include "uni.h"
#include "utf8.h"

static bool in_comment(rune_t);
static bool is_arg_char(rune_t);

void
lexstr(const char *s, struct lextoks *toks)
{
	dainit(toks, 64);

	while (*s) {
		rune_t ch;
		size_t i = 0;
		struct lextok tok;

		s = utf8skipf(s, unispace);
		ch = utf8iter(s, &i);

		/* Comments */
		if (ch == '#') {
			s = utf8skipf(s, in_comment);
			continue;
		}

		tok.p = s;
		s = utf8skipf(s, is_arg_char);
		tok.len = s - tok.p;
		tok.kind = LTK_ARG;
		dapush(toks, tok);
	}
}

bool
in_comment(rune_t ch)
{
	return ch != '\n' && ch != '\0';
}

bool
is_arg_char(rune_t ch)
{
	return !unispace(ch);
}
